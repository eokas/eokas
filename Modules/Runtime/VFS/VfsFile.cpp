#include "VfsFile.h"
#include "VfsChunkStore.h"
#include "VfsHash.h"
#include "Base/Hash.h"
#include "VfsManifest.h"
#include "VfsPath.h"
#include <cstring>
#include <ctime>
#include <map>
#include <memory>

namespace eokas
{
    struct VfsFile::State
    {
        std::shared_ptr<VfsChunkStore> readStore;
        std::shared_ptr<VfsChunkStore> writeStore;
        VfsManifest manifest;
        VfsManifest baseline;
        bool ownsOld = false;
        bool dirty = false;
        bool materialized = false;
        bool open = false;
        VfsMode mode = VfsMode::Read;
        u64_t pos = 0;
        u32_t cacheIndex = 0;
        std::vector<u8_t> cache;
        bool cacheValid = false;
        struct Piece
        {
            bool resident = false;
            bool zero = false;
            String hash;
            u32_t length = 0;
            std::vector<u8_t> bytes;
        };
        std::vector<Piece> pieces;
        u64_t fileSize = 0;
        u32_t chunkBytes = VfsChunkBytes;
        enum class Phase
        {
            Edit,
            Pinned,
            Manifest,
            Linked
        };
        Phase phase = Phase::Edit;
        bool editedDuringCommit = false;
        VfsManifest fresh;
        std::map<String, std::vector<u8_t>> bodies;
        std::map<String, int> freshCounts;
        std::map<String, int> pinned;
        std::map<String, int> released;
    };

    static void countChunks(const VfsManifest& manifest, std::map<String, int>& counts)
    {
        for (auto& chunk : manifest.chunks)
            counts[chunk.hash] += 1;
    }

    VfsResult VfsFile::spillResident(State& state)
    {
        if (state.writeStore == nullptr)
            return VfsResult::Access;
        for (auto& piece : state.pieces)
        {
            if (!piece.resident)
                continue;
            if (piece.bytes.empty())
            {
                piece.resident = false;
                piece.length = 0;
                continue;
            }
            String hash = vfsHash(piece.bytes.data(), piece.bytes.size());
            VfsResult result = state.writeStore->putChunk(hash, piece.bytes.data(), piece.bytes.size());
            if (result != VfsResult::Ok)
                return result;
            piece.hash = hash;
            piece.length = (u32_t) piece.bytes.size();
            piece.bytes.clear();
            piece.resident = false;
            piece.zero = false;
            state.cacheValid = false;
        }
        return VfsResult::Ok;
    }

    VfsResult VfsFile::ensureResident(State& state, size_t index)
    {
        if (index >= state.pieces.size())
            state.pieces.resize(index + 1);
        if (state.pieces[index].resident)
            return VfsResult::Ok;
        VfsResult spilled = spillResident(state);
        if (spilled != VfsResult::Ok)
            return spilled;
        VfsFile::State::Piece& piece = state.pieces[index];
        if (piece.zero)
        {
            piece.bytes.assign(piece.length, 0);
            piece.zero = false;
            piece.resident = true;
            state.cacheValid = false;
            return VfsResult::Ok;
        }
        if (!piece.hash.isEmpty())
        {
            if (state.readStore == nullptr)
                return VfsResult::Io;
            VfsResult result = state.readStore->getChunk(piece.hash, piece.bytes);
            if (result != VfsResult::Ok)
                return result;
            if (piece.bytes.size() < piece.length)
                return VfsResult::Corrupt;
            piece.bytes.resize(piece.length);
            piece.resident = true;
            state.cacheValid = false;
            return VfsResult::Ok;
        }
        piece.bytes.clear();
        piece.length = 0;
        piece.resident = true;
        state.cacheValid = false;
        return VfsResult::Ok;
    }

    VfsResult VfsFile::growZero(State& state, u64_t target)
    {
        while (state.fileSize < target)
        {
            size_t index = (size_t) (state.fileSize / state.chunkBytes);
            u64_t begin = (u64_t) index * state.chunkBytes;
            u64_t chunkEnd = begin + state.chunkBytes;
            u64_t fillTo = target < chunkEnd ? target : chunkEnd;
            if (index >= state.pieces.size())
                state.pieces.resize(index + 1);
            if (state.fileSize == begin && fillTo == chunkEnd)
            {
                State::Piece& piece = state.pieces[index];
                piece.bytes.clear();
                piece.resident = false;
                piece.zero = true;
                piece.hash = String();
                piece.length = state.chunkBytes;
                state.cacheValid = false;
                state.fileSize = fillTo;
                continue;
            }
            VfsResult ready = ensureResident(state, index);
            if (ready != VfsResult::Ok)
                return ready;
            u32_t newLen = (u32_t) (fillTo - begin);
            if (state.pieces[index].bytes.size() < newLen)
                state.pieces[index].bytes.resize(newLen, 0);
            state.pieces[index].length = (u32_t) state.pieces[index].bytes.size();
            state.pieces[index].zero = false;
            state.fileSize = begin + state.pieces[index].length;
            if (state.pieces[index].length == state.chunkBytes)
            {
                VfsResult spilled = spillResident(state);
                if (spilled != VfsResult::Ok)
                    return spilled;
            }
        }
        return VfsResult::Ok;
    }

    VfsFile::VfsFile()
        : mState(nullptr)
    {
    }

    VfsFile::~VfsFile()
    {
        if (close() != VfsResult::Ok)
            mState.reset();
    }

    VfsFile::VfsFile(VfsFile&& other) noexcept
        : mState(std::move(other.mState))
    {
    }

    VfsFile& VfsFile::operator=(VfsFile&& other) noexcept
    {
        if (this != &other)
        {
            if (close() != VfsResult::Ok)
                mState.reset();
            mState = std::move(other.mState);
        }
        return *this;
    }

    bool VfsFile::isOpen() const
    {
        return mState != nullptr && mState->open;
    }

    u64_t VfsFile::tell() const
    {
        if (!isOpen())
            return 0;
        return mState->pos;
    }

    u64_t VfsFile::size() const
    {
        if (!isOpen())
            return 0;
        return mState->fileSize;
    }

    VfsResult VfsFile::prepare(std::shared_ptr<VfsChunkStore> readStore, std::shared_ptr<VfsChunkStore> writeStore, const VfsManifest& manifest, const VfsManifest* baseline, VfsMode mode, bool materialized, bool dirty, u64_t pos)
    {
        VfsResult closed = close();
        if (closed != VfsResult::Ok)
            return closed;
        mState = std::make_unique<State>();
        mState->readStore = readStore;
        mState->writeStore = writeStore;
        mState->manifest = manifest;
        if (baseline != nullptr)
        {
            mState->baseline = *baseline;
            mState->ownsOld = true;
        }
        mState->mode = mode;
        mState->materialized = materialized;
        mState->dirty = dirty;
        mState->pos = pos;
        mState->open = true;
        mState->chunkBytes = manifest.chunkSize == 0 ? VfsChunkBytes : manifest.chunkSize;
        mState->fileSize = manifest.size;
        for (auto& chunk : manifest.chunks)
        {
            State::Piece piece;
            piece.hash = chunk.hash;
            piece.length = chunk.length;
            mState->pieces.push_back(piece);
        }
        return VfsResult::Ok;
    }

    VfsResult VfsFile::read(void* data, u64_t size, u64_t& count)
    {
        count = 0;
        if (!isOpen())
            return VfsResult::Closed;
        if (size == 0)
            return VfsResult::Ok;
        u64_t fileSize = this->size();
        if (mState->pos >= fileSize)
            return VfsResult::Ok;
        u64_t remain = fileSize - mState->pos;
        if (size < remain)
            remain = size;
        u8_t* dst = static_cast<u8_t*>(data);
        u64_t done = 0;
        while (done < remain)
        {
            size_t index = (size_t) (mState->pos / mState->chunkBytes);
            if (index >= mState->pieces.size())
                return VfsResult::Corrupt;
            State::Piece& piece = mState->pieces[index];
            u64_t inner = mState->pos % mState->chunkBytes;
            const u8_t* src = nullptr;
            u64_t available = 0;
            if (piece.resident)
            {
                if (inner > piece.bytes.size())
                    return VfsResult::Corrupt;
                src = piece.bytes.data();
                available = piece.bytes.size() - inner;
            }
            else if (piece.zero)
            {
                if (inner > piece.length)
                    return VfsResult::Corrupt;
                available = piece.length - inner;
            }
            else
            {
                if (mState->readStore == nullptr)
                    return VfsResult::Io;
                if (!mState->cacheValid || mState->cacheIndex != (u32_t) index)
                {
                    VfsResult result = mState->readStore->getChunk(piece.hash, mState->cache);
                    if (result != VfsResult::Ok)
                        return result;
                    mState->cacheIndex = (u32_t) index;
                    mState->cacheValid = true;
                }
                if (inner > piece.length || piece.length > mState->cache.size())
                    return VfsResult::Corrupt;
                src = mState->cache.data();
                available = piece.length - inner;
            }
            if (available > remain - done)
                available = remain - done;
            if (available == 0)
                return VfsResult::Corrupt;
            if (piece.zero)
                std::memset(dst + done, 0, (size_t) available);
            else
                std::memcpy(dst + done, src + inner, (size_t) available);
            mState->pos += available;
            done += available;
        }
        count = done;
        return VfsResult::Ok;
    }

    VfsResult VfsFile::write(const void* data, u64_t size, u64_t& count)
    {
        count = 0;
        if (!isOpen())
            return VfsResult::Closed;
        if (mState->mode == VfsMode::Read)
            return VfsResult::Access;
        if (mState->phase != State::Phase::Edit || !mState->pinned.empty())
            mState->editedDuringCommit = true;
        mState->dirty = true;
        if (size == 0)
            return VfsResult::Ok;
        if (mState->pos > mState->fileSize)
        {
            VfsResult grown = growZero(*mState, mState->pos);
            if (grown != VfsResult::Ok)
                return grown;
        }
        const u8_t* src = static_cast<const u8_t*>(data);
        u64_t done = 0;
        while (done < size)
        {
            u64_t at = mState->pos + done;
            size_t index = (size_t) (at / mState->chunkBytes);
            u64_t begin = (u64_t) index * mState->chunkBytes;
            u32_t inner = (u32_t) (at - begin);
            u32_t room = mState->chunkBytes - inner;
            u32_t take = room;
            if ((u64_t) take > size - done)
                take = (u32_t) (size - done);
            VfsResult ready = ensureResident(*mState, index);
            if (ready != VfsResult::Ok)
                return ready;
            State::Piece& piece = mState->pieces[index];
            if (piece.bytes.size() < (size_t) inner + take)
                piece.bytes.resize((size_t) inner + take, 0);
            if (src != nullptr)
                std::memcpy(piece.bytes.data() + inner, src + done, take);
            piece.length = (u32_t) piece.bytes.size();
            piece.zero = false;
            u64_t chunkEnd = begin + piece.length;
            if (chunkEnd > mState->fileSize)
                mState->fileSize = chunkEnd;
            if (piece.length == mState->chunkBytes)
            {
                VfsResult spilled = spillResident(*mState);
                if (spilled != VfsResult::Ok)
                    return spilled;
            }
            done += take;
        }
        mState->pos += done;
        count = done;
        return VfsResult::Ok;
    }

    VfsResult VfsFile::seek(i64_t offset, VfsSeek origin)
    {
        if (!isOpen())
            return VfsResult::Closed;
        i64_t target = 0;
        if (origin == VfsSeek::Begin)
            target = offset;
        else if (origin == VfsSeek::Current)
            target = (i64_t) mState->pos + offset;
        else
            target = (i64_t) size() + offset;
        if (target < 0)
            return VfsResult::InvalidPath;
        mState->pos = (u64_t) target;
        return VfsResult::Ok;
    }

    VfsResult VfsFile::flush()
    {
        if (!isOpen())
            return VfsResult::Closed;
        if (!mState->dirty)
            return VfsResult::Ok;
        if (mState->writeStore == nullptr)
            return VfsResult::Access;

        if (mState->phase == State::Phase::Edit && mState->pinned.empty())
        {
            mState->fresh = VfsManifest();
            mState->fresh.kind = VfsKind::File;
            mState->fresh.path = mState->manifest.path;
            mState->fresh.size = mState->fileSize;
            mState->fresh.mtime = (i64_t) std::time(nullptr);
            mState->fresh.chunkSize = mState->chunkBytes;
            mState->bodies.clear();
            mState->freshCounts.clear();
            mState->released.clear();
            SHA256 fileHash;
            fileHash.reset();
            u64_t fed = 0;
            auto feed = [&](const u8_t* data, u32_t length) {
                if (length == 0)
                    return;
                fileHash.update(data, length);
                fed += length;
            };
            std::map<u32_t, String> zeroHashes;
            for (auto& piece : mState->pieces)
            {
                VfsChunkRef ref;
                ref.length = piece.length;
                if (piece.resident)
                {
                    String hash = vfsHash(piece.bytes.empty() ? nullptr : piece.bytes.data(), piece.bytes.size());
                    if (mState->bodies.find(hash) == mState->bodies.end())
                        mState->bodies.emplace(hash, piece.bytes);
                    ref.hash = hash;
                    feed(piece.bytes.empty() ? nullptr : piece.bytes.data(), piece.length);
                }
                else if (piece.zero)
                {
                    auto found = zeroHashes.find(piece.length);
                    if (found == zeroHashes.end())
                    {
                        std::vector<u8_t> zeros(piece.length, 0);
                        String hash = vfsHash(zeros.empty() ? nullptr : zeros.data(), zeros.size());
                        mState->bodies.emplace(hash, std::move(zeros));
                        zeroHashes.emplace(piece.length, hash);
                        ref.hash = hash;
                    }
                    else
                    {
                        ref.hash = found->second;
                    }
                    const std::vector<u8_t>& zeros = mState->bodies[ref.hash];
                    feed(zeros.empty() ? nullptr : zeros.data(), piece.length);
                }
                else
                {
                    if (mState->readStore == nullptr)
                        return VfsResult::Io;
                    std::vector<u8_t> bytes;
                    VfsResult result = mState->readStore->getChunk(piece.hash, bytes);
                    if (result != VfsResult::Ok)
                        return result;
                    if (bytes.size() < piece.length)
                        return VfsResult::Corrupt;
                    ref.hash = piece.hash;
                    feed(bytes.data(), piece.length);
                }
                mState->fresh.chunks.push_back(ref);
                mState->freshCounts[ref.hash] += 1;
            }
            if (fed != mState->fileSize)
                return VfsResult::Corrupt;
            mState->fresh.hash = fileHash.finish();
        }

        std::map<String, int> oldCounts;
        if (mState->ownsOld)
            countChunks(mState->baseline, oldCounts);

        if (mState->phase == State::Phase::Edit)
        {
            for (auto& body : mState->bodies)
            {
                VfsResult result = mState->writeStore->putChunk(body.first, body.second.data(), body.second.size());
                if (result != VfsResult::Ok)
                    return result;
            }
            for (auto& item : mState->freshCounts)
            {
                int oldCount = oldCounts.count(item.first) ? oldCounts[item.first] : 0;
                int need = item.second - oldCount;
                if (need <= 0)
                    continue;
                int already = 0;
                auto found = mState->pinned.find(item.first);
                if (found != mState->pinned.end())
                    already = found->second;
                if (already >= need)
                    continue;
                VfsResult result = mState->writeStore->addRefs(item.first, (u32_t) (need - already));
                if (result != VfsResult::Ok)
                    return result;
                mState->pinned[item.first] = need;
            }
            mState->phase = State::Phase::Pinned;
        }

        if (mState->phase == State::Phase::Pinned)
        {
            VfsResult written = mState->writeStore->writeManifest(mState->fresh);
            if (written != VfsResult::Ok)
                return written;
            mState->phase = State::Phase::Manifest;
        }

        if (mState->phase == State::Phase::Manifest)
        {
            String parent = VfsPath::parent(mState->fresh.path);
            String name = VfsPath::name(mState->fresh.path);
            VfsResult linked = mState->writeStore->upsertChild(parent, name, VfsKind::File);
            if (linked != VfsResult::Ok)
                return linked;
            mState->phase = State::Phase::Linked;
        }

        for (auto& item : oldCounts)
        {
            int freshCount = mState->freshCounts.count(item.first) ? mState->freshCounts[item.first] : 0;
            int need = item.second - freshCount;
            if (need <= 0)
                continue;
            int already = 0;
            auto found = mState->released.find(item.first);
            if (found != mState->released.end())
                already = found->second;
            if (already >= need)
                continue;
            VfsResult result = mState->writeStore->releaseRefs(item.first, (u32_t) (need - already));
            if (result != VfsResult::Ok)
                return result;
            mState->released[item.first] = need;
        }

        mState->manifest = mState->fresh;
        mState->baseline = mState->fresh;
        mState->ownsOld = true;
        mState->cacheValid = false;
        bool again = mState->editedDuringCommit;
        if (!again)
        {
            for (size_t index = 0; index < mState->pieces.size() && index < mState->fresh.chunks.size(); ++index)
            {
                mState->pieces[index].hash = mState->fresh.chunks[index].hash;
                mState->pieces[index].length = mState->fresh.chunks[index].length;
                mState->pieces[index].bytes.clear();
                mState->pieces[index].resident = false;
                mState->pieces[index].zero = false;
            }
            mState->fileSize = mState->fresh.size;
        }
        mState->phase = State::Phase::Edit;
        mState->pinned.clear();
        mState->released.clear();
        mState->bodies.clear();
        mState->freshCounts.clear();
        mState->editedDuringCommit = false;
        mState->dirty = again;
        if (again)
            return flush();
        return VfsResult::Ok;
    }

    VfsResult VfsFile::close()
    {
        if (mState == nullptr)
            return VfsResult::Ok;
        if (!mState->open)
        {
            mState.reset();
            return VfsResult::Ok;
        }
        if (mState->dirty)
        {
            VfsResult result = flush();
            if (result != VfsResult::Ok)
                return result;
        }
        mState.reset();
        return VfsResult::Ok;
    }

    VfsResult VfsFile::readText(String& text)
    {
        if (!isOpen())
            return VfsResult::Closed;
        u64_t saved = mState->pos;
        VfsResult result = seek(0, VfsSeek::Begin);
        if (result != VfsResult::Ok)
            return result;
        u64_t fileSize = size();
        std::vector<u8_t> bytes((size_t) fileSize, 0);
        u64_t count = 0;
        if (fileSize > 0)
        {
            result = read(bytes.data(), fileSize, count);
            if (result != VfsResult::Ok)
                return result;
        }
        mState->pos = saved;
        for (u64_t index = 0; index < count; ++index)
        {
            if (bytes[(size_t) index] == 0)
                return VfsResult::Corrupt;
        }
        std::vector<char> chars((size_t) count + 1, '\0');
        if (count > 0)
            std::memcpy(chars.data(), bytes.data(), (size_t) count);
        text = String(chars.data());
        return VfsResult::Ok;
    }

    VfsResult VfsFile::writeText(const String& text)
    {
        if (!isOpen())
            return VfsResult::Closed;
        if (mState->mode == VfsMode::Read)
            return VfsResult::Access;
        u64_t count = 0;
        return write(text.cstr(), text.length(), count);
    }
}

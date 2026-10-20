#include "../NativeClipboard.h"
#include "NativeInternal.h"

#include <X11/Xatom.h>

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>

namespace eokas
{
    namespace
    {
        struct Slot
        {
            std::mutex mutex;
            std::condition_variable ready;
            int wakeRead = -1;
            int wakeWrite = -1;
            bool open = false;
            bool failed = false;
            bool dirty = false;
            bool owned = false;
            bool hasText = false;
            bool hasBinary = false;
            std::string text;
            std::vector<unsigned char> binary;

            bool claimFinished = false;
            bool fetchPending = false;
            bool fetchDone = false;
            bool fetchOk = false;
            ::Atom fetchTarget = None;
            uint64_t fetchSerial = 0;
            uint64_t fetchDoneSerial = 0;
            std::string fetchBytes;

            ::Atom clipboard = None;
            ::Atom utf8 = None;
            ::Atom targets = None;
            ::Atom binaryType = None;
            ::Atom result = None;
            ::Atom incr = None;
        };

        std::mutex& apiMutex()
        {
            static std::mutex mutex;
            return mutex;
        }

        void wake(Slot& slot)
        {
            if (slot.wakeWrite < 0) {
                return;
            }
            char byte = 1;
            while (true) {
                ssize_t wrote = ::write(slot.wakeWrite, &byte, 1);
                if (wrote >= 0 || errno == EAGAIN || errno == EWOULDBLOCK) {
                    return;
                }
                if (errno != EINTR) {
                    return;
                }
            }
        }

        void drainWake(int fd)
        {
            char buffer[64];
            while (true) {
                ssize_t count = ::read(fd, buffer, sizeof(buffer));
                if (count < 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    return;
                }
                if (count == 0) {
                    return;
                }
            }
        }

        size_t maxPropertyBytes(::Display* display)
        {
            unsigned long units = XMaxRequestSize(display);
            if (units < 64) {
                return 4096;
            }
            unsigned long bytes = units * 4;
            if (bytes > 512) {
                bytes -= 512;
            }
            return static_cast<size_t>(bytes);
        }

        std::string readProperty(::Display* display, ::Window window, ::Atom property, ::Atom& type)
        {
            std::string data;
            type = None;
            long offset = 0;
            while (true) {
                ::Atom actual = None;
                int format = 0;
                unsigned long count = 0;
                unsigned long remaining = 0;
                unsigned char* buffer = nullptr;
                int status = XGetWindowProperty(display, window, property, offset, 1 << 18, False, AnyPropertyType,
                    &actual, &format, &count, &remaining, &buffer);
                if (status != Success) {
                    if (buffer != nullptr) {
                        XFree(buffer);
                    }
                    break;
                }
                if (type == None) {
                    type = actual;
                }
                if (format == 8 && buffer != nullptr && count > 0) {
                    data.append(reinterpret_cast<char*>(buffer), count);
                    offset += static_cast<long>((count + 3) / 4);
                }
                bool more = format == 8 && count > 0 && remaining > 0;
                if (buffer != nullptr) {
                    XFree(buffer);
                }
                if (!more) {
                    break;
                }
            }
            return data;
        }

        std::string latin1FromUtf8(const std::string& utf8)
        {
            std::string latin1;
            latin1.reserve(utf8.size());
            for (size_t index = 0; index < utf8.size();) {
                auto lead = static_cast<unsigned char>(utf8[index]);
                if (lead < 0x80) {
                    latin1.push_back(static_cast<char>(lead));
                    index += 1;
                    continue;
                }
                latin1.push_back('?');
                size_t need = 1;
                if ((lead & 0xE0) == 0xC0) need = 2;
                else if ((lead & 0xF0) == 0xE0) need = 3;
                else if ((lead & 0xF8) == 0xF0) need = 4;
                index += need;
                if (index > utf8.size()) {
                    break;
                }
            }
            return latin1;
        }

        std::string utf8FromLatin1(const std::string& latin1)
        {
            std::string utf8;
            utf8.reserve(latin1.size());
            for (unsigned char unit : latin1) {
                if (unit < 0x80) {
                    utf8.push_back(static_cast<char>(unit));
                } else {
                    utf8.push_back(static_cast<char>(0xC0 | (unit >> 6)));
                    utf8.push_back(static_cast<char>(0x80 | (unit & 0x3F)));
                }
            }
            return utf8;
        }

        void sendNotify(::Display* display, const XSelectionRequestEvent& request, ::Atom property)
        {
            XEvent notify = {};
            notify.xselection.type = SelectionNotify;
            notify.xselection.display = display;
            notify.xselection.requestor = request.requestor;
            notify.xselection.selection = request.selection;
            notify.xselection.target = request.target;
            notify.xselection.property = property;
            notify.xselection.time = request.time;
            XSendEvent(display, request.requestor, False, 0, &notify);
            XFlush(display);
        }

        void serveSelection(::Display* display, Slot* slot, const XSelectionRequestEvent& request)
        {
            ::Atom property = request.property == None ? request.target : request.property;
            std::string text;
            std::vector<unsigned char> binary;
            bool hasText = false;
            bool hasBinary = false;
            {
                std::lock_guard<std::mutex> lock(slot->mutex);
                hasText = slot->hasText;
                hasBinary = slot->hasBinary;
                text = slot->text;
                binary = slot->binary;
            }

            size_t limit = maxPropertyBytes(display);
            bool accepted = false;

            if (request.target == slot->targets) {
                long values[4];
                int count = 0;
                values[count++] = static_cast<long>(slot->targets);
                if (hasText) {
                    values[count++] = static_cast<long>(slot->utf8);
                    values[count++] = static_cast<long>(XA_STRING);
                }
                if (hasBinary) {
                    values[count++] = static_cast<long>(slot->binaryType);
                }
                XChangeProperty(display, request.requestor, property, XA_ATOM, 32, PropModeReplace,
                    reinterpret_cast<unsigned char*>(values), count);
                accepted = true;
            } else if (request.target == slot->utf8 && hasText && text.size() <= limit) {
                XChangeProperty(display, request.requestor, property, slot->utf8, 8, PropModeReplace,
                    reinterpret_cast<const unsigned char*>(text.data()), static_cast<int>(text.size()));
                accepted = true;
            } else if (request.target == XA_STRING && hasText) {
                std::string latin1 = latin1FromUtf8(text);
                if (latin1.size() <= limit) {
                    XChangeProperty(display, request.requestor, property, XA_STRING, 8, PropModeReplace,
                        reinterpret_cast<const unsigned char*>(latin1.data()), static_cast<int>(latin1.size()));
                    accepted = true;
                }
            } else if (request.target == slot->binaryType && hasBinary && binary.size() <= limit) {
                XChangeProperty(display, request.requestor, property, slot->binaryType, 8, PropModeReplace,
                    binary.empty() ? nullptr : binary.data(), static_cast<int>(binary.size()));
                accepted = true;
            }

            sendNotify(display, request, accepted ? property : None);
        }

        struct FetchState
        {
            bool active = false;
            bool incr = false;
            uint64_t serial = 0;
            std::string bytes;
        };

        void finishFetch(Slot* slot, FetchState& fetch, bool ok)
        {
            {
                std::lock_guard<std::mutex> lock(slot->mutex);
                slot->fetchDoneSerial = fetch.serial;
                slot->fetchOk = ok;
                slot->fetchBytes = ok ? fetch.bytes : std::string();
                slot->fetchDone = true;
            }
            fetch.active = false;
            fetch.incr = false;
            fetch.bytes.clear();
            slot->ready.notify_all();
        }

        void threadMain(Slot* slot)
        {
            ::Display* display = XOpenDisplay(nullptr);
            if (display == nullptr) {
                std::lock_guard<std::mutex> lock(slot->mutex);
                slot->failed = true;
                slot->open = true;
                slot->ready.notify_all();
                return;
            }

            int screen = DefaultScreen(display);
            ::Window window = XCreateWindow(display, RootWindow(display, screen), -10, -10, 1, 1, 0,
                CopyFromParent, InputOutput, CopyFromParent, 0, nullptr);
            if (window == None) {
                std::lock_guard<std::mutex> lock(slot->mutex);
                slot->failed = true;
                slot->open = true;
                slot->ready.notify_all();
                return;
            }
            XSelectInput(display, window, PropertyChangeMask);

            slot->clipboard = XInternAtom(display, "CLIPBOARD", False);
            slot->utf8 = XInternAtom(display, "UTF8_STRING", False);
            slot->targets = XInternAtom(display, "TARGETS", False);
            slot->binaryType = XInternAtom(display, "eokas.NativeClipboard.binary", False);
            slot->result = XInternAtom(display, "EOKAS_CLIPBOARD_DATA", False);
            slot->incr = XInternAtom(display, "INCR", False);
            {
                std::lock_guard<std::mutex> lock(slot->mutex);
                slot->open = true;
            }
            slot->ready.notify_all();

            FetchState fetch;
            while (true) {
                bool claim = false;
                bool startFetch = false;
                ::Atom fetchTarget = None;
                uint64_t fetchSerial = 0;
                {
                    std::lock_guard<std::mutex> lock(slot->mutex);
                    if (slot->dirty) {
                        slot->dirty = false;
                        claim = true;
                    }
                    if (slot->fetchPending) {
                        slot->fetchPending = false;
                        startFetch = true;
                        fetchTarget = slot->fetchTarget;
                        fetchSerial = slot->fetchSerial;
                    }
                }

                if (claim) {
                    bool hasText = false;
                    bool hasBinary = false;
                    {
                        std::lock_guard<std::mutex> lock(slot->mutex);
                        hasText = slot->hasText;
                        hasBinary = slot->hasBinary;
                    }
                    if (!hasText && !hasBinary) {
                        XSetSelectionOwner(display, slot->clipboard, None, CurrentTime);
                    } else {
                        XSetSelectionOwner(display, slot->clipboard, window, CurrentTime);
                    }
                    XFlush(display);
                    bool owned = XGetSelectionOwner(display, slot->clipboard) == window;
                    {
                        std::lock_guard<std::mutex> lock(slot->mutex);
                        slot->owned = owned && (hasText || hasBinary);
                        slot->claimFinished = true;
                    }
                    slot->ready.notify_all();
                }

                if (startFetch) {
                    fetch.active = true;
                    fetch.incr = false;
                    fetch.serial = fetchSerial;
                    fetch.bytes.clear();
                    XConvertSelection(display, slot->clipboard, fetchTarget, slot->result, window, CurrentTime);
                    XFlush(display);
                }

                while (XPending(display) > 0) {
                    XEvent event;
                    XNextEvent(display, &event);
                    if (event.type == SelectionRequest) {
                        serveSelection(display, slot, event.xselectionrequest);
                    } else if (event.type == SelectionClear) {
                        {
                            std::lock_guard<std::mutex> lock(slot->mutex);
                            slot->owned = false;
                            slot->hasText = false;
                            slot->hasBinary = false;
                            slot->text.clear();
                            slot->binary.clear();
                        }
                        slot->ready.notify_all();
                    } else if (event.type == SelectionNotify && fetch.active) {
                        if (event.xselection.property == None) {
                            finishFetch(slot, fetch, false);
                        } else {
                            ::Atom type = None;
                            std::string chunk = readProperty(display, window, event.xselection.property, type);
                            XDeleteProperty(display, window, event.xselection.property);
                            XFlush(display);
                            if (type == slot->incr) {
                                fetch.incr = true;
                                fetch.bytes.clear();
                            } else {
                                fetch.bytes = std::move(chunk);
                                finishFetch(slot, fetch, true);
                            }
                        }
                    } else if (event.type == PropertyNotify && fetch.active && fetch.incr
                        && event.xproperty.atom == slot->result
                        && event.xproperty.state == PropertyNewValue) {
                        ::Atom type = None;
                        std::string chunk = readProperty(display, window, slot->result, type);
                        XDeleteProperty(display, window, slot->result);
                        XFlush(display);
                        if (chunk.empty()) {
                            finishFetch(slot, fetch, true);
                        } else {
                            fetch.bytes.append(chunk);
                        }
                    }
                }

                XFlush(display);
                int xfd = ConnectionNumber(display);
                fd_set reads;
                FD_ZERO(&reads);
                FD_SET(xfd, &reads);
                FD_SET(slot->wakeRead, &reads);
                int maxFd = xfd > slot->wakeRead ? xfd : slot->wakeRead;
                int selected = ::select(maxFd + 1, &reads, nullptr, nullptr, nullptr);
                if (selected < 0 && errno == EINTR) {
                    continue;
                }
                drainWake(slot->wakeRead);
            }
        }

        Slot& clipboard()
        {
            static Slot slot;
            static std::once_flag once;
            std::call_once(once, [] {
                ensureXThreads();
                int fds[2] = { -1, -1 };
                if (pipe2(fds, O_CLOEXEC | O_NONBLOCK) != 0) {
                    slot.failed = true;
                    slot.open = true;
                    return;
                }
                slot.wakeRead = fds[0];
                slot.wakeWrite = fds[1];
                std::thread(threadMain, &slot).detach();
                std::unique_lock<std::mutex> lock(slot.mutex);
                slot.ready.wait(lock, [&] { return slot.open; });
            });
            return slot;
        }

        bool claim(Slot& slot)
        {
            {
                std::lock_guard<std::mutex> lock(slot.mutex);
                if (slot.failed) {
                    return false;
                }
                slot.claimFinished = false;
                slot.dirty = true;
                slot.owned = false;
            }
            wake(slot);
            std::unique_lock<std::mutex> lock(slot.mutex);
            slot.ready.wait(lock, [&] { return slot.claimFinished || slot.failed; });
            return !slot.failed && slot.owned;
        }

        bool fetch(Slot& slot, ::Atom target, std::string& bytes)
        {
            uint64_t serial = 0;
            {
                std::lock_guard<std::mutex> lock(slot.mutex);
                if (slot.failed || target == None) {
                    return false;
                }
                serial = ++slot.fetchSerial;
                slot.fetchTarget = target;
                slot.fetchPending = true;
                slot.fetchDone = false;
                slot.fetchOk = false;
                slot.fetchBytes.clear();
            }
            wake(slot);
            std::unique_lock<std::mutex> lock(slot.mutex);
            bool signaled = slot.ready.wait_for(lock, std::chrono::seconds(2), [&] {
                return slot.fetchDone && slot.fetchDoneSerial == serial;
            });
            if (!signaled || !slot.fetchOk) {
                return false;
            }
            bytes = std::move(slot.fetchBytes);
            return true;
        }
    }

    bool NativeClipboard::writeText(const String& text)
    {
        std::lock_guard<std::mutex> apiLock(apiMutex());
        Slot& slot = clipboard();
        if (slot.failed) {
            return false;
        }
        {
            std::lock_guard<std::mutex> lock(slot.mutex);
            slot.hasBinary = false;
            slot.binary.clear();
            if (text.isEmpty()) {
                slot.hasText = false;
                slot.text.clear();
            } else {
                const char* utf8 = text.cstr();
                if (utf8 == nullptr) {
                    return false;
                }
                slot.text = utf8;
                slot.hasText = true;
            }
        }
        if (text.isEmpty()) {
            claim(slot);
            return true;
        }
        return claim(slot);
    }

    bool NativeClipboard::readText(String& text)
    {
        text.clear();
        std::lock_guard<std::mutex> apiLock(apiMutex());
        Slot& slot = clipboard();
        if (slot.failed) {
            return false;
        }
        {
            std::lock_guard<std::mutex> lock(slot.mutex);
            if (slot.owned && slot.hasText) {
                text = String(slot.text);
                return true;
            }
        }

        std::string bytes;
        ::Atom utf8 = None;
        ::Atom latin1 = XA_STRING;
        {
            std::lock_guard<std::mutex> lock(slot.mutex);
            utf8 = slot.utf8;
        }
        if (fetch(slot, utf8, bytes)) {
            while (!bytes.empty() && bytes.back() == '\0') {
                bytes.pop_back();
            }
            if (!bytes.empty()) {
                text = String(bytes);
                return true;
            }
        }
        if (!fetch(slot, latin1, bytes)) {
            return false;
        }
        while (!bytes.empty() && bytes.back() == '\0') {
            bytes.pop_back();
        }
        if (bytes.empty()) {
            return false;
        }
        text = String(utf8FromLatin1(bytes));
        return true;
    }

    bool NativeClipboard::writeBinary(const void* data, size_t size)
    {
        if ((data == nullptr && size > 0) || size > 0xFFFFFFFFu) {
            return false;
        }
        std::lock_guard<std::mutex> apiLock(apiMutex());
        Slot& slot = clipboard();
        if (slot.failed) {
            return false;
        }
        {
            std::lock_guard<std::mutex> lock(slot.mutex);
            slot.hasText = false;
            slot.text.clear();
            if (data == nullptr || size == 0) {
                slot.hasBinary = false;
                slot.binary.clear();
            } else {
                auto* bytes = static_cast<const unsigned char*>(data);
                slot.binary.assign(bytes, bytes + size);
                slot.hasBinary = true;
            }
        }
        if (data == nullptr || size == 0) {
            claim(slot);
            return true;
        }
        return claim(slot);
    }

    bool NativeClipboard::readBinary(void* data, size_t capacity, size_t& size)
    {
        size = 0;
        std::lock_guard<std::mutex> apiLock(apiMutex());
        Slot& slot = clipboard();
        if (slot.failed) {
            return false;
        }
        bool found = false;
        std::vector<unsigned char> payload;
        ::Atom type = None;
        {
            std::lock_guard<std::mutex> lock(slot.mutex);
            type = slot.binaryType;
            if (slot.owned && slot.hasBinary) {
                payload = slot.binary;
                found = true;
            }
        }
        if (!found) {
            std::string bytes;
            if (!fetch(slot, type, bytes) || bytes.empty()) {
                return false;
            }
            payload.assign(bytes.begin(), bytes.end());
        }
        size = payload.size();
        if (data == nullptr || capacity < size) {
            return false;
        }
        if (size > 0) {
            std::memcpy(data, payload.data(), size);
        }
        return true;
    }
}

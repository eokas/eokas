
#include "./hash.h"
#include "./string.h"
#include <cstring>

namespace eokas {
/**
 * =================================================================
 * MD5
 * =================================================================
 */
    // Constants for MD5Transform routine.
#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21


    // F, G, H and I are basic MD5 functions.
    inline u32_t MD5::F(u32_t x, u32_t y, u32_t z) {
        return x&y | ~x&z;
    }

    inline u32_t MD5::G(u32_t x, u32_t y, u32_t z) {
        return x&z | y&~z;
    }

    inline u32_t MD5::H(u32_t x, u32_t y, u32_t z) {
        return x^y^z;
    }

    inline u32_t MD5::I(u32_t x, u32_t y, u32_t z) {
        return y ^ (x | ~z);
    }

    // rotate_left rotates x left n bits.
    inline u32_t MD5::rotate_left(u32_t x, int n) {
        return (x << n) | (x >> (32-n));
    }

    // FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
    // Rotation is separate from addition to prevent recomputation.
    inline void MD5::FF(u32_t &a, u32_t b, u32_t c, u32_t d, u32_t x, u32_t s, u32_t ac) {
        a = rotate_left(a+ F(b,c,d) + x + ac, s) + b;
    }

    inline void MD5::GG(u32_t &a, u32_t b, u32_t c, u32_t d, u32_t x, u32_t s, u32_t ac) {
        a = rotate_left(a + G(b,c,d) + x + ac, s) + b;
    }

    inline void MD5::HH(u32_t &a, u32_t b, u32_t c, u32_t d, u32_t x, u32_t s, u32_t ac) {
        a = rotate_left(a + H(b,c,d) + x + ac, s) + b;
    }

    inline void MD5::II(u32_t &a, u32_t b, u32_t c, u32_t d, u32_t x, u32_t s, u32_t ac) {
        a = rotate_left(a + I(b,c,d) + x + ac, s) + b;
    }

    // encodes input (u32) into output (u8). Assumes len is
    // a multiple of 4.
    void MD5::encode(u8_t* output, const u32_t* input, u32_t len)
    {
        for (u32_t i = 0, j = 0; j < len; i++, j += 4) {
            output[j] = input[i] & 0xff;
            output[j+1] = (input[i] >> 8) & 0xff;
            output[j+2] = (input[i] >> 16) & 0xff;
            output[j+3] = (input[i] >> 24) & 0xff;
        }
    }

    // decodes input (u8) into output (u32). Assumes len is a multiple of 4.
    void MD5::decode(u32_t* output, const u8_t* input, u32_t len)
    {
        for (u32_t i = 0, j = 0; j < len; i++, j += 4) {
            u32_t a = (u32_t) input[j];
            u32_t b = ((u32_t) input[j + 1]) << 8;
            u32_t c = ((u32_t) input[j + 2]) << 16;
            u32_t d = ((u32_t) input[j + 3]) << 24;
            output[i] = a | b | c | d;
        }
    }

    MD5::MD5()
    { }

    String MD5::compute(const String& input)
    {
        u8_t digest[DIGEST_SIZE];
        memset(digest, 0, DIGEST_SIZE);

        this->init();
        this->update((const u8_t*)input.cstr(), (u32_t)input.length());
        this->finalize(digest);

        char buf[DIGEST_SIZE * 2 + 1];
        for (int i = 0; i < DIGEST_SIZE; i++)
        {
            // sprintf(buf + i * 2, "%02x", digest[i]);
            snprintf(buf + i * 2, 2, "%02x", digest[i]);
        }
        buf[DIGEST_SIZE * 2] = 0;

        return String{buf};
    }

    void MD5::init()
    {
        count[0] = 0;
        count[1] = 0;

        // load magic initialization constants.
        state[0] = 0x67452301;
        state[1] = 0xefcdab89;
        state[2] = 0x98badcfe;
        state[3] = 0x10325476;
    }

    // apply MD5 algo on a block
    void MD5::transform(const u8_t block[blocksize]) {
        u32_t a = state[0], b = state[1], c = state[2], d = state[3], x[16];
        decode(x, block, blocksize);

        /* Round 1 */
        FF(a, b, c, d, x[0], S11, 0xd76aa478); /* 1 */
        FF(d, a, b, c, x[1], S12, 0xe8c7b756); /* 2 */
        FF(c, d, a, b, x[2], S13, 0x242070db); /* 3 */
        FF(b, c, d, a, x[3], S14, 0xc1bdceee); /* 4 */
        FF(a, b, c, d, x[4], S11, 0xf57c0faf); /* 5 */
        FF(d, a, b, c, x[5], S12, 0x4787c62a); /* 6 */
        FF(c, d, a, b, x[6], S13, 0xa8304613); /* 7 */
        FF(b, c, d, a, x[7], S14, 0xfd469501); /* 8 */
        FF(a, b, c, d, x[8], S11, 0x698098d8); /* 9 */
        FF(d, a, b, c, x[9], S12, 0x8b44f7af); /* 10 */
        FF(c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
        FF(b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
        FF(a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
        FF(d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
        FF(c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
        FF(b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

        /* Round 2 */
        GG(a, b, c, d, x[1], S21, 0xf61e2562); /* 17 */
        GG(d, a, b, c, x[6], S22, 0xc040b340); /* 18 */
        GG(c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
        GG(b, c, d, a, x[0], S24, 0xe9b6c7aa); /* 20 */
        GG(a, b, c, d, x[5], S21, 0xd62f105d); /* 21 */
        GG(d, a, b, c, x[10], S22, 0x2441453); /* 22 */
        GG(c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
        GG(b, c, d, a, x[4], S24, 0xe7d3fbc8); /* 24 */
        GG(a, b, c, d, x[9], S21, 0x21e1cde6); /* 25 */
        GG(d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
        GG(c, d, a, b, x[3], S23, 0xf4d50d87); /* 27 */
        GG(b, c, d, a, x[8], S24, 0x455a14ed); /* 28 */
        GG(a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
        GG(d, a, b, c, x[2], S22, 0xfcefa3f8); /* 30 */
        GG(c, d, a, b, x[7], S23, 0x676f02d9); /* 31 */
        GG(b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

        /* Round 3 */
        HH(a, b, c, d, x[5], S31, 0xfffa3942); /* 33 */
        HH(d, a, b, c, x[8], S32, 0x8771f681); /* 34 */
        HH(c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
        HH(b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
        HH(a, b, c, d, x[1], S31, 0xa4beea44); /* 37 */
        HH(d, a, b, c, x[4], S32, 0x4bdecfa9); /* 38 */
        HH(c, d, a, b, x[7], S33, 0xf6bb4b60); /* 39 */
        HH(b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
        HH(a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
        HH(d, a, b, c, x[0], S32, 0xeaa127fa); /* 42 */
        HH(c, d, a, b, x[3], S33, 0xd4ef3085); /* 43 */
        HH(b, c, d, a, x[6], S34, 0x4881d05); /* 44 */
        HH(a, b, c, d, x[9], S31, 0xd9d4d039); /* 45 */
        HH(d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
        HH(c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
        HH(b, c, d, a, x[2], S34, 0xc4ac5665); /* 48 */

        /* Round 4 */
        II(a, b, c, d, x[0], S41, 0xf4292244); /* 49 */
        II(d, a, b, c, x[7], S42, 0x432aff97); /* 50 */
        II(c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
        II(b, c, d, a, x[5], S44, 0xfc93a039); /* 52 */
        II(a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
        II(d, a, b, c, x[3], S42, 0x8f0ccc92); /* 54 */
        II(c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
        II(b, c, d, a, x[1], S44, 0x85845dd1); /* 56 */
        II(a, b, c, d, x[8], S41, 0x6fa87e4f); /* 57 */
        II(d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
        II(c, d, a, b, x[6], S43, 0xa3014314); /* 59 */
        II(b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
        II(a, b, c, d, x[4], S41, 0xf7537e82); /* 61 */
        II(d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
        II(c, d, a, b, x[2], S43, 0x2ad7d2bb); /* 63 */
        II(b, c, d, a, x[9], S44, 0xeb86d391); /* 64 */

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;

        // Zeroize sensitive information.
        memset(x, 0, sizeof x);
    }

    // MD5 block update operation. Continues an MD5 message-digest
    // operation, processing another message block
    void MD5::update(const u8_t* input, u32_t length)
    {
        // compute number of bytes mod 64
        u32_t index = count[0] / 8 % blocksize;

        // Update number of bits
        if ((count[0] += (length << 3)) < (length << 3))
            count[1]++;
        count[1] += (length >> 29);

        // number of bytes we need to fill in buffer
        u32_t firstpart = 64 - index;

        // transform as many times as possible.
        u32_t offset = 0;
        if (length >= firstpart)
        {
            // fill buffer first, transform
            memcpy(&buffer[index], input, firstpart);
            transform(buffer);

            // transform chunks of blocksize (64 bytes)
            for (offset = firstpart; offset + blocksize <= length; offset += blocksize)
            {
                transform(&input[offset]);
            }

            index = 0;
        }

        // buffer remaining input
        memcpy(&buffer[index], &input[offset], length - offset);
    }

    // MD5 finalization. Ends an MD5 message-digest operation, writing the
    // the message digest and zeroizing the context.
    void MD5::finalize(u8_t* digest) {
        static unsigned char padding[64] = {
            0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        };

        // Save number of bits
        u8_t bits[8];
        encode(bits, count, 8);

        // pad out to 56 mod 64.
        u32_t index = count[0] / 8 % 64;
        u32_t padLen = (index < 56) ? (56 - index) : (120 - index);
        update(padding, padLen);

        // Append length (before padding)
        update(bits, 8);

        // Store state in digest
        encode(digest, state, DIGEST_SIZE);

        // Zeroize sensitive information.
        memset(buffer, 0, sizeof buffer);
        memset(count, 0, sizeof count);
    }

    String md5(const String& input)
    {
        return MD5().compute(input);
    }

/**
 * =================================================================
 * SHA256
 * =================================================================
 * */

#define SHA2_SHFR(x, n)    (x >> n)
#define SHA2_ROTR(x, n)   ((x >> n) | (x << ((sizeof(x) << 3) - n)))
#define SHA2_ROTL(x, n)   ((x << n) | (x >> ((sizeof(x) << 3) - n)))
#define SHA2_CH(x, y, z)  ((x & y) ^ (~x & z))
#define SHA2_MAJ(x, y, z) ((x & y) ^ (x & z) ^ (y & z))
#define SHA256_F1(x) (SHA2_ROTR(x,  2) ^ SHA2_ROTR(x, 13) ^ SHA2_ROTR(x, 22))
#define SHA256_F2(x) (SHA2_ROTR(x,  6) ^ SHA2_ROTR(x, 11) ^ SHA2_ROTR(x, 25))
#define SHA256_F3(x) (SHA2_ROTR(x,  7) ^ SHA2_ROTR(x, 18) ^ SHA2_SHFR(x,  3))
#define SHA256_F4(x) (SHA2_ROTR(x, 17) ^ SHA2_ROTR(x, 19) ^ SHA2_SHFR(x, 10))

#define SHA2_UNPACK32(x, str)                 \
    {                                             \
        *((str) + 3) = (u8_t) ((x)      );       \
        *((str) + 2) = (u8_t) ((x) >>  8);       \
        *((str) + 1) = (u8_t) ((x) >> 16);       \
        *((str) + 0) = (u8_t) ((x) >> 24);       \
    }

#define SHA2_PACK32(str, x)                   \
    {                                             \
        *(x) =   ((u32_t) *((str) + 3)      )    \
               | ((u32_t) *((str) + 2) <<  8)    \
               | ((u32_t) *((str) + 1) << 16)    \
               | ((u32_t) *((str) + 0) << 24);   \
    }

    const u32_t SHA256::sha256_k[64] = //UL = u32_t
    {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    SHA256::SHA256()
    { }

    String SHA256::compute(const eokas::String& input)
    {
        u8_t digest[SHA256::DIGEST_SIZE];
        memset(digest,0,SHA256::DIGEST_SIZE);

        SHA256 ctx = SHA256();
        ctx.init();
        ctx.update((const u8_t*)input.cstr(), (u32_t)input.length());
        ctx.finalize(digest);

        char buf[SHA256::DIGEST_SIZE * 2 + 1];
        for (int i = 0; i < SHA256::DIGEST_SIZE; i++)
        {
            // sprintf(buf + i * 2, "%02x", digest[i]);
            snprintf(buf + i * 2, 2, "%02x", digest[i]);
        }
        buf[SHA256::DIGEST_SIZE * 2] = 0;

        return String(buf);
    }

    void SHA256::init()
    {
        m_h[0] = 0x6a09e667;
        m_h[1] = 0xbb67ae85;
        m_h[2] = 0x3c6ef372;
        m_h[3] = 0xa54ff53a;
        m_h[4] = 0x510e527f;
        m_h[5] = 0x9b05688c;
        m_h[6] = 0x1f83d9ab;
        m_h[7] = 0x5be0cd19;
        m_len = 0;
        m_tot_len = 0;
    }

    void SHA256::transform(const u8_t* message, u32_t block_nb)
    {
        u32_t w[64];
        u32_t wv[8];
        for (int i = 0; i < (int) block_nb; i++) {
            const u8_t* sub_block = message + (i << 6);
            for (int j = 0; j < 16; j++) {
                SHA2_PACK32(&sub_block[j << 2], &w[j]);
            }
            for (int j = 16; j < 64; j++) {
                w[j] =  SHA256_F4(w[j -  2]) + w[j -  7] + SHA256_F3(w[j - 15]) + w[j - 16];
            }
            for (int j = 0; j < 8; j++) {
                wv[j] = m_h[j];
            }
            for (int j = 0; j < 64; j++) {
                u32_t t1 = wv[7] + SHA256_F2(wv[4]) + SHA2_CH(wv[4], wv[5], wv[6]) + sha256_k[j] + w[j];
                u32_t t2 = SHA256_F1(wv[0]) + SHA2_MAJ(wv[0], wv[1], wv[2]);
                wv[7] = wv[6];
                wv[6] = wv[5];
                wv[5] = wv[4];
                wv[4] = wv[3] + t1;
                wv[3] = wv[2];
                wv[2] = wv[1];
                wv[1] = wv[0];
                wv[0] = t1 + t2;
            }
            for (int j = 0; j < 8; j++) {
                m_h[j] += wv[j];
            }
        }
    }

    void SHA256::update(const u8_t* message, u32_t len)
    {
        u32_t tmp_len = SHA224_256_BLOCK_SIZE - m_len;
        u32_t rem_len = len < tmp_len ? len : tmp_len;
        memcpy(&m_block[m_len], message, rem_len);
        if (m_len + len < SHA224_256_BLOCK_SIZE) {
            m_len += len;
            return;
        }

        u32_t new_len = len - rem_len;
        u32_t block_nb = new_len / SHA224_256_BLOCK_SIZE;
        const u8_t* shifted_message = message + rem_len;
        transform(m_block, 1);
        transform(shifted_message, block_nb);
        rem_len = new_len % SHA224_256_BLOCK_SIZE;
        memcpy(m_block, &shifted_message[block_nb << 6], rem_len);
        m_len = rem_len;
        m_tot_len += (block_nb + 1) << 6;
    }

    void SHA256::finalize(u8_t* digest)
    {
        u32_t block_nb = (1 + ((SHA224_256_BLOCK_SIZE - 9) < (m_len % SHA224_256_BLOCK_SIZE)));
        u32_t len_b = (m_tot_len + m_len) << 3;
        u32_t pm_len = block_nb << 6;
        memset(m_block + m_len, 0, pm_len - m_len);
        m_block[m_len] = 0x80;
        SHA2_UNPACK32(len_b, m_block + pm_len - 4);
        transform(m_block, block_nb);
        for (int i = 0 ; i < 8; i++) {
            SHA2_UNPACK32(m_h[i], &digest[i << 2]);
        }
    }

    String sha256(const String& input)
    {
        return SHA256().compute(input);
    }
}

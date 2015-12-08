#include "Sha1.h"
#include <Util.h>

//----------------------------
#if 1

C_sha1::C_sha1():
   lenW(0),
   sizeHi(0),
   sizeLo(0)
{
   H[0] = 0x67452301;
   H[1] = 0xEFCDAB89;
   H[2] = 0x98BADCFE;
   H[3] = 0x10325476;
   H[4] = 0xC3D2E1F0;

   MemSet(W, 0, 80);
}

//----------------------------
#define SHA1_ROTL(X,n) ((((X) << (n)) | ((X) >> (32-(n)))) & 0xFFFFFFFF)

void C_sha1::HashBlock(){

   int i;
   dword A, B, C, D, E, T;
   for(i = 16; i < 80; i++) {
      W[i] = SHA1_ROTL(W[i-3] ^ W[i-8] ^ W[i-14] ^ W[i-16], 1);
   }

   A = H[0];
   B = H[1];
   C = H[2];
   D = H[3];
   E = H[4];

   for(i = 0; i < 20; i++){
      T = (SHA1_ROTL(A, 5) + (((C ^ D) & B) ^ D) + E + W[i] + 0x5A827999) & 0xFFFFFFFF;
      E = D;
      D = C;
      C = SHA1_ROTL(B, 30);
      B = A;
      A = T;
   }

   for(i = 20; i < 40; i++) {
      T = (SHA1_ROTL(A, 5) + (B ^ C ^ D) + E + W[i] + 0x6ED9EBA1) & 0xFFFFFFFF;
      E = D;
      D = C;
      C = SHA1_ROTL(B, 30);
      B = A;
      A = T;
   }

   for(i = 40; i < 60; i++) {
      T = (SHA1_ROTL(A, 5) + ((B & C) | (D & (B | C))) + E + W[i] + 0x8F1BBCDC) & 0xFFFFFFFF;
      E = D;
      D = C;
      C = SHA1_ROTL(B, 30);
      B = A;
      A = T;
   }

   for(i = 60; i < 80; i++) {
      T = (SHA1_ROTL(A, 5) + (B ^ C ^ D) + E + W[i] + 0xCA62C1D6) & 0xFFFFFFFF;
      E = D;
      D = C;
      C = SHA1_ROTL(B, 30);
      B = A;
      A = T;
   }
   H[0] += A;
   H[1] += B;
   H[2] += C;
   H[3] += D;
   H[4] += E;
}

//----------------------------

void C_sha1::Update(const byte *input, dword length){

   for(dword i=0; i<length; i++){
      W[lenW / 4] <<= 8;
      W[lenW / 4] |= input[i];

      if((++lenW) % 64 == 0){
         HashBlock();
         lenW = 0;
      }
      sizeLo += 8;
      sizeHi += (sizeLo < 8);
   }
}

//----------------------------

void C_sha1::Finish(byte digest[20]){

   byte pad0x80 = 0x80, pad0x00 = 0;
   byte padlen[8];

   padlen[0] = (byte)((sizeHi >> 24) & 255);
   padlen[1] = (byte)((sizeHi >> 16) & 255);
   padlen[2] = (byte)((sizeHi >> 8) & 255);
   padlen[3] = (byte)((sizeHi >> 0) & 255);
   padlen[4] = (byte)((sizeLo >> 24) & 255);
   padlen[5] = (byte)((sizeLo >> 16) & 255);
   padlen[6] = (byte)((sizeLo >> 8) & 255);
   padlen[7] = (byte)((sizeLo >> 0) & 255);

                              //pad with a 1, then zeroes, then length
   Update(&pad0x80, 1);
   while(lenW != 56)
      Update(&pad0x00, 1);
   Update(padlen, 8);

   for(int i=0; i<20; i++){
      digest[i] = (byte)(H[i / 4] >> 24);
      H[i/4] <<= 8;
   }
}

//----------------------------

static word MakeHexString(byte b){
   dword lo = b&0xf;
   dword hi = b>>4;
   dword ret = ((lo<10) ? (lo+'0') : (lo+'a'-10)) << 8;
   ret |= (hi<10) ? (hi+'0') : (hi+'a'-10);
   return (word)ret;
}

//----------------------------

void C_sha1::FinishToString(char buf[41]){

   byte digest[20];
   Finish(digest);
   for(int i=0; i<20; i++){
      word w = MakeHexString(digest[i]);
      buf[i*2+0] = char(w&0xff);
      buf[i*2+1] = char(w>>8);
   }
   buf[40] = 0;
}

//----------------------------

#else//----------------------------

C_sha1::C_sha1(){
   total[0] = 0;
   total[1] = 0;

   state[0] = 0x67452301;
   state[1] = 0xefcdab89;
   state[2] = 0x98badcfe;
   state[3] = 0x10325476;
   state[4] = 0xc3d2e1f0;
}

//----------------------------

#define GET_UINT32_BE(n, b, i) { \
    (n) = ( (dword) (b)[(i) ] << 24 ) | ( (dword) (b)[(i) + 1] << 16 ) | ( (dword) (b)[(i) + 2] <<  8 ) | ( (dword) (b)[(i) + 3] ); }

#define PUT_UINT32_BE(n,b,i) { \
    (b)[(i)    ] = (byte) ( (n) >> 24 );       \
    (b)[(i) + 1] = (byte) ( (n) >> 16 );       \
    (b)[(i) + 2] = (byte) ( (n) >>  8 );       \
    (b)[(i) + 3] = (byte) ( (n)       );       \
}

//----------------------------

void C_sha1::Process(const byte data[64]){

   dword temp, W[16], A, B, C, D, E;

   GET_UINT32_BE( W[0],  data,  0 );
   GET_UINT32_BE( W[1],  data,  4 );
   GET_UINT32_BE( W[2],  data,  8 );
   GET_UINT32_BE( W[3],  data, 12 );
   GET_UINT32_BE( W[4],  data, 16 );
   GET_UINT32_BE( W[5],  data, 20 );
   GET_UINT32_BE( W[6],  data, 24 );
   GET_UINT32_BE( W[7],  data, 28 );
   GET_UINT32_BE( W[8],  data, 32 );
   GET_UINT32_BE( W[9],  data, 36 );
   GET_UINT32_BE( W[10], data, 40 );
   GET_UINT32_BE( W[11], data, 44 );
   GET_UINT32_BE( W[12], data, 48 );
   GET_UINT32_BE( W[13], data, 52 );
   GET_UINT32_BE( W[14], data, 56 );
   GET_UINT32_BE( W[15], data, 60 );

#define S(x,n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define R(t)                                            \
   (                                                       \
   temp = W[(t -  3) & 0x0F] ^ W[(t - 8) & 0x0F] ^     \
   W[(t - 14) & 0x0F] ^ W[ t      & 0x0F],      \
   ( W[t & 0x0F] = S(temp,1) )                         \
   )

#define P(a,b,c,d,e,x)                                  \
   {                                                       \
   e += S(a,5) + F(b,c,d) + K + x; b = S(b,30);        \
   }

   A = state[0];
   B = state[1];
   C = state[2];
   D = state[3];
   E = state[4];

#define F(x,y,z) (z ^ (x & (y ^ z)))
#define K 0x5A827999

   P( A, B, C, D, E, W[0]  );
   P( E, A, B, C, D, W[1]  );
   P( D, E, A, B, C, W[2]  );
   P( C, D, E, A, B, W[3]  );
   P( B, C, D, E, A, W[4]  );
   P( A, B, C, D, E, W[5]  );
   P( E, A, B, C, D, W[6]  );
   P( D, E, A, B, C, W[7]  );
   P( C, D, E, A, B, W[8]  );
   P( B, C, D, E, A, W[9]  );
   P( A, B, C, D, E, W[10] );
   P( E, A, B, C, D, W[11] );
   P( D, E, A, B, C, W[12] );
   P( C, D, E, A, B, W[13] );
   P( B, C, D, E, A, W[14] );
   P( A, B, C, D, E, W[15] );
   P( E, A, B, C, D, R(16) );
   P( D, E, A, B, C, R(17) );
   P( C, D, E, A, B, R(18) );
   P( B, C, D, E, A, R(19) );

#undef K
#undef F

#define F(x,y,z) (x ^ y ^ z)
#define K 0x6ED9EBA1

   P( A, B, C, D, E, R(20) );
   P( E, A, B, C, D, R(21) );
   P( D, E, A, B, C, R(22) );
   P( C, D, E, A, B, R(23) );
   P( B, C, D, E, A, R(24) );
   P( A, B, C, D, E, R(25) );
   P( E, A, B, C, D, R(26) );
   P( D, E, A, B, C, R(27) );
   P( C, D, E, A, B, R(28) );
   P( B, C, D, E, A, R(29) );
   P( A, B, C, D, E, R(30) );
   P( E, A, B, C, D, R(31) );
   P( D, E, A, B, C, R(32) );
   P( C, D, E, A, B, R(33) );
   P( B, C, D, E, A, R(34) );
   P( A, B, C, D, E, R(35) );
   P( E, A, B, C, D, R(36) );
   P( D, E, A, B, C, R(37) );
   P( C, D, E, A, B, R(38) );
   P( B, C, D, E, A, R(39) );

#undef K
#undef F

#define F(x,y,z) ((x & y) | (z & (x | y)))
#define K 0x8F1BBCDC

   P( A, B, C, D, E, R(40) );
   P( E, A, B, C, D, R(41) );
   P( D, E, A, B, C, R(42) );
   P( C, D, E, A, B, R(43) );
   P( B, C, D, E, A, R(44) );
   P( A, B, C, D, E, R(45) );
   P( E, A, B, C, D, R(46) );
   P( D, E, A, B, C, R(47) );
   P( C, D, E, A, B, R(48) );
   P( B, C, D, E, A, R(49) );
   P( A, B, C, D, E, R(50) );
   P( E, A, B, C, D, R(51) );
   P( D, E, A, B, C, R(52) );
   P( C, D, E, A, B, R(53) );
   P( B, C, D, E, A, R(54) );
   P( A, B, C, D, E, R(55) );
   P( E, A, B, C, D, R(56) );
   P( D, E, A, B, C, R(57) );
   P( C, D, E, A, B, R(58) );
   P( B, C, D, E, A, R(59) );

#undef K
#undef F

#define F(x,y,z) (x ^ y ^ z)
#define K 0xCA62C1D6

   P( A, B, C, D, E, R(60) );
   P( E, A, B, C, D, R(61) );
   P( D, E, A, B, C, R(62) );
   P( C, D, E, A, B, R(63) );
   P( B, C, D, E, A, R(64) );
   P( A, B, C, D, E, R(65) );
   P( E, A, B, C, D, R(66) );
   P( D, E, A, B, C, R(67) );
   P( C, D, E, A, B, R(68) );
   P( B, C, D, E, A, R(69) );
   P( A, B, C, D, E, R(70) );
   P( E, A, B, C, D, R(71) );
   P( D, E, A, B, C, R(72) );
   P( C, D, E, A, B, R(73) );
   P( B, C, D, E, A, R(74) );
   P( A, B, C, D, E, R(75) );
   P( E, A, B, C, D, R(76) );
   P( D, E, A, B, C, R(77) );
   P( C, D, E, A, B, R(78) );
   P( B, C, D, E, A, R(79) );

#undef K
#undef F

   state[0] += A;
   state[1] += B;
   state[2] += C;
   state[3] += D;
   state[4] += E;
}

//----------------------------

void C_sha1::Update(const byte *input, dword length){

   if(!length)
      return;
   dword left = total[0]&0x3f;
   dword fill = 64 - left;

   total[0] += length;
   total[0] &= 0xffffffff;

   if(total[0] < length)
      total[1]++;

   if(left && length >= fill){
      MemCpy(buffer + left, input, fill);
      Process(buffer);
      input += fill;
      length -= fill;
      left = 0;
   }
   while(length >= 64){
      Process(input);
      input += 64;
      length -= 64;
   }
   if(length)
      MemCpy(buffer + left, input, length);
}

//----------------------------

void C_sha1::Finish(byte digest[20]){

   byte msglen[8];

   dword high = (total[0] >> 29) | (total[1] <<  3);
   dword low  = (total[0] << 3);

   PUT_UINT32_BE( high, msglen, 0 );
   PUT_UINT32_BE( low,  msglen, 4 );

   dword last = total[0] & 0x3f;
   dword padn = (last < 56) ? (56 - last) : (120 - last);

   byte sha1_padding[64];
   MemSet(sha1_padding, 0, 64);
   sha1_padding[0] = 0x80;
   Update((byte*)sha1_padding, padn);
   Update(msglen, 8);

   PUT_UINT32_BE(state[0], digest, 0);
   PUT_UINT32_BE(state[1], digest, 4);
   PUT_UINT32_BE(state[2], digest, 8);
   PUT_UINT32_BE(state[3], digest, 12);
   PUT_UINT32_BE(state[4], digest, 16);
}

#endif
//----------------------------

#if 0
/*
 * FIPS-180-1 test vectors
 */
static const char sha1_test_str[3][57] = {
   { "abc" },
   { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" },
   { "" }
};

static const byte sha1_test_sum[3][20] = {
   { 0xA9, 0x99, 0x3E, 0x36, 0x47, 0x06, 0x81, 0x6A, 0xBA, 0x3E, 0x25, 0x71, 0x78, 0x50, 0xC2, 0x6C, 0x9C, 0xD0, 0xD8, 0x9D },
   { 0x84, 0x98, 0x3E, 0x44, 0x1C, 0x3B, 0xD2, 0x6E, 0xBA, 0xAE, 0x4A, 0xA1, 0xF9, 0x51, 0x29, 0xE5, 0xE5, 0x46, 0x70, 0xF1 },
   { 0x34, 0xAA, 0x97, 0x3C, 0xD4, 0xC4, 0xDA, 0xA4, 0xF6, 0x1E, 0xEB, 0x2B, 0xDB, 0xAD, 0x27, 0x31, 0x65, 0x34, 0x01, 0x6F }
};

/*
 * Checkup routine
 */
int sha1_self_test(){
   int i, j;
   byte buf[1000];
   byte sha1sum[20];

   for( i = 0; i < 3; i++ )
   {
      printf( "  SHA-1 test #%d: ", i + 1 );

      C_sha1 ctx;

      if( i < 2 )
         ctx.Update((byte *) sha1_test_str[i],
         strlen( sha1_test_str[i] ) );
      else
      {
         MemSet( buf, 'a', 1000 );
         for( j = 0; j < 1000; j++ )
            ctx.Update(buf, 1000 );
      }

      ctx.Finish(sha1sum );

      if( memcmp( sha1sum, sha1_test_sum[i], 20 ) != 0 )
      {
         printf( "failed\n" );
         return( 1 );
      }

      printf( "passed\n" );
   }

   printf( "\n" );
   return( 0 );
}
#endif

//----------------------------

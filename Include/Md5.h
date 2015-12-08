//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------
// Optimized MD5 implementation conforms to RFC 1321.

#ifndef _MD5_H
#define _MD5_H

#include <Rules.h>

class C_md5{
   void Process(const byte data[64]);
   dword total[2];
   dword state[4];
   byte buffer[64];
public:

   C_md5();
   void Update(const void *input, dword length);
   void Finish(byte digest[16]);
   void FinishToString(char str[33]);
};

#endif

//----------------------------

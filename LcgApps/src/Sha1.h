#ifndef _SHA1_H
#define _SHA1_H

#include <Rules.h>

/*
class C_sha1{
   void Process(const byte data[64]);
   dword total[2];            //number of bytes processed
   dword state[5];            //intermediate digest state
   byte buffer[64];           //data block being processed
public:
   C_sha1();
   void Update(const byte *input, dword length);
   void Finish(byte digest[20]);
};
*/

class C_sha1{
   //void Process(const byte data[64]);
   void HashBlock();

   dword H[5], W[80];
	int lenW;
	dword sizeHi;
	dword sizeLo;
public:
   C_sha1();
   void Update(const byte *input, dword length);
   void Finish(byte digest[20]);
   void FinishToString(char str[41]);

   void SetSizeLo(dword s){ sizeLo = s; }
};


//----------------------------

#endif

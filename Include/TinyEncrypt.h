//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#ifndef _TINY_ENCRYPT_H_
#define _TINY_ENCRYPT_H_

#include <Cstr.h>

//----------------------------

class C_tiny_encrypt{
   dword key[4];

   void BlockEncrypt(dword &_v0, dword &_v1);
   void BlockDecrypt(dword &_v0, dword &_v1);
   static word MakeChecksum(const Cstr_c &text);
public:
   C_tiny_encrypt(const char *crypt_key);

//----------------------------
// Encrypt string with crypt key. Result is base-64 encoded binary data.
   Cstr_c Encrypt(const Cstr_c &text);

//----------------------------
// Decrypt string encrypted with crypt key. Input is base-64 encoded binary data. In case of error, enpty string is returned.
   Cstr_c Decrypt(const Cstr_c &_in);
};

#endif

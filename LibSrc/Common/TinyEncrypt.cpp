//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <TinyEncrypt.h>
#include <C_buffer.h>
#include <C_vector.h>
#include <Base64.h>

//----------------------------

C_tiny_encrypt::C_tiny_encrypt(const char *crypt_key){

                           //make key with padding
   int len = StrLen(crypt_key);
   int sz = 16;
   char *dst = (char*)key;
   while(sz){
      int cl = Min(sz, len);
      MemCpy(dst, crypt_key, cl);
      dst += cl;
      sz -= cl;
   }
}

//----------------------------

void C_tiny_encrypt::BlockEncrypt(dword &_v0, dword &_v1){

   dword v0 = _v0, v1 = _v1, sum = 0;
   for(int i=0; i<32; i++){
      sum -= 0x61c88647;
      v0 += ((v1<<4) + key[0]) ^ (v1 + sum) ^ ((v1>>5) + key[1]);
      v1 += ((v0<<4) + key[2]) ^ (v0 + sum) ^ ((v0>>5) + key[3]);
   }
   _v0 = v0, _v1 = v1;
}

//----------------------------

void C_tiny_encrypt::BlockDecrypt(dword &_v0, dword &_v1){

   dword v0 = _v0, v1 = _v1, sum = 0xc6ef3720;
   for(int i=0; i<32; i++){
      v1 -= (((v0<<4) + key[2]) ^ (v0+sum)) ^ ((v0>>5) + key[3]);
      v0 -= (((v1<<4) + key[0]) ^ (v1+sum)) ^ ((v1>>5) + key[1]);
      sum += 0x61c88647;
   }
   _v0 = v0, _v1 = v1;
}

//----------------------------

word C_tiny_encrypt::MakeChecksum(const Cstr_c &text){
   dword c = 0;
   for(dword i=text.Length(); i--; )
      c += byte(text[i]);
   return c&0xffff;
}

//----------------------------

Cstr_c C_tiny_encrypt::Encrypt(const Cstr_c &text){

                           //pad by ' ' to 8-bytes
   int len = text.Length();
                           //store str len into 1st char
   int buf_len = ((len+4+7) & -8)/4;
   C_buffer<dword> buf;
   buf.Resize(buf_len);
   dword *bp = buf.Begin();
   ((word*)bp)[0] = word(len);
   ((word*)bp)[1] = MakeChecksum(text);
   MemCpy((byte*)bp+4, (const char*)text, len);
                           //pad by '*'
   MemSet((byte*)bp+4+len, '*', buf_len*4-4-len);

   for(int i=0; i<buf_len; i+=2)
      BlockEncrypt(bp[i], bp[i+1]);

   C_vector<char> v;
   EncodeBase64((byte*)bp, buf_len*4, v, false);

   Cstr_c ret;
   ret.Allocate(v.begin(), v.size());
   return ret;
}

//----------------------------

Cstr_c C_tiny_encrypt::Decrypt(const Cstr_c &_in){

   Cstr_c in = _in;
   while(in.Length()&3)
      in<<'=';
   C_vector<byte> buf;
   if(!DecodeBase64(in, in.Length(), buf) || (buf.size()&3))
      return NULL;
   dword *bp = (dword*)buf.begin();
   int buf_len = buf.size()/4;

   for(int i=0; i<buf_len; i+=2)
      BlockDecrypt(bp[i], bp[i+1]);
   dword len = ((word*)bp)[0];
   word csum = ((word*)bp)[1];
   if(len>dword(buf.size()-4))
      return NULL;
   Cstr_c ret;
   ret.Allocate((char*)buf.begin()+4, len);
   if(MakeChecksum(ret)!=csum)
      ret.Clear();
   return ret;
}

//----------------------------

//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#ifndef __CRC_H
#define __CRC_H

//----------------------------

class C_crc{
   dword *crctab;
public:
   C_crc(dword seed = 0xedb88320):
      crctab(new dword[256])
   {
      for(int i=0; i<256; i++){
         dword crc = i;
         for(int j=8; j>0; j--){
            if(crc&1)
               crc = (crc >> 1) ^ seed;
            else
               crc >>= 1;
         }
         crctab[i] = crc;
      }
   }
   ~C_crc(){
      delete[] crctab;
   }
   dword GetCRC32(const byte *data, dword len){

      dword ret = 0xffffffff;
      for(dword i=0; i<len; i++){
         byte b = *data++;
         ret = ((ret>>8) & 0x00ffffff) ^ crctab[(ret^b)&0xff];
      }
      return ret;
   }
};

//----------------------------

#endif

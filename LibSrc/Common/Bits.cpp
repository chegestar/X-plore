// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <Rules.h>

//----------------------------

dword FindHighestBit(dword mask){

   int base = 0;
   if(mask&0xffff0000){
      base = 16;
      mask >>= 16;
   }
   if(mask&0xff00){
      base += 8;
      mask >>= 8;
   }
   if(mask&0xf0){
      base += 4;
      mask >>= 4;
   }
   static const signed char lut[] = {-1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3};
   return base + lut[mask];
}

//----------------------------

dword FindLowestBit(dword mask){

   if(!mask)
      return dword(-1);
   int base = 0;
   if(!(mask&0xffff)){
      base = 16;
      mask >>= 16;
   }
   if(!(mask&0xff)){
      base += 8;
      mask >>= 8;
   }
   if(!(mask & 0xf)){
      base += 4;
      mask >>= 4;
   }
   static const signed char lut[] = {-1, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0};
   return base + lut[mask&15];
}

//----------------------------

dword ReverseBits(dword mask){

   mask = ((mask >>  1) & 0x55555555) | ((mask <<  1) & 0xaaaaaaaa);
   mask = ((mask >>  2) & 0x33333333) | ((mask <<  2) & 0xcccccccc);
   mask = ((mask >>  4) & 0x0f0f0f0f) | ((mask <<  4) & 0xf0f0f0f0);
   mask = ((mask >>  8) & 0x00ff00ff) | ((mask <<  8) & 0xff00ff00);
   mask = ((mask >> 16) & 0x0000ffff) | ((mask << 16) & 0xffff0000) ;
   return mask;
}

//----------------------------

dword CountBits(dword mask){

   mask = (mask&0x55555555) + ((mask&0xaaaaaaaa) >> 1);
   mask = (mask&0x33333333) + ((mask&0xcccccccc) >> 2);
   mask = (mask&0x0f0f0f0f) + ((mask&0xf0f0f0f0) >> 4);
   mask = (mask&0x00ff00ff) + ((mask&0xff00ff00) >> 8);
   mask = (mask&0x0000ffff) + ((mask&0xffff0000) >> 16);
   return mask;
}

//----------------------------

dword RotateLeft(dword mask, int shift){
   return (mask<<shift) | (mask>>(32-shift));
}

//----------------------------

dword RotateRight(dword mask, int shift){

   return (mask>>shift) | (mask<<(32-shift));
}

//----------------------------


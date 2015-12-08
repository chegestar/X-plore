//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <RndGen.h>
#include <Util.h>

//----------------------------

C_rnd_gen::C_rnd_gen(){
   seed[0] = GetTickTime();
}

//----------------------------

void C_rnd_gen::SetSeed(int s){
   seed[0] = s;
}

//----------------------------

dword C_rnd_gen::Get(){
   dword hi = ((seed[0] = seed[0] * 214013l + 2531011l) >> 16) << 16;
   return ((seed[0] = seed[0] * 214013l + 2531011l) >> 16) | hi;
}

//----------------------------

dword C_rnd_gen::Get(dword max){

   switch(max){
   case 0: return Get();
   case 1: return 0;
   }
                                       //setup mask - max. possible number
                                       //with all low bits set
   dword mask = (1 << (FindHighestBit(max - 1) + 1)) - 1;
   if(!mask) mask = 0xffffffff;
   dword val;
   do{
                                       //get 32-bit random number
      val = Get();
                                       //loop until it fits to the max
   }while((val&mask) >= (dword)max);
   return val & mask;
}

//----------------------------

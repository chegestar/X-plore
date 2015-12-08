//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#ifndef __RND_GEN_H
#define __RND_GEN_H

#include <rules.h>

//----------------------------

class C_rnd_gen{
   dword seed[2];
public:
//----------------------------
// Default consturctor, initializing seed by current time.
   C_rnd_gen();

//----------------------------
// Debug constructor, initializing seed to fixed number.
   C_rnd_gen(int _seed){
      SetSeed(_seed);
   }

//----------------------------
// Changing seed.
   void SetSeed(int seed);

//----------------------------
// Get random 32-bit value.
   dword Get();

//----------------------------
// Get random number in range from 0 to max-1.
// If max is 0, full 32-bit number is returned.
   dword Get(dword max);
};

//----------------------------

#endif

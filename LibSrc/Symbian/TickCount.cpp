//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <Rules.h>
#include <e32std.h>
#include <Util.h>
#include <HAL.h>

//----------------------------

dword GetTickTime(){

#if defined __SYMBIAN_3RD__
   dword cnt = User::NTickCount();

#ifdef _WIN32
   cnt *= 5;
#endif

#else//__SYMBIAN_3RD__

   dword cnt = User::TickCount();
#ifdef _WIN32
                              //Wins ticks at 10x per second
   cnt *= 100;
#else//_WIN32
                              //device ticks at 64 per second
   cnt = cnt*1000 / 64;
#endif//!_WIN32
#endif//!__SYMBIAN_3RD__
   return cnt;
}

//----------------------------

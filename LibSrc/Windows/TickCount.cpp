//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

namespace win{
#include <Windows.h>
#include <mmsystem.h>
}
#include <Util.h>

#ifdef _WINDOWS
#pragma comment(lib, "Winmm")
#endif

//----------------------------

dword GetTickTime(){
#ifdef _WINDOWS
   return win::timeGetTime();
#else
   return win::GetTickCount();
#endif
}

//----------------------------
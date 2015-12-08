//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <E32Std.h>
#include <AppBase.h>

//----------------------------

#ifdef __SYMBIAN_3RD__
extern S_global_data *global_data;
S_global_data *global_data;

const S_global_data *GetGlobalData(){
   return global_data;
}

#else

const S_global_data *GetGlobalData(){
   return (S_global_data*)Dll::Tls();
}

#endif

//----------------------------

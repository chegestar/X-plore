//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <Rules.h>

//----------------------------
/*
#undef assert

#ifdef NDEBUG

#define assert(exp) ((void)0)

#else

#define assert(exp) if(!(exp)){\
   const char *__f__ = __FILE__, *__e__ = #exp; int __l__ = __LINE__; __asm push __e__ __asm push __l__ __asm push __f__ __asm push 0x12345678 \
   __asm int 3 \
   __asm add esp, 16 }

#endif
*/
//----------------------------

#pragma comment(lib, "DebugMem.lib")

extern"C"{
void __declspec(dllimport) *__cdecl _DebugAlloc(size_t);
void __declspec(dllimport) __cdecl _DebugFree(void*);
void __declspec(dllimport) *__cdecl _DebugRealloc(void*, size_t);
void __declspec(dllimport) __cdecl _SetBreakAlloc(dword);
void __declspec(dllimport) __cdecl _SetBreakAllocPtr(void*);
void __declspec(dllimport) __cdecl _DumpMemoryLeaks();
void __declspec(dllexport) __cdecl _GetMemInfo(dword &num_blocks, dword &total_size);
void __declspec(dllexport) __cdecl _SetMemAllocLimit(dword bytes);
int force_debug_alloc_include;
}


//----------------------------

void *__cdecl operator new(size_t sz){

   if(!sz)
      return NULL;
   return _DebugAlloc(sz);
}

//----------------------------

void __cdecl operator delete(void *vp){

   if(!vp)
      return;
   _DebugFree(vp);
}

//----------------------------

void *DebugAlloc(size_t sz){
   if(!sz)
      return NULL;
   return _DebugAlloc(sz);
}

//----------------------------

void DebugFree(void *vp){

   if(!vp)
      return;
   _DebugFree(vp);
}

//----------------------------

void *DebugRealloc(void *vp, size_t sz){

   if(!vp)
      return DebugAlloc(sz);
   if(!sz){
      DebugFree(vp);
      return NULL;
   }
   return _DebugRealloc(vp, sz);
}

//----------------------------

void SetBreakAlloc(dword id){
   _SetBreakAlloc(id);
}

//----------------------------

void SetBreakAllocPtr(void *vp){
   _SetBreakAllocPtr(vp);
}

//----------------------------

void DumpMemoryLeaks(){
   _DumpMemoryLeaks();
}

//----------------------------

void GetMemInfo(dword &num_blocks, dword &total_size){
   _GetMemInfo(num_blocks, total_size);
}

//----------------------------

void SetMemAllocLimit(dword bytes){
   _SetMemAllocLimit(bytes);
}

//----------------------------

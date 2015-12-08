//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <Rules.h>
#include <Util.h>

//----------------------------

#if !defined BADA && !defined __SYMBIAN32__
#include <Memory.h>
#endif
#ifdef ANDROID
#pragma GCC diagnostic ignored "-Wshadow"
#endif

#include <String.h>

#ifdef LINUX
#include <wchar.h>
#endif

//----------------------------

void MemSet(void *dst, byte c, dword len){
   memset(dst, c, len);
}

//----------------------------

void MemCpy(void *dst, const void *src, dword len){
   memcpy(dst, src, len);
}

//----------------------------

int MemCmp(const void *mem1, const void *mem2, dword len){
   return memcmp(mem1, mem2, len);
}

//----------------------------

dword StrLen(const char *cp){
   return strlen(cp);
}

//----------------------------

dword StrLen(const wchar *wp){
#if defined BADA || defined ANDROID
   dword r = 0;
   while(*wp++)
      ++r;
   return r;
#else
   return wcslen(wp);
#endif
}

//----------------------------

void *operator new(size_t sz, bool){
#if !(defined BADA || defined ANDROID)
   if(!sz) return NULL;
#endif
   void *vp = new byte[sz];
   if(!vp){
                              //todo: fatal error
      Fatal("Failed to alloc memory", sz);
   }
#ifdef _DEBUG
   //MemSet(vp, 0xff, sz);
#endif
   return vp;
}

//----------------------------
#ifdef _WIN32_WCE
//----------------------------
namespace win{
#include <Windows.h>
#include <Tlhelp32.h>
}
#pragma comment(lib, "toolhelp.lib")

void KillDebugger(){

   win::PROCESSENTRY32 pe;
   pe.dwSize = sizeof(pe);
   win::HANDLE h_snap = win::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
   if(win::Process32First(h_snap, &pe)){
      do{
         const wchar *n = pe.szExeFile;
         if(wcslen(n)==7 && tolower(n[1])=='d' && tolower(n[3])=='.' && tolower(n[5])=='x' && tolower(n[0])=='e' && tolower(n[2])=='m' && tolower(n[4])=='e' && tolower(n[6])=='e'){

            win::HANDLE h = win::OpenProcess(PROCESS_TERMINATE, false, pe.th32ProcessID);
            if(h){
               win::TerminateProcess(h, 0);
               win::CloseHandle(h);
               //exit(0);
            }
            break;
         }
      }while(win::Process32Next(h_snap, &pe));
   }
   win::CloseHandle(h_snap);
}

//----------------------------
#else
//----------------------------
#if defined _DEBUG && !defined __CW32__

void *operator new[](size_t sz){
   return operator new(sz);
}

//----------------------------

void operator delete[](void *vp)
#ifdef __MWERKS__
   throw()
#endif
{
   operator delete(vp);
}
#endif

#endif
//----------------------------

void operator delete(void *vp, bool){
   operator delete(vp);
}

//----------------------------

dword StrCpy(char *dst, const char *src){

   dword len = 0;
   do{
      *dst++ = *src;
      ++len;
   }while(*src++);
   return len - 1;
}

//----------------------------

dword StrCpy(wchar *dst, const wchar *src){
   dword len = 0;
   do{
      *dst++ = *src;
      ++len;
   }while(*src++);
   return len - 1;
}

//---------------------------

int StrCmp(const char *_cp1, const char *_cp2){

   const byte *cp1 = (byte*)_cp1;
   const byte *cp2 = (byte*)_cp2;
   while(true){
      dword c1 = *cp1++;
      dword c2 = *cp2++;
      if(c1<c2)
         return -1;
      if(c1>c2)
         return 1;
      if(!c1)
         return 0;
   }
}

//----------------------------

int StrCmp(const wchar *cp1, const wchar *cp2){

   while(true){
      dword c1 = *cp1++;
      dword c2 = *cp2++;
      if(c1<c2)
         return -1;
      if(c1>c2)
         return 1;
      if(!c1)
         return 0;
   }
}

//----------------------------

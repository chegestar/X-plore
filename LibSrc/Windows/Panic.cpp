//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <Rules.h>
#include <Cstr.h>
//#include <stdlib.h>
namespace win{
#include <Windows.h>
}
extern"C"{
void __cdecl exit(int _Code);
size_t __cdecl wcslen(const wchar_t *);
wchar_t *__cdecl wcsncpy(wchar_t *, const wchar_t *, size_t);
wchar_t *__cdecl wcscpy(wchar_t *, const wchar_t *);
}

//----------------------------

#ifdef _WINDOWS
#pragma comment(lib, "user32.lib")
#else
#pragma comment(lib, "coredll.lib")
#endif

//----------------------------

static void MakeHexCode(dword code, char buf[11]){

   *buf++ = '0';
   *buf++ = 'x';
   bool zero = true;
   for(int i=8; i--; ){
      dword c = (code>>(i*4))&0xf;
      if(!c && zero)
         continue;
      zero = false;
      c += c<10 ? '0' : 'a'-10;
      *buf++ = char(c);
   }
   *buf = 0;
}

//----------------------------

static void InfoMsg(const wchar *title, const wchar *msg, dword code, dword flags){

   wchar tmp[44+11];
   if(code){
      wcsncpy(tmp, msg, 31);
      int n = wcslen(tmp);
      tmp[n++] = ' ';
      tmp[n++] = '(';
      char buf[11];
      MakeHexCode(code, buf);
      for(int i=0; buf[i]; i++)
         tmp[n++] = buf[i];
      tmp[n++] = ')';
      tmp[n++] = ' ';
      wchar dec[11];
      dec[10] = 0;
      int di = 10;
      while(code){
         dec[--di] = '0' + (code%10);
         code /= 10;
      }
      wcscpy(tmp+n, dec+di);
      //tmp[n] = 0;
      msg = tmp;
   }
   win::MessageBox(NULL, msg, title, MB_OK | flags);
}

//----------------------------

void Info(const char *msg, dword code, bool do_exit){

   Cstr_w s; s.Copy(msg);
   InfoMsg(L"Info", s, code, MB_ICONINFORMATION);
   if(do_exit)
      exit(0);
}

//----------------------------

void Fatal(const char *msg, dword code){

#if !defined _WIN32_WCE && defined _DEBUG
   void SetMemAllocLimit(dword bytes);
   SetMemAllocLimit(0);
#endif
   wchar tmp[32];
   for(int i=0; i<32; i++){
      char c = msg[i];
      tmp[i] = c;
      if(!c)
         break;
   }
   tmp[31] = 0;
   //Cstr_w s; s.Copy(msg);
   InfoMsg(L"Fatal error", tmp, code, MB_ICONERROR);
   exit(1);
}

//----------------------------

void Info(const wchar *msg, dword code, bool do_exit){
   InfoMsg(L"Info", msg, code, MB_ICONINFORMATION);
   if(do_exit)
      exit(0);
}

//----------------------------

void Fatal(const wchar *msg, dword code){

   InfoMsg(L"Fatal error", msg, code, MB_ICONERROR);
   exit(1);
}

//----------------------------

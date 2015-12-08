//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <Util.h>
#include <LogMan.h>
#include <C_file.h>
namespace win{
#include <Windows.h>
}

//----------------------------

#define EOL "\r\n"

//----------------------------

class C_imp{
public:
   win::HANDLE h;
};

//----------------------------

C_logman::C_logman():
   imp(NULL)
{
}

//----------------------------

void C_logman::Close(){
   C_imp *i = (C_imp*)imp;
   if(i){
      if(i->h!=win::HANDLE(-1))
         win::CloseHandle(i->h);
      delete i;
      imp = NULL;
   }
}

//----------------------------

bool C_logman::Open(const wchar *fname, bool replace){

   imp = new C_imp;
   C_imp *i = (C_imp*)imp;
   C_file::MakeSurePathExists(fname);
   Cstr_w fp;
   C_file::GetFullPath(fname, fp);

   if(replace)
      win::DeleteFile(fp);
   i->h = win::CreateFile(fp, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
      OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
   if(i->h==win::HANDLE(-1))
      return false;
   win::SetFilePointer(i->h, 0, 0, FILE_END);
   return true;
}

//----------------------------

void C_logman::AddBinary(const void *buf, dword len){

   C_imp *i = (C_imp*)imp;
   if(i->h!=win::HANDLE(-1)){
      dword wr;
      win::WriteFile(i->h, buf, len, &wr, NULL);
   }
}

//----------------------------

void C_logman::AddInt(int i){
   Cstr_c s; s<<i;
   AddBinary(s, s.Length());
}

//----------------------------

void C_logman::Add(const char *txt, bool eol){

   AddBinary(txt, StrLen(txt));
   if(eol)
      AddBinary(EOL, 2);
}

//----------------------------

C_logman &C_logman::operator()(const char *txt, bool eol){
   AddBinary(txt, StrLen(txt));
   if(eol)
      AddBinary(EOL, 2);
   return *this;
}

C_logman &C_logman::operator()(const char *txt, int num, bool eol){
   AddBinary(txt, StrLen(txt));
   AddInt(num);
   if(eol)
      AddBinary(EOL, 2);
   return *this;
}

C_logman &C_logman::operator()(const char *txt, int num, int num1, bool eol){
   AddBinary(txt, StrLen(txt));
   AddInt(num);
   AddBinary(", ", 2);
   AddInt(num1);
   if(eol)
      AddBinary(EOL, 2);
   return *this;
}

//----------------------------

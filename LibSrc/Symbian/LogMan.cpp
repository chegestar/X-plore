//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <f32file.h>
#include <Rules.h>
#include <LogMan.h>
#include <Util.h>
#include <BaUtils.h>

//----------------------------

#ifdef _WIN32
#define EOL "\r\n"
const int EOL_LEN = 2;
#else
#define EOL "\n"
const int EOL_LEN = 1;
#endif

//----------------------------

class C_imp{
public:
   RFs fs;
   RFile file;
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
      i->file.Close();
      i->fs.Close();
      delete i;
      imp = NULL;
   }
}

//----------------------------

bool C_logman::Open(const wchar *fname, bool replace){

   imp = new(true) C_imp;
   C_imp *i = (C_imp*)imp;
   User::LeaveIfError(i->fs.Connect());
   TPtrC desc((word*)fname, StrLen(fname));
   BaflUtils::EnsurePathExistsL(i->fs, desc);
   TInt err;
   if(replace)
      err = i->file.Replace(i->fs, desc, EFileShareAny | EFileWrite | EFileStreamText);
   else{
      err = i->file.Open(i->fs, desc, EFileShareAny | EFileWrite | EFileStreamText);
      switch(err){
      case KErrNotFound:
         err = i->file.Create(i->fs, desc, EFileShareAny | EFileWrite | EFileStreamText);
         break;
      case KErrNone:
         {
            int pos = 0;
            i->file.Seek(ESeekEnd, pos);
         }
         break;
      }
   }
   return (err==KErrNone);
}

//----------------------------

void C_logman::AddBinary(const void *buf, dword len){
   C_imp *i = (C_imp*)imp;
   i->file.Write(TPtrC8((byte*)buf, len));
   i->file.Flush();
}

//----------------------------

void C_logman::AddInt(int i){
   TBuf8<32> buf;
   buf.Format(_L8("%i%c"), i, 0);
   AddBinary((char*)buf.Ptr(), buf.Length());
}

//----------------------------

void C_logman::Add(const char *txt, bool eol){

   AddBinary(txt, StrLen(txt));
   if(eol)
      AddBinary(EOL, EOL_LEN);
}

//----------------------------

C_logman &C_logman::operator()(const char *txt, bool eol){
   AddBinary(txt, StrLen(txt));
   if(eol)
      AddBinary(EOL, EOL_LEN);
   return *this;
}

C_logman &C_logman::operator()(const char *txt, int num, bool eol){
   AddBinary(txt, StrLen(txt));
   AddInt(num);
   if(eol)
      AddBinary(EOL, EOL_LEN);
   return *this;
}

C_logman &C_logman::operator()(const char *txt, int num, int num1, bool eol){
   AddBinary(txt, StrLen(txt));
   AddInt(num);
   AddBinary(", ", 2);
   AddInt(num1);
   if(eol)
      AddBinary(EOL, EOL_LEN);
   return *this;
}

//----------------------------

#ifdef __SYMBIAN32__
#include <E32Std.h>
#endif
#include "Main.h"

//----------------------------
#ifdef __SYMBIAN32__

//----------------------------
#include <EikEnv.h>

//----------------------------
//----------------------------

#include <ApgCli.h>

//----------------------------
#if defined S60 && defined __SYMBIAN_3RD__
#include <S32File.h>
#include <CntDb.h>
#include <cntitem.h>

static void SymbianOpenContactCard(const TDesC &fn){

   _LIT(title, "Contact card");
   TBool add = CEikonEnv::Static()->QueryWinL(title, _L("Add to contacts?"));
   if(!add)
      return;

   RFs fs;
   fs.Connect();
   RFileReadStream rs;

   if(rs.Open(fs, fn, EFileRead)){
      CEikonEnv::Static()->InfoWinL(_L("Can't open file"), fn);
      fs.Close();
      return;
   }
   struct S_lf: public C_leave_func{
      CContactDatabase *cdb;
      virtual int Run(){
         cdb = CContactDatabase::OpenL();
         return 0;
      }
   } lf;
   lf.cdb = NULL;
   lf.Execute();


   TBool success = false;
   CArrayPtr<CContactItem> *arr = lf.cdb->ImportContactsL(TUid::Uid(KUidVCardConvDefaultImpl), rs, success, 0);

   for(int i=arr->Count(); i--; )
      delete (*arr)[i];
   delete arr;

   rs.Close();
   fs.Close();

   delete lf.cdb;
   CEikonEnv::Static()->InfoWinL(title, success ? _L("Contact card added to contacts") : _L("Failed to add to contacts"));
}

//----------------------------
#include <CalSession.h>
#include <CalEntryView.h>
#include <CalDataExchange.h>
#include <CalDataFormat.h>
#include <CalProgressCallback.h>

static void SymbianImportCalendarEntry(const TDesC &fn){

   _LIT(title, "Calendar entry");
   TBool add = CEikonEnv::Static()->QueryWinL(title, _L("Add to calendar?"));
   if(!add)
      return;

   class C_help: public MCalProgressCallBack{
   public:
      CActiveSchedulerWait *asw;
      TInt err;
      virtual void Progress(TInt aPercentageCompleted){}
      virtual TBool NotifyProgress(){ return false; }
      virtual void Completed(TInt aError){ err = aError; asw->AsyncStop(); return; }
   } help;
   help.err = 0;

   CCalSession *cs = CCalSession::NewL();
   cs->OpenL(cs->DefaultFileNameL());
   help.asw = new CActiveSchedulerWait;
   CCalEntryView *cv = CCalEntryView::NewL(*cs, help);
   help.asw->Start();
   delete help.asw;
   //Fatal("herr", help.err);
   if(!help.err){
      RFs fs;
      fs.Connect();

      struct S_lf: public C_leave_func{
         RFileReadStream rs;
         CCalDataExchange *cd;
         RPointerArray<CCalEntry> ea;
         virtual int Run(){
            cd->ImportL(KUidVCalendar, rs, ea);
            return 0;
         }
      } lf;
      int err = lf.rs.Open(fs, fn, EFileRead);
      //Fatal("op err", err);
      if(err){
         CEikonEnv::Static()->InfoWinL(_L("Can't open file"), fn);
      }else{
         lf.cd = CCalDataExchange::NewL(*cs);
         //Fatal("cd", dword(lf.cd));
         int err = lf.Execute();
         //Fatal("ex err", err);
         TInt num = 0;
         if(!err){
            cv->StoreL(lf.ea, num);
            for(int i=lf.ea.Count(); i--; )
               delete lf.ea[i];
            lf.ea.Close();
         }
         delete lf.cd;
         lf.rs.Close();
         //Fatal("!", num);
         CEikonEnv::Static()->InfoWinL(title, num ? _L("Entry was added to calendar") : _L("Failed to add to calendar"));
         //Fatal("!");
      }
      fs.Close();
   }  
   delete cv;
   delete cs;
}

#endif

//----------------------------
#endif //__SYMBIAN32__

#if defined _WINDOWS || defined _WIN32_WCE
#include <String.h>
namespace win{
#include <Windows.h>
}
#endif //_WINDOWS || _WIN32_WCE

//----------------------------

bool C_client::OpenFileBySystem(const wchar *filename, dword app_uid){

#ifdef __SYMBIAN32__

   E_FILE_TYPE ft = DetermineFileType(text_utils::GetExtension(filename));
   if(ft==FILE_TYPE_EXE
#if defined S60 && defined __SYMBIAN_3RD__ && 1
      || ft==FILE_TYPE_CONTACT_CARD || ft==FILE_TYPE_CALENDAR_ENTRY
#endif
      ){
      Cstr_w full_path;
      C_file::GetFullPath(filename, full_path);
      TPtrC fn((word*)(const wchar*)full_path);

      switch(ft){
#if defined S60 && defined __SYMBIAN_3RD__
      case FILE_TYPE_CONTACT_CARD:
         SymbianOpenContactCard(fn);
         return true;
      case FILE_TYPE_CALENDAR_ENTRY:
         SymbianImportCalendarEntry(fn);
         return true;
#endif
      case FILE_TYPE_EXE:
         {
            RApaLsSession ls;
            ls.Connect();
            TBool is_prg;
            bool ok = false;
            if(!ls.IsProgram(fn, is_prg) && is_prg){
               CApaCommandLine *cl = CApaCommandLine::NewL();
#ifdef __SYMBIAN_3RD__
               cl->SetExecutableNameL(fn);
#else
               cl->SetFullCommandLineL(fn);
#endif
               ok = !ls.StartApp(*cl);
               delete cl;
            }
            ls.Close();
            return ok;
         }
         break;
      }
   }
#endif   //__SYMBIAN32__

   return system::OpenFileBySystem(*this, filename, app_uid);
}

//----------------------------

void C_client::StartBrowser(const char *url){
   system::OpenWebBrowser(*this, url);
}

//----------------------------

#ifdef _WIN32_WCE
void C_client::PatchedCodeHackCatch(){
   volatile int a = 0x67452301;
   ++a;
}
#endif

//----------------------------

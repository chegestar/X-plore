#include <E32Std.h>
#include <AppBase.h>
#include <ApgCli.h>
#include <ApaCmdLn.h>
#include <TextUtils.h>
#include <ApgTask.h>
#include <EikEnv.h>
#include <Hal.h>

//----------------------------

void Symbian_User_IMB_Range(void *start, void *end){
   User::IMB_Range(start, end);
}

int Symbian_HAL_Get(dword id, int &val){
   return HAL::Get(HALData::TAttribute(id), val);
}

//----------------------------
namespace system{

bool OpenFileBySystem(C_application_base &app, const wchar *filename, dword app_uid){

   Cstr_w full_path;
   C_file::GetFullPath(filename, full_path);
   TPtrC fn((word*)(const wchar*)full_path);

   int err = 0;
   {
      RApaLsSession ls;
      ls.Connect();
#if !defined __SYMBIAN_3RD__
      //if(!app_uid)
      {
         TThreadId tid;
         err = 1;
         if(!app_uid){
            TRAPD(te, err = ls.StartDocument(fn, tid));
         }else{
            TUid uid;
            uid.iUid = app_uid;
            TRAPD(te, err = ls.StartDocument(fn, uid, tid));
         }
      }//else
//#endif
#else
      {
         TUid uid;
         if(app_uid){
            uid.iUid = app_uid;
            err = 0;
         }else{
#if defined __SYMBIAN_3RD__
            TDataType dt;
            err = ls.AppForDocument(fn, uid, dt);
#endif
         }
         if(!err){
            TApaAppInfo ai;
            err = ls.GetAppInfo(ai, uid);
            if(!err){
               //User::Panic(ai.iFullName, 0);
               CApaCommandLine *cmd = CApaCommandLine::NewL();
#ifdef __SYMBIAN_3RD__
               cmd->SetExecutableNameL(ai.iFullName);
#else
               cmd->SetLibraryNameL(ai.iFullName);
#endif
               cmd->SetDocumentNameL(fn);
               cmd->SetCommandL(EApaCommandOpen);
               err = ls.StartApp(*cmd);
               delete cmd;
            }
         }
      }
#endif
      ls.Close();
   }
   return (!err);
}

//----------------------------

static bool StartBrowser(dword uid, const char *url){

   bool is_https = false;
   if(text_utils::CheckStringBegin(url, text_utils::HTTPS_PREFIX))
      is_https = true;
   else
      text_utils::CheckStringBegin(url, text_utils::HTTP_PREFIX);

   const TUid uid_value = { uid };
   Cstr_w par;
   par.Format(L"4 %#%") <<(!is_https ? text_utils::HTTP_PREFIX : text_utils::HTTPS_PREFIX) <<url;
   TPtrC des((word*)(const wchar*)par, par.Length());

   TApaTaskList tl(CEikonEnv::Static()->WsSession());
   TApaTask task = tl.FindApp(uid_value);
   bool ok = false;

   bool exists = task.Exists();
   //Info("uid", uid);
   if(exists && uid==0x10008d39){
                              //kill web browser
      task.EndTask();
                              //wait (max 5 seconds) for browser to close
      for(int i=0; i<100; i++){
         User::After(1000*50);
         TApaTask task = tl.FindApp(uid_value);
         if(!task.Exists()){
            exists = false;
            break;
         }
      }
   }
   if(exists){
      task.BringToForeground();
      /*
      Cstr_c s8;
      s8.Copy(par);
      TPtrC8 des8((byte*)(const char*)s8, s8.Length());
      task.SendMessage(TUid::Uid(0), des8); // UID not used
      */
      HBufC8 *param = HBufC8::NewL(par.Length() + 2);
      param->Des().Append(des);
      task.SendMessage(TUid::Uid(0), *param); // Uid is not used
      delete param;

      ok = true;
   }else
   {
      RApaLsSession ls;
      if(!ls.Connect()){
         TThreadId tid;
         int err = ls.StartDocument(des, uid_value, tid);
         ls.Close();
         ok = (!err);
      }
   }
   return ok;
}

//----------------------------
void OpenWebBrowser(C_application_base &app, const char *url){

#ifdef __SYMBIAN_3RD__
   /*
   RApaLsSession ls;
   if(!ls.Connect()){
      bool is_https = false;
      if(text_utils::CheckStringBegin(url, text_utils::HTTPS_PREFIX))
         is_https = true;
      else
         text_utils::CheckStringBegin(url, text_utils::HTTP_PREFIX);
      Cstr_w par;
      par.Format(L"4 %#%") <<(!is_https ? text_utils::HTTP_PREFIX : text_utils::HTTPS_PREFIX) <<url;

      TDataType mimeDatatype(_L8("application/x-web-browse"));
      TUid handlerUID;
      ls.AppForDataType(mimeDatatype, handlerUID);
      //Fatal("id", handlerUID.iUid);
      if(!handlerUID.iUid)
         handlerUID = TUid::Uid(0x1020724d);
      text_utils::CheckStringBegin(url, text_utils::HTTP_PREFIX) ||
         text_utils::CheckStringBegin(url, text_utils::HTTPS_PREFIX);

      LaunchBrowserL(TPtrC((word*)(const wchar*)par, par.Length()), handlerUID);
      ls.Close();
      return;
   }
   */
   RApaLsSession ls;
   if(!ls.Connect()){
      TUid hid;
      ls.AppForDataType(_L8("application/x-web-browse"), hid);
      ls.Close();
      //Fatal("uid", hid.iUid);
      if(hid.iUid){
         if(StartBrowser(hid.iUid, url))
            return;
      }
   }

                              //try 'Web' browser
   if(StartBrowser(0x1020724d, url))
      return;
#endif
                              //use 'Services'
   StartBrowser(0x10008d39, url);
}

//----------------------------

bool IsApplicationInstalled(dword uid){
   RApaLsSession ls;
   ls.Connect();
   TApaAppInfo ai;
   TInt err = ls.GetAppInfo(ai, TUid::Uid(uid));
   return (err==KErrNone);
}

//----------------------------
}
//----------------------------

//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <AppBase.h>
#include <Util.h>
#include <TextUtils.h>
namespace win{
#include <Windows.h>
}

//----------------------------

#if defined DEBUG_LOG

void LOG_RUN(const char *msg){
   win::OutputDebugStringA(msg);
   win::OutputDebugStringA("\n");
}

void LOG_RUN_N(const char *msg, int n){
   Cstr_c s; s<<msg <<' ' <<n;
   LOG_RUN(s);
   //__android_log_print(ANDROID_LOG_INFO, "LCG", "%s %i (0x%x)", msg, n, n);
}

void LOG_DEBUG(const char *msg){
   LOG_RUN(msg);
}

void LOG_DEBUG_N(const char *msg, int n){
   LOG_RUN_N(msg, n);
}

#endif

//----------------------------

namespace system{

//----------------------------
#ifdef _WIN32_WCE

bool WM_RunOwnerNameDialog(C_application_base &app){
   win::PROCESS_INFORMATION pi;
   const wchar *name, *cmd;
   if(!IsWMSmartphone()){
      name = L"\\Windows\\ctlpnl.exe";
      cmd = L"cplmain.cpl,2,0";
   }else{
      name = L"\\Windows\\settings.exe";
      cmd = L"owner.cpl.xml";
   }
   win::HWND hwnd = (win::HWND)app.GetIGraph()->GetHWND();
   win::RECT rc;
   win::GetWindowRect(hwnd, &rc);
   win::SetWindowPos(hwnd, NULL, 0, 1000, 0, 0, SWP_NOSIZE);

   if(!win::CreateProcess(name, cmd, NULL, NULL, FALSE, 0, NULL, NULL, NULL, &pi))
      return false;
   while(true){
      dword code;
      if(!win::GetExitCodeProcess(pi.hProcess, &code))
         break;
      if(code!=STILL_ACTIVE)
         break;
      win::Sleep(100);
   }
   win::CloseHandle(pi.hProcess);
   win::CloseHandle(pi.hThread);
   win::SetWindowPos(hwnd, NULL, 0, rc.top, 0, 0, SWP_NOSIZE);
   win::SetActiveWindow(hwnd);
   return true;
}

#endif
//----------------------------

bool OpenFileBySystem(C_application_base &app, const wchar *filename, dword){

   Cstr_w full_path;
   C_file::GetFullPath(filename, full_path);

   win::SHELLEXECUTEINFO si = {
      sizeof(win::SHELLEXECUTEINFO),
      SEE_MASK_FLAG_NO_UI,
      (win::HWND)app.GetIGraph()->GetHWND(),
      L"open",
      full_path,
      NULL,                   //lpParameters
      NULL,                   //lpDirectory
      SW_SHOW,
      NULL,                   //hInstApp
      NULL,                   //lpIDList
      NULL,                   //lpClass
      NULL,                   //hkeyClass
      0,                      //dwHotKey
      NULL,
      NULL                    //hProcess
   };
   return win::ShellExecuteEx(&si);
}

//----------------------------

void OpenWebBrowser(C_application_base &app, const char *url){

   Cstr_w str;
   if(text_utils::CheckStringBegin(url, text_utils::HTTPS_PREFIX))
      str<<text_utils::HTTPS_PREFIX;
   else if(text_utils::CheckStringBegin(url, text_utils::HTTP_PREFIX))
      str<<text_utils::HTTP_PREFIX;
   else if(text_utils::CheckStringBegin(url, "mailto:", true, false)){
   }else if(!text_utils::CheckStringBegin(url, "tel:", true, false))
      str<<text_utils::HTTP_PREFIX;

   str<<url;
   //str.Format(L"%#%") <<(!is_https ? text_utils::HTTP_PREFIX : HTTPS_PREFIX) <<url;
   win::SHELLEXECUTEINFO si = { sizeof(win::SHELLEXECUTEINFO), SEE_MASK_FLAG_NO_UI,
      (win::HWND)app.GetIGraph()->GetHWND(),
      L"open",
      str,
      NULL,                   //lpParameters
      NULL,                   //lpDirectory
      SW_SHOW,
      NULL,                   //hInstApp
      NULL,                   //lpIDList
      NULL,                   //lpClass
      NULL,                   //hkeyClass
      0,                      //dwHotKey
      NULL,
      NULL                    //hProcess
   };
   win::ShellExecuteEx(&si);
}

//----------------------------

bool IsApplicationInstalled(const char *name){
                              //not implemented
   return false;
}

//----------------------------
}


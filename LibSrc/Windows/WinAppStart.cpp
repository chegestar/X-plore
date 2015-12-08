//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <String.h>
namespace win{
#ifdef _WINDOWS
#define _WIN32_WINNT 0x400
#endif
#include <Windows.h>
#ifdef _WINDOWS
//#include <zmouse.h>
#endif
}
#include <AppBase.h>
#include <Util.h>
#include <Cstr.h>
#include <C_vector.h>
#include <TimeDate.h>
#include <Phone.h>

#if (defined WIN32_PLATFORM_PSPC || defined WIN32_PLATFORM_WFSP) && _WIN32_WCE < 0x500
#pragma comment(lib, "ccrtrtti.lib")
#endif

#ifdef _WIN32_WCE
namespace win{
#include <winioctl.h>
extern "C" __declspec(dllimport) BOOL KernelIoControl(DWORD dwIoControlCode, LPVOID lpInBuf, DWORD nInBufSize, LPVOID lpOutBuf, DWORD nOutBufSize, LPDWORD lpBytesReturned);
}
#endif

//----------------------------

namespace system{

dword GetDeviceId(){
   return 0;
}

//----------------------------

void SetAsSystemApp(bool b){
   //win_is_system_app = b;
   //LOG_RUN_N("SetAsSystemApp", b);
}

//----------------------------

void RebootDevice(){
#ifdef _WIN32_WCE
   if(IsWMSmartphone()){
      typedef win::BOOL (*t_ExitWindowsEx)(win::UINT uFlags, win::DWORD dwReason);

      win::HMODULE hi = win::LoadLibraryEx(_T("aygshell.dll"), NULL, 0);
      if(hi){
         t_ExitWindowsEx f = (t_ExitWindowsEx)win::GetProcAddress(hi, L"ExitWindowsEx");
         if(f)
            (*f)(EWX_REBOOT, 0);
         win::FreeLibrary(hi);
      }
   }else{
#define IOCTL_HAL_REBOOT CTL_CODE(FILE_DEVICE_HAL, 15, METHOD_BUFFERED, FILE_ANY_ACCESS)
      win::KernelIoControl(IOCTL_HAL_REBOOT, NULL, 0, NULL, 0, NULL);
   }
#endif
}

//----------------------------

bool CanRebootDevice(){
#ifdef _WIN32_WCE
   return true;
#else
   return false;
#endif
}

//----------------------------

void Sleep(dword time){
   win::Sleep(time);
}

//----------------------------

}//system
//----------------------------

class C_app_observer{
public:
   class C_timer_imp: public C_application_base::C_timer{
   public:
      C_application_base *app;
      enum{
         TYPE_PERIODIC,
         TYPE_ALARM,
      } type;
      void *context;
      union{
         dword ms;
         dword alarm_time;
      };
      dword last_tick_time;
      bool on;

      C_timer_imp(C_application_base *_app):
         on(false),
         app(_app)
      {}
      ~C_timer_imp(){
         Pause();
      }
      virtual void Pause(){
         if(on){
            win::KillTimer((win::HWND)app->GetIGraph()->GetHWND(), dword(this));
            on = false;
            //LOG_RUN("Stop timer");
         }
      }
      virtual void Resume(){
         if(!on){
            win::HWND hwnd = (win::HWND)app->GetIGraph()->GetHWND();
            assert(hwnd);
            dword id;
            if(type==TYPE_PERIODIC){
               id = win::SetTimer(hwnd, dword(this), ms, NULL);//&RunPeriodicThunk);
               //LOG_RUN_N("Start timer", ms);
            }else{
               id = win::SetTimer(hwnd, dword(this),
#ifdef _DEBUG
               1000,
#else
               5000,
#endif
               NULL);//&RunAlarmThunk);
               //LOG_RUN_N("Start alarm", ms);
            }
            assert(id==dword(this));
            on = true;
         }
         last_tick_time = GetTickTime();
      }
      /*
      static void CALLBACK RunPeriodicThunk(win::HWND hwnd, win::UINT uMsg, win::UINT_PTR idEvent, win::DWORD dwTime){
         ((C_timer_imp*)idEvent)->RunPeriodic();
      }
      void RunPeriodic(){
         dword t = GetTickTime();
         dword d = t - last_tick_time;
         last_tick_time = t;
         app->TimerTick(this, context, d);
      }
      static void CALLBACK RunAlarmThunk(win::HWND hwnd, win::UINT uMsg, win::UINT_PTR idEvent, win::DWORD dwTime){
         ((C_timer_imp*)idEvent)->RunAlarm();
      }
      void RunAlarm(){
         S_date_time dt;
         dt.GetCurrent();
         if(dt.sort_value>=alarm_time){
            Pause();
            app->AlarmNotify(this, context);
         }
      }
      */
   };

//----------------------------

   C_application_base::C_timer *CreateTimer(dword ms, void *context){

      C_timer_imp *tmr = new C_timer_imp(app);
      //tmr->obs = this;
      tmr->type = C_timer_imp::TYPE_PERIODIC;
      tmr->ms = ms;
      tmr->context = context;
      tmr->Resume();
      return tmr;
   }

//----------------------------

   C_application_base::C_timer *CreateAlarm(dword time, void *context){

      C_timer_imp *tmr = new C_timer_imp(app);
      //tmr->obs = this;
      tmr->type = C_timer_imp::TYPE_ALARM;
      tmr->alarm_time = time;
      tmr->context = context;
      tmr->Resume();
      return tmr;
   }

//----------------------------

   void MakeTouchFeedback(C_application_base::E_TOUCH_FEEDBACK_MODE mode){
#ifndef _WIN32_WCE
      win::FlashWindow((win::HWND)app->GetIGraph()->GetHWND(), true);
#endif
   }

//----------------------------

public:
   C_application_base *app;

   C_app_observer():
      app(NULL)
      //hwnd(NULL)
   {}

//----------------------------

   void Close(){
      delete app;
   }

//----------------------------

   virtual void Exit(){
      win::PostQuitMessage(0);
   }

//----------------------------

   bool InitApp(
#ifdef _WIN32_WCE
      const win::LPWSTR
#else
      const win::LPSTR
#endif
      cmd_line){

      app = CreateApplication();
      if(!app)
         return false;
      app->InitInternal(this, (*cmd_line!=0));
      app->Construct();
      app->UpdateScreen();
      //hwnd = (win::HWND)app->GetIGraph()->GetHWND();

      if(*cmd_line){
#ifdef _WIN32_WCE
         app->OpenDocument(cmd_line);
#else
         Cstr_w cl; cl.Copy(cmd_line);
         app->OpenDocument(cl);
         app->UpdateScreen();
#endif
      }
      if(win::GetFocus()==app->GetIGraph()->GetHWND())
         app->FocusChange(true);
      return true;
   }

//----------------------------

   static void MainLoop(){

      win::MSG msg;
      while(win::GetMessage(&msg, NULL, 0, 0)){
         win::TranslateMessage(&msg);
         /*
#ifdef _DEBUG
                              //process here, otherwise our debug exception handler doesn't work
         if(msg.message==WM_TIMER && msg.lParam==int((void*)&C_timer_imp::RunPeriodicThunk)){
            ((C_timer_imp*)msg.wParam)->RunPeriodic();
         }else
#endif
            */
         win::DispatchMessage(&msg);
      }
   }
};

//----------------------------

void C_application_base::Exit(){ app_obs->Exit(); }
C_application_base::C_timer *C_application_base::CreateTimer(dword ms, void *context){ return app_obs->CreateTimer(ms, context); }
C_application_base::C_timer *C_application_base::CreateAlarm(dword time, void *context){ return app_obs->CreateAlarm(time, context); }
void C_application_base::MakeTouchFeedback(E_TOUCH_FEEDBACK_MODE mode){ app_obs->MakeTouchFeedback(mode); }

//----------------------------
#include <C_file.h>
#if !defined _WIN32_WCE && defined _DEBUG
extern bool debug_has_kb = true, debug_has_full_kb = true;
#endif


void C_application_base::InitInputCaps(){

#ifdef _WIN32_WCE
   has_kb = IsWMSmartphone();
   has_full_kb = false;

   /*
                              //can't use this, it reports KB wrongly
   dword ks = win::GetKeyboardStatus();
   if(ks&1){                  //KBDI_KEYBOARD_PRESENT
      has_kb = true;
   }
   if(ks&4){                  //KBDI_KEYBOARD_ALPHA_NUM
      has_full_kb = has_kb = true;
   }
   /**/
   C_file fl;
   if(fl.Open(L":HKCU\\Software\\Microsoft\\Shell\\HasKeyboard")){
      dword val;
      if(fl.ReadDword(val)){
         has_full_kb = bool(val);
         if(has_full_kb)
            has_kb = true;
      }
   }
                              //PocketPC has mouse, Smartphone has not
   has_mouse = !IsWMSmartphone();
#else
#ifdef _DEBUG
   //has_mouse = debug_has_mouse;
   has_full_kb = debug_has_full_kb;
   has_kb = debug_has_kb;
#else
   has_mouse = has_kb = has_full_kb = true;
#endif
#endif
}

//----------------------------

void C_application_base::MinMaxApplication(bool min){
   win::HWND hw = (win::HWND)igraph->GetHWND();
   ShowWindow(hw, min ? SW_MINIMIZE : SW_RESTORE);
   if(!min)
      SetActiveWindow(hw);
}

//----------------------------

#ifdef _WIN32_WCE
extern wchar base_dir[MAX_PATH];
#endif

//----------------------------

void GetClassName(wchar buf[MAX_PATH]){
   win::GetModuleFileName(NULL, buf, MAX_PATH);
}

//----------------------------
#ifdef _WIN32_WCE
static bool is_smartphone = false;

bool IsWMSmartphone(){ return is_smartphone; }

static void DetectWMPlatform(){

   wchar pt[20];
   if(win::SystemParametersInfo(SPI_GETPLATFORMTYPE, sizeof(pt), pt, 0)){
      Cstr_w s = pt;
      s.ToLower();
      is_smartphone = (s==L"smartphone");
   }
}

#endif
//----------------------------

int WINAPI WinMain(win::HINSTANCE hInstance, win::HINSTANCE hPrevInstance,
#ifdef _WIN32_WCE
   win::LPWSTR cmd_line,
#else
   win::LPSTR cmd_line,
#endif
   int nCmdShow){

   wchar clsname[MAX_PATH];
   GetClassName(clsname);

#ifdef _WIN32_WCE
   DetectWMPlatform();
                              //on Windows, we run only one instance of application
                              // check if our app is already running
   win::HWND hwnd_app = win::FindWindow(clsname, NULL);
   if(hwnd_app){
      win::SetForegroundWindow(hwnd_app);
      return 0;
   }
   win::GetModuleFileName(hInstance, base_dir, MAX_PATH);
   for(int i=StrLen(base_dir); i--; ){
      if(base_dir[i]=='\\'){
         base_dir[i+1] = 0;
         break;
      }
   }
#else
#ifdef _DEBUG
   //if(!win::IsDebuggerPresent())
   {
      static win::HINSTANCE hi = win::LoadLibrary(L"iexcpt.dll");
      if(hi){
         typedef void (__stdcall*t_InitializeExceptions)(dword);
         t_InitializeExceptions fp = (t_InitializeExceptions)win::GetProcAddress(hi, "?InitializeExceptions@@YGXK@Z");
         if(fp)
            fp(dword(-1));
      }
   }
#endif
                              //set current directory to the location of executable
   {
      wchar *buf = new wchar[MAX_PATH];
      int i = GetModuleFileName(hInstance, buf, MAX_PATH);
      while(i && buf[i-1]!='\\') --i;
      buf[i] = 0;
      win::SetCurrentDirectory(buf);
      delete[] buf;
   }
#endif

   {
                              //try to init application
      C_app_observer app_obs;
      if(!app_obs.InitApp(cmd_line))
         return 1;
                              //create and run message loop
      app_obs.MainLoop();
      app_obs.Close();
   }

#if defined _DEBUG && !defined _WIN32_WCE
   void DumpMemoryLeaks();
   DumpMemoryLeaks();
#endif
   return 0;
}

//----------------------------

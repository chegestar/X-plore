//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <Util.h>
#include <Phone.h>

//----------------------------
#ifdef _WIN32_WCE
namespace win{
#include <Windows.h>
#include <Aygshell.h>
#include <nled.h>
}
#include <C_file.h>
                              //from vibrate.h:
typedef enum{
   VDC_AMPLITUDE,
   VDC_FREQUENCY,
   VDC_LAST
} VIBRATEDEVICECAPS;

typedef struct{
   win::WORD wDuration;
   win::BYTE bAmplitude;
   win::BYTE bFrequency;
} VIBRATENOTE;

//#define INFO(n) Info(n)
#define INFO(n){}

/*
extern "C" {
win::BOOL WINAPI NLedGetDeviceInfo(win::UINT nInfoId, void *pOutput );
win::BOOL WINAPI NLedSetDevice(win::UINT nDeviceId, void *pInput );
};
*/

//----------------------------

bool C_phone_profile::IsSilentProfile(){
   C_file ck;
   if(ck.Open(L":HKCU\\ControlPanel\\Profiles\\ActiveProfile")){
      wchar buf[32];
      int nr = Min((int)ck.GetFileSize(), (int)sizeof(buf)-2);
      if(ck.Read(buf, nr)){
         buf[nr/2] = 0;
         if(!StrCmp(buf, L"Silent") || !StrCmp(buf, L"Meeting"))
            return true;
      }
   }
   return false;
}

//----------------------------

class C_imp{

   win::HINSTANCE hi_dll;
   typedef win::HRESULT (*t_Vibrate)(win::DWORD cvn, const VIBRATENOTE *rgvn, win::BOOL fRepeat, win::DWORD dwTimeout);
   typedef win::HRESULT (*t_VibrateStop)();

   t_Vibrate Vibrate;
   t_VibrateStop VibrateStop;
   dword curr_vibrate_time;

   int led_vibrate;           //-1 = no

//----------------------------

   void InitLed(){
      win::NLED_COUNT_INFO lci = { 0 };
      if(NLedGetDeviceInfo(NLED_COUNT_INFO_ID, &lci)){
         for(dword i=0; i<lci.cLeds; i++){
            win::NLED_SUPPORTS_INFO si;
            si.LedNum = i;
            if(NLedGetDeviceInfo(NLED_SUPPORTS_INFO_ID, &si)){
               if(si.lCycleAdjust == -1){
                  led_vibrate = i;
                  break;
               }
            }
         }
      }
   }

//----------------------------

   void Init(){
      hi_dll = win::LoadLibrary(L"aygshell.dll");
      if(hi_dll){
         typedef int (*t_VibrateGetDeviceCaps)(VIBRATEDEVICECAPS vdc);
         t_VibrateGetDeviceCaps vgdc = (t_VibrateGetDeviceCaps)GetProcAddress(hi_dll, L"VibrateGetDeviceCaps");
         if(vgdc){
            int nc = vgdc(VDC_AMPLITUDE);
            if(nc){
               Vibrate = (t_Vibrate)GetProcAddress(hi_dll, L"Vibrate");
               VibrateStop = (t_VibrateStop)GetProcAddress(hi_dll, L"VibrateStop");
               if(Vibrate && VibrateStop){
                  return;
               }else
                  INFO("No Vibrate & VibrateStop");
               Vibrate = NULL;
               VibrateStop = NULL;
            }else
               INFO("No vibration caps");
         }else
            INFO("No function VibrateGetDeviceCaps");
      }else
         INFO("Can't load aygshell.dll");
                              //no vibration, try leds
      InitLed();
   }

//----------------------------

   void LedOn(bool on){
      win::NLED_SETTINGS_INFO settings;
      settings.LedNum = led_vibrate;
      settings.OffOnBlink = on;
      NLedSetDevice(NLED_SETTINGS_INFO_ID, &settings);
   }

//----------------------------

   void Stop(){
      if(led_vibrate!=-1)
         LedOn(false);
      else if(VibrateStop)
         VibrateStop();
   }

//----------------------------

   void ThreadProc(){
      //INFO("Start vibrate");
      if(led_vibrate!=-1)
         LedOn(true);
      else{
         win::HRESULT hr = Vibrate(0, NULL, true, curr_vibrate_time);
         if(hr<0){
            Vibrate = NULL;
            VibrateStop = NULL;
            InitLed();
            if(led_vibrate!=-1)
               LedOn(true);
         }
      }
      //Info("Vibrate result", hr);
      win::Sleep(curr_vibrate_time);
      Stop();
      win::HANDLE h = h_thread;
      h_thread = NULL;
      win::CloseHandle(h);
   }
   static win::DWORD _ThreadProc(void *param){
      C_imp *_this = (C_imp*)param;
      _this->ThreadProc();
      return 0;
   }
   win::HANDLE h_thread;

   void StopThread(){
      if(h_thread){
         win::TerminateThread(h_thread, 0);
         win::CloseHandle(h_thread);
         h_thread = NULL;
         Stop();
      }
   }

public:
   C_imp():
      hi_dll(NULL),
      Vibrate(NULL),
      VibrateStop(NULL),
      h_thread(NULL),
      led_vibrate(-1)
   {}
   ~C_imp(){
      if(hi_dll){
         StopThread();
         win::FreeLibrary(hi_dll);
      }
   }

   static C_imp *Alloc(){
      C_imp *imp = new(true) C_imp;
      imp->Init();
      return imp;
   }

   inline bool IsOk() const{ return (Vibrate!=NULL || led_vibrate!=-1); }

   bool Do(dword ms){
      StopThread();
      curr_vibrate_time = ms;
      dword tid;
      h_thread = win::CreateThread(NULL, 0, _ThreadProc, this, 0, &tid);
      return true;
   }
};

//----------------------------
#else

bool C_phone_profile::IsSilentProfile(){
   return false;
}

//----------------------------
                              //empty implementation
class C_imp{
public:
   static C_imp *Alloc(){ return new(true) C_imp; }
   inline bool IsOk() const{ return true; }
   bool Do(dword ms){ return true; }
};


#endif
//----------------------------

C_vibration::C_vibration():
   imp(NULL)
{}

//----------------------------

C_vibration::~C_vibration(){
   C_imp *vimp = (C_imp*)imp;
   delete vimp;
}

//----------------------------

bool C_vibration::IsSupported(){

   if(!imp)
      imp = C_imp::Alloc();
   C_imp *vimp = (C_imp*)imp;
   return vimp->IsOk();
}

//----------------------------

bool C_vibration::Vibrate(dword ms){

   if(!IsSupported())
      return false;
   C_imp *vimp = (C_imp*)imp;
   return vimp->Do(ms);
}

//----------------------------

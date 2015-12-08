#include <e32base.h>
#include <e32std.h>
#include <Util.h>
#include <Phone.h>

//----------------------------
                              ////S60 3rd
#if defined S60 && defined __SYMBIAN_3RD__
#include <HwrmVibra.h>
#include <HwrmVibraSdkCrKeys.h>
#include <CentralRepository.h>
#include <MProfileEngine.h>
#include <MProfile.h>
#include <MprofileExtraTones.h>
#include <ProfileEngineSdkCrKeys.h>

//----------------------------

bool C_phone_profile::IsVibrationEnabled(){

   bool en = false;
   CRepository *cr = CRepository::NewL(KCRUidVibraCtrl);
   if(cr){
      int val;
      if(cr->Get(KVibraCtrlProfileVibraEnabled, val))
         val = 0;
      delete cr;
      en = (val!=0);
   }
   return en;
}

//----------------------------
#include <EikEnv.h>

bool C_phone_profile::GetEmailAlertTone(Cstr_w &fn){

   fn.Clear();
#ifndef _DEBUG                //not in debug, don't have ProfileEng.lib
   class C_lf: public C_leave_func{
   public:
      MProfileEngine *eng;
      MProfile *prof;
      Cstr_w s;
      int Run(){
         eng = CreateProfileEngineL(&CEikonEnv::Static()->FsSession());
         prof = eng->ActiveProfileL();
         TFileName fn = prof->ProfileExtraTones().EmailAlertTone();
         s = (const wchar*)fn.PtrZ();
         return 0;
      }
      C_lf():
         eng(NULL),
         prof(NULL)
      {}
      ~C_lf(){
         if(prof)
            prof->Release();
         if(eng)
            eng->Release();
      }
   } lf;
   int err = lf.Execute();
   if(!err){
      fn = lf.s;
      return true;
   }
#endif
   return false;
}

//----------------------------

int C_phone_profile::GetProfileVolume(){

   int ret = 100;
#ifndef _DEBUG
   class C_lf: public C_leave_func{
   public:
      MProfileEngine *eng;
      int vol;
      int Run(){
         eng = CreateProfileEngineL(&CEikonEnv::Static()->FsSession());
         vol = eng->TempRingingVolumeL()*10;
         //Fatal("vol", vol);
         return 0;
      }
      C_lf(): eng(NULL){}
      ~C_lf(){
         if(eng)
            eng->Release();
      }
   } lf;
   int err = lf.Execute();
   if(!err)
      ret = lf.vol;
   //else Fatal("err", err);
#endif
   return ret;
}

//----------------------------

bool C_phone_profile::IsSilentProfile(){
   struct S_lf: public C_leave_func{
      virtual int Run(){
         cr = CRepository::NewL(KCRUidProfileEngine);
         return 0;
      }
      CRepository *cr;
   } lf;
   lf.cr = NULL;
   lf.Execute();
   if(lf.cr){
      int val;
      //if(lf.cr->Get(KProEngActiveProfile, val))
      if(lf.cr->Get(KProEngActiveMessageAlert, val))
         val = 0;
      delete lf.cr;
      //if(val==1)              //silent profile
      if(val==0)              //silent ringing type
         return true;
   }
   return false;
}

//----------------------------

class C_imp{
   RLibrary lib;
   CHWRMVibra *vc;

   void Init(){
      int err = lib.Load(_L("hwrmvibraclient.dll"));
      if(err)
         return;
      typedef CHWRMVibra *(t_CreateVibra)();
      t_CreateVibra *vp = (t_CreateVibra*)lib.Lookup(2);
      if(vp)
         vc = vp();
   }
public:
   C_imp():
      vc(NULL)
   {}
   ~C_imp(){
      if(vc)
         vc->StopVibraL();
      delete vc;
      lib.Close();
   }

   static C_imp *Alloc(){
      C_imp *imp = new(true) C_imp;
      imp->Init();
      return imp;
   }

   inline bool IsOk() const{ return (vc!=NULL); }

   void Do(dword ms){
      assert(vc);
                              //we must check vibration setting in active profile, otherwise attempt to use vibration when it is disabled causes crash
      if(C_phone_profile::IsVibrationEnabled()){
                              //also don't try to start vibration if it is not allowed - it may crash
         CHWRMVibra::TVibraStatus vs = vc->VibraStatus();
         if(vs!=CHWRMVibra::EVibraStatusNotAllowed)
            vc->StartVibraL(TUint16(ms));
      }
   }
};

//----------------------------
#elif defined S60

#include <SaClient.h>

bool C_phone_profile::IsSilentProfile(){
   const TUid KUidSilentMode = {0x100052df};
   RSystemAgent agent;
   agent.Connect();
   int silent = agent.GetState(KUidSilentMode);
   agent.Close();
   if(silent==1)
      return true;
   return false;
}

//----------------------------

class CVibraControl : public CBase{
public:
   enum TVibraModeState{
      EVibraModeON = 0,   // Vibration setting in the user profile is on.
      EVibraModeOFF,      // Vibration setting in the user profile is off.
      EVibraModeUnknown   // For debugging/development and signalling an error condition.
   };
   enum TVibraRequestStatus{
      EVibraRequestOK = 0,      // Request is OK.
      EVibraRequestFail,        // Request is failed.
      EVibraRequestNotAllowed,  // Vibra is set off in the user profile
      EVibraRequestStopped,     // Vibra is stopped
      EVibraRequestUnableToStop,// Unable to stop vibra
      EVibraRequestUnknown      // For debugging/development and signalling an error condition.
   };
   enum TVibraCtrlPanic{
      EPanicUnableToGetVibraSetting,
      EPanicVibraGeneral
   };
public:
   virtual ~CVibraControl();
   virtual TInt StartVibraL(TUint16 aDuration) = 0;
   virtual TInt StopVibraL() = 0;
   virtual TVibraModeState VibraSettings() const = 0;
protected:
   CVibraControl();
};

class MVibraControlObserver{
public:
   virtual void VibraModeStatus(CVibraControl::TVibraModeState aStatus) = 0;
   virtual void VibraRequestStatus(CVibraControl::TVibraRequestStatus aStatus) = 0;
};

class VibraFactory{
public:
   IMPORT_C static CVibraControl* NewL();
   IMPORT_C static CVibraControl* NewL(MVibraControlObserver* aCallback);
   IMPORT_C static CVibraControl* NewLC(MVibraControlObserver* aCallback);
private:
   VibraFactory();
};

//----------------------------

class C_imp{
   RLibrary lib;
   CVibraControl *vc;

   void Init(){
      static const wchar lib_name[] =
#if !defined __SYMBIAN_3RD__ && !defined _DEBUG
         L"z:\\system\\libs\\"
#endif
         L"vibractrl.dll";

      int err = lib.Load(TPtrC((word*)lib_name, sizeof(lib_name)/2-1));
      if(err)
         return;
      typedef CVibraControl* (t_CreateVibra)();
      t_CreateVibra *vp = (t_CreateVibra*)lib.Lookup(2);
      if(vp)
         vc = vp();
   }

public:
   C_imp():
      vc(NULL)
   {}
   ~C_imp(){
      delete vc;
      lib.Close();
   }

   static C_imp *Alloc(){
      C_imp *imp = new(true) C_imp;
      imp->Init();
      return imp;
   }

   inline bool IsOk() const{ return (vc!=NULL); }

   void Do(dword ms){
      assert(vc);
      vc->StartVibraL(TUint16(ms));
   }
};

//----------------------------

#else
                              //empty implementation
class C_imp{
public:
   static C_imp *Alloc(){ return new(true) C_imp; }
   inline bool IsOk() const{ return false; }
   void Do(dword ms){ }
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
   vimp->Do(ms);
   return true;
}

//----------------------------

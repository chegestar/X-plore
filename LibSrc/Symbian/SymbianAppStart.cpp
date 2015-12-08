//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <EikApp.h>

#if defined S60
#include <AknView.h>
#include <AknViewAppUi.h>
#include <AknApp.h>
#include <EikBtGpc.h>
#ifdef __SYMBIAN_3RD__
#include <FeatDiscovery.h>
#include <FeatureInfo.h>
#endif
#define CDOCUMENT CEikDocument
#define CAPPUI CAknViewAppUi
#define CAPPLICATION CAknApplication
#else
#error
#endif

#include "DeviceUID.h"
#include <EikDoc.h>
#include <Bacline.h>
#include <ApgTask.h>
#include <Hal.h>
#include <Hal_data.h>
#include <EikEnv.h>

#include <Util.h>
#include <C_file.h>

#include <AppBase.h>
#include <C_vector.h>
#include <TimeDate.h>

extern
#ifndef __WINS__
   const
#endif//__WINS__

#ifdef __SYMBIAN_3RD__
#ifdef __WINS__
   TEmulatorImageHeader uid;
#else
   dword uid;
#endif
#else//__SYMBIAN_3RD__
   TUid uid[];
#endif//!__SYMBIAN_3RD__

//----------------------------
namespace system{

dword GetDeviceId(){
   int machine_uid;
   HAL::Get(HALData::EMachineUid, machine_uid);
   return machine_uid;
}

//----------------------------

void SetAsSystemApp(bool b){
   CEikonEnv::Static()->SetSystem(b);
}

//----------------------------

void RebootDevice(){
#ifdef __SYMBIAN_3RD__
   /*
   RLibrary lib;
   if(!lib.Load(_L("z:sysutil"))){
      enum TSWStartupReason{
                  // Normal startup reasons (100..149)
                  // Nothing set the (default value).
         ESWNone = 100,
                  // Restore Factory Settings (Normal)
         ESWRestoreFactorySet = 101,
                  // Language Switched
         ESWLangSwitch = 102,
                  // Warranty transfer
         ESWWarrantyTransfer = 103,
                  // Possibly needed for handling power off & charger connected use case.
         ESWChargerConnected = 104,
                  // Restore Factory Settings (Deep)
         ESWRestoreFactorySetDeep = 105
      };
      typedef TInt (*t_ShutdownAndRestart)(TUid const&, TSWStartupReason);
      t_ShutdownAndRestart ShutdownAndRestart = (t_ShutdownAndRestart)lib.Lookup(4);
      if(ShutdownAndRestart){
         TUid t = {0x101fbad3};
         int err = ShutdownAndRestart(t, ESWNone);
         //Fatal("err", err);
      }
      lib.Close();
   }
   */
   RProcess p;
   int err = p.Create(_L("z:starter"), _L(""));
   if(!err)
      p.Resume();
   /*

#else
   while(true);
#endif
   */
#else
   UserSvr::ResetMachine(EStartupWarmReset);
#endif
}

//----------------------------

bool CanRebootDevice(){
#ifdef __SYMBIAN_3RD__
   return false;
#else
   return true;
#endif
}

//----------------------------

void Sleep(dword time){
   User::After(time*1000);
}

//----------------------------

}//system
//----------------------------

class C_doc_help: public CEikDocument{
public:
   C_doc_help(CEikApplication &a): CEikDocument(a){}
   inline CEikAppUi *AppUI(){ return iAppUi; }
};

CEikAppUi *S_global_data::GetAppUi() const{
   return ((C_doc_help*)doc)->AppUI();
}

//----------------------------

#ifdef __SYMBIAN_3RD__
extern S_global_data *global_data;

#else

#endif
//----------------------------

enum TTouchLogicalFeedback{
   ETouchFeedbackNone,
   ETouchFeedbackBasic,
   ETouchFeedbackSensitive
};
enum TTouchEventType{ ETouchEventStylusDown };

class MTouchFeedback{
public:
   IMPORT_C static MTouchFeedback* Instance();
   IMPORT_C static MTouchFeedback* CreateInstanceL();
   IMPORT_C static void DestroyInstance();
   virtual TBool TouchFeedbackSupported() = 0;
   virtual void SetFeedbackEnabledForThisApp( TBool aEnabled ) = 0;
   virtual TBool FeedbackEnabledForThisApp() = 0;
   virtual TInt SetFeedbackArea(const CCoeControl* aControl, TUint32 aIndex, TRect aRect, TTouchLogicalFeedback aFeedbackType, TTouchEventType aEventType ) = 0;
   virtual void RemoveFeedbackArea( const CCoeControl* aControl, TUint32 aIndex ) = 0;
   virtual void RemoveFeedbackForControl( const CCoeControl* aControl ) = 0;
   virtual void ChangeFeedbackArea( const CCoeControl* aControl, TUint32 aIndex, TRect aNewRect ) = 0;
   virtual void ChangeFeedbackType( const CCoeControl* aControl, TUint32 aIndex, TTouchLogicalFeedback aNewType ) = 0;
   virtual void MoveFeedbackAreaToFirstPriority( const CCoeControl* aControl, TUint32 aIndex ) = 0;
   virtual void FlushRegistryUpdates( ) = 0;
   virtual void InstantFeedback(TTouchLogicalFeedback aType ) = 0;
   virtual void InstantFeedback(const CCoeControl* aControl, TTouchLogicalFeedback aType ) = 0;
   virtual TBool ControlHasFeedback( const CCoeControl* aControl ) = 0;
   virtual TBool ControlHasFeedback( const CCoeControl* aControl, TUint32 aIndex ) = 0;
   virtual void EnableFeedbackForControl( const CCoeControl* aControl, TBool aEnable ) = 0;
   virtual void EnableFeedbackForControl( const CCoeControl* aControl, TBool aEnableVibra, TBool aEnableAudio ) = 0;
   virtual void SetFeedbackEnabledForThisApp( TBool aVibraEnabled, TBool aAudioEnabled ) = 0;
};

//----------------------------

class C_app_observer{
public:
   C_application_base *app_base;
   bool want_quit;
   bool has_focus;

   C_app_observer():
      app_base(NULL),
      want_quit(false),
      has_focus(true),
#ifdef S60
      view(NULL),
#endif
      timer_manager(this)
   {}

   virtual void Exit(){
      want_quit = true;
   }

//----------------------------

   class C_touch_feedback{
      RLibrary lib;
      bool inited;
      MTouchFeedback *tf;
   public:
      C_touch_feedback():
         inited(false),
         tf(NULL)
      {}
      ~C_touch_feedback(){
         lib.Close();
      }
      void Make(C_application_base::E_TOUCH_FEEDBACK_MODE mode){
         if(!inited){
            inited = true;
            if(!lib.Load(_L("z:touchfeedback"))){
               typedef MTouchFeedback *(*t_Instance)();
               t_Instance Instance = (t_Instance)lib.Lookup(3);
               tf = Instance();
            }
         }
         if(tf){
            //mode
            TTouchLogicalFeedback tlf = ETouchFeedbackBasic;
            switch(mode){
            case C_application_base::TOUCH_FEEDBACK_SELECT_MENU_ITEM:
            case C_application_base::TOUCH_FEEDBACK_SCROLL:
            case C_application_base::TOUCH_FEEDBACK_SELECT_LIST_ITEM:
               tlf = ETouchFeedbackSensitive;
               break;
            }
            tf->InstantFeedback(tlf);
         }
      }
   } touch_feedback;

   virtual void MakeTouchFeedback(C_application_base::E_TOUCH_FEEDBACK_MODE mode){
      touch_feedback.Make(mode);
   }

//----------------------------
   void DestroyAppAndExit(){
      DestroyApp();
//#ifdef __SYMBIAN_3RD__
                  //ugly thread exit, because our compressed loader crashes when using AppUi::Exit
      User::Exit(0);
      /*
#else
      CAPPUI::Exit();
#endif
      */
   }

//----------------------------

   virtual void DestroyApp() = 0;

//----------------------------
                        //use own timer manager, because those on Symbian is not ideal - it calls only 1st timer if it's ticking too fast
   class C_timer_manager;
   friend class C_timer_manager;

//----------------------------

   inline void TimerTick(C_application_base::C_timer *t, void *context, dword tt){
      app_base->TimerTick(t, context, tt);
   }

//----------------------------

   class C_timer_manager{
   public:
      CPeriodic *timer;

   //----------------------------
                              //single application timer
      class C_timer_imp: public C_application_base::C_timer{
         bool running;  //indicating if it is running or paused
      public:
         void *context; //app context
         C_timer_manager &mgr;
         int ms;        //desired time value
         int curr_count;   //current cumulated time

         C_timer_imp(C_timer_manager &m, void *c, dword _ms):
            mgr(m),
            context(c),
            ms(_ms),
            curr_count(0),
            running(false)
         {}
         ~C_timer_imp(){
            Pause();
         }

      //----------------------------
         virtual void Pause(){
            if(running){
               mgr.RemoveTimer(this);
               running = false;
            }
         }
         virtual void Resume(){
            if(!running){
               curr_count = 0;   //restart counting on resumed timer
               mgr.AddTimer(this);
               running = true;
            }
         }
      };

   //----------------------------
      C_app_observer *obs;
                        //all active timers
      C_vector<C_timer_imp*> timers;
      dword last_tick_time;

      C_timer_manager(C_app_observer *_obs):
         obs(_obs),
         timer(NULL)
      {}
      ~C_timer_manager(){
         DestroyTimer();
      }
      void DestroyTimer(){
         if(timer){
            timer->Cancel();
            delete timer;
            timer = NULL;
         }
      }

   //----------------------------

      inline void Tick(){

         if(!obs->app_base)
            return;
                        //run all timers which should be
         dword t = GetTickTime();
         int d = t - last_tick_time;
         last_tick_time = t;
         for(int i=timers.size(); i--; ){
            C_timer_imp *t = timers[i];
            t->curr_count += d;
            const int check_ms = t->ms*7/8;
            if(t->curr_count >= check_ms){
               dword tt = t->curr_count;
               t->curr_count = 0;

               obs->TimerTick(t, t->context, tt);
               if(obs->want_quit){
                  obs->DestroyAppAndExit();
                  return;
               }
               if(i>timers.size())
                  i = timers.size();
            }
         }
                        //let other threads live (good practice if our app uses much CPU)
         User::After(1);
      }

   //----------------------------

      static TInt TickThunk(TAny *c){
         ((C_timer_manager*)c)->Tick();
         return 1;
      }

   //----------------------------

      void ManageTimer(){

         if(!timers.size()){
            //DestroyTimer();
            timer->Cancel();
            return;
         }
         if(!timer)
            timer = CPeriodic::NewL(CActive::EPriorityStandard-1);   //priority less than standard, so that we let other active objs to run if ticking takes 100% of time
         else
            timer->Cancel();
                        //determine lowest timer value
         dword min_ms = 0xffffffff;
         for(int i=timers.size(); i--; ){
            min_ms = Min(min_ms, timers[i]->ms);
         }
#if defined __WINS__ && !defined __SYMBIAN_3RD__
                        //limit timer to less than 10x/sec
         min_ms = Max(min_ms, 110);
#endif
         dword us = min_ms*1000;
         timer->Start(us, us, TCallBack(TickThunk, this));
         last_tick_time = GetTickTime();
      }

   //----------------------------

      void AddTimer(C_timer_imp *t){

         timers.push_back(t);
         ManageTimer();
      }

   //----------------------------

      void RemoveTimer(C_timer_imp *t){
         for(int i=timers.size(); i--; ){
            if(timers[i]==t){
               timers.remove_index(i);
               ManageTimer();
               break;
            }
         }
      }
   } timer_manager;

//----------------------------

   inline void AlarmNotify(C_application_base::C_timer *t, void *context){
      app_base->AlarmNotify(t, context);
   }

//----------------------------

   class C_alarm_imp;
   friend class C_alarm_imp;

   class C_alarm_imp: public C_application_base::C_timer, public CTimer{
      C_app_observer *obs;
      dword time;
      void *context;

      virtual void RunL(){
         //Info("Alarm RunL");
#if 1
                        //fix: alarm runs out of given period, detect this
         S_date_time dt;
         dt.GetCurrent();
         int delta = time - dt.GetSeconds();
         //Fatal("delta", delta);
         if(delta > 10)
            Resume();
         else
#endif
            obs->AlarmNotify(this, context);
      }
   public:
      C_alarm_imp(C_app_observer *_obs, void *_c, dword tm):
         CTimer(CActive::EPriorityStandard-1),
         obs(_obs),
         context(_c),
         time(tm)
      {
         CTimer::ConstructL();
         CActiveScheduler::Add(this);
      }
      ~C_alarm_imp(){
         Pause();
         Deque();
      }
      virtual void Pause(){
         if(IsActive())
            Cancel();
      }
      virtual void Resume(){
         if(!IsActive()){
            S_date_time dt;
            dt.SetFromSeconds(time);
            /*
            Info("y", dt.year);
            Info("m", dt.month);
            Info("d", dt.day);
            Info("h", dt.hour);
            Info("m", dt.minute);
            Info("s", dt.second);
            /**/
            TDateTime st(dt.year, TMonth(dt.month), dt.day, dt.hour, dt.minute, dt.second, 0);
            TTime tm(st);
            At(tm);
            //RunL();
         }
      }
   };

//----------------------------

   virtual C_application_base::C_timer *CreateTimer(dword ms, void *context){
      C_timer_manager::C_timer_imp *tmr = new(true) C_timer_manager::C_timer_imp(timer_manager, context, ms);
      tmr->Resume();
      return tmr;
   }
   virtual C_application_base::C_timer *CreateAlarm(dword tm, void *context){
      C_alarm_imp *alm = new(true) C_alarm_imp(this, context, tm);
      alm->Resume();
      return alm;
   }

   void HandleForegroundEvent(bool foreground){

      if(has_focus==foreground)
         return;
      has_focus = foreground;
      if(app_base){
         IGraph *ig = app_base->GetIGraph();
         if(foreground){
            if(ig)
               ig->StartDrawing();
         }else{
            if(ig)
               ig->StopDrawing();
         }
         app_base->FocusChange(foreground);
      }
   }

#ifdef S60

   class C_view: public CAknView{
      CCoeControl *cc;
      CAPPUI *app;

      virtual TUid Id() const{ return TUid::Uid(1); }

      virtual void DoActivateL(const TVwsViewId &aPrevViewId, TUid aCustomMessageId, const TDesC8 &aCustomMessage){
         if(cc)
            app->AddToStackL(cc);
      }
      virtual void DoDeactivate(){
      }
   public:
      /*
      inline void *operator new(size_t sz){
         void *vp = ::operator new(sz);
         MemSet(vp, 0, sz);
         return vp;
      }
      */

      C_view(CCoeControl *c, CAPPUI *a):
         cc(c), app(a)
      {}
      ~C_view(){
      }
      void ConstructL(){
                        //construct using resource ID for menubar created in LCG's resource file
         CAknView::BaseConstructL(0x02284004);
      }
   } *view;
#endif

   void InitView(IGraph *ig, CAPPUI *app_ui){
#if defined S60
      CCoeControl *cc = NULL;
      if(ig)
         cc = ig->GetCoeControl();
      view = new(true) C_view(cc, app_ui);
      view->ConstructL();
      app_ui->AddViewL(view);
      cc->SetMopParent(view);
      app_ui->SetDefaultViewL(*view);
#else
      if(ig)
         app_ui->AddToStackL(ig->GetCoeControl());
#endif
   }

   void FinishAppInit(C_application_base *ab, CAPPUI *app_ui, bool is_embedded){

      ab->InitInternal(this, is_embedded);
      ab->Construct();

      assert(!app_base);
      IGraph *ig = ab->GetIGraph();
      InitView(ig, app_ui);
      app_base = ab;
                        //did we miss focus gain? if yes, focus application now
      if(has_focus){
         has_focus = false;
         HandleForegroundEvent(true);
      }
      {
         IGraph *ig = app_base->GetIGraph();
         if(ig)
            ig->StartDrawing();
      }
   }

   void OpenDocument(const wchar *fn){
      if(app_base)
         app_base->OpenDocument(fn);
   }

   void ScreenChanged(){
      app_base->InitInputCaps();
      if(app_base->GetIGraph())
         app_base->GetIGraph()->ScreenChanged();
   }
};

void C_application_base::Exit(){ app_obs->Exit(); }
C_application_base::C_timer *C_application_base::CreateTimer(dword ms, void *context){ return app_obs->CreateTimer(ms, context); }
C_application_base::C_timer *C_application_base::CreateAlarm(dword tm, void *context){ return app_obs->CreateAlarm(tm, context); }
void C_application_base::MakeTouchFeedback(E_TOUCH_FEEDBACK_MODE mode){ return app_obs->MakeTouchFeedback(mode); }

//----------------------------
#if defined __SYMBIAN_3RD__ && defined S60
class C_library_func{
   RLibrary lib;
   bool loaded;
public:
   C_library_func():
      loaded(false)
   {
   }
   ~C_library_func(){
      if(loaded)
         lib.Close();
   }
   void *Get(const wchar *lib_name, int ord){
      void *fn = NULL;
      if(!lib.Load(TPtrC((word*)lib_name, StrLen(lib_name)))){
         loaded = true;
         fn = (void*)lib.Lookup(ord);
      }
      return fn;
   }
};

#endif
//----------------------------

void C_application_base::InitInputCaps(){

                              //doesn't work:
   //int err = HAL::Get(HALData::EKeyboard, val);
   //if(!err && val!=0)
      //has_kb = true;
   has_kb = true;

#if defined S60 && defined __SYMBIAN_3RD__
   if(has_kb){
      has_full_kb = CFeatureDiscovery::IsFeatureSupportedL(KFeatureIdQwertyInput);

      int val;
      HAL::Get(HALData::EMachineUid, val);
      switch(val){
      case UID_NOKIA_5800:
         has_full_kb = has_kb = false;
         break;
      case UID_NOKIA_E55:  //doubled keys, use numeric instead
      case UID_NOKIA_N76:
         has_full_kb = false;
         break;
      case UID_SAMSUNG_I8510: has_full_kb = false; break;   //device wrongly reports this
      case UID_NOKIA_E75:
      case UID_NOKIA_E70:
         if(has_full_kb){
                              //has full kb only in landscape
            TPixelsAndRotation por;
            CCoeEnv::Static()->ScreenDevice()->GetScreenModeSizeAndRotation(CCoeEnv::Static()->ScreenDevice()->CurrentScreenMode(), por);
            if(por.iRotation==CFbsBitGc::EGraphicsOrientationNormal)
               has_full_kb = false;
         }
         break;
      case UID_NOKIA_E90:
         if(has_full_kb){
            TPixelsAndRotation por;
            CCoeEnv::Static()->ScreenDevice()->GetScreenModeSizeAndRotation(CCoeEnv::Static()->ScreenDevice()->CurrentScreenMode(), por);
            if(por.iPixelSize.iWidth==240)
               has_full_kb = false;
         }
         break;
#ifdef _DEBUG
      case 0x10005f62:        //emulator
         has_full_kb = false;
#endif
      }
   }
#endif

#if defined __SYMBIAN_3RD__ && defined S60
                              //detect touch screen (possible only on 3rd/5th edition)
   C_library_func lfn;
   typedef TBool (*t_Fn)();   //TBool AknLayoutUtils::PenEnabled();
   t_Fn fn = (t_Fn)lfn.Get(L"z:avkon.dll", 4251);
   if(fn)
      has_mouse = fn();
#ifdef _DEBUG
   {
      int val;
      HAL::Get(HALData::EMachineUid, val);
      //has_mouse = (has_mouse || (val==0x10005f62));   //emulator
      if(val==0x10005f62){
                              //emulator
         if(CCoeEnv::Static()->ScreenDevice()->SizeInPixels().iWidth>320 || CCoeEnv::Static()->ScreenDevice()->SizeInPixels().iHeight>320){
            has_mouse = true;
         }
      }
   }
#endif

#endif   //S60 3rd
}

//----------------------------

void C_application_base::MinMaxApplication(bool min){

   TApaTaskList tl(CCoeEnv::Static()->WsSession());
   TUid id = { GetSymbianAppUid() };
   TApaTask at = tl.FindApp(id);
   if(at.Exists()){
      if(min){
         at.SendToBackground();
      }else
         at.BringToForeground();
   }
}

//----------------------------

class C_eik_application: public CAPPLICATION{

//----------------------------
//----------------------------

   class C_document: public CDOCUMENT{

//----------------------------
//----------------------------

      class C_application_ui: public CAPPUI, public C_app_observer{
//----------------------------
      public:
         void DestroyApp(){
            if(app_base){
               IGraph *ig = app_base->GetIGraph();
               if(ig)
                  RemoveFromStack(app_base->GetIGraph()->GetCoeControl());
               delete app_base;
               app_base = NULL;
            }
         }
      private:
//----------------------------

         void HandleWsEventL(const TWsEvent &wse, CCoeControl *dst_ctrl){

            switch(wse.Type()){
            case EEventScreenDeviceChanged:
#if defined S60 && !defined __SYMBIAN_3RD__
            case 0x10005a30:  //KUidValueAknsSkinChangeEvent:
#endif
               if(app_base){
                  CAPPUI::HandleWsEventL(wse, dst_ctrl);
                  ScreenChanged();
                  return;
               }
               break;
               /*
            case EEventKey:
            case EEventKeyDown:
            case EEventKeyUp:
            case EEventFocusGained:
            case EEventFocusLost:
            case EEventWindowVisibilityChanged:
            case 0x10281f36:  //KAknFullOrPartialForegroundGained
            case 0x10281f37:  //KAknFullOrPartialForegroundLost
            case EEventNull:
            case EEventUser:
               break;
            default:
               Fatal("ev", wse.Type());
               /**/

               /*
#if defined S60 && !defined __SYMBIAN_3RD__
            case EEventKeyDown:
            case EEventKeyUp:
               if(wse.Key()->iScanCode==EStdKeyLeftShift || wse.Key()->iScanCode==EStdKeyRightShift)
                  wse.Key()->iScanCode = 0xff;     //post unused code instead
               break;
#endif
               */
            }

            switch(wse.Type()){
#ifdef __SYMBIAN_3RD__
            case KAknUidValueEndKeyCloseEvent:
               if(!app_base || app_base->RedKeyWantClose()){
                  system::SetAsSystemApp(false);      //can't be system app, otherwise it won't close
                  CAPPUI::HandleWsEventL(wse, dst_ctrl);
                  //want_quit = true;
               }
               break;
#endif
            default:
                              // don't pass to specialized AppUI, it makes some problems
                              //note: can't do that, it breaks function of FEP
               //CEikAppUi::HandleWsEventL(wse, dst_ctrl);

               CAPPUI::HandleWsEventL(wse, dst_ctrl);
            }
            if(want_quit)
               DestroyAppAndExit();
         }

      //----------------------------
      // From CEikAppUi, takes care of command handling.
         virtual void HandleCommandL(TInt cmd){

            switch(cmd){
            case 0x100: //EEikCmdExit:
               DestroyAppAndExit();
               return;
            }
            CAPPUI::HandleCommandL(cmd);
         }

      //----------------------------
#ifdef S60
         virtual void ProcessCommandL(TInt cmd){

            switch(cmd){
            case 3000:
            case 3001:
               {
                              //handle softkey presses delivered if resource file defines CBA
                  TKeyEvent ke = { EKeyDevice0+cmd-3000, EStdKeyDevice0+cmd-3000, 0, 0 };
                  app_base->GetIGraph()->GetCoeControl()->OfferKeyEventL(ke, EEventKey);
                  app_base->GetIGraph()->GetCoeControl()->OfferKeyEventL(ke, EEventKeyUp);
               }
               return;
            }
            CAPPUI::ProcessCommandL(cmd);
         }
#endif
      //----------------------------
#ifdef _DEBUG
         virtual void HandleApplicationSpecificEventL(TInt aType, const TWsEvent &aEvent){
            //Info("HASE");
            CAPPUI::HandleApplicationSpecificEventL(aType, aEvent);
         }
#endif
      //----------------------------

         virtual void HandleForegroundEventL(TBool foreground){

            HandleForegroundEvent(foreground);
            CAPPUI::HandleForegroundEventL(foreground);
         }

      //----------------------------

#if defined S60 && defined __SYMBIAN_3RD__
         void RemoveToolbar(){
            //*
                              //remove ugly toolbar on S60 5th ed in landscape mode
            C_library_func lfn;
            typedef CAknToolbar* (*t_Fn)(CEikAppUi*); //CAknToolbar *CAknAppUi::CurrentFixedToolbar() const
            t_Fn fn = (t_Fn)lfn.Get(L"z:avkon.dll", 4485);
            if(fn){
               CAknToolbar *tb = fn(GetGlobalData()->GetAppUi());
               if(tb){
                  C_library_func lfn;
                  typedef void (*t_Fn)(CAknToolbar*, int);  //void CAknToolbar::SetToolbarVisibility(int)
                  t_Fn fn = (t_Fn)lfn.Get(L"z:eikcoctl.dll", 1744);
                  if(fn)
                     fn(tb, 0);
               }
            }
            StopDisplayingPopupToolbar();
            /**/
            //CEikButtonGroupContainer *cba = Cba();
            //cba->MakeVisible(false);

         }
#endif

      //----------------------------
      // Symbian construction, inherited from base class.
      // Calls normal Construct, leaves if error occurs.
         virtual void ConstructL(){

            enum{
               ELayoutAwareAppFlag = 8,   //from Series60 scalable UI (FP3, 3rd edition) - application is resolution-aware, no 176x208 compatibility mode
               EAknEnableSkinFlag = 0x80000,
            };
            dword cflg = 0;
#if defined S60 && defined __SYMBIAN_3RD__
            cflg |= EAknEnableSkinFlag;
            cflg |= 0x00800000;  //EAknTouchCompatibleFlag
#endif
#ifndef __SYMBIAN_3RD__
            cflg |= ELayoutAwareAppFlag;
#endif
            BaseConstructL(cflg);

            C_application_base *ab = CreateApplication();
#ifdef __SYMBIAN_3RD__
            bool is_embedded = false;
#ifdef S60
            RemoveToolbar();
#endif
            FinishAppInit(ab, this, is_embedded);

#else

            bool is_embedded = (iDoorObserver!=NULL);
                              //Symbian cmdline launching:
                              // param 0 = executable
                              // param 1 = .app
                              // param 2 = optional parameter, 1st letter is special: R=standalone, V=from inbox by messaging (embedded), O=open
            bool is_cmdline = false;
            CCommandLineArguments *cline = CCommandLineArguments::NewL();
            if(!is_embedded && cline->Count()>=3){
               const TDesC &cmd = cline->Arg(2);
               if(cmd.Length() >= 2){
                  switch(cmd[0]){
                  case 'O':   //open by utilities (explorer, etc)
                  case 'W':   //open by mail or wap (UIQ)
                  //case 'R':   //stand-alone
                  case '-':   //cmdline param (our tools)
                     is_cmdline = true;
                     is_embedded = true;
                     break;
                  }
               }
            }
            FinishAppInit(ab, this, is_embedded);

            if(is_cmdline){
               TBuf<512> buf;
               for(int i=2; i<cline->Count(); i++){
                  const TDesC &cmd = cline->Arg(i);
                  if(i==2){
                     int l = 1;
                     if(cmd[l]=='\"')
                        ++l;
                     buf.Append(cmd.Right(cmd.Length()-l));
                  }else{
                     buf.Append(' ');
                     buf.Append(cmd);
                  }
               }
               if(buf[buf.Length()-1]=='\"')
                  buf.SetLength(buf.Length()-1);
               buf.Append(0);
               OpenDocument((wchar*)buf.Ptr());
            }
            delete cline;
#endif
#ifdef S60
            SetKeyBlockMode(ENoKeyBlock);
#endif
         }
      //----------------------------
      public:
         C_application_ui(){
         }
         ~C_application_ui(){
            DestroyApp();
            if(iDoorObserver)
               iDoorObserver->NotifyExit(MApaEmbeddedDocObserver::ENoChanges);
         }

//----------------------------

#if defined S60 && defined __SYMBIAN_3RD__
         virtual TBool ProcessCommandParametersL(TApaCommand aCommand, TFileName &aDocumentName, const TDesC8 &aTail){

#if 1
            if(aDocumentName.Length()!=0){
               OpenDocument((wchar*)aDocumentName.PtrZ());
            }
#else
            bool is_embedded = (aDocumentName.Length()!=0);

            C_application_base *ab = CreateApplication();
            ab->InitInternal(this, is_embedded);
            ab->Construct();
            app_base = ab;

            InitView(app_base->GetIGraph());
            if(is_embedded)
               OpenDocument((wchar*)aDocumentName.PtrZ());
#endif

            return CAPPUI::ProcessCommandParametersL(aCommand, aDocumentName, aTail);
         }
#endif
      };
      C_application_ui *app_ui;

   //----------------------------

#ifndef __SYMBIAN_3RD__
      virtual CFileStore *OpenFileL(TBool aDoOpen, const TDesC &aFilename, RFs& aFs){
         if(app_ui){
            TBuf<257> buf;
            buf.Copy(aFilename);
            buf.Append(0);
            app_ui->OpenDocument((wchar*)buf.Ptr());
         }
         return NULL;
      }
#endif

   //----------------------------
                              //struct for keeping interesting data accessible in various places of application
      S_global_data global_data;

//----------------------------

      static void ExceptionHandler(TExcType type){

         //Info("Crash");
         //{ C_file fl; fl.Open(L"C:\\crash", fl.FILE_WRITE); }
         const S_global_data *gd = GetGlobalData();
         C_document *_this = (C_document*)gd->doc;
         if(!_this)
            User::Exit(type);
#ifdef __SYMBIAN_3RD__
         ::global_data = NULL;
#else
         Dll::SetTls(NULL);
#endif
                              //close interfaces and pass exception to kernel
         if(_this->app_ui){
            //_this->app_ui->DestroyApp();   //can't destroy app normal way, when we got here by exception (crash); deleting of application may cause corruption on data
         }
         /*
#if defined S60 && defined __SYMBIAN_3RD__
         Fatal("Exception", type);
#else
         */
#if 0
         const char *err = "Exception";
         switch(type){
         case EExcIntegerDivideByZero: err = "Divide by zero"; break;
         case EExcAccessViolation: err = "Access Violation"; break;
         }
         Info(err, type, true);
#else
#ifdef __SYMBIAN_3RD__
         User::HandleException(NULL);
#else
         User::HandleException(NULL, type);
#endif
#endif
      }

   //----------------------------
   // From CEikDocument, create C_application App-UI object.
      CEikAppUi *CreateAppUiL(){

#ifdef __SYMBIAN_3RD__
         User::SetExceptionHandler(ExceptionHandler, 0xffffffff);
#else
                              //cacth exception, clean nicely
         RThread self;
         self.SetExceptionHandler(ExceptionHandler, 0xffffffff);
#endif

         TParse app_file_name;
         app_file_name.Set(Application()->AppFullName(), NULL, NULL);
         TFileName fp = app_file_name.DriveAndPath();
         StrCpy(global_data.full_path, (const wchar*)fp.PtrZ());
         global_data.doc = this;
         global_data.main_thread_id = RThread().Id();
#ifdef __SYMBIAN_3RD__
         ::global_data = &global_data;
#else
         Dll::SetTls(&global_data);
#endif
         app_ui = new(true) C_application_ui;
         return app_ui;
      }

   public:
      C_document(CEikApplication &app):
         CDOCUMENT(app)
      {
      }

   //----------------------------

      virtual ~C_document(){
#ifdef __SYMBIAN_3RD__
         ::global_data = NULL;
#else
         Dll::SetTls(NULL);
#endif
      }
   };

public:

//----------------------------
   CApaDocument *CreateDocumentL(){
      return new(true) C_document(*this);
   }

//----------------------------
// From CApaApplication, returns application's UID.
   TUid AppDllUid() const{
      TUid id = { GetSymbianAppUid() };
      return id;
   }
   ~C_eik_application(){
   }
};

//----------------------------
// Symbian entry point - constructs application's main class.
extern"C"
EXPORT_C CApaApplication *SymbianMain();

//----------------------------

EXPORT_C CApaApplication *SymbianMain(){

#ifdef __WINS__
                              //we don't want BREAKPOINT exception on Panic, rather behave as on device
   User::SetJustInTime(false);
#endif
   return new(true) C_eik_application;
}

//----------------------------

dword GetSymbianAppUid(){
#ifdef __SYMBIAN_3RD__
#ifdef __WINS__
   return uid.iUids[2].iUid;
#else
   return uid;
#endif
#else
   return uid[2].iUid;
#endif
}

//----------------------------

#ifdef __SYMBIAN_3RD__
#include <EikStart.h>
#include <Cstr.h>

EXPORT_C TInt E32Main(){
                              //switch to bigger heap in emulator
#ifdef __WINS__
   RHeap *heap = UserHeap::ChunkHeap(NULL, 2*1024*1024, 10*1024*1024, 1024*1024);
   User::SwitchHeap(heap);
#endif
   int ret = EikStart::RunApplication(SymbianMain);
   return ret;
}

extern"C"
EXPORT_C TInt Lcg32Main(){
   return E32Main();
}
#endif

//----------------------------

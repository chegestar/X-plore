#ifdef __SYMBIAN32__
#include <e32std.h>
#endif
#include "NotifyWindow.h"
#include <Util.h>
#include <Cstr.h>
#include <C_file.h>
#include <AppBase.h>


#define USE_LED_BLINK         //on S60 3rd, on supported devices (disabled, as it causes crashes because of trap handler)

//----------------------------
#ifdef __SYMBIAN32__

#include <CoeCntrl.h>
#include <CoeMain.h>
#include <apgtask.h>
#if defined __SYMBIAN_3RD__ && defined S60
#include <HWRMLight.h>
#endif

//----------------------------

class C_new_mail_notify_Symbian: public C_notify_window, public CCoeControl{
   RWindowGroup wg;
   dword bpp;

   CFbsBitmap *bbuf;
   C_application_base &app;

   virtual void HandlePointerEventL(const TPointerEvent &pe){
      switch(pe.iType){
      case TPointerEvent::EButton1Down:
         {
                              //activate our app
            TApaTaskList tl(CCoeEnv::Static()->WsSession());
            TUid id = { GetSymbianAppUid() };
            TApaTask at = tl.FindApp(id);
            if(at.Exists())
               at.BringToForeground();
         }
         break;
      }
   }

#ifdef _DEBUG_
   virtual IMPORT_C TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType){
      //Info("OKE", aType);
      return EKeyWasConsumed;
   }
#endif
public:
   C_new_mail_notify_Symbian(C_application_base &_app):
      app(_app),
      bpp(_app.GetPixelFormat().bits_per_pixel),
      bbuf(NULL)
   {
      width = 0;
   }
   ~C_new_mail_notify_Symbian(){
      //GetGlobalData()->GetAppUi()->RemoveFromStack(this);
      wg.Close();
      delete bbuf;
   }

   void ConstructL(){
      wg = RWindowGroup(iCoeEnv->WsSession());
      User::LeaveIfError(wg.Construct((TUint32)&wg, false));
      wg.EnableReceiptOfFocus(false);
      wg.SetOrdinalPosition(0, ECoeWinPriorityAlwaysAtFront);
      CreateWindowL(&wg);

      RWindow &win = Window();
      win.SetShadowHeight(1);
      win.SetShadowDisabled(false);

      //GetGlobalData()->GetAppUi()->AddToStackL(this);
   }

   virtual void Draw(const TRect &rc) const{
      CWindowGc &gc = SystemGc();
      if(bbuf)
         gc.BitBlt(TPoint(0, 0), bbuf);
   }

   virtual void SizeChanged(){
      CCoeControl::SizeChanged();
      delete bbuf;
      bbuf = new(true) CFbsBitmap;
      bbuf->Create(Size(), bpp==12 ? EColor4K : EColor64K);
   }

   virtual void DrawToBuffer(const S_rect &rc, const void *src, dword pitch){

      width = rc.sx;
      SetRect(TRect(rc.x, rc.y, rc.x+rc.sx, rc.y+rc.sy));
      assert(bbuf);
      if(bbuf){
#ifdef __SYMBIAN_3RD__
         bbuf->LockHeap();
#endif
         word *dst = (word*)bbuf->DataAddress();
         for(int y=rc.sy; y--; ){
            MemCpy(dst, src, rc.sx*sizeof(word));
            (word*&)src += pitch;
            dst += rc.sx;
         }
#ifdef __SYMBIAN_3RD__
         bbuf->UnlockHeap();
#endif
      }
      ActivateL();
      DrawDeferred();
   }
};

//----------------------------

C_notify_window *C_notify_window::Create(C_application_base &app){
   C_new_mail_notify_Symbian *mn = new(true) C_new_mail_notify_Symbian(app);
   mn->ConstructL();
   return mn;
}

//----------------------------
#if defined __SYMBIAN_3RD__ && defined S60 && defined USE_LED_BLINK

class C_led_flash: public C_unknown, public C_leave_func{
   //static const int NUM_COUNTS = 30*60*2;
   int counter;
   CHWRMLight *lp;
   CPeriodic *timer;
   int target;
   dword on_time;

//----------------------------

   void LightOff(){
      struct S_lf: public C_leave_func{
         CHWRMLight *lp;
         int target;
         virtual int Run(){
            lp->LightOffL(target);
            return 0;
         }
      } lf;
      lf.lp = lp;
      lf.target = target;
      lf.Execute();
   }

//----------------------------
                           //from C_leave_func:
   virtual int Run(){
      lp->LightOnL(target, on_time, KHWRMLightMaxIntensity, EFalse);
      //lp->LightOnL(target, on_time);
      //lp->LightBlinkL(target, KHWRMInfiniteDuration, 500, 1000, KHWRMLightMaxIntensity);
      return 0;
   }
   bool Tick(){
      int err = C_leave_func::Execute();
      if(err// || ++counter==NUM_COUNTS
         ){
         timer->Cancel();
         return false;
      }
      return true;
   }
   static TInt TickThunk(TAny *c){
      return ((C_led_flash*)c)->Tick();
   }
public:
   C_led_flash():
      counter(0),
      lp(NULL),
      target(CHWRMLight::ECustomTarget1),
      on_time(50),
      timer(NULL)
   {
      switch(system::GetDeviceId()){
      case 0x20029a73:  //Nokia N8
         target = CHWRMLight::ECustomTarget3;      //http://hex.ro/wp/blog/controlling-the-backlight-of-nokia-n8
         break;
      case 0x2002bf96: target = CHWRMLight::ECustomTarget2; on_time = 500; break; //E7-00
      case 0x20014ddd: case 0x20014dde: case 0x20023766: //n97
      case 0x2000da56:        //5800
         on_time = 500;
         break;
      }
   }
   void Construct(){
      struct S_lf: public C_leave_func{
         CHWRMLight *lp;
         virtual int Run(){
            lp = CHWRMLight::NewL();
            return 0;
         }
      } lf;
      lf.lp = NULL;
      if(!lf.Execute() && lf.lp){
         lp = lf.lp;
         LightOff();
      }
      if(lp){
         counter = 0;
         timer = CPeriodic::NewL(CActive::EPriorityStandard);
         timer->Start(1, 2000*1000, TCallBack(TickThunk, this));
      }
   }
   ~C_led_flash(){
      Close();
   }
   void Close(){
      delete timer; timer = NULL;
      if(lp){
         LightOff();
         delete lp;
         lp = NULL;
      }
   }
};


C_unknown *SymbianCreateLedFlash(){

#if defined __SYMBIAN_3RD__ && defined S60 && defined USE_LED_BLINK
   /*
   switch(system::GetDeviceId()){
   case 0x20002495:        //E50
   case 0x20002498:        //E51
   case 0x20014dcc:        //E52
   case 0x20001858:        //E61
   case 0x20002d7f:        //E61i
   case 0x20001859:        //E62
   case 0x200025c3:        //E63
   case 0x2000249c:        //E66
   case 0x2000249b:        //E71
   case 0x20014dd8:        //E71x
   case 0x20014dd0:        //E72
   case 0x2000249d:        //E75
   case 0x2000da56:        //5800
   case 0x20002d81:        //N78
   case 0x20014ddd:        //N97
   case 0x20014dde:        //N97a
   case 0x20023766:           //N97 mini
   case 0x20014dcf:        //E55
   case 0x2002bf91:        //C6-00
   case 0x20024100:        //E6-00
   //case 0x2000060b:        //N95
      {
      */
         C_led_flash *lf = new(true) C_led_flash;
         lf->Construct();
         return lf;
         /*
      }
      break;
   }
   */
#endif
   return NULL;
}

#else

C_unknown *SymbianCreateLedFlash(){ return NULL; }

#endif
//----------------------------

#endif //__SYMBIAN32__
//----------------------------

#if defined _WINDOWS || defined _WIN32_WCE
#include <String.h>
namespace win{
#include <Windows.h>
#if defined _WIN32_WCE
#include <aygshell.h>
#endif
}

class C_new_mail_notify_win: public C_notify_window{

#ifdef _WIN32_WCE
   win::HWND hwnd_ig;
   win::HWND hwnd_sink;

   static win::LRESULT CALLBACK WindowProc(win::HWND hwnd, win::UINT msg, win::WPARAM wParam, win::LPARAM lParam){

      switch(msg){
      case WM_CREATE:
         {
            win::CREATESTRUCT *cs = (win::CREATESTRUCT*)lParam;
            win::SetWindowLong(hwnd, GWL_USERDATA, (win::LPARAM)cs->lpCreateParams);
         }
         break;
         /*
      case TRAY_NOTIFYICON:
         switch(lParam){
         case WM_LBUTTONDOWN:
            if(wParam==ID_TRAY){
               C_new_mail_notify_win *_this = (C_new_mail_notify_win*)GetWindowLong(hwnd, GWL_USERDATA);
               ShowWindow(_this->hwnd_ig, SW_SHOW);
               SetForegroundWindow(_this->hwnd_ig);
            }
            break;
         }
         break;

      case WM_COMMAND:
         switch(LOWORD(wParam)){
         case 200:
            {
               C_new_mail_notify_win *_this = (C_new_mail_notify_win*)GetWindowLong(hwnd, GWL_USERDATA);
               ShowWindow(_this->hwnd_ig, SW_SHOW);
               SetForegroundWindow(_this->hwnd_ig);
            }
            break;
         case 201:
            return 1;
            break;
         }
         break;
         */

      case WM_NOTIFY:
         {
            win::NMSHN *nd = (win::NMSHN*)lParam;
            C_new_mail_notify_win *_this = (C_new_mail_notify_win*)win::GetWindowLong(hwnd, GWL_USERDATA);
            switch(nd->hdr.code){
            case SHNN_SHOW:
               //*
               {
                  win::ShowWindow(_this->hwnd_ig, SW_SHOW);
                  win::SetForegroundWindow(_this->hwnd_ig);
               }
               return 1;
            case SHNN_DISMISS:
               if(!nd->fTimeout)
                  _this->RemoveNotify(true);
               break;
               /**/
               //break;
            }
         }
         break;
      }
      return 0;
   }

/*
   void TrayMessage(dword dwMessage, HICON hIcon){

      NOTIFYICONDATA tnd;
      tnd.cbSize = sizeof(NOTIFYICONDATA);
      tnd.hWnd = hwnd_sink;
      tnd.uID = ID_TRAY;
      tnd.uFlags = NIF_MESSAGE | NIF_ICON;
      tnd.uCallbackMessage = TRAY_NOTIFYICON;
      tnd.hIcon = hIcon;
      tnd.szTip[0] = 0;
   
      Shell_NotifyIcon(dwMessage, &tnd);
   }

//----------------------------

public:
   C_new_mail_notify_win(HWND hwnd_p):
      hwnd_ig(hwnd_p)
   {
      width = 0;

      WNDCLASS wc;
      wc.style = CS_HREDRAW | CS_VREDRAW;
      wc.lpfnWndProc = (WNDPROC)WindowProc;
      wc.cbClsExtra = 0;
      wc.cbWndExtra = 0;
      wc.hInstance = GetModuleHandle(NULL);
      wc.hIcon = 0;
      wc.hCursor = 0;
      wc.hbrBackground = (HBRUSH)COLOR_BACKGROUND + 1;
      wc.lpszMenuName = NULL;
      wc.lpszClassName = class_name;
      UnregisterClass(wc.lpszClassName, wc.hInstance);
      RegisterClass(&wc);

      hwnd_sink = CreateWindow(wc.lpszClassName, class_name,
         //WS_VISIBLE |
         WS_POPUP,
         0, 0, 10, 10, hwnd_p, NULL, NULL, this);
      HICON hicon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(1));
      TrayMessage(NIM_ADD, hicon);
   }
   ~C_new_mail_notify_win(){
      TrayMessage(NIM_DELETE, NULL);
      DestroyWindow(hwnd_sink);
      UnregisterClass(class_name, GetModuleHandle(NULL));
   }
*/
   static win::LPGUID ClsID(){
      static win::GUID guid = {
         0x5ec102e3, 0x4867, 0x40da, { 0x81, 0x4f, 0x85, 0xd5, 0x56, 0x33, 0x28, 0x2e }
      };
      return &guid;
   }

   //SHNotificationUpdate

   Cstr_w class_name;

   void AddNotify(){
      if(!IsWMSmartphone()){
         if(!class_name.Length()){
            win::WNDCLASS wc;
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = (win::WNDPROC)WindowProc;
            wc.cbClsExtra = 0;
            wc.cbWndExtra = 0;
            wc.hInstance = win::GetModuleHandle(NULL);
            wc.hIcon = 0;
            wc.hCursor = 0;
            wc.hbrBackground = (win::HBRUSH)COLOR_BACKGROUND + 1;
            wc.lpszMenuName = NULL;
                                 //make unique class name (from app path)
            C_file::GetFullPath(L"Notify", class_name);
            wc.lpszClassName = class_name;
            win::UnregisterClass(wc.lpszClassName, wc.hInstance);
            win::RegisterClass(&wc);
         }
         hwnd_sink = win::CreateWindow(class_name, class_name,
            //WS_VISIBLE |
            WS_POPUP,
            0, 0, 10, 10, hwnd_ig, NULL, NULL, this);

         win::HICON hicon = win::LoadIcon(win::GetModuleHandle(NULL), (const wchar*)1);
         Cstr_w msg;
         msg.Format(
            L"% new message"
            )
            <<display_count;
         if(display_count>1)
            msg<<'s';
         /*
            L"You have % new messages.<br>"
            L"<a href=\"cmd:200\">Read mail</a><br>"
            L"<a href=\"cmd:201\">Close</a>"
            )
            <<num;
            */
         win::SHNOTIFICATIONDATA nd = {
            sizeof(nd),
            1,
            //SHNP_ICONIC,
            win::SHNP_INFORM,
            0,
            hicon,
            SHNF_STRAIGHTTOTRAY | SHNF_SILENT,
            {0},
            hwnd_sink,
            //NULL, NULL,
            msg, L"ProfiMail",
            NULL
         };
         nd.clsid = *ClsID();
         win::SHNotificationAdd(&nd);
      }else{
         /*http://store.handango.com/reportUnlockCode.jsp?requestId=1647BB7B-8D3A-4F15-A044-39B9C1F740B7 
#ifdef _DEBUG
         if(!class_name.Length()){
            win::WNDCLASS wc;
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = (win::WNDPROC)WindowProc;
            wc.cbClsExtra = 0;
            wc.cbWndExtra = 0;
            wc.hInstance = win::GetModuleHandle(NULL);
            wc.hIcon = 0;
            wc.hCursor = 0;
            wc.hbrBackground = (win::HBRUSH)COLOR_BACKGROUND + 1;
            wc.lpszMenuName = NULL;
                                 //make unique class name (from app path)
            C_file::GetFullPath(L"Notify", class_name);
            wc.lpszClassName = class_name;
            win::UnregisterClass(wc.lpszClassName, wc.hInstance);
            win::RegisterClass(&wc);
         }
         hwnd_sink = win::CreateWindow(class_name, class_name,
            //WS_VISIBLE |
            WS_POPUP,
            0, 0, 10, 10, hwnd_ig, NULL, NULL, this);
         win::HICON hicon = win::LoadIcon(win::GetModuleHandle(NULL), (const wchar*)1);

         win::NOTIFYICONDATA nd = {
            sizeof(nd),
            hwnd_sink,
            1,
            NIF_ICON,
            WM_USER,
            hicon,
            NULL
         };
         win::Shell_NotifyIcon(NIM_ADD, &nd);
#endif
         */
      }
   }

//----------------------------

   void RemoveNotify(bool also_not){
      if(hwnd_sink){
         if(also_not)
            win::SHNotificationRemove(ClsID(), 1);
         win::DestroyWindow(hwnd_sink);
         hwnd_sink = NULL;
      }
   }
public:
   C_new_mail_notify_win(win::HWND hwnd_p):
      hwnd_ig(hwnd_p),
      hwnd_sink(NULL)
   {
      //AddNotify();
      /*
#else
      NOTIFYICONDATA nd = {
         sizeof(nd),
         hwnd_sink,
         1,
         NIF_ICON,
         WM_USER,
         hicon,
         NULL
      };
      win::Shell_NotifyIcon(NIM_ADD, &nd);
      Vibrate(0, NULL, TRUE, INFINITE);
      ::Sleep(2000);
      VibrateStop(); 
#endif
      */
   }
   ~C_new_mail_notify_win(){
      //TrayMessage(NIM_DELETE, NULL);
      RemoveNotify(true);
      if(class_name.Length())
         win::UnregisterClass(class_name, win::GetModuleHandle(NULL));
   }
   virtual void ShowNotify(){
      RemoveNotify(true);
      AddNotify();
   }
#else
public:
   C_new_mail_notify_win(win::HWND hwnd_p)
   {}
#endif
};

//----------------------------

C_notify_window *C_notify_window::Create(C_application_base &app){
   return new(true) C_new_mail_notify_win((win::HWND)app.GetIGraph()->GetHWND());
}

#endif// _WINDOWS
//----------------------------
#ifdef ANDROID

#include <Android\GlobalData.h>

class C_new_mail_notify_android: public C_notify_window{

   jobject notify;
   jclass notify_class;
   dword curr_icon;

public:
   C_new_mail_notify_android():
      notify(NULL),
      notify_class(NULL)
   {
   }
   ~C_new_mail_notify_android(){
      RemoveNotify();
      JNIEnv &env = jni::GetEnv();
      if(notify_class)
         env.DeleteGlobalRef(notify_class);
   }

//----------------------------

   void RemoveNotify(){
      if(notify){
         JNIEnv &env = jni::GetEnv();
         env.CallStaticVoidMethod(notify_class, env.GetStaticMethodID(notify_class, "Remove", "(Landroid/content/Context;)V"), android_globals.java_context);
         env.DeleteGlobalRef(notify);
         notify = NULL;
      }
   }

//----------------------------

   virtual void ShowNotify(dword icon, const wchar *name, const wchar *title, const wchar *text, const wchar *activity_name, bool led_flash){
      JNIEnv &env = jni::GetEnv();
      if(!notify_class){
         notify_class = env.FindClass("com/lonelycatgames/Notify/Notify");
         notify_class = (jclass)env.NewGlobalRef(notify_class);
      }
      //Cstr_w s_title;
      //s_title.Format(L"New messages: %") <<int(display_count);
      if(notify && curr_icon!=icon){
                              //some changes, delete old notify
         RemoveNotify();         
      }
      curr_icon = icon;
      if(!notify){
                              //notify not present, create it now
         //LOG_RUN("create notify");
         jstring jname = jni::NewString(name);
         notify = env.CallStaticObjectMethod(notify_class, env.GetStaticMethodID(notify_class, "Add",
            "(Ljava/lang/String;IZ)Landroid/app/Notification;"),
            jname, icon, led_flash);
         notify = env.NewGlobalRef(notify);
         env.DeleteLocalRef(jname);
      }
                              //update notify texts
      //LOG_RUN("update notify");
      jstring jtitle = jni::NewString(title);
      jstring jtext = jni::NewString(text);
      jstring jactivity_name = jni::NewString(activity_name);

      env.CallStaticVoidMethod(notify_class, env.GetStaticMethodID(notify_class, "Update",
         "(Landroid/content/Context;Landroid/app/Notification;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V"),
         android_globals.java_context, notify, jtitle, jtext, jactivity_name);

      env.DeleteLocalRef(jtitle);
      env.DeleteLocalRef(jtext);
      env.DeleteLocalRef(jactivity_name);
   }
};


//----------------------------
C_notify_window *C_notify_window::Create(C_application_base &app){
   
   return new(true) C_new_mail_notify_android;
}
#endif
//----------------------------

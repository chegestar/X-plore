//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <UI\InfoMessage.h>

#if defined _WINDOWS && defined _DEBUG
namespace win{
#include "Windows.h"
}
#endif

//----------------------------

C_mode_info_message::C_mode_info_message(C_application_ui &_app, const Cstr_w &title, const wchar *msg, dword lsk_tid, dword rsk_tid, const char *img_name):
   super(_app, lsk_tid, rsk_tid),
   ctrl_img(NULL)
{
   mode_id = ID;
   tdi.body_w = msg;
   tdi.is_wide = true;

   SetTitle(title);
   if(img_name){
      Cstr_w fn; fn.Copy(img_name); fn<<L".png";
      ctrl_img = new(true) C_ctrl_image(this, fn, true);
      ctrl_img->SetAllowZoomUp(true);
      AddControl(ctrl_img);
   }
}

//----------------------------

const char *C_mode_info_message::GetImageName(E_INFO_MESSAGE_IMAGE img){

   switch(img){
   case INFO_MESSAGE_IMG_SUCCESS: return "InfoIcons\\Success";
   case INFO_MESSAGE_IMG_EXCLAMATION: return "InfoIcons\\Exclamation";
   case INFO_MESSAGE_IMG_QUESTION: return "InfoIcons\\Question";
   }
   return NULL;
}

//----------------------------
#ifdef ANDROID
#include <Android/GlobalData.h>

class C_mode_info_java: public C_mode_app<C_application_ui>{
   typedef C_mode_app<C_application_ui> super;
   jobject dlg;
public:
   C_mode_info_java(C_application_ui &_app, const Cstr_w &_title, const wchar *msg):
      super(_app)
   {
      app.RedrawScreen();
      static C_register_natives n_reg("com/lcg/App/LcgApplication", "JniInfoClose", "(I)V", (void*)&JniInfoClose);
      JNIEnv &env = jni::GetEnv();
      jstring jtitle = jni::NewString(_title);
      jstring jmsg = jni::NewString(msg);
      dlg = env.CallObjectMethod(android_globals.java_app, env.GetMethodID(android_globals.java_app_class, "CreateInfoMessage", "(ILjava/lang/String;Ljava/lang/String;)Landroid/app/AlertDialog;"),
         this, jtitle, jmsg);
      if(dlg)
         dlg = env.NewGlobalRef(dlg);
      env.DeleteLocalRef(jtitle);
      env.DeleteLocalRef(jmsg);
      if(dlg)
         Activate();
      else{
         LOG_RUN("Dialog not created");
      }
   }
   ~C_mode_info_java(){
      if(dlg){
         JNIEnv &env = jni::GetEnv();
         env.CallVoidMethod(dlg, env.GetMethodID(jni::GetObjectClass(dlg), "dismiss", "()V"));
         env.DeleteGlobalRef(dlg);
      }
   }
   virtual void InitLayout(){}
   //virtual void ProcessInput(S_user_input &ui, bool &redraw){}
   virtual void Draw() const{
      DrawParentMode(false);
   }
   static void JniInfoClose(JNIEnv *env, jobject thiz, C_mode_info_java *mod){
      mod->Close();
   }
};

#endif

//----------------------------

void CreateInfoMessage(C_application_ui &app, dword title_txt_id, const wchar *msg, E_INFO_MESSAGE_IMAGE img){

#if defined _WINDOWS && defined _DEBUG
   if(app.use_system_ui){
      app.UpdateScreen();
      dword flg = 0;
      switch(img){
      case INFO_MESSAGE_IMG_EXCLAMATION: flg = MB_ICONEXCLAMATION; break;
      case INFO_MESSAGE_IMG_QUESTION: flg = MB_ICONQUESTION; break;
      }
      win::MessageBox((win::HWND)app.GetIGraph()->GetHWND(), msg, app.GetText(title_txt_id), MB_OK);
      return;
   }
#endif
   CreateInfoMessage(app, title_txt_id, msg, C_mode_info_message::GetImageName(img));
}

//----------------------------

void CreateInfoMessage(C_application_ui &app, dword title_txt_id, const wchar *msg, const char *img_name){

   if(app.use_system_ui){
#if defined _WINDOWS && defined _DEBUG
      app.UpdateScreen();
                              //modal
      win::MessageBox((win::HWND)app.GetIGraph()->GetHWND(), msg, app.GetText(title_txt_id), MB_OK);
      return;
#endif
#ifdef ANDROID
      new(true) C_mode_info_java(app, app.GetText(title_txt_id), msg);
      return;
#endif
   }
   C_mode_info_message &mod = *new(true) C_mode_info_message(app, app.GetText(title_txt_id), msg, app.SPECIAL_TEXT_OK, 0, img_name);
   mod.tdi.ts.font_index = app.UI_FONT_BIG;
   mod.tdi.ts.text_color = app.GetColor(app.COL_TEXT_POPUP);
   mod.InitLayout();
   app.ActivateMode(mod);
}

//----------------------------

void C_mode_info_message::InitLayout(){

   super::InitLayout();
   const int img_size = !ctrl_img ? 0 : Min(app.fdb.line_spacing*2, rc_area.sx/5);
   rc = S_rect(0, 0, Min(app.ScrnSX()*7/8, app.fdb.cell_size_x*25), img_size);

   tdi.rc = S_rect(0, 0, rc.sx - (6+img_size), 0);
   if(ctrl_img)
      tdi.rc.sx -= app.fdb.cell_size_x;
   app.CountTextLinesAndHeight(tdi, app.UI_FONT_BIG);

   rc.sy = Min(app.ScrnSY()-app.fdb.line_spacing*5/2, Max(rc.sy, tdi.total_height));

   SetClientSize(rc.Size());
   if(ctrl_img){
      ctrl_img->SetRect(S_rect(rc_area.Right()-img_size-app.fdb.cell_size_x/2, rc_area.CenterY()-img_size/2, img_size, img_size));
   }
   tdi.rc.sy = rc_area.sy;
   tdi.rc.x = rc_area.x+app.fdb.cell_size_x/2;
   tdi.rc.y = rc_area.y;
}

//----------------------------

void C_mode_info_message::ProcessInput(S_user_input &ui, bool &redraw){

   super::ProcessInput(ui, redraw);
   switch(ui.key){
   case K_SOFT_SECONDARY:
   case K_BACK:
   case K_ESC:
   case K_SOFT_MENU:
   case K_ENTER:
      Close();
      return;
   }
}

//----------------------------

void C_mode_info_message::Draw() const{

   super::Draw();

   //app.DrawOutline(tdi.rc, 0x80ff0000);
   app.DrawFormattedText(tdi);
}

//----------------------------

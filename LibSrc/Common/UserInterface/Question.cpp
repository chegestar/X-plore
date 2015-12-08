//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <UI\Question.h>

//----------------------------

class C_question_base{
protected:
   C_question_callback *cb;
   bool delete_cb;

   C_question_base(C_question_callback *_cb, bool del):
      cb(_cb),
      delete_cb(del)
   {
   }
   ~C_question_base(){
      if(delete_cb)
         delete cb;
   }
};

//----------------------------

class C_mode_question: public C_mode_info_message, public C_question_base{
   typedef C_mode_info_message super;

   virtual void Close(bool redraw_screen){
                              //just ignore closing from super::ProcessInput
   }
public:
   C_mode_question(C_application_ui &_app, const Cstr_w &_title, const wchar *msg, dword _lsb_txt_id, dword _rsb_txt_id, C_question_callback *_cb, bool del):
      super(_app, _title, msg, _lsb_txt_id, _rsb_txt_id, GetImageName(INFO_MESSAGE_IMG_QUESTION)),
      C_question_base(_cb, del)
   {
      tdi.ts.font_index = app.UI_FONT_BIG;
      tdi.ts.text_color = app.GetColor(app.COL_TEXT_POPUP);
      //LoadImages(fn);
      InitLayout();
      Activate();
   }
//----------------------------
   virtual void ProcessInput(S_user_input &ui, bool &redraw){

      super::ProcessInput(ui, redraw);
      switch(ui.key){
      case K_SOFT_SECONDARY:
      case K_BACK:
      case K_SOFT_MENU:
      case K_ENTER:
      case K_ESC:
         {
            if(saved_parent)
               super::Close(false);
            else
               app.mode = NULL;
            if(ui.key==K_SOFT_MENU || ui.key==K_ENTER){
                                 //confirmed, perform asked action
               cb->QuestionConfirm();
            }else
               cb->QuestionReject();
            if(app.mode)
               app.RedrawScreen();
            else
               app.Exit();
         }
         return;
      }
   }
};

//----------------------------
#ifdef ANDROID
#include <Android/GlobalData.h>

class C_mode_question_java: public C_mode_app<C_application_ui>, public C_question_base{
   typedef C_mode_app<C_application_ui> super;
   jobject dlg;
public:
   C_mode_question_java(C_application_ui &_app, const Cstr_w &_title, const wchar *msg, bool is_html, dword _lsb_txt_id, dword _rsb_txt_id,
         C_question_callback *_cb, bool del):
      super(_app),
      C_question_base(_cb, del)
   {
      static C_register_natives n_reg("com/lcg/App/LcgApplication", "JniQuestionCallback", "(II)V", (void*)&JniQuestionCallback);

      JNIEnv &env = jni::GetEnv();
      jstring jtitle = jni::NewString(_title);
      jstring jmsg = jni::NewString(msg);
      dlg = env.CallObjectMethod(android_globals.java_app, 
         env.GetMethodID(android_globals.java_app_class, "CreateQuestion", "(IIILjava/lang/String;Ljava/lang/String;Z)Landroid/app/AlertDialog;"),
         this, _lsb_txt_id, _rsb_txt_id, jtitle, jmsg, is_html);
      env.DeleteLocalRef(jtitle);
      env.DeleteLocalRef(jmsg);
      if(dlg){
         dlg = env.NewGlobalRef(dlg);
         //CreateTimer(1000);
         Activate();
      }else{
         LOG_RUN("Question not created");
         _cb->QuestionReject();
      }
   }
   ~C_mode_question_java(){
      if(dlg){
         JNIEnv &env = jni::GetEnv();
         env.CallVoidMethod(dlg, env.GetMethodID(jni::GetObjectClass(dlg), "dismiss", "()V"));
         env.DeleteGlobalRef(dlg);
      }
   }
   //virtual void Tick(dword time, bool &redraw){ Close(true); } //test
   virtual void InitLayout(){}
   //virtual void ProcessInput(S_user_input &ui, bool &redraw){}
   virtual void Draw() const{
      DrawParentMode(false);
   }
   void QuestionCallback(int code){
      //LOG_RUN_N("c", code);
      AddRef();
      if(saved_parent)
         Close(false);
      else
         app.mode = NULL;
      if(cb)
      switch(code){
      case -1: cb->QuestionReject(); break;
      case -2: cb->QuestionConfirm(); break;
      }
      if(app.mode)
         app.RedrawScreen();
      else
         app.Exit();
      app.UpdateScreen();
      Release();
   }
   static void JniQuestionCallback(JNIEnv *env, jobject thiz, C_mode_question_java *mod, int code){
      mod->QuestionCallback(code);
   }
};

//----------------------------

void CreateQuestionAndroid(C_application_ui &app, const wchar *title, const wchar *msg, bool is_html, C_question_callback *cb, bool delete_cb, dword lsb_txt_id, dword rsb_txt_id){
   new(true) C_mode_question_java(app, title, msg, is_html, lsb_txt_id, rsb_txt_id, cb, delete_cb);
}

#endif
//----------------------------

void CreateQuestion(C_application_ui &app, dword title_txt_id, const wchar *msg, C_question_callback *cb, bool delete_cb, dword lsb_txt_id, dword rsb_txt_id){
#ifdef ANDROID
   if(app.use_system_ui){
      CreateQuestionAndroid(app, app.GetText(title_txt_id), msg, false, cb, delete_cb, lsb_txt_id, rsb_txt_id);
      return;
   }
#endif
   new(true) C_mode_question(app, app.GetText(title_txt_id), msg, lsb_txt_id, rsb_txt_id, cb, delete_cb);
}

//----------------------------

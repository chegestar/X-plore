//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <UI\UserInterface.h>
#include <UI\TextEntry.h>
#include <UI\InfoMessage.h>

//----------------------------
#define USE_CTRL

//----------------------------

class C_text_entry_base{
protected:
   C_text_entry_callback *cb;
   bool delete_cb;

   C_text_entry_base(C_text_entry_callback *_cb, bool del):
      cb(_cb),
      delete_cb(del)
   {
   }
   ~C_text_entry_base(){
      if(delete_cb)
         delete cb;
   }
};

//----------------------------

class C_mode_text_entry: public C_mode_dialog_base, public C_text_entry_base{
   typedef C_mode_dialog_base super;
#ifdef USE_CTRL
   C_ctrl_text_entry_line *ctrl_txt;
#ifdef _WIN32_WCE
   C_ctrl_softkey_bar skb;    //for processing kb button
#endif
#else
   C_smart_ptr<C_text_editor> te_edit;
#endif

   mutable C_smart_ptr<C_image> texted_bgnd;

public:

   C_mode_text_entry(t_type &_app, dword _title_txt_id, C_text_entry_callback *_cb, bool del, const wchar *init_text, dword max_text_len, dword tedf, dword _mode_id):
      super(_app, _app.SPECIAL_TEXT_OK, _app.SPECIAL_TEXT_CANCEL),
#ifdef _WIN32_WCE
      skb(this),
#endif
      C_text_entry_base(_cb, del)
   {
      SetTitle(_app.GetText(_title_txt_id));
      mode_id = _mode_id;

#ifdef USE_CTRL
      ctrl_txt = new(true) C_ctrl_text_entry_line(this, max_text_len, tedf | TXTED_ACTION_ENTER
//#ifdef _DEBUG
         //, TXTED_REAL_VISIBLE
//#endif
         );
      AddControl(ctrl_txt);
      if(tedf&TXTED_SECRET)
         //ctrl_txt->SetCase(C_text_editor::CASE_ALL);
         ctrl_txt->GetTextEditor()->SetCase(C_text_editor::CASE_ALL, C_text_editor::CASE_LOWER);
      else
      if(!(tedf&(TXTED_EMAIL_ADDRESS|TXTED_WWW_URL)))
         //ctrl_txt->SetCase(C_text_editor::CASE_ALL);
         ctrl_txt->GetTextEditor()->SetCase(C_text_editor::CASE_ALL, C_text_editor::CASE_LOWER);
      else
         ctrl_txt->SetCase(C_text_editor::CASE_LOWER);
#else
      te_edit = app.CreateTextEditor(tedf, app.UI_FONT_BIG, 0, init_text, max_text_len);
      te_edit->Release();
      if(tedf&TXTED_SECRET)
         te_edit->SetCase(C_text_editor::CASE_ALL, C_text_editor::CASE_LOWER);
      else
      if(!(tedf&(TXTED_EMAIL_ADDRESS|TXTED_WWW_URL)))
         te_edit->SetCase(C_text_editor::CASE_ALL, C_text_editor::CASE_ALL);
      else
         te_edit->SetCase(C_text_editor::CASE_ALL, C_text_editor::CASE_LOWER);
#endif
      InitLayout();
      ctrl_txt->SetText(init_text);
      SetFocus(ctrl_txt);
#ifdef _WIN32_WCE
      skb.SetActiveTextEditor(ctrl_txt->GetTextEditor());
#endif
      app.ActivateMode(*this);
   }
   virtual void InitLayout();
   virtual void ProcessInput(S_user_input &ui, bool &redraw);
   virtual void Draw() const;
   virtual void TextEditNotify(bool cursor_moved, bool text_changed, bool &redraw){
      //DrawTextArea();
      redraw = true;
#ifdef USE_CTRL
      if(text_changed)
         ctrl_txt->OnChange();
#endif
   }
//----------------------------

   virtual void InitButtons(){
      super::InitButtons();
      S_rect brc;
      if(buttons[0]){
         brc = buttons[0]->GetRect();
         brc.x = rc.x+2;
         buttons[0]->SetRect(brc);
      }
      if(buttons[1]){
         brc = buttons[1]->GetRect();
         brc.x = rc.Right()-brc.sx-2;
         buttons[1]->SetRect(brc);
      }
   }

//----------------------------

   void WindowMoved(){
      rc.x = (app.ScrnSX()-rc.sx)/2;

      const int title_sy = app.GetDialogTitleHeight();
      /*
      rc.y = Max(2, Min(rc.y, app.ScrnSY()-rc.sy-(int)app.GetSoftButtonBarHeight()-2));
      */
#ifdef USE_CTRL
      S_rect trc = ctrl_txt->GetRect();
#else
      S_rect trc = te_edit->GetRect();
#endif
      trc.x = rc.x+app.fdb.cell_size_x/2;
      trc.y = rc.y+title_sy+2;
#ifdef USE_CTRL
      ctrl_txt->SetRect(trc);
#else
      te_edit->SetRect(trc);
#endif
      //rc_texted_bgnd.x = rc.x;
      //rc_texted_bgnd.y = rc.y+title_sy;
      //rc_texted_bgnd.SetIntersection(rc_texted_bgnd, S_rect(0, 0, app.ScrnSX(), app.ScrnSY()));
   }
   void DrawTextArea(dword soft_keys[] = NULL) const;
   void Close();
};

//----------------------------

void C_mode_text_entry::Close(){

   C_mode *p = GetParent();
   AddRef();              //keep ref while mode is replaced
   app.mode = p;
   Release();
   if(p && p->timer)
      p->timer->Resume();
}

//----------------------------

void C_mode_text_entry::InitLayout(){

   /*
   const int title_sy = app.GetDialogTitleHeight();

   rc = S_rect(0, 0, Min(app.ScrnSX()*7/8, app.fdb.cell_size_x*30), title_sy + app.fdb.line_spacing*5/2);
   rc.Center(app.ScrnSX(), app.ScrnSY());

   */
   super::InitLayout();
   int sy = app.fdb.line_spacing*10/8;
   if(!HasButtonsInDialog())
      sy += app.fdb.line_spacing;
   SetClientSize(S_point(0, sy));
#ifdef USE_CTRL
   ctrl_txt->SetRect(S_rect(0, 0, rc.sx-app.fdb.cell_size_x, app.fdb.line_spacing+2));
#else
   C_text_editor &te = *te_edit;
   te.SetRect(S_rect(0, 0, rc.sx-app.fdb.cell_size_x, app.fdb.line_spacing));
#endif
   //rc_texted_bgnd = S_rect(0, 0, rc.sx, rc.sy-app.GetDialogTitleHeight());
   //rc_texted_bgnd = S_rect(0, 0, rc.sx, app.fdb.line_spacing);
   WindowMoved();

   texted_bgnd = NULL;
}

//----------------------------

void C_mode_text_entry::ProcessInput(S_user_input &ui, bool &redraw){

   super::ProcessInput(ui, redraw);
#ifdef USE_MOUSE
#ifndef USE_CTRL
   if(app.ProcessMouseInTextEditor(*te_edit, ui))
      redraw = true;
#endif
#ifdef _WIN32_WCE
   skb.ProcessInput(ui);
#endif

#endif
   switch(ui.key){
   case K_SOFT_SECONDARY:
   case K_BACK:
   case K_ESC:
      {
#ifdef USE_CTRL
         RemoveControl(ctrl_txt);
         ctrl_txt = NULL;
#else
         te_edit = NULL;
#endif
         AddRef();
         Close();
         cb->Canceled();
         if(!app.mode)
            app.Exit();
         else
            app.RedrawScreen();
         Release();
      }
      return;

   case K_SOFT_MENU:
   case K_ENTER:
      {
#ifdef USE_CTRL
         Cstr_w s = ctrl_txt->GetText();
         RemoveControl(ctrl_txt);
         ctrl_txt = NULL;
#else
         Cstr_w s = te_edit->GetText();
         te_edit = NULL;
#endif
         AddRef();
         Close();
         app.RedrawScreen();
         cb->TextEntered(s);
         if(!app.mode)
            app.Exit();
         Release();
      }
      return;
   default:
#ifdef USE_CTRL
      SetFocus(ctrl_txt);
#else
      te_edit->Activate(true);
#endif
   }
}

//----------------------------

void C_mode_text_entry::DrawTextArea(dword soft_keys[]) const{

   /*
   if(texted_bgnd)
      texted_bgnd->Draw(rc_texted_bgnd.x, rc_texted_bgnd.y);
   else
      app.FillRect(rc_texted_bgnd, app.GetColor(app.COL_LIGHT_GREY));
   */
#ifdef USE_CTRL
   ctrl_txt->Draw();
#else
   app.DrawEditedText(*te_edit, false, app.COL_LIGHT_GREY);
#endif
   //app.AddDirtyRect(rc_texted_bgnd);

#ifndef USE_CTRL
   dword lsk = app.SPECIAL_TEXT_OK, rsk = app.SPECIAL_TEXT_CANCEL;
   app.DrawTextEditorState(*te_edit, lsk, rsk, rc.Bottom()-app.fdb.line_spacing);
   if(soft_keys){
      soft_keys[0] = lsk;
      soft_keys[1] = rsk;
   }
#endif
}

//----------------------------

void C_mode_text_entry::Draw() const{

   super::Draw();
   dword sk[2];
   app.DrawTextEditorState(*ctrl_txt->GetTextEditor(), sk[0], sk[1], rc.Bottom()-app.fdb.line_spacing);
#ifdef USE_CTRL
#else
   app.DrawEditedTextFrame(te_edit->GetRect(), app.COL_LIGHT_GREY);
   DrawTextArea(NULL);
#endif
}

//----------------------------
#ifdef ANDROID
#include <Android/GlobalData.h>

class C_mode_text_entry_java: public C_mode_app<C_application_ui>, public C_text_entry_base{
   typedef C_mode_app<C_application_ui> super;
   jobject dlg;

   void Close(){

      C_mode *p = GetParent();
      AddRef();              //keep ref while mode is replaced
      app.mode = p;
      Release();
      if(p && p->timer)
         p->timer->Resume();
   }
public:
   C_mode_text_entry_java(C_application_ui &_app, const Cstr_w &_title, const wchar *init_text, C_text_entry_callback *_cb, bool del, dword _mode_id, dword tedf):
      super(_app),
      C_text_entry_base(_cb, del)
   {
      mode_id = _mode_id;
      static C_register_natives n_reg("com/lcg/App/LcgApplication", "JniTextEntryDone", "(ILjava/lang/String;)V", (void*)&JniTextEntryDone);

      JNIEnv &env = jni::GetEnv();
      jstring jtitle = jni::NewString(_title);
      jstring jinit = init_text ? jni::NewString(init_text) : NULL;
      dword ted_input_type = 0;

      if(tedf&TXTED_NUMERIC)
         ted_input_type = 2;      //TYPE_CLASS_NUMBER
      else{
         ted_input_type = 1;      //TYPE_CLASS_TEXT
         if(tedf&TXTED_ALLOW_EOL)
            ted_input_type |= 0x20000;  //TYPE_TEXT_FLAG_MULTI_LINE
         if(tedf&TXTED_ALLOW_PREDICTIVE)
            ted_input_type |= 0x8000;  //TYPE_TEXT_FLAG_AUTO_CORRECT
         else if(tedf&TXTED_EMAIL_ADDRESS)
            ted_input_type |= 0x20;  //TYPE_TEXT_VARIATION_EMAIL_ADDRESS
         else if(tedf&TXTED_WWW_URL)
            ted_input_type |= 0x10;  //TYPE_TEXT_VARIATION_URI
         else if(tedf&TXTED_SECRET)
            ted_input_type |= 0x80;  //  TYPE_TEXT_VARIATION_PASSWORD
      }


      dlg = env.CallObjectMethod(android_globals.java_app, env.GetMethodID(android_globals.java_app_class, "CreateTextEntry", "(ILjava/lang/String;Ljava/lang/String;I)Landroid/app/AlertDialog;"),
         this, jtitle, jinit, ted_input_type);
      dlg = env.NewGlobalRef(dlg);
      env.DeleteLocalRef(jtitle);
      if(jinit)
         env.DeleteLocalRef(jinit);
      Activate();
   }
   ~C_mode_text_entry_java(){
      JNIEnv &env = jni::GetEnv();
      env.CallVoidMethod(dlg, env.GetMethodID(jni::GetObjectClass(dlg), "dismiss", "()V"));
      env.DeleteGlobalRef(dlg);
   }
   virtual void InitLayout(){}
   //virtual void ProcessInput(S_user_input &ui, bool &redraw){}
   virtual void Draw() const{
      DrawParentMode(false);
   }
   void TextEntryDone(jstring txt){
      //LOG_RUN_N("c", code);
      AddRef();

      Close();
      if(txt){
         cb->TextEntered(jni::GetString(txt));
      }else
         cb->Canceled();
      if(!app.mode)
         app.Exit();
      else
         app.RedrawScreen();
      Release();
   }
   static void JniTextEntryDone(JNIEnv *env, jobject thiz, C_mode_text_entry_java *mod, jstring txt){
      mod->TextEntryDone(txt);
   }
};

#endif
//----------------------------

C_mode &CreateTextEntryMode(C_application_ui &app, dword title_txt_id, C_text_entry_callback *cb, bool delete_cb, dword max_text_len, const wchar *init_text,
   dword text_edit_flags, dword mode_id){

#ifdef ANDROID
   if(app.use_system_ui){
      return *new(true) C_mode_text_entry_java(app, app.GetText(title_txt_id), init_text, cb, delete_cb, mode_id, text_edit_flags);
   }
#endif
   return *new(true) C_mode_text_entry(app, title_txt_id, cb, delete_cb, init_text, max_text_len, text_edit_flags, mode_id);
}

//----------------------------

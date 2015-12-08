//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <UI\UserInterface.h>
#include <Ui\MultiQuestion.h>

//----------------------------

class C_multi_question_base{
protected:
   C_multi_selection_callback *cb;
   bool delete_cb;

   C_multi_question_base(C_multi_selection_callback *_cb, bool del):
   cb(_cb),
      delete_cb(del)
   {
   }
   ~C_multi_question_base(){
      if(delete_cb)
         delete cb;
   }
};

//----------------------------

class C_mode_multi_selection: public C_mode_dialog_base, public C_multi_question_base{
   typedef C_mode_dialog_base super;
public:
   class C_list: public C_list_mode_base{
      C_application_ui &app;
      virtual C_application_ui &AppForListMode() const{ return app; }
      virtual bool IsPixelMode() const{ return true; }
   public:
      C_mode_multi_selection *mod;
      C_buffer<Cstr_w> options;

      C_list(C_application_ui &_app):
         app(_app)
      {}

      virtual int GetNumEntries() const{ return options.Size(); }
      virtual void DrawContents() const{
         mod->DrawContents();
      }
   } list;

   S_text_display_info tdi;

   C_mode_multi_selection(C_application_ui &_app, const Cstr_w &_title, C_multi_selection_callback *_cb, bool _del_cb):
      super(_app, C_application_ui::SPECIAL_TEXT_OK, C_application_ui::SPECIAL_TEXT_CANCEL),
      C_multi_question_base(_cb, _del_cb),
      list(app)
   {
      SetTitle(_title);
      list.mod = this;
   }
   virtual void InitLayout();
   virtual void ProcessInput(S_user_input &ui, bool &redraw);
//----------------------------
   virtual void Draw() const{
      super::Draw();
      app.DrawFormattedText(tdi);
      if(tdi.body_w.Length())
         app.DrawThickSeparator(rc.x+app.fdb.cell_size_x*3, rc.sx-app.fdb.cell_size_x*6, list.rc.y-app.fdb.line_spacing/2);

      DrawContents();
   }
//----------------------------
   void DrawContents() const;
};


//----------------------------

void C_mode_multi_selection::InitLayout(){

   super::InitLayout();
   dword sx = app.ScrnSX();
   int sz_x = Min(int(sx)-app.fdb.letter_size_x*4, app.fdb.cell_size_x*30);
                              //initial rect (horizontal position is correct)
   rc = S_rect(0, 0, sz_x, 0);
                              //determine height of displayed text
   tdi.rc = S_rect(0, 0, rc.sx-4, 0);
   app.CountTextLinesAndHeight(tdi, app.UI_FONT_BIG);
                              //correct height and y
   rc.sy = Min(app.ScrnSY()-4, Max(rc.sy, tdi.total_height));
   if(tdi.body_w.Length())
      rc.sy += app.fdb.line_spacing;

   int opt_sy = list.options.Size() * list.entry_height;
   const int MAX_SY = app.ScrnSY() - app.fdb.line_spacing*4;
   list.sb.total_space = list.options.Size()*list.entry_height;
   list.sb.visible_space = list.sb.total_space;
   if(rc.sy+opt_sy > MAX_SY){
      list.sb.visible_space = MAX_SY-rc.sy;
      opt_sy = list.sb.visible_space;
   }
   rc.sy += opt_sy;
   SetClientSize(rc.Size());

   tdi.rc.sy = tdi.total_height;
   list.sb.pos = 0;

   tdi.rc.x = rc_area.x+2;
   tdi.rc.y = rc_area.y;

   int opts_y = tdi.rc.Bottom();
   if(tdi.body_w.Length())
      opts_y += app.fdb.line_spacing;
   list.rc = S_rect(rc_area.x, opts_y, rc_area.sx, Min(int(list.options.Size())*list.entry_height, list.sb.visible_space));

   list.sb.rc = S_rect(rc_area.Right()-app.GetScrollbarWidth()-2, opts_y, app.GetScrollbarWidth(), rc_area.Bottom()-opts_y-1);

   list.sb.SetVisibleFlag();
}

//----------------------------

void C_mode_multi_selection::ProcessInput(S_user_input &ui, bool &redraw){

   super::ProcessInput(ui, redraw);
   list.ProcessInputInList(ui, redraw);
   switch(ui.key){
   case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
   case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
      {
         if(list.options.Size()<2 || list.options.Size()>9)
            break;
         dword ii;
         if(ui.key>='1' && ui.key<='9')
            ii = ui.key - '1';
         else
            ii = ui.key - 'a';
         if(ii >= list.options.Size())
            break;
         list.selection = ii;
      }
   case K_LEFT_SOFT:
   case K_ENTER:
      {
         AddRef();
         if(saved_parent)
            Close(false);
         else
            app.mode = NULL;
         cb->Select(list.selection);
         app.RedrawScreen();
         Release();
      }
      return;
   case K_RIGHT_SOFT:
   case K_BACK:
   case K_ESC:
      {
         //AddRef();
         Close(false);
         /*
         app.mode = saved_parent;
         if(app.mode && app.mode->timer)
            app.mode->timer->Resume();
            */
         cb->Cancel();
         app.RedrawScreen();
         //Release();
      }
      return;
   }
}

//----------------------------

void C_mode_multi_selection::DrawContents() const{

   const dword col_text = app.GetColor(app.COL_TEXT_POPUP);
   app.DrawDialogBase(rc, false, &list.rc);
   int x = rc.x + app.fdb.cell_size_x/2;
   int max_x = list.GetMaxX();

                              //draw options
   S_rect rc_item;
   int item_index = -1;
   while(list.BeginDrawNextItem(rc_item, item_index)){
      dword color = col_text;
      if(item_index==list.selection){
         //S_rect rc(mod.rc.x, y, max_x-mod.rc.x, mod.list.entry_height);
         app.DrawSelection(rc_item);
         color = app.GetColor(app.COL_TEXT_HIGHLIGHTED);
      }
      int max_w = max_x - x - app.fdb.cell_size_x/2;
      int ty = rc_item.y;
      ty += (list.entry_height-app.fdb.line_spacing)/2;
      if(app.HasKeyboard()){
         if(!(list.options.Size()<2 || list.options.Size()>10)){
                                 //shortcut
            Cstr_w s; s.Format(L"[%]");
            if(app.HasFullKeyboard())
               s<<char('A'+item_index);
            else
               s<<(item_index+1);
            max_w -= app.DrawString(s, max_x-app.fdb.cell_size_x/2, ty, app.UI_FONT_BIG, FF_RIGHT, color) + app.fdb.cell_size_x;
         }
      }
      app.DrawString(list.options[item_index], x, ty, app.UI_FONT_BIG, 0, color, -max_w);
      //y += mod.list.entry_height;
   }
   list.EndDrawItems();
   app.DrawScrollbar(list.sb);
}

//----------------------------

#ifdef ANDROID
#include <Android/GlobalData.h>

class C_mode_multiq_java: public C_mode_app<C_application_ui>, public C_multi_question_base{
   typedef C_mode_app<C_application_ui> super;
   jobject dlg;
public:
   C_mode_multiq_java(C_application_ui &_app, const Cstr_w &_title, const wchar *msg, const wchar *const options[], dword num_options, C_multi_selection_callback *_cb, bool _del_cb):
      super(_app),
      C_multi_question_base(_cb, _del_cb)
   {
      static C_register_natives n_reg("com/lcg/App/LcgApplication", "JniMultiQCallback", "(II)V", (void*)&JniMultiQCallback);

      JNIEnv &env = jni::GetEnv();

      jobjectArray arr = env.NewObjectArray(num_options, jni::FindClass("java/lang/String"), NULL);
      for(dword i=0; i<num_options; i++){
         jstring s = jni::NewString(options[i]);
         env.SetObjectArrayElement(arr, i, s);
         env.DeleteLocalRef(s);
      }

      Cstr_w t = _title;
      if(msg && *msg)
         t<<'\n' <<msg;
      jstring jtitle = jni::NewString(t);
      dlg = env.CallObjectMethod(android_globals.java_app, env.GetMethodID(android_globals.java_app_class, "CreateMultiQuestion", "(ILjava/lang/String;[Ljava/lang/String;)Landroid/app/AlertDialog;"),
         this, jtitle, arr);
      dlg = env.NewGlobalRef(dlg);
      env.DeleteLocalRef(jtitle);
      env.DeleteLocalRef(arr);
      Activate();
   }
   ~C_mode_multiq_java(){
      JNIEnv &env = jni::GetEnv();
      env.CallVoidMethod(dlg, env.GetMethodID(jni::GetObjectClass(dlg), "dismiss", "()V"));
      env.DeleteGlobalRef(dlg);
   }
   virtual void InitLayout(){}
   //virtual void ProcessInput(S_user_input &ui, bool &redraw){}
   virtual void Draw() const{
      DrawParentMode(false);
   }

   void MultiQCallback(int sel){
      AddRef();
      //LOG_RUN_N("sel", sel);
      if(sel==-1){
         Close(false);
         cb->Cancel();
      }else{
         if(saved_parent)
            Close(false);
         else
            app.mode = NULL;
         cb->Select(sel);
      }
      app.RedrawScreen();
      Release();
   }

   static void JniMultiQCallback(JNIEnv *env, jobject thiz, C_mode_multiq_java *mod, int sel){
      mod->MultiQCallback(sel);
   }
};

#endif
//----------------------------

void CreateMultiSelectionMode(C_application_ui &app, dword title_txt_id, const wchar *msg, const wchar *const options[], dword num_options, C_multi_selection_callback *cb, bool del_cb){

#ifdef ANDROID
   if(app.use_system_ui){
      new(true) C_mode_multiq_java(app, app.GetText(title_txt_id), msg, options, num_options, cb, del_cb);
      return;
   }
#endif
   C_mode_multi_selection &mod = *new(true) C_mode_multi_selection(app, app.GetText(title_txt_id), cb, del_cb);

   mod.list.options.Resize(num_options);
   for(dword i=0; i<num_options; i++)
      mod.list.options[i] = options[i];
   mod.tdi.body_w = msg;
   mod.tdi.is_wide = true;
   mod.tdi.ts.font_index = app.UI_FONT_BIG;
   mod.tdi.ts.text_color = app.GetColor(app.COL_TEXT_POPUP);
   mod.list.entry_height = app.fdb.line_spacing;
   if(app.HasMouse())
      mod.list.entry_height *= 2;

   mod.InitLayout();
   mod.Activate();
}

//----------------------------

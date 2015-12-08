// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <UI\About.h>
#include <UI\FatalError.h>
#include <TextUtils.h>

//----------------------------

class C_mode_about: public C_mode_app<C_application_ui>{
   const dword app_version;
   const char *const build_date;
                              //key codes:
   C_about_keystroke_callback *const KeyStrokeCode;
   C_smart_ptr<C_text_editor> te_keystroke;
   dword key_code;
   int click_count;

   Cstr_w app_name;

   C_ctrl_image *img;
   C_ctrl_text_line *lines[4];

   virtual void InitLayout();
   virtual void ProcessInput(S_user_input &ui, bool &redraw);
   virtual void TextEditNotify(bool cursor_moved, bool text_changed, bool &redraw);

   virtual bool ProcessKeyCode(int code, bool &redraw);

   virtual dword GetMenuSoftKey() const{ return 0; }
public:
   enum{
      CTRL_IMAGE = 1,
      CTRL_APP_NAME,
      CTRL_BUILD_DATE,
      CTRL_COPYRIGHT,
      CTRL_WEB,
   };
   C_mode_about(C_application_ui &app, dword _txt_id_about, const wchar *_app_name, const wchar *_logo_filename, dword _app_version, C_about_keystroke_callback *ksc, const char *_build_date);
};

//----------------------------

C_mode_about::C_mode_about(C_application_ui &_app, dword txt_id_about, const wchar *_app_name, const wchar *_logo_filename, dword _app_version, C_about_keystroke_callback *ksc, const char *_build_date)
   :C_mode_app<C_application_ui>(_app),
   click_count(0),
   key_code(0),
   app_name(_app_name),
   build_date(_build_date),
   app_version(_app_version)
   ,KeyStrokeCode(ksc)
{
   img = (C_ctrl_image*)&AddControl(new C_ctrl_image(this, _logo_filename, true));
   img->SetAllowZoomUp(true);
   img->SetShadowMode(C_ctrl_image::SHADOW_YES_IF_OPAQUE);
   img->SetShrinkRectVertically(true);
   img->template_index = 0;

   Cstr_w buf;
   buf<<app.GetText(txt_id_about) <<L' ' <<app_name;
   SetTitle(buf);

   for(int i=0; i<4; i++){
      C_ctrl_text_line *ctrl = (C_ctrl_text_line*)&AddControl(new C_ctrl_text_line(this));
      lines[i] = ctrl;
      Cstr_w s;
      dword font_size = C_application_base::UI_FONT_SMALL;
      dword color = 0;
      dword fflg = 0;
      switch(i){
      case 0:
         s.Format(L"% %.#02%") <<app_name <<int(app_version>>16) <<int((app_version>>8)&0xff);
         if(app_version&0xff)
            s.AppendFormat(L".%") <<char(app_version&0xff);
         font_size = C_application_base::UI_FONT_BIG;
         break;
      case 1:
         s.Format(L"Build date: %") <<build_date;
         ctrl->SetAlpha(0x80);
         break;
      case 2: s = L"(c) Lonely Cat Games"; break;
      case 3: s = L"www.lonelycatgames.com";
         //color = 0xff8080ff;
         fflg |= FF_UNDERLINE;
         break;
      }
      ctrl->template_index = short(1+i);  //!!!
      ctrl->SetText(s, font_size, fflg | FF_CENTER, color);
   }

   InitLayout();
   app.ActivateMode(*this);
}

//----------------------------

enum E_LAYOUT_OPERATION{
   LAYOUT_OP_NO,
   //LAYOUT_OP_PLUS,
   //LAYOUT_OP_MINUS,
   LAYOUT_OP_MULTIPLY,
   //LAYOUT_OP_DIVIDE,
   LAYOUT_OP_PERCENT,
   LAYOUT_OP_PLUS_PTR,
   LAYOUT_OP_MINUS_PTR,
   LAYOUT_OP_MULTIPLY_PTR,
   //LAYOUT_OP_DIVIDE_PTR,
};

struct S_layout_left{
   enum{
      DIM_LEFT,
      DIM_CENTER,
      DIM_RIGHT,
      DIM_CENTER_BY_WIDTH,
      DIM_FONT_SMALL_WIDTH,
      DIM_FONT_BIG_WIDTH,
   } dim;
   E_LAYOUT_OPERATION operand;
   int parameter;
};

struct S_layout_width{
   enum{
      DIM_WIDTH,
      DIM_CENTER,
      DIM_FONT_SMALL_WIDTH,
      DIM_FONT_BIG_WIDTH,
   } dim;
   E_LAYOUT_OPERATION operand;
   int parameter;
};

struct S_layout_top{
   enum{
      DIM_TOP,
      DIM_LAST_BOTTOM,
      DIM_CENTER,
      DIM_BOTTOM,
      DIM_CENTER_BY_HEIGHT,
      DIM_FONT_SMALL_LINE,
      DIM_FONT_SMALL_LINE_HALF,
      DIM_FONT_BIG_LINE,
      DIM_FONT_BIG_LINE_HALF,
   } dim;
   E_LAYOUT_OPERATION operand;
   int parameter;
};

struct S_layout_height{
   enum{
      DIM_DEFAULT,            //control's default height
      DIM_HEIGHT,
      DIM_CENTER,
      DIM_FONT_SMALL_LINE,
      DIM_FONT_SMALL_LINE_HALF,
      DIM_FONT_BIG_LINE,
      DIM_FONT_BIG_LINE_HALF,
      DIM_NEXT_PTR,
   } dim;
   E_LAYOUT_OPERATION operand;
   int parameter;
};

//----------------------------

struct S_layout_item{
   enum E_CONTROL_TYPE{
      CTRL_NULL,
      CTRL_TEXT,
      CTRL_IMAGE,
   };
   E_CONTROL_TYPE ctrl;
   dword id;

   S_layout_left left;
   S_layout_width width;
   S_layout_top top;
   S_layout_height height;
};

//----------------------------

static const S_layout_height heigh_img[3] = {
                              //fds.line_spacing*5 + fdb.line_spacing
   {S_layout_height::DIM_NEXT_PTR, LAYOUT_OP_PLUS_PTR, int(heigh_img+2) },
   {S_layout_height::DIM_FONT_SMALL_LINE, LAYOUT_OP_MULTIPLY, 5 },
   {S_layout_height::DIM_FONT_BIG_LINE, LAYOUT_OP_MULTIPLY, 1 },
}, heigh_text_1[] = {
   {S_layout_height::DIM_FONT_SMALL_LINE_HALF, },
};

static const S_layout_item layout[] = {
   {
                              //image (logo)
      S_layout_item::CTRL_IMAGE, C_mode_about::CTRL_IMAGE,
      {S_layout_left::DIM_CENTER_BY_WIDTH }, {S_layout_width::DIM_WIDTH, LAYOUT_OP_PERCENT, 50 },
      {S_layout_top::DIM_FONT_SMALL_LINE_HALF }, {S_layout_height::DIM_HEIGHT, LAYOUT_OP_MINUS_PTR, int(heigh_img) }
   },
   {
      S_layout_item::CTRL_TEXT, C_mode_about::CTRL_APP_NAME,
      {S_layout_left::DIM_CENTER_BY_WIDTH }, {S_layout_width::DIM_WIDTH, LAYOUT_OP_PERCENT, 95 },
      {S_layout_top::DIM_LAST_BOTTOM, LAYOUT_OP_PLUS_PTR, int(heigh_text_1) }, {S_layout_height::DIM_DEFAULT }
   },
   {
      S_layout_item::CTRL_TEXT, C_mode_about::CTRL_BUILD_DATE,
      {S_layout_left::DIM_CENTER_BY_WIDTH }, {S_layout_width::DIM_FONT_BIG_WIDTH, LAYOUT_OP_MULTIPLY, 50 },
      {S_layout_top::DIM_LAST_BOTTOM }, {S_layout_height::DIM_DEFAULT }
   },
   {
      S_layout_item::CTRL_TEXT, C_mode_about::CTRL_COPYRIGHT,
      {S_layout_left::DIM_CENTER_BY_WIDTH }, {S_layout_width::DIM_WIDTH, LAYOUT_OP_PERCENT, 95 },
      {S_layout_top::DIM_LAST_BOTTOM }, {S_layout_height::DIM_DEFAULT }
   },
   {
      S_layout_item::CTRL_TEXT, C_mode_about::CTRL_WEB,
      {S_layout_left::DIM_CENTER_BY_WIDTH }, {S_layout_width::DIM_WIDTH, LAYOUT_OP_PERCENT, 95 },
      {S_layout_top::DIM_LAST_BOTTOM }, {S_layout_height::DIM_DEFAULT }
   },
   { S_layout_item::CTRL_NULL }
};

//----------------------------

void C_mode_about::InitLayout(){

   C_mode::InitLayout();

   const S_rect &rc_client = GetClientRect();
   /*
   int logo_sy = rc_client.sy-(app.fdb.line_spacing*1+app.fds.line_spacing*5);
   logo_sy = Max(20, logo_sy);
   const int logo_sx = app.ScrnSX()*4/8;
   img->SetRect(S_rect((app.ScrnSX()-logo_sx)/2, rc_client.y+app.fds.line_spacing/2, logo_sx, logo_sy));
   */

                              //init all controls from layout structure
   short template_index = 0;
   int last_bottom = rc_client.y;
   for(const S_layout_item *li = layout; li->ctrl; ++li, ++template_index){
                              //find the control
      C_ui_control *ctrl = NULL;
      for(int i=0; i<controls.size(); i++){
         if(controls[i]->template_index==template_index){
            ctrl = controls[i];
            break;
         }
      }
      if(!ctrl){
         assert(0);
         continue;
      }
                              //compute desired rectangle
      S_rect rc;
      {
         const S_layout_width &width = li->width;
         switch(width.dim){
         default: assert(0);
         case S_layout_width::DIM_WIDTH: rc.sx = rc_client.sx; break;
         case S_layout_width::DIM_CENTER: rc.sx = rc_client.sx/2; break;
         case S_layout_width::DIM_FONT_BIG_WIDTH: rc.sx = app.fdb.space_width; break;
         case S_layout_width::DIM_FONT_SMALL_WIDTH: rc.sx = app.fds.space_width; break;
         }
         switch(width.operand){
         default: assert(0);
         case LAYOUT_OP_NO: break;
         case LAYOUT_OP_PERCENT: rc.sx = rc.sx*width.parameter/100; break;
         case LAYOUT_OP_MULTIPLY: rc.sx *= width.parameter; break;
         }
      }
      {
         const S_layout_left &left = li->left;
         switch(left.dim){
         default: assert(0);
         case S_layout_left::DIM_LEFT: rc.x = 0; break;
         case S_layout_left::DIM_CENTER: rc.x = rc_client.sx/2; break;
         case S_layout_left::DIM_RIGHT: rc.x = rc_client.sx; break;
         case S_layout_left::DIM_CENTER_BY_WIDTH: rc.x = (rc_client.sx-rc.sx)/2; break;
         case S_layout_left::DIM_FONT_SMALL_WIDTH: rc.x = app.fds.space_width; break;
         case S_layout_left::DIM_FONT_BIG_WIDTH: rc.x = app.fdb.space_width; break;
         }
         switch(left.operand){
         default: assert(0);
         case LAYOUT_OP_NO: break;
         //case LAYOUT_OP_PLUS: rc.x += left.parameter; break;
         //case LAYOUT_OP_MINUS: rc.x -= left.parameter; break;
         case LAYOUT_OP_MULTIPLY: rc.x *= left.parameter; break;
         /*
         case LAYOUT_OP_DIVIDE:
         case LAYOUT_OP_PERCENT:
         case LAYOUT_OP_PLUS_PTR:
         case LAYOUT_OP_MINUS_PTR:
         case LAYOUT_OP_MULTIPLY_PTR:
         case LAYOUT_OP_DIVIDE_PTR:
         */
         }
         rc.x += rc_client.x;
      }
      struct S_hlp{
         static void EvalHeight(const S_layout_height &height, int &sy, const S_rect &_rc_client, const C_ui_control *_ctrl){
            const C_application_base &app = _ctrl->App();
            switch(height.dim){
            default: assert(0);
            case S_layout_height::DIM_DEFAULT: sy = _ctrl->GetDefaultHeight(); break;
            case S_layout_height::DIM_HEIGHT: sy = _rc_client.sy; break;
            case S_layout_height::DIM_CENTER: sy = _rc_client.sy/2; break;
            case S_layout_height::DIM_FONT_SMALL_LINE: sy = app.fds.line_spacing; break;
            case S_layout_height::DIM_FONT_SMALL_LINE_HALF: sy = app.fds.line_spacing/2; break;
            case S_layout_height::DIM_FONT_BIG_LINE: sy = app.fdb.line_spacing; break;
            case S_layout_height::DIM_FONT_BIG_LINE_HALF: sy = app.fdb.line_spacing/2; break;
            case S_layout_height::DIM_NEXT_PTR: EvalHeight(*(&height+1), sy, _rc_client, _ctrl); break;
            }
            switch(height.operand){
            default: assert(0);
            case LAYOUT_OP_NO: break;
            case LAYOUT_OP_PERCENT: sy = sy*height.parameter/100; break;
            case LAYOUT_OP_MULTIPLY: sy *= height.parameter; break;
            case LAYOUT_OP_PLUS_PTR:
            case LAYOUT_OP_MINUS_PTR:
               {
                  int tmp;
                  EvalHeight(*(S_layout_height*)height.parameter, tmp, _rc_client, _ctrl);
                  if(height.operand==LAYOUT_OP_PLUS_PTR)
                     sy += tmp;
                  else
                     sy -= tmp;
               }
               break;
            }
         }
      };
      S_hlp::EvalHeight(li->height, rc.sy, rc_client, ctrl);
      {
         const S_layout_top &top = li->top;
         switch(top.dim){
         default: assert(0);
         case S_layout_top::DIM_TOP: rc.y = 0; break;
         case S_layout_top::DIM_LAST_BOTTOM: rc.y = last_bottom - rc_client.y; break;
         case S_layout_top::DIM_CENTER: rc.y = rc_client.sy/2; break;
         case S_layout_top::DIM_BOTTOM: rc.y = rc_client.sy; break;
         case S_layout_top::DIM_CENTER_BY_HEIGHT: rc.y = rc.sy/2; break;
         case S_layout_top::DIM_FONT_SMALL_LINE: rc.y = app.fds.line_spacing; break;
         case S_layout_top::DIM_FONT_SMALL_LINE_HALF: rc.y = app.fds.line_spacing/2; break;
         case S_layout_top::DIM_FONT_BIG_LINE: rc.y = app.fdb.line_spacing; break;
         case S_layout_top::DIM_FONT_BIG_LINE_HALF: rc.y = app.fdb.line_spacing/2; break;
         }
         switch(top.operand){
         default: assert(0);
         case LAYOUT_OP_NO: break;
         //case LAYOUT_OP_PLUS: rc.x += left.parameter; break;
         //case LAYOUT_OP_MINUS: rc.x -= left.parameter; break;
         case LAYOUT_OP_MULTIPLY: rc.y *= top.parameter; break;
         case LAYOUT_OP_PLUS_PTR:
         case LAYOUT_OP_MINUS_PTR:
            {
               int tmp;
               S_hlp::EvalHeight(*(S_layout_height*)top.parameter, tmp, rc_client, ctrl);
               if(top.operand==LAYOUT_OP_PLUS_PTR)
                  rc.y += tmp;
               else
                  rc.y -= tmp;
            }
            break;

         /*
         case LAYOUT_OP_DIVIDE:
         case LAYOUT_OP_PERCENT:
         case LAYOUT_OP_PLUS_PTR:
         case LAYOUT_OP_MINUS_PTR:
         case LAYOUT_OP_MULTIPLY_PTR:
         case LAYOUT_OP_DIVIDE_PTR:
         */
         }
         rc.y += rc_client.y;
      }
      ctrl->SetRect(rc);
      last_bottom = ctrl->GetRect().Bottom();
   }
}

//----------------------------

void C_mode_about::TextEditNotify(bool cursor_moved, bool text_changed, bool &redraw){

   if(text_changed && te_keystroke->GetTextLength()==3){
      Cstr_c s; s.Copy(te_keystroke->GetText());
      te_keystroke->SetInitText(NULL);
      int code;
      if(text_utils::ScanInt(s, code)){
         if(ProcessKeyCode(code, redraw))
            app.CloseMode(*this);
      }
   }
}

//----------------------------

bool C_mode_about::ProcessKeyCode(int code, bool &redraw){

   bool close = false;
                        //evaluate code
   if(KeyStrokeCode)
      close = KeyStrokeCode->ProcessKeyCode(code);
   if(!close){
      if(code==328){
         CreateModeFatalError(&app, 902);
      }else if(code==376){
         app.ToggleUseClientRect();
      }
   }
   return close;
}

//----------------------------

void C_mode_about::ProcessInput(S_user_input &ui, bool &redraw){

   C_mode::ProcessInput(ui, redraw);
#ifdef USE_MOUSE
   if(te_keystroke && app.ProcessMouseInTextEditor(*te_keystroke, ui)){
      redraw = true;
   }else
   if(ui.mouse_buttons&MOUSE_BUTTON_1_DOWN){
      S_rect rc = img->GetImageRect();
      rc.sx = rc.sy = 0;
      rc.Expand((app.ScrnSX()+app.ScrnSY())/38);
      if(ui.CheckMouseInRect(rc)){
         if(++click_count==3){
            click_count = 0;
            if(!te_keystroke){
               te_keystroke = app.CreateTextEditor(TXTED_NUMERIC, 0, 0, NULL, 3);
               te_keystroke->Release();
               te_keystroke->InvokeInputDialog();
            }else
               te_keystroke = NULL;
            redraw = true;
            return;
         }
      }else
         click_count = 0;

      if(ui.CheckMouseInRect(lines[3]->GetRect())){
                              //open web link
         system::OpenWebBrowser(app, "http://www.lonelycatgames.com");
      }
   }
#endif
#ifdef _DEBUG_
   switch(ui.key){
   case K_CURSORLEFT:
   case K_CURSORRIGHT:
      static dword indx = 0;
      if(ui.key==K_CURSORLEFT){
         if(indx)
            --indx;
      }else{
         ++indx;
      }
      lines[0]->SetText(app.GetText(indx), app.UI_FONT_BIG, FF_CENTER);
      Cstr_w s; s<<int(indx);
      lines[2]->SetText(s);
      redraw = true;
      break;
   }
#endif

   if(ui.key>='0' && ui.key<='9'){
      key_code *= 10;
      key_code += ui.key-'0';
      if(key_code>=100){
         dword code = key_code;
         key_code = 0;
         if(ProcessKeyCode(code, redraw))
            app.CloseMode(*this);
      }
   }
}

//----------------------------
#ifdef ANDROID
#include <Android/GlobalData.h>
#include "..\..\Android\C++\JniPP.h"

class C_jni_About: public jni::Object{
   JB_LIVE_CLASS(C_jni_About);
   C_about_keystroke_callback *ksc;
public:
   C_jni_About(C_about_keystroke_callback *cb);
   bool OnSecretKeyEntered(int code){
      return ksc->ProcessKeyCode(code);
   }
};
typedef jni::ObjectPointer<C_jni_About> PC_jni_About;

#define JB_CURRENT_CLASS C_jni_About

JB_DEFINE_LIVE_CLASS("com/lcg/JniAbout",
   NoFields,
   Methods
      ( Ctr, "<init>", "()V" )
   ,
   Callbacks
      ( Method(OnSecretKeyEntered), "onSecretKeyEntered", "(I)Z" )
)

C_jni_About::C_jni_About(C_about_keystroke_callback *cb):
   jni::Object(JB_NEW(Ctr), GetInstanceFieldID()),
   ksc(cb)
{
}

#undef JB_CURRENT_CLASS

#endif

void CreateModeAbout(C_application_ui &app, dword txt_id_about, const wchar *title, const wchar *logo_filename, int app_version_hi, int app_version_lo,
   char app_build, C_about_keystroke_callback *ksc, const char *_build_date){

#ifdef ANDROID
   if(app.use_system_ui){
      JNIEnv &env = jni::GetEnv();
      jni::LClass cls_About = jni::FindClass("com/lonelycatgames/About");

      //(new C_jni_About(ksc))->GetJObject()
      PC_jni_About ja = PC_jni_About(new C_jni_About(ksc));

      jobject about = env.NewObject(cls_About, env.GetMethodID(cls_About, "<init>", "(Landroid/content/Context;Lcom/lonelycatgames/About$OnSecretKeyEnteredListener;)V"),
         android_globals.java_activity, ja.GetJObject());
      env.CallVoidMethod(about, env.GetMethodID(cls_About, "show", "()V"));
      env.DeleteLocalRef(about);
      return;
   }
#endif
   new(true) C_mode_about(app, txt_id_about, title, logo_filename, (app_version_hi<<16) | (app_version_lo<<8) | app_build, ksc, _build_date);
}

//----------------------------

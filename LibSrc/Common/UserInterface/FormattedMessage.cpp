//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <UI\FormattedMessage.h>

//----------------------------

void C_mode_formatted_message::Create(C_application_ui &app, const Cstr_w &title, const C_vector<char> &buf, C_question_callback *cb, bool del_cb, dword lsb, dword rsb){

   C_mode_formatted_message &mod = *new(true) C_mode_formatted_message(app, title, lsb, rsb, cb, del_cb);
   mod.ctrl->SetText(buf.begin(), buf.size());

   mod.InitLayout();

   mod.Activate();
}

//----------------------------

C_mode_formatted_message::C_mode_formatted_message(C_application_ui &_app, const Cstr_w &title, dword _lsb_txt_id, dword _rsb_txt_id, C_question_callback *_cb, bool _del_cb):
   super(_app, _lsb_txt_id, _rsb_txt_id),
   cb(_cb), del_cb(_del_cb)
{
   SetTitle(title);

   ctrl = new(true) C_ctrl_rich_text(this);
   AddControl(ctrl);
   ctrl->SetOutlineColor(0xfff0f0f0, 0x8000);
   SetFocus(ctrl);
}

//----------------------------

C_mode_formatted_message::~C_mode_formatted_message(){

   if(del_cb)
      delete cb;
}

//----------------------------

void C_mode_formatted_message::InitLayout(){

   super::InitLayout();

                              //prepare desired width
   S_point cl_sz;
   cl_sz.x = Min(app.ScrnSX()*7/8, app.fdb.cell_size_x*25);
   cl_sz.y = Min(app.ScrnSY()*6/8-app.fdb.line_spacing, app.fdb.line_spacing*10) - int(app.GetTitleBarHeight()-app.GetSoftButtonBarHeight());

   SetClientSize(cl_sz);
   ctrl->SetRect(rc_area.GetExpanded(-app.fdb.cell_size_x/2));
}

//----------------------------

void C_mode_formatted_message::ProcessInput(S_user_input &ui, bool &redraw){

   super::ProcessInput(ui, redraw);
   switch(ui.key){
   case K_SOFT_SECONDARY:
   case K_BACK:
   case K_ESC:
   case K_SOFT_MENU:
   case K_ENTER:
      {
         AddRef();

         if(saved_parent)
            Close(false);
         else
            app.mode = NULL;
         if(cb){
            if(ui.key==K_SOFT_MENU || ui.key==K_ENTER)
               cb->QuestionConfirm();
            else
               cb->QuestionReject();
         }
         if(app.mode)
            app.RedrawScreen();
         else
            app.Exit();
         Release();
         //CloseMode(mod);
      }
      return;
   }
}

//----------------------------

void CreateFormattedMessage(C_application_ui &app, const Cstr_w &title, const C_vector<char> &buf, C_question_callback *cb, bool del_cb, dword lsb_txt_id, dword rsb_txt_id){

#ifdef ANDROID
   if(app.use_system_ui){
      void CreateQuestionAndroid(C_application_ui &app, const wchar *title, const wchar *msg, bool is_html, C_question_callback *cb, bool delete_cb, dword lsb_txt_id, dword rsb_txt_id);

                              //convert formatted buf to simple html
      C_vector<wchar> html;
      html.reserve(buf.size());
      const char *cp = buf.begin(), *end = buf.end();
      static const wchar 
         br[] = L"<br>",
         font_color[] = L"<font color='",
         font_color_[] = L"'>",
         _font[] = L"</font>",
         lt[] = L"&lt;",
         gt[] = L"&gt;",
         amp[] = L"&amp;",
         b[] = L"<b>",
         _b[] = L"</b>";
#define ADD_HTML(s) html.insert(html.end(), s, s+sizeof(s)/2-1)
      S_text_style ts;
      while(cp<end){
         char c = *cp++;
         if(S_text_style::IsControlCode(c)){
            switch(c){
            case CC_WIDE_CHAR: html.push_back(S_text_style::ReadWideChar(cp)); break;
            case CC_TEXT_FONT_FLAGS:
               {
                  int font_flags = ts.font_flags;
                  ts.ReadCode(--cp, NULL);
                  font_flags ^= ts.font_flags;
                  while(font_flags){
                     if(font_flags&FF_BOLD){
                        if(ts.font_flags&FF_BOLD)
                           ADD_HTML(b);
                        else
                           ADD_HTML(_b);
                        font_flags &= ~FF_BOLD;
                     }else
                        LOG_RUN_N("CreateFormattedMessage: unknown flags", font_flags);
                  }
               }
               break;
            case CC_TEXT_COLOR:
               {
                  if(ts.text_color!=0xff000000)
                     ADD_HTML(_font);
                  ts.ReadCode(--cp, NULL);
                  if(ts.text_color!=0xff000000){
                     ADD_HTML(font_color);
                     Cstr_w col;
                     col.Format(L"#x%") <<(ts.text_color&0xffffff);
                     html.insert(html.end(), col, col+col.Length());
                     ADD_HTML(font_color_);
                  }
               }
               break;
            case CC_TEXT_FONT_SIZE:
               {
                  //if(ts.font_index!=-1)
                     //ADD_HTML(_font);
                  ts.ReadCode(--cp, NULL);
                  LOG_RUN_N("CreateFormattedMessage fs", ts.font_index);
                  /*if(ts.font_index!=-1){
                     ADD_HTML(font_color);
                     ADD_HTML(font_color_);
                  }*/
               }
               break;
            default:
               --cp;
               S_text_style::SkipCode(cp);
               LOG_RUN_N("CreateFormattedMessage: unknown style code", c);
            }
         }else{
            switch(c){
            case '\n': ADD_HTML(br); break;
            case '<': ADD_HTML(lt); break;
            case '>': ADD_HTML(gt); break;
            case '&': ADD_HTML(amp); break;
            default:
               html.push_back(c);
            }
         }
      }
      html.push_back(0);

      CreateQuestionAndroid(app, title, html.begin(), true, cb, del_cb, lsb_txt_id, rsb_txt_id);
      return;
   }
#endif
   C_mode_formatted_message::Create(app, title, buf, cb, del_cb, lsb_txt_id, rsb_txt_id);
}

//----------------------------

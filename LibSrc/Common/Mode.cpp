//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <Ui\UserInterface.h>

//----------------------------

C_mode::C_mode(C_application_ui &_app, C_mode *sm):
   saved_parent(sm),
   app(_app),
   mode_id(0),
   timer(NULL),
   title_bar(NULL),
   softkey_bar(this),
   last_socket_draw_time(0),
   socket_redraw_timer(NULL),
   ctrl_touched(NULL),
   ctrl_focused(NULL),
   softkey_bar_shown(false),
   any_ctrl_redraw(false),
   force_redraw(false)
{
   AddSoftkeyBar();
}

//----------------------------

void C_mode::SetTitle(const Cstr_w &t){

   if(!title_bar){
      title_bar = CreateTitleBar();
      if(title_bar)
         AddControl(title_bar);
   }
   if(title_bar)
      title_bar->SetText(t);
   /*
   if(t.Length()){
      if(!title_bar){
         title_bar = CreateTitleBar();
         AddControl(title_bar);
         InitLayout();
      }
      title_bar->SetText(t);
   }else{
      if(title_bar){
         RemoveControl(title_bar);
         title_bar = NULL;
         InitLayout();
      }
   }
   */
}

//----------------------------

C_smart_ptr<C_menu> C_mode::CreateMenu(dword menu_id){
   return app.CreateMenu(*this, menu_id);
}

//----------------------------

C_ui_control &C_mode::AddControl(C_ui_control *ctrl, bool release_ref){

   for(int i=controls.size(); i--; ){
      if(ctrl==controls[i]){
                              //already added!
         assert(0);
      }
   }
   controls.push_back(ctrl);
   if(release_ref)
      ctrl->Release();
   return *ctrl;
}

//----------------------------

bool C_mode::RemoveControl(const C_ui_control *ctrl){

   if(ctrl_focused==ctrl)
      SetFocus(NULL);
   if(ctrl_touched==ctrl)
      ctrl_touched = NULL;
   for(int i=controls.size(); i--; ){
      if(ctrl==controls[i]){
         controls.remove_index(i);
         return true;
      }
   }
   return false;
}

//----------------------------

void C_mode::SetFocus(C_ui_control *ctrl){
   if(ctrl_focused!=ctrl){
      if(ctrl_focused)
         ctrl_focused->SetFocus(false);
      ctrl_focused = ctrl;
      if(ctrl_focused){
                              //focus new control if we're active mode (otherwise it'll be SetFocus-ed in Activate)
         if(IsActive())
            ctrl_focused->SetFocus(true);
      }
   }
}

//----------------------------

dword C_mode::GetMenuSoftKey() const{ return C_application_ui::SPECIAL_TEXT_MENU; }
dword C_mode::GetSecondarySoftKey() const{ return C_application_ui::SPECIAL_TEXT_BACK; }

//----------------------------

const C_smart_ptr<C_zip_package> C_mode::CreateDtaFile() const{
   return app.CreateDtaFile();
}

//----------------------------

bool C_mode::IsActive() const{
   return(this == (C_mode*)app.mode);
}

//----------------------------

void C_mode::Activate(bool draw){
   if(app.mode){
      if(app.mode->timer && !app.mode->WantInactiveTimer())
         app.mode->timer->Pause();
      if(app.mode->GetFocus())
         app.mode->GetFocus()->SetFocus(false);
   }
   app.mode = this;
   Release();
                              //really SetFocus focused control now
   if(ctrl_focused)
      ctrl_focused->SetFocus(true);
   if(draw)
      app.RedrawScreen();
}

//----------------------------

void C_mode::Close(bool redraw_screen){
   SetFocus(NULL);
   C_mode *p = GetParent();
   C_application_ui &_app = app;
   _app.mode = p;
   if(p){
      if(p->timer)
         p->timer->Resume();
      if(p->GetFocus())
         p->GetFocus()->SetFocus(true);
      if(redraw_screen)
         _app.RedrawScreen();
   }else
      _app.Exit();
}

//----------------------------

void C_mode::CreateTimer(dword ms){
   assert(!timer);
   timer = app.C_application_base::CreateTimer(ms, this);
}

//----------------------------
/*
void C_mode::ResetTouchInput(){
   if(ctrl_touched)
      ctrl_touched->ResetTouchInput();
}
*/
//----------------------------

void C_mode::InitLayout(){

#ifdef UI_ALLOW_ROTATED_SOFTKEYS
   bool sk_right_aligned = (!app.HasMouse() && app.GetScreenRotation()==C_application_base::ROTATION_90CW);
   softkey_bar.right_aligned = sk_right_aligned;
#endif

   if(softkey_bar_shown)
      softkey_bar.InitLayout();

   if(!title_bar){
      title_bar = CreateTitleBar();
      if(title_bar)
         AddControl(title_bar);
   }
   int top = 0;
   if(title_bar){
#ifdef UI_ALLOW_ROTATED_SOFTKEYS
      title_bar->draw_soft_key = sk_right_aligned;
#endif
      title_bar->InitLayout();
      top = title_bar->rc.Bottom();
   }
   int bot = app.ScrnSY();
   if(softkey_bar_shown)
      bot -= softkey_bar.GetRect().sy;
   rc_area = S_rect(0, top, app.ScrnSX(), bot-top);

   /*
   for(int i=0; i<controls.size(); i++){
      controls[i]->InitLayout();
   }
   /**/
}

//----------------------------

void C_mode::DrawDirtyControls(){

   if(IsActive()){
      if(menu)
         force_redraw = true;
      if(force_redraw){
         force_redraw = false;
         Draw();
         if(menu)
            app.RedrawDrawMenuComplete(menu);
      }else
      if(any_ctrl_redraw){
                                 //some controls need to be redrawn
         any_ctrl_redraw = false;
#if 1
         for(int i=0; i<controls.size(); i++){
            C_ui_control *ctrl = controls[i];
            if(ctrl->need_draw){
               if(ctrl->IsTransparent()){
                  const S_rect &crc = ctrl->GetRect();
                  ClearToBackground(crc);
                  if(ctrl->overlapped){
                              //draw all overlapped controls below it
                     bool cr_set = false;
                     for(int j=0; j<i; j++){
                        C_ui_control *ctrl1 = controls[j];
                        if(!ctrl1->need_draw && ctrl1->GetRect().IsRectOverlapped(crc)){
                           if(!cr_set){
                              app.SetClipRect(crc);
                              cr_set = true;
                           }
                           DrawControl(ctrl1);
                        }
                     }
                     if(cr_set)
                        app.ResetClipRect();
                  }
               }
               DrawControl(ctrl);
            }
         }
         for(int i=0; i<controls.size(); i++)
            controls[i]->need_draw = false;
#else
         for(int i=0; i<controls.size(); i++){
            C_ui_control *ctrl = controls[i];
            if(ctrl->need_draw){
               ctrl->need_draw = false;
               if(ctrl->IsTransparent())
                  ClearToBackground(ctrl->GetRect());
               DrawControl(ctrl);
            }
         }
#endif
      }
   }
}

//----------------------------

void C_mode::ProcessInput(S_user_input &ui, bool &redraw){

#ifdef USE_MOUSE
   if(ui.mouse_buttons){
      if(ui.mouse_buttons&MOUSE_BUTTON_1_DOWN){
                              //touch down, perform hit test on all controls
         for(int i=controls.size(); i--; ){
            C_ui_control *ctrl = controls[i];
            if(ctrl->IsVisible() && ui.CheckMouseInRect(ctrl->GetRect())){
               ctrl_touched = ctrl;
               if(ctrl!=ctrl_focused && ctrl->IsTouchFocusable())
                  SetFocus(ctrl);
               break;
            }
         }
      }
      if(ctrl_touched){
         ctrl_touched->ProcessInput(ui);
         if(!menu && (ui.mouse_buttons&MOUSE_BUTTON_1_DOWN)){
            InitTouchMenu(ctrl_touched);
            if(menu){
               app.PrepareTouchMenu(menu, ui);
            }
         }
      }
      if(ui.mouse_buttons&MOUSE_BUTTON_1_UP)
         ctrl_touched = NULL;
   }
#endif
   if(ui.key && ctrl_focused)
      ctrl_focused->ProcessInput(ui);

   DrawDirtyControls();
                              //default processing of soft keys
   switch(ui.key){
   case K_SOFT_MENU:
   case K_MENU:
      if(GetMenuSoftKey()==C_application_ui::SPECIAL_TEXT_MENU){
         menu = app.CreateMenu(*this);
         InitMenu();
         app.PrepareMenu(menu);
      }
      break;
   case K_SOFT_SECONDARY:
   case K_BACK:
   case K_ESC:
      if(GetSecondarySoftKey()==C_application_ui::SPECIAL_TEXT_BACK){
         Close();
      }
      break;
   }
}

//----------------------------

void C_mode::Draw() const{

   force_redraw = false;
#ifdef _DEBUG
   app.ClearScreen(0xffff0000);
#endif
   app.ClearToBackground(rc_area);
   DrawAllControls();
}

//----------------------------

void C_mode::DrawAllControls() const{
   for(int i=0; i<controls.size(); i++){
      const C_ui_control *ctrl = controls[i];
      ctrl->need_draw = false;
      DrawControl(ctrl);
   }
}

//----------------------------
#ifdef _DEBUG
extern bool debug_draw_controls_rects;
bool debug_draw_controls_rects;
#endif

void C_mode::DrawControl(const C_ui_control *ctrl) const{
   if(ctrl->visible){
      ctrl->Draw();
#ifdef _DEBUG
      if(debug_draw_controls_rects){
         app.DrawOutline(ctrl->GetRect(), 0x80ff0000);
      }
#endif
   }
}

//----------------------------

void C_mode::DrawParentMode(bool wash_out) const{

   const C_mode *p = GetParent();
   if(p)
      p->Draw();
   else
      ClearToBackground(S_rect(0, 0, app.ScrnSX(), app.ScrnSY()));
   if(wash_out)
      app.DrawWashOut();
}

//----------------------------

void C_mode::ClearToBackground(const S_rect &rc) const{
   app.ClearToBackground(rc);
   /*
   if(1){
                              //draw all controls
   }
   */
}

//----------------------------

void C_mode::RemoveSoftkeyBar(){
   if(softkey_bar_shown){
      RemoveControl(&softkey_bar);
      softkey_bar_shown = false;
   }
}

//----------------------------

void C_mode::AddSoftkeyBar(){
   if(!softkey_bar_shown){
      AddControl(&softkey_bar, false);
      softkey_bar_shown = true;
   }
}

//----------------------------
//----------------------------

C_ui_control::C_ui_control(C_mode *m, dword _id):
   mod(*m),
   id(_id),
   need_draw(false),
   template_index(-1),
   visible(true),
   overlapped(false),
   rc(0, 0, 0, 0)
{}

//----------------------------

void C_ui_control::Draw() const{
#ifdef _DEBUG_
   App().DrawOutline(rc, 0x40000000, 0x40000000);
#endif
}

//----------------------------

void C_ui_control::SetVisible(bool v){
   visible = v;
   if(!v){
      if(mod.GetFocus()==this)
         mod.SetFocus(NULL);
   }
}

//----------------------------

void C_ui_control::MarkRedraw(){
   need_draw = true;
   mod.any_ctrl_redraw = true;
}

//----------------------------

C_application_base::C_timer *C_ui_control::CreateTimer(dword ms){
   return App().C_application_base::CreateTimer(ms, (void*)(dword(this)|3));
}

//----------------------------
//----------------------------

void C_control_outline::SetOutlineColor(dword col, dword contrast){
   outline_color = col;
   outline_contrast = contrast;
   MarkRedraw();
}

//----------------------------

void C_control_outline::Draw(const S_rect *draw_rc) const{

   if(outline_color){
      if(!draw_rc)
         draw_rc = &rc;
      dword a = 0xff000000&outline_color;
      dword lt = BlendColor(a, outline_color, outline_contrast);
      dword rb = BlendColor(a|0xffffff, outline_color, outline_contrast);
      App().DrawOutline(*draw_rc, lt, rb);
   }
}

//----------------------------
//----------------------------

C_ctrl_image::C_ctrl_image(C_mode *m, const Cstr_w &fn, bool _use_dta, dword _id):
   super(m, _id),
   use_dta(_use_dta),
   allow_zoom_up(false),
   layout_shrink_rect_sy(false),
   shadow_mode(SHADOW_NO),
   halign(HALIGN_CENTER),
   valign(VALIGN_CENTER),
   filename(fn)
{
   img = C_image::Create(App());
   img->Release();
}

//----------------------------

void C_ctrl_image::SetRect(const S_rect &_rc){

   S_rect prev = GetRect();
   super::SetRect(_rc);
   //if(prev.sx!=rc.sx || prev.sy!=rc.sy)
      img->Close();
   if(!img->SizeY()){
      C_smart_ptr<C_zip_package> dta = NULL;
      if(use_dta)
         dta = mod.CreateDtaFile();
      img->Open(filename, rc.sx, rc.sy, dta);
   }
                              //compute real image's rect
   rc_img = rc;
   if(!allow_zoom_up){
                              //image will be drawn in 100% zoom, just center
      rc_img.sx = img->SizeX();
      rc_img.sy = img->SizeY();
   }else{
      C_image::ComputeZoomedImageSize(img->SizeX(), img->SizeY(), rc_img.sx, rc_img.sy);
   }
   if(layout_shrink_rect_sy){
                              //reduce height to drawn image height
      rc.sy = Min(rc.sy, rc_img.sy);
   }
   switch(halign){
   case HALIGN_LEFT: break;
   case HALIGN_CENTER: rc_img.x += (rc.sx-rc_img.sx)/2; break;
   case HALIGN_RIGHT: rc_img.x = rc.Right()-rc_img.sx; break;
   }
   switch(valign){
   case VALIGN_TOP: break;
   case VALIGN_CENTER: rc_img.y += (rc.sy-rc_img.sy)/2; break;
   case VALIGN_BOTTOM: rc_img.y = rc.Bottom()-rc_img.sy; break;
   }
}

//----------------------------

void C_ctrl_image::SetHorizontalAlign(E_ALIGN_HORIZONTAL ha){
   halign = ha;
}

//----------------------------

void C_ctrl_image::SetVerticalAlign(E_ALIGN_VERTICAL va){
   valign = va;
}

//----------------------------

void C_ctrl_image::Draw() const{

   C_application_ui &app = App();
   super::Draw(&rc_img);
#ifdef _DEBUG_
   app.DrawOutline(rc_img, 0x40ff0000, 0x40ff0000);
#endif
   if(!allow_zoom_up){
      img->Draw(rc_img.x, rc_img.y);
   }else{
      img->DrawZoomed(rc_img);
   }
   switch(shadow_mode){
   case SHADOW_YES_IF_OPAQUE:
      if(img->IsTransparent())
         break;
   case SHADOW_YES:
      if(img->SizeY())
         app.DrawShadow(rc_img);
      break;
   }
}

//----------------------------
//----------------------------

void C_ctrl_text_line::SetText(const Cstr_w &t, dword _font_size, dword flags, dword _color){
   if(text!=t || draw_text_flags!=flags || font_size!=_font_size || color!=_color){
      text = t;
      draw_text_flags = flags;
      font_size = _font_size;
      color = _color;
      MarkRedraw();
   }
}

//----------------------------

void C_ctrl_text_line::Draw() const{

   C_application_ui &app = App();
   C_ui_control::Draw();

   dword col_text = color ? color : app.GetColor(app.COL_TEXT);
   int x = rc.x;
   if(draw_text_flags&FF_CENTER)
      x = rc.CenterX();
   if(draw_text_flags&FF_RIGHT)
      x = rc.Right();
   if(alpha!=0xff)
      col_text = MulAlpha(col_text, alpha<<8);
   app.DrawString(text, x, rc.y, font_size, draw_text_flags, col_text);
}

//----------------------------

dword C_ctrl_text_line::GetDefaultWidth() const{
   return App().GetTextWidth(text, font_size, draw_text_flags);
}

//----------------------------

dword C_ctrl_text_line::GetDefaultHeight() const{

   const S_font &fd = App().font_defs[font_size];
   //return fd.line_spacing;
   return fd.cell_size_y;
}

//----------------------------
//----------------------------

void C_ctrl_title_bar::InitLayout(){

   C_application_ui &app = App();
   rc = S_rect(0, 0, app.ScrnSX(), app.GetTitleBarHeight());
}

//----------------------------

void C_ctrl_title_bar::Draw() const{

   C_application_ui &app = App();
   super::Draw();
   app.DrawTitleBar(text, -1, false);
#ifdef UI_ALLOW_ROTATED_SOFTKEYS
   if(draw_soft_key){
      const wchar *str = app.GetText(
#ifdef UI_MENU_ON_RIGHT
         app.menu_on_right ? mod.GetMenuSoftKey() :
#endif
         mod.GetSecondarySoftKey());
      if(str){
         int x = app.ScrnSX()-1;
         dword flg = FF_BOLD;
         flg |= FF_RIGHT;
         flg |= 0x40<<FF_SHADOW_ALPHA_SHIFT;
         app.DrawString(str, x, 1, app.UI_FONT_BIG, flg, app.GetColor(app.COL_TEXT_SOFTKEY));
      }
   }
#endif//UI_ALLOW_ROTATED_SOFTKEYS
}

//----------------------------
//----------------------------

C_ctrl_softkey_bar::C_ctrl_softkey_bar(C_mode *m):
   C_ui_control(m, 0)
{
   ResetAllButtons();
}

//----------------------------

void C_ctrl_softkey_bar::ResetAllButtons(){
   for(int i=0; i<NUM_BUTTONS; i++){
      S_button_info &bi = button_info[i];
      bi.img_index = -1;
      bi.enabled = true;
      bi.txt_hint = 0;
   }
}

//----------------------------

void C_ctrl_softkey_bar::SetActiveTextEditor(C_text_editor *te){
   if(te!=curr_te){
      curr_te = te;
      MarkRedraw();
   }
}

//----------------------------

void C_ctrl_softkey_bar::InitLayout(){

   C_application_ui &app = App();
   int sbh = app.GetSoftButtonBarHeight();
   rc = S_rect(0, app.ScrnSY()-sbh, app.ScrnSX(), sbh);

   C_button *sb = soft_keys;
                           //init buttons rects
   S_rect brc = S_rect(1, 0, app.ScrnSX()/4-1, 0);
   int sy = sbh;
   if(!app.HasMouse())
      sy = sy*3/4;
   else
      sy = sy*7/8;
   brc.sy = sy - 1;
   brc.y = app.ScrnSY() - (brc.sy+1);
   sb[0].SetRect(brc);

   brc.x = app.ScrnSX()-brc.sx-1;
   sb[1].SetRect(brc);

#ifdef UI_ALLOW_ROTATED_SOFTKEYS
   if(right_aligned){
      sb[0] = sb[1];
      S_rect rc = sb[1].GetRect();
      rc.y = 1;
      sb[1].SetRect(rc);
#if defined S60 && defined __SYMBIAN_3RD__
      //check Nokia E90, swap names for it
      if(system::GetDeviceId()==0x20002496 && app.ScrnSX()==800)
         Swap(sb[0], sb[1]);
#endif
   }
#endif//UI_ALLOW_ROTATED_SOFTKEYS
#ifdef UI_MENU_ON_RIGHT
   if(app.menu_on_right){
      S_rect rc0 = sb[0].GetRect(), rc1 = sb[1].GetRect();
      sb[0].SetRect(rc1);
      sb[1].SetRect(rc0);
   }
#endif
}

//----------------------------
#ifdef _WIN32_WCE
#include "UserInterface\TextEditorCommon.h"
#endif

void C_ctrl_softkey_bar::ProcessInput(S_user_input &ui){

#ifdef USE_MOUSE
   C_application_ui &app = App();
   if(app.HasMouse()){
                              //process soft keys
      dword tid[2] = { mod.GetMenuSoftKey(), mod.GetSecondarySoftKey() };
      for(int i=0; i<2; i++){
         if(!tid[i])
            continue;
         bool redraw = false;
         if(soft_keys[i].ProcessMouse(ui, redraw, &app)){
                              //soft key pressed
            ui.key = !i ? K_SOFT_MENU : K_SOFT_SECONDARY;
         }
         if(redraw)
            MarkRedraw();
      }
      //if(!curr_te)
      {
                              //process bottom buttons
         const int HINT_DELAY = 500, HINT_TIME = 2500;
                              //check hint delays
         for(int i=0; i<NUM_BUTTONS; i++){
            const S_button_info &bi = button_info[i];
            if(bi.img_index==-1)
               continue;
            C_button &but = app.softkey_bar_buttons[i];
            if(!bi.enabled && (but.down || but.clicked)){
               but.down = but.clicked = false;
               MarkRedraw();
            }
            if(but.down){
               dword cnt = GetTickTime() - but.down_count;
               bool on = (cnt >= dword(HINT_DELAY) && cnt < dword(HINT_DELAY+HINT_TIME));
               if(on!=but.hint_on){
                  but.hint_on = on;
                  MarkRedraw();
               }
            }
            if(bi.enabled){
               bool redraw = false;
               bool clicked = but.ProcessMouse(ui, redraw, &app);
               if(redraw)
                  MarkRedraw();
               if(clicked){
                  mod.OnSoftBarButtonPressed(i);
               }
            }
         }
      }
   }
#ifdef _WIN32_WCE
   if(!IsWMSmartphone() && curr_te){
      bool redraw = false;
      bool click = app.but_keyboard.ProcessMouse(ui, redraw, &app);
      if(redraw)
         MarkRedraw();
      if(click){
         C_text_editor_common &tew = (C_text_editor_common&)*curr_te;
         tew.last_sip_on = !tew.last_sip_on;
         app.UpdateGraphicsFlags(tew.last_sip_on ? IG_SCREEN_SHOW_SIP : 0, IG_SCREEN_SHOW_SIP);
         tew.want_keyboard = tew.last_sip_on;
      }
   }
#endif//_WIN32_WCE

#endif
}

//----------------------------

void C_ctrl_softkey_bar::Draw() const{

   C_application_ui &app = App();
   C_ui_control::Draw();
   app.ClearSoftButtonsArea();

   const C_menu *mp = mod.GetMenu();
   if(mp && mp->IsAnimating())
      mp = NULL;
                              //draw soft keys
   if(mod.IsActive() && !mp && app.GetSoftButtonBarHeight()){
      const dword color = app.GetColor(app.COL_TEXT_SOFTKEY);

      dword tid[2] = { mod.GetMenuSoftKey(), mod.GetSecondarySoftKey() };
      if(curr_te && !mp){
         app.DrawTextEditorState(*curr_te, tid[0], tid[1]);
      }
      for(int i=0; i<2; i++){
         const C_button &but = soft_keys[i];
#ifdef UI_ALLOW_ROTATED_SOFTKEYS
         if(right_aligned && but.GetRect().y<rc.y) continue;
#endif
         const wchar *str = app.GetText(tid[i]);
         if(!str)
            continue;
         dword flg = FF_BOLD;
         int shadow_size = app.ScrnSX()/128;

         //int y = but.rc.y+1;
         int y = but.GetRect().CenterY() - app.fdb.line_spacing/2;
         y = Max(y, but.GetRect().y+1);
         int x = but.GetRect().x;
         if(but.GetRect().x>app.ScrnSX()/2
#ifdef UI_ALLOW_ROTATED_SOFTKEYS
            || right_aligned
#endif
            ){
            x = but.GetRect().Right();
            flg |= FF_RIGHT;
         }
         int max_w = 0;
         if(app.DrawSoftButtonRectangle(but)){
            x = but.GetRect().CenterX();
            flg &= ~FF_RIGHT;
            flg |= FF_CENTER;
            max_w = -but.GetRect().sx;
         }else
            app.AddDirtyRect(but.GetRect());
         if(but.down)
            ++x, ++y, --shadow_size;
         if(shadow_size>0)
            flg |= 0x40<<FF_SHADOW_ALPHA_SHIFT;
         app.DrawString(str, x, y, app.UI_FONT_BIG, flg, color, max_w);
      }
#ifdef USE_MOUSE
      if(app.HasMouse() && app.GetButtonsImage()){
                              //draw bottom buttons (app must have touch screen and inited button image)
         int hint_but = -1;
         for(int i=NUM_BUTTONS; i--; ){
            const S_button_info &bi = button_info[i];
            if(bi.img_index==-1)
               continue;
            const C_button &but = app.softkey_bar_buttons[i];

            if(but.down && but.hint_on)
               hint_but = i;

            app.DrawButton(but, bi.enabled);

                                    //draw image
            C_fixed alpha = C_fixed::Percent(bi.enabled ? 100 : 50);
            S_rect irc(0, 0, 0, app.GetButtonsImage()->SizeY());
            irc.sx = irc.sy;
            irc.x = irc.sx*bi.img_index;
            int dx = but.GetRect().CenterX() - irc.sx/2, dy = but.GetRect().CenterY() - irc.sy/2;
            if(but.down)
               ++dx, ++dy;
            app.GetButtonsImage()->DrawSpecial(dx, dy, &irc, alpha);
         }
         if(hint_but!=-1){
            dword tid_hint = button_info[hint_but].txt_hint;
            if(tid_hint){
               const C_button &but = app.softkey_bar_buttons[hint_but];
               const wchar *wp = app.GetText(tid_hint);
               S_rect hrc(Max(1, but.GetRect().x-10), but.GetRect().y-12, app.GetTextWidth(wp, app.UI_FONT_BIG)+2, app.fdb.cell_size_y);
               hrc.x = Min(hrc.x, app.ScrnSX()-hrc.sx-3);
               app.FillRect(hrc, 0xffffffff);
               app.DrawOutline(hrc, 0xff000000, 0xff000000);
               app.DrawString(wp, hrc.x+1, hrc.y, app.UI_FONT_BIG);
               app.DrawSimpleShadow(hrc, true);
            }
         }
      }
#endif
   }
}

//----------------------------
//----------------------------

C_ctrl_button_base::C_ctrl_button_base(C_mode *m, dword _id):
   super(m, _id),
   enabled(true)
{}

//----------------------------

void C_ctrl_button_base::OnClicked(){
   mod.OnButtonClicked(id);
}

//----------------------------

void C_ctrl_button_base::SetRect(const S_rect &_rc){
   super::SetRect(_rc);
   but.SetRect(_rc);
}

//----------------------------

void C_ctrl_button_base::ProcessInput(S_user_input &ui){

   if(!enabled)
      return;
   bool redraw = false;
   bool clicked = but.ProcessMouse(ui, redraw, &App());
   if(redraw)
      MarkRedraw();
   if(clicked){
      OnClicked();
   }
}

//----------------------------

void C_ctrl_button_base::SetEnabled(bool en){
   if(enabled!=en){
      enabled = en;
      MarkRedraw();
   }
}

//----------------------------

void C_ctrl_button_base::Draw() const{

   C_application_ui &app = App();
   app.DrawButton(but, enabled);
}

//----------------------------
//----------------------------

void C_ctrl_button::Draw() const{

   super::Draw();
   C_application_ui &app = App();
   int shadow_size = app.ScrnSX()/128;
   int x = rc.CenterX(), y = rc.y+(rc.sy-app.fdb.cell_size_y)/2;
   if(but.down)
      ++x, ++y,// --shadow_size;
      shadow_size = 0;
   dword flg = FF_CENTER|FF_BOLD;
   dword col = app.GetColor(app.COL_TEXT);
   if(!enabled){
      col = MulAlpha(col, 0xc000);
      shadow_size = 0;
   }
   if(shadow_size>0)
      flg |= 0x40<<FF_SHADOW_ALPHA_SHIFT;
   app.DrawString(text, x, y, app.UI_FONT_BIG, flg, col, -rc.sx);
}

//----------------------------

void C_ctrl_button::SetText(const Cstr_w &t){
   if(text!=t){
      text = t;
      MarkRedraw();
   }
}

//----------------------------

dword C_ctrl_button::GetDefaultWidth() const{
   C_application_ui &app = App();
   dword w = app.GetTextWidth(text, app.UI_FONT_BIG, FF_BOLD)+1;
   w += app.fdb.cell_size_x*2;
   return w;
}

//----------------------------

dword C_ctrl_button::GetDefaultHeight() const{
   //return App().fdb.line_spacing*3/2;
   return App().GetSoftButtonBarHeight()*3/4;
}

//----------------------------
//----------------------------

C_ctrl_image_button::C_ctrl_image_button(C_mode *m, const wchar *fname, dword _id):
   super(m, _id),
   filename(fname)
{
   img = C_image::Create(App());
   img->Release();
}

//----------------------------

void C_ctrl_image_button::SetRect(const S_rect &_rc){

   super::SetRect(_rc);
   but.SetRect(_rc);

   rc_img = rc;
   rc_img.Compact(Min(rc.sx, rc.sy)*2/8);

   img->Close();
   img->Open(filename, rc_img.sx, rc_img.sy, mod.CreateDtaFile());
                              //compute real image's rect
   //rc_img = rc;
   C_image::ComputeZoomedImageSize(img->SizeX(), img->SizeY(), rc_img.sx, rc_img.sy);
   rc_img.x = rc.CenterX()-rc_img.sx/2;
   rc_img.y = rc.CenterY()-rc_img.sy/2;

}

//----------------------------

void C_ctrl_image_button::Draw() const{

   super::Draw();
   //C_application_ui &app = App();
   //app.DrawOutline(rc_img, 0x80ff0000);
   img->DrawZoomed(rc_img);
}

//----------------------------
//----------------------------

C_ctrl_text_entry_base::C_ctrl_text_entry_base(C_mode *m, dword text_limit, dword tedf, dword font_size, dword _id):
   C_ui_control(m, _id)
{
   margin.x = margin.y = 0;
   te = App().CreateTextEditor(tedf | TXTED_CREATE_INACTIVE, font_size, 0, NULL, text_limit, this);
   te->Release();
   SetMargin(App().font_defs[font_size].cell_size_x/2, 0);
}

//----------------------------

void C_ctrl_text_entry_base::SetMargin(int x, int y){
   margin.x = x;
   margin.y = y;
}

//----------------------------

void C_ctrl_text_entry_base::SetFocus(bool focused){
   te->Activate(focused);
   if(!te->IsReadOnly()){
      C_ctrl_softkey_bar *sb = mod.GetSoftkeyBar();
      if(sb)
         sb->SetActiveTextEditor(focused ? (C_text_editor*)te : NULL);
   }
}

//----------------------------

void C_ctrl_text_entry_base::CursorBlinked(){
   MarkRedraw();
}

//----------------------------

void C_ctrl_text_entry_base::CursorMoved(){
   MarkRedraw();
   /*
   Draw();
   App().UpdateScreen();
   */
}

//----------------------------

void C_ctrl_text_entry_base::TextChanged(){
   MarkRedraw();
   /*
   Draw();
   App().UpdateScreen();
   */
}

//----------------------------

void C_ctrl_text_entry_base::SetRect(const S_rect &_rc){
   te->SetRect(_rc);
   C_ui_control::SetRect(_rc);
}

//----------------------------
//----------------------------

C_ctrl_text_entry_line::C_ctrl_text_entry_line(C_mode *m, dword text_limit, dword tedf, dword font_size, dword _id):
   C_ctrl_text_entry_base(m, text_limit, tedf, font_size, _id)
{
}

//----------------------------

bool C_ctrl_text_entry_line::IsTransparent() const{
   return App().IsEditedTextFrameTransparent();
}

//----------------------------

void C_ctrl_text_entry_line::SetCursorPos(int pos, int sel){
   C_ctrl_text_entry_base::SetCursorPos(pos, sel);
   App().MakeSureCursorIsVisible(*te);
}

//----------------------------

void C_ctrl_text_entry_line::Draw() const{
   C_application_ui &app = App();
   if(!(te->GetInitFlags()&TXTED_REAL_VISIBLE)){
      app.DrawEditedText(*te);
   }
}

//----------------------------

void C_ctrl_text_entry_line::ProcessInput(S_user_input &ui){
   //te->ProcessInput(ui);
   if(App().ProcessMouseInTextEditor(*te, ui))
      MarkRedraw();
}

//----------------------------
//----------------------------

C_ctrl_text_entry::C_ctrl_text_entry(C_mode *m, dword text_limit, dword tedf, dword font_size, C_ctrl_text_entry_notify *_notify, dword _id):
   C_ctrl_text_entry_base(m, text_limit, tedf | TXTED_ALLOW_EOL, font_size, _id),
   bgnd_color(0xffffffff),
   pos_cursor(0, 0), pos_min(0, 0), pos_max(0, 0),
   notify(_notify),
   sel_type(SEL_NO),
   mouse_selecting(false)
{
}

//----------------------------

void C_ctrl_text_entry::SetText(const wchar *text){
   C_ctrl_text_entry_base::SetText(text);
   lines.reserve(te->GetTextLength()/32);
   PrepareLines(true);
   MarkRedraw();
}

//----------------------------

void C_ctrl_text_entry::SetCursorPos(int pos, int sel){
   C_ctrl_text_entry_base::SetCursorPos(pos, sel);
   PrepareLines(true);
}

//----------------------------

void C_ctrl_text_entry::Cut(){
   C_ctrl_text_entry_base::Cut();
   PrepareLines(true);
}

//----------------------------

void C_ctrl_text_entry::Paste(){
   C_ctrl_text_entry_base::Paste();
   PrepareLines(true);
}

//----------------------------

void C_ctrl_text_entry::ReplaceSelection(const wchar *txt){
   C_ctrl_text_entry_base::ReplaceSelection(txt);
   PrepareLines(true);
}

//----------------------------

void C_ctrl_text_entry::SetRect(const S_rect &_rc){
   C_ctrl_text_entry_base::SetRect(_rc);
   int sb_width = App().GetScrollbarWidth();
   sb.rc = S_rect(rc.Right()-sb_width-margin.x/2, rc.y+margin.y, sb_width, rc.sy - margin.y*2);
   sb.visible_space = (rc.sy-margin.y*2);///App().font_defs[te->font_index].line_spacing;
   sb.visible = true;
   PrepareLines(true);
}

//----------------------------

void C_ctrl_text_entry::CursorMoved(){
   PrepareLines(true);
   C_ctrl_text_entry_base::CursorMoved();
}

//----------------------------

void C_ctrl_text_entry::TextChanged(){
   PrepareLines(true);
   C_ctrl_text_entry_base::TextChanged();
}

//----------------------------

void C_ctrl_text_entry::SetFocus(bool focused){
   C_ctrl_text_entry_base::SetFocus(focused);
   if(focused)
      PrepareLines(true);
}

//----------------------------

void C_ctrl_text_entry::ProcessInput(S_user_input &ui){

   C_application_ui &app = App();

   C_text_editor &te1 = *(this->te);

   C_scrollbar::E_PROCESS_MOUSE pm = app.ProcessScrollbarMouse(sb, ui);
   switch(pm){
   case C_scrollbar::PM_CHANGED:
      te1.scroll_y = sb.pos;
   case C_scrollbar::PM_PROCESSED:
      MarkRedraw();
      return;
   }
   //const dword rc_width = rc.sx - (app.GetScrollbarWidth()+2 + margin.x*2);
   if(mod.GetFocus()==this){
#if 1//def USE_MOUSE
      bool set_pos = false, set_sel = false, sel_word = false;

      te1.ProcessInput(ui);

      if(mouse_selecting){
         if(ui.mouse_buttons&MOUSE_BUTTON_1_DRAG){
            set_pos = true;
         }
         if(ui.mouse_buttons&MOUSE_BUTTON_1_UP){
            mouse_selecting = false;
            if(ui.CheckMouseInRect(rc) && te1.GetCursorPos()==te1.GetCursorSel()){
               te1.InvokeInputDialog();
            }
         }
      }else
      if(ui.mouse_buttons&MOUSE_BUTTON_1_DOWN){
         if(ui.CheckMouseInRect(rc)){
                                 //set active selection, and start multi-selection
            if(ui.is_doubleclick){
               mouse_selecting = false;
               sel_word = true;
            }else{
               if(!(ui.mouse_buttons&MOUSE_BUTTON_1_UP))
                  mouse_selecting = true;
               set_pos = set_sel = true;
            }
         }
      }
      const S_font &fd = app.font_defs[te1.font_index];
      if(set_pos || set_sel || sel_word){
                                 //determine line
         int line = (ui.mouse.y - rc.y + te1.scroll_y) / fd.line_spacing;
         PrepareLines(false);
         int pos = 0;
         int best_i = 0, best_w = 0x7fffffff;
         int len = 0;
         if(line < lines.size()){
            int i;
            for(i=0; i<line; i++)
               pos += lines[i].len;
            const wchar *wp = te->GetText() + pos;
                                 //determine character offset
            int xx = ui.mouse.x - te1.GetRect().x - 2;
                                 //ignore clicks out of left border
            xx = Max(0, xx);
            if(line<0){
               line = 0;
               xx = 0;
            }
            len = lines[line].len;
            if(len && wp[len-1]=='\n')
               --len;
            for(i=0; i<=len; i++){
               int w = !i ? 0 : app.GetTextWidth(wp, te1.font_index, te1.font_flags, i) + fd.letter_size_x/2;
               int delta = xx-w;
               if(delta>0){
                  if(delta>best_w)
                     break;
               }else
                  delta = -delta;
               if(best_w>delta){
                  best_w = delta;
                  best_i = i;
               }
            }
         }else{
                                 //beyond last line, set to end of text
            sel_word = false;
            set_pos = true;
            line = lines.size()-1;
            pos = te1.GetTextLength();
         }
         if(sel_word){
            const wchar *wp = te1.GetText() + pos;
            int n, x;
            for(n=best_i; n && wp[n-1]!=' '; --n);
            for(x=best_i; x<len; x++){
               if(wp[x]==' ' || wp[x]=='\n')
                  break;
            }
            te1.SetCursorPos(pos+x, pos+n);
            PrepareLines(true);
         }else{
            assert(set_pos);
            pos += best_i;
            te1.SetCursorPos(pos, set_sel ? pos : te1.GetCursorSel());
            PrepareLines(true);
         }
         te1.ResetCursor();
         MarkRedraw();
         CursorMoved();
      }
#endif//USE_MOUSE
      switch(ui.key){
      case K_CURSORUP:
      case K_CURSORDOWN:
         {
            int cursor_x_pixel = 0;
            PrepareLines(true);
            int cursor_line = 0;
            int cursor_col = te->GetCursorPos();
            for(int i=0; i<lines.size(); i++){
               const S_text_line &ln = lines[i];
               if(cursor_col<ln.len || cursor_line==lines.size()-1){
                  if(cursor_col)
                     cursor_x_pixel = app.GetTextWidth(ln.str, te->font_index, 0, cursor_col);
                  break;
               }
               ++cursor_line;
               cursor_col -= ln.len;
            }

            S_text_line ln;
            int cp;
            if(ui.key==K_CURSORUP){
               if(!cursor_line){
                  if(!(ui.key_bits&GKEY_SHIFT)){
                     if(notify)
                        notify->TextEntryCursorMovedUpOnTop(this);
                     return;
                  }
                                 //shift
                  if(!cursor_col)
                     return;
                  cp = 0;
                  cursor_x_pixel = 0;
                  ln = lines[0];
               }else{
                  ln = lines[cursor_line-1];
                  cp = te1.GetCursorPos() - cursor_col - ln.len;
               }
            }else{
               if(cursor_line>=lines.size()-1){
                  if(!(ui.key_bits&GKEY_SHIFT)){
                     if(notify)
                        notify->TextEntryCursorMovedDownOnBottom(this);
                     return;
                  }
                  cp = te1.GetTextLength();
                  cursor_x_pixel = 0;
                  ln = lines.back();
               }else{
                  ln = lines[cursor_line+1];
                  if(cursor_line+1 == lines.size()-1)
                     ++ln.len;
                  cp = te1.GetCursorPos() - cursor_col + lines[cursor_line].len;
               }
            }
            if(cursor_x_pixel && ln.len>1){
               int i;
               int last_delta = 0;
               for(i=1; i<ln.len-1; i++){
                  int xx = app.GetTextWidth(ln.str, te1.font_index, 0, i);
                  int d = cursor_x_pixel-xx;
                  if(xx>=cursor_x_pixel){
                     if(last_delta < -d)
                        --i;
                     break;
                  }
                  last_delta = d;
               }
               cp += i;
            }
            int sel = cp;
            if(ui.key_bits&GKEY_SHIFT)
               sel = te1.GetCursorSel();
            te1.SetCursorPos(cp, sel);
            PrepareLines(true);
            MarkRedraw();
            CursorMoved();
         }
         break;
      }
   }
}

//----------------------------

void C_ctrl_text_entry::PrepareLines(bool check_scroll){

   if(te->GetInitFlags()&TXTED_REAL_VISIBLE)
      return;
   C_application_ui &app = App();
   const S_font &fd = app.font_defs[te->font_index];
   lines.clear();
   pos_cursor = S_point(0, 0);
   pos_min = S_point(0, 0);
   pos_max = S_point(0, 0);

   const wchar *wp = te->GetText();
                              //determine selection (inline text is also considered to be kind of selection for rendering)
   int cp = te->GetCursorPos(), sel_min = cp, sel_max = te->GetCursorSel();
   if(sel_min>sel_max)
      Swap(sel_min, sel_max);
   sel_type = (sel_min != sel_max) ? SEL_NORMAL : SEL_NO;
   if(!sel_type){
      sel_max = te->GetInlineText(sel_min);
      if(sel_max){
         sel_max += sel_min;
         if(sel_min != sel_max)
            sel_type = SEL_INLINE;
      }
   }

   S_text_style ts; ts.font_index = te->font_index;
                              //count lines and determine their widths (in chars)
                              // also determine cursor/anchor line and row
   const dword rc_width = rc.sx - (app.GetScrollbarWidth()+2 + margin.x*2);
   while(true){
      int line_height;
      int w = app.ComputeFittingTextLength(wp, true, rc_width, NULL, ts, line_height, ts.font_index) / sizeof(wchar);
      assert(line_height==fd.line_spacing);
      if(cp>=0 && cp<=w){
         pos_cursor.y = lines.size();
         pos_cursor.x = cp;
      }
      cp -= w;
      if(sel_min>=0 && sel_min<=w){
         pos_min.y = lines.size();
         pos_min.x = sel_min;
      }
      sel_min -= w;
      if(sel_max>=0 && sel_max<=w){
         pos_max.y = lines.size();
         pos_max.x = sel_max;
      }
      sel_max -= w;

      S_text_line l = {wp, w};
      lines.push_back(l);
      if(!w || (!wp[w] && wp[w-1]!='\n'))
         break;
      wp += w;
   }
   sb.total_space = lines.size() * fd.line_spacing;

   if(check_scroll){
                              //check scrolling
      int &scroll_y = te->scroll_y;
                              //adjust top line, if necessary
      if(pos_cursor.y*fd.line_spacing < scroll_y){
                              //scroll down
         scroll_y = pos_cursor.y*fd.line_spacing;
      }else
      if((pos_cursor.y+1)*fd.line_spacing-sb.visible_space >= scroll_y){
                              //scroll up
         scroll_y = Max(0, (pos_cursor.y+1)*fd.line_spacing - sb.visible_space);
      }else
      if(scroll_y>(sb.total_space-sb.visible_space)){
         scroll_y = Max(0, sb.total_space-sb.visible_space);
      }
      sb.pos = scroll_y;
   }
   sb.SetVisibleFlag();
}

//----------------------------

void C_ctrl_text_entry::Draw() const{

   C_application_ui &app = App();
   if(te->GetInitFlags()&TXTED_REAL_VISIBLE)
      return;
   const bool draw_cursor = true;
                              //clear background
   app.FillRect(rc, bgnd_color);

   const dword font_index = te->font_index;
   const S_font &fd = app.font_defs[font_index];
   const dword rc_width = rc.sx - (app.GetScrollbarWidth()+2 + margin.x*2);

   const int x = rc.x + margin.x;
   S_rect rc_clip = rc;
   rc_clip.x += margin.x;
   rc_clip.y += margin.y;
   rc_clip.sx = rc_width;
   rc_clip.sy -= margin.y*2;
   //app.DrawOutline(rc_clip, 0x40ff0000);
   app.SetClipRect(rc_clip);

   int top_line = te->scroll_y/fd.line_spacing;
   int y = rc.y+margin.y;
   y -= te->scroll_y - top_line*fd.line_spacing;
   S_text_style ts; ts.font_index = font_index;

   S_point _pos_min = pos_min;
                              //draw visible lines
   for(int i=0; y<(rc.Bottom()-margin.y); i++, y+=fd.line_spacing){
      int li = i + top_line;
      if(li>=lines.size())
         break;
      const S_text_line &line = lines[li];
      const wchar *wp1 = line.str;
      int len = line.len;
      int xx = x;
                              //check if we're going to draw selection
      if(sel_type){
         if(_pos_min.y<=li){
            if(_pos_min.x && _pos_min.y==li){
               assert(_pos_min.x <= len);
                              //draw part of text before selection
               xx += app.DrawEncodedString(wp1, true, false, xx, y, COD_DEFAULT, ts, _pos_min.x*sizeof(wchar)) + 1;
               wp1 += _pos_min.x;
               len -= _pos_min.x;
            }
            _pos_min.y = 0x7fffffff;
            if(sel_type==SEL_NORMAL){
               ts.bgnd_color = 0xff0000ff;
               ts.text_color = 0xffffffff;
            }else{
               ts.font_flags |= FF_UNDERLINE;
               ts.text_color = 0xff0000ff;
            }
         }
         if(pos_max.y<=li){
            if(pos_max.x && pos_max.y==li){
                              //draw part of text before selection
               int n = pos_max.x - (wp1 - line.str);
               assert(n);
               xx += app.DrawEncodedString(wp1, true, false, xx, y, COD_DEFAULT, ts, n*sizeof(wchar)) + 1;
               wp1 += n;
               len -= n;
            }
            if(sel_type==SEL_NORMAL){
               ts.bgnd_color = 0;
               ts.text_color = 0xff000000;
            }else{
               ts.font_flags &= ~FF_UNDERLINE;
               ts.text_color = 0xff000000;
            }
         }
      }
      if(len)
         xx += app.DrawEncodedString(wp1, true, false, xx, y, COD_DEFAULT, ts, len*sizeof(wchar)) + 1;
      if(ts.bgnd_color){
                              //draw end-of-line selection
         app.DrawEncodedString(L" ", true, false, xx, y, COD_DEFAULT, ts, 1);
      }
                              //draw cursor
      if(draw_cursor && li==pos_cursor.y && te->IsCursorVisible() && mod.GetFocus()==this){
         int xx1 = x;
         if(pos_cursor.x)
            xx1 += app.GetTextWidth(line.str, font_index, 0, pos_cursor.x);
         app.FillRect(S_rect(xx1, y, 2, fd.cell_size_y), 0xffff0000);
      }
   }
   app.ResetClipRect();
   app.DrawScrollbar(sb);
}

//----------------------------

void C_ctrl_horizontal_line::Draw() const{

   C_application_ui &app = App();
   if(rc.sy==2)
      app.DrawThickSeparator(rc.x, rc.sx, rc.y);
   else{
      S_rect rc1 = rc;
      rc1.Compact();
      app.DrawOutline(rc1, 0x40000000, 0x80ffffff);
      app.FillRectGradient(rc1, 0x80ffffff, 0x40000000);
   }
}

//----------------------------

void C_ctrl_progress_bar::SetRect(const S_rect &_rc){
   super::SetRect(_rc);
   progress.rc = _rc;
}

//----------------------------

void C_ctrl_progress_bar::Draw() const{
   //super::Draw();
   App().DrawProgress(progress, 0, true, 0);
}

//----------------------------

void C_ctrl_progress_bar::SetPosition(int pos){
   progress.pos = pos;
   MarkRedraw();
}

//----------------------------

void C_ctrl_progress_bar::SetTotal(int total){
   progress.total = total;
   MarkRedraw();
}

//----------------------------
//----------------------------

C_ctrl_rich_text::C_ctrl_rich_text(C_mode *m):
   super(m),
   rc_set(false),
   dragging(false)
{}

//----------------------------

dword C_ctrl_rich_text::AddImage(const S_image_record &ir){
   tdi.images.push_back(ir);
   return tdi.images.size()-1;
}

//----------------------------

void C_ctrl_rich_text::SetText(const char *text, dword size, dword _font_index){

   tdi.body_c.Allocate(text, size);
   tdi.is_wide = false;
   tdi.ts.font_index = font_index = (byte)_font_index;
   if(rc_set){
      App().CountTextLinesAndHeight(tdi, font_index);
      sb.total_space = tdi.total_height;
      sb.SetVisibleFlag();
      MarkRedraw();
   }
}

//----------------------------

void C_ctrl_rich_text::ScrollToTop(){
   if(App().ScrollText(tdi, -tdi.total_height)){
      sb.pos = tdi.top_pixel;
      MarkRedraw();
   }
}

//----------------------------

void C_ctrl_rich_text::ScrollToBottom(){
   if(App().ScrollText(tdi, tdi.total_height)){
      sb.pos = tdi.top_pixel;
      MarkRedraw();
   }
}

//----------------------------

void C_ctrl_rich_text::SetRect(const S_rect &_rc){

   super::SetRect(_rc);
   C_application_ui &app = App();

   kinetic_movement.Reset();

   tdi.active_link_index = -1;
   tdi.active_object_offs = -1;
   tdi.byte_offs = 0;
   tdi.pixel_offs = 0;
   tdi.top_pixel = 0;
   tdi.rc.x = rc.x+app.fdb.letter_size_x/2;
   tdi.rc.y = rc.y;
   tdi.rc.sx = rc.sx - app.GetScrollbarWidth()-3;
   tdi.rc.sy = rc.sy;

   app.CountTextLinesAndHeight(tdi, font_index);

   sb.rc = S_rect(rc.Right()-app.GetScrollbarWidth()-3, rc.y+3, app.GetScrollbarWidth(), rc.sy-6);
   sb.pos = 0;
   sb.visible_space = tdi.rc.sy;
   sb.total_space = tdi.total_height;

   sb.SetVisibleFlag();
   rc_set = true;
}

//----------------------------

void C_ctrl_rich_text::ProcessInput(S_user_input &ui){

   C_application_ui &app = App();
   int scroll_pixels = 0;
   C_scrollbar::E_PROCESS_MOUSE pm = app.ProcessScrollbarMouse(sb, ui);
   switch(pm){
   case C_scrollbar::PM_PROCESSED:
      MarkRedraw();
      break;
   case C_scrollbar::PM_CHANGED:
      scroll_pixels = sb.pos - tdi.top_pixel;
      kinetic_movement.Reset();
      break;
   default:
      if(kinetic_movement.ProcessInput(ui, tdi.rc, 0, -1)){
         if(!kinetic_movement.timer)
            kinetic_movement.timer = CreateTimer(20);
      }
      if((ui.mouse_buttons&MOUSE_BUTTON_1_DOWN)// && ui.CheckMouseInRect(rc)
         )
         dragging = true;
      if((ui.mouse_buttons&MOUSE_BUTTON_1_DRAG) && dragging)
         scroll_pixels = -ui.mouse_rel.y;
      if(ui.mouse_buttons&MOUSE_BUTTON_1_UP)
         dragging = false;
   }
   switch(ui.key){
   case K_CURSORUP:
   case K_CURSORDOWN:
      if(ui.key_bits&GKEY_CTRL)
         ui.key = (ui.key==K_CURSORUP) ? '*' : '#';
   case K_PAGE_UP:
   case K_PAGE_DOWN:
   case '*':
   case '#':
   case 'q':
   case 'a':
      {
         kinetic_movement.Reset();
         scroll_pixels = app.fdb.line_spacing;
         if(ui.key=='*' || ui.key=='#' || ui.key=='q' || ui.key=='a' || ui.key==K_PAGE_UP || ui.key==K_PAGE_DOWN)
            scroll_pixels = sb.visible_space-app.fdb.line_spacing;
         if(ui.key==K_CURSORUP || ui.key=='*' || ui.key=='q' || ui.key==K_PAGE_UP)
            scroll_pixels = -scroll_pixels;
      }
      break;
   }
   if(scroll_pixels){
      if(app.ScrollText(tdi, scroll_pixels)){
         sb.pos = tdi.top_pixel;
         MarkRedraw();
      }
   }
}

//----------------------------

void C_ctrl_rich_text::Tick(dword time){

   if(kinetic_movement.IsAnimating()){
      S_point p;
      kinetic_movement.Tick(time, &p);

      if(App().ScrollText(tdi, p.y)){
         sb.pos = tdi.top_pixel;
         MarkRedraw();
      }else
         kinetic_movement.Reset();
   }else
      kinetic_movement.DeleteTimer();
}

//----------------------------

void C_ctrl_rich_text::Draw() const{

   super::Draw();
   C_application_ui &app = App();
   dword col_fill = 0xffc0c0c0;
   if(tdi.bgnd_color!=0xffffffff)
      col_fill = tdi.bgnd_color;
   app.FillRect(rc, 0xff000000 | col_fill);
   //app.FillRect(rc_info, col_fill);
   app.DrawFormattedText(tdi);
   app.DrawScrollbar(sb);
}

//----------------------------
//----------------------------

C_ctrl_list::C_ctrl_list(C_mode *m):
   super(m),
   dragging(false),
   entry_height(0),
   scroll_timer(NULL),
   one_click_mode(false),
   touch_down_selection(-1),
   selection(0)
{}

//----------------------------

C_ctrl_list::~C_ctrl_list(){
   DeleteScrollTimer();
}

//----------------------------

void C_ctrl_list::SetFocus(bool on){
   if(!on){
      dragging = false;
      touch_down_selection = -1;
   }
}

//----------------------------

dword C_ctrl_list::GetPageSize(bool up){

   return sb.visible_space/entry_height - 1;
}

//----------------------------

void C_ctrl_list::SetEntryHeight(int h){
   entry_height = h;
   MarkRedraw();
}

//----------------------------

void C_ctrl_list::SetRect(const S_rect &_rc){

   super::SetRect(_rc);
   C_application_ui &app = App();

   sb.rc = S_rect(rc.Right(), rc.y+2, app.GetScrollbarWidth(), rc.sy-4);
   sb.rc.x -= sb.rc.sx+2;
   sb.visible_space = rc.sy;
   sb.total_space = GetNumEntries() * entry_height;
   sb.SetVisibleFlag();
   EnsureVisible();
}

//----------------------------

void C_ctrl_list::ProcessInput(S_user_input &ui){

   int num_e = GetNumEntries();
   if(!num_e)
      return;
   C_application_ui &app = App();

   //int old_sel = selection;
//#ifdef USE_MOUSE
   {
      C_scrollbar::E_PROCESS_MOUSE pm = app.ProcessScrollbarMouse(sb, ui);
      switch(pm){
      case C_scrollbar::PM_PROCESSED:
      case C_scrollbar::PM_CHANGED:
         MarkRedraw();
         ui.mouse_buttons = 0;
         StopScrolling();
         ScrollChanged();
         break;
      default:
         if(ui.mouse_buttons&MOUSE_BUTTON_1_DOWN){
            app.MakeTouchFeedback(app.TOUCH_FEEDBACK_SELECT_LIST_ITEM);

            DeleteScrollTimer();
            //if(ui.CheckMouseInRect(rc))
            {
               assert(entry_height);
               int line = (ui.mouse.y - rc.y + sb.pos) / entry_height;
               if(line < num_e){
                  if(selection!=line){
                     int prev = selection;
                     selection = line;
                                 //fix scrolling, so that selected line is completely on screen
                     int sel_y = selection * entry_height;
                     if(sel_y < sb.pos)
                        sb.pos = sel_y;
                     else
                     if(sel_y+entry_height >= sb.visible_space+sb.pos){
                        sb.pos = sel_y-sb.visible_space+entry_height;
                     }
                     SelectionChanged(prev);
                     MarkRedraw();
                     if(one_click_mode)
                        touch_down_selection = selection;
                  }else
                     touch_down_selection = selection;
                  touch_move_mouse_detect = ui.mouse.y;
                  if(one_click_mode)
                     MarkRedraw();
               }
               dragging = true;
            }
         }
         if((ui.mouse_buttons&MOUSE_BUTTON_1_DRAG) && dragging){
                              //detect beginning of movement
            if(touch_move_mouse_detect!=-1){
               const int MOVE_TRESH = app.fdb.line_spacing;
               int d = touch_move_mouse_detect - ui.mouse.y;
               if(Abs(d) >= MOVE_TRESH){
                  assert(Abs(d)<0x10000);
                  touch_down_selection = -1;
                  touch_move_mouse_detect = -1;
                  ui.mouse_rel.y = -d;
                  assert(Abs(ui.mouse_rel.y)<0x10000);
               }
            }
            if(touch_move_mouse_detect==-1){
               int pos = sb.pos - ui.mouse_rel.y;
               pos = Max(0, Min(sb.total_space-sb.visible_space, pos));
               if(sb.pos!=pos){
                  sb.pos = pos;
                  MarkRedraw();
                  ScrollChanged();
                                 //if moved too much, don't allow activating the item
                  //if(Abs(int(kinetic_scroll.beg)-sb.pos) >= entry_height/2){
                    // touch_down_selection = -1;
                  //}
               }
            }
         }
         if(ui.mouse_buttons&MOUSE_BUTTON_1_UP){
            if(touch_move_mouse_detect!=-1)
               kinetic_scroll.Reset();
            dragging = false;
         }
         if(app.IsKineticScrollingEnabled()){
            if(kinetic_scroll.ProcessInput(ui, rc, 0, -1)){
               if(!scroll_timer)
                  scroll_timer = CreateTimer(20);
               kinetic_scroll.SetAnimStartPos(0, sb.pos);
            }else if(ui.mouse_buttons&MOUSE_BUTTON_1_UP){
               if(touch_down_selection==selection)
                  ui.key = K_ENTER;
            }
         }else
         if(ui.mouse_buttons&MOUSE_BUTTON_1_UP){
            if(touch_down_selection==selection)
               ui.key = K_ENTER;
         }
         if(ui.mouse_buttons&MOUSE_BUTTON_1_UP){
            if(touch_down_selection!=-1){
               if(one_click_mode)
                  ui.key = K_ENTER;
               touch_down_selection = -1;
            }//else
               //MarkRedraw();
         }
      }
   }
//#endif

   dword key = ui.key;
   if(ui.key_bits&GKEY_CTRL)
   switch(key){
   case K_CURSORUP: key = '*'; break;
   case K_CURSORDOWN: key = '#'; break;
   }

   if(key){
      StopScrolling();
      //app.ui_touch_mode = false;
   }
   switch(key){
#ifdef _DEBUG_
   case 't':
      if(!scroll_timer){
         scroll_timer = ((C_application_base&)app).CreateTimer(20, (void*)(dword((C_list_mode_base*)this)|2));
      }
      kinetic_scroll.length = entry_height*9;
      if(ui.key_bits&GKEY_CTRL)
         kinetic_scroll.length = -kinetic_scroll.length;
      kinetic_scroll.time = 0;
      kinetic_scroll_beg = sb.pos;
      break;
#endif
   case K_CURSORUP:
   case K_CURSORDOWN:
      {
         int old_sel1 = selection;
         if(key==K_CURSORUP){
            if(!selection)
               selection = num_e;
            --selection;
         }else{
            if(++selection==num_e)
               selection = 0;
         }
         if(selection!=old_sel1){
            EnsureVisible();
            SelectionChanged(old_sel1);
            MarkRedraw();
         }
      }
      break;

   case K_HOME:
   case K_END:
      {
         int s = selection;
         s = ui.key==K_HOME ? 0 : num_e-1;
         if(selection!=s){
            int old_sel1 = selection;
            selection = s;
            EnsureVisible();
            SelectionChanged(old_sel1);
            MarkRedraw();
         }
      }
      break;

   case K_PAGE_UP:
   case K_PAGE_DOWN:
   case '*':
   case '#':
   case 'q':
   case 'a':
      {
         int s = selection;
         bool up = (key=='*' || key=='q' || key==K_PAGE_UP);
         int add = GetPageSize(up);
         if(up)
            add = -add;
         s += add;
         s = Max(0, Min(num_e-1, s));
         if(selection!=s){
            int old_sel1 = selection;
            selection = s;
            EnsureVisible();
            SelectionChanged(old_sel1);
            MarkRedraw();
         }
      }
      break;
   }
}

//----------------------------

void C_ctrl_list::Tick(dword time){

   if(kinetic_scroll.Tick(time))
      DeleteScrollTimer();
   int sb_pos = kinetic_scroll.GetCurrPos().y;
   sb.pos = Max(0, Min(sb.total_space-sb.visible_space, sb_pos));
   if(sb.pos!=sb_pos)
      StopScrolling();
   ScrollChanged();
   MarkRedraw();
}

//----------------------------

void C_ctrl_list::Draw() const{

   super::Draw();
   int num_e = GetNumEntries();
   if(!num_e || !entry_height)
      return;
   C_application_ui &app = App();

   S_rect crc = app.GetClipRect();
   S_rect nrc;
   nrc.SetIntersection(crc, rc);
   if(nrc.IsEmpty())
      return;
   app.SetClipRect(nrc);

   const int max_x = GetMaxX();
   const bool want_draw_selection = (!app.IsUiTouchMode() || mod.menu || !one_click_mode);
                              //compute item rect
   S_rect rc_item = rc;
   rc_item.sx = GetMaxX()-rc.x;
   rc_item.sy = entry_height;
                                 //pixel offset
   rc_item.y -= (sb.pos % entry_height);
   int item_index = sb.pos / entry_height;
   while(rc_item.y < rc.Bottom() && item_index<num_e){
      bool sel_hl = false;
      if(item_index==selection){
         if(want_draw_selection){
            app.DrawSelection(rc_item);
            sel_hl = true;
         }
      }
      if(item_index && (item_index<selection || item_index>selection+1 || !want_draw_selection))
         app.DrawSeparator(rc_item.x+app.fdb.letter_size_x*1, max_x-rc_item.x-app.fdb.letter_size_x*2, rc_item.y);
      DrawItem(rc_item, item_index, sel_hl);
                              //move to next item
      rc_item.y += rc_item.sy;
      ++item_index;
   }
   app.SetClipRect(crc);
   app.DrawScrollbar(sb);
}

//----------------------------
/*
void C_ctrl_list::MakeSureSelectionIsOnScreen(){

   selection = Max(selection, top_line);
   selection = Min(selection, top_line+sb.visible_space-1);
}
*/
//----------------------------

bool C_ctrl_list::EnsureVisible(){

   int prev_pos = sb.pos;
   /*
   if(sb.visible_space==1){
                              //if just one line, set top_line to this
      sb.pos = 0;
   }else*/
   if(!App().IsUiTouchMode() || !one_click_mode){
      int sel_y = selection * entry_height;
      if(sb.visible_space<=entry_height*3){
         if(sel_y < sb.pos)
            sb.pos = sel_y;
         else if(sel_y+entry_height >= sb.visible_space+sb.pos)
            sb.pos = Min(sel_y, sel_y-sb.visible_space+entry_height);
      }else{
         if(sel_y < sb.pos+entry_height)
            sb.pos = sel_y-entry_height;
         else
         if(sel_y+entry_height >= sb.visible_space+sb.pos-entry_height)
            sb.pos = sel_y-sb.visible_space+entry_height*2;
      }
   }
   sb.pos = Max(0, Min(sb.total_space-sb.visible_space, sb.pos));
   return (prev_pos!=sb.pos);
}

//----------------------------

void C_ctrl_list::StopScrolling(){
   DeleteScrollTimer();
   kinetic_scroll.Reset();
}

//----------------------------

void C_ctrl_list::SetSelection(int sel){
   selection = sel;
}

//----------------------------

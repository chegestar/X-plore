#include "Main.h"
#include "AudioPreview.h"

//----------------------------

class C_client_audio_preview: public C_client{
private:
   class C_mode_this;
   friend class C_mode_this;

   void InitLayout(C_mode_this &mod);
   void ProcessInput(C_mode_this &mod, S_user_input &ui, bool &redraw);
   void Tick(C_mode_this &mod, dword time, bool &redraw);
   void Draw(const C_mode_this &mod);

   //----------------------------

   class C_mode_this: public C_mode{
   public:
      S_rect rc;
      C_smart_ptr<C_simple_sound_player> plr;
      Cstr_w display_filename;
      bool paused, finished;
      byte &volume;
      C_button play_button;
      C_button volume_button[2];

      C_mode_this(C_application_ui &_app, C_mode *m, byte &vol):
      C_mode(_app, m),
         paused(false),
         finished(false),
         volume(vol)
      {
      }
      ~C_mode_this();
      virtual void InitLayout(){ static_cast<C_client_audio_preview&>(app).InitLayout(*this); }
      virtual void ProcessInput(S_user_input &ui, bool &redraw){ static_cast<C_client_audio_preview&>(app).ProcessInput(*this, ui, redraw); }
      virtual void Tick(dword time, bool &redraw){ static_cast<C_client_audio_preview&>(app).Tick(*this, time, redraw); }
      virtual void Draw() const{ static_cast<C_client_audio_preview&>(app).Draw(*this); }
   };
   void DrawSongInfo(const C_mode_this &mod, bool clear);
public:
   bool SetMode(const wchar *fn, byte &volume, const wchar *display_filename);
};

//----------------------------

bool C_client_audio_preview::SetMode(const wchar *fn, byte &volume, const wchar *display_filename){

   Cstr_w fn_full;
   C_file::GetFullPath(fn, fn_full);

   if(!volume)                //we share volume with video preview; but here it can't be zero
      ++volume;
   C_simple_sound_player *plr = C_simple_sound_player::Create(fn_full, volume);
   if(!plr)
      return false;

   C_mode_this &mod = *new(true) C_mode_this(*this, mode, volume);
   mod.plr = plr;
   plr->Release();
   if(display_filename && *display_filename)
      mod.display_filename = display_filename;
   else
      mod.display_filename = file_utils::GetFileNameNoPath(Cstr_w(fn));
   CreateTimer(mod, 100);

   InitLayout(mod);
   ActivateMode(mod);
   //UpdateGraphicsFlags(IG_USE_MEDIA_KEYS, IG_USE_MEDIA_KEYS);
   return true;
}

//----------------------------

C_client_audio_preview::C_mode_this::~C_mode_this(){
   app.UpdateGraphicsFlags(0, IG_USE_MEDIA_KEYS);
}

//----------------------------

void C_client_audio_preview::InitLayout(C_mode_this &mod){

   const int border = 2;
   const int top = GetTitleBarHeight();
   mod.rc = S_rect(border, top, ScrnSX()-border*2, ScrnSY()-top-GetSoftButtonBarHeight()-border);

   int vol_sz = fdb.line_spacing;

   S_rect rc(mod.rc.x+GetTextWidth(GetText(TXT_VOLUME), UI_FONT_BIG, 0)+fdb.cell_size_x*2, mod.rc.y+fdb.line_spacing*11/2, 0, 0);
   rc.Expand(vol_sz-2);
   mod.volume_button[0].SetRect(rc);
   rc.y += fdb.line_spacing*3;
   mod.volume_button[1].SetRect(rc);

   rc = S_rect(mod.rc.CenterX(), mod.rc.y+fdb.line_spacing*7, 0, 0);
   rc.Expand(fdb.line_spacing);
   rc.x = Max(rc.x, mod.volume_button[0].GetRect().Right()+fdb.cell_size_x);
   mod.play_button.SetRect(rc);
}

//----------------------------

void C_client_audio_preview::ProcessInput(C_mode_this &mod, S_user_input &ui, bool &redraw){

#ifdef USE_MOUSE
   if(!ProcessMouseInSoftButtons(ui, redraw)){
      if(ButtonProcessInput(mod.play_button, ui, redraw)){
         ui.key = K_ENTER;
      }else
      for(int i=0; i<2; i++){
         bool en = !i ? (mod.volume<10) : (mod.volume>1);
         if(en && ButtonProcessInput(mod.volume_button[i], ui, redraw)){
            ui.key = !i ? K_CURSORUP : K_CURSORDOWN;
            break;
         }
      }
      if(ui.mouse_buttons&MOUSE_BUTTON_1_DOWN)
      if(!mod.plr->IsDone() && !mod.paused && mod.plr->HasPositioning()){
         dword t = mod.plr->GetPlayTime();
         if(t){
            S_rect rc(mod.rc.x+fdb.cell_size_x/2, mod.rc.y+fdb.line_spacing*7/2, mod.rc.sx-fdb.cell_size_x, fdb.line_spacing);
            if(ui.CheckMouseInRect(rc)){
               dword pos = (ui.mouse.x-rc.x) * t / rc.sx;
               mod.plr->SetPlayPos(pos);
            }
         }
      }
   }
#endif

   switch(ui.key){
   case K_LEFT_SOFT:
   case K_RIGHT_SOFT:
   case K_BACK:
   case K_ESC:
      CloseMode(mod);
      return;

   case K_CURSORDOWN:
   case K_CURSORUP:
   case K_VOLUME_UP:
   case K_VOLUME_DOWN:
      {
         byte v = mod.volume;
         if(ui.key==K_CURSORUP || ui.key==K_VOLUME_UP){
            if(v<10)
               ++v;
         }else{
            if(v>1)
               --v;
         }
         if(mod.volume!=v){
            mod.volume = v;
            SaveConfig();
            mod.plr->SetVolume(v);
            redraw = true;
         }
      }
      break;
   case K_ENTER:
      if(mod.plr->HasPositioning()){
         mod.paused = !mod.paused;
         if(mod.paused){
            mod.plr->Pause();
            delete mod.timer; mod.timer = NULL;
            redraw = true;
         }else{
            if(!mod.plr->IsDone())
               mod.plr->Resume();
            else{
               mod.finished = false;
               mod.plr->SetPlayPos(0);
               mod.plr->Resume();
            }
            if(!mod.timer)
               CreateTimer(mod, 50);
         }
         redraw = true;
      }
      break;
   case K_CURSORLEFT:
   case K_CURSORRIGHT:
      if(!mod.plr->IsDone() && !mod.paused && mod.plr->HasPositioning()){
         dword t = mod.plr->GetPlayTime();
         if(t){
            dword pos = mod.plr->GetPlayPos();
            const dword STEP = 5000;
            if(ui.key==K_CURSORLEFT){
               pos = Max(0, int(pos-STEP));
            }else{
               pos = Min(t, pos+STEP);
            }
            mod.plr->SetPlayPos(pos);
         }
      }
      break;
   }
}

//----------------------------

void C_client_audio_preview::Tick(C_mode_this &mod, dword time, bool &redraw){

   UpdateGraphicsFlags(IG_USE_MEDIA_KEYS, IG_USE_MEDIA_KEYS); // here because of ~C_client_video_preview
   if(!mod.plr->IsDone()){
      if(!mod.paused){
         //redraw = true;
                              //draw what's needed
         DrawSongInfo(mod, true);
      }
   }else
   if(!mod.finished){
      mod.finished = true;
      mod.paused = true;
      delete mod.timer; mod.timer = NULL;
      redraw = true;
   }
}

//----------------------------

void C_client_audio_preview::DrawSongInfo(const C_mode_this &mod, bool clear){

   int x = mod.rc.x+fdb.cell_size_x/2;
   int y = mod.rc.y + fdb.line_spacing*3/2;
   const dword col_text = GetColor(COL_TEXT);
   if(clear)
      ClearWorkArea(S_rect(mod.rc.x, y, mod.rc.sx, fdb.line_spacing*3));
   const dword play_time = mod.plr->GetPlayTime();
   const dword curr_time = mod.plr->GetPlayPos();
   {
      Cstr_w s, s_l;
      S_date_time dt;
      dt.SetFromSeconds(play_time/1000);
      s_l = text_utils::GetTimeString(dt);
      s.Format(L"%: %") <<GetText(TXT_LENGTH) <<s_l;
      DrawString(s, x, y, UI_FONT_BIG, 0, col_text);
      y += fdb.line_spacing;
   }
   if(mod.plr->HasPositioning() && play_time){
      {
         Cstr_w s, s_l;
         S_date_time dt;
         dword t = mod.plr->IsDone() ? play_time : curr_time;
         dt.SetFromSeconds(t/1000);
         s_l = text_utils::GetTimeString(dt);
         s.Format(L"%: %") <<GetText(TXT_POSITION) <<s_l;
         DrawString(s, x, y, UI_FONT_BIG, 0, col_text);
         y += fdb.line_spacing;
      }
      {
         const int yy = y+fdb.line_spacing/2;
         const int line_sx = mod.rc.sx-fdb.cell_size_x;
         DrawThickSeparator(x, line_sx, yy);
         int asz = fdb.cell_size_x;
         int ax = x + curr_time*(line_sx-asz)/play_time;
         DrawArrowHorizontal(ax, yy-asz/2, asz, !mod.paused ? col_text : MulAlpha(col_text, 0x8000), true);
         y += fdb.line_spacing;
      }
   }else
      y += fdb.line_spacing*2;
}

//----------------------------

void C_client_audio_preview::Draw(const C_mode_this &mod){

   const dword col_text = GetColor(COL_TEXT);
   ClearWorkArea(S_rect(0, mod.rc.y, ScrnSX(), mod.rc.sy+2));
   DrawTitleBar(GetText(TXT_AUDIO_PREVIEW));

   ClearSoftButtonsArea();
   DrawEtchedFrame(mod.rc);
   int y = mod.rc.y + fdb.line_spacing/2;

   int x = mod.rc.x+fdb.cell_size_x/2;
   {
      Cstr_w s;
      s<<GetText(TXT_FILE) <<':';
      int xx = x+DrawString(s, x, y, UI_FONT_BIG, 0, col_text) + fdb.cell_size_x/2;
      DrawString(mod.display_filename, xx, y, UI_FONT_BIG, 0, col_text, -(mod.rc.Right()-xx));
   }
   DrawSongInfo(mod, false);
   y += fdb.line_spacing*5;
   {
                              //volume buttons
      for(int i=0; i<2; i++){
         const C_button &b = mod.volume_button[i];
         bool en = !i ? (mod.volume<10) : (mod.volume>1);
         if(HasMouse())
            DrawButton(b, en);
         dword color = col_text;
         if(!en)
            color = MulAlpha(color, 0x8000);
         const S_rect &rc = b.GetRect();
         dword sz = rc.sx/2;
         DrawArrowVertical(rc.x+sz/2, rc.y+sz/2, sz, color, i==1);
      }
      y += fdb.line_spacing;

      Cstr_w s;
      s.Format(L"%: #%%%") <<GetText(TXT_VOLUME) <<int(mod.volume*10);
      DrawString(s, x, y, UI_FONT_BIG, 0, col_text);
      y += fdb.line_spacing;
   }
   {
      if(HasMouse())
         DrawButton(mod.play_button, true);
      dword color = col_text;
      const S_rect &rc = mod.play_button.GetRect();
      dword sz = rc.sx/2;
      int x1 = rc.x+sz/2, y1 = rc.y+sz/2;
      if(mod.paused)
         DrawArrowHorizontal(x1, y1, sz, color, true);
      else{
         S_rect rc1(x1, y1, sz/3, sz);
         FillRect(rc1, color);
         rc1.x += rc1.sx*2;
         FillRect(rc1, color);
      }
   }
   DrawSoftButtonsBar(mod, 0, TXT_BACK);
   SetScreenDirty();
}

//----------------------------

//----------------------------

bool SetModeAudioPreview(C_client &app, const wchar *fn, byte &volume, const wchar *display_filename){

   C_client_audio_preview &_this = (C_client_audio_preview&)app;
   return _this.SetMode(fn, volume, display_filename);
}

//----------------------------

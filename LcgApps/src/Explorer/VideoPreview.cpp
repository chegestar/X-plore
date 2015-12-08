#ifdef __SYMBIAN32__
#include <E32Std.h>
#endif
#include "..\Main.h"
#include "Main_Explorer.h"
#include "VideoPreview.h"
#include "..\AudioPreview.h"

//----------------------------
#if defined __SYMBIAN32__// && defined __SYMBIAN_3RD__
#ifdef __SYMBIAN_3RD__
#include <VideoPlayer.h>
#else
#include <mda\common\controller.h>
class CVideoPlayerUtility;
class TMMFEvent;
class MVideoPlayerUtilityObserver{
public:
	virtual void MvpuoOpenComplete(TInt aError) = 0;
	virtual void MvpuoPrepareComplete(TInt aError) = 0;
	virtual void MvpuoFrameReady(CFbsBitmap& aFrame,TInt aError) = 0;
	virtual void MvpuoPlayComplete(TInt aError) = 0;
	virtual void MvpuoEvent(const TMMFEvent& aEvent) = 0;
};
#endif

#include <EikEnv.h>
#include <CoeCntrl.h>

class C_simple_video_player_imp: public C_simple_video_player, public MVideoPlayerUtilityObserver{
   class C_ctrl: public CCoeControl{
   public:
      inline RWindow &Window() const{ return CCoeControl::Window(); }
   } *ctrl;
   CVideoPlayerUtility *plr;
   bool inited, finished, open_err;
   dword init_volume;

   CActiveSchedulerWait *asw;

#ifndef __SYMBIAN_3RD__
   RLibrary lib_mcv;

   typedef CVideoPlayerUtility *(*t_NewL)(MVideoPlayerUtilityObserver& aObserver, TInt aPriority, TMdaPriorityPreference aPref, RWsSession& aWs,
      CWsScreenDevice& aScreenDevice, RWindowBase& aWindow, const TRect& aScreenRect, const TRect& aClipRect);

#ifdef _DEBUG
   typedef void (CVideoPlayerUtility::*t_OpenFileL)(const TDesC& aFileName, TUid aControllerUid);
   typedef void (CVideoPlayerUtility::*t_Prepare)();
   typedef void (CVideoPlayerUtility::*t_Play)();
   typedef void (CVideoPlayerUtility::*t_PauseL)();
   typedef void (CVideoPlayerUtility::*t_SetDisplayWindowL)(RWsSession& aWs, CWsScreenDevice& aScreenDevice, RWindowBase& aWindow, TRect& aScreenRect, TRect& aClipRect);
   typedef void (CVideoPlayerUtility::*t_VideoFrameSizeL)(TSize& aSize) const;
   typedef void (CVideoPlayerUtility::*t_SetPositionL)(const TTimeIntervalMicroSeconds& aPosition);
   typedef TTimeIntervalMicroSeconds (CVideoPlayerUtility::*t_PositionL)() const;
   typedef TTimeIntervalMicroSeconds (CVideoPlayerUtility::*t_DurationL)() const;
   typedef void (CVideoPlayerUtility::*t_SetVolumeL)(TInt aVolume);
   typedef TInt (CVideoPlayerUtility::*t_MaxVolume)() const;
#else
   typedef void (*t_OpenFileL)(CVideoPlayerUtility*, const TDesC& aFileName, TUid aControllerUid);
   typedef void (*t_Prepare)(CVideoPlayerUtility*);
   typedef void (*t_Play)(CVideoPlayerUtility*);
   typedef void (*t_PauseL)(CVideoPlayerUtility*);
   typedef void (*t_SetDisplayWindowL)(CVideoPlayerUtility*,RWsSession& aWs, CWsScreenDevice& aScreenDevice, RWindowBase& aWindow, TRect& aScreenRect, TRect& aClipRect);
   typedef void (*t_VideoFrameSizeL)(const CVideoPlayerUtility*,TSize& aSize);
   typedef void (*t_SetPositionL)(CVideoPlayerUtility*,const TTimeIntervalMicroSeconds& aPosition);
   typedef TTimeIntervalMicroSeconds (*t_PositionL)(const CVideoPlayerUtility*);
   typedef TTimeIntervalMicroSeconds (*t_DurationL)(const CVideoPlayerUtility*);
   typedef void (*t_SetVolumeL)(CVideoPlayerUtility*,TInt aVolume);
   typedef TInt (*t_MaxVolume)(const CVideoPlayerUtility*);
#endif
   t_NewL mcv_NewL;
   t_OpenFileL mcv_OpenFileL;
   t_Prepare mcv_Prepare;
   t_Play mcv_Play;
   t_PauseL mcv_PauseL;
   t_SetDisplayWindowL mcv_SetDisplayWindowL;
   t_VideoFrameSizeL mcv_VideoFrameSizeL;
   t_SetPositionL mcv_SetPositionL;
   t_PositionL mcv_PositionL;
   t_DurationL mcv_DurationL;
   t_SetVolumeL mcv_SetVolumeL;
   t_MaxVolume mcv_MaxVolume;
#endif
//----------------------------

   virtual void MvpuoOpenComplete(TInt err){
      if(err){
         open_err = true;
         finished = true;
         asw->AsyncStop();
      }else{
#ifdef __SYMBIAN_3RD__
         plr->Prepare();
#else
#ifdef _DEBUG
         (plr->*mcv_Prepare)();
#else
         mcv_Prepare(plr);
#endif
#endif
      }
   }
//----------------------------

   virtual void MvpuoPrepareComplete(TInt err){
      /*
      if(err){
#ifdef _DEBUG
         Fatal("prep err", err);
#endif
         open_err = true;
         finished = true;
      }else*/
      {
         inited = true;
         SetVolume(init_volume);
         SetClipRect(rc_clip);
         Resume();
      }
      asw->AsyncStop();
   }

//----------------------------
   virtual void MvpuoFrameReady(CFbsBitmap &aFrame, TInt err){
#ifdef _DEBUG
      Fatal("MvpuoFrameReady", err);
#endif
   }
//----------------------------
   virtual void MvpuoPlayComplete(TInt err){
      finished = true;
   }
//----------------------------
   virtual void MvpuoEvent(const TMMFEvent &aEvent){
#ifdef _DEBUG
      Fatal("MvpuoEvent");
#endif
   }

   S_rect rc_clip, rc_play;
   TRect rc, clip;

//----------------------------

   virtual void SetClipRect(const S_rect &_rc_clip){
      rc_clip = _rc_clip;
      if(inited){
#if 1
         TSize sz;
#ifdef __SYMBIAN_3RD__
         plr->VideoFrameSizeL(sz);
#else
#ifndef _DEBUG
         mcv_VideoFrameSizeL(plr, sz);
#endif
#endif
         int sx = Min(sz.iWidth, rc_clip.sx);
         int sy = Min(sz.iHeight, rc_clip.sy);
         /*
         int x = rc_clip.x + (rc_clip.sx-sx)/2;
         int y = rc_clip.y;

         rc_play = S_rect(x, y, sx, sy);
         */
         rc_play = rc_clip;

         if(sx && sy){
            C_fixed ratio_w = C_fixed(rc_clip.sx)/C_fixed(sx);
            C_fixed ratio_h = C_fixed(rc_clip.sy)/C_fixed(sy);

                                       //fit the width/height of image to fit into (image_width x image_height) space
            if(ratio_h > ratio_w){
               rc_play.sx = C_fixed(sx)*ratio_w;
               rc_play.sy = C_fixed(sy)*ratio_w;
            }else{
               rc_play.sx = C_fixed(sx)*ratio_h;
               rc_play.sy = C_fixed(sy)*ratio_h;
            }
            rc_play.sx = (rc_play.sx+1) & -2;
            rc_play.sy = (rc_play.sy+1) & -2;
            rc_play.x += (rc_clip.sx-rc_play.sx)/2;
            rc_play.y += (rc_clip.sy-rc_play.sy)/2;
         }else{
            rc_play.sx = 0;
            rc_play.sy = 0;
         }
#else
         rc_play = rc_clip;
#endif
         rc = TRect(rc_play.x, rc_play.y, rc_play.Right(), rc_play.Bottom());
         clip = rc;
#ifdef __SYMBIAN_3RD__
         plr->SetDisplayWindowL(CEikonEnv::Static()->WsSession(), *CCoeEnv::Static()->ScreenDevice(), ctrl->Window(), rc, clip);
#else
#ifndef _DEBUG
         mcv_SetDisplayWindowL(plr, CEikonEnv::Static()->WsSession(), *CCoeEnv::Static()->ScreenDevice(), ctrl->Window(), rc, clip);
#endif
#endif
      }
   }

   virtual bool IsAudioOnly() const{
      return inited && rc_play.sy==0;
   }

   virtual const S_rect &GetRect() const{
      return rc_play;
   }
   virtual void GetFrame() const{
      //plr->GetFrameL(EColor16MU);
   }
public:
   C_simple_video_player_imp(C_application_base &_app, IGraph *igraph, dword vol):
      plr(NULL),
      asw(NULL),
      open_err(false),
      inited(false),
      finished(false),
      init_volume(vol),
      ctrl((C_ctrl*)igraph->GetCoeControl())
   {
   }
   ~C_simple_video_player_imp(){
      delete asw;
#ifdef __SYMBIAN_3RD__
      delete plr;
#else
      delete (CBase*)plr;
      lib_mcv.Close();
#endif
   }

//----------------------------

   bool Construct(C_application_base &app, const wchar *fname, const S_rect &_rc_clip){

      rc_clip = _rc_clip;
      rc_play = rc_clip;
      rc = TRect(rc_clip.x, rc_clip.y, rc_clip.Right(), rc_clip.Bottom());
      clip = rc;
#ifndef __SYMBIAN_3RD__
      const TUid ss_dll_uid = { 0x101f4549 };
      const TUidType uid(KNullUid, ss_dll_uid, KNullUid);
      if(lib_mcv.Load(_L("mediaclientvideo.dll"), uid)){
         return false;
      }
#ifdef _DEBUG
      MemSet(&mcv_OpenFileL, 0, ((byte*)&mcv_MaxVolume+1)-(byte*)&mcv_OpenFileL);
      mcv_NewL = (t_NewL)lib_mcv.Lookup(23);
      *(TLibraryFunction*)&mcv_OpenFileL = lib_mcv.Lookup(32);
      *(TLibraryFunction*)&mcv_Prepare = lib_mcv.Lookup(90);
      *(TLibraryFunction*)&mcv_Play = lib_mcv.Lookup(38);
#else
      mcv_NewL = (t_NewL)lib_mcv.Lookup(23);
      mcv_OpenFileL = (t_OpenFileL)lib_mcv.Lookup(32);
      mcv_Prepare = (t_Prepare)lib_mcv.Lookup(90);
      mcv_Play = (t_Play)lib_mcv.Lookup(38);
      mcv_VideoFrameSizeL = (t_VideoFrameSizeL)lib_mcv.Lookup(69);
      mcv_SetDisplayWindowL = (t_SetDisplayWindowL)lib_mcv.Lookup(51);
      mcv_DurationL = (t_DurationL)lib_mcv.Lookup(11);
      mcv_PositionL = (t_PositionL)lib_mcv.Lookup(39);
      mcv_SetPositionL = (t_SetPositionL)lib_mcv.Lookup(54);
      mcv_PauseL = (t_PauseL)lib_mcv.Lookup(36);
      mcv_MaxVolume = (t_MaxVolume)lib_mcv.Lookup(20);
      mcv_SetVolumeL = (t_SetVolumeL)lib_mcv.Lookup(61);
#endif
      if(!mcv_NewL || !mcv_OpenFileL || !mcv_Prepare || !mcv_Play || !mcv_VideoFrameSizeL || !mcv_SetDisplayWindowL || !mcv_DurationL || !mcv_PositionL ||
         !mcv_PauseL || !mcv_SetPositionL || !mcv_MaxVolume || !mcv_SetVolumeL){

         return false;
      }
      asw = new(true) CActiveSchedulerWait;
      plr = mcv_NewL(*this, EMdaPriorityNormal, EMdaPriorityPreferenceTimeAndQuality, CEikonEnv::Static()->WsSession(), *CCoeEnv::Static()->ScreenDevice(),
         ctrl->Window(), rc, clip);
#ifdef _DEBUG
      TRAPD(err, (plr->*mcv_OpenFileL)(TPtrC((word*)fname, StrLen(fname)), KNullUid));
#else
      TRAPD(err, mcv_OpenFileL(plr, TPtrC((word*)fname, StrLen(fname)), KNullUid));
#endif
      if(err)
         return false;
#else
      asw = new(true) CActiveSchedulerWait;
      plr = CVideoPlayerUtility::NewL(*this, EMdaPriorityNormal, EMdaPriorityPreferenceNone, CEikonEnv::Static()->WsSession(), *CCoeEnv::Static()->ScreenDevice(),
         ctrl->Window(), rc, clip);
      plr->OpenFileL(TPtrC((word*)fname, StrLen(fname)));
#endif
      asw->Start();
      delete asw;
      asw = NULL;
      return !open_err;
   }

//----------------------------

   virtual bool IsDone() const{ return finished; }

//----------------------------

   virtual dword GetPlayTime() const{
      if(!inited)
         return 0;
#ifdef __SYMBIAN_3RD__
      return dword(plr->DurationL().Int64()/TInt64(1000));
#else
#ifdef _DEBUG
      return 0;
#else
      return (mcv_DurationL(plr).Int64()/TInt64(1000)).Low();
#endif
#endif
   }

//----------------------------

   virtual dword GetPlayPos() const{
      if(!inited)
         return 0;
      if(finished)
         return GetPlayTime();
#ifdef __SYMBIAN_3RD__
      TTimeIntervalMicroSeconds p = plr->PositionL();
      return Min(dword(p.Int64()/TInt64(1000)), GetPlayTime());
#else
#ifdef _DEBUG
      return 0;
#else
      TTimeIntervalMicroSeconds p = p = mcv_PositionL(plr);
      return Min((p.Int64()/TInt64(1000)).Low(), GetPlayTime());
#endif
#endif
   }

//----------------------------

   virtual void SetPlayPos(dword pos){
      if(inited){
         Pause();
         TTimeIntervalMicroSeconds p(TUint(pos*1000));
#ifdef __SYMBIAN_3RD__
         plr->SetPositionL(p);
#else
#ifndef _DEBUG
         mcv_SetPositionL(plr, p);
#endif
#endif
         Resume();
      }
   }

//----------------------------

   virtual void SetVolume(dword vol){
      if(inited){
#ifdef __SYMBIAN_3RD__
         dword v = (plr->MaxVolume() * vol) / 10;
         plr->SetVolumeL(v);
#else
#ifndef _DEBUG
         dword v = (mcv_MaxVolume(plr) * vol) / 10;
         mcv_SetVolumeL(plr, v);
#endif
#endif
      }else
         init_volume = vol;
   }

//----------------------------

   virtual void Pause(){
      if(inited){
#ifdef __SYMBIAN_3RD__
         plr->PauseL();
#else
#ifndef _DEBUG
         mcv_PauseL(plr);
#endif
#endif
      }
   }

//----------------------------

   virtual void Resume(){
      if(inited){
#ifdef __SYMBIAN_3RD__
         plr->Play();
#else

#ifdef _DEBUG
         assert(0);
#else
         mcv_Play(plr);
#endif

#endif
         finished = false;
      }
   }

//----------------------------

   virtual bool HasPositioning() const{ return true; }
};

#elif defined _DEBUG

#include <RndGen.h>

class C_simple_video_player_imp: public C_simple_video_player{
   S_rect rc_clip;
   mutable C_rnd_gen rnd;   
public:
   C_simple_video_player_imp(C_application_base &app, IGraph *igraph, dword vol){}

   virtual bool IsDone() const{ return false; }
   virtual dword GetPlayTime() const{ return 5000; }
   virtual dword GetPlayPos() const{ return rnd.Get(5000); }
   virtual void SetPlayPos(dword){}
   virtual void SetVolume(dword vol){}
   virtual void Pause(){}
   virtual void Resume(){}
   virtual bool HasPositioning() const{ return true; }
   virtual void SetClipRect(const S_rect &_rc_clip){ rc_clip = _rc_clip; }
   virtual const S_rect &GetRect() const{ return rc_clip; }
   //virtual bool IsAudioOnly() const{ return true; } // test

   bool Construct(C_application_base &app, const wchar *fname, const S_rect &_rc_clip){
      rc_clip = _rc_clip;
      return true;
   }
};

#else

class C_simple_video_player_imp: public C_simple_video_player{
   S_rect rc_clip;
public:
   C_simple_video_player_imp(C_application_base &app, IGraph *igraph, dword vol){}
   virtual bool IsDone() const{ return false; }
   virtual dword GetPlayTime() const{ return 0; }
   virtual dword GetPlayPos() const{ return 0; }
   virtual void SetPlayPos(dword){}
   virtual void SetVolume(dword vol){}
   virtual void Pause(){}
   virtual void Resume(){}
   virtual bool HasPositioning() const{ return true; }
   virtual void SetClipRect(const S_rect &_rc_clip){ }
   virtual const S_rect &GetRect() const{ return rc_clip; }
   bool Construct(C_application_base &app, const wchar *fname, const S_rect &_rc_clip){
      return false;
   }
};
#endif

//----------------------------

C_simple_video_player *C_simple_video_player::Create(C_application_base &app, IGraph *igraph, const wchar *fn, const S_rect &rc_clip, dword vol){
   C_simple_video_player_imp *plr = new(true) C_simple_video_player_imp(app, igraph, vol);
   if(!plr->Construct(app, fn, rc_clip)){
      plr->Release();
      plr = NULL;
   }
   return plr;
}

//----------------------------

bool C_client_video_preview::SetMode(const wchar *fn, byte &volume){

   C_mode_this &mod = *new(true) C_mode_this(*this, volume, fn);
   mod.InitLayout();

   C_simple_video_player *plr = C_simple_video_player::Create(*this, igraph, fn, mod.rc_clip, volume);
   if(!plr){
      mod.Release();
      return false;
   }

   mod.plr = plr;
   plr->Release();
   mod.fn_raw = file_utils::GetFileNameNoPath(mod.fn);
   CreateTimer(mod, 50);

   ActivateMode(mod);
   UpdateGraphicsFlags(IG_USE_MEDIA_KEYS, IG_USE_MEDIA_KEYS);
   return true;
}

//----------------------------

C_client_video_preview::C_mode_this::~C_mode_this(){
   EnableBacklight(false);
   app.UpdateGraphicsFlags(0, IG_USE_MEDIA_KEYS);
}

//----------------------------

void C_client_video_preview::C_mode_this::EnableBacklight(bool on){

   app.UpdateGraphicsFlags(on ? IG_ENABLE_BACKLIGHT : 0, IG_ENABLE_BACKLIGHT);
}

//----------------------------

void C_client_video_preview::C_mode_this::InitLayout(){

   const int border = 3;
   const int top = app.GetTitleBarHeight();
   rc = S_rect(border, top, app.ScrnSX()-border*2, app.ScrnSY()-top-app.GetSoftButtonBarHeight());

   int y = rc.y+app.fdb.line_spacing*4;
   rc_clip = S_rect(rc.x+app.fdb.cell_size_x, y+1, rc.sx-app.fdb.cell_size_x*2, rc.Bottom()-y-app.fdb.cell_size_x);

   //if(plr)
      //plr->SetClipRect(rc_clip);
}

//----------------------------

void C_client_video_preview::C_mode_this::ProcessInput(S_user_input &ui, bool &redraw){

#ifdef USE_MOUSE
   if(!app.ProcessMouseInSoftButtons(ui, redraw)){
      if(ui.mouse_buttons&MOUSE_BUTTON_1_DOWN)
      if(!plr->IsDone() && plr->HasPositioning()){
         if(ui.CheckMouseInRect(rc)){
            ui.key = K_ENTER;
         }else if(!paused){
            dword t = plr->GetPlayTime();
            if(t){
               S_rect rc(this->rc.x+app.fdb.cell_size_x/2, this->rc.y+app.fdb.line_spacing*3, this->rc.sx-app.fdb.cell_size_x, app.fdb.line_spacing+1);
               if(ui.CheckMouseInRect(rc)){
                  dword pos = (ui.mouse.x-rc.x) * t / rc.sx;
                  plr->SetPlayPos(pos);
               }
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
      app.CloseMode(*this);
      return;

   case K_CURSORDOWN:
   case K_CURSORUP:
   case K_VOLUME_UP:
   case K_VOLUME_DOWN:
      {
         byte v = volume;
         if(ui.key==K_CURSORUP || ui.key==K_VOLUME_UP){
            if(v<10)
               ++v;
         }else{
            if(v>0)
               --v;
         }
         if(volume!=v){
            volume = v;
            app.SaveConfig();
            plr->SetVolume(v);
            //redraw = true;
            DrawBottomBar();
         }
      }
      break;
   case K_ENTER:
      if(plr->HasPositioning()){
         paused = !paused;
         if(paused){
            EnableBacklight(false);
            plr->Pause();
            delete timer; timer = NULL;
         }else{
            if(!plr->IsDone()){
               plr->Resume();
            }else{
               finished = false;
               plr->SetPlayPos(0);
               plr->Resume();
            }
            EnableBacklight(true);
            if(!timer)
               app.CreateTimer(*this, 50);
         }
         DrawTimePos();
      }
      break;
   case K_CURSORLEFT:
   case K_CURSORRIGHT:
      if(!plr->IsDone() && !paused && plr->HasPositioning()){
         dword t = plr->GetPlayTime();
         if(t){
            dword pos = plr->GetPlayPos();
            const dword STEP = 5000;
            if(ui.key==K_CURSORLEFT){
               pos = Max(0, int(pos-STEP));
            }else{
               pos = Min(t, pos+STEP);
            }
            plr->SetPlayPos(pos);
         }
      }
      break;
      /*
   case 'f':
      plr->GetFrame();
      break;
      */
   }
}

//----------------------------

void C_client_video_preview::C_mode_this::Tick(dword time, bool &redraw){

   if(plr->IsAudioOnly()){
      app.CloseMode(*this);
      C_explorer_client::S_config_xplore &config_xplore = ((C_explorer_client&)app).config_xplore;
      SetModeAudioPreview(app, fn, config_xplore.audio_volume);
      return;
   }
   if(!plr->IsDone()){
      if(!paused){
         dword ds = plr->GetPlayPos()/1000;
         if(last_drawn_second!=ds)
            DrawPositionTime();
         DrawTimePos();
      }
   }else
   if(!finished){
      finished = true;
      paused = true;
      delete timer; timer = NULL;
      redraw = true;
      EnableBacklight(false);
   }
}

//----------------------------

void C_client_video_preview::C_mode_this::DrawPositionTime() const{

   const dword play_time = plr->GetPlayTime();
   if(!play_time)
      return;
   const dword curr_time = plr->GetPlayPos();
   const int x = rc.x+app.fdb.cell_size_x/2;
   const int y = rc.y+app.fdb.line_spacing*3/2;
   const dword col_text = app.GetColor(COL_TEXT);

   S_rect rc1(rc.x, y, rc.sx, app.fdb.line_spacing+1);
   app.ClearWorkArea(rc1);

   Cstr_w s, s_p, s_t;
   S_date_time dtp, dtt;
   dword t = plr->IsDone() ? play_time : curr_time;
   last_drawn_second = t/1000;
   dtp.SetFromSeconds(last_drawn_second);
   dtt.SetFromSeconds(play_time/1000);

   s_p = text_utils::GetTimeString(dtp);
   s_t = text_utils::GetTimeString(dtt);
   s.Format(L"%: % / %") <<app.GetText(TXT_POSITION) <<s_p <<s_t;
   app.DrawString(s, x, y, UI_FONT_BIG, 0, col_text);
   app.AddDirtyRect(rc1);
}

//----------------------------

void C_client_video_preview::C_mode_this::DrawTimePos() const{

   const dword play_time = plr->GetPlayTime();
   if(!play_time)
      return;
   const dword curr_time = plr->GetPlayPos();
   const int x = rc.x+app.fdb.cell_size_x/2;
   const int y = rc.y+app.fdb.line_spacing*3;
   const int line_sx = rc.sx-app.fdb.cell_size_x;
   const dword col_text = app.GetColor(COL_TEXT);
   const int asz = app.fdb.cell_size_x;

   S_rect rc1(rc.x, y-asz/2, rc.sx, asz+1);
   app.ClearWorkArea(rc1);

   app.DrawThickSeparator(x, line_sx, y);
   int ax = x + curr_time*(line_sx-asz)/play_time;
   app.DrawArrowHorizontal(ax, y-asz/2, asz, !paused ? col_text : MulAlpha(col_text, 0x8000), true);

   app.AddDirtyRect(rc1);
}

//----------------------------

void C_client_video_preview::C_mode_this::DrawBottomBar() const{

   app.ClearSoftButtonsArea(rc.Bottom() + 2);
   {
      Cstr_w s;
      s.Format(L"%: #%%%") <<app.GetText(TXT_VOLUME) <<int(volume*10);
      app.DrawString(s, rc.CenterX(), app.ScrnSY()-app.fdb.line_spacing, UI_FONT_BIG, FF_CENTER, app.GetColor(COL_TEXT_SOFTKEY));
   }
   app.DrawSoftButtonsBar(*this, TXT_NULL, TXT_BACK);
}

//----------------------------

void C_client_video_preview::C_mode_this::Draw() const{

   const dword col_text = app.GetColor(COL_TEXT);
   const dword sx = app.ScrnSX();
   app.ClearWorkArea(S_rect(0, rc.y, sx, rc.sy+2));
   app.DrawTitleBar(app.GetText(TXT_VIDEO_PREVIEW));

   const dword c0 = app.GetColor(COL_SHADOW), c1 = app.GetColor(COL_HIGHLIGHT);
   app.DrawOutline(rc, c0, c1);
   //app.DrawOutline(plr->GetRect(), 0xff000000, 0xff000000);
   {
      S_rect rc1 = rc;
      rc1.Expand();
      app.DrawOutline(rc1, c1, c0);
   }
   int y = rc.y+app.fdb.line_spacing/2;

   int x = rc.x+app.fdb.cell_size_x/2;
   {
      Cstr_w s;
      s<<app.GetText(TXT_FILE) <<':';
      int xx = x+app.DrawString(s, x, y, UI_FONT_BIG, 0, col_text) + app.fdb.cell_size_x/2;
      app.DrawString(fn_raw, xx, y, UI_FONT_BIG, 0, col_text, -(rc.Right()-xx));
      y += app.fdb.line_spacing;
   }
   //app.FillRect(plr->GetRect(), 0xff000000);
   const dword play_time = plr->GetPlayTime();
   /*
   {
      Cstr_w s, s_l;
      S_date_time dt;
      dt.SetFromSeconds(play_time/1000);
      s_l = text_utils::GetTimeString(dt);
      s.Format(L"%: %") <<GetText(TXT_LENGTH) <<s_l;
      DrawString(s, x, y, UI_FONT_BIG, 0, col_text);
      y += app.fdb.line_spacing;
   }
   */
   if(plr->HasPositioning() && play_time){
      DrawPositionTime();
      DrawTimePos();
   }
   DrawBottomBar();
   app.SetScreenDirty();
   app.UpdateScreen();
   ((C_simple_video_player*)(const C_simple_video_player*)plr)->SetClipRect(rc_clip);
}

//----------------------------

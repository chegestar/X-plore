#include "Main.h"
#include <Thread.h>
#if defined MAIL
#include "Email\Main_Email.h"
#endif
#include "Viewer.h"
#ifdef EXPLORER
#include "Explorer\Main_Explorer.h"
#endif

#define IMG_OPEN_IN_THREAD

//----------------------------
#ifdef _DEBUG
//#define DEBUG_IMAGE_SCALE .7f
//#define IMG_BGND_COLOR 0xff606060
#endif

const int SCALE_DISPLAY_COUNTDOWN = 2000,
   SCALE_DISPLAY_FADE = 1000;

//----------------------------

class C_mode_image_viewer: public C_mode_app<C_client>, public C_image::C_open_progress
#ifdef IMG_OPEN_IN_THREAD
   , public C_thread
#endif
{
   typedef C_mode_app<C_client> super;

   Cstr_w title;

   virtual bool WantInactiveTimer() const{ return false; }
#ifdef IMG_OPEN_IN_THREAD
   virtual void ThreadProc();
#endif
//----------------------------
   virtual bool ImageLoadProgress(dword pos){
#ifdef IMG_OPEN_IN_THREAD
      if(IsThreadQuitRequest())
         return false;
      const int CHANGE_BITS = 3;
      if((int(pos)>>CHANGE_BITS)==(img_load_progress.pos>>CHANGE_BITS))
         return true;
      if(!img_load_progress.pos){
         FinishImageOpen(true);
      }
      img_load_progress.pos = pos;
      if(IsThreadRunning())
         img_state_changed = true;
      else{
         app.RedrawScreen();
         app.UpdateScreen();
      }
#endif
      return true;
   }
//----------------------------
   void FinishImageOpen(bool set_pos);
   void DrawImage() const;

   virtual C_ctrl_title_bar *CreateTitleBar(){
      if(fullscreen)
         return NULL;
      C_ctrl_title_bar *tb = super::CreateTitleBar();
      tb->SetText(title);
      return tb;
   }
public:
   S_rect rc;

   C_smart_ptr<C_image> img;
   Cstr_w filename;
   dword img_file_offset;
   const C_smart_ptr<C_zip_package> arch;
   C_fixed img_scale, min_scale;
   C_fixed img_x, img_y;       //image display offset (relative to rectangle left-top corner)

   S_rect rc_img;        //rectangle in which image is drawn (may be reduced by scrollbar(s))
   C_scrollbar sb, sb_h;       //horizontal scrollbar for image
   C_viewer_previous_next *viewer_previous_next;

   bool is_mouse_drag;
   S_point drag_mouse;
   bool fullscreen;
   bool landscape, show_scrollbars;         //only for image viewer
   dword scroll_key_bits;
   bool has_previous, has_next;

   C_kinetic_movement kinetic_movement;

#ifdef IMG_OPEN_IN_THREAD
   enum{
      IMG_CLOSED,
      IMG_OPENING,
      IMG_OPENED,
      IMG_ERROR_LOW_MEMORY,
      IMG_ERROR_OTHER,
   } img_state;
   bool img_state_changed;
   C_progress_indicator img_load_progress;
#endif
   int scale_display_countdown;

   C_mode_image_viewer(C_client &_app, C_viewer_previous_next *vpn, const Cstr_w &_title, const Cstr_w &fn, const C_zip_package *_arch, dword _img_file_offset):
      super(_app),
      viewer_previous_next(vpn),
      title(_title),
      filename(fn),
      img_file_offset(_img_file_offset),
      img_scale(0),
      min_scale(1),
      img_x(0), img_y(0),
      is_mouse_drag(false),
      landscape(false),
      scroll_key_bits(0),
      show_scrollbars(true),
      scale_display_countdown(0),
      has_previous(false),
      has_next(false),
      fullscreen(false)
   {
      arch.SetPtr(_arch);
      CreateTimer(30);
#ifdef EXPLORER
      fullscreen = (app.config.flags&C_explorer_client::S_config_xplore::CONF_VIEWER_FULLSCREEN);
      landscape = (app.config.flags&C_explorer_client::S_config_xplore::CONF_VIEWER_LANDSCAPE);
#endif
      SetTitle(title);
#ifndef IMG_OPEN_IN_THREAD
      min_scale = C_fixed(img->SizeX()) / C_fixed(img->GetOriginalSize().x);
      assert(min_scale <= C_fixed::One());
#endif
      C_ctrl_softkey_bar *skb = GetSoftkeyBar();
      skb->InitButton(0, app.BUT_ZOOM_ORIGINAL, TXT_ZOOM_FIT);
      skb->InitButton(1, app.BUT_ZOOM_FIT, TXT_ZOOM_ORIGINAL);
      skb->InitButton(2, app.BUT_ZOOM_IN, TXT_ZOOM_IN);
      skb->InitButton(3, app.BUT_ZOOM_OUT, TXT_ZOOM_OUT);
      EnableButtons();

      InitLayout();

      OpenImage();
      Activate();
   }
   ~C_mode_image_viewer(){
#ifdef IMG_OPEN_IN_THREAD
      AskToQuitThread(2000);
#endif
   }
//----------------------------
   void EnableButtons(){
      C_ctrl_softkey_bar *skb = GetSoftkeyBar();
      if(skb){
         dword buts_on = 0;
         if(min_scale!=C_fixed::One()){
            if(img_scale!=C_fixed::Zero())
               buts_on |= 2|8;
            if(img_scale!=C_fixed::One())
               buts_on |= 1|4;
         }
         for(int i=0; i<4; i++)
            skb->EnableButton(i, (buts_on&(1<<i)));
      }
   }
//----------------------------
   virtual void OnSoftBarButtonPressed(dword index){
      switch(index){
      case 0: ImageZoomOriginal(); break;
      case 1: ImageZoomFit(); break;
      case 2: ImageZoomIn(); break;
      case 3: ImageZoomOut(); break;
      }
   }

   virtual void InitLayout();
   virtual void ProcessInput(S_user_input &ui, bool &redraw);
   virtual void ProcessMenu(int itm, dword menu_id);
   virtual void Tick(dword time, bool &redraw);
   virtual void Draw() const;

   void CheckSlideToPrevNext();

   virtual void DrawScalePercent() const;

   bool OpenImage();
//----------------------------

   bool ImageZoomFit(){

      if(img_scale!=C_fixed::Zero()){
         img_scale = C_fixed::Zero();
         return OpenImage();
      }
      return false;
   }

   //----------------------------

   bool ImageZoomOriginal(){

      if(img_scale!=C_fixed::One() && min_scale!=C_fixed::One()){
         img_scale = C_fixed::One();
         return OpenImage();
      }
      return false;
   }

   //----------------------------

   bool ImageZoomIn(){

      if(img_scale==C_fixed::One())
         return false;
      if(img_scale==C_fixed::Zero())
         img_scale = min_scale;
         //img_scale = C_fixed(int(min_scale * C_fixed(10))) * C_fixed(.1f);
      img_scale += C_fixed::Percent(10);
      if(img_scale > C_fixed::One())
         img_scale = C_fixed::One();
      return OpenImage();
   }

   //----------------------------

   bool ImageZoomOut(){

      if(img_scale==C_fixed::Zero())
         return false;
      img_scale -= C_fixed::Percent(10);
      if(img_scale <= min_scale)
         img_scale = C_fixed::Zero();
      return OpenImage();
   }

   //----------------------------

   bool OpenPreviousNext(bool next){

      if(!viewer_previous_next)
         return false;
      Cstr_w fn, tit;
      if(!viewer_previous_next->GetPreviousNextFile(next, &fn, &tit))
         return false;
      filename = fn;
      SetTitle(tit);
      img_scale = 0;
      min_scale = C_fixed::One();
      if(OpenImage()){
         /*
         dword osx, osy;
         img->GetOriginalSize(osx, osy);
         min_scale = C_fixed(img->SizeX()) / C_fixed(osx);
         assert(min_scale <= C_fixed::One());
         */
      }
      EnableButtons();
      return true;
   }

//----------------------------

   void ScrollImage(const S_point &scrl, bool &redraw);

//----------------------------

   virtual void InitMenu(){
      {
         //accessing previous/next image
         if(viewer_previous_next){
            menu->AddItem(TXT_NEXT, has_next ? 0 : C_menu::DISABLED, "[3]", "[N]");
            menu->AddItem(TXT_PREVIOUS, has_previous ? 0 : C_menu::DISABLED, "[2]", "[P]");
            menu->AddSeparator();
         }
         //zooming options
         bool in = false, out = false;
         if(min_scale!=C_fixed::One()){
            in = (img_scale!=C_fixed::Zero());
            out = (img_scale!=C_fixed::One());
         }
         menu->AddItem(TXT_ZOOM_ORIGINAL, out ? 0 : C_menu::DISABLED, "[*]", "[R]", app.BUT_ZOOM_ORIGINAL);
         menu->AddItem(TXT_ZOOM_FIT, in ? 0 : C_menu::DISABLED, "[#]", "[T]", app.BUT_ZOOM_FIT);
         menu->AddItem(TXT_ZOOM_IN, out ? 0 : C_menu::DISABLED, "[5]", "[I]", app.BUT_ZOOM_IN);
         menu->AddItem(TXT_ZOOM_OUT, in ? 0 : C_menu::DISABLED, "[0]", "[O]", app.BUT_ZOOM_OUT);
         menu->AddSeparator();
      }
#ifdef EXPLORER
      menu->AddItem(TXT_FULLSCREEN, fullscreen ? C_menu::MARKED : 0, "[4]", "[F]");
      if(img){
         menu->AddItem(TXT_LANDSCAPE, landscape ? C_menu::MARKED : 0, "[7]", "[L]");
         int sx = img->SizeX();
         int sy = img->SizeY();
         if(landscape)
            Swap(sx, sy);
         if(sx>rc_img.sx || sy>rc_img.sy)
            menu->AddItem(TXT_SHOW_SCROLLBARS, show_scrollbars ? C_menu::MARKED : 0, "[8]", "[B]");
         if(C_wallpaper_utils::CanSetWallpaper()){
            menu->AddSeparator();
            menu->AddItem(TXT_SET_AS_WALLPAPER);
         }
      }
      menu->AddSeparator();
#endif
      menu->AddItem(!GetParent() ? TXT_EXIT : TXT_BACK);
   }
};

//----------------------------
#ifdef IMG_OPEN_IN_THREAD

void C_mode_image_viewer::ThreadProc(){

   img_state = IMG_OPENING;
   int lx = 0, ly = 0;
   C_fixed scale = img_scale;
   if(img_scale==C_fixed::Zero()){
                              //fit-to-screen mode
      lx = rc.sx;
      ly = rc.sy;
      if(landscape)
         Swap(lx, ly);
      scale = C_fixed::One();
#ifdef DEBUG_IMAGE_SCALE
      scale = C_fixed(DEBUG_IMAGE_SCALE);
      lx = C_fixed(lx)*scale;
      ly = C_fixed(ly)*scale;
#endif
   }
   img_y = -1;
   C_image::E_RESULT err;
   //Fatal(filename);
   //Info(filename);
   err = img->Open(filename, lx, ly, scale, arch, this, img_file_offset);
   if(err){
      img_state = err==C_image::IMG_NOT_ENOUGH_MEMORY ? IMG_ERROR_LOW_MEMORY : IMG_ERROR_OTHER;
      img_state_changed = true;
      return;
   }
   FinishImageOpen((img_load_progress.pos==0));
   img_state = IMG_OPENED;
   scale_display_countdown = SCALE_DISPLAY_COUNTDOWN;
   img_state_changed = true;

   if(IsThreadRunning())
   if(img->IsAnimated()){
      dword last_t = GetTickTime();
      while(!IsThreadQuitRequest()){
         system::Sleep(30);
         dword t = GetTickTime();
         if(img->Tick(t-last_t)){
            img_state_changed = true;
         }
         last_t = t;
      }
      img->Close();
   }
}

#endif//IMG_OPEN_IN_THREAD
//----------------------------

void C_mode_image_viewer::InitLayout(){

   /*
   if(fullscreen){
      RemoveSoftkeyBar();
   }else
   */
   {
      //AddSoftkeyBar();
   }
   super::InitLayout();
   if(fullscreen){
      rc_area.sy = app.ScrnSY()-rc_area.y;
   }
   rc = rc_area;
   if(!fullscreen)
      rc.Compact(1);
   {
      int sb_size = app.GetScrollbarWidth();
      if(fullscreen)
         sb_size = Max(3, sb_size/2);
                              //init scrollbar
      sb.visible_space = rc.sy;
      sb.rc = S_rect(rc.Right()-sb_size-2, rc.y+3, sb_size, rc.sy-6);

      sb_h.rc = S_rect(rc.x+3, rc.Bottom()-sb_size-2, rc.sx-sb.rc.sx-9, sb_size);

      sb.SetVisibleFlag();
   }
#ifdef IMG_OPEN_IN_THREAD
   img_load_progress.rc = S_rect(rc.CenterX(), rc.Bottom()-app.fds.line_spacing-3, Min(app.ScrnSX()/2, app.fds.cell_size_x*20), app.fds.line_spacing);
   img_load_progress.rc.x -= img_load_progress.rc.sx/2;
#endif
   if(img)
      OpenImage();
}

//----------------------------

void C_mode_image_viewer::FinishImageOpen(bool set_pos){

   if(min_scale==C_fixed::One()){
      min_scale = C_fixed(img->SizeX()) / C_fixed(img->GetOriginalSize().x);
      assert(min_scale <= C_fixed::One());
   }
                           //init view rectangle, scrollbars, offset
   rc_img = rc;
   int sx = img->SizeX();
   int sy = img->SizeY();
   if(landscape)
      Swap(sx, sy);

   if((sx>rc.sx || sy>rc.sy) && show_scrollbars){
                           //check height, width, and then height again for case it was lowered
      bool h_low = false;
      if(sy>rc.sy){
         rc_img.sx -= sb.rc.sx+4;
         h_low = true;
      }
      if(sx>rc_img.sx){
         rc_img.sy -= sb_h.rc.sy+4;
         if(!h_low && sy>rc_img.sy)
            rc_img.sx -= sb.rc.sx+4;
      }

      sb.visible_space = rc_img.sy;
      sb.total_space = sy;
      sb.SetVisibleFlag();

      sb_h.rc.sx = rc.sx - 6;
      if(sb.visible)
         sb_h.rc.sx -= sb.rc.sx + 3;
      sb_h.visible_space = rc_img.sx;
      sb_h.total_space = sx;
      sb_h.SetVisibleFlag();
   }else{
      sb_h.visible = false;
      sb.visible = false;
   }
   if(set_pos){
      img_x = (rc_img.sx - sx) / 2;
      img_y = (rc_img.sy - sy) / 2;
   }
   sb.pos = -img_y;
   sb_h.pos = -img_x;
   EnableButtons();
}

//----------------------------

bool C_mode_image_viewer::OpenImage(){

   if(viewer_previous_next){
      has_previous = viewer_previous_next->GetPreviousNextFile(false, NULL, NULL);
      has_next = viewer_previous_next->GetPreviousNextFile(true, NULL, NULL);
   }
#ifdef IMG_OPEN_IN_THREAD
   AskToQuitThread();
   img = C_image::Create(app); img->Release();

   img_state = IMG_CLOSED;
   img_state_changed = false;
   img_load_progress.pos = 0;
   img_load_progress.total = 100;
   scale_display_countdown = 0;
   rc_img = rc;
   kinetic_movement.Reset();

#if defined __SYMBIAN32__ && defined EXPLORER
                              //on Symbian, we can't open file from msgs archive in another thread
   if(arch)
      ThreadProc();
   else
#endif
   CreateThread();
#else
   img = C_image::Create(app); img->Release();
                              //prepare loading progress
   C_client::S_progress_display_help ip(&app);
   app.PrepareProgressBarDisplay(TXT_OPENING, NULL, ip, 100);

   int lx = 0, ly = 0;
   C_fixed scale = img_scale;
   if(img_scale==C_fixed::Zero()){
                              //fit-to-screen mode
      lx = rc.sx;
      ly = rc.sy;
      if(landscape)
         Swap(lx, ly);
      scale = C_fixed::One();
#ifdef DEBUG_IMAGE_SCALE
      scale = C_fixed(DEBUG_IMAGE_SCALE);
      lx = C_fixed(lx)*scale;
      ly = C_fixed(ly)*scale;
#endif
   }
   C_image::E_RESULT err = img->Open(filename, lx, ly, scale, DisplayProgressBar, &ip, arch, img_file_offset);
   if(err){
      if(err==C_image::IMG_NOT_ENOUGH_MEMORY)
         app.ShowErrorWindow(TXT_ERROR, TXT_ERR_NOT_ENOUGH_MEM);
      return false;
   }
   FinishImageOpen(true);
#endif
   return true;
}

//----------------------------

void C_mode_image_viewer::ProcessMenu(int itm, dword mid){

   switch(itm){
   case TXT_ZOOM_FIT:
      ImageZoomFit();
      break;
   case TXT_ZOOM_ORIGINAL:
      ImageZoomOriginal();
      break;
   case TXT_ZOOM_IN:
      ImageZoomIn();
      break;
   case TXT_ZOOM_OUT:
      ImageZoomOut();
      break;

   case TXT_PREVIOUS:
   case TXT_NEXT:
      OpenPreviousNext((itm==TXT_NEXT));
      break;

   case TXT_BACK:
   case TXT_EXIT:
      Close();
      return;
#ifdef EXPLORER
   case TXT_FULLSCREEN:
      fullscreen = !fullscreen;
      DestroyTitleBar();
      InitLayout();
      break;
   case TXT_LANDSCAPE:
      landscape = !landscape;
      InitLayout();
      break;
   case TXT_SHOW_SCROLLBARS:
      show_scrollbars = !show_scrollbars;
      InitLayout();
      break;

   case TXT_SET_AS_WALLPAPER:
      if(!C_wallpaper_utils::SetWallpaper(filename)){
         app.ShowErrorWindow(TXT_ERROR, L"Can't set wallpaper");
      }else{
         CreateInfoMessage(app, TXT_INFORMATION, L"Wallpaper changed");
      }
      return;
#endif//EXPLORER
   }
}

//----------------------------

void C_mode_image_viewer::ProcessInput(S_user_input &ui, bool &redraw){

   super::ProcessInput(ui, redraw);
   if(ui.CheckMouseInRect(rc)){
      if(ui.mouse_buttons&MOUSE_BUTTON_1_DOWN){
#ifdef USE_MOUSE
         menu = app.CreateTouchMenu();
         if(viewer_previous_next){
            menu->AddItem(TXT_PREVIOUS, has_previous ? 0 : C_menu::DISABLED, "[2]", "[P]");
            menu->AddItem(TXT_NEXT, has_next ? 0 : C_menu::DISABLED, "[3]", "[N]");
         }else{
            menu->AddSeparator();
            menu->AddSeparator();
         }
         bool in = false, out = false;
         if(min_scale!=C_fixed::One()){
            in = (img_scale!=C_fixed::Zero());
            out = (img_scale!=C_fixed::One());
         }
         menu->AddItem(TXT_ZOOM_IN, out ? 0 : C_menu::DISABLED, 0, 0, app.BUT_ZOOM_IN);
         menu->AddItem(TXT_ZOOM_OUT, in ? 0 : C_menu::DISABLED, 0, 0, app.BUT_ZOOM_OUT);
         if(in || out)
            menu->AddItem(in ? TXT_ZOOM_FIT : TXT_ZOOM_ORIGINAL, 0, 0, 0, char(in ? app.BUT_ZOOM_FIT : app.BUT_ZOOM_ORIGINAL));
         app.PrepareTouchMenu(menu, ui);
#endif
      }
   }

   switch(ui.key){
   case K_CURSORLEFT:
   case K_CURSORRIGHT:
      if(img_scale==C_fixed(0)){
         if(OpenPreviousNext((ui.key==K_CURSORRIGHT)))
            redraw = true;
      }
      break;

   case '#':
   case 'a':
   case 't':
   case K_PAGE_DOWN:
      if(ImageZoomFit())
         redraw = true;
      break;

   case '*':
   case 'q':
   case 'r':
   case K_PAGE_UP:
      if(ImageZoomOriginal())
         redraw = true;
      break;

   case 'i':
   case '5':
      if(ImageZoomIn())
         redraw = true;
      break;

   case 'o':
   case '0':
      if(ImageZoomOut())
         redraw = true;
      break;

   case '2':
   case '3':
   case 'p':
   case 'n':
      if(OpenPreviousNext((ui.key=='3' || ui.key=='n')))
         redraw = true;
      break;

#ifdef EXPLORER
   case '4':
   case 'f':
      fullscreen = !fullscreen;
      DestroyTitleBar();
      InitLayout();
      redraw = true;
      break;

   case '7':
   case 'l':
      landscape = !landscape;
      InitLayout();
      redraw = true;
      break;

   case '8':
   case 'b':
      if(img && (img->SizeX()>dword(rc_img.sx) || img->SizeY()>dword(rc_img.sy))){
         show_scrollbars = !show_scrollbars;
         InitLayout();
         redraw = true;
      }
      break;
#endif
   }
   {
      S_point scrl(0, 0);
      C_scrollbar::E_PROCESS_MOUSE pm = app.ProcessScrollbarMouse(sb, ui);
      switch(pm){
      case C_scrollbar::PM_PROCESSED: redraw = true; break;
      case C_scrollbar::PM_CHANGED:
         img_y = -sb.pos;
         redraw = true;
         break;
      default:
         pm = app.ProcessScrollbarMouse(sb_h, ui);
         switch(pm){
         case C_scrollbar::PM_PROCESSED: redraw = true; break;
         case C_scrollbar::PM_CHANGED:
            img_x = -sb_h.pos;
            redraw = true;
            break;
         default:
            if((ui.mouse_buttons&MOUSE_BUTTON_1_DOWN) && ui.CheckMouseInRect(rc)){
               is_mouse_drag = true;
               drag_mouse = ui.mouse;
            }
            if(ui.mouse_buttons&MOUSE_BUTTON_1_DRAG){
               if(is_mouse_drag){
                  scrl = ui.mouse - drag_mouse;
                  drag_mouse = ui.mouse;
               }
            }
            if(ui.mouse_buttons&MOUSE_BUTTON_1_UP)
               is_mouse_drag = false;
         }
      }
      ScrollImage(scrl, redraw);

      kinetic_movement.ProcessInput(ui, rc);
      scroll_key_bits = ui.key_bits;
   }
}

//----------------------------

void C_mode_image_viewer::ScrollImage(const S_point &scrl, bool &redraw){

   int sx = img->SizeX();
   int sy = img->SizeY();
   if(landscape)
      Swap(sx, sy);

   if(scrl.y!=0 && sy > rc_img.sy){
      img_y += scrl.y;
      if(scrl.y > 0){
         if(img_y > 0)
            img_y = 0;
      }else{
         int min(rc_img.sy - sy);
         if(img_y < min)
            img_y = min;
      }
      sb.pos = -img_y;
      redraw = true;
   }
   if(scrl.x!=0 //&& sx > rc_img.sx
      ){
      img_x += scrl.x;
      /*
      if(scrl.x > 0){
         if(img_x > 0)
            img_x = 0;
      }else{
         int min(rc_img.sx - sx);
         if(img_x < min)
            img_x = min;
      }
      /**/
      sb_h.pos = -img_x;
      redraw = true;

      CheckSlideToPrevNext();
   }
}

//----------------------------

void C_mode_image_viewer::CheckSlideToPrevNext(){
   int sx = img->SizeX();
   int sy = img->SizeY();
   if(landscape)
      Swap(sx, sy);
   C_fixed min_x, max_x;
   if(sx<=rc_img.sx)
      min_x = max_x = (rc_img.sx - sx) / 2;
   else
      max_x = 0, min_x = rc_img.sx - sx;
   if(img_x<min_x || img_x>max_x){
                           //check if we're out too much
      const int NEXT_TRESH = Min(app.ScrnSX()/2, app.fdb.cell_size_x*15);
      if(min_x-img_x>=NEXT_TRESH || img_x-max_x>=NEXT_TRESH){
         if(OpenPreviousNext((img_x<min_x))){
            is_mouse_drag = false;
         }
      }
   }
}

//----------------------------

void C_mode_image_viewer::Tick(dword time, bool &redraw){

   bool redraw_img = false;
                              //image scrolling
   S_point scrl(0, 0);
   if(img_scale!=C_fixed::Zero()){
      C_fixed add = C_fixed(time) * C_fixed::Percent(20);
      if(scroll_key_bits&(GKEY_UP|GKEY_DOWN)){
         if(scroll_key_bits&GKEY_UP)
            scrl.y = add;
         else
            scrl.y = -add;
      }
      if(scroll_key_bits&(GKEY_LEFT|GKEY_RIGHT)){
         if(scroll_key_bits&GKEY_LEFT)
            scrl.x = add;
         else
            scrl.x = -add;
      }
   }
   if(kinetic_movement.IsAnimating()){
      S_point p;
      kinetic_movement.Tick(time, &p);
      scrl.x += p.x;
      scrl.y += p.y;
   }
   ScrollImage(scrl, redraw_img);
//#ifdef IMG_OPEN_IN_THREAD
   if(img_state_changed){
      img_state_changed = false;
      redraw = true;
   }
//#else
#if defined __SYMBIAN32__ && defined EXPLORER
                           //tick animations
   if(img->IsAnimated() && !IsThreadRunning()){
      if(img->Tick(time))
         redraw_img = true;
   }
#endif
//#endif
   {
      int sx = img->SizeX();
      int sy = img->SizeY();
      if(landscape)
         Swap(sx, sy);
      C_fixed min_x, max_x;
      if(sx<=rc_img.sx)
         min_x = max_x = (rc_img.sx - sx) / 2;
      else
         max_x = 0, min_x = rc_img.sx - sx;
      if(img_x<min_x || img_x>max_x){
                              //check if we're out too much
         if(!is_mouse_drag// && img_state==IMG_OPENED
            ){
                              //try to return back
            const int SCROLL_PER_SEC = app.ScrnSX()*3;
            C_fixed scroll = SCROLL_PER_SEC*time/1000;
            if(img_x < min_x){
               img_x = Min(img_x+scroll, min_x);
            }else{
               img_x = Max(img_x-scroll, max_x);
            }
            redraw_img = true;
         }
      }
   }
   if(scale_display_countdown){
      if((scale_display_countdown-=time)<0)
         scale_display_countdown = 0;
      if(scale_display_countdown<SCALE_DISPLAY_FADE){
         redraw_img = true;
      }
   }
   if(redraw_img){
      if(!menu)
         DrawImage();
      else
         redraw = true;
   }
}

//----------------------------

void C_mode_image_viewer::DrawImage() const{

   app.AddDirtyRect(rc_area);
#ifdef IMG_BGND_COLOR
   app.FillRect(rc, IMG_BGND_COLOR);
#else
   app.FillRect(rc, 0xff000000);
#endif
   if(viewer_previous_next && is_mouse_drag){
                              //draw prev/next arrows
      if(has_previous)
         app.DrawString(L"<", rc_img.x+app.fdb.cell_size_x, rc_img.CenterY()-app.fdb.cell_size_y/2, app.UI_FONT_BIG, FF_BOLD, 0x40ffffff);
      if(has_next)
         app.DrawString(L">", rc_img.Right()-app.fdb.cell_size_x, rc_img.CenterY()-app.fdb.cell_size_y/2, app.UI_FONT_BIG, FF_RIGHT|FF_BOLD, 0x40ffffff);
   }

   if((img_state==IMG_OPENING && img_load_progress.pos>0) || img_state==IMG_OPENED){
      int dx = rc_img.x + img_x, dy = rc_img.y + img_y;
      if(img->IsTransparent()){
         S_rect rc_fit(dx, dy, img->SizeX(), img->SizeY());
         if(landscape)
            Swap(rc_fit.sx, rc_fit.sy);
         rc_fit.SetIntersection(rc_fit, rc_img);
                           //draw chess board
         app.SetClipRect(rc_fit);
         S_rect rc_chess(rc_img.x, rc_img.y, app.fdb.cell_size_x*2, 0);
         rc_chess.sy = rc_chess.sx;
         dword c0 = 0xff505050, c1 = 0xff202020;
         while(rc_chess.y < rc_fit.Bottom()){
            S_rect rc1 = rc_chess;
            dword c0a = c0, c1a = c1;
            while(rc1.x < rc_fit.Right()){
               app.FillRect(rc1, c0a);
               rc1.x += rc_chess.sx;
               Swap(c0a, c1a);
            }
            Swap(c0, c1);
            rc_chess.y += rc_chess.sy;
         }
      }
      S_rect rc_clip = rc_img;
#ifdef IMG_OPEN_IN_THREAD
      if(img_state==IMG_OPENING){
                                 //change drawn image height
         S_rect rcx(dx, dy, img->SizeX(), img->SizeY());
         int d = (rcx.sy * (img_load_progress.pos-1) / 100) - 1;
         if(landscape){
            Swap(rcx.sx, rcx.sy);
            rcx.x += rcx.sx-d;
            rcx.sx = d;
         }else
            rcx.sy = d;
         rc_clip.SetIntersection(rc_clip, rcx);
      }
#endif
      if(!rc_clip.IsEmpty()){
         app.SetClipRect(rc_clip);
         if(landscape)
            img->Draw(dx, dy, landscape ? -1 : 0);
         else
            img->Draw(dx, dy);
         app.ResetClipRect();
      }
#ifdef IMG_OPEN_IN_THREAD
      if(img_state!=IMG_OPENING)
#endif
      {
         app.DrawScrollbar(sb);
         app.DrawScrollbar(sb_h);
      }
      DrawScalePercent();
   }
   const wchar *txt = NULL;
   Cstr_w tmp;
   switch(img_state){
   case IMG_CLOSED:
   case IMG_OPENING:
      txt = app.GetText(TXT_OPENING);
      break;
   case IMG_ERROR_LOW_MEMORY:
      txt = app.GetText(TXT_ERR_NOT_ENOUGH_MEM);
      break;
   case IMG_ERROR_OTHER:
      tmp<<app.GetText(TXT_ERR_CANT_OPEN_FILE) <<' ' <<file_utils::GetFileNameNoPath(filename);
      txt = tmp;
      break;
   }
   if(txt)
      app.DrawString(txt, rc.CenterX(), img_load_progress.rc.y-app.fdb.line_spacing, app.UI_FONT_BIG, (0xc0<<FF_SHADOW_ALPHA_SHIFT) | FF_CENTER, 0xffffffff);
   if(img_state==IMG_OPENING)
      app.DrawProgress(img_load_progress);

}

//----------------------------

void C_mode_image_viewer::DrawScalePercent() const{

   if(img_state!=IMG_OPENING && img_state!=IMG_CLOSED && scale_display_countdown){
                           //draw scale percent
      dword isx = img->SizeX(), isy = img->SizeY();
      const S_point osz = img->GetOriginalSize();
      if(isx && isy && osz.x && osz.y){
         dword px;
         if(isx>isy)
            px = (isx * 100 + osz.x/2) / osz.x;
         else
            px = (isy * 100 + osz.y/2) / osz.y;
         Cstr_w s; s<<px <<L'%';
         dword col = 0x80ffffff;
         if(scale_display_countdown<SCALE_DISPLAY_FADE)
            col = MulAlpha(col, (scale_display_countdown<<16)/SCALE_DISPLAY_FADE);
         if(col&0xff000000)
            app.DrawString(s, rc_area.x+app.fdb.letter_size_x, rc_area.y, app.UI_FONT_BIG, (0x80<<FF_SHADOW_ALPHA_SHIFT), col);
      }
   }
}

//----------------------------

void C_mode_image_viewer::Draw() const{

   super::Draw();
   DrawImage();
   if(menu && fullscreen && !app.HasMouse())
      app.ClearSoftButtonsArea();
   if(!fullscreen){
      const dword c0 = app.GetColor(app.COL_SHADOW), c1 = app.GetColor(app.COL_HIGHLIGHT);
      app.DrawOutline(rc, c0, c1);
   }
}

//----------------------------

bool CreateImageViewer(C_client &app, C_viewer_previous_next *vpn, const Cstr_w &title, const Cstr_w &fn, const C_zip_package *arch, dword img_file_offset,
   bool allow_delete){

   new(true) C_mode_image_viewer(app, vpn, title, fn, arch, img_file_offset);
   return true;
}

//----------------------------

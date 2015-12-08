//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <UI\SkinnedApp.h>
#include <TextUtils.h>
#include "Graph_i.h"

//----------------------------

class C_skin_interface: public C_unknown{
public:
   virtual dword GetColor(C_application_ui::E_COLOR col) const = 0;
   virtual void DrawDialogBase(const S_rect &rc, bool top_bar, const S_rect *rc_clip) = 0;
   virtual void DrawEditedTextFrame(const S_rect &rc) = 0;
   virtual void DrawButton(const C_button &but, bool enabled) = 0;
   virtual bool DrawSoftButtonRectangle(const C_button &but) = 0;
   virtual void DrawSelection(const S_rect &rc, bool clear_to_bgnd) = 0;
   virtual void ClearToBackground(const S_rect &rc) = 0;
   virtual void ClearWorkArea(const S_rect &rc) = 0;
   virtual void DrawMenuBackground(const C_menu *menu, bool also_frame, bool dimmed) = 0;
   virtual void DrawScrollbar(const C_scrollbar &sb, dword bgnd) = 0;
};
#define GET_SKIN_INTERFACE C_skin_interface &skin = (C_skin_interface&)*skin_imp; skin

//----------------------------

class C_skin: public C_skin_interface{
   C_application_skinned &app;
public:
   enum E_SKIN_ITEM{
      SKIN_ITEM_BACKGROUND,
      SKIN_ITEM_SCROLL_BAR,
      SKIN_ITEM_SCROLL_THUMB,
      SKIN_ITEM_MENU,
      SKIN_ITEM_SELECTION,
      SKIN_ITEM_SOFTKEY,
      SKIN_ITEM_TEXT,
      SKIN_ITEM_BUTTON,
      SKIN_ITEM_LAST
   };
   C_smart_ptr<C_image> images[SKIN_ITEM_LAST];
   struct{
      dword text, popup, menu, highlight, softkeys, title;
   } colors;

   C_skin(C_application_skinned &_app):
      app(_app)
   {}
   bool Init();

//----------------------------

   void DrawSkinnedItem(const S_rect &rc, dword si){
      const C_image *img = images[si];
      const int border = img->SizeX()/5;

      img->DrawNinePatch(rc, S_point(border, border));
      app.AddDirtyRect(rc);
   }

//----------------------------

   virtual dword GetColor(C_application_ui::E_COLOR col) const{

      switch(col){
      default: return app.C_application_ui::GetColor(col);
      case C_application_ui::COL_EDITING_STATE:
      case C_application_ui::COL_TEXT: return colors.text;
      case C_application_ui::COL_TEXT_POPUP: return colors.popup;
      case C_application_ui::COL_TEXT_MENU_SECONDARY:
      case C_application_ui::COL_TEXT_MENU: return colors.menu;
      case C_application_ui::COL_TEXT_HIGHLIGHTED: return colors.highlight;
      case C_application_ui::COL_TEXT_SOFTKEY: return colors.softkeys;
      case C_application_ui::COL_TEXT_TITLE: return colors.title;
      }
   }
//----------------------------
   virtual void DrawDialogBase(const S_rect &rc, bool top_bar, const S_rect *rc_clip){
      S_rect save_clip_rc = app.GetClipRect();
      if(rc_clip)
         app.C_application_base::SetClipRect(*rc_clip);
      if(!rc_clip)
         app.DrawShadow(rc, true);

      S_rect rc1 = rc;
      rc1.Expand(1);
      app.AddDirtyRect(rc1);
      DrawSkinnedItem(rc, SKIN_ITEM_MENU);

      if(top_bar){
         S_rect rc_top(rc.x+1, rc.y+1, rc.sx-2, app.GetDialogTitleHeight()-2);
         app.FillRect(rc_top, MulAlpha(app.GetColor(app.COL_TEXT_POPUP), 0x4000));

         int y = rc_top.Bottom()-1;
         app.DrawHorizontalLine(rc.x, y, rc.sx, MulAlpha(app.GetColor(app.COL_TEXT_POPUP), 0x4000));
      }
      if(rc_clip)
         app.C_application_base::SetClipRect(save_clip_rc);
   }
//----------------------------
   virtual void DrawEditedTextFrame(const S_rect &rc){
      S_rect rc1 = rc;
      rc1.Expand(1);
      DrawSkinnedItem(rc1, C_skin::SKIN_ITEM_TEXT);
   }
//----------------------------
   virtual void DrawButton(const C_button &but, bool enabled){
      if(!images[C_skin::SKIN_ITEM_BUTTON]->SizeX()){
         app.C_application_ui::DrawButton(but, enabled);
      }else{
         S_rect rc1 = but.GetRect();
         rc1.Expand(1);
         DrawSkinnedItem(rc1, SKIN_ITEM_BUTTON);
         if(but.down)
            app.FillRect(rc1, MulAlpha(app.GetColor(app.COL_TEXT_SOFTKEY), 0x4000));
      }
   }
//----------------------------
   virtual bool DrawSoftButtonRectangle(const C_button &but){
      if(!app.HasMouse())
         return false;
      S_rect rc1 = but.GetRect();
      rc1.Expand(1);
      DrawSkinnedItem(rc1, SKIN_ITEM_SOFTKEY);
      if(but.down)
         app.FillRect(rc1, MulAlpha(app.GetColor(app.COL_TEXT_SOFTKEY), 0x4000));
      return true;
   }
//----------------------------
   virtual void DrawSelection(const S_rect &rc, bool clear_to_bgnd){

      if(clear_to_bgnd)
         ClearToBackground(rc);
      DrawSkinnedItem(rc, SKIN_ITEM_SELECTION);
   }
//----------------------------
   virtual void ClearToBackground(const S_rect &rc){
      C_clip_rect_helper crh(app);
      if(crh.CombineClipRect(rc))
         images[SKIN_ITEM_BACKGROUND]->DrawZoomed(S_rect(0, 0, app.ScrnSX(), app.ScrnSY()), false);
      app.AddDirtyRect(rc);
   }
//----------------------------
   virtual void ClearWorkArea(const S_rect &rc){
      ClearToBackground(rc);
   }
//----------------------------
   virtual void DrawMenuBackground(const C_menu *menu, bool also_frame, bool dimmed){
      S_rect rc = menu->GetRect();
      if(also_frame)
         app.DrawShadow(rc, false);
      rc.Expand(1);
      DrawSkinnedItem(rc, SKIN_ITEM_MENU);
      if(dimmed)
         app.FillRect(rc, 0x80404040);
   }
//----------------------------
   virtual void DrawScrollbar(const C_scrollbar &sb, dword bgnd){

      if(!sb.visible)
         return;
      S_rect rc = sb.rc;
      rc.Expand(1);
      if(bgnd){
         switch(bgnd){
         case C_application_ui::SCROLLBAR_DRAW_BACKGROUND:
            ClearToBackground(rc);
            break;
         default:
            app.FillRect(rc, bgnd);
         }
      }
      bool vertical = (sb.rc.sy > sb.rc.sx);
      if(vertical){
         //C_application_ui::DrawScrollbar(sb, bgnd);
         app.AddDirtyRect(rc);

         C_clip_rect_helper crh(app);
         {
            //DrawOutline(rc, 0xff000000);
                              //draw frame
            const C_image *img = images[C_skin::SKIN_ITEM_SCROLL_BAR];

            S_rect rc_top(rc.x, rc.y, rc.sx, Min(rc.sx, rc.sy/2));
            if(crh.CombineClipRect(rc_top))
               img->Draw(rc_top.x, rc_top.y);

            S_rect rc_bot(rc.x, rc.Bottom()-rc_top.sy, rc.sx, rc_top.sy);
            if(crh.CombineClipRect(rc_bot))
               img->Draw(rc_bot.x, rc_bot.Bottom()-img->SizeY());

            S_rect rc_cnt(rc.x, rc_top.Bottom(), Min(rc.sx, (int)img->SizeX()), rc_bot.y-rc_top.Bottom());
            if(crh.CombineClipRect(rc_cnt)){
               rc_cnt.y -= rc_cnt.sy;
               rc_cnt.sy *= 3;
               img->DrawZoomed(rc_cnt);
            }
         }
         rc.Compact(2);
         {
            sb.MakeThumbRect(app, rc);
            int osx = rc.sx;
            if(sb.mouse_drag_pos!=-1){
#ifdef ANDROID
               rc.x -= rc.sx*2;
               rc.sx *= 3;
#else
               rc.Compact();
               osx -= 2;
#endif
            }
            rc.Compact();
            //DrawOutline(rc, 0xff000000);

            const C_image *img = images[C_skin::SKIN_ITEM_SCROLL_THUMB];

            S_rect rc_top(rc.x, rc.y, rc.sx, Min(osx, rc.sy));
            if(crh.CombineClipRect(rc_top))
               img->DrawZoomed(S_rect(rc_top.x, rc_top.y, rc_top.sx, img->SizeY()));

            S_rect rc_bot(rc.x, rc.Bottom()-rc_top.sy, rc.sx, rc_top.sy);
            if(crh.CombineClipRect(rc_bot))
               img->DrawZoomed(S_rect(rc_bot.x, rc_bot.Bottom()-img->SizeY(), rc_bot.sx, img->SizeY()));

            S_rect rc_cnt(rc.x, rc_top.Bottom(), rc.sx, rc_bot.y-rc_top.Bottom());
            if(crh.CombineClipRect(rc_cnt)){
               rc_cnt.y -= rc_cnt.sy;
               rc_cnt.sy *= 3;
               img->DrawZoomed(rc_cnt);
            }
         }
      }else
         app.C_application_ui::DrawScrollbar(sb, bgnd);
   }
};

//----------------------------

bool C_skin::Init(){

   Cstr_w fn;
#ifdef ANDROID
   fn<<L"<\\";
#endif
   if(app.GetDataPath())
      fn<<app.GetDataPath();
   fn<<L"Skin.dta";
   C_zip_package *dta = C_zip_package::Create(fn);
   if(!dta){
      //Fatal(fn);
      LOG_RUN("Failed to load skin.dta");
      assert(0);
      return false;
   }

   static const struct S_def{
      const wchar *name;
      byte border_percent;
   } defs[C_skin::SKIN_ITEM_LAST] = {
      { L"bgnd.jpg" },
      { L"scroll_bar.png" },
      { L"scroll_thumb.png" },
      { L"menu.png", },
      { L"selection.png", 80 },
      { L"softkey.png", 50 },
      { L"text.png", 70 },
      { L"button.png", 70 },
   };
   for(int itm_i=0; itm_i<C_skin::SKIN_ITEM_LAST; itm_i++){
      const S_def &def = defs[itm_i];

      C_image *img = C_image::Create(app);
      switch(itm_i){
      case C_skin::SKIN_ITEM_BACKGROUND:
         img->Open(def.name, app.ScrnSX(), app.ScrnSY(), C_fixed::One(), dta, NULL, 0, NULL,
            //C_image::IMAGE_OPEN_NO_DITHERING |
            C_image::IMAGE_OPEN_NO_ASPECT_RATIO);
         break;

      case C_skin::SKIN_ITEM_SCROLL_BAR:
      case C_skin::SKIN_ITEM_SCROLL_THUMB:
         img->Open(def.name, app.GetScrollbarWidth()+(itm_i==C_skin::SKIN_ITEM_SCROLL_BAR ? 2 : -2), 0, C_fixed::One(), dta, NULL, 0, NULL, C_image::IMAGE_OPEN_NO_DITHERING);
         break;

      case C_skin::SKIN_ITEM_MENU:
      case C_skin::SKIN_ITEM_SELECTION:
      case C_skin::SKIN_ITEM_SOFTKEY:
      case C_skin::SKIN_ITEM_BUTTON:
      case C_skin::SKIN_ITEM_TEXT:
         {
            C_fixed f_border = C_fixed(app.ScrnSX()+app.ScrnSY())/C_fixed(160);
            if(def.border_percent)
               f_border *= C_fixed::Percent(def.border_percent);
            int limit = Max(1, int(f_border)) * 5;
            img->Open(def.name, limit, limit, C_fixed::One(), dta, NULL, 0, NULL, C_image::IMAGE_OPEN_NO_DITHERING);
         }
         break;
      default:
         assert(0);
      }
      images[itm_i] = img;
      img->Release();
   }
                           //init colors
   {
      MemSet(&colors, 0xff, sizeof(colors));
      C_file_zip fl;
      Cstr_w fn; fn<<L"colors.txt";
      if(fl.Open(fn, dta)){
         dword di = 0;
         while(!fl.IsEof() && di<sizeof(colors)/sizeof(dword)){
            Cstr_c line = fl.GetLine();
            const char *cp = line;
            if(*cp==';')
               continue;
            dword col;
            if(!text_utils::ScanHexNumber(cp, (int&)col)){
               assert(0);
               break;
            }
            col |= 0xff000000;
            ((dword*)&colors)[di++] = col;
         }
      }
   }
   dta->Release();
   return true;
}

//----------------------------

void C_application_skinned::EnableSkins(byte en){

   use_skins = en;
   if(!en || !igraph){
      skin_imp = NULL;
      return;
   }
   C_skin *skin = new(true) C_skin(*this);
   if(skin->Init())
      skin_imp = skin;
   else
      use_skins = false;

   skin->Release();
}

//----------------------------

void C_application_skinned::InitAfterScreenResize(){

   super::InitAfterScreenResize();
   EnableSkins(use_skins);
}

//----------------------------

void C_application_skinned::ClearToBackground(const S_rect &rc){

   if(skin_imp){
      GET_SKIN_INTERFACE.ClearToBackground(rc);
   }else
      super::ClearToBackground(rc);
}

//----------------------------

void C_application_skinned::ClearWorkArea(const S_rect &rc){

   if(skin_imp){
      GET_SKIN_INTERFACE.ClearWorkArea(rc);
   }else
      super::ClearWorkArea(rc);
}

//----------------------------

void C_application_skinned::DrawScrollbar(const C_scrollbar &sb, dword bgnd){

   if(!skin_imp){
      super::DrawScrollbar(sb, bgnd);
      return;
   }
   GET_SKIN_INTERFACE.DrawScrollbar(sb, bgnd);
}

//----------------------------

dword C_application_skinned::GetScrollbarWidth() const{

                              //limit by width of skin's scrollbar's image
   return Min(30ul, super::GetScrollbarWidth());
}

//----------------------------

void C_application_skinned::DrawMenuBackground(const C_menu *menu, bool also_frame, bool dimmed){

   if(!skin_imp){
      super::DrawMenuBackground(menu, also_frame, dimmed);
      return;
   }
   GET_SKIN_INTERFACE.DrawMenuBackground(menu, also_frame, dimmed);
}

//----------------------------

void C_application_skinned::DrawSelection(const S_rect &rc, bool clear_to_bgnd){

   if(!skin_imp){
      super::DrawSelection(rc, clear_to_bgnd);
      return;
   }
   GET_SKIN_INTERFACE.DrawSelection(rc, clear_to_bgnd);
}

//----------------------------

bool C_application_skinned::DrawSoftButtonRectangle(const C_button &but){

   if(!skin_imp)
      return super::DrawSoftButtonRectangle(but);
   GET_SKIN_INTERFACE;
   return skin.DrawSoftButtonRectangle(but);
}

//----------------------------

void C_application_skinned::DrawButton(const C_button &but, bool enabled){
   if(!skin_imp)
      super::DrawButton(but, enabled);
   else{
      GET_SKIN_INTERFACE.DrawButton(but, enabled);
   }
}

//----------------------------

bool C_application_skinned::IsEditedTextFrameTransparent() const{
   return true;
}

//----------------------------

void C_application_skinned::DrawEditedTextFrame(const S_rect &rc){

   if(!skin_imp){
      super::DrawEditedTextFrame(rc);
      return;
   }
   GET_SKIN_INTERFACE.DrawEditedTextFrame(rc);
}

//----------------------------

void C_application_skinned::DrawDialogBase(const S_rect &rc, bool draw_top_bar, const S_rect *rc_clip){

   if(!skin_imp){
      super::DrawDialogBase(rc, draw_top_bar, rc_clip);
      return;
   }
   GET_SKIN_INTERFACE.DrawDialogBase(rc, draw_top_bar, rc_clip);
}

//----------------------------

dword C_application_skinned::GetColor(E_COLOR col) const{

   if(skin_imp){
      GET_SKIN_INTERFACE;
      return skin.GetColor(col);
   }
   return super::GetColor(col);
}

//----------------------------

void C_application_skinned::GetDialogRectPadding(int &l, int &t, int &r, int &b) const{ super::GetDialogRectPadding(l, t, r, b); }
void C_application_skinned::SetClipRect(const S_rect &rc){ super::SetClipRect(rc); }
void C_application_skinned::DrawProgress(const C_progress_indicator &pi, dword color, bool border, dword col_bgnd){ super::DrawProgress(pi, color, border, col_bgnd); }
void C_application_skinned::DrawSeparator(int l, int sx, int y){ super::DrawSeparator(l, sx, y); }
void C_application_skinned::DrawThickSeparator(int l, int sx, int y){ super::DrawThickSeparator(l, sx, y); }
void C_application_skinned::DrawMenuSelection(const C_menu *menu, const S_rect &rc){ super::DrawMenuSelection(menu, rc); }
void C_application_skinned::DrawPreviewWindow(const S_rect &rc){ super::DrawPreviewWindow(rc); }
dword C_application_skinned::GetProgressbarHeight() const{ return super::GetProgressbarHeight(); }
void C_application_skinned::ClearTitleBar(int bottom){ super::ClearTitleBar(bottom); }
void C_application_skinned::ClearSoftButtonsArea(int top){ super::ClearSoftButtonsArea(top); }
void C_application_skinned::DrawCheckbox(int x, int y, int size, bool enabled, bool frame, byte alpha){ super::DrawCheckbox(x, y, size, enabled, frame, alpha); }

//----------------------------

//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------
#include <E32Std.h>
#include <UI\SkinnedApp.h>

//----------------------------

#ifdef USE_SYSTEM_SKIN

void C_application_skinned::EnableSkins(byte en){
   use_skins = en;
   if(!use_skins)
      skin_imp = NULL;
}

#ifdef S60
#include <AknsUtils.h>
#include <AknsBasicBackgroundControlContext.h>
#include <AknsDrawUtils.h>
#include <CoeCntrl.h>

class C_skin_cache: public C_unknown{
public:
   CAknsBasicBackgroundControlContext *bgnd;
   C_skin_cache():
      bgnd(NULL)
   {}
   ~C_skin_cache(){
      delete bgnd;
   }
};

//----------------------------

void C_application_skinned::InitAfterScreenResize(){
   skin_imp = NULL;
   super::InitAfterScreenResize();
}

//----------------------------

void C_application_skinned::SetClipRect(const S_rect &rc){

   super::SetClipRect(rc);
   if(use_skins && !system_font.use){
      S_rect rc1 = rc;
      rc1.sx += rc1.x;
      rc1.sy += rc1.y;
      const TRect &rrc((TRect&)rc1);
      CFbsBitGc &gc = igraph->GetBitmapGc();
      gc.SetClippingRect(rrc);
      //gc.SetClippingRegion(RRegion(rrc));
   }
}

//----------------------------

void C_application_skinned::ClearToBackground(const S_rect &rc){

   if(use_skins){
      TRect rect(rc.x, rc.y, rc.sx, rc.sy);
      rect.iBr += rect.iTl;
      C_skin_cache *sc;
      if(!skin_imp){
         sc = new(true) C_skin_cache;
         skin_imp = sc;
         sc->Release();
         TPoint scrn_pt = igraph->GetCoeControl()->Position();
         sc->bgnd = CAknsBasicBackgroundControlContext::NewL(KAknsIIDQsnBgScreen, TRect(-scrn_pt.iX, -scrn_pt.iY, ScrnSX(), ScrnSY()), false);
      }else
         sc = (C_skin_cache*)(C_unknown*)skin_imp;
      AknsDrawUtils::DrawBackground(AknsUtils::SkinInstance(), sc->bgnd,
         NULL,
         igraph->GetBitmapGc(),
         rect.iTl,
         rect,
         KAknsDrawParamDefault);

      AddDirtyRect(rc);
   }else
      super::ClearToBackground(rc);
}

//----------------------------

static bool SymbianS60DrawSkin(CFbsBitGc &gc, const S_rect &rc, const TAknsItemID &id, dword border = 0, bool clear_bgnd = true){

   TRect rect(rc.x, rc.y, rc.x+rc.sx, rc.y+rc.sy),
      rc_in = rect;
   rc_in.Shrink(border, border);
   return AknsDrawUtils::DrawFrame(AknsUtils::SkinInstance(), gc, rect, rc_in, id, KAknsIIDDefault, clear_bgnd ? KAknsDrawParamRGBOnly : KAknsDrawParamDefault);
}

//----------------------------

void C_application_skinned::DrawScrollbar(const C_scrollbar &sb, dword bgnd){

   if(!sb.visible)
      return;
   if(use_skins){
      S_rect rc = sb.rc;
      rc.Expand(1);
      if(bgnd){
         switch(bgnd){
         case SCROLLBAR_DRAW_BACKGROUND:
            ClearToBackground(rc);
            break;
         default:
            FillRect(rc, bgnd);
         }
      }
      CFbsBitGc &gc = igraph->GetBitmapGc();

      /*
      static const TAknsItemID KAknsIIDQsnCpScrollBgBottomPressed = { EAknsMajorGeneric, 0x20f5 },
         KAknsIIDQsnCpScrollBgMiddlePressed = { EAknsMajorGeneric, 0x20f6 },
         KAknsIIDQsnCpScrollBgTopPressed = { EAknsMajorGeneric, 0x20f7 },
         KAknsIIDQsnCpScrollHandleBottomPressed = { EAknsMajorGeneric, 0x20f8 },
         KAknsIIDQsnCpScrollHandleMiddlePressed = { EAknsMajorGeneric, 0x20f9 },
         KAknsIIDQsnCpScrollHandleTopPressed = { EAknsMajorGeneric, 0x20fa },
         KAknsIIDQsnCpScrollHorizontalBgBottomPressed = { EAknsMajorGeneric, 0x20fb },
         KAknsIIDQsnCpScrollHorizontalBgMiddlePressed = { EAknsMajorGeneric, 0x20fc },
         KAknsIIDQsnCpScrollHorizontalBgTopPressed = { EAknsMajorGeneric, 0x20fd },
         KAknsIIDQsnCpScrollHorizontalHandleBottomPressed = { EAknsMajorGeneric, 0x20fe },
         KAknsIIDQsnCpScrollHorizontalHandleMiddlePressed = { EAknsMajorGeneric, 0x20ff },
         KAknsIIDQsnCpScrollHorizontalHandleTopPressed = { EAknsMajorGeneric, 0x2100 };
         */

      bool vertical = (sb.rc.sy > sb.rc.sx);
      //bool pressed = (sb.mouse_drag_pos!=-1);
      if(vertical){
         AddDirtyRect(rc);
         {
                              //draw frame
            rc.Expand(1);
            S_rect rc_top(rc.x, rc.y, rc.sx, Min(rc.sx, rc.sy/2));
            S_rect rc_bot(rc.x, rc.Bottom()-rc_top.sy, rc.sx, rc_top.sy);
            S_rect rc_mid(rc.x, rc_top.Bottom(), rc.sx, rc_bot.y-rc_top.Bottom());
            SymbianS60DrawSkin(gc, rc_top, KAknsIIDQsnCpScrollBgTop, 0, false);
            SymbianS60DrawSkin(gc, rc_bot, KAknsIIDQsnCpScrollBgBottom, 0, false);
            SymbianS60DrawSkin(gc, rc_mid, KAknsIIDQsnCpScrollBgMiddle, 0, false);
            rc.Compact(1);
         }
         {
            sb.MakeThumbRect(*this, rc);
            if(sb.mouse_drag_pos!=-1){
               rc.Compact();
               //rc.x++;
               //rc.sx -= 2;
            }
            S_rect rc_top(rc.x, rc.y, rc.sx, Min(rc.sx, rc.sy));
            S_rect rc_bot(rc.x, rc.Bottom()-rc_top.sy, rc.sx, rc_top.sy);
            S_rect rc_mid(rc.x, rc_top.Bottom(), rc.sx, rc_bot.y-rc_top.Bottom());
            SymbianS60DrawSkin(gc, rc_top, KAknsIIDQsnCpScrollHandleTop, 0, false);
            SymbianS60DrawSkin(gc, rc_bot, KAknsIIDQsnCpScrollHandleBottom, 0, false);
            SymbianS60DrawSkin(gc, rc_mid, KAknsIIDQsnCpScrollHandleMiddle, 0, false);
         }
      }else{
         /*
         if(HasMouse()){
            static const TAknsItemID KAknsIIDQsnCpScrollHorizontalBgBottom = { EAknsMajorGeneric, 0x177a },
               KAknsIIDQsnCpScrollHorizontalBgMiddle = { EAknsMajorGeneric, 0x177b },
               KAknsIIDQsnCpScrollHorizontalBgTop = { EAknsMajorGeneric, 0x177c },
               KAknsIIDQsnCpScrollHorizontalHandleBottom = { EAknsMajorGeneric, 0x177d },
               KAknsIIDQsnCpScrollHorizontalHandleMiddle = { EAknsMajorGeneric, 0x177e },
               KAknsIIDQsnCpScrollHorizontalHandleTop = { EAknsMajorGeneric, 0x177f };

                              //Symbian 9.4
            {
                                 //draw frame
               rc.Expand(1);
               S_rect rc_left(rc.x, rc.y, Min(rc.sy, rc.sx/2), rc.sy);
               S_rect rc_right(rc.Right()-rc_left.sx, rc.y, rc_left.sx, rc.sy);
               S_rect rc_mid(rc_left.Right(), rc.y, rc_right.x-rc_left.Right(), rc.sy);
               SymbianS60DrawSkin(gc, rc_left, KAknsIIDQsnCpScrollHorizontalBgTop, 0, false);
               SymbianS60DrawSkin(gc, rc_right, KAknsIIDQsnCpScrollHorizontalBgBottom, 0, false);
               SymbianS60DrawSkin(gc, rc_mid, KAknsIIDQsnCpScrollHorizontalBgMiddle, 0, false);
               rc.Compact(1);
            }
            {
               sb.MakeThumbRect(*this, rc);
               S_rect rc_left(rc.x, rc.y, Min(rc.sx, rc.sy), rc.sy);
               S_rect rc_right(rc.Right()-rc_left.sx, rc.y, rc_left.sx, rc.sy);
               S_rect rc_mid(rc_left.Right(), rc.y, rc_right.x-rc_left.Right(), rc.sy);
               SymbianS60DrawSkin(gc, rc_left, KAknsIIDQsnCpScrollHorizontalHandleTop, 0, false);
               SymbianS60DrawSkin(gc, rc_right, KAknsIIDQsnCpScrollHorizontalHandleBottom, 0, false);
               SymbianS60DrawSkin(gc, rc_mid, KAknsIIDQsnCpScrollHorizontalHandleMiddle, 0, false);
            }
            AddDirtyRect(rc);
         }else
         */
            super::DrawScrollbar(sb, bgnd);
      }
   }else
      super::DrawScrollbar(sb, bgnd);
}

//----------------------------

void C_application_skinned::DrawProgress(const C_progress_indicator &pi, dword color, bool border, dword col_bgnd){

   if(use_skins && !color){
      S_rect rc = pi.rc;
      CFbsBitGc &gc = igraph->GetBitmapGc();
      rc.Expand(1);
      AddDirtyRect(rc);
      if(border){

         S_rect rc_left(rc.x, rc.y, Min(rc.sy, rc.sx/2), rc.sy);
         S_rect rc_right(rc.Right()-rc_left.sx, rc.y, rc_left.sx, rc.sy);
         S_rect rc_mid(rc_left.Right(), rc.y, rc_right.x-rc_left.Right(), rc.sy);
         SymbianS60DrawSkin(gc, rc_left, KAknsIIDQgnGrafBarFrameSideL, 0, false);
         SymbianS60DrawSkin(gc, rc_right, KAknsIIDQgnGrafBarFrameSideR, 0, false);
         SymbianS60DrawSkin(gc, rc_mid, KAknsIIDQgnGrafBarFrameCenter, 0, false);
         rc.Compact(1);
      }
      int sx = 0;
      dword t = pi.total;
      if(t){
         dword p = pi.pos;
                                 //don't make math overflow
         while(t>=0x800000)
            t >>= 1, p >>= 1;
         sx = Min((dword)rc.sx, (dword)rc.sx * Max(p, 0ul) / t);
      }
      if(sx){
         rc.sx = sx;
         SymbianS60DrawSkin(gc, rc, KAknsIIDQgnGrafBarProgress, 0, false);
      }
   }else
      super::DrawProgress(pi, color, border, col_bgnd);
}

//----------------------------

void C_application_skinned::DrawMenuBackground(const C_menu *menu, bool also_frame, bool dimmed){

   if(use_skins// && !menu->touch_mode
      ){
      const int border = 2;
      S_rect rc = menu->GetRect();
      rc.Expand(1);
      SymbianS60DrawSkin(igraph->GetBitmapGc(), rc, !menu->GetParentMenu() ? KAknsIIDQsnFrPopup : KAknsIIDQsnFrPopupSub, border, false);
      AddDirtyRect(rc);
      if(also_frame)
         DrawShadow(menu->GetRect(), true);
      if(dimmed)
         FillRect(rc, 0x80404040);
   }else
      super::DrawMenuBackground(menu, also_frame, dimmed);
}

//----------------------------

bool C_application_skinned::IsEditedTextFrameTransparent() const{
   return true;
}

//----------------------------

void C_application_skinned::DrawEditedTextFrame(const S_rect &rc){

   if(use_skins){
      S_rect rc1 = rc;
      rc1.Expand(1);
      AddDirtyRect(rc1);
      SymbianS60DrawSkin(igraph->GetBitmapGc(), rc1, KAknsIIDQsnFrInput, 2, false);
   }else
      super::DrawEditedTextFrame(rc);
}

//----------------------------

void C_application_skinned::DrawSelection(const S_rect &rc, bool clear_to_bgnd){

   if(use_skins){
      if(clear_to_bgnd)
         ClearToBackground(rc);
      SymbianS60DrawSkin(igraph->GetBitmapGc(), rc, KAknsIIDQsnFrList, 4, false);
      AddDirtyRect(rc);
   }else
      super::DrawSelection(rc, clear_to_bgnd);
}

//----------------------------

void C_application_skinned::DrawSeparator(int l, int sx, int y){

   if(use_skins){
      //DrawHorizontalLine(l, y, sx, 0x40000000 & GetColor(COL_TEXT_POPUP));

      const S_rect &clip_rc = GetClipRect();
      if(y>=clip_rc.y && y<clip_rc.Bottom()){
         S_rect rc(l, y, sx, 1);
         AddDirtyRect(rc);
         SymbianS60DrawSkin(igraph->GetBitmapGc(), rc,
            //KAknsIIDQgnGrafLineSecondaryHorizontal,
            KAknsIIDQgnGrafLinePrimaryHorizontal,
            0, false);
                              //it seems to reset gc's clip rect, need to call this
         SetClipRect(clip_rc);
      }
   }else
      super::DrawSeparator(l, sx, y);
}

//----------------------------

void C_application_skinned::DrawThickSeparator(int l, int sx, int y){
   super::DrawThickSeparator(l, sx, y);
}

//----------------------------

void C_application_skinned::DrawPreviewWindow(const S_rect &rc){

   if(use_skins){
      S_rect rc1 = rc;
      rc1.Expand(2);
      AddDirtyRect(rc1);
      SymbianS60DrawSkin(igraph->GetBitmapGc(), rc1,
         //KAknsIIDQsnFrPopupPreview, 2);
         KAknsIIDQsnFrPopup, 0);
   }else
      super::DrawPreviewWindow(rc);
}

//----------------------------

void C_application_skinned::DrawDialogBase(const S_rect &rc, bool draw_top_bar, const S_rect *rc_clip){

   if(use_skins){
      CFbsBitGc &gc = igraph->GetBitmapGc();
      S_rect save_clip_rc = GetClipRect();
      if(rc_clip){
         S_rect crc;
         crc.SetIntersection(save_clip_rc, *rc_clip);
         if(crc.IsEmpty())
            return;
         SetClipRect(crc);
      }else
         DrawShadow(rc, true);

      const int border = 1;
      S_rect rc1 = rc;
      rc1.Expand(1);
      AddDirtyRect(rc1);
      SymbianS60DrawSkin(gc, rc1, KAknsIIDQsnFrPopup, border);

      if(draw_top_bar){
         S_rect rc_top(rc.x, rc.y, rc.sx, GetDialogTitleHeight()-1);
                              //can't use KAknsIIDQsnFrPopupHeading, because it's uncommon and some themes make it colored that it is unreadable
         //SymbianS60DrawSkin(igraph->GetBitmapGc(), rc_top, KAknsIIDQsnFrPopupHeading, 2);
                              //just make color gradient
         FillRect(rc_top, 0x40000000 & GetColor(COL_TEXT_POPUP));

         int y = rc_top.Bottom()-1;
         DrawHorizontalLine(rc.x, y, rc.sx, MulAlpha(GetColor(COL_TEXT_POPUP), 0x4000));
      }
      if(rc_clip)
         SetClipRect(save_clip_rc);
   }else
      super::DrawDialogBase(rc, draw_top_bar, rc_clip);
}

//----------------------------

void C_application_skinned::DrawButton(const C_button &but, bool enabled){

   if(use_skins){
      S_rect rc1 = but.GetRect();
      rc1.Expand(1);
      AddDirtyRect(rc1);

      static const TAknsItemID KAknsIIDQsnFrFunctionButtonNormal = { EAknsMajorSkin, 0x2658 },
         KAknsIIDQsnFrFunctionButtonInactive = { EAknsMajorSkin, 0x264e },
         KAknsIIDQsnFrFunctionButtonPressed = { EAknsMajorSkin, 0x2662 };

      SymbianS60DrawSkin(igraph->GetBitmapGc(), rc1,
         !enabled ? KAknsIIDQsnFrFunctionButtonInactive : but.down ? KAknsIIDQsnFrFunctionButtonPressed : KAknsIIDQsnFrFunctionButtonNormal, rc1.sy/4, false);
   }else
      super::DrawButton(but, enabled);
}

//----------------------------

bool C_application_skinned::DrawSoftButtonRectangle(const C_button &but){

   if(!HasMouse())
      return false;
   if(use_skins){
      S_rect rc1 = but.GetRect();
      rc1.Expand(1);
      AddDirtyRect(rc1);

      static const TAknsItemID KAknsIIDQgnFrSctrlSkButton = { EAknsMajorSkin, 0x2400 },
         KAknsIIDQgnFrSctrlSkButtonPressed = { EAknsMajorSkin, 0x2676 };

      SymbianS60DrawSkin(igraph->GetBitmapGc(), rc1,
         but.down ? KAknsIIDQgnFrSctrlSkButtonPressed : KAknsIIDQgnFrSctrlSkButton, rc1.sy/4, false);

      return true;
   }else
      return super::DrawSoftButtonRectangle(but);
}

//----------------------------

void C_application_skinned::ClearWorkArea(const S_rect &rc){

   if(use_skins){
      ClearToBackground(rc);
   }else
      super::ClearWorkArea(rc);
}

//----------------------------

void C_application_skinned::DrawMenuSelection(const C_menu *menu, const S_rect &rc){
   DrawSelection(rc);
}

//----------------------------

dword C_application_skinned::GetScrollbarWidth() const{
   return super::GetScrollbarWidth();
}

//----------------------------

dword C_application_skinned::GetProgressbarHeight() const{
   return super::GetProgressbarHeight();
}

#endif//S60

//----------------------------

dword C_application_skinned::GetColor(E_COLOR col) const{

   switch(col){
   case COL_TEXT:
   case COL_TEXT_HIGHLIGHTED:
   case COL_TEXT_SELECTED:
   case COL_TEXT_SELECTED_BGND:
   case COL_TEXT_INPUT:
   case COL_EDITING_STATE:
   case COL_TEXT_SOFTKEY:
   case COL_TEXT_TITLE:
   case COL_TEXT_POPUP:
   case COL_TEXT_MENU:
   case COL_TEXT_MENU_SECONDARY:
      if(use_skins){
#ifdef S60
         TRgb rgb;
         dword ci;
#ifdef __SYMBIAN_3RD__
         TAknsItemID id = KAknsIIDQsnTextColors;
         switch(col){
         default: assert(0);
         case COL_TEXT: ci = EAknsCIQsnTextColorsCG6; break;
         case COL_TEXT_HIGHLIGHTED: ci = EAknsCIQsnTextColorsCG10; break;
         case COL_TEXT_SELECTED: ci = EAknsCIQsnTextColorsCG24; break;
         case COL_TEXT_SELECTED_BGND: id = KAknsIIDQsnHighlightColors; ci = EAknsCIQsnHighlightColorsCG2; break;
         case COL_TEXT_INPUT: ci = EAknsCIQsnTextColorsCG27; break;
         case COL_EDITING_STATE:
            //id = KAknsIIDQsnIconColors; ci = EAknsCIQsnIconColorsCG4;
                              //return color of softkeys, as normal editing state is typically at top, while we draw it at bottom
            ci = EAknsCIQsnTextColorsCG13;
            break;
         case COL_TEXT_SOFTKEY: ci = EAknsCIQsnTextColorsCG13; break;
         case COL_TEXT_TITLE: ci = EAknsCIQsnTextColorsCG1; break;
         case COL_TEXT_MENU:
         case COL_TEXT_POPUP:
            ci = EAknsCIQsnTextColorsCG19;
            break;
         case COL_TEXT_MENU_SECONDARY: ci = EAknsCIQsnTextColorsCG20; break;
         }
#else
         TAknsItemID id = KAknsIIDQsnComponentColors;
         switch(col){
         default: //assert(0);
            return super::GetColor(col);
         case COL_TEXT_INPUT:
         case COL_TEXT: ci = EAknsCIQsnComponentColorsCG5; break;
         case COL_TEXT_HIGHLIGHTED:
         case COL_TEXT_TITLE: ci = EAknsCIQsnComponentColorsCG6a; break;
         case COL_EDITING_STATE:
         case COL_TEXT_SOFTKEY: ci = EAknsCIQsnComponentColorsCG2; break;
         case COL_TEXT_MENU:
         case COL_TEXT_MENU_SECONDARY:
         case COL_TEXT_POPUP: ci = EAknsCIQsnComponentColorsCG3; break;
         }
#endif
         AknsUtils::GetCachedColor(AknsUtils::SkinInstance(), rgb, id, ci);
         return 0xff000000 | rgb.Color16M();
#endif
      }//else
         //return color_themes[config.color_theme].text_color;
      break;
   }
   return super::GetColor(col);
}

//----------------------------

void C_application_skinned::ClearTitleBar(int b){ super::ClearTitleBar(b); }
void C_application_skinned::ClearSoftButtonsArea(int t){ super::ClearSoftButtonsArea(t); }
void C_application_skinned::GetDialogRectPadding(int &l, int &t, int &r, int &b) const{ super::GetDialogRectPadding(l, t, r, b); }
void C_application_skinned::DrawCheckbox(int x, int y, int size, bool enabled, bool frame, byte alpha){ super::DrawCheckbox(x, y, size, enabled, frame, alpha); }

//----------------------------

#else
#include "..\Common\SkinsOwn.cpp"
#endif   //__SYMBIAN_3RD__

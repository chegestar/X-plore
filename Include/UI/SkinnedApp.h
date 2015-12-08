//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#ifndef __SKINS_H_
#define __SKINS_H_

#if defined __SYMBIAN_3RD__ || defined ANDROID
#define USE_SYSTEM_SKIN
#endif
#include <UI\UserInterface.h>

class C_application_skinned: public C_application_ui{
   typedef C_application_ui super;
protected:
   byte use_skins;
   C_smart_ptr<C_unknown> skin_imp;
public:
   virtual dword GetColor(E_COLOR col) const;
   virtual void ClearToBackground(const S_rect &rc);
   virtual void ClearWorkArea(const S_rect &rc);
   virtual void DrawScrollbar(const C_scrollbar &sb, dword bgnd = SCROLLBAR_DRAW_NO_BGND);
   virtual void DrawProgress(const C_progress_indicator &pi, dword color = 0, bool border = true, dword col_bgnd = 0);
   virtual void DrawMenuBackground(const C_menu *menu, bool also_frame, bool dimmed);
   virtual void ClearTitleBar(int bottom = -1);
   virtual void ClearSoftButtonsArea(int top = -1);
   virtual void DrawEditedTextFrame(const S_rect &rc);
   virtual bool IsEditedTextFrameTransparent() const;
   virtual void DrawSelection(const S_rect &rc, bool clear_to_bgnd = false);
   virtual void DrawSeparator(int l, int sx, int y);
   virtual void DrawThickSeparator(int l, int sx, int y);
   virtual void DrawPreviewWindow(const S_rect &rc);
   virtual void DrawDialogBase(const S_rect &rc, bool draw_top_bar, const S_rect *rc_clip = NULL);
   virtual void GetDialogRectPadding(int &l, int &t, int &r, int &b) const;
   virtual void DrawMenuSelection(const C_menu *menu, const S_rect &rc);
   virtual bool DrawSoftButtonRectangle(const C_button &but);
   virtual dword GetScrollbarWidth() const;
   virtual dword GetProgressbarHeight() const;
   virtual void DrawButton(const C_button &but, bool enabled);
   virtual void DrawCheckbox(int x, int y, int size, bool enabled, bool frame = true, byte alpha = 255);

   void SetClipRect(const S_rect &rc);

   C_application_skinned():
      use_skins(false)
   {}
   virtual void InitAfterScreenResize();
//----------------------------
// Enable using of system skins.
// type: 0 = off, 1+ = on
   void EnableSkins(byte type);
};

//----------------------------

#endif

//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//
// Base for mode (single activity unit).
// This is base class for one activity mode which is active and displayed at one time in application.
//----------------------------

#ifndef __MODE_H_
#define __MODE_H_

//----------------------------
class C_mode;
                              //base class for single UI control.
class C_ui_control: public C_unknown{
   friend class C_mode;
   friend class C_application_ui;
   mutable bool need_draw;
   bool visible;              //visibility flag, checked in C_mode::DrawControl
   bool overlapped;           //marks that control is placed on top of other control, so if it is transparent, it is redrawn specially
protected:
   S_rect rc;                 //rectangle relative to screen origin, in which this control fits
   C_mode &mod;               //mode on which we operate
   dword id;                  //unique identifier of this control (may be 0 if ctrl doesn't need identification)

   virtual void SetFocus(bool){}
   virtual bool IsTouchFocusable() const{ return false; }

//----------------------------
// Reset touch input; called by touch menu when it takes control of touch.
   //virtual void ResetTouchInput(){}

public:
   short template_index;      //!!!(hide) index into layout template (internal)
   C_ui_control(C_mode *m, dword _id = 0);
   inline C_application_ui &App() const;

   const S_rect &GetRect() const{ return rc; }

//----------------------------
// Set placement of control. It is done in layout phase.
   virtual void SetRect(const S_rect &_rc){ rc = _rc; }

//----------------------------
   virtual void SetVisible(bool v);
   inline bool IsVisible() const{ return visible; }

//----------------------------
// Set flag for proper redrawing of control placed above other control.
   void SetOverlapped(bool o = true){ overlapped = o; }
//
   //virtual void Layout(){}

//----------------------------
// Mark this control to be redrawn. It is redrawn stand-alone, or as part ot mode's drawing.
// In stand-alone drawing, if it is transparent, screen is cleared to background (C_mode::ClearToBackground).
   void MarkRedraw();

//----------------------------
// Return true if control is transparent, which means that background is drawn before control draws its own graphics.
   virtual bool IsTransparent() const{ return false; }

//----------------------------
   //virtual void InitLayout(){}

//----------------------------
// Draw contents of control.
   virtual void Draw() const;

//----------------------------
// Process input in control. It may be touch event on clicked control, or key event on focused control.
   virtual void ProcessInput(S_user_input &ui){ }

//----------------------------
// Get default height of control (e.g. line spacing for text line).
   virtual dword GetDefaultHeight() const{ return 0; }

//----------------------------
// Create timer for control. Control's Tick function will be called in specified intervals.
// Inherited control class must keep timer itself, it's not stored in C_ui_control, since most controls don't need timer.
// It's control's task to delete the timer when no longer needed.
   C_application_base::C_timer *CreateTimer(dword ms);

//----------------------------
// Tick control by periodical timer. 'time' is time from last call of this function, in milliseconds.
   virtual void Tick(dword time){ }
};

//----------------------------
//----------------------------

class C_ctrl_text_line: public C_ui_control{
   Cstr_w text;
   dword draw_text_flags;
   dword font_size;
   dword color;               //0 = default
   byte alpha;
   virtual bool IsTransparent() const{ return true; }
public:
   C_ctrl_text_line(C_mode *m, dword _id = 0):
      C_ui_control(m, _id),
      draw_text_flags(0),
      font_size(0),
      alpha(0xff)
   {}

   void SetText(const Cstr_w &t, dword _font_size = C_application_base::UI_FONT_BIG, dword flags = 0, dword _color = 0);
   inline const Cstr_w &GetText() const{ return text; }
   inline void SetAlpha(byte a){ alpha = a; }

   virtual void Draw() const;
//----------------------------
// Compute exact width of control (in pixels), assuming text widht.
   dword GetDefaultWidth() const;
//----------------------------
   virtual dword GetDefaultHeight() const;
};

//----------------------------
//----------------------------
                              //control with optional outline drawn around its rectange
class C_control_outline: public C_ui_control{
protected:
   dword outline_color, outline_contrast;
   void Draw(const S_rect *rc = NULL) const;
public:
   C_control_outline(C_mode *m, dword _id = 0):
      C_ui_control(m, _id),
      outline_color(0)
   {}
//----------------------------
// Set color for outline drawn around image. It may be drawn outside of control's rectangle. Setting col to 0 disables outline (default).
// Color is fixed-point value specifying how top-left color will differ from bottom-right color, 0 means these are same, 0x10000 means there's maximum difference.
   void SetOutlineColor(dword col, dword contrast = 0);
};

//----------------------------
//----------------------------

class C_ctrl_image: public C_control_outline{
   typedef C_control_outline super;
public:
   enum E_SHADOW_MODE{
      SHADOW_NO,              //don't draw shadow
      SHADOW_YES,             //draw shadow
      SHADOW_YES_IF_OPAQUE,   //    ''      if image is not transparent
   };
   enum E_ALIGN_HORIZONTAL{
      HALIGN_LEFT,
      HALIGN_CENTER,
      HALIGN_RIGHT,
   };
   enum E_ALIGN_VERTICAL{
      VALIGN_TOP,
      VALIGN_CENTER,
      VALIGN_BOTTOM,
   };
protected:
   C_smart_ptr<C_image> img;
   Cstr_w filename;
   bool use_dta;
   bool allow_zoom_up;
   bool layout_shrink_rect_sy;
   E_SHADOW_MODE shadow_mode;
   S_rect rc_img;
   E_ALIGN_HORIZONTAL halign;
   E_ALIGN_VERTICAL valign;
public:
   C_ctrl_image(C_mode *m, const Cstr_w &fn, bool _use_dta = true, dword id = 0);

   //virtual void InitLayout();
//----------------------------
// Set rectangle, and load image. Real control's rect may be adjusted (smaller) after loading images and adjusting alignment.
   virtual void SetRect(const S_rect &_rc);
   virtual void Draw() const;

//----------------------------
// Get real rectangle into which image is drawn.
   inline const S_rect &GetImageRect() const{ return rc_img; }
   inline const S_point &GetImageSize() const{ return img->GetSize(); }

//----------------------------
// Set how image is drawn. If zoom-up is allowed, image tries to fit rect even if source is smaller. Otherwise it is drawn maximally in original resolution.
// This must be set before layout phase (when image is loaded and rect computed).
   inline void SetAllowZoomUp(bool b){ allow_zoom_up = b; }

//----------------------------
// Set horizontal/vertical alignment of image's rect in control's rect for cases when it doesn't fill entire control's rect.
// By default it is centered.
   void SetHorizontalAlign(E_ALIGN_HORIZONTAL ha);
   void SetVerticalAlign(E_ALIGN_VERTICAL va);

//----------------------------
// Set shadow mode for image.
   inline void SetShadowMode(E_SHADOW_MODE m){ shadow_mode = m; }

//----------------------------
// Set if control's rect is shrink vertically to drawn image size, or original height remains.
   inline void SetShrinkRectVertically(bool b){ layout_shrink_rect_sy = b; }
};

//----------------------------
//----------------------------

class C_ctrl_title_bar: public C_ui_control{
   typedef C_ui_control super;
   Cstr_w text;
#ifdef UI_ALLOW_ROTATED_SOFTKEYS
   bool draw_soft_key;
#endif
   friend class C_mode;
public:
   C_ctrl_title_bar(C_mode *m):
      super(m)
#ifdef UI_ALLOW_ROTATED_SOFTKEYS
      , draw_soft_key(false)
#endif
   {}
   inline void SetText(const Cstr_w &t){ text = t; }
   inline const Cstr_w &GetText() const{ return text; }

//----------------------------
// Init title bar rectangle. This func places it at top with default height.
   virtual void InitLayout();

//----------------------------
   virtual void Draw() const;
};

//----------------------------
//----------------------------

class C_ctrl_softkey_bar: public C_ui_control{
   static const int NUM_BUTTONS = 4;
   struct S_button_info{
      short img_index;
      bool enabled;
      dword txt_hint;
   };
   S_button_info button_info[NUM_BUTTONS];
   C_smart_ptr<C_text_editor> curr_te;
protected:
   C_button soft_keys[2];
#ifdef UI_ALLOW_ROTATED_SOFTKEYS
   bool right_aligned;        //true if softkeys are aligned on right (rotated screen)
#endif
   friend class C_mode;
public:
   C_ctrl_softkey_bar(C_mode *m);
   virtual void Draw() const;
   virtual void InitLayout();
   virtual void ProcessInput(S_user_input &ui);

//----------------------------
//
   void ResetAllButtons();
   void EnableButton(dword index, bool en){ button_info[index].enabled = en; MarkRedraw(); }
   void InitButton(dword index, short img_index, dword txt_id_hint = 0){
      S_button_info &bi = button_info[index];
      bi.img_index = img_index;
      bi.txt_hint = txt_id_hint;
      MarkRedraw();
   }
//----------------------------
// Set currently active text editor, of which state is shown at softkey bar.
// C_ctrl_text_entry_base is doint it itself in its SetFocus call.
   void SetActiveTextEditor(C_text_editor *te);
};

//----------------------------
//----------------------------

class C_ctrl_button_base: public C_ui_control{
   typedef C_ui_control super;
protected:
   virtual bool IsTransparent() const{ return true; }
   C_button but;
   bool enabled;
//----------------------------
// Called when button is clicked. Default function calls C_mode::OnButtonClicked.
   virtual void OnClicked();
public:
   C_ctrl_button_base(C_mode *m, dword id = 0);
   virtual void SetRect(const S_rect &_rc);
   virtual void ProcessInput(S_user_input &ui);
   virtual void Draw() const;

//----------------------------
// Enable/disable button.
   void SetEnabled(bool en);
};

//----------------------------
//----------------------------

class C_ctrl_button: public C_ctrl_button_base{
   typedef C_ctrl_button_base super;
   Cstr_w text;
public:
   C_ctrl_button(C_mode *m, dword _id = 0): super(m, _id){}
   virtual void Draw() const;

//----------------------------
   void SetText(const Cstr_w &t);
   inline const Cstr_w &GetText() const{ return text; }

//----------------------------
// Compute suggested width of button (in pixels), assuming text widht.
   dword GetDefaultWidth() const;

//----------------------------
// Get suggested height of button (in pixels).
   dword GetDefaultHeight() const;
};

//----------------------------
//----------------------------

class C_ctrl_image_button: public C_ctrl_button_base{
   typedef C_ctrl_button_base super;
   Cstr_w filename;
   C_smart_ptr<C_image> img;
   S_rect rc_img;
public:
   C_ctrl_image_button(C_mode *m, const wchar *fname, dword id = 0);
   virtual void SetRect(const S_rect &_rc);
   virtual void Draw() const;
};

//----------------------------
//----------------------------

class C_ctrl_text_entry_base: public C_ui_control, protected C_text_editor::C_text_editor_notify{
protected:
   C_smart_ptr<C_text_editor> te;
   S_point margin;

   virtual void SetFocus(bool focused);

                              //C_text_editor:
   virtual void CursorBlinked();
   virtual void CursorMoved();
   virtual void TextChanged();
   virtual bool IsTouchFocusable() const{ return true; }
public:
   C_ctrl_text_entry_base(C_mode *m, dword text_limit, dword tedf, dword font_size, dword id);
   virtual void OnChange(){}
   virtual void SetRect(const S_rect &_rc);
   virtual void SetText(const wchar *text){ te->SetInitText(text); }
   const wchar *GetText() const{ return te->GetText(); }
   const dword GetTextLength() const{ return te->GetTextLength(); }
   void SetCase(dword txt_case){ te->SetCase(txt_case, txt_case); }
   virtual void SetCursorPos(int pos, int sel){ te->SetCursorPos(pos, sel); }
   inline void SetCursorPos(int pos){ SetCursorPos(pos, pos); }
   virtual void Cut(){ te->Cut(); }
   virtual void Copy(){ te->Copy(); }
   virtual bool CanPaste() const{ return te->CanPaste(); }
   virtual void Paste(){ te->Paste(); }
   int GetCursorPos() const{ return te->GetCursorPos(); }
   int GetCursorSel() const{ return te->GetCursorSel(); }
   virtual void ReplaceSelection(const wchar *txt){ te->ReplaceSelection(txt); }
   dword GetMaxLength() const{ return te->GetMaxLength(); }
   void SetMargin(int x, int y);
   C_text_editor *GetTextEditor(){ return te; }
};

//----------------------------
//----------------------------

class C_ctrl_text_entry_line: public C_ctrl_text_entry_base{
protected:
public:
   C_ctrl_text_entry_line(C_mode *m, dword text_limit = 100, dword tedf = 0, dword font_size = C_application_base::UI_FONT_BIG, dword id = 0);

   virtual bool IsTransparent() const;
   virtual void SetCursorPos(int pos, int sel);
   inline void SetCursorPos(int pos){ SetCursorPos(pos, pos); }
   virtual void Draw() const;
   virtual void ProcessInput(S_user_input &ui);
};

//----------------------------
//----------------------------

class C_ctrl_text_entry: public C_ctrl_text_entry_base{
public:
                              //notification callback interface:
   class C_ctrl_text_entry_notify{
   public:
   //----------------------------
   // Cursor moved up while we're already on top line.
      virtual void TextEntryCursorMovedUpOnTop(C_ctrl_text_entry *ctrl){}
   //----------------------------
   // Cursor moved down while we're already on bottom line.
      virtual void TextEntryCursorMovedDownOnBottom(C_ctrl_text_entry *ctrl){}
   };
private:
   dword bgnd_color;
   C_scrollbar sb;
   bool mouse_selecting;
   struct S_text_line{
      const wchar *str;
      int len;
   };
   C_vector<S_text_line> lines;
   S_point pos_cursor, pos_min, pos_max;

   void PrepareLines(bool check_scroll);
   enum{
      SEL_NO,
      SEL_NORMAL,
      SEL_INLINE,
   } sel_type;
   C_ctrl_text_entry_notify *notify;
protected:
                              //C_text_editor:
   virtual void CursorMoved();
   virtual void TextChanged();

   virtual void SetFocus(bool focused);
public:
   C_ctrl_text_entry(C_mode *m, dword text_limit = 100, dword tedf = 0, dword font_size = C_application_base::UI_FONT_BIG, C_ctrl_text_entry_notify *notify = NULL, dword id = 0);
   virtual void SetRect(const S_rect &_rc);
   virtual void ProcessInput(S_user_input &ui);
   virtual void Draw() const;
   virtual void SetText(const wchar *text);
   virtual void SetCursorPos(int pos, int sel);
   virtual void ReplaceSelection(const wchar *txt);
   inline void SetCursorPos(int pos){ SetCursorPos(pos, pos); }

   virtual void Cut();
   virtual void Paste();
};

//----------------------------
//----------------------------
                              //progress bar
class C_ctrl_progress_bar: public C_ui_control{
   typedef C_ui_control super;
protected:
   virtual bool IsTransparent() const{ return true; }
   C_progress_indicator progress;
public:
   C_ctrl_progress_bar(C_mode *m, dword _id = 0):
      super(m, _id)
   {}
   virtual void SetRect(const S_rect &_rc);
   virtual void Draw() const;
   void SetPosition(int pos);
   void SetTotal(int total);
};

//----------------------------
//----------------------------
                              //simple horizontal line
class C_ctrl_horizontal_line: public C_ui_control{
public:
   C_ctrl_horizontal_line(C_mode *m):
      C_ui_control(m)
   {}
   virtual void Draw() const;
};

//----------------------------
//----------------------------

class C_ctrl_rich_text: public C_control_outline{
   typedef C_control_outline super;
   bool rc_set;
   byte font_index;
   bool dragging;
   class C_kmove: public C_kinetic_movement{
      virtual void AnimationStopped(){
         DeleteTimer();
         C_kinetic_movement::AnimationStopped();
      }
   public:
      C_application_base::C_timer *timer;
      C_kmove(): timer(NULL){}
      ~C_kmove(){ DeleteTimer(); }
      void DeleteTimer(){
         delete timer; timer = NULL;
      }
   } kinetic_movement;
protected:
   virtual bool IsTouchFocusable() const{ return true; }
   C_scrollbar sb;
   S_text_display_info tdi;
public:
   C_ctrl_rich_text(C_mode *m);

   void SetText(const char *text, dword size, dword font_index = C_application_base::UI_FONT_BIG);
//----------------------------
// Set bgnd color below text. -1 = default color (white).
   void SetBackgroundColor(dword color){
      tdi.bgnd_color = color;
   }
//----------------------------
// Add image into rich text. Returns index of image usable in rich text.
   dword AddImage(const S_image_record &ir);

//----------------------------
   void ScrollToTop();
   void ScrollToBottom();

   virtual void SetRect(const S_rect &_rc);
   virtual void ProcessInput(S_user_input &ui);
   virtual void Tick(dword time);
   virtual void Draw() const;
};

//----------------------------
//----------------------------
                              //control for displaying list of items
class C_ctrl_list: public C_ui_control{
   typedef C_ui_control super;
   bool dragging;
   int touch_down_selection;  //helper for selection by touch; keeps selection item activated when pen went down; for case when item is opened by pen up
   int touch_move_mouse_detect;
   C_kinetic_movement kinetic_scroll;
   C_application_base::C_timer *scroll_timer;   //timer for kinetic scrolling
   bool one_click_mode;
protected:
   int entry_height;          //height of single entry, in pixels
   int selection;             //current selection index from first entry
   C_scrollbar sb;

   void DeleteScrollTimer(){
      delete scroll_timer;
      scroll_timer = NULL;
   }

//----------------------------

   virtual void SetFocus(bool);

//----------------------------
// Return right-most x pixel, normally it's right pos of rect, if scrollbar is visible then it's at left side of scrollbar.
   inline int GetMaxX() const{
      return sb.visible ? sb.rc.x-1 : rc.Right();
   }

//----------------------------
// Draw item specified by item_index into given rectangle. Clip rect is set and can't be changed by this func.
// selection_highlight is set when selection was drawn below this item.
   virtual void DrawItem(S_rect &rc_item, int item_index, bool selection_highlight) const = 0;

//----------------------------
// Get number of entries in the list.
   virtual int GetNumEntries() const = 0;

//----------------------------
// Return number of items to move selection when skipping page up or down. Default function computes this from number of visible items.
   virtual dword GetPageSize(bool up);

//----------------------------
// Called from ProcessInputInList when selection is changed to different item.
   virtual void SelectionChanged(int prev_selection){}

//----------------------------
// Called from ProcessInputInList when scroll position is changed as result of using scroll bar, or dragging and moving content, or in kinetic scrolling animation.
   virtual void ScrollChanged(){}

public:
   C_ctrl_list(C_mode *m);
   ~C_ctrl_list();

   virtual void SetRect(const S_rect &_rc);
   virtual void ProcessInput(S_user_input &ui);
   virtual void Tick(dword time);
   virtual void Draw() const;
   virtual bool IsTransparent() const{ return true; }
   virtual bool IsTouchFocusable() const{ return true; }

   void SetOneClickMode(bool oc){ one_click_mode = oc; }

//----------------------------
   void SetEntryHeight(int h);

//----------------------------
// After scrolling by other mechanism (e.g. manipulating scrollbar), make sure selection is positioned on screen.
// This changes selection, not scrolling.
   //void MakeSureSelectionIsOnScreen();

//----------------------------
// Make sure that list is scrolled so that selected item is visible. Return true if top scroll position changed.
   bool EnsureVisible();

//----------------------------
// Stop any scrolling by timer.
   void StopScrolling();

   void SetSelection(int sel);
   inline int GetSelection() const{ return selection; }
};

//----------------------------
//----------------------------
                              //base class for operation mode - defining type and reference counter
                              // child classes contain specialization data for particular mode
class C_mode: public C_unknown{
   C_ctrl_title_bar *title_bar; //ref is kept in controls; if NULL, we don't have title bar
   C_ctrl_softkey_bar softkey_bar;
   bool softkey_bar_shown;

   friend class C_application_ui;
public://!!!
   C_vector<C_smart_ptr<C_ui_control> > controls;
private:
   C_ui_control *ctrl_focused, *ctrl_touched;
   bool any_ctrl_redraw;      //set when any control has dirty redraw flag
   mutable bool force_redraw;

   friend class C_ui_control;
protected:
   S_rect rc_area;            //usable client area without title bar and soft button bar; computed in InitLayout
   dword mode_id;             //mode identifier; set to unique four-cc code (only if it's needed to identify the mode)

//----------------------------
// Draw all dirty controls, or entire mode if force_redraw is set.
   void DrawDirtyControls();
public:
   C_application_ui &app;
   C_smart_ptr<C_mode> saved_parent;   //parent mode
   C_smart_ptr<C_menu> menu;     //menu used in this mode
   C_application_base::C_timer *timer;         //timer operating on this mode
   C_application_base::C_timer *socket_redraw_timer; //timer for redrawing socked operation
   dword last_socket_draw_time;

   C_mode(C_application_ui &_app, C_mode *sm);
   virtual ~C_mode(){
      DeleteTimer();
      delete socket_redraw_timer;
      //SetFocus(NULL); //can't be, could call funcs of already destroyed controls
   }

//----------------------------
// Set timer for mode. Mode will be ticked in specified intervals.
   void CreateTimer(dword ms);

//----------------------------
   inline void DeleteTimer(){
      delete timer;
      timer = NULL;
   }
//----------------------------
   inline C_ctrl_softkey_bar *GetSoftkeyBar(){ return softkey_bar_shown ? &softkey_bar : NULL; }
   inline const C_ctrl_softkey_bar *GetSoftkeyBar() const{ return softkey_bar_shown ? &softkey_bar : NULL; }
   inline C_ctrl_title_bar *GetTitleBar(){ return title_bar; }
   inline const C_ctrl_title_bar *GetTitleBar() const{ return title_bar; }
   inline void DestroyTitleBar(){ title_bar = NULL; }
//----------------------------
// Get application's reference.
   //inline C_application_ui &App() const{ return app; }
//----------------------------
// Get mode identifier.
   inline dword Id() const{ return mode_id; }
//----------------------------
// Get parent mode.
   inline C_mode *GetParent(){ return saved_parent; }
   inline const C_mode *GetParent() const{ return const_cast<C_mode*>(this)->GetParent(); }
//----------------------------
// Get menu.
   inline C_menu *GetMenu(){ return menu; }
   inline void SetMenu(C_menu *m){ menu = m; }
   inline const C_menu *GetMenu() const{ return const_cast<C_mode*>(this)->GetMenu(); }

//----------------------------
// Check if this mode is active mode.
   bool IsActive() const;

//----------------------------
// Test whether this mode is actively using Internet connection. If yes, connection is not closed automatically after some time.
   virtual bool IsUsingConnection() const{ return false; }

//----------------------------
// Test if mode needs active timer also when it is not active mode.
// If not, timer is paused when the mode is getting inactive (another mode is being activated), and timer is resumed when the mode is getting again top mode.
   virtual bool WantInactiveTimer() const{ return true; }

//----------------------------
// Test if mode needs active timer also when application is in background.
   virtual bool WantBackgroundTimer() const{ return WantInactiveTimer(); }

//----------------------------
// Reset touch input; called by touch menu when it takes control of touch.
   virtual void ResetTouchInput(){}

//----------------------------
// Initialize mode for drawing.
   virtual void InitLayout();

//----------------------------
// Process keys and touch input.
   virtual void ProcessInput(S_user_input &ui, bool &redraw);

//----------------------------
// Notification on selected menu item 'itm', or -1 if menu is closed, or -2 if selection changes (and menu sel change notify is set).
   virtual void ProcessMenu(int itm, dword menu_id){ assert(0); }

//----------------------------
// Tick mode by periodical timer. 'time' is time from last call of this function, in milliseconds.
   virtual void Tick(dword time, bool &redraw){ }

//----------------------------
// Draw entire mode's screen.
   virtual void Draw() const;

//----------------------------
// Event from socket operation.
   virtual void SocketEvent(C_socket_notify::E_SOCKET_EVENT ev, C_socket *socket, bool &redraw){ assert(0); }

//----------------------------
// Notification from active text editor.
   virtual void TextEditNotify(bool cursor_moved, bool text_changed, bool &redraw){ redraw = true; }

//----------------------------
// Called when application focus is changed.
   virtual void FocusChange(bool foreground){}

//----------------------------
// Draw parent mode, or clear screen if there's no parent.
   void DrawParentMode(bool wash_out = false) const;




//----------------------------
// Create menu for this mode. If there's already menu, it is stored as new menu's parent menu.
   C_smart_ptr<C_menu> CreateMenu(dword menu_id = 0);

                              //controls:
//----------------------------
// Set mode's title. This is done by calling active title_bar's SetText method.
   void SetTitle(const Cstr_w &title);

//----------------------------
// Create title bar. This allows modes to provide their own custom title bars.
   virtual C_ctrl_title_bar *CreateTitleBar(){
      return new(true) C_ctrl_title_bar(this);
   }

//----------------------------
// Add single control element. Returns reference to ctrl.
// If release_ref is set, Release is called on added control, so ownership is held by C_mode.
   C_ui_control &AddControl(C_ui_control *ctrl, bool release_ref = true);

//----------------------------
// Remove control from list of controls.
   bool RemoveControl(const C_ui_control *ctrl);

//----------------------------
// Get client rectangle (those between title bar and soft button bar).
   inline const S_rect &GetClientRect() const{ return rc_area; }

//----------------------------
// Set focus to given control. Keys will be sent to focused control.
   virtual void SetFocus(C_ui_control *ctrl);

//----------------------------
// Get currently focused control.
   C_ui_control *GetFocus(){ return ctrl_focused; }
   const C_ui_control *GetFocus() const{ return ctrl_focused; }

//----------------------------
// Mark entire mode for redrawing.
   void MarkRedraw(){ force_redraw = true; }
   inline bool IsForceRedraw() const{ return force_redraw; }

//----------------------------
// Get text id's for soft keys.
// If returned values are default (menu on left, back on right), ProcessInput provides given action (invokes menu or closes mode).
   virtual dword GetMenuSoftKey() const;
   virtual dword GetSecondarySoftKey() const;

//----------------------------
// Initialize menu items. Called by system when GetMenuSoftKey() returns SPECIAL_TEXT_MENU (default).
// Menu is created before call. Also focused control is un-focused before call.
   virtual void InitMenu(){}

//----------------------------
// Chance to initialize touch menu when some control is clicked.
// Implementing mode must create touch menu and setup menu items. if menu!=NULL, then it will be further prepared by caller code.
   virtual void InitTouchMenu(C_ui_control *touch_ctrl){ }

//----------------------------
// Activate this mode, make it current mode. Stop timer of parent mode (if any) if needed.
// After assigning to app.mode (smart ptr), Release is called on this.
   void Activate(bool draw = true);

//----------------------------
// Close this mode, give control to parent mode, or Exit app if there's no parent mode.
   virtual void Close(bool redraw_screen = true);

//----------------------------
// Remove soft key bar. It won't be drawn and react to input.
   void RemoveSoftkeyBar();
   void AddSoftkeyBar();

//----------------------------
// Called when button located on softkey bar is clicked (not softkey, but button between softkeys).
   virtual void OnSoftBarButtonPressed(dword index){}

//----------------------------
// Called when button control is clicked.
   virtual void OnButtonClicked(dword ctrl_id){}

//----------------------------
// Draw all controls in this mode.
   void DrawAllControls() const;

//----------------------------
// Called when control should be drawn. This allows to draw additional graphics on control.
   virtual void DrawControl(const C_ui_control *ctrl) const;

//----------------------------
// Clear to background in given rectangle of this mode.
   virtual void ClearToBackground(const S_rect &rc) const;

//----------------------------
// Calls app.CreateDtaFile, allows to cache dta if mode has many controls loading from dta.
   virtual const C_smart_ptr<C_zip_package> CreateDtaFile() const;
};

//----------------------------

inline C_application_ui &C_ui_control::App() const{ return mod.app; }

//----------------------------

#endif

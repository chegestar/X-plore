//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#ifndef __APP_BASE_H_
#define __APP_BASE_H_

#include <C_buffer.h>

#if defined _WIN32_WCE// || defined _WINDOWS
#define UI_MENU_ON_RIGHT      //softkey menu is on right, and may be optionally placed on left
#endif

#ifdef __SYMBIAN32__

                              //definition of Symbian App UID (each symbian app has different UID)
#ifdef __SYMBIAN_3RD__

#ifdef __WINS__
#define SYMBIAN_UID(n, c) \
   extern TEmulatorImageHeader uid; \
   TEmulatorImageHeader uid = { {0x1000007a, 0, n}/*UIDs*/, (TProcessPriority)350/*ProcessPriority=EPriorityForeground*/, {n/*SecureId*/, 0/*VendorId*/, {c, 0}/*CapabilitySet*/}, 0x00010000/*ModuleVersion*/, 0/*Flags*/};
#else
#define SYMBIAN_UID(n, c) \
   extern const dword uid; \
   const dword uid = n;
#endif

#else//__SYMBIAN_3RD__

#ifdef __WINS__
#define SYMBIAN_UID_BASE(n) \
   extern TUid uid[]; \
   TUid uid[] = {0x10000079, 0x100039ce, n};
#else
#define SYMBIAN_UID_BASE(n) \
   extern const TUid uid[]; \
   const TUid uid[] = {0x10000079, 0x100039ce, n};
#endif

#define SYMBIAN_UID(uid) SYMBIAN_UID_BASE(uid);

#endif//!__SYMBIAN_3RD__

#include <Rules.h>

                              //Symbian global data
struct S_global_data{
   wchar full_path[256];
   class CEikDocument *doc;
   class CEikApplication *GetApp() const;
   class CEikAppUi *GetAppUi() const;
   dword main_thread_id;
};
const S_global_data *GetGlobalData();

#endif//__SYMBIAN32__

#include <IGraph.h>

extern const int NUM_SYSTEM_FONTS;
//----------------------------

//----------------------------
// point - specifying 2D screen position
struct S_point{
   int x, y;
   S_point(){}
   S_point(int _x, int _y):
      x(_x), y(_y)
   {}
   inline bool operator ==(const S_point &p) const{ return (x==p.x && y==p.y); }
   inline bool operator !=(const S_point &p) const{ return !operator==(p); }
   inline void operator +=(const S_point &p){ x += p.x; y += p.y; }
   inline void operator -=(const S_point &p){ x -= p.x; y -= p.y; }

   S_point operator +(const S_point &p) const{ return S_point(x+p.x, y+p.y); }
   S_point operator -(const S_point &p) const{ return S_point(x-p.x, y-p.y); }
   S_point operator *(int n) const{ return S_point(x*n, y*n); }
};

//----------------------------
// rectangle - specifying top-left corner and size of rectangle
struct S_rect{
   int x, y, sx, sy;
   S_rect(){}
   S_rect(int _x, int _y, int _sx, int _sy):
      x(_x), y(_y), sx(_sx), sy(_sy)
   {}
   S_rect(const S_point &lt, const S_point &sz):
      x(lt.x), y(lt.y), sx(sz.x), sy(sz.y)
   {}
//----------------------------
   inline S_point &TopLeft() const{ return (S_point&)x; }
   inline S_point &Size() const{ return (S_point&)sx; }

//----------------------------
// Get center of rectangle.
   inline int CenterX() const{ return x + sx/2; }
   inline int CenterY() const{ return y + sy/2; }
   inline S_point Center() const{ return S_point(CenterX(), CenterY()); }
//----------------------------
// Get right/bottom point of rectangle (point is 1 pixel out of rectangle area).
   inline int Right() const{ return x + sx; }
   inline int Bottom() const{ return y + sy; }

//----------------------------
// Expand/compact rectangle by given pixels on all 4 sides.
   inline void Expand(int n = 1){
      x -= n; y -= n; sx += 2*n; sy += 2*n;
   }
   inline void Compact(int n = 1){ Expand(-n); }

//----------------------------
// Check if rectange is empty (width or height is zero or less).
   inline bool IsEmpty() const{ return (sx<=0 || sy<=0); }

//----------------------------
// Set this rectangle to intersection of 2 rectangles. Resulting rect size may be also negative.
   void SetIntersection(const S_rect &rc1, const S_rect &rc2);

//----------------------------
// Set this rectangle to union of 2 rectangles.
   void SetUnion(const S_rect &rc1, const S_rect &rc2);

//----------------------------
// Center rectangle to provided rectangle size.
   void PlaceToCenter(int size_x, int size_y);

//----------------------------
// Get copy of this rectangle expanded horizontally or vertically. By using negative values, result may be shrank.
   S_rect GetExpanded(int x, int y) const;
   inline S_rect GetExpanded(int n) const{ return GetExpanded(n, n); }
   S_rect GetHorizontallyExpanded(int n) const;
   S_rect GetVerticallyExpanded(int n) const;

//----------------------------
// Get copy of this rectangle moved.
   S_rect GetMoved(int move_x, int move_y) const;

//----------------------------
// Check if point is inside of rectangle.
   bool IsPointInRect(const S_point &p) const;

//----------------------------
// Check if two rects overlap.
   bool IsRectOverlapped(const S_rect &rc) const;
};

//----------------------------

                              //font definition
struct S_font{
   int cell_size_x, cell_size_y;
   int space_width;
   int letter_size_x;         //average letter width
   int line_spacing;
   int extra_space;           //additional space between letters
   int bold_width_add;        //pixels added to width of bold letter
   int bold_width_subpix;     //sub-pixels added for creating bold letters
   int bold_width_pix;        //whole pixels   ''                          - must be (bold_width_subpix+2)/3

#ifndef USE_SYSTEM_FONT
   enum E_PUNKTATION{
      PK_NO,
      PK_HOOK,                   // v
      PK_LINE,                   // /
      PK_2DOT,                   // ..
      PK_HOOK_I,                 // ^
      PK_CIRCLE,                 // o
      PK_2LINES,                 // ,,
      PK_LINE_I,                 // '\'
      PK_TILDE,                  // ~
      PK_DOT,                    // .
      PK_LOW_HOOK,               //, (at bottom)
      PK_HLINE,                  //_
      PK_SIDE_HOOK,              //l'
      PK_SIDE_LINE,              //l/
      PK_LAST
   };
   signed char punkt_center[PK_LAST];  //center pixel offset of punktation characters
#endif//!USE_SYSTEM_FONT
};

//----------------------------
                              //font drawing flags
static const int
   FF_CENTER = 1,      //center text
   FF_RIGHT = 2,       //adjust text to right
   FF_BOLD = 4,
   FF_ITALIC = 8,
   FF_UNDERLINE = 0x10,
   FF_ACTIVE_HYPERLINK = 0x20,   //internal

                              //shadow alpha bits: 16-23
   FF_SHADOW_ALPHA_SHIFT = 16,
   FF_SHADOW_ALPHA_MASK = 0x00ff0000,
                              //rotation
   FF_ROTATE_CCW = 0x40000000,
   FF_ROTATE_180 = 0x80000000,
   FF_ROTATE_CW = 0xc0000000,
   FF_ROTATE_MASK = 0xc0000000,
   FF_ROTATE_SHIFT = 30;

//----------------------------

enum E_TEXT_CODING{
   COD_DEFAULT,
   COD_WESTERN,               //8859-1
   COD_CENTRAL_EUROPE,        //8859-2
   COD_CENTRAL_EUROPE_WINDOWS,//Windows-1250
   COD_CYRILLIC_WIN1251,      //Windows-1251
   COD_TURKISH,               //8859-9 (Latin-5)
   COD_GREEK,                 //8859-7
   COD_UTF_8,                 //utf-8
   COD_KOI8_R,                //koi8-r
   COD_CYRILLIC_ISO,          //8859-5
   COD_8859_13,               //8859-13 (Baltic)
   COD_BALTIC_ISO,            //8859-4 baltic iso (Latin-4)
   COD_BALTIC_WIN_1257,       //Windows-1257
   COD_LATIN9,                //8859-15 (Western European Latin 9)

                              //8859-3 = South European (Latin-3)
                              //8859-6 = Latin/Arabic
                              //8859-8 = Latin/Hebrew
                              //8859-10 = Nordic (Latin-6)
                              //8859-14 = Celtic (Latin-8)
                              //8859-16 = South-Eastern European (Latin-10)
   COD_LAST,

   COD_BIG5 = COD_LAST,
   COD_GB2312,
   COD_GBK,
   COD_SHIFT_JIS,
   COD_JIS,                   //iso-2022-jp
   COD_EUC_KR,                //euc-kr
   COD_HEBREW,                //windows-1255
   COD_KOI8_U,                //koi8-u
};

//----------------------------
                              //passed to functions where user input is processed
struct S_user_input{
   dword key_bits;            //combination of GKEY_* bits
   dword key;                 //IG_KEY or character
   bool key_is_autorepeat;    //set if key is produced as auto-repeat

   dword mouse_buttons;       //combination of MOUSE_BUTTON_* events (may have cumulated more of these)
   S_point mouse;             //mouse screen position
   S_point mouse_2nd_touch;   //used with MOUSE_BUTTON_1_DRAG_MULTITOUCH, this is position of 2nd touch
   bool is_doubleclick;       //valid for mouse DOWN event
   S_point mouse_rel;         //relative mouse movement since last call; valid for mouse DRAG event

//----------------------------
// Check if mouse position is located in given rectangle.
   inline bool CheckMouseInRect(const S_rect &rc) const{
      return rc.IsPointInRect(mouse);
   }

   void Clear(){
      key_bits = 0;
      key = K_NOKEY;
      mouse_buttons = 0;
      mouse.x = mouse.y = 0;
      is_doubleclick = false;
   }
};

//----------------------------

class C_application_base{
   class C_app_observer *app_obs;
   bool embedded;
   bool has_mouse, has_kb, has_full_kb;
   bool is_focused;
   S_pixelformat pixel_format;//pixel format of graphics buffer
   int _scrn_sx, _scrn_sy;      //screen resolution
   dword ig_flags;

   S_rect rc_screen_dirty, clip_rect;
   int last_click_time;       //for detecting doubleclicks
   S_point last_mouse_pos;    //for DOWN and DRAG mouse events
   C_smart_ptr<C_unknown> dyn_cod_draw_zoomed;

   friend class IGraph_imp;
   friend class C_app_observer;

   void ProcessInputInternal(IG_KEY key, bool auto_repeat, dword key_bits, const S_point &touch_pos, dword pen_state, const S_point *secondary_pos = NULL);
protected:
   virtual void InitInputCaps();
   IGraph *igraph;

//----------------------------
   C_application_base();

public:
#ifdef UI_MENU_ON_RIGHT
   bool menu_on_right;        //display menu on right side
#endif
   bool use_system_ui;        //use UI of system instead of own

//----------------------------
// Return true if app was launched in embedded mode = needs to open document.
   inline bool IsEmbedded() const{ return embedded; }

//----------------------------
// Exit request. Application calls this when it wants to close itself.
// Note: this call may never return on some system, while it returns normally on others.
   void Exit();

//----------------------------
// Get current graphics flags used to initialize graphics (IG_*). These may be modified by UpdateGraphicsFlags.
   inline dword GetGraphicsFlags() const{ return ig_flags; }

//----------------------------
// Reinit graphics with new flags. This call may result in screen resizing.
// Currently these flags may be specified: IG_SCREEN_USE_CLIENT_RECT, IG_SCREEN_USE_DSA, IG_ENABLE_BACKLIGHT
   inline void UpdateGraphicsFlags(dword new_flags, dword flags_mask){ igraph->_UpdateFlags(new_flags, flags_mask); }

//----------------------------
// Timer class - handle to timer that is called periodically on this class' function TimerTick.
// Multiple timers may run at same time.
// Timers and other callbacks (e.g. to process input) are synchronized, no other call happens by system until previous call returns.
   class C_timer{
   public:
      virtual ~C_timer(){}
      virtual void Pause() = 0;  //pause timer
      virtual void Resume() = 0; //resume timer (with same frequency used when creating); if timer is not paused, it is not restarted
   };

//----------------------------
// Create new timer. Timer will call TimerTick periodically. To remove timer, delete returned object.
   C_timer *CreateTimer(dword ms, void *context);

//----------------------------
// Create new alarm. Alarm will call AlarmNotify at given time. To remove alarm, delete returned object. 'date_time' is in seconds in format of S_date_time.
   C_timer *CreateAlarm(dword date_time, void *context);

protected:
//----------------------------
// Periodic timer tick, called for timers. 'time' is time from last call of this callback, in milliseconds.
   virtual void TimerTick(C_timer *t, void *context, dword time) = 0;

//----------------------------
// Alarm notify, called for alarms.
   virtual void AlarmNotify(C_timer *a, void *context){}

//----------------------------
// Handle change in focus. Application may override this.
   virtual void FocusChange(bool foreground){
      is_focused = foreground;
   }

//----------------------------
// Construction - initialize application.
   virtual void Construct() = 0;

//----------------------------
// Open file from command-line (embedded mode). This is called after application is initialized.
   virtual void OpenDocument(const wchar *fname){}

//----------------------------
// Processing of user input (keys, pen).
   virtual void ProcessInput(S_user_input &ui) = 0;

//----------------------------
// Called when screen was resized. igraph is reinitialized before this is called.
   virtual void ScreenReinit(bool resized);

//----------------------------
// Called from ScreenReinit to reinit application after screen size changed.
   virtual void InitAfterScreenResize();
public:

//----------------------------
// Redraw currently active mode.
   virtual void RedrawScreen() = 0;

//----------------------------
// Check if application has currently focus (is active application).
   inline bool IsFocused() const{ return is_focused; }

//----------------------------
// Get name of application's data file.
   virtual const wchar *GetDataFileName() const = 0;
   virtual const C_smart_ptr<C_zip_package> CreateDtaFile() const;

//----------------------------
// Tests if device has pen/mouse (touch screen) function.
   inline bool HasMouse() const{ return has_mouse; }

//----------------------------
// Test if device has hardware keyboard (full or keypad).
   inline bool HasKeyboard() const{ return has_kb; }

//----------------------------
// Test if device has full (alphanumeric) hardware keyboard.
   inline bool HasFullKeyboard() const{ return has_full_kb; }

   enum E_TOUCH_FEEDBACK_MODE{
      TOUCH_FEEDBACK_DEFAULT,
      TOUCH_FEEDBACK_CREATE_TOUCH_MENU,
      TOUCH_FEEDBACK_SELECT_MENU_ITEM,
      TOUCH_FEEDBACK_BUTTON_PRESS,
      TOUCH_FEEDBACK_SELECT_LIST_ITEM,
      TOUCH_FEEDBACK_SCROLL,
   };
//----------------------------
// Send touch feedback.
   virtual void MakeTouchFeedback(E_TOUCH_FEEDBACK_MODE mode = TOUCH_FEEDBACK_DEFAULT);

//----------------------------
// Minimize this application (put to background) or restore to foreground.
   void MinMaxApplication(bool min);

//----------------------------
// Toggle window to use client rect or fullscreen.
   virtual void ToggleUseClientRect();

//----------------------------
//----------------------------//Graphics functions

   enum E_COLOR{
      COL_BACKGROUND,
      COL_TITLE,
      COL_AREA,
      COL_SELECTION,
      COL_SCROLLBAR,
      COL_MENU,

      COL_BLACK,
      COL_DARK_GREY,
      COL_GREY,
      COL_LIGHT_GREY,
      COL_WHITE,
      COL_SHADOW,
      COL_HIGHLIGHT,
      COL_TEXT,               //general text on background
      COL_TEXT_POPUP,         //text on dialog window and in menu
      COL_TEXT_MENU,
      COL_TEXT_MENU_SECONDARY,
      COL_TEXT_HIGHLIGHTED,   //text on selection rectangle
      COL_TEXT_SELECTED,      //marked text in text editor
      COL_TEXT_SELECTED_BGND, //background of ''
      COL_TEXT_INPUT,         //text in text editor
      COL_TEXT_SOFTKEY,       //softkey labels
      COL_TEXT_TITLE,         //title (at top of screen)
      COL_EDITING_STATE,      //editing state indicators
      COL_TOUCH_MENU_LINE,
      COL_TOUCH_MENU_FILL,
      COL_WASHOUT,            //overlay color for drawing in DrawWashOut

      COL_LAST
   };
//----------------------------
// Get argb color from predefined color constant.
   virtual dword GetColor(E_COLOR col) const;

//----------------------------
// Get pixel in backbuffer pixel format from source RGB data.
   dword GetPixel(dword rgb) const;

//----------------------------
// Get backbuffer pixel format.
   inline const S_pixelformat &GetPixelFormat() const{ return pixel_format; }

//----------------------------
// Get pointer to top-left corner of backbuffer.
   inline void *GetBackBuffer() const{ return igraph->_GetBackBuffer(); }

//----------------------------
// Get pitch (number of bytes) between 2 lines in backbuffer.
   inline dword GetScreenPitch() const{ return igraph->_GetScreenPitch(); }

//----------------------------
// Get size of screen.
   inline int ScrnSX() const{ return _scrn_sx; }
   inline int ScrnSY() const{ return _scrn_sy; }

//----------------------------
// Get current clipping rectangle.
   inline const S_rect &GetClipRect() const{ return clip_rect; }

//----------------------------
// Set current clipping rectangle.
   virtual void SetClipRect(const S_rect &rc);

//----------------------------
// Set clipping rectangle to cover entire screen area.
   void ResetClipRect();

//----------------------------
// Clear entire backbuffer to specified color.
   void ClearScreen(dword color = 0);

//----------------------------
// Fill rectangle to specified color.
//    color ... ARGB color value; alpha value specify opacity of rectangle (0=transparent, 255=opaque)
// If optimized is true, alpha value will be rounded to nearest value of 0, 0x40, 0x80, 0xc0, 0xff, and optimized faster drawing will be used.
   void FillRect(const S_rect &rc, dword color, bool optimized = true);

//----------------------------
// Draw horizontal and vertical lines.
   void DrawHorizontalLine(int x, int y, int width, dword color);
   void DrawVerticalLine(int x, int y, int height, dword color);

//----------------------------
// Draw outline rectangle one pixel outside of 'rc'.
   void DrawOutline(const S_rect &rc, dword col_left_top, dword col_right_bottom);
   inline void DrawOutline(const S_rect &rc, dword color){ DrawOutline(rc, color, color); }

//----------------------------
// Same as DrawOutline, but line is dashed.
   void DrawDashedOutline(const S_rect &rc, dword col1, dword col2);

//----------------------------
// Draw plus or minus sign in a box.
// Returned value is width of drawn area.
   dword DrawPlusMinus(int x, int y, int size, bool plus);

//----------------------------
// Draw fast and simple shadow (using lines) below rectangle.
   void DrawSimpleShadow(const S_rect &rc, bool expand);

//----------------------------
// Draw simple arrows.
   void DrawArrowHorizontal(int x, int y, int size, dword color, bool right);
   void DrawArrowVertical(int x, int y, int size, dword color, bool down);

//----------------------------
// Draw 2-pixel etched frame around rectangle.
   virtual void DrawEtchedFrame(const S_rect &rc);

//----------------------------
// Reset dirty rectangle.
   void ResetDirtyRect();

//----------------------------
// Add rectangle to screen's dirty rectangles.
   void AddDirtyRect(const S_rect &rc);

//----------------------------
// Add entire screen area to dirty rectangle.
   void SetScreenDirty();

//----------------------------
// Update screen's dirty rectangle, and reset dirty area to nothing.
   void UpdateScreen();

   enum E_ROTATION{
      ROTATION_NONE,
      ROTATION_90CCW,
      ROTATION_180,
      ROTATION_90CW,
   };
//----------------------------
// Get screen's rotation relative to device's default orientation.
   inline E_ROTATION GetScreenRotation() const{ return (E_ROTATION)igraph->_GetRotation(); }

//----------------------------
// Get resolution of entire screen area.
   inline S_point GetFullScreenSize() const{ return igraph->_GetFullScreenSize(); }

//----------------------------
// Get LCD subpixel mode - how RGB triplets are physically oriented. If they're vertical or unknown, LCD_SUBPIXEL_UNKNOWN is returned;
   enum E_LCD_SUBPIXEL_MODE{
      LCD_SUBPIXEL_UNKNOWN,
      LCD_SUBPIXEL_RGB,
      LCD_SUBPIXEL_BGR,
      LCD_SUBPIXEL_LAST
   };
   E_LCD_SUBPIXEL_MODE DetermineSubpixelMode() const;

//----------------------------
public:
#ifndef USE_SYSTEM_FONT
   static const int NUM_FONTS = 2;  //number of fonts available at one time

   struct S_internal_font{
      static const int NUM_INTERNAL_FONTS = 6;  //number of all available internal fonts
      static const int NUM_FONT_LETTETS = 16*14;

      static const S_font all_font_defs[NUM_INTERNAL_FONTS];

   //----------------------------
      struct S_font_data{
                                 //pixel data for fonts: normal, bold, italic, bold italic
                                 // data are pixels in 16-bit RGB format (word for each pixel)
                                 // data are compressed (empty letter lines are omited), resulting in about 50% compression

                                 //array of real widths of all letters; followed by 1 'empty' bit for each letter's line; 1 byte for 8 neighbour letters
                                 // real witdh specifies width of letter, in pixels (if highest bit is set, then font spacing is 1 pixel less than its width)
                                 // empty bits - if set for particular line, then the line is totally empty, thus not stored in compressed font data
         C_buffer<byte> width_empty_bits;
         C_buffer<word> compr_data[4]; //compressed font data (first is NUM_FONT_LETTETS*word table of offset to letter data)
         dword italic_add_sx;
      } font_data[NUM_FONTS];

      struct S_render_letter{
         const word *font_src;
         const byte *width_empty_bits;
         dword text_color;          //color of text
         dword bgnd_color;          //color of background
         int letter_size_x, letter_size_y, letter_bold_width;
         dword font_flags;
         dword rotation, physical_rotation;  //0-3
         const S_internal_font::S_font_data *font_data;
      };

      E_LCD_SUBPIXEL_MODE curr_subpixel_mode;
      int curr_def_font_size;   //used for reinitialization of fonts if font size may change during screen switch
      int curr_relative_size;
      dword screen_rotation;

      S_internal_font():
         curr_subpixel_mode(LCD_SUBPIXEL_LAST),
         curr_def_font_size(-1),
         curr_relative_size(0)
      {}

   //----------------------------
   // Convert unicode char to displayable char plus possible punktation.
      static dword GetCharMapping(dword c, S_font::E_PUNKTATION &punkt);

      void Close();
   };
   S_internal_font internal_font;

//----------------------------
private:
   void InitInternalFonts(const C_zip_package *dta, int size_delta);

//----------------------------
// Clip and render letter into screen.
//    scrn_dst ... top-left address of screen window
//    letter_index ... index of rendered letter (acii starting from ' ')
//    x, y ... position of letter relative to screen window
   void RenderLetter(const S_internal_font::S_render_letter &rl, byte *scrn_dst, dword letter_index, int x, int y) const;

protected:
//----------------------------
// Get application's preference about font settings.
   virtual void GetFontPreference(int &relative_size, bool &use_system_font, bool &antialias) const{  //!! old func, replaced by new
      relative_size = 0;
      use_system_font = true;
      antialias = true;
   }
   virtual void GetFontPreference(int &relative_size, bool &use_system_font, bool &antialias, int &small_font_size_percent) const{
      GetFontPreference(relative_size, use_system_font, antialias);
      small_font_size_percent = 82;
   }

#endif//!USE_SYSTEM_FONT
//----------------------------
#if defined _WINDOWS || defined _WIN32_WCE
   friend class C_text_editor_win32;
#endif
#ifdef __SYMBIAN32__
   friend class C_text_editor_Symbian;
#endif

   struct S_system_font{
      IGraph *igraph;

      static const dword NUM_CACHE_WIDTH_CHARS = 128-32;
      byte *font_width;       //cached font char widths
      S_font font_defs[2];

      bool use, use_antialias;

#ifdef __SYMBIAN32__
      mutable void *font_handle[2][2][2];   //[bold][italic][size]
#elif defined BADA
      mutable void *font_handle[2][2][2];   //[bold][italic][size]
#elif defined ANDROID
      mutable void *font_handle[1][1][1];
#else
      mutable void *font_handle[4][2][2][2];   //[rotation][bold][italic][size]
#endif
      mutable void *last_used_font;
      void SelectFont(void *h) const;

      mutable dword last_text_color;
      mutable byte font_baseline[2][2];
#ifdef __SYMBIAN32__
      class CFbsTypefaceStore *ts;
      mutable class CFbsBitGc *gc;
      void InitGc() const;
      int big_size, small_size;  //font size in pixels
#elif defined BADA
      int big_size, small_size;  //font size in pixels
      S_point bbuf_offs;
#elif defined ANDROID
#else
      mutable void *hdc;
      void InitHdc() const;
      int size_big, size_small;
      S_rect clip_rect;       //used as {l, t, r, b}
#endif
      C_buffer<byte> transp_tmp_buf;
      void SelectFont(bool bold, bool italic, bool big) const{
         void *h = font_handle[bold][italic][big];
         if(last_used_font!=h)
            SelectFont(h);
      }

      S_system_font();
      ~S_system_font(){
         Close();
      }
      void Close();
      void InitFont(dword rotation, bool bold, bool italic, bool big, int height) const;
      dword GetCharWidth(wchar c, bool bold, bool italic, bool big) const;
      void SetClipRect(const S_rect &rc);
   } system_font;
private:
   void InitSystemFont(int size_delta, bool use_antialias, int small_font_size_percent);
   void RenderSystemChar(int x, int y, wchar c, dword curr_font, dword font_flags, dword text_color);

public:
//----------------------------
// Get default size of font, either for system or for internal font.
   int GetDefaultFontSize(bool use_system_font) const;

//----------------------------

   const S_font *font_defs;
   S_font fds, fdb;           //font definitions for small and big texts

protected:
//----------------------------
// Draw one character 'c',
   void DrawLetter(int x, int y, wchar c, dword curr_font, dword font_flags, dword color);
public:
   static const int UI_FONT_SMALL = 0, UI_FONT_BIG = 1;
   inline const S_font &GetFontDef(dword i) const{ assert(int(i)<NUM_FONTS); return font_defs[i]; }
   inline IGraph *GetIGraph(){ return igraph; }

//----------------------------
// Get width of single character.
   int GetCharWidth(wchar c, bool bold, bool italic, dword font_index) const;

#if defined __SYMBIAN32__ && defined __SYMBIAN_3RD__
// Red key pressed, which usually makes app close on S60 3rd ed. By default we ignore it.
   virtual bool RedKeyWantClose() const{ return false; }
#endif
protected:
//----------------------------
// Function used by startup code. Do not call directly.
   void InitInternal(C_app_observer *obs, bool emb){
      app_obs = obs;
      embedded = emb;
      InitInputCaps();
   }

//----------------------------
// Initialize application's graphics.
   void InitGraphics(dword ig_flags);

public:
   virtual ~C_application_base(){
      delete igraph;
   }
};

//----------------------------
// Global function creating the specialized application.
C_application_base *CreateApplication();

//----------------------------

#endif

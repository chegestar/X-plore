//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#ifndef __USER_INTERFACE_H_
#define __USER_INTERFACE_H_

#include <Rules.h>
#include <C_vector.h>
#include <AppBase.h>
#include <Socket.h>

#if defined __SYMBIAN_3RD__ || defined _WINDOWS
#define UI_ALLOW_ROTATED_SOFTKEYS   //allow softkeys to be rotated with screen
#endif

//----------------------------

#if !(defined S60 && defined __SYMBIAN32__ && !defined __SYMBIAN_3RD__)
#define USE_MOUSE             //includes mouse-based code
#endif

//----------------------------
                              //ASCII codes for LCG rich text
enum E_TEXT_CONTROL_CODE{
   CC_TEXT_FONT_FLAGS = 0x10,   //combination of font flags FF_* (byte)
   CC_TEXT_FONT_SIZE,         //font size; 0=small, 1=normal (byte)
   CC_TEXT_COLOR,             //rgb triplet (3 bytes)
   CC_BGND_COLOR,             //       ''
   CC_BLOCK_QUOTE_COUNT,      //number of block quotes (byte)

   CC_HYPERLINK,              //hyperlink index into hyperlinks array (word)
   CC_HYPERLINK_END,          // ''
   CC_TEXT_LAST,

   CC_WIDE_CHAR = CC_TEXT_LAST,  //unicode character (wchar)
   CC_IMAGE,                  //image index into images array (word)
   CC_FORM_INPUT,             //index into form_inputs (word)
   CC_HORIZONTAL_LINE,        //horizontal line (dword color, byte relative_width)
   CC_CUSTOM_ITEM,            //index into custom_items (word)

   CC_LAST,
};

//----------------------------
                              //text style, representing parameters of rendered text
struct S_text_style{
public:
   int font_flags;            //combination of FF_* flags (bold, italic, centering, etc)
   int font_index;            //-1 = default, 0 = small, 1 = normal
   dword text_color;
   dword bgnd_color;          //0 = default (not set)
   bool href;                 //active hyperlink marker
   byte block_quote_count;

   S_text_style(){
      SetDefault();
   }
   S_text_style(int _font_index, int _font_flags, dword _text_color):
      font_flags(_font_flags),
      font_index(_font_index),
      text_color(_text_color),
      bgnd_color(0),
      href(false),
      block_quote_count(0)
   {}

//----------------------------
// Write control code with parameters into text buffer.
   static void WriteCode(E_TEXT_CONTROL_CODE code, const void *old, const void *nw, dword sz, C_vector<char> &buf);

//----------------------------
// Check if given character is style code or control code.
   static bool IsStyleCode(dword c){ return (c >= CC_TEXT_FONT_FLAGS && c < CC_TEXT_LAST); }
   static bool IsControlCode(dword c){ return (c >= CC_TEXT_FONT_FLAGS && c < CC_LAST); }

//----------------------------
// Set default style values.
   void SetDefault();

//----------------------------
// Used for building text; update this style with values of ts, and write difference to char buffer.
   void WriteAndUpdate(const S_text_style &ts, C_vector<char> &buf);

//----------------------------
// Add character to buffer. If it is greater than 256, then CC_WIDE_CHAR is used, otherwise it's written as-is.
   static void AddWideCharToBuf(C_vector<char> &buf, wchar c);

//----------------------------
// Add string to char buffer, with given font and color.
   void AddWideStringToBuf(C_vector<char> &buf, const wchar *str, int font_flags = -1, int color = 0);

//----------------------------
   static void AddHorizontalLine(C_vector<char> &buf, dword color, byte width_percent);

//----------------------------
// Read packed formatting, and advance pointer.
// Return # of bytes read from stream.
   int ReadCode(const char *&cp, const int *active_link_index = NULL);

//----------------------------
// Same as ReadCode, but parsing char buffer backwards (used for fast text scrolling).
   int ReadCodeBack(const char *&cp, const int *active_link_index = NULL);

//----------------------------
// Skip control code.
   static void SkipCode(const char *&cp);

//----------------------------
// Read CC_WIDE_CHAR code.
   static word ReadWideChar(const char *&cp);

//----------------------------
// Read index, used for control codes with word index value.
   static dword Read16bitIndex(const char *&cp);

//----------------------------
// Read info about horizontal line.
// rc_width = width of rendered rectangle
// Returns width of line in pixels
   static dword ReadHorizontalLine(const char *&cp, dword rc_width, dword &color);
};

//----------------------------
                              //keyboard accelerator - simple class for computing accelerating value when key is held down
struct S_key_accelerator{
   int speed;
   int last_frac, acc_frac;
   S_key_accelerator():
      speed(0),
      last_frac(0),
      acc_frac(0)
   {}
   int GetValue(bool pressed, int time, int min_speed, int max_speed, int accel);
};

//----------------------------
                              //scrollbar (vertical or horizontal)
class C_scrollbar{
public:
   bool visible;              //if visible, then it is drawn
   S_rect rc;                 //rectangle where it is drawn; if widht is greater than height then it is horizontal scrollbar

                              //units here are undefined (may be pixels, lines, bytes or anything needed)
   int total_space;           //total amount of units
   int visible_space;         //units visible in given rectangle
   int pos;                   //position from beginning

   int mouse_drag_pos;        //mouse drag init position, or -1 if was not dragging
   int drag_init_pos;         //drag init scroll position
   bool button_down;
   dword last_button_down_time;
   int repeat_countdown;      //counter for repeated page scrolls while mouse is held down
   enum{
      DRAW_PAGE_NO,
      DRAW_PAGE_UP,
      DRAW_PAGE_DOWN
   } draw_page;

   C_scrollbar():
      visible(false),
      mouse_drag_pos(-1),
      drag_init_pos(0),
      button_down(false),
      draw_page(DRAW_PAGE_NO),
      pos(0),
      total_space(0),
      visible_space(0)
   {}

   inline void SetVisibleFlag(){
      visible = (total_space > visible_space);
   }
   enum E_PROCESS_MOUSE{
      PM_NO,                  //mouse not processed
      PM_PROCESSED,           //mouse processed
      PM_CHANGED,             //    '' and position changed
   };

//----------------------------
// Compute size of thumb rectangle. Return true if position is vertical.
   bool MakeThumbRect(const C_application_base &app, S_rect &rc) const;
};

//----------------------------
                              //progress indicator
class C_progress_indicator{
public:
   S_rect rc;                 //rectangle where it is drawn

                              //units here are undefined (may be pixels, lines, bytes or anything needed)
   int total;                 //total size
   int pos;                   //current position
   C_progress_indicator():
      total(100),
      pos(0)
   {}
};

//----------------------------

class C_button{
   S_rect rc;                 //rectangle in which this button is drawn
   S_rect rc_click;
public:
   bool clicked, down, hint_on;
   int down_count;            //counter when button was clicked (for drawing hint)
   C_button():
      clicked(false),
      down(false), hint_on(false),
      down_count(0)
   {}

//----------------------------
   void SetRect(const S_rect &_rc, int click_expand_pixels = 0){
      rc = _rc;
      rc_click = rc;
      rc_click.Expand(click_expand_pixels);
   }
   const S_rect &GetRect() const{ return rc; }
   const S_rect &GetClickRect() const{ return rc_click; }

//----------------------------
// Process button input. Return true if button was clicked.
   bool ProcessMouse(const S_user_input &ui, bool &redraw, C_application_base *app_for_touch_feedback = NULL);
};

//----------------------------
namespace encoding{

                                 //single-byte string, coded by some non-unicode coding
class C_str_cod: public Cstr_c{
public:
   E_TEXT_CODING coding;
   C_str_cod():
      coding(COD_DEFAULT)
   {}
   C_str_cod(const Cstr_c &s):
      Cstr<char>(s),
      coding(COD_DEFAULT)
   {}
   C_str_cod(const char *cp):
      Cstr<char>(cp),
      coding(COD_DEFAULT)
   {}
};

//----------------------------
// Convert character from specified coding to unicode character.
dword ConvertCodedCharToUnicode(dword c, E_TEXT_CODING coding);

//----------------------------
// Convert charset name to encoding type.
void CharsetToCoding(const Cstr_c &charset, E_TEXT_CODING &cod);

}//encoding
//----------------------------
                              //hyperling, text link with index to image
struct S_hyperlink
#ifdef WEB_FORMS
   : public S_js_events
#endif
{
   Cstr_c link;               //hyperlink (http:, mailto:, #, etc
   int image_index;           //link to image (-1 = no); used for opening image in fullscreen
   S_hyperlink():
      image_index(-1)
   {}
};

//----------------------------
                              //information about image stored in rich text
struct S_image_record{
                                 //align mode - vertical and horizontal
   enum{
      ALIGN_H_DEFAULT = 0,
      ALIGN_H_LEFT = 1,
      ALIGN_H_RIGHT = 2,
      ALIGN_H_CENTER = 3,
      ALIGN_H_MASK = 3,
      ALIGN_V_DEFAULT = 0x00,
      ALIGN_V_TOP = 0x10,
      ALIGN_V_CENTER = 0x20,
      ALIGN_V_BOTTOM = 0x30,
      ALIGN_V_MASK = 0x30,
   };
   typedef word t_align_mode;

   C_smart_ptr<C_image> img;
   int sx, sy;                //image resolution (real if image is loaded, proposed (from img tag) if not available)
   bool invalid;
   bool size_set;             //size set explicitly by html tag
   bool draw_border;
   Cstr_c src;
   Cstr_w alt;
   Cstr_w filename;
   t_align_mode align_mode;

   S_image_record():
      invalid(false),
      size_set(false),
      draw_border(false),
      align_mode(0)
   {}
};

//----------------------------
                              //struct holding info about displayed rich text in a rectangle
struct S_text_display_info{
   S_rect rc;
   bool is_wide;              //true if text is wide version (body_w), false if coded version (body_c)
   bool justify;
   encoding::C_str_cod body_c;
   Cstr_w body_w;
   dword byte_offs;           //1st byte from body which is displayed
   S_text_style ts;           //style of text at the beginning of displayed line
   int pixel_offs;            //how many pixels is top line drawn above top border of rectangle

   int num_lines;             //number of lines in text
   int top_pixel;             //top pixel position from beginning of text
   int total_height;          //total height of entire text, in pixels

                              //text marking, works only in non-html text
   int mark_start, mark_end;  //byte offset of specially marked text; if mark_end==0 then no marking is drawn

   Cstr_w html_title;         //title of hypertext document
   dword bgnd_color;          //background color; -1 = use default
   int bgnd_image;            //index into background image; -1 = use default

   dword link_color, alink_color;   //hyperlink color, active link color

                              //hyperlink info:
   C_vector<S_hyperlink> hyperlinks;

   void BeginHyperlink(C_vector<char> &dst_buf, const S_hyperlink &hr);
   void EndHyperlink(C_vector<char> &dst_buf);

                              //list of images that were rendered last time (kept due to easy update of image animations)
   mutable C_vector<const class C_image*> last_rendered_images;

//----------------------------

   inline dword Length() const{
      return is_wide ? body_w.Length() : body_c.Length();
   }

//----------------------------
// Return active (focused) hyperlink, or NULL if no active.
   const S_hyperlink *GetActiveHyperlink() const;

//----------------------------

   int active_link_index;     //index of active hyperlink (-1 = no)
   int active_object_offs;    //byte offset of active link in body (-1 = no)

                           //all images embedded in text
   C_vector<S_image_record> images;

#ifdef WEB_FORMS
                              //forms
   C_vector<S_html_form> forms;
   C_vector<S_html_form_select_option> form_select_options;
   C_vector<S_html_form_input> form_inputs;

   S_html_form_input *GetActiveFormInput();

   C_smart_ptr<C_text_editor> te_input;   //text editor for active form
                              //html auto-refresh
   int refresh_delay;
   Cstr_c refresh_link;
#ifdef WEB
   struct S_javascript{
      S_dom_element dom;
      Cstr_c on_body_unload;

      void Clear();
   } js;
#endif//WEB
#endif//WEB_FORMS

                              //bookmarks
   struct S_bookmark{
      Cstr_c name;
      dword byte_offset;
   };
   C_vector<S_bookmark> bookmarks;   //local bookmarks

   typedef void (C_application_base::*t_DrawProc)(const S_rect &rc, const Cstr_c &custom_data);
   struct S_custom_item{
      word sx, sy;
      Cstr_c custom_data;
      t_DrawProc DrawProc;
   };
   C_vector<S_custom_item> custom_items;

   S_text_display_info(){
      Clear();
   }
   void Clear();
   inline bool HasDefaultBgndColor() const{ return (bgnd_color==0xffffffff); }
};

//----------------------------
                              //menu containing option items
class C_menu: public C_unknown{
protected:
   C_menu(){}
public:
   enum{                      //menu item flags
      DISABLED = 1,
      HAS_SUBMENU = 2,
      MARKED = 4,
   };
   int selected_item;

   struct S_item{
      Cstr_w text;            //if not NULL, it is used instead of txt_id
      word txt_id;
      char but;               //bottom button index (-1 = no)
      Cstr_c shortcut, shortcut_full_kb;
      dword flags;

      inline bool IsSeparator() const{ return (!text.Length() && txt_id==0); }
   };

//----------------------------
// Add menu item displaying text from txt_id. ProcessMenu will receive txt_id if item is selected.
   virtual void AddItem(dword txt_id, dword flags = 0, const char *shortcut = NULL, const char *shortcut_full_kb = NULL, char img_index = char(-1)) = 0;

//----------------------------
// Add menu item displaying string. ProcessMenu will receive 0x10000 | item index (from top of menu) if item is selected.
   virtual void AddItem(const Cstr_w &txt, dword flags = 0, const char *right_txt = NULL) = 0;

//----------------------------
// Add separator line item.
   inline void AddSeparator(){ AddItem(0); }

//----------------------------
// Get menu ID. Used for special cases when menu need to be identified.
   virtual dword GetMenuId() const = 0;

   virtual const C_menu *GetParentMenu() const = 0;

   virtual const S_rect &GetRect() const = 0;

//----------------------------
// Request notifications of selection change. If set, then in case of selection change, C_mode::ProcessMenu is called with itm==-2.
   virtual void RequestSelectionChangeNotify() = 0;

   virtual void SetImage(C_image *img) = 0;

   virtual bool IsAnimating() const = 0;
};

//----------------------------

class C_application_ui;

//----------------------------
                              //simple class used for calculating kinetic animated movement

class C_kinetic_movement{
   S_point dir;               //move direction
   S_point acc_dir;           //accumulated direction from last movement
   dword acc_beg_time;        //time when we stored acc_dir
   dword detect_time;         //timer used to detect movement
   dword anim_timer;          //timer used for animation
   dword total_anim_time;     //time how long animation will run since beginning to end
   C_fixed r_total_time;      //1.0/total_anim_time
   S_point start_pos;         //starting animation position
   S_point curr_pos;          //current animated position
protected:
//----------------------------
// Called when active animation stops. It gives change to inherited classes to stop animation timer or otherwise react.
   virtual void AnimationStopped(){}
public:
   C_kinetic_movement(): anim_timer(0){ Reset(); }

//----------------------------
// Reset to non-animating, non-detecting state.
   void Reset();

//----------------------------
// Process input to calculate kinetic movement.
// rc_active is area where clicking is able to start animated movement.
// Returns true when user releases touch and kinetic movement animation starts.
   bool ProcessInput(const S_user_input &ui, const S_rect &rc_active, int axis_x = 1, int axis_y = 1);

//----------------------------
// After ProcessInput returns true, caller can set starting position for animation. By default, ProcessInput sets it to current mouse position.
   void SetAnimStartPos(int x, int y){ start_pos.x = x; start_pos.y = y; curr_pos = start_pos; }

//----------------------------
// Tick movement. Returns true when movement is finished.
// If rel_movement is not NULL, it will be set to relative movement since last call to Tick.
   bool Tick(dword time, S_point *rel_movement = NULL);

//----------------------------
// During animation, this is current animated position.
   const S_point &GetCurrPos() const{ return curr_pos; }

//----------------------------
// Check if we're currently animating.
   bool IsAnimating() const{ return (anim_timer!=0); }
};

//----------------------------
                              //base class for modes which provide list of items, possibly scrollable
class C_list_mode_base{
   int touch_move_mouse_detect;
   bool dragging;
protected:
//----------------------------
// Inherited class must provide ref to application, for class's funcs to work.
   virtual C_application_ui &AppForListMode() const = 0;

//----------------------------
// Return number of items to move selection when skipping page up or down. Default function computes this from number of visible items.
   virtual dword GetPageSize(bool up);
public:
   int &top_line;             //top line displayed (mapped to scrollbar position); valid only in non-pixel mode
   int selection;             //current selection index from first entry
   C_scrollbar sb;

   S_rect rc;

   int entry_height;          //height of single entry, in pixels

   int touch_down_selection;  //helper for selection by touch; keeps selection item activated when pen went down; for case when item is opened by pen up

   C_application_base::C_timer *scroll_timer;   //timer for kinetic scrolling; fully managed by C_application_ui

   C_kinetic_movement kinetic_scroll;

   void DeleteScrollTimer(){
      delete scroll_timer;
      scroll_timer = NULL;
   }
   void StopKineticScroll(){
      DeleteScrollTimer();
      kinetic_scroll.Reset();
   }

   void ResetTouchInput(){
      dragging = false;
   }

//----------------------------
// Pixel mode means that vertical scrolling happens in pixels (suitable for kinetic scrolling).
   virtual bool IsPixelMode() const{ return false; }

// Get number of entries in the list. In non-pixel mode, this is equal to scrollbar's total space, so default function returns this.
   virtual int GetNumEntries() const{ return sb.total_space; }
//----------------------------
// Make sure that list is scrolled so that selected line is visible. Return true if top scroll position changed.
   virtual bool EnsureVisible();

//----------------------------
// Check if kinetic scroll animation is currently active.
   inline bool IsKineticScrollAnimation() const{ return (scroll_timer!=NULL); }

//----------------------------
// After scrolling by other mechanism (e.g. manipulating scrollbar), make sure selection is positioned on screen.
   void MakeSureSelectionIsOnScreen();

//----------------------------
// Return right-most x pixel, normally it's right pos of rect, if scrollbar is visible then it's at left side of scrollbar.
   inline int GetMaxX() const{
      return sb.visible ? sb.rc.x-1 : rc.Right();
   }

   C_list_mode_base():
      top_line(sb.pos),
      selection(0),
      entry_height(0),
      scroll_timer(NULL),
      dragging(false),
      touch_move_mouse_detect(-1),
      touch_down_selection(-1)
   {}
   ~C_list_mode_base(){
      StopKineticScroll();
   }

//----------------------------
// Called from ProcessInputInList when selection is changed to different item.
   virtual void SelectionChanged(int prev_selection){}

//----------------------------
// Called from ProcessInputInList when scroll position is changed as result of using scroll bar, or dragging and moving content, or in kinetic scrolling animation.
   virtual void ScrollChanged(){}

//----------------------------
// Draw contents of the list. It's also called from ProcessInputInList if list contents should be redrawn.
   virtual void DrawContents() const{
      assert(0);  //!!
   }

//----------------------------
// Process input in mode using item list. This is moving selection (cursor up/down, mouse down).
// Special keys used: */# Q/A Ctrl+Up/down for moving selection by page.
// Returns true if selection changed.
   bool ProcessInputInList(S_user_input &ui, bool &redraw);

//----------------------------
// Begin drawing next item.
// Before first call, item_index must be set to -1 for properly starting of drawing.
// Returns true if next item should be drawn (it's visible, and item_index is in range).
// If it returns false, rc_item is not moved to new position.
   bool BeginDrawNextItem(S_rect &rc_item, int &item_index) const;

//----------------------------
// After all item list items were drawn, call this.
   void EndDrawItems() const;
};

//----------------------------
                              //text editor, class for using text entry
class C_text_editor: public C_unknown{
protected:
   S_rect rc;
public:
   enum{
      CASE_UPPER = 1,
      CASE_LOWER = 2,
      CASE_CAPITAL = 4,
      CASE_ALL = 7,
   };
   class C_text_editor_notify{
   public:
      virtual void CursorMoved(){}
      virtual void CursorBlinked(){}
      virtual void TextChanged() = 0;
   };

//----------------------------
// Get initialization flags.
   virtual dword GetInitFlags() const = 0;

   int font_index;
   dword font_flags;

//----------------------------
                              //used for multi-line editor:
   //int top_line;
   int scroll_y;

//----------------------------
   inline const S_rect &GetRect() const{ return rc; }
   virtual void SetRect(const S_rect &_rc){ rc = _rc; }
//----------------------------

   virtual void SetInitText(const Cstr_w &init_text) = 0;
   virtual void SetInitText(const char *init_text) = 0;

//----------------------------

   C_text_editor(dword te_flags, int _font_index, dword _font_flags):
      scroll_y(0),
      font_index(_font_index),
      font_flags(_font_flags)
   {}
   virtual ~C_text_editor(){}

//----------------------------
// Reset cursor's flashing, so that cursor phase is on.
   virtual void ResetCursor() = 0;

//----------------------------
// Get text string (null-terminated).
   virtual const wchar *GetText() const = 0;

//----------------------------
// Get length of edited text, without terminating null character.
   virtual dword GetTextLength() const = 0;

//----------------------------
// Get maximal lenght of editable text.
   virtual dword GetMaxLength() const = 0;

//----------------------------
   virtual void SetCase(dword _allowed, dword curr) = 0;

//----------------------------
// Check if cursor is now visible (enabled and flash phase is visible).
   virtual bool IsCursorVisible() const = 0;

//----------------------------
// Replace current selection by text.
   virtual void ReplaceSelection(const wchar *txt) = 0;

//----------------------------
// Check if editor is in secret mode.
   virtual bool IsSecret() const = 0;

//----------------------------
// Set cursor position. pos = cursor position, sel = anchor (selection) position
   virtual void SetCursorPos(int pos, int sel) = 0;
   inline void SetCursorPos(int pos){ SetCursorPos(pos, pos); }

   virtual int GetCursorPos() const = 0;
   virtual int GetCursorSel() const = 0;

//----------------------------
// Get length of currently edited inline text length and position.
// Inline text range is used on some platforms in multi-tap or predictive mode, and should be displayed differently (underline).
   virtual int GetInlineText(int &pos) const{ return 0; }

//----------------------------
// Activate or deactivate text editor. Only one editor may be active at one time.
   virtual void Activate(bool) = 0;

//----------------------------
// CCPU operations.
   virtual void Cut() = 0;
   virtual void Copy() = 0;
   virtual bool CanPaste() const = 0;
   virtual void Paste() = 0;

//----------------------------
// Copy/get text to/from clipboard.
   static void ClipboardCopy(const wchar *text);
   static Cstr_w ClipboardGet();

//----------------------------
// Used on some platforms on touch screen to invoke text input dialog.
   virtual void InvokeInputDialog(){}

//----------------------------
// If needed, directly process input.
   virtual void ProcessInput(const S_user_input &ui){}

   virtual bool IsReadOnly() const = 0;
};

                              //text editor initialization flags
static const int
   TXTED_NUMERIC = 1,         //only numeric text entry
   TXTED_ALLOW_PREDICTIVE = 2,//use predictive text (if available)
   TXTED_ALLOW_EOL = 4,       //allow multiline text
   TXTED_ANSI_ONLY = 8,       //only ansi characters allowed
   TXTED_SECRET = 0x10,       //initialize in secret mode (for password entering)

   TXTED_CREATE_INACTIVE = 0x20, //create text editor initially deactivated
   TXTED_EMAIL_ADDRESS = 0x40,//entering mail address
   TXTED_WWW_URL = 0x80,      //entering url
   TXTED_READ_ONLY = 0x100,   //text is not editable, cursor is visible
   TXTED_REAL_VISIBLE = 0x200,

                              //Android: action for virtual keypad button, only one may be used
   TXTED_ACTION_DONE = 0x0000,   //"Done", results in hiding virtual keypad
   TXTED_ACTION_ENTER = 0x1000,  //normal enter button in shown, results in sending K_ENTER
   TXTED_ACTION_GO = 0x2000,     //"Go", results in sending K_ENTER
   TXTED_ACTION_SEARCH = 0x3000, //"Search"    ''           K_ENTER
   TXTED_ACTION_SEND = 0x4000,   //"Send"    ''             K_ENTER
   TXTED_ACTION_NEXT = 0x5000,   //"Next"      ''           K_CURSORDOWN
   TXTED_ACTION_MASK = 0xf000;

//----------------------------
#include "Mode.h"

//----------------------------
                              //main class for application with user interface,
                              // providing graphics, text entry, socket notify, menu, modes
class C_application_ui: public C_application_base, public C_socket_notify{
   void InitMenuAfterRescale(C_menu *menu);
   virtual void InitInputCaps();
   bool shadow_inited;
   C_smart_ptr<C_image> text_edit_state_icons;//icons used for text editor state; created from txt_icons.png in data file
   C_smart_ptr<C_image> buttons_image; //long image with all buttons images

                              //soft buttons
#ifndef ANDROID
   enum{
      TITLE_BUTTON_MINIMIZE,
      TITLE_BUTTON_MAXIMIZE,
      TITLE_BUTTON_LAST
   };
#endif
#ifdef _WIN32_WCE
   C_button but_keyboard;     //WM virtual keyboard invoke button
#endif
#ifdef USE_MOUSE
   enum{ MAX_BUTTONS = 4 };
   C_button softkey_bar_buttons[MAX_BUTTONS];   //used only with mouse-based device
   void InitSoftkeyBarButtons();
   friend class C_ctrl_softkey_bar;
#endif
   void InitShadow();
   void CloseShadow();

//----------------------------
// Fill screen rectangle by color ('pix' contains color in destination colorspace).
// Similar to FillRect, but faster.
   void FillPixels(const S_rect &rc, dword pix);

   wchar *texts_buf;          //text buffer - first are offsets to all TXT_ID's, then are actual texts
   dword num_text_lines;
   dword LoadTexts(const char *lang_name, const C_zip_package *dta, wchar *&texts_buf, dword num_lines);

   bool ui_touch_mode;           //true if user controls by touches, false if by keypad; used to optimize drawing/controlling UI

//----------------------------
// Xor given rectangle.
   void XorRect(const S_rect &rc) const;

//----------------------------
// Notification from text editor when something in its state changes:
//    cursor_blink: cursor blinking changes
//    cursor_moved: cursor position was changed
//    text_changed: text was altered
//!! replaced by notifications on control
   void TextEditNotify(C_text_editor *te, bool cursor_blink, bool cursor_moved, bool text_changed);

   friend class C_text_editor_common;

protected:
   C_application_ui();
   ~C_application_ui();

//----------------------------

   bool Construct(const wchar *data_path);
public:
//----------------------------
// Draw just menu items.
   void DrawMenuContents(const C_menu *menu, bool clear_bgnd, bool dimmed = false);

//----------------------------
// Draw parent menu of the menu.
   void DrawParentMenu(const C_menu *mp);

//----------------------------
// Completely redraw menu
   void RedrawDrawMenuComplete(const C_menu *menu);

#ifndef ANDROID
//----------------------------
// Check if app can use internal font based on language selection.
   bool CanUseInternalFont() const;
#endif
//----------------------------
protected:
   C_text_editor *CreateTextEditorInternal(dword flags, int font_index, dword font_flags, const wchar *init_text, dword max_length, C_text_editor::C_text_editor_notify *notify);

//----------------------------
// Default socket event callback, treating context as C_mode*, routing event to mode's SocketEvent function.
   virtual void SocketEvent(E_SOCKET_EVENT ev, C_socket *socket, void *context);

//----------------------------

   C_button soft_buttons[
      2
#if defined USE_MOUSE && !defined ANDROID
      + TITLE_BUTTON_LAST
#endif
   ];

   const wchar *data_path;    //pointer to application's data path (including last '\\')
public:
   const wchar *temp_path;    //temp path where working files can be downloaded (must be in lower-case)
protected:
                              //shadow
   enum{
      SHD_LT,
      SHD_LT1,
      SHD_T,
      SHD_RT, SHD_RT1,
      SHD_L,
      SHD_R,
      SHD_LB,
      SHD_LB1,
      SHD_B,
      SHD_RB,
      SHD_RB1,
      SHD_LAST
   };
   C_smart_ptr<C_image> shadow[SHD_LAST];

                              //helper for drawing formatted text
   struct S_draw_fmt_string_data{
      const S_text_display_info *tdi;
      int line_height;
      const S_rect *rc_bgnd;
      bool justify;
   };

public:
   inline const wchar *GetDataPath() const{ return data_path; }

                              //from C_apllication_base
   virtual void ScreenReinit(bool resized);
   virtual void InitAfterScreenResize();

   inline bool IsUiTouchMode() const{ return ui_touch_mode; }

//----------------------------
// Load texts from localized text file.
   void LoadTexts(const char *lang_name, const C_zip_package *dta, dword last_id);

//----------------------------
   enum E_SPECIAL_TEXT_ID{    //special text id's used in this class
      SPECIAL_TEXT_NULL = 0,
      SPECIAL_TEXT_OK = 0x8000,
      SPECIAL_TEXT_SELECT,
      SPECIAL_TEXT_CANCEL,
      SPECIAL_TEXT_SPELL,
      SPECIAL_TEXT_PREVIOUS,  //previous entry in predictive text
      SPECIAL_TEXT_CUT,
      SPECIAL_TEXT_COPY,
      SPECIAL_TEXT_PASTE,
      SPECIAL_TEXT_YES,
      SPECIAL_TEXT_NO,
      SPECIAL_TEXT_BACK,
      SPECIAL_TEXT_MENU,
   };

//----------------------------
// Returns width of text, in pixels. If max_size is positive, it means number of chars, negative means number of pixels
   int GetTextWidth(const void *str, bool wide, E_TEXT_CODING, dword font_index, dword font_flags = 0, int max_width = 0, const S_text_display_info *tdi = NULL) const;
   inline int GetTextWidth(const wchar *str, dword font_index, dword font_flags = 0, int max_chars = 0) const{
      return GetTextWidth(str, true, COD_DEFAULT, font_index, font_flags, max_chars);
   }

//----------------------------
// Get info of edited text, useful for text editing of wrapped text.
// - determine lengths of all lines as they fit into given width (display_width) with given font
// - from given cursor position, determine line, row and pixel offset where cursor is located
   struct S_text_line{
      const wchar *str;
      int len;
   };
   void DetermineTextLinesLengths(const wchar *wp, int font_index, int display_width, int cursor_pos, C_vector<S_text_line> &lines, int &cursor_line, int &cursor_row, int &cursor_pixel) const;

//----------------------------
// Get localized text line identified by id. If id is beyond loaded range, then NULL is returned.
// Localized application should override this function, convert id's in E_SPECIAL_TEXT_ID range to localized text id and call this func.
   virtual const wchar *GetText(dword id) const;

//----------------------------
// Returns width of text, in pixels.
// If 'max_width' is negative, then it represents widht in pixels, otherwise it is # of bytes to be drawn from input string.
   int DrawEncodedString(const void *str, bool wide, bool bottom_align, int x, int y, E_TEXT_CODING, S_text_style &ts, int max_width, const S_draw_fmt_string_data *fmt = NULL, const S_text_display_info *tdi = NULL);
   int DrawString2(const void *str, bool wide, int x, int y, dword font_index, dword font_flags, dword color, int max_width);
   inline int DrawString(const wchar *str, int x, int y, dword font_index, dword font_flags = 0, dword color = 0xff000000, int max_width = 0){
      return DrawString2(str, true, x, y, font_index, font_flags, color, max_width);
   }
   inline int DrawStringSingle(const Cstr_c &str, int x, int y, dword font_index, dword font_flags = 0, dword color = 0xff000000, int max_width = 0){
      return DrawString2(str, false, x, y, font_index, font_flags, color, max_width);
   }
//----------------------------
// html image drawing
   int DrawHtmlImage(const S_image_record &ir, const S_text_style &ts, const S_draw_fmt_string_data *fmt, int x, int y, dword bgnd_pix);

//----------------------------
// From given string, style and width, find length that fits in provided width.
// The length is number of bytes, including any formatting commands.
// The function adjusts 'ts' from parsed text.
// Returns number of bytes of text.
   int ComputeFittingTextLength(const void *cp, bool wide, int rc_width, const S_text_display_info *tdi, S_text_style &ts, int &line_height, int default_font_index) const;

//----------------------------
// Count lines in given text, and vertical size in pixels.
// It writes to num_lines and total_height of 'tdi' structure.
   void CountTextLinesAndHeight(S_text_display_info &tdi, dword default_font_index) const;

//----------------------------
// Scroll displayed text, defined by 'tdi' to desired number of pixels (positive=scroll forward, negative = scroll back)
// This func changes members of 'tdi': pixel_offset, byte_offs, ts.
// Returns true if something was changed.
   bool ScrollText(S_text_display_info &tdi, int num_pixels) const;

//----------------------------
// Draw message body into specified rectangle, using given font.
   void DrawFormattedText(const S_text_display_info &tdi, const S_rect *rc_bgnd = NULL);

//----------------------------
// Draw title text on top of dialog window.
   void DrawDialogTitle(const S_rect &rc, const wchar *txt);

//----------------------------
// Get height of soft button bar.
   virtual dword GetSoftButtonBarHeight() const;

//----------------------------
// Initialize rectangles of soft buttons.
   virtual void InitSoftButtonsRectangles();

//----------------------------
// Draw soft buttons.
   virtual void DrawSoftButtons(const wchar *menu, const wchar *secondary);

//----------------------------
// Get height of dialog title bar.
   virtual dword GetDialogTitleHeight() const;

//----------------------------
// Get height of title bar.
   virtual dword GetTitleBarHeight() const;

//----------------------------
// Check if screen is considered to be used in wide-screen mode.
   virtual bool IsWideScreen() const;

//----------------------------
// Draw shadow under rectangle. If expand is true, rc is expanded one pixel.
   virtual void DrawShadow(const S_rect &rc, bool expand = false);

//----------------------------
// Fill rectangle by gradient.
   void FillRectGradient(const S_rect &rc, dword col);
   void FillRectGradient(const S_rect &rc, dword col_lt, dword col_rb);

//----------------------------
// Draw dialog rectangle with possible top bar area.
   void DrawDialogRect(const S_rect &rc, dword col_fill, bool draw_top_bar = false);
   inline void DrawDialogRect(const S_rect &rc, E_COLOR col_fill = COL_BACKGROUND, bool draw_top_bar = false){
      DrawDialogRect(rc, GetColor(col_fill), draw_top_bar);
   }

//----------------------------
// Process soft buttons by mouse. Returns true if soft button action was taken (and ui.key had written code of softkey).
// When process_title_bar=true, also title bar maximize and minimize buttons are processed.
   bool ProcessMouseInSoftButtons(S_user_input &ui, bool &redraw, bool process_title_bar = true);

//----------------------------
// Process single button, return true if button was clicked (button's action should be invoked).
   bool ButtonProcessInput(C_button &but, const S_user_input &ui, bool &redraw);

//----------------------------
// Create menu.
   C_smart_ptr<C_menu> CreateMenu(C_mode &mod, dword menu_id = 0, bool inherit_touch = true);

//----------------------------
// Create sub-menu on menu containing Cut, Copy, Paste options, which are enabled by state of text editor.
   static C_smart_ptr<C_menu> CreateEditCCPSubmenu(const C_text_editor *te, C_menu *parent_menu);

//----------------------------
// Prepare menu (after all items were added to it).
   void PrepareMenu(C_menu *menu, int selection = 0);

//----------------------------
// flags = TXTED_* flags
// font_index = UI_FONT_SMALL or UI_FONT_BIG
// font_flags = FF_* flags
   C_text_editor *CreateTextEditor(dword flags, int font_index, dword font_flags = 0, const wchar *init_text = NULL, dword max_length = 0, C_text_editor::C_text_editor_notify *notify = NULL);

//----------------------------
// Draw edited text field.
// bgnd_color is color below edited text, for drawing possible transparent rectangle.
   void DrawEditedText(const C_text_editor &te, bool draw_rect = true);

//----------------------------
// Process mouse selections in text editor. Return true if screen should be redrawn.
   bool ProcessMouseInTextEditor(C_text_editor &te, const S_user_input &ui);

//----------------------------
// Setup text editor so, that cursor is visible when drawn using DrawTextEditor.
   void MakeSureCursorIsVisible(C_text_editor &te) const;

//----------------------------
// Draw text editor's state (case mode, current multi-press letters, etc).
// Drawing can override names of softkey buttons, if editing states uses them for own purpuse.
// y means vertical position (by default on softkey bar), can be positioned elsewhere for special purpose.
   void DrawTextEditorState(const C_text_editor &te, dword &soft_button_menu_txt_id, dword &soft_button_secondary_txt_id, int y = -1);

//----------------------------
// Proceess mouse in scrollbar - move position. Returns state of change.
   C_scrollbar::E_PROCESS_MOUSE ProcessScrollbarMouse(C_scrollbar &sb, const S_user_input &ui) const;

//----------------------------
// Draw title bar.
// clear_bar_sy: -1 = fill default height, 0 = don't fill; >0 = fill given height
   virtual void DrawTitleBar(const wchar *txt, int clear_bar_sy = -1, bool draw_titlebar_buttons = true);

//----------------------------//----------------------------// Skinnable elements

//----------------------------
// Draw washout effect over entire screen. Used for drawing active items over inactive background.
// Can be overriden by application to provide own effect.
   virtual void DrawWashOut();

// Clear rectangle to background color.
   virtual void ClearToBackground(const S_rect &rc);

//----------------------------
// Clear rectangle to work area color.
   virtual void ClearWorkArea(const S_rect &rc);

//----------------------------
// Draw scrollbar with speciffied background.
   enum E_SCROLLBAR_DRAW{
      SCROLLBAR_DRAW_NO_BGND,
      SCROLLBAR_DRAW_BACKGROUND,
      SCROLLBAR_DRAW_DIALOG,
                              //other values mean argb color
   };
   virtual void DrawScrollbar(const C_scrollbar &sb, dword bgnd = SCROLLBAR_DRAW_NO_BGND);

//----------------------------
// Draw progress bar.
//    color = fill color, or default if not specified
//    col_bgnd = background color, or grey if not specified
   virtual void DrawProgress(const C_progress_indicator &pi, dword color = 0, bool border = true, dword col_bgnd = 0);

protected:
//----------------------------
// Draw background of menu rectangle.
// When 'also_frame' is set, function draws also frame around menu, otherwise draws only area of menu rectangle.
   virtual void DrawMenuBackground(const C_menu *menu, bool also_frame, bool dimmed);

public:
//----------------------------
// Clear top title bar by title color. If 'bottom' is -1, then default height of title bar is used, otherwise specified height is used.
   virtual void ClearTitleBar(int bottom = -1);

//----------------------------
// Clear soft-button area. 'top' specifies top y pixel from which up to screen bottom will be cleared; if -1, then default height of soft bar is used.
   virtual void ClearSoftButtonsArea(int top = -1);

//----------------------------
// Draw text frame and background. 'bgnd_color' is used if frame is transparent.
   virtual void DrawEditedTextFrame(const S_rect &rc);

//----------------------------
// Flag if DrawEditedTextFrame draws transparent or opaque rectangle.
   virtual bool IsEditedTextFrameTransparent() const;

//----------------------------
// Draw selection outline.
   virtual void DrawSelection(const S_rect &rc, bool clear_to_bgnd = false);

//----------------------------
// Draw single-line separator.
   virtual void DrawSeparator(int l, int sx, int y);

//----------------------------
// Draw double-line separator.
   virtual void DrawThickSeparator(int l, int sx, int y);

//----------------------------
// Draw preview window with 2-pixel expanded frame.
   virtual void DrawPreviewWindow(const S_rect &rc);

//----------------------------
// Draw base of dialog - rectangle, outline, title, shadow. If rc_clip is used, then 'rc' is rectangle of dialog base, and it is drawn only into 'rc_clip'.
   virtual void DrawDialogBase(const S_rect &rc, bool draw_top_bar, const S_rect *rc_clip = NULL);

//----------------------------
// Get padding around dialog rectangle (additional pixels drawn around dialog's rectangle).
   virtual void GetDialogRectPadding(int &l, int &t, int &r, int &b) const;

//----------------------------
// Draw menu item selection.
   virtual void DrawMenuSelection(const C_menu *menu, const S_rect &rc){ DrawSelection(rc); }

//----------------------------
// Draw soft button rectangle. Returns true if rectangle was drawn (for touch screens).
   virtual bool DrawSoftButtonRectangle(const C_button &but);

//----------------------------
// Scrollbar width.
   virtual dword GetScrollbarWidth() const;

//----------------------------
// Progress bar height.
   virtual dword GetProgressbarHeight() const;

//----------------------------
// Draw button base.
   virtual void DrawButton(const C_button &but, bool enabled);

//----------------------------
// Draw checkbox.
   virtual void DrawCheckbox(int x, int y, int size, bool enabled, bool frame = true, byte alpha = 255);

//----------------------------
// Application's preference for kinetic scrolling.
   virtual bool IsKineticScrollingEnabled() const{ return true; }

//----------------------------

   C_smart_ptr<C_mode> mode;  //currently active mode

//----------------------------
// Calls parent class's FocusChange, then calls FocusChange on active mode and all parent modes.
   virtual void FocusChange(bool foreground);

//----------------------------
// Process input in menu. Return text id of selected item, or -1 if not selected yet.
// For items represented as strings (not text id), value 0x10000 + index of item is returned.
   int MenuProcessInput(C_mode &mod, S_user_input &ui, bool &redraw);

   C_smart_ptr<C_menu> CreateTouchMenu(C_menu *parent = NULL, dword menu_id = 0);

#ifdef USE_MOUSE
//----------------------------
// Prepare touch menu (circle) on given position.
   void PrepareTouchMenu(C_menu *menu, const S_point &pos, bool animation = true);
// Similar, but position is taken from ui.
   inline void PrepareTouchMenu(C_menu *menu, const S_user_input &ui){
      PrepareTouchMenu(menu, ui.mouse);
   }
private:
   void PrepareTouchMenuCircle(C_menu *menu);
   void AnimateTouchMenu(C_mode &mod, int time);
   void PrepareTouchMenuItems(C_menu *menu);
public:
#endif//USE_MOUSE

   const C_image *GetButtonsImage() const{ return buttons_image; }

//----------------------------
// Common key names, which may differ among platfomrs (in [] brackets, used in menus).
   static const char send_key_name[], ok_key_name[];
   const char *delete_key_name, *shift_key_name;

//----------------------------
// Get shortcut name used in menus for keys with Shift. Format 'fmt' should be similar to "[% + K]", where % is replaced by shift key name (Shift).
   Cstr_c GetShiftShortcut(const char *fmt) const;

//----------------------------
// Make this mode active. It also calls Release on mod.
// If currently set mode has timer, the timer is paused.
// !! replaced by C_mode::Activate
   void ActivateMode(C_mode &mod, bool draw = true){
      mod.Activate(draw);
   }

//----------------------------
// Close this mode - make it inactive, and activate it's parent. If there's no parent, call Exit.
// If reactivated mode has timer, it is resumed.
// After reactivation, redraw screen (optional).
// !! replaced by C_mode::Close
   void CloseMode(C_mode &mod, bool redraw_screen = true){
      mod.Close(redraw_screen);
   }

//----------------------------
// Set timer for mode. Mode will be ticked in specified intervals.
// !! replaced by C_mode::CreateTimer
   inline void CreateTimer(C_mode &mod, dword ms){ mod.CreateTimer(ms); }

//----------------------------
// Find mode identified by ID. Starting from active mode, iterating up by parents.
   C_mode *FindMode(dword id);

//----------------------------
// Redraw currently active mode.
   virtual void RedrawScreen();

//----------------------------
// Implements C_application_base::TimerTick, provides timer tick of active mode.
   virtual void TimerTick(C_timer *t, void *context, dword ms);

//----------------------------
// Calls Tick on mod. 'time' is time from last call of this callback, in milliseconds.
   virtual void ClientTick(C_mode &mod, dword time);

//----------------------------
// Implements C_application_base::ProcessInput, routes input to active mode.
   virtual void ProcessInput(S_user_input &ui);

//----------------------------

#ifdef USE_MOUSE
//----------------------------
// Update buttons - detect clicked are down state.
// Return index of clicked button, or -1.
// !! replaced by C_ctrl_softkey_bar
   int TickBottomButtons(const S_user_input &ui, bool &redraw, dword enabled_buttons = (1<<MAX_BUTTONS)-1);

//----------------------------
// Draw buttons at bottom.
// mod = mode on which buttons are drawn
// but_defs = button indexes (rectangle index in Buttons.png image); or -1 for not drawing button
// txt_id_hints = text id's for displaying text hint about button (when touching for longer on button); or 0 for not drawing text hint
// enabled_buttons = bitmask of enabled buttons
// !! replaced by C_ctrl_softkey_bar
   void DrawBottomButtons(const C_mode &mod, const char but_defs[MAX_BUTTONS], const dword txt_id_hints[MAX_BUTTONS], dword enabled_buttons = (1<<MAX_BUTTONS)-1);
#endif//USE_MOUSE

//----------------------------
// Draw info on softkey bar - button names and possibly text editor state.
   void DrawSoftButtonsBar(const C_mode &mod, dword txt_id_menu, dword txt_id_secondary, const C_text_editor *te = NULL);

//----------------------------
//----------------------------
                              //currently initiated connection
   C_smart_ptr<C_internet_connection> connection;

//----------------------------
// Init internet connection (if not yet), store it in connection variable. Return pointer to it.
   virtual C_internet_connection *CreateConnection();

//----------------------------
// Close Internet connection in variable 'connection'.
   virtual void CloseConnection(){ connection = NULL; }
};

//----------------------------

                           //C_mode class with reference to C_application_ui-derived class 'app'
template<typename T = C_application_ui>
class C_mode_app: public C_mode{
protected:
   typedef T t_type;

   C_mode_app(T &_app):
      C_mode(_app, _app.mode),
      app(_app)
   {}
public:
   T &app;
};

//----------------------------
                              //base class for popup dialog window
class C_mode_dialog_base: public C_mode_app<C_application_ui>{
   class C_ctrl_dialog_base_title_bar;

   bool buttons_in_dlg;

   virtual dword GetMenuSoftKey() const{ return NULL; }
   virtual dword GetSecondarySoftKey() const{ return NULL; }
protected:
   virtual void InitButtons();
   virtual void ClearToBackground(const S_rect &rc);
   virtual C_ctrl_title_bar *CreateTitleBar();

   S_rect rc;                 //rect of dialog popup (not including shadow)

//----------------------------
// Sets size of client area. Should be called from InitLayout. It computes rc size/position and places correctly controls.
// Width may be 0, in which case default width will be set.
   void SetClientSize(const S_point &sz);

//----------------------------
// true if soft buttons are inside of dialog, not at softkey area.
   inline bool HasButtonsInDialog() const{ return buttons_in_dlg; }

   class C_but: public C_ctrl_button{
   public:
      C_but(C_mode *m, dword _id):
         C_ctrl_button(m, _id)
      {}
      virtual void OnClicked(){
         S_user_input ui; ui.Clear(); ui.key = id;
         bool r;
         mod.ProcessInput(ui, r);
      }
      C_button &GetBut(){ return but; }
   } *buttons[2];
   Cstr_w lsb_txt, rsb_txt;

public:
   C_mode_dialog_base(C_application_ui &_app, dword lsk_tid, dword rsk_tid);
   void SetSoftKeys(const wchar *lsk_tid, const wchar *rsk_tid);

//----------------------------
// Init layout so that dialog takes full screen. Child mode can change size by calling SetClientSize func.
   virtual void InitLayout();

//----------------------------
   virtual void ProcessInput(S_user_input &ui, bool &redraw);
   virtual void Draw() const;
};

//----------------------------
//----------------------------

#endif

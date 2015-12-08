#include <Rules.h>
#include <AppBase.h>
#include <C_vector.h>
#include <C_buffer.h>
#include <SmartPtr.h>
#include <Fixed.h>
#include <TimeDate.h>
#include <C_file.h>
#include <Cstr.h>
#include <Socket.h>
#include <UI\UserInterface.h>
#include <TextUtils.h>
#include <UI\SkinnedApp.h>
#include <UI\Config.h>
#include <UI\About.h>

//----------------------------

#if (defined MAIL || defined MESSENGER) && defined CONNECTION_HAVE_ALTERNATIVE_IAP
#define USE_ALT_IAP
#endif


//----------------------------
extern const int VERSION_HI, VERSION_LO;
extern const char VERSION_BUILD; //may be 0 or a-z

//----------------------------
//----------------------------

namespace encoding{

//----------------------------
// Determine whether given coding uses multi-byte set (e.g. utf8).
bool IsMultiByteCoding(E_TEXT_CODING coding);

}//encoding
//----------------------------
#include <Network.h>

extern const wchar app_name[];
//----------------------------

enum E_TEXT_ID{
   TXT_NULL,
#ifdef MAIL

#include "Email\TextId.h"

#elif defined MUSICPLAYER
#include "MusicPlayer\TextId.h"
#elif defined EXPLORER

#include "Explorer\TextId.h"

#elif defined MESSENGER
#include "Messenger\TextId.h"
#elif defined SMARTMOVIE
#include "VideoPlayer\SmartMovie\TextId.h"
#endif
   TXT_LAST,
};

//----------------------------

#ifdef WEB_FORMS
struct S_html_form{
   //Cstr_c name, id;
   Cstr_c action;
   Cstr_c js_onsubmit,        //form is submitted
      js_onreset;             //form is reset
   bool use_post;
   S_html_form():
      use_post(false)
   {}
};

//----------------------------

struct S_html_form_select_option: public C_list_mode_base{
   /*
   int selection;
   C_scrollbar sb;
   S_rect rc;
   */

   struct S_option{
      Cstr_c value;
      Cstr_w display_name;
      bool selected, init_selected;
      S_option():
         selected(false),
         init_selected(false)
      {}
   };
   C_buffer<S_option> options;
   int selected_option, init_selected_option;     //for SELECT_SINGLE
   int select_visible_lines;  //for SELECT_MULTI

   S_html_form_select_option():
      selected_option(-1),
      select_visible_lines(0)
   {}
   S_html_form_select_option &operator =(const S_html_form_select_option &o){
      selection = o.selection;
      sb = o.sb;
      rc = o.rc;
      options = o.options;
      init_selected_option = o.init_selected_option;
      select_visible_lines = o.select_visible_lines;
      return *this;
   }
};

//----------------------------

struct S_js_events{
   Cstr_c js_onclick,         //pointing device button is clicked over an element
      js_onfocus,             //element receives focus either by the pointing device or by tabbing navigation
      js_onblur,              //element loses focus either by the pointing device or by tabbing navigation
      js_onselect,            //user selects some text in a text field; usable with INPUT (text) and TEXTAREA
      js_onchange;            //control loses the input focus and its value has been modified since gaining focus
   bool ParseAttribute(const Cstr_c &attr, const Cstr_c &val);
};

//----------------------------

struct S_html_form_input: public S_js_events{
   enum E_TYPE{
      TEXT,
      PASSWORD,
      CHECKBOX,
      RADIO,
      IMAGE,
      HIDDEN,
      SUBMIT,
      RESET,
      SELECT_SINGLE,
      SELECT_MULTI,
      TEXTAREA,
      FILE,
   } type;
   Cstr_c name, id;
   Cstr_w value, init_value;
   bool disabled;
   bool checked, init_checked;   //for RADIO, CHECKBOX
   int max_length;            //for TEXTBOX, PASSWORD, TEXTAREA
   int form_index;            //form to which this input applies
   int form_select;           //select form index, which keeps data of this input (valid for SELECT type only)
   short edit_sel, edit_pos;  //for CHECKBOX, PASSWORD

   int sx, sy;                //size in pixels

                              //initial values

   S_html_form_input():
      type(TEXT),
      checked(false), init_checked(false),
      max_length(0),
      sx(0), sy(0),
      form_select(-1),
      edit_sel(0), edit_pos(0),
      disabled(false)
      //rows(20), cols(4)
   {}
};

//----------------------------
#ifdef WEB
                              //document-object-model (DOM) entry
struct S_dom_element{
   Cstr_c name;
   C_vector<S_dom_element> children;
};
#endif//WEB
#endif//WEB_FORMS

//----------------------------
                              //base mode for settings - it is an item list, with possible text editor
class C_mode_settings: public C_list_mode_base{
public:
   C_smart_ptr<C_text_editor> text_editor;

   int max_textbox_width;  //max width of single-line text editor (in pixels)

   C_mode_settings()
   {}
};

//----------------------------

class C_archive_simulator: public C_zip_package{
public:
   virtual bool DeleteFile(const wchar *filename) = 0;
   virtual const wchar *GetRealFileName(const wchar *filename) const = 0;

   typedef void (*t_Change)(void*);
   virtual void SetChangeObserver(t_Change c, void *context) = 0;

   virtual bool GetFileWriteTime(const wchar *filename, S_date_time &td) const;
};

//----------------------------

template<class T_APP>
class C_mode_list: public C_mode_app<T_APP>, public C_list_mode_base{
   virtual C_application_ui &AppForListMode() const{ return C_mode_app<T_APP>::app; }
public:
   C_mode_list(T_APP &_app):
      C_mode_app<T_APP>(_app)
   {}
   virtual void ResetTouchInput(){
      touch_down_selection = -1;
      C_list_mode_base::ResetTouchInput();
      C_mode_app<T_APP>::ResetTouchInput();
   }
};

//----------------------------
//----------------------------

class C_client: public C_application_skinned, public C_about_keystroke_callback{
   typedef C_application_skinned super;
   bool show_license;
protected:
   virtual void ToggleUseClientRect();

#ifdef WEB_FORMS
   int DrawHtmlFormInput(const S_text_display_info &tdi, const S_html_form_input &fi, bool active, const S_text_style &ts,
      const S_draw_fmt_string_data *fmt, int x, int y, dword bgnd_pix, const char *text_stream);
#endif

   void ConvertStringToUnicode(const char *src, E_TEXT_CODING coding, Cstr_w &dst) const;

//----------------------------
// Convert multi-byte coding into unicode. If string is not multi-byte, then ConvertStringToUnicode is called.
   virtual void ConvertMultiByteStringToUnicode(const char *src, E_TEXT_CODING coding, Cstr_w &dst) const;

//----------------------------
public:
//----------------------------
// Draw nicely-formatted filename, so that extension is always shown.
   void DrawNiceFileName(const wchar *fn, int x, int y, dword font_index, dword color, int width);
   void DrawNiceURL(const char *url, int x, int y, dword font_index, dword color, int width);

   enum E_BOTTOM_BUTTON{
      BUT_NO = -1,
      BUT_ZOOM_FIT,
      BUT_ZOOM_ORIGINAL,
      BUT_ZOOM_IN,
      BUT_ZOOM_OUT,
      BUT_COPY,
      BUT_MOVE,
      BUT_DELETE,
      BUT_LAST_BASE
   };
protected:
//----------------------------

   static const S_config_store_type save_config_values_base[];

//----------------------------

   struct S_connection_settings{
#if defined __SYMBIAN32__
      int iap_id;
      int secondary_iap_id;
#else
      Cstr_w wce_iap_name;  //access point name
#endif
      int connection_time_out;   //in seconds

      S_connection_settings():
#ifdef __SYMBIAN32__
         iap_id(-1),
         secondary_iap_id(-1),
#endif
         connection_time_out(60)
      {}
   };
public:
   struct S_config: public S_connection_settings{
      enum{                   //common flags (lower 16 bits)
         CONF_PRG_AUTO_UPDATE = 1,     //check for updates automatically
         //CONF_APPROVED_CUSTOMER = 2,   //customer approved online for this registered copy
         CONF_PROBABLY_PIRATE = 4,     //    '' not    ''
         CONF_KEEP_CONNECTION = 8,     //keep open connection (no auto-disconnect)

         /*
         CONF_DRAW_COUNTER_NO = 0,     
         CONF_DRAW_COUNTER_CURRENT = 0x40,
         CONF_DRAW_COUNTER_TOTAL = 0x80,
         CONF_DRAW_COUNTER_MASK = 0xc0,
         */

         CONF_BROWSER_SHOW_HIDDEN = 0x100,   //show hidden files in browser
         CONF_BROWSER_SHOW_ROM = 0x200,      //show ROM drives in browser
         CONF_BROWSER_SHOW_SYSTEM = 0x400,   //show system files in browser (sys, system, private, resource, ...)
         CONF_BROWSER_SHOW_RAM = 0x800,      //show hidden files in browser

         CONF_NEW_VERSION_AVAILABLE = 0x1000,//new version was detected on server
         //CONF_USE_SYSTEM_FONT = 0x2000,      //
         _CONF_SYSTEM_FONT_ANTIALIAS = 0x4000,
         _CONF_USE_CLIENT_RECT = 0x8000,
      };
      dword flags;
      short ui_font_size_delta;     //font index to use for UI; +- delta range from default font size
      byte viewer_font_index; //font index to use for displaying text in viewers (0 - 1)
      int last_auto_update;   //time of last update (seconds) (-1 = never)
      //C_fixed img_ratio;
      Cstr_c app_language;
      Cstr_w last_browser_path;
      int color_theme;
      int total_data_sent, total_data_received;
      bool fullscreen;
      bool use_system_font;

      S_config();

      virtual bool IsFullscreenMode() const{ return fullscreen; }
   };
   S_config &config;

   virtual const S_config_store_type *GetConfigStoreTypes() const = 0;
   virtual Cstr_w GetConfigFilename() const;

   bool LoadConfig(){ return LoadConfiguration(GetConfigFilename(), &config, GetConfigStoreTypes()); }
   void SaveConfig(){ SaveConfiguration(GetConfigFilename(), &config, GetConfigStoreTypes()); }

#if defined USE_SYSTEM_SKIN || defined USE_OWN_SKIN
   void EnableSkinsByConfig();
#endif

//----------------------------

   enum E_CONFIG_ITEM_TYPE_CLIENT{
      CFG_ITEM_COLOR_THEME = CFG_ITEM_LAST_BASIC,
      CFG_ITEM_TEXT_FONT_SIZE,    //works only on config
#ifndef USE_ANDROID_TEXTS
      CFG_ITEM_LANGUAGE,          //language - works only on 'config'
#endif
      CFG_ITEM_APP_PASSWORD,      //Cstr_w
      CFG_ITEM_FONT_SIZE,         //elem_offset is offs to short

      CFG_ITEM_LAST_CLIENT
   };

//----------------------------
   class C_configuration_editing_client: public C_configuration_editing{
   protected:
      C_client &app;
      bool modified, need_save;
   public:
#ifndef USE_ANDROID_TEXTS
      C_vector<Cstr_c> langs;
      void CollectLanguages();
#endif
      C_configuration_editing_client(C_client &_a):
         app(_a),
         need_save(false),
         modified(false)
      {}
      virtual void OnConfigItemChanged(const S_config_item &ec);
      virtual void OnClick(const S_config_item &ec){}
      virtual void OnClose(){ CommitChanges(); }
      virtual void CommitChanges();

   };
//----------------------------
protected:
   class C_mode_config_client;
   friend class C_mode_config_client;

   class C_mode_config_client: public C_mode_configuration{
      inline C_client &App(){ return (C_client&)app; }
      inline const C_client &App() const{ return (C_client&)app; }
   public:
      C_vector<C_internet_connection::S_access_point> iaps;

      C_mode_config_client(C_client &app, const S_config_item *opts, dword num_opts, C_configuration_editing *ce, dword title_id = TXT_CONFIGURATION, void *cfg_base = NULL);
      ~C_mode_config_client(){
         delete configuration_editing;
      }
      virtual bool GetConfigOptionText(const S_config_item &ec, Cstr_w &str, bool &draw_left_arrow, bool &draw_right_arrow) const;
      virtual bool ChangeConfigOption(const S_config_item &ec, dword key, dword key_bits);
      virtual void DrawItem(const S_config_item &ec, const S_rect &rc_item, dword color) const;
   };

//----------------------------
public:

   inline void MarkNewVersionAvailable(){ config.flags |= config.CONF_NEW_VERSION_AVAILABLE; SaveConfig(); }

//----------------------------

   enum E_FILE_TYPE{
      FILE_TYPE_ARCHIVE,
      FILE_TYPE_WORD,
      FILE_TYPE_TEXT,
      FILE_TYPE_UNKNOWN,
      FILE_TYPE_IMAGE,
      FILE_TYPE_VIDEO,
      FILE_TYPE_AUDIO,
      FILE_TYPE_EXE,
      FILE_TYPE_XLS,
      FILE_TYPE_PDF,
      FILE_TYPE_EMAIL_MESSAGE,
      FILE_TYPE_CONTACT_CARD, //VCF
      FILE_TYPE_CALENDAR_ENTRY, //VCS
      FILE_TYPE_LINK, //LNK
      FILE_TYPE_URL, //URL
      FILE_TYPE_LAST
   };

   static E_FILE_TYPE DetermineFileType(const wchar *ext);

//----------------------------

   void CopyTextToClipboard(const wchar *txt);

   static void MapScrollingKeys(S_user_input &ui);

//----------------------------
//----------------------------
   void AddEditSubmenu(C_menu *menu){
      menu->AddItem(TXT_EDIT, C_menu::HAS_SUBMENU);
   }

   void ProcessCCPMenuOption(int option, C_text_editor *te);

//----------------------------
   virtual void GetDateString(const S_date_time &dt, Cstr_w &str, bool force_short = false) const;
protected:
//----------------------------
// Try to open image for text, and set it to 'ir'. Compute proper image size.
// If it can't be opened, set alt text of 'ir'.
   C_image *OpenHtmlImage(const Cstr_w &fname, const S_text_display_info &tdi, S_image_record &ir) const;

//----------------------------
// Get Y coordinate of top-of-line at specified byte offset in body.
   int GetLineYAtOffset(const S_text_display_info &tdi, int byte_offs) const;

//----------------------------

   dword InitHtmlImage(const Cstr_c &img_src, const Cstr_w &img_alt, int img_sx, int img_sy, const char *www_domain,
      S_text_display_info &tdi, C_vector<char> &dst_buf, S_image_record::t_align_mode align_mode, int border) const;

//----------------------------

   static bool GoToNextActiveObject(S_text_display_info &tdi);
   static bool GoToPrevActiveObject(S_text_display_info &tdi);

   static void InitActiveObjectFocus(S_text_display_info &tdi, E_TEXT_CONTROL_CODE cc, int indx);

//----------------------------
public:
   struct S_progress_display_help{
      C_client *_this;
      C_progress_indicator progress;
      C_progress_indicator progress1;
      bool need_update_whole_screen;

      S_progress_display_help(C_client *t):
         _this(t),
         need_update_whole_screen(true)
      {}
   };
   void PrepareProgressBarDisplay(E_TEXT_ID title, const wchar *txt, S_progress_display_help &pd, dword progress_total, E_TEXT_ID progress1_text = TXT_NULL);
   static bool DisplayProgressBar(dword pos, void *context);

//----------------------------

   enum{
      SOCKET_LOG_NO,
      SOCKET_LOG_YES,
      SOCKET_LOG_YES_NOSHOW,
   } socket_log;

   virtual const wchar *GetText(dword id) const;
#ifdef USE_ANDROID_TEXTS
   mutable Cstr_w texts_cache[TXT_LAST];
#endif

//----------------------------

   virtual bool ProcessKeyCode(dword code);
protected:

//----------------------------

   virtual void GetFontPreference(int &relative_size, bool &use_system_font, bool &antialias) const{
      relative_size = config.ui_font_size_delta;
      use_system_font = config.use_system_font;
      antialias = true;
   }

//----------------------------
//----------------------------
public:

//----------------------------
// Create popup window with error message and OK button.
   void ShowErrorWindow(E_TEXT_ID title, const wchar *err);
   inline void ShowErrorWindow(E_TEXT_ID title_txt_id, dword err_txt_id){ ShowErrorWindow(title_txt_id, GetText(err_txt_id)); }

   void ModifySettingsTextEditor(C_mode_settings &mod, const S_config_item &ec, byte *cfg_base);
protected:


//----------------------------
public:
//----------------------------
//----------------------------
   bool OpenFileBySystem(const wchar *filename, dword app_uid = 0);

//----------------------------
// Draw floating data counter. If data_cnt_bgnd is NULL, it is drawn entirely. If it points to image, this image is used as cached background under counters for faster redrawing.
   //void DrawFloatDataCounters(C_smart_ptr<C_image> &data_cnt_bgnd, bool save_bgnd = true, int force_right = -1, int force_y = -1);

//----------------------------
// On Windows Mobile, run owner-name control panel to setup device owner name.
   void WM_RunOwnerNameDialog(){
#ifdef _WIN32_WCE
      if(system::WM_RunOwnerNameDialog(*this)){
         trial_info.InitSerialNumbers();
         RedrawScreen();
      }
#endif
   }

   const wchar *GetDataPath() const{ return data_path; }

   virtual C_internet_connection *CreateConnection();
   virtual void CloseConnection();
   void StoreDataCounters();

//----------------------------

   class C_multi_item_loader: public C_application_http::C_http_data_loader{
   public:
      C_vector<Cstr_c> entries;
      C_progress_indicator prog_all;

      void BeginNextEntry();
      void Suspend();
   };

//----------------------------
// Open web browser with given url.
   void StartBrowser(const char *url);

protected:

   Cstr_w GetErrorName(const Cstr_w &text) const;

//----------------------------
public:
                                 //class for viewing text (possibly containing images, hyperlinks, etc),
                                 // allowing dynamically loading images from net,
                                 // having menu
   class C_text_viewer{
   public:
      int title_height;
      S_rect rc;

                                 //html image loader
      C_multi_item_loader img_loader;

      S_text_display_info text_info;
      int auto_scroll_time;   //zero=no, positive=down, negative=up
      C_scrollbar sb;

                                 //hyperlink show
      S_rect rc_link_show;
      Cstr_c link_show;
      int link_show_count;

      bool mouse_drag;
      S_point drag_mouse_begin;
      S_point drag_mouse_prev;

      C_text_viewer():
         auto_scroll_time(0),
         link_show_count(0),
         mouse_drag(false)
      {}
      void ResetTouchInput(){
         text_info.active_object_offs = -1;
         text_info.active_link_index = -1;
      }
      void SuspendLoader();
   };
//----------------------------
// Start displaying html link (in small text window).
   void ViewerShowLink(C_text_viewer &mod) const;

//----------------------------
// Process mouse in text - scroll, activate hyperlinks.
// Returns true if currently active hyperlink should be opened.
   bool ViewerProcessMouse(C_text_viewer &mod, const S_user_input &ui, int &scroll_pixels, bool &redraw);

   void ViewerProcessKeys(C_text_viewer &mod, const S_user_input &ui, int &scroll_pixels, bool &redraw);
   void ViewerGoToVisibleHyperlink(C_text_viewer &mod, bool up);

//----------------------------
// Tick viewer.
   void ViewerTickCommon(C_text_viewer &mod, int time, bool &redraw);

protected:
//----------------------------
#ifdef _WIN32_WCE
   virtual void PatchedCodeHackCatch();   //must be virtual, otherwise it's optimized by linker
#endif

//----------------------------
// Return true if agreement was displayed.
   bool DisplayLicenseAgreement();

   virtual void FinishConstruct() = 0;

//----------------------------

   static bool CopyFile(const wchar *src, const wchar *dst);

//----------------------------

   C_client(S_config &con);
public:
   C_client():
      config(config){}  //dummy constructor for declarations of shadow classes
   virtual ~C_client();

//----------------------------
// Return true if config was successfully loaded.
   bool BaseConstruct(const C_zip_package *dta, dword ig_flags = 0);

//----------------------------

   virtual void Exit();

//----------------------------

   static C_file *CreateFile(const wchar *filename, const C_zip_package *arch = NULL);

   C_smart_ptr<C_image> CreateImage() const{
      C_smart_ptr<C_image> r = C_image::Create(*this);
      r->Release();
      return r;
   }

};

//----------------------------
class C_viewer_previous_next{
public:
//----------------------------
// Callback function for getting previous or next filename from logical list, used in viewer.
// If filename is NULL, we're just testing if adjacent item is available.
   virtual bool GetPreviousNextFile(bool next, Cstr_w *filename, Cstr_w *title) = 0;
};

//----------------------------

#include <UI\InfoMessage.h>
#include <UI\Question.h>
#include <UI\FormattedMessage.h>

//----------------------------
//----------------------------

namespace file_utils{
bool ReadString(C_file &ck, encoding::C_str_cod &s);
//----------------------------
// Read string from file.
// Watch for reading behind file size.
// First dword is string size; if it is -1, the string is explicitly 'undefined';
//    otherwise it is size of string (in characters); if OR'd by bit 31, the string is wide-char, otherwise it is single-char.
// Optional parameter 'string_valid' is set to string 'validity' (false if string undefined).
bool ReadString(C_file &ck, Cstr_w &s);
bool ReadStringAsUtf8(C_file &ck, Cstr_w &s);
bool ReadString(C_file &ck, Cstr_c &s);

//----------------------------
void WriteString(C_file &ck, const Cstr_w &s);
void WriteStringAsUtf8(C_file &ck, const Cstr_w &s);
void WriteString(C_file &ck, const Cstr_c &s);
void WriteString(C_file &ck, const encoding::C_str_cod &s);

//----------------------------
// Write/read encrypted string. Suitable for saving passwords.
void WriteEncryptedString(C_file &ck, const Cstr_c &s, const char *encription);
bool ReadEncryptedString(C_file &ck, Cstr_c &s, const char *encription);

}//file_utils

//----------------------------

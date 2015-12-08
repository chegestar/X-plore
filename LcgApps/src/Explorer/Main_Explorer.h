#include "..\FileBrowser.h"
#include <Ui\TextEntry.h>

//----------------------------

C_zip_package *CreateRarPackage(const wchar *fname);
C_zip_package *CreateRarPackage(C_file *fp, bool take_ownership);

//----------------------------

                              //specialized file class, reading data from zip package
class C_file_rar: public C_file{
public:
   bool Open(const wchar *fname, const C_zip_package *rar);
};

//----------------------------

class C_messaging_session: public C_unknown{
public:
   typedef void (*t_Change)(void*);
protected:
   t_Change change_callback;
   void *change_context;
public:
#ifdef _DEBUG
   void SimulateChangeCallback(){
      if(change_callback)
         change_callback(change_context);
   }
#endif

   C_messaging_session():
      change_callback(NULL)
   {}
   void SetChangeObserver(t_Change c, void *context){
      change_callback = c;
      change_context = context;
   }
   static C_messaging_session *Create();
};

//----------------------------

class C_mailbox_arch_sim: public C_archive_simulator{
public:
   struct S_item{
      Cstr_w filename;
      Cstr_w full_filename;
      dword id;
      dword size;
      S_date_time modify_date;

      bool DeleteMessage(class C_messaging_session *ms);
      C_file *CreateFile(const C_messaging_session *ms) const;
      bool SetMessageAsRead(C_messaging_session *ms);
   };
protected:
   C_vector<S_item> items;

   void EnumMessages(C_messaging_session *ms);
};

//----------------------------

C_mailbox_arch_sim *CreateMessagingArchive();

//----------------------------

class C_wallpaper_utils{
public:
   static bool CanSetWallpaper();
   static bool SetWallpaper(const wchar *filename);
};

//----------------------------
//----------------------------

class C_explorer_client: public C_client{
protected:
   friend class C_client;
   friend class C_client_file_mgr;
   friend class C_client_viewer;

   virtual const wchar *GetDataFileName() const{ return L"Explorer\\Data.dta"; }

//----------------------------

   mutable void *char_conv;
   void CloseCharConv();

//----------------------------

                              //C_application_base methods:
   virtual void FocusChange(bool foreground);
   virtual void Construct();
   virtual void FinishConstruct();
//----------------------------

public:
   virtual void OpenDocument(const wchar *fname);

   static const int MAX_FIND_TEXT_LEN = 30;
   virtual dword GetColor(E_COLOR col) const;
   virtual void ConvertMultiByteStringToUnicode(const char *src, E_TEXT_CODING coding, Cstr_w &dst) const;

   Cstr_w last_find_text;

   struct S_config_xplore: public S_config{
      enum{
         CONF_VIEWER_LANDSCAPE = 0x40000,
         CONF_SYSTEM_APP = 0x80000,
         /*
         CONF_SORT_BY_NAME = 0x000000,
         CONF_SORT_BY_DATE = 0x100000,
         CONF_SORT_BY_EXT = 0x200000,
         CONF_SORT_BY_SIZE = 0x300000,
         CONF_SORT_MASK = 0x300000,
         */
         //CONF_KEEP_GPRS = 0x400000,    //keep open gprs connection (no auto-disconnect)
         CONF_ASK_TO_EXIT = 0x800000,
         CONF_VIEWER_FULLSCREEN = 0x1000000,
      };
      enum E_SORT_MODE{
         SORT_BY_NAME,
         SORT_BY_DATE,
         SORT_BY_EXT,
         SORT_BY_SIZE,
         SORT_BY_DATE_REVERSE,
      } sort_mode;
      Cstr_w app_password;
      E_TEXT_CODING default_text_coding;
      byte audio_volume;
      Cstr_w find_files_text;

   //----------------------------
                              //hash/unhash password
      void SetPassword(const wchar *txt);
      Cstr_w GetPassword() const;

      S_config_xplore();
   };
   S_config_xplore config_xplore;
protected:

   static const S_config_store_type save_config_values[];

   virtual const S_config_store_type *GetConfigStoreTypes() const{ return save_config_values; }

//----------------------------

   struct S_association{
      Cstr_w ext;
      Cstr_w app_name;
      dword uid;
   };
   C_buffer<S_association> associations;
   bool assocs_loaded;

   void LoadAssociations();
   void SaveAssociations() const;

//----------------------------

public:
   Cstr_w quick_folders[10];
   bool quick_folders_loaded;
   void LoadQuickFolders();
   void SaveQuickFolders() const;
protected:
   bool QuickFolders_SelectCallback(const Cstr_w *file, const C_vector<Cstr_w> *files);

//----------------------------
   enum E_CONTROL_TYPE{
      CFG_ITEM_TEXT_CODING = CFG_ITEM_LAST_CLIENT,
      CFG_ITEM_SORT_MODE,
   };

   static const S_config_item config_options[];

//----------------------------
   class C_configuration_editing_xplore: public C_configuration_editing_client{
      typedef C_configuration_editing_client super;
      C_explorer_client &App(){ return (C_explorer_client&)app; }
   public:
      C_configuration_editing_xplore(C_explorer_client &_a):
         super(_a)
      { }
      virtual void OnConfigItemChanged(const S_config_item &ec);
      virtual void OnClose();
      virtual void OnClick(const S_config_item &ec);
   };
//----------------------------

   class C_mode_config_explorer;
   friend class C_mode_config_explorer;

   class C_mode_config_explorer: public C_mode_config_client{
      typedef C_mode_config_client super;
      inline C_explorer_client &App(){ return (C_explorer_client&)app; }
      inline const C_explorer_client &App() const{ return (C_explorer_client&)app; }

   public:
      C_mode_config_explorer(C_explorer_client &_app, const S_config_item *opts, dword num_opts, C_configuration_editing_xplore *ce):
         super(_app, opts, num_opts, ce)
      {
      }
      virtual bool GetConfigOptionText(const S_config_item &ec, Cstr_w &str, bool &draw_left_arrow, bool &draw_right_arrow) const;
      virtual bool ChangeConfigOption(const S_config_item &ec, dword key, dword key_bits);
   };

//----------------------------

   static int CompareFileEntries(const void *m1, const void *m2, void *context);

//----------------------------
   class C_mode_attributes;
   friend class C_mode_attributes;

   class C_mode_attributes: public C_mode_app<C_explorer_client>{
   public:
      S_rect rc;
      int selection;
      C_vector<Cstr_w> filenames;
      struct S_attrib{
         bool not_same;
         char state;          //0=false, 1=true, 2=don't change
      } atts[3];
      mutable bool draw_bgnd;
      bool changed;

      C_mode_attributes(C_explorer_client &_app):
         C_mode_app<C_explorer_client>(_app),
         draw_bgnd(true),
         selection(0),
         changed(false)
      {}
      virtual void InitLayout();
      virtual void ProcessInput(S_user_input &ui, bool &redraw);
      virtual void Draw() const;
   };
public:
   void SetModeEditAttributes(C_client_file_mgr::C_mode_file_browser &mod, int all_in_dir = -1);
protected:
//----------------------------
//----------------------------

   struct S_device_info{
      dword total_ram, free_ram;
      int cpu_speed;
      int battery_percent;

      S_device_info():
         battery_percent(-1),
         cpu_speed(-1)
      {}
      void Read();
   };
   void SetModeDeviceInfo();

//----------------------------
//----------------------------
                              //mode for viewing tasks/processes
public:
   class C_mode_item_list;
   friend class C_mode_item_list;
protected:
   void InitLayoutItemList(C_mode_item_list &mod);
   void ItemListProcessMenu(C_mode_item_list &mod, int itm, dword menu_id);
   void ItemListProcessInput(C_mode_item_list &mod, S_user_input &ui, bool &redraw);
   void DrawItemListContents(const C_mode_item_list &mod);
   void DrawItemList(const C_mode_item_list &mod);
public:
   class C_mode_item_list: public C_mode, public C_list_mode_base{
      virtual C_application_ui &AppForListMode() const{ return app; }
   public:
      enum E_TYPE{
         PROCESSES,
         TASKS,
         CHOOSE_APP,
         ASSOCIATIONS,
         QUICK_DIRS,
      } type;
      struct S_item{
         Cstr_w display_name;
         Cstr_w internal_name;
         dword id;
         dword size;

         bool operator ==(const S_item &itm) const{
            return (display_name==itm.display_name && internal_name==itm.internal_name && id==itm.id && size==itm.size);
         }

         bool TerminateProcess();
         bool TerminateTask();
         bool StartTask();

         void Test();
      };
      C_vector<S_item> items;

      virtual int GetNumEntries() const{ return items.size(); }

      C_smart_ptr<C_text_editor> te_edit;
      Cstr_w edited_assoc_ext;

      C_mode_item_list(C_application_ui &_app, E_TYPE t, C_mode *sm):
         C_mode(_app, sm),
         type(t)
      {}
      static int CompareItems(const void*, const void*, void*);

      virtual void InitLayout(){ static_cast<C_explorer_client&>(app).InitLayoutItemList(*this); }
      virtual void ProcessInput(S_user_input &ui, bool &redraw){ static_cast<C_explorer_client&>(app).ItemListProcessInput(*this, ui, redraw); }
      virtual void ProcessMenu(int itm, dword menu_id){ static_cast<C_explorer_client&>(app).ItemListProcessMenu(*this, itm, menu_id); }
      virtual void DrawContents() const{ static_cast<C_explorer_client&>(app).DrawItemListContents(*this); }
      virtual void Draw() const{ static_cast<C_explorer_client&>(app).DrawItemList(*this); }
   };
public:
   C_mode_item_list &SetModeItemList(C_mode_item_list::E_TYPE type, bool draw = true);
protected:
   void ItemList_Close(C_mode_item_list &mod);

   void SetModeFileAssociations(const wchar *init_ext);

   bool ItemList_TerminateProcessOrTask(C_mode_item_list &mod, bool ask);
   void ItemList_TerminateProcessOrTaskConfirm(void*){
      ItemList_TerminateProcessOrTask((C_mode_item_list&)*mode, false);
   }
   void ItemList_ActivateTask(C_mode_item_list &mod);

   class C_del_assoc_question;
   friend class C_del_assoc_question;
   class C_del_assoc_question: public C_question_callback{
      C_explorer_client &app;
      virtual void QuestionConfirm(){
         app.ItemList_DeleteAssociation();
      }
   public:
      C_del_assoc_question(C_explorer_client &_a): app(_a){}
   };

   void ItemList_DeleteAssociation();
   void ItemList_EditOrNewAssociation(C_mode_item_list &mod);
   void ItemList_BeginEditAssociation(C_mode_item_list &mod);
   void ItemList_AssociationChooseApp(C_mode_item_list &mod);

   void ItemList_ConfirmChooseApp(C_mode_item_list &mod);

   void ItemList_SelectQuickFolder(C_mode_item_list &mod);

   void EnumProcesses(C_vector<C_mode_item_list::S_item> &items);
   void EnumTasks(C_vector<C_mode_item_list::S_item> &items, bool only_running);

//----------------------------
//----------------------------

   class C_mode_hex_edit;
   friend class C_mode_hex_edit;
   void InitLayoutHexEdit(C_mode_hex_edit &mod);
   void HexEditProcessMenu(C_mode_hex_edit &mod, int itm, dword menu_id);
   void HexEditProcessInput(C_mode_hex_edit &mod, S_user_input &ui, bool &redraw);
   void DrawHexEdit(const C_mode_hex_edit &mod);
   void HexTextEditNotify(C_mode_hex_edit &mod, bool cursor_moved, bool text_changed, bool &redraw);

   class C_mode_hex_edit: public C_mode_app<C_explorer_client>{
   public:
      static const dword MAX_EDIT_FILE_SIZE = 1024*1024*8;

      Cstr_w filename;
      Cstr_w display_name;

      int font_index;
      int title_height;
      dword num_cols;
      int zero_char_width;
      int x_bytes, sx_byte;   //x position of drawn hex bytes, and width of one hex byte
      int x_chars, sx_char;
      enum{
         SEARCH_NO,
         SEARCH_CHARS,
         SEARCH_HEX,
      } last_search_mode;

      S_rect rc;
      C_scrollbar sb;
      C_file *fp;
      dword file_size;
      bool modified;
      bool insert_mode;
      bool can_edit;
      bool edit_chars;
      dword mark_start, mark_end;

      byte *edit_buf;   //data kept when editing
      dword edit_buf_size;
      C_smart_ptr<C_text_editor> te_edit;
      int edit_x, edit_y;

      void FindChars();
      void FindHex();
      inline dword Size() const{ return file_size; }
      inline bool IsMarked(dword offs) const{ return (offs>=mark_start && offs<mark_end); }
      inline bool IsEditMode() const{ return (te_edit!=NULL); }
      void EditEnsureVisible();
      bool EditCursorLeft();
      void EditCursorRight();
      void IncEditBuf(dword offs);

      C_mode_hex_edit(C_explorer_client &_app, const wchar *fn, C_file *fp1, bool can_edit_):
         C_mode_app<C_explorer_client>(_app),
         filename(fn),
         modified(false),
         edit_buf(NULL),
         insert_mode(false),
         edit_chars(false),
         last_search_mode(SEARCH_NO),
         fp(fp1),
         can_edit(can_edit_),
         file_size(fp1->GetFileSize()),
         mark_start(0), mark_end(0),
         edit_x(0), edit_y(0)
      {
      }
      ~C_mode_hex_edit(){
         delete fp;
         delete[] edit_buf;
      }
      virtual void InitLayout(){ static_cast<C_explorer_client&>(app).InitLayoutHexEdit(*this); }
      virtual void ProcessInput(S_user_input &ui, bool &redraw){ static_cast<C_explorer_client&>(app).HexEditProcessInput(*this, ui, redraw); }
      virtual void ProcessMenu(int itm, dword menu_id){ static_cast<C_explorer_client&>(app).HexEditProcessMenu(*this, itm, menu_id); }
      virtual void Draw() const{ static_cast<C_explorer_client&>(app).DrawHexEdit(*this); }
      virtual void TextEditNotify(bool cursor_moved, bool text_changed, bool &redraw){ static_cast<C_explorer_client&>(app).HexTextEditNotify(*this, cursor_moved, text_changed, redraw); }
   };
public:
   bool SetModeHexEdit(const wchar *fname, const C_zip_package *arch, bool edit);
protected:
   void HexEditClose(C_mode_hex_edit &mod, bool ask = true);
   void HexEdit_BeginEdit(C_mode_hex_edit &mod);

   void TxtHexEditorFind(C_mode_hex_edit &mod, const Cstr_w &txt, bool hex);
   void TxtHexEditorFindChars(C_mode_hex_edit &mod, const Cstr_w &txt){ TxtHexEditorFind(mod, txt, false); }
   void TxtHexEditorFindHex(C_mode_hex_edit &mod, const Cstr_w &txt){ TxtHexEditorFind(mod, txt, true); }

//----------------------------
//----------------------------
public:
   void SetModeConfig();

//----------------------------
                              //password management
   void TxtNewPasswordEntered(const Cstr_w &txt);
   void TxtCurrPasswordEntered(const Cstr_w &txt);
   void TxtStartPasswordEntered(const Cstr_w &txt);

//----------------------------

   void TxtViewerFindEntered(C_mode &mod, const Cstr_w &txt);
   void ViewerFindText();
   void FileBrowserFindTextEntered(C_client_file_mgr::C_mode_file_browser &mod, const Cstr_w &txt);
protected:
//----------------------------
   bool OpenFileByAssociationApp(const wchar *fname);

//----------------------------
// Extract file from archive to temporary folder, and open by system.
   bool FileBrowser_ExtractToTempAndOpen(const C_client_file_mgr::C_mode_file_browser &mod);

   class C_question_extract;
   friend class C_question_extract;
   class C_question_extract: public C_question_callback{
      C_client_file_mgr::C_mode_file_browser &mod;
      virtual void QuestionConfirm(){
         ((C_explorer_client&)mod.C_mode::app).FileBrowser_ExtractToTempAndOpen(mod);
      }
   public:
      C_question_extract(C_client_file_mgr::C_mode_file_browser &_m): mod(_m){}
   };

   void FileBrowser_GoToRealFile(C_client_file_mgr::C_mode_file_browser &mod);

//----------------------------

   class C_question_restart;
   friend class C_question_restart;
   class C_question_restart: public C_question_callback{
      C_explorer_client &app;
      virtual void QuestionConfirm(){
         app.FileBrowser_ExitOk();
         system::RebootDevice();
      }
   public:
      C_question_restart(C_explorer_client &_a): app(_a){}
   };

//----------------------------

   struct S_viewer_pos{
      enum{ MAX_ENTRIES = 5 };
      struct S_entry{
         Cstr_w name;
         dword offs;
      };
      C_vector<S_entry> entries;
      bool read;

      void Load();
      void Save();
      S_viewer_pos():
         read(false)
      {}
      void SaveTextPos(dword offs, const wchar *filename, const C_zip_package *arch);
      int GetTextPos(const wchar *filename, const C_zip_package *arch);
   } viewer_pos;

//----------------------------

   void DeleteCachePath();

//----------------------------

   inline void FileBrowser_ExitOk(){
      C_client_file_mgr::FileBrowser_Close(this, (C_client_file_mgr::C_mode_file_browser&)*mode);
   }
public:
   class C_question_exit;
   friend class C_question_exit;
   class C_question_exit: public C_question_callback{
      C_explorer_client &app;
      virtual void QuestionConfirm(){
         app.FileBrowser_ExitOk();
      }
   public:
      C_question_exit(C_explorer_client &_a): app(_a){}
   };
protected:
//----------------------------
// Search for text from given byte offset, return found byte offset, or -1 if not found. match_len is set to bytes range of matching source text (may wary depending on encoding).
   int SearchText(S_text_display_info &tdi, int offset, const Cstr_w &text, bool ignore_case, int &match_len) const;

public:
   void CreateTextEditMode(const wchar *filename, const wchar *display_name, bool is_new_file);
   C_explorer_client();
   ~C_explorer_client();
};

//----------------------------
//----------------------------

#ifndef __FILE_BROWSER_H
#define __FILE_BROWSER_H

#include <Ui\MultiQuestion.h>
#include "SimpleSoundPlayer.h"

#ifdef EXPLORER
#define STDC
#include <Zlib.h>
#endif

#if defined MAIL //|| defined EXPLORER
#define AUDIO_PREVIEW
#endif

//----------------------------
                              //callback used for selecting file(s) using file browser
                              // either single file/folder or multiselection may be returned.
class C_file_browser_callback{
public:
//----------------------------
// Functions called when file(s) are selected.
// Before calling these, the file browser mode is closed.
   virtual void FileSelected(const Cstr_w &file){}
   virtual void FilesSelected(const C_vector<Cstr_w> &files){}
};

//----------------------------
                              //client with file browser and viewer
class C_client_file_mgr: public C_client{
public:
   static const dword ID = FOUR_CC('F','M','G','R');

   class C_obex_file_send{
   public:

      static bool CanSendByBt();
      static bool CanSendByIr();

      virtual ~C_obex_file_send(){}

   //----------------------------

      enum E_STATUS{
         ST_NULL,
         ST_INITIALIZING,
         ST_SENDING,
         ST_CLOSING,
         ST_DONE,
         ST_CANCELED,
         ST_FAILED,
      };
      enum E_TRANSFER{
         TR_BLUETOOTH,
         TR_INFRARED,
      };

      virtual E_STATUS GetStatus() const = 0;
      virtual dword GetBytesSent() const = 0;
      virtual dword GetFileIndex() const = 0;
   };

//----------------------------
//----------------------------
                              //mode for exploring file system,
                              // choosing destination directory
   class C_mode_file_browser;
   friend class C_mode_file_browser;
private:

   void ProcessMenu(C_mode_file_browser &mod, int itm, dword menu_id);
public:
   class C_mode_file_browser: public C_mode_list<C_client_file_mgr>, public C_viewer_previous_next{
      virtual C_application_ui &AppForListMode(){ return app; }
      virtual bool IsPixelMode() const{ return true; }
      virtual bool WantInactiveTimer() const{ return false; }
      virtual void ScrollChanged();
      virtual bool GetPreviousNextFile(bool next, Cstr_w *filename, Cstr_w *title);
   public:
      enum{
         SPR_BROWSER_ICON_MESSAGING,
         SPR_BROWSER_ICON_DRIVE,
         SPR_BROWSER_ICON_DIR,
         SPR_BROWSER_ICON_FILE_ARCHIVE,
         SPR_BROWSER_ICON_FILE_WORD,
         SPR_BROWSER_ICON_FILE_TEXT,
         SPR_BROWSER_ICON_FILE_DEFAULT,
         SPR_BROWSER_ICON_FILE_IMAGE,
         SPR_BROWSER_ICON_FILE_VIDEO,
         SPR_BROWSER_ICON_FILE_AUDIO,
         SPR_BROWSER_ICON_FILE_EXE,
      };
      C_smart_ptr<C_image> icons;
      dword icon_sx;

      enum E_MODE{
         MODE_EXPLORER,       //classic explorer and all non-specialized modes
         MODE_ARCHIVE_EXPLORER,   //archive explorer, showing contents of an archive file
         MODE_GET_SAVE_FILENAME, //get name of destination filename for save
#if defined MAIL || defined WEB || defined EXPLORER
         MODE_COPY,           //select destination for copy operation
         MODE_MOVE,           // ''  move
#endif
      } browser_mode;
      E_TEXT_ID title;
      enum{
         FLG_SAVE_LAST_PATH = 1,
         FLG_ALLOW_RENAME = 2,
         FLG_ALLOW_DELETE = 4,
         FLG_ALLOW_MAKE_DIR = 8,
#if defined MAIL || defined WEB || defined EXPLORER
         FLG_ALLOW_COPY_MOVE = 0x10,
         FLG_ALLOW_SHOW_SUBMENU = 0x20,//add Show Hidden/Rom menu
         FLG_ALLOW_SEND_BY = 0x40,     //add Send By menu
#endif
         FLG_ALLOW_DETAILS = 0x80,
         FLG_ALLOW_MARK_FILES = 0x100, //allow multiselection
         FLG_ACCEPT_FILE = 0x200,      //accept files for opening/multiselection
         FLG_ACCEPT_DIR = 0x400,       //  ''    dirs    ''
         FLG_SELECT_OPTION = 0x800,    //add Select option as default action (instead of Open)
         FLG_AUTO_COLLAPSE_DIRS = 0x1000,   //automatically collapse dirs to have only one opened tree
         FLG_ALLOW_ATTRIBUTES = 0x2000,
         FLG_AUDIO_PREVIEW = 0x4000,
      };
      dword flags;
                              //callback for open action; accepting either one file/dir, or list of marked files/dirs
                              // old style, use C_file_browser_callback instead
      typedef bool (C_client::*t_OpenCallback)(const Cstr_w *file, const C_vector<Cstr_w> *files);
      t_OpenCallback _OpenCallback;
      C_file_browser_callback *open_callback;
      bool del_open_cb;

      struct S_entry{
         Cstr_w name;
         enum{
            DRIVE,
            ARCHIVE,
            DIR,
            FILE,
         } type;
         enum{
            MAX_DRAW_LEVEL = 15
         };
         int level;
                              //expand status
         enum{
            COLLAPSED,
            EXPANDED,
            NEVER_EXPAND,
         } expand_status;

         enum{
            ATT_MEMORY_CARD = 0x80000000,
         };
         dword atts;
         union{
            struct{
               dword free[2], total[2];
            } drive;
            dword file_size;  //file size; -1 = unknown (linux)
         } size;
         S_date_time modify_date;
         S_entry():
            expand_status(COLLAPSED),
            atts(0)
         {
#ifdef UNIX_FILE_SYSTEM
            atts = C_file::ATT_ACCESS_READ | C_file::ATT_ACCESS_WRITE;
#endif
         }
      };
      C_vector<S_entry> entries;
      C_smart_ptr<C_text_editor> te_rename;
      dword op_flags;

      virtual int GetNumEntries() const{ return entries.size(); }

                              //multi-selection (marked files)
      C_vector<Cstr_w> marked_files;
      schar shift_mark, drag_mark;
      //bool shift_release_toggle_mark;
      bool title_touch_down;

      Cstr_w save_as_name;    //save-as default filename

                              //for MODE_ARCHIVE_EXPLORER: explored archive
      const C_smart_ptr<C_zip_package> arch;
      C_file ck_arch;         //archive cache (used for just keeping zip file locked)

      const char *extension_filter;

      dword enabled_buttons;  //bits specify which bottom buttons are enabled
      dword expand_box_size;

      bool timer_need_refresh;
      bool timer_file_details;
      C_smart_ptr<C_image> img_thumbnail;

#ifdef AUDIO_PREVIEW
      class C_audio_preview{
      public:
         enum{
            //BUTTON_BACK,
            BUTTON_PLAY,
            //BUTTON_FORWARD,
            BUTTON_VOL_UP,
            BUTTON_VOL_DOWN,
            BUTTON_LAST
         };
         bool active;
         bool paused;
         bool err;
         S_rect rc;
         C_button buttons[BUTTON_LAST];
         C_smart_ptr<C_simple_sound_player> plr;
         C_progress_indicator progress;
         S_rect rc_volbar;
         byte &volume;

         C_audio_preview(byte &_vol):
            active(false),
            err(false),
            volume(_vol)
         {}
         void Close(){
            plr = NULL;
            active = false;
         }
      } audio_preview;
      void LayoutAudioPreview();
      bool ChangeAudioPreviewVolume(bool up);
      void AudioPreviewTogglePlay();
      void AudioPreviewEnable();
#endif

      C_mode_file_browser(C_client_file_mgr &app, E_MODE fbm, E_TEXT_ID _tit, t_OpenCallback oc, dword opf, C_file_browser_callback *cb, bool _del_open_cb);
      ~C_mode_file_browser();
      static int CompareEntries(const void *m1, const void *m2, void *context);
      static void SwapEntries(void *m1, void *m2, dword w, void *context);
      void BeginRename();

   //----------------------------
   // Check if browser is launched as stand-alone application.
      inline bool IsStandalone() const{ return (!saved_parent); }

      virtual void InitLayout();
      virtual void ProcessInput(S_user_input &ui, bool &redraw);
      virtual void ProcessMenu(int itm, dword menu_id){ static_cast<C_client_file_mgr&>(app).ProcessMenu(*this, itm, menu_id); }
      virtual void Tick(dword time, bool &redraw);
      virtual void Draw() const;
      virtual void DrawContents() const;
      virtual dword GetPageSize(bool up);
      virtual void SelectionChanged(int prev_selection);

      void ChangeSelection(int sel);
      void FindTextEntered(const Cstr_w &txt);
   };

//----------------------------
   class C_mode_long_operation;
   friend class C_mode_long_operation;

   class C_mode_long_operation: public C_mode_dialog_base
#ifdef EXPLORER
      , public C_multi_selection_callback
#endif
   {
      typedef C_mode_dialog_base super;
      virtual bool WantInactiveTimer() const{ return false; }
      virtual bool WantBackgroundTimer() const{ return true; }
#ifdef EXPLORER
      virtual void Select(int option){
         ((C_client_file_mgr&)app).FileBrowser_OverwriteConfirm(option);
      }
      virtual void Cancel(){
         ((C_client_file_mgr&)app).FileBrowser_OverwriteConfirm(-1);
      }
#endif
      bool DeleteFile(const wchar *fn, bool is_folder);
      enum{
         DEL_RD_ONLY_ASK,
         DEL_RD_ONLY_ALL,
         DEL_RD_ONLY_NO,
      } del_rd_only;

      void EndOfWork(E_TEXT_ID err_title = TXT_NULL, const wchar *err = NULL);
      void WindowMoved();  //!!
   public:
      int frame_count;

      C_progress_indicator progress_1;
      C_progress_indicator progress_2;
      C_vector<Cstr_w> filenames;
      dword file_index;
      mutable int last_prog_1, last_prog_2;
                              //copy, move:
      C_vector<Cstr_w> filenames_dst;
      C_file *fl_src;
      C_file fl_dst;
      Cstr_w fn_dst;

      enum E_OVERWRITE_MODE{
         OVERWRITE_ASK,
         OVERWRITE_YES,
         OVERWRITE_NO,
      } overwrite_mode;
      C_buffer<byte> copy_buf;

#ifdef EXPLORER
      struct S_zip_file_info{
         dword local_hdr_offs;
         dword size_compr, size_uncompr;
         dword crc;
         dword time_date;
         Cstr_c filename_utf8;
         Cstr_w rel_filename;
      };
      C_vector<S_zip_file_info> zip_files;
      C_buffer<byte> compr_buf;
      bool zip_move;

      z_stream zs;
#endif

      enum E_OPERATION{
         OPERATION_DELETE,
         OPERATION_COPY,
         OPERATION_MOVE,
#ifdef EXPLORER
         OPERATION_ZIP,
#endif
      } operation;

      C_mode_long_operation(C_application_ui &_app, E_OPERATION _op, const Cstr_w &_title):
         C_mode_dialog_base(_app, 0, TXT_CANCEL),
         operation(_op),
         file_index(0),
         fl_src(NULL),
         frame_count(0),
         del_rd_only(DEL_RD_ONLY_ASK),
         overwrite_mode(OVERWRITE_ASK)
      {
         SetTitle(_title);
      }
      ~C_mode_long_operation(){
         delete fl_src;
                              //delete unfinished file
         if(fl_dst.GetMode()!=C_file::FILE_NO){
            fl_dst.Close();
            C_file::DeleteFile(fn_dst);
         }
      }

      inline C_mode_file_browser &GetFileBrowser(){ return (C_mode_file_browser&)*saved_parent; }

      virtual void InitLayout();
      virtual void ProcessInput(S_user_input &ui, bool &redraw);
      virtual void Tick(dword time, bool &redraw);
      virtual void Draw() const;

      void DrawContent() const;
   };

   C_mode_long_operation *SetModeCopyMove(C_mode_file_browser &mod, const C_vector<Cstr_w> &src_files, const Cstr_w &dst_path, bool move);

#ifdef EXPLORER
   void FileBrowser_OverwriteConfirm(int sel);
#endif

   C_smart_ptr<C_image> CreateImageThumbnail(const wchar *fn, int limit_sx, int limit_sy, const C_zip_package *dta) const;
private:
   C_mode_file_browser &SetModeFileBrowser(C_mode_file_browser::E_MODE mode = C_mode_file_browser::MODE_EXPLORER, bool draw_screen = true, C_mode_file_browser::t_OpenCallback OpenCallback = NULL,
      E_TEXT_ID title = TXT_NULL, const wchar *init_path = NULL, dword op_flags = GETDIR_DIRECTORIES | GETDIR_FILES, const char *ext_filter = NULL, C_file_browser_callback *fbcb = NULL, bool _del_open_cb = false);
   C_mode_file_browser &SetModeFileBrowser_GetSaveFileName(const Cstr_w &proposed_dst_name, C_mode_file_browser::t_OpenCallback OpenCallback, E_TEXT_ID title = TXT_SAVE_AS, const wchar *init_path = NULL, C_file_browser_callback *fbcb = NULL, bool _del_open_cb = false);
   bool SetModeFileBrowser_Archive(const wchar *filename, const wchar *show_name, const C_zip_package *in_arch = NULL);

   void FileBrowser_GoToPath(C_mode_file_browser &mod, const wchar *path, bool expand);

   C_smart_ptr<C_image> CreateImageThumbnail(C_mode_file_browser &mod, const wchar *fn);

   void FileBrowser_SaveLastPath(C_mode_file_browser &mod);
   void FileBrowser_Close(C_mode_file_browser &mod);
   void FileBrowser_EnterSaveAsFilename(C_mode_file_browser &mod);
   static bool FileBrowser_IsFileMarked(const C_mode_file_browser &mod, const C_mode_file_browser::S_entry &e, int *sel_indx = NULL);
   static void FileBrowser_MarkCurrFile(C_mode_file_browser &mod, bool toggle, bool mark);
   static void FileBrowser_MarkOnSameLevel(C_mode_file_browser &mod);
   static void FileBrowser_CleanMarked(C_mode_file_browser &mod);

   void FileBrowser_ShowDetails(C_mode_file_browser &mod);
   void FileBrowser_DrawIcon(const C_mode_file_browser &mod, int x, int y, dword index, C_fixed alpha) const;

   static bool GetDiskSpace(C_mode_file_browser::S_entry &e, dword &free_lo, dword &free_hi, dword &total_lo, dword &total_hi);

//----------------------------
// Get root names of all directories (e'g' "C:"), not including last '\'
   void GetAllDrives(C_mode_file_browser &mod, C_vector<C_mode_file_browser::S_entry> &entries, bool allow_rom_drives, bool allow_ram_drives) const;

   bool FileBrowser_CollapseDir(C_mode_file_browser &mod, int indx = -1);
   static void FileBrowser_CancelRename(C_mode_file_browser &mod);

//----------------------------
// Delete selected file or dir (including all sub-dirs).
   void FileBrowser_DeleteSelected();

//----------------------------
// Begin deletion - ask question.
   void FileBrowser_BeginDelete(C_mode_file_browser &mod);

   void FileBrowser_BeginMakeNewFile(C_mode_file_browser &mod, bool dir);
   static int FileBrowser_GetDriveEntry(C_mode_file_browser &mod, int indx = -1);

   static void FileBrowser_UpdateDriveInfo(C_mode_file_browser &mod, int indx = -1);

   void FileBrowser_FinishGetSaveFileName(C_mode_file_browser &mod, bool ask_overwrite = true);

   bool CopyFile(C_file &fl_src, const wchar *src_name, const wchar *dst, E_TEXT_ID dlg_title = TXT_COPYING, int total_prog_size = -1, int total_prog_pos = -1);

//----------------------------
// Perform action for Open or Select.
   bool FileBrowser_OpenFile(C_mode_file_browser &mod);

   bool FileBrowser_ExpandDir(C_mode_file_browser &mod, int indx = -1);

#if defined MAIL || defined WEB || defined EXPLORER
   void CollectCopyOrMoveFolder(C_mode_file_browser &mod, C_mode_long_operation &mod_lop, const wchar *src_base, const wchar *src_name, const wchar *dst);

   void FileBrowser_CopyMove(C_mode_file_browser &mod, bool move, bool ask_dest = true);
   void FileBrowser_ExtractFile(C_mode_file_browser &mod);
   bool FileBrowser_ExtractCallback(const Cstr_w *file, const C_vector<Cstr_w> *files);

   bool FileBrowser_SendBy(C_mode_file_browser &mod, C_obex_file_send::E_TRANSFER);
   void FileBrowser_SetButtonsState(C_mode_file_browser &mod) const;
#endif

#ifdef EXPLORER
   static void FileBrowser_RefreshEntries1(void *c);
   bool FileBrowser_OpenInHexViewer(C_mode_file_browser &mod);

//----------------------------
   bool FileBrowserEditFile(C_mode_file_browser &mod, bool is_new_file);

   void StartCreateZip(C_mode_file_browser &mod, bool move);

   void MakeZip(const Cstr_w &fn, bool move);
   bool CopyFilesToZip(const Cstr_w *file, const C_vector<Cstr_w> *files){ MakeZip(*file, false); return true; }
   bool MoveFilesToZip(const Cstr_w *file, const C_vector<Cstr_w> *files){ MakeZip(*file, true); return true; }
#endif//EXPLORER

public:
   void FileBrowser_CollectMarkedOrSelectedFiles(C_mode_file_browser &mod, C_vector<Cstr_w> &filenames, bool add_folders = false, bool folders_first = true) const;

   static C_client_file_mgr *GetThis(C_client *cl){ return (C_client_file_mgr*)cl; }
//----------------------------
// Refresh entries - check if all exist, revise folders contents.
   void FileBrowser_RefreshEntries(C_mode_file_browser &mod);

   static bool CopyFile(C_client *cl, C_file &fl_src, const wchar *src_name, const wchar *dst, E_TEXT_ID dlg_title = TXT_COPYING, int total_prog_size = -1, int total_prog_pos = -1){
      return GetThis(cl)->CopyFile(fl_src, src_name, dst, dlg_title, total_prog_size, total_prog_pos);
   }
   enum{                      //flags used in op_flags
      GETDIR_DIRECTORIES = 1,
      GETDIR_FILES = 2,
      GETDIR_HIDDEN = 4,
      GETDIR_SYSTEM = 8,      //include Sys, private, and other system folders
      GETDIR_CHECK_IF_ANY = 0x10,//when set, func returns true if any searched entry is found (without writing to 'entries'), otherwise func returns all found entries
      OP_FLAGS_TOUCH_FRIENDLY = 0x20,
   };
//----------------------------
// Get directory contents.
//    dir ... dir of which to get contents
//    entries ... entries filled with data when returned
//    ext_filter ... null-terminated strings of extensions which to add for files
   static bool FileBrowser_GetDirectoryContents(const C_mode_file_browser *mod, const wchar *dir, const C_mode_file_browser::S_entry *e, C_vector<C_mode_file_browser::S_entry> *entries,
      dword flags, const char *ext_filter = NULL);
   static bool FileBrowser_GetArchiveContents(const C_mode_file_browser &mod, const wchar *dir, C_vector<C_mode_file_browser::S_entry> &entries, dword flags);

//----------------------------
// Get directory statistics.
// If returns false, it was canceled by user.
   static bool FileBrowser_GetStatistics(const C_mode_file_browser &mod, const wchar *dir, dword &num_d, dword &num_f, longlong &total_size);

//----------------------------
// Delete dir including all its contents. 'dir' cannot end by '\'.
   static bool DeleteDirRecursively(const wchar *dir, const C_mode_file_browser *mod);

//----------------------------
// Get full name of directory or currently selected entry.
   static void FileBrowser_GetFullName(const C_mode_file_browser &mod, Cstr_w &name, int indx = -1, bool force_real_name = true);

   static bool CheckExtensionMatch(const wchar *fname, const char *exts);

//----------------------------
// Refresh entries - check if all exist, revise folders contents.
   static void FileBrowser_RefreshEntries(C_client *cl, C_mode_file_browser &mod){
      GetThis(cl)->FileBrowser_RefreshEntries(mod);
   }
//----------------------------
// Get index of entry which represents selected dir.
// If dir or drive is selected, it returns selection, for files it returns entry in which the file resides.
   static int FileBrowser_GetDirEntry(C_mode_file_browser &mod, int indx = -1);

   static bool FileBrowser_ExpandDir(C_client *cl, C_mode_file_browser &mod, int indx = -1){
      return GetThis(cl)->FileBrowser_ExpandDir(mod, indx);
   }
   static void FileBrowser_CollectFolderFiles(C_mode_file_browser &mod, const wchar *folder, C_vector<Cstr_w> &filenames, bool add_folders = false, bool folders_first = true);

   static C_mode_file_browser &SetModeFileBrowser(C_client *cl, C_mode_file_browser::E_MODE mode = C_mode_file_browser::MODE_EXPLORER, bool draw_screen = true, C_mode_file_browser::t_OpenCallback OpenCallback = NULL,
      E_TEXT_ID title = TXT_NULL, const wchar *init_path = NULL, dword op_flags = GETDIR_DIRECTORIES | GETDIR_FILES, const char *ext_filter = NULL, C_file_browser_callback *fbcb = NULL, bool _del_open_cb = false){
      return GetThis(cl)->SetModeFileBrowser(mode, draw_screen, OpenCallback, title, init_path, op_flags, ext_filter, fbcb, _del_open_cb);
   }
   static void SetModeFileBrowser_GetSaveFileName(C_client *cl, const Cstr_w &proposed_dst_name, C_mode_file_browser::t_OpenCallback OpenCallback, E_TEXT_ID title = TXT_SAVE_AS, const wchar *init_path = NULL, C_file_browser_callback *fbcb = NULL, bool _del_open_cb = false){
      GetThis(cl)->SetModeFileBrowser_GetSaveFileName(proposed_dst_name, OpenCallback, title, init_path, fbcb, _del_open_cb);
   }
   static bool SetModeFileBrowser_Archive(C_client *cl, const wchar *filename, const wchar *show_name, const C_zip_package *in_arch = NULL){
      return GetThis(cl)->SetModeFileBrowser_Archive(filename, show_name, in_arch);
   }
   static void FileBrowser_GoToPath(C_client *cl, C_mode_file_browser &mod, const wchar *path, bool expand){
      GetThis(cl)->FileBrowser_GoToPath(mod, path, expand);
   }
   static void FileBrowser_SaveLastPath(C_client *cl, C_mode_file_browser &mod){
      GetThis(cl)->FileBrowser_SaveLastPath(mod);
   }
   static void FileBrowser_Close(C_client *cl, C_mode_file_browser &mod){
      GetThis(cl)->FileBrowser_Close(mod);
   }
};

//----------------------------
#endif

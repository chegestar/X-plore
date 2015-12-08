#ifdef __SYMBIAN32__
#include <E32Std.h>
#include <F32File.h>
#include <EikEnv.h>
#endif

#include "Main.h"
#include "Explorer\Main_Explorer.h"
#include <Ui\MultiQuestion.h>
#include <UI\TextEntry.h>
#include "BtSend.h"
#include <UI\About.h>

#include "Viewer.h"
#include "FileBrowser.h"
#include <Directory.h>

//----------------------------

#define LEVEL_OFFSET (expand_box_size/2+1 + 1)

//----------------------------
// Check if filename is contained in specified folder.
static bool IsFileNameInFolder(const wchar *fn, const wchar *folder){

   int folder_len = StrLen(folder);
   if(folder[folder_len-1]=='\\')
      --folder_len;
   return (fn[folder_len]=='\\' && !MemCmp(fn, folder, folder_len*sizeof(wchar)));
}

//----------------------------

static inline bool IsDirectory(const Cstr_w &fn){
   dword len = fn.Length();
   return (!len || fn[len-1]=='\\');
}

//----------------------------

C_client_file_mgr::C_mode_file_browser::C_mode_file_browser(C_client_file_mgr &_app, E_MODE fbm, E_TEXT_ID _tit, t_OpenCallback oc, dword opf, C_file_browser_callback *cb, bool _del_open_cb):
   C_mode_list<C_client_file_mgr>(_app),
   browser_mode(fbm),
   title_touch_down(false),
   title(_tit),
   _OpenCallback(oc),
   open_callback(cb),
   del_open_cb(_del_open_cb),
   op_flags(opf),
   timer_need_refresh(false),
   timer_file_details(false),
   //cut_mode(false),
   flags(FLG_SAVE_LAST_PATH | FLG_ALLOW_RENAME | FLG_ALLOW_DELETE | FLG_ALLOW_MAKE_DIR |
   FLG_ALLOW_DETAILS |
#if defined MAIL || defined WEB || defined EXPLORER
   FLG_ALLOW_SHOW_SUBMENU | FLG_ALLOW_SEND_BY | FLG_ALLOW_COPY_MOVE | FLG_ALLOW_ATTRIBUTES |
#endif
   FLG_ALLOW_MARK_FILES | FLG_ACCEPT_FILE | FLG_ACCEPT_DIR),
   shift_mark(-1),
   drag_mark(-1),
   //shift_release_toggle_mark(false),
#ifdef AUDIO_PREVIEW
   audio_preview(
#ifdef MAIL
   ((C_mail_client::S_config_mail&)(reinterpret_cast<C_mail_client&>(_app).config)).audio_volume
#elif defined EXPLORER
   ((C_explorer_client::S_config_xplore&)(reinterpret_cast<C_explorer_client&>(_app).config)).audio_volume
#else
#error
#endif
   ),
#endif
   expand_box_size(0)
{
   mode_id = C_client_file_mgr::ID;
}

//----------------------------

C_client_file_mgr::C_mode_file_browser::~C_mode_file_browser(){
   if(del_open_cb)
      delete open_callback;
}

//----------------------------

void C_client_file_mgr::FileBrowser_GetFullName(const C_mode_file_browser &mod, Cstr_w &name, int indx, bool force_real_name){

   if(!mod.entries.size())
      return;
   if(indx==-1)
      indx = mod.selection;
   const C_mode_file_browser::S_entry &e = mod.entries[indx];
   if(e.type!=e.ARCHIVE){
      name = e.name;
   }else{
      if(!e.level){
         if(force_real_name)
            name = L"";
         else
            name = e.name;
      }
   }
   int level = e.level;
   while(level--){
                              //find upper level
      while(indx--){
         const C_mode_file_browser::S_entry &e1 = mod.entries[indx];
         if(e1.level == level){
            if(e1.type!=e1.ARCHIVE){
               Cstr_w n;
               {
                  n<<e1.name;
               }
               n<<L'\\' <<name;
               name = n;
            }
            break;
         }
      }
                              //not found?
      if(indx==-1)
         break;
   }
}

//----------------------------

C_client_file_mgr::C_mode_file_browser &C_client_file_mgr::SetModeFileBrowser(C_mode_file_browser::E_MODE fbm, bool draw_screen, C_mode_file_browser::t_OpenCallback OpenCallback, E_TEXT_ID title,
   const wchar *init_path, dword getdir_glags, const char *ext_filter, C_file_browser_callback *fbcb, bool _del_open_cb){

   C_mode_file_browser &mod = *new(true) C_mode_file_browser(*this, fbm, title, OpenCallback, getdir_glags, fbcb, _del_open_cb);
   ActivateMode(mod, false);

   mod.extension_filter = ext_filter;
   if(config.flags&config.CONF_BROWSER_SHOW_HIDDEN)
      mod.op_flags |= GETDIR_HIDDEN;

#ifdef EXPLORER
   if(config.flags&config.CONF_BROWSER_SHOW_SYSTEM)
      mod.op_flags |= GETDIR_SYSTEM;
#else
   mod.op_flags |= GETDIR_SYSTEM;
#endif
   if(!HasMouse())
      mod.op_flags &= ~OP_FLAGS_TOUCH_FRIENDLY;

   mod.InitLayout();

   if(mod.browser_mode!=mod.MODE_ARCHIVE_EXPLORER){
      GetAllDrives(mod, mod.entries, (config.flags&config.CONF_BROWSER_SHOW_ROM),
#ifdef EXPLORER
         (config.flags&config.CONF_BROWSER_SHOW_RAM)
#else
         true
#endif
         );
      for(int i=0; i<mod.entries.size(); i++){
         C_mode_file_browser::S_entry &en = mod.entries[FileBrowser_GetDriveEntry(mod, i)];
         switch(en.type){
         case C_mode_file_browser::S_entry::DRIVE:
            if(!FileBrowser_GetDirectoryContents(NULL, en.name, &en, NULL, mod.op_flags | GETDIR_CHECK_IF_ANY, mod.extension_filter))
               en.expand_status = en.NEVER_EXPAND;
                              //flow...
         case C_mode_file_browser::S_entry::DIR:
            FileBrowser_UpdateDriveInfo(mod, i);
            break;
         }
      }
      mod.sb.total_space = mod.entries.size()*mod.entry_height;
      mod.sb.SetVisibleFlag();

                                 //set selection to initial path
      if(!init_path || !*init_path)
         init_path = config.last_browser_path;
      //LOG_RUN(Cstr_w(init_path).ToUtf8());
      bool expand = (*init_path=='*');
      init_path += int(expand);
      FileBrowser_GoToPath(mod, init_path, expand);
#if defined USE_MOUSE && (defined MAIL || defined EXPLORER)
      if(HasMouse())
         FileBrowser_SetButtonsState(mod);
#endif
   }
   if(draw_screen)
      RedrawScreen();
   return mod;
}

//----------------------------

void C_client_file_mgr::FileBrowser_DrawIcon(const C_mode_file_browser &mod, int x, int y, dword index, C_fixed alpha) const{

   S_rect rc(0, 0, 0, mod.icons->SizeY());
   rc.x = rc.sy*index*27/22;
   rc.sx = rc.sy*(index+1)*27/22;
   rc.sx -= rc.x;
   mod.icons->DrawSpecial(x, y, &rc, alpha);
}

//----------------------------

void C_client_file_mgr::FileBrowser_GoToPath(C_mode_file_browser &mod, const wchar *path, bool expand){

                              //collapse all top-level entries first
   for(int i=0; i<mod.entries.size(); i++){
      C_mode_file_browser::S_entry &e = mod.entries[i];
      if(e.expand_status==e.EXPANDED)
         FileBrowser_CollapseDir(mod, i);
   }
   mod.sb.total_space = mod.entries.size()*mod.entry_height;
   mod.sb.SetVisibleFlag();

   if(*path){
      Cstr_w tmp = path;
      wchar *cp = &tmp.At(0), *np;
      for(np = cp+1; *np; ++np){
         if(*np=='\\'){
            *np++ = 0;
            break;
         }
      }
                           //find drive first
      int level = 0;
      for(int i=mod.entries.size(); i--; ){
         if(!text_utils::CompareStringsNoCase(mod.entries[i].name, cp)){
            mod.selection = i;
                           //open paths now
            for(cp = np; *cp; cp = np){
               for(np = cp; *np; ++np){
                  if(*np=='\\'){
                     *np++ = 0;
                     break;
                  }
               }
               if(!FileBrowser_ExpandDir(mod))
                  break;
               ++level;
               while(++i<mod.entries.size() && mod.entries[i].level==level){
                  if(!text_utils::CompareStringsNoCase(mod.entries[i].name, cp)){
                     mod.selection = i;
                     break;
                  }
               }
               cp = np;
               if(i==mod.entries.size())
                  break;
            }
            if(expand)
               FileBrowser_ExpandDir(mod);
            mod.ChangeSelection(mod.selection);
            mod.EnsureVisible();
            break;
         }
      }
   }
}

//----------------------------
#ifdef AUDIO_PREVIEW

void C_client_file_mgr::C_mode_file_browser::LayoutAudioPreview(){

   const S_entry &e = entries[selection];
   int y_pos = selection*entry_height - sb.pos;
   int BUTTON_SZ = app.fdb.line_spacing*10/4;
   const int PROGRESS_SZ = app.fds.line_spacing/2;

   int rc_x = this->rc.x + expand_box_size + 2 + Min(e.level, int(S_entry::MAX_DRAW_LEVEL)) * LEVEL_OFFSET + app.fdb.cell_size_x;
   int rc_sx = BUTTON_SZ*audio_preview.BUTTON_LAST + PROGRESS_SZ;
   if(rc_x+rc_sx > rc.Right()-2){
      audio_preview.rc.sx = rc.Right()-2 - rc_x;
      BUTTON_SZ = (audio_preview.rc.sx-PROGRESS_SZ)/audio_preview.BUTTON_LAST;
      rc_sx = BUTTON_SZ*audio_preview.BUTTON_LAST + PROGRESS_SZ;
   }
   audio_preview.rc = S_rect(rc_x, 0, rc_sx, BUTTON_SZ);
   audio_preview.rc.sy = BUTTON_SZ + PROGRESS_SZ;

   audio_preview.rc.y = this->rc.y + y_pos+entry_height + app.fdb.line_spacing/8;
   if(audio_preview.rc.Bottom() > this->rc.Bottom()-app.fdb.line_spacing/4)
      audio_preview.rc.y -= audio_preview.rc.sy + entry_height + app.fdb.line_spacing/2;

   audio_preview.progress.rc = audio_preview.rc;
   audio_preview.progress.rc.sy = PROGRESS_SZ;
   audio_preview.progress.rc.y = audio_preview.rc.Bottom()-audio_preview.progress.rc.sy;
   audio_preview.progress.rc.Compact(PROGRESS_SZ/4);

   audio_preview.rc_volbar = audio_preview.rc;
   audio_preview.rc_volbar.sx = PROGRESS_SZ;
   audio_preview.rc_volbar.sy = BUTTON_SZ;
   audio_preview.rc_volbar.x = audio_preview.rc.Right()-audio_preview.rc_volbar.sx;
   audio_preview.rc_volbar.Compact(PROGRESS_SZ/4);

   S_rect brc(audio_preview.rc.x, audio_preview.rc.y, BUTTON_SZ, BUTTON_SZ);
   brc.Compact(app.fdb.cell_size_x/4);
   for(int i=0; i<audio_preview.BUTTON_LAST; i++){
      C_button &but = audio_preview.buttons[i];
      but.SetRect(brc);
      brc.x += BUTTON_SZ;
   }
}

//----------------------------

void C_client_file_mgr::C_mode_file_browser::AudioPreviewEnable(){
                              //check if file has real name
   if(!arch || arch->GetArchiveType()>=1000){
         //activate audio preview
      audio_preview.active = true;
      audio_preview.err = false;
      LayoutAudioPreview();
      DeleteTimer();
   }
}

//----------------------------

void C_client_file_mgr::C_mode_file_browser::AudioPreviewTogglePlay(){
   if(!audio_preview.plr){
      Cstr_w fn;
      app.FileBrowser_GetFullName(*this, fn);
      if(arch && arch->GetArchiveType()>=1000){
         C_archive_simulator &as = (C_archive_simulator&)*arch;
         fn = as.GetRealFileName(fn);
      }
         
      audio_preview.plr = C_simple_sound_player::Create(fn, audio_preview.volume);
      if(audio_preview.plr){
         audio_preview.plr->Release();
         audio_preview.paused = false;
         audio_preview.progress.pos = 0;
         audio_preview.progress.total = audio_preview.plr->GetPlayTime();
         app.CreateTimer(*this, 100);
      }else
         audio_preview.err = true;
   }else{
      audio_preview.paused = !audio_preview.paused;
      if(audio_preview.paused){
         audio_preview.plr->Pause();
      }else{
         audio_preview.plr->Resume();
      }
      /*
            //toggle playback
      if(audio_preview.plr->IsDone()){
         audio_preview.plr->SetPlayPos(0);
         
      }
      */
   }
}

//----------------------------

bool C_client_file_mgr::C_mode_file_browser::ChangeAudioPreviewVolume(bool up){

   byte nv = audio_preview.volume;
   if(up){
      if(nv<10) ++nv;
   }else
      if(nv>1) --nv;
   if(audio_preview.volume!=nv){
      audio_preview.volume = nv;
      app.SaveConfig();
      if(audio_preview.plr)
         audio_preview.plr->SetVolume(nv);
      return true;
   }
   return false;
}

#endif
//----------------------------

void C_client_file_mgr::C_mode_file_browser::ScrollChanged(){
   C_mode_list<C_client_file_mgr>::ScrollChanged();
#ifdef AUDIO_PREVIEW
   if(audio_preview.active)
      LayoutAudioPreview();
#endif
}

//----------------------------

void C_client_file_mgr::C_mode_file_browser::InitLayout(){

   entry_height = app.fdb.line_spacing;
   if(op_flags&OP_FLAGS_TOUCH_FRIENDLY)
      entry_height = entry_height*2;
   {
      icons = C_image::Create(app);
      icons->Release();
      icons->Open(L"icons_file.png", 0, app.fdb.line_spacing*30/32, app.CreateDtaFile());
      icon_sx = icons->SizeX()/13;
   }
   const int border = 1;
   int top = app.GetTitleBarHeight()-1;
   if(title!=TXT_NULL)
      top += app.fdb.line_spacing;
   rc = S_rect(border, top, app.ScrnSX()-border*2, app.ScrnSY()-top-app.GetSoftButtonBarHeight()-border);

   expand_box_size = app.fdb.line_spacing/3;
   if(op_flags&OP_FLAGS_TOUCH_FRIENDLY)
      expand_box_size = app.fdb.line_spacing/2;
   expand_box_size = Max(5ul, expand_box_size|1);
   
                              //align to whole lines
   int num_lines = rc.sy / entry_height;
   rc.sy = num_lines * entry_height;

   sb.visible_space = num_lines*entry_height;
   sb.SetVisibleFlag();
   const int sb_width = app.GetScrollbarWidth();
   sb.rc = S_rect(rc.Right()-sb_width-1, rc.y+3, sb_width, rc.sy-6);

   EnsureVisible();
#ifdef AUDIO_PREVIEW
   if(audio_preview.active)
      LayoutAudioPreview();
#endif
   if(te_rename){
      S_rect trc = te_rename->GetRect();
      trc.y = rc.y + 1 + entry_height*selection - sb.pos;
      te_rename->SetRect(trc);
   }
}

//----------------------------

C_client_file_mgr::C_mode_file_browser &C_client_file_mgr::SetModeFileBrowser_GetSaveFileName(const Cstr_w &proposed_dst_name,
   C_mode_file_browser::t_OpenCallback OpenCallback, E_TEXT_ID title, const wchar *init_path, C_file_browser_callback *fbcb, bool _del_open_cb){

   C_mode_file_browser &mod = SetModeFileBrowser(C_mode_file_browser::MODE_GET_SAVE_FILENAME, true, OpenCallback, title, init_path, GETDIR_DIRECTORIES|GETDIR_FILES,
      NULL, fbcb, _del_open_cb);
   mod.flags = mod.FLG_ACCEPT_DIR | mod.FLG_ACCEPT_FILE | mod.FLG_ALLOW_DELETE | mod.FLG_ALLOW_DETAILS | mod.FLG_ALLOW_MAKE_DIR | mod.FLG_ALLOW_RENAME |
#if defined MAIL || defined WEB || defined EXPLORER
      mod.FLG_ALLOW_SHOW_SUBMENU |
#endif
      mod.FLG_SAVE_LAST_PATH | mod.FLG_SELECT_OPTION;
   mod.save_as_name = proposed_dst_name;
   return mod;
}

//----------------------------
#if defined MAIL || defined EXPLORER

bool C_client_file_mgr::SetModeFileBrowser_Archive(const wchar *arch_name, const wchar *view_name, const C_zip_package *in_arch){

   C_zip_package *arch = NULL;
   Cstr_w ext = text_utils::GetExtension(arch_name);
   ext.ToLower();
   dword in_arch_file_size = 0;
#ifdef EXPLORER
   if(ext==L"rar"){
      if(in_arch){
         C_file *fp = in_arch->OpenFile(arch_name);
         if(!fp)
            return false;
         in_arch_file_size = fp->GetFileSize();
         arch = CreateRarPackage(fp, true);
         if(!arch)
            delete fp;
      }else
         arch = CreateRarPackage(arch_name);
   }else
   if(!StrCmp(arch_name, L"*msgs*")){
      arch = CreateMessagingArchive();
   }else
#endif
   if(ext==L"zip" || ext==L"jar")
   {
      if(in_arch){
         C_file *fp = in_arch->OpenFile(arch_name);
         if(!fp)
            return false;
         in_arch_file_size = fp->GetFileSize();
         arch = C_zip_package::Create(fp, true);
      }else
         arch = C_zip_package::Create(arch_name);
   }
   if(!arch)
      return false;
   if(mode && mode->Id()==C_client_file_mgr::ID)
      FileBrowser_SaveLastPath((C_mode_file_browser&)*mode);

   C_mode_file_browser &mod = SetModeFileBrowser(C_mode_file_browser::MODE_ARCHIVE_EXPLORER, false);
   mod.flags &= ~(mod.FLG_SAVE_LAST_PATH | mod.FLG_ALLOW_DELETE | mod.FLG_ALLOW_MAKE_DIR | mod.FLG_ALLOW_RENAME | mod.FLG_ALLOW_ATTRIBUTES |
#if defined MAIL || defined WEB || defined EXPLORER
      mod.FLG_ALLOW_SEND_BY | mod.FLG_ALLOW_SHOW_SUBMENU | mod.FLG_ALLOW_COPY_MOVE |
#endif
      0);
#ifdef EXPLORER
   if(arch->GetArchiveType()==1000){
      mod.flags |= mod.FLG_ALLOW_COPY_MOVE | mod.FLG_ALLOW_DELETE | mod.FLG_ALLOW_DETAILS;
      C_archive_simulator &as = (C_archive_simulator&)*arch;
      as.SetChangeObserver(C_client_file_mgr::FileBrowser_RefreshEntries1, this);
   }
#endif
   mod.arch.SetPtr(arch);
   arch->Release();
   if(!in_arch)
      mod.ck_arch.Open(arch_name);
   {
      C_mode_file_browser::S_entry e;
      e.type = e.ARCHIVE;
      e.name = view_name;
      e.level = 0;
      if(!in_arch && mod.ck_arch.GetMode())
         e.size.file_size = mod.ck_arch.GetFileSize();
      else
         e.size.file_size = in_arch_file_size;
#ifdef UNIX_FILE_SYSTEM
      e.atts = C_file::ATT_ACCESS_READ;
#endif
      mod.entries.push_back(e);

      mod.sb.total_space += mod.entry_height;
      mod.sb.SetVisibleFlag();
      mod.selection = 0;
      mod.EnsureVisible();
   }
                              //expand top level
   FileBrowser_ExpandDir(mod);
#if defined USE_MOUSE && (defined MAIL || defined EXPLORER)
   if(HasMouse())
      FileBrowser_SetButtonsState(mod);
#endif
   RedrawScreen();
   return true;
}

#endif
//----------------------------

bool C_client_file_mgr::FileBrowser_GetArchiveContents(const C_mode_file_browser &mod, const wchar *dir, C_vector<C_mode_file_browser::S_entry> &entries, dword flags){

   dword dir_l = StrLen(dir);

   const wchar *cp;
   dword file_size;
   void *h = mod.arch->GetFirstEntry(cp, file_size);
   if(h) do{
      Cstr_w fnw = cp;
      dword fs = fnw.Length();
      if(fs<=dir_l)
         continue;
      const wchar *fn = fnw;
      if(MemCmp(fn, dir, dir_l*2))
         continue;
                              //this file is inside of 'dir', check if it's file or another dir
      if(dir_l){
         if(fn[dir_l]!='\\')
            continue;
         fn += dir_l + 1;
         fs -= dir_l + 1;
         assert(fn[-1]=='\\');
                              //ignore directory entries
         if(!fs)
            continue;
      }
      dword i;
      for(i=0; i<fs; i++){
         if(fn[i]=='\\')
            break;
      }
      C_mode_file_browser::S_entry e;
      MemSet(&e.modify_date, 0, sizeof(e.modify_date));
      if(fn[i]=='\\'){
         if(!(flags&GETDIR_CHECK_IF_ANY)){
                              //sub-dir, check if it's not already there
            int ei;
            for(ei=entries.size(); ei--; ){
               const C_mode_file_browser::S_entry &e1 = entries[ei];
               if(e1.name.Length()==i && !MemCmp(e1.name, fn, i*2))
                  break;
            }
            if(ei!=-1)
               continue;
         }
         e.type = e.DIR;
         e.name.Allocate(fn, i);
      }else{
                              //it's file, add
         e.type = e.FILE;
         e.size.file_size = file_size;
         mod.arch->GetFileWriteTime(fnw, e.modify_date);
         e.name = fn;
      }
      if(flags&GETDIR_CHECK_IF_ANY)
         return true;
#ifdef UNIX_FILE_SYSTEM
      e.atts = C_file::ATT_ACCESS_READ;
#endif
      entries.push_back(e);
   }while((h = mod.arch->GetNextEntry(h, cp, file_size), h));
   return false;
}

//----------------------------

int C_client_file_mgr::C_mode_file_browser::CompareEntries(const void *m1, const void *m2, void *context){

   S_entry &e1 = *(S_entry*)m1;
   S_entry &e2 = *(S_entry*)m2;
                              //first by type (drive, dir, file)
   if(e1.type != e2.type)
      return (e1.type < e2.type) ? -1 : 1;
                              //then by name
   return text_utils::CompareStringsNoCase(e1.name, e2.name);
}

//----------------------------

void C_client_file_mgr::C_mode_file_browser::SwapEntries(void *m1, void *m2, dword w, void *context){

   if(m1!=m2){
      S_entry &e1 = *(S_entry*)m1;
      S_entry &e2 = *(S_entry*)m2;
      Swap(e1, e2);
   }
}

//----------------------------

void C_client_file_mgr::C_mode_file_browser::ChangeSelection(int sel){

   selection = sel;
   img_thumbnail = NULL;
#ifdef AUDIO_PREVIEW
   audio_preview.Close();
#endif
   {
      DeleteTimer();
      app.CreateTimer(*this, 1000);
      timer_file_details = true;
      if(((C_mode*)app.mode)!=this)
         timer->Pause();
   }
}

//----------------------------

void C_client_file_mgr::FileBrowser_SaveLastPath(C_mode_file_browser &mod){

   if(mod.arch)
      return;
                              //save last path
   Cstr_w &dst = config.last_browser_path;
   dst.Clear();
   if(mod.entries.size()){
      Cstr_w path;
      FileBrowser_GetFullName(mod, path);
      assert(path.Length() < 256);
      const C_mode_file_browser::S_entry &e = mod.entries[mod.selection];
      if(e.expand_status==e.EXPANDED)
         dst<<'*';
      dst<<path;
   }
   SaveConfig();
}

//----------------------------

void C_client_file_mgr::FileBrowser_Close(C_mode_file_browser &mod){

   if(mod.flags&mod.FLG_SAVE_LAST_PATH)
      FileBrowser_SaveLastPath(mod);
   if(mod.saved_parent){
      mod.Close(false);
      if(mode->Id()==C_client_file_mgr::ID){
         C_mode_file_browser &mod_fb = (C_mode_file_browser&)*mode;
         FileBrowser_RefreshEntries(mod_fb);
      }
      RedrawScreen();
   }else{
      {
         CloseMode(mod);
         Exit();
      }
   }
}

//----------------------------

bool C_client_file_mgr::FileBrowser_ExpandDir(C_mode_file_browser &mod, int indx){

   if(indx==-1)
      indx = mod.selection;
   if(!mod.entries.size())
      return false;
   C_mode_file_browser::S_entry &e = mod.entries[indx];
   if(e.type==e.FILE || e.expand_status!=e.COLLAPSED)
      return false;
   e.expand_status = e.EXPANDED;

   C_vector<C_mode_file_browser::S_entry> entries;
   Cstr_w full_name;
   FileBrowser_GetFullName(mod, full_name, indx);
   assert(indx!=-1);
   FileBrowser_GetDirectoryContents(&mod, full_name, &mod.entries[indx], &entries, mod.op_flags, mod.extension_filter);

   int i = entries.size();
   if(i){
                              //assign level & for dirs, determine if they contain sub-contents
      while(i--){
         C_mode_file_browser::S_entry &ne = entries[i];
         ne.level = e.level + 1;
         if(ne.type==ne.DIR){
            Cstr_w n;
            if(full_name.Length()){
               n<<full_name <<L"\\";
            }
            n<<ne.name;
            if(!FileBrowser_GetDirectoryContents(&mod, n, &ne, NULL, mod.op_flags | GETDIR_CHECK_IF_ANY, mod.extension_filter))
               ne.expand_status = e.NEVER_EXPAND;
         }
      }
      QuickSort(entries.begin(), entries.size(), sizeof(C_mode_file_browser::S_entry),
#ifdef EXPLORER
         &C_explorer_client::CompareFileEntries, &((C_explorer_client::S_config_xplore&)config).sort_mode,
#else
         &mod.CompareEntries, &config.flags,
#endif
         &mod.SwapEntries);

                              //add to list
      mod.entries.insert(&mod.entries[indx+1], entries.begin(), entries.end());
      mod.sb.total_space += entries.size()*mod.entry_height;
      mod.sb.SetVisibleFlag();
                              //try to show most possible items of newly added list
      int want_lines = indx - mod.sb.pos/mod.entry_height + entries.size() - mod.sb.visible_space/mod.entry_height + 1;
      while(want_lines > 0 && mod.sb.pos/mod.entry_height < indx-1){
         mod.sb.pos += mod.entry_height;
         --want_lines;
      }
      if(mod.flags&mod.FLG_AUTO_COLLAPSE_DIRS){
                              //collapse all dirs except of this
         for(int j=0; j<mod.entries.size(); j++){
            C_mode_file_browser::S_entry &e1 = mod.entries[j];
            if(j!=mod.selection && e1.type!=e1.FILE && e1.expand_status==e1.EXPANDED){
               if(mod.browser_mode==mod.MODE_ARCHIVE_EXPLORER && !j)
                  continue;
               Cstr_w fn;
               FileBrowser_GetFullName(mod, fn, j);
               if(!IsFileNameInFolder(full_name, fn))
                  FileBrowser_CollapseDir(mod, j);
            }
         }
      }
      mod.EnsureVisible();
   }else{
      e.expand_status = e.NEVER_EXPAND;
   }
   return true;
}

//----------------------------

bool C_client_file_mgr::FileBrowser_CollapseDir(C_mode_file_browser &mod, int indx){

   if(indx==-1)
      indx = mod.selection;
   if(!mod.entries.size())
      return false;
                              //collapse dir
   C_mode_file_browser::S_entry &e = mod.entries[indx];
   if(e.type==e.FILE || e.expand_status!=e.EXPANDED)
      return false;
   e.expand_status = e.COLLAPSED;
                              //count all children (entries having level > than ours)
   int i;
   for(i=indx+1; i<mod.entries.size(); i++){
      if(mod.entries[i].level <= e.level)
         break;
   }
                              //remove from list
   mod.entries.erase(&mod.entries[indx+1], &mod.entries[i]);
   C_scrollbar &sb = mod.sb;
   sb.total_space -= (i-indx-1)*mod.entry_height;
   if(mod.selection>=i)
      mod.ChangeSelection(mod.selection-(i-indx-1));
   else
   if(mod.selection>indx)
      mod.ChangeSelection(indx);
                              //make sure top line is not too down
   sb.SetVisibleFlag();
   mod.EnsureVisible();
   return true;
}

//----------------------------

void C_client_file_mgr::C_mode_file_browser::BeginRename(){

   if(!(flags&FLG_ALLOW_RENAME))
      return;
   if(!entries.size())
      return;
   const S_entry &e = entries[selection];
   if(e.type==e.DRIVE)
      return;
   te_rename = app.CreateTextEditor(TXTED_ACTION_ENTER, UI_FONT_BIG, e.type==e.DIR ? FF_BOLD : 0, e.name, 200);
   C_text_editor &te = *te_rename;
   te.Release();
   if(e.type==e.FILE){
      const wchar *ext = text_utils::GetExtension(e.name);
      if(ext)
         te.SetCursorPos(e.name.Length()-StrLen(ext)-1, 0);
   }

   int x = rc.x + LEVEL_OFFSET*Min(e.level, int(S_entry::MAX_DRAW_LEVEL)) + icon_sx + app.fdb.letter_size_x/2 + expand_box_size + 1;
   te.SetRect(S_rect(x, rc.y + 1 + entry_height*selection-sb.pos, sb.rc.x - x - app.fdb.cell_size_x/2, app.fdb.cell_size_y+1));

   app.MakeSureCursorIsVisible(te);
}

//----------------------------

void C_client_file_mgr::FileBrowser_CancelRename(C_mode_file_browser &mod){

   if(!mod.te_rename)
      return;
   mod.te_rename = NULL;
   C_mode_file_browser::S_entry &e = mod.entries[mod.selection];
                     //if we canceled renaming of existing file/dir, do nothing,
                     // otherwise remove the entry
   if(!e.name.Length()){
      switch(mod.browser_mode){
      default:
                        //unnamed file/dir, had to be just created, delete
         mod.entries.erase(&e);
         mod.sb.total_space -= mod.entry_height;
         mod.sb.SetVisibleFlag();
         if(mod.selection){
            mod.ChangeSelection(mod.selection-1);
            C_mode_file_browser::S_entry &ex = mod.entries[mod.selection];
            Cstr_w name;
            FileBrowser_GetFullName(mod, name);
            if(!FileBrowser_GetDirectoryContents(&mod, name, &ex, NULL, mod.op_flags | GETDIR_CHECK_IF_ANY, mod.extension_filter))
               ex.expand_status = ex.NEVER_EXPAND;
         }
         mod.EnsureVisible();
         break;
      case C_mode_file_browser::MODE_GET_SAVE_FILENAME:
                           //remove proposed entry
         mod.entries.erase(&e);
         mod.sb.total_space -= mod.entry_height;
         mod.sb.SetVisibleFlag();
         assert(mod.selection);
         mod.ChangeSelection(mod.selection-1);
         mod.EnsureVisible();
         break;
      }
   }
}

//----------------------------

bool C_client_file_mgr::DeleteDirRecursively(const wchar *dir, const C_mode_file_browser *mod){

   C_vector<C_mode_file_browser::S_entry> entries;
   FileBrowser_GetDirectoryContents(mod, dir, NULL, &entries, GETDIR_FILES | GETDIR_DIRECTORIES | GETDIR_HIDDEN | GETDIR_SYSTEM);
   bool ret = true;

   for(int i=entries.size(); i--; ){
      const C_mode_file_browser::S_entry &e = entries[i];
      Cstr_w full_name;
      if(*dir){
         full_name<<dir;
         if(full_name[full_name.Length()-1]!='\\')
            full_name<<L'\\';
      }
      full_name <<e.name;
      if(e.type==e.FILE){
         bool ok;
#ifdef EXPLORER
         if(mod && mod->arch && mod->arch->GetArchiveType()>=1000)
            ok = ((C_archive_simulator&)*mod->arch).DeleteFile(full_name);
         else
#endif
            ok = C_file::DeleteFile(full_name);
         if(!ok)
            ret = false;
      }else{
         if(!DeleteDirRecursively(full_name, mod))
            ret = false;
      }
   }
   if(!*dir)
      return ret;
   return (C_dir::RemoveDirectory(dir) && ret);
}

//----------------------------

void C_client_file_mgr::C_mode_long_operation::InitLayout(){

   super::InitLayout();
   int sy = app.fdb.line_spacing*2;
   if(filenames.size()>1 && operation!=OPERATION_DELETE)
      sy *= 2;
   SetClientSize(S_point(0, sy));
   WindowMoved();
                              //redraw
   last_prog_1 = -1;
   last_prog_2 = -1;
}

//----------------------------

void C_client_file_mgr::C_mode_long_operation::WindowMoved(){

   //C_mode_info_message::WindowMoved();
   progress_1.rc = S_rect(rc.x+app.fdb.cell_size_x, rc.y+app.GetDialogTitleHeight()+app.fdb.line_spacing, rc.sx-app.fdb.cell_size_x*2, app.fdb.cell_size_y);

   progress_2.rc = progress_1.rc;
   progress_2.rc.y += app.fdb.line_spacing*2;
   last_prog_1 = -1;
   last_prog_2 = -1;
}

//----------------------------

void C_client_file_mgr::C_mode_long_operation::ProcessInput(S_user_input &ui, bool &redraw){

   switch(ui.key){
   case K_RIGHT_SOFT:
   case K_BACK:
   case K_ESC:
      {
#ifdef EXPLORER
         if(operation==OPERATION_ZIP){
            deflateEnd(&zs);
         }
#endif
         AddRef();
         C_mode_file_browser &mod = (C_mode_file_browser&)*GetParent();
         app.CloseMode(*this);
         mod.marked_files.clear();
         ((C_client_file_mgr&)app).FileBrowser_RefreshEntries(mod);
         Release();
      }
      return;
   }
   super::ProcessInput(ui, redraw);
}

//----------------------------
#ifdef EXPLORER

void C_client_file_mgr::FileBrowser_OverwriteConfirm(int sel){

   C_mode_long_operation &mod = (C_mode_long_operation&)*mode;
   mod.timer->Resume();
   switch(sel){
   case -1:
      {
                              //cancel operation
         CloseMode(mod, false);
         C_mode_file_browser &mod_fb = (C_mode_file_browser&)*mode;
         mod_fb.marked_files.clear();
         FileBrowser_RefreshEntries(mod_fb);
      }
      break;
   case 1: mod.overwrite_mode = mod.OVERWRITE_NO; break;
   case 0: mod.overwrite_mode = mod.OVERWRITE_YES; break;
   }
}

//----------------------------

void C_client_file_mgr::MakeZip(const Cstr_w &fn_, bool move){

   C_mode_file_browser &mod = (C_mode_file_browser&)*mode;
   FileBrowser_CleanMarked(mod);

   C_mode_long_operation &mod_lop = *new(true) C_mode_long_operation(*this, C_mode_long_operation::OPERATION_ZIP, GetText(TXT_CREATING_ZIP));
   mod_lop.zip_move = move;

   if(!mod.marked_files.size())
      FileBrowser_MarkCurrFile(mod, false, true);

                              //collect all files
   for(int i=0; i<mod.marked_files.size(); i++){
      const Cstr_w &fn = mod.marked_files[i];
      struct S_hlp{
         static void AddFile(C_mode_long_operation &_mod_lop, const Cstr_w &_fn, const Cstr_w &dir_base){
            if(IsDirectory(_fn)){
               C_dir d;
               if(d.ScanBegin(_fn)){
                  while(true){
                     dword attributes;
                     const wchar *name = d.ScanGet(&attributes);
                     if(!name)
                        break;
                     Cstr_w s;
                     s<<_fn <<name;
                     if(attributes&C_file::ATT_DIRECTORY)
                        s<<'\\';
                     AddFile(_mod_lop, s, dir_base);
                  }
               }
               if(_mod_lop.zip_move){
                  _mod_lop.zip_files.push_back(C_mode_long_operation::S_zip_file_info());
                  _mod_lop.filenames.push_back(_fn);
               }
            }else{
               C_mode_long_operation::S_zip_file_info fi;
               fi.rel_filename = _fn.RightFromPos(dir_base.Length());
               _mod_lop.zip_files.push_back(fi);
               _mod_lop.filenames.push_back(_fn);
            }
         }
      };
      Cstr_w base_path;
      if(IsDirectory(fn))
         base_path = file_utils::GetFilePath(fn.Left(fn.Length()-1));
      else
         base_path = file_utils::GetFilePath(fn);
      S_hlp::AddFile(mod_lop, fn, base_path);
   }
   {
                              //open the zip file
      Cstr_w fn = fn_;
      if(!text_utils::GetExtension(fn))
         fn<<L".zip";

      if(!mod_lop.fl_dst.Open(fn, C_file::FILE_WRITE)){
         Cstr_w s;
         s.Format(GetText(TXT_ERR_CANT_WRITE_FILE)) <<' ' <<fn;
         ShowErrorWindow(TXT_ERROR, s);
         mod_lop.Release();
         return;
      }
      mod_lop.fn_dst = fn;
   }
                              //prepare files, determine total size
   S_progress_display_help progress(this);
   progress.progress1.total = 0;
   mod_lop.progress_2.total = 0;
   for(int i=0; i<mod_lop.zip_files.size(); i++){
      const Cstr_w &fn = mod_lop.filenames[i];
      if(IsDirectory(fn))
         continue;
      C_mode_long_operation::S_zip_file_info &fi = mod_lop.zip_files[i];
      C_file fl_src;
      if(!fl_src.Open(fn)){
         Cstr_w s, fn1 = fn;
#ifdef UNIX_FILE_SYSTEM
         fn1 = file_utils::ConvertPathSeparatorsToUnix(fn1);
#endif
         s.Format(GetText(TXT_ERR_CANT_OPEN_FILE)) <<' ' <<fn1;
         ShowErrorWindow(TXT_ERROR, s);
         mod_lop.Release();
         return;
      }
      fi.filename_utf8.ToUtf8(fi.rel_filename);
                              //change slashes \\ -> /
      for(int j=fi.filename_utf8.Length(); j--; ){
         char &c = fi.filename_utf8.At(j);
         if(c=='\\')
            c = '/';
      }
      progress.progress1.total += fl_src.GetFileSize();
      mod_lop.progress_2.total += fl_src.GetFileSize();
   }
                              //temp buffers
   const dword TMP_SIZE = 0x4000;
   mod_lop.copy_buf.Resize(TMP_SIZE);
   mod_lop.compr_buf.Resize(TMP_SIZE);

   if(mod_lop.filenames.size()<=1)
      mod_lop.progress_2.total = -1;

   CreateTimer(mod_lop, 50);
   mod_lop.InitLayout();
   ActivateMode(mod_lop);
}


#endif
//----------------------------

bool C_client_file_mgr::C_mode_long_operation::DeleteFile(const wchar *fn, bool is_folder){

   bool ok;
   if(is_folder)
      ok = C_dir::RemoveDirectory(fn);
   else
      ok = C_file::DeleteFile(fn);
#ifdef EXPLORER
   if(!ok){
      dword atts;
      C_file::GetAttributes(fn, atts);
      if(atts&C_file::ATT_READ_ONLY){
         if(del_rd_only==DEL_RD_ONLY_NO)
            return true;
         if(C_file::SetAttributes(fn, C_file::ATT_READ_ONLY, 0)){
            if(del_rd_only==DEL_RD_ONLY_ASK){
               C_file::SetAttributes(fn, C_file::ATT_READ_ONLY, C_file::ATT_READ_ONLY);
               bool can_be_multi_choice = (filenames.size()>1);
               const wchar *options[] = { app.GetText(TXT_YES), app.GetText(TXT_NO), app.GetText(TXT_DELETE_ALL), app.GetText(TXT_SKIP_ALL) };
               Cstr_w s = fn;
               s<<L"\n\n" <<app.GetText(TXT_Q_DELETE_READ_ONLY);

               class C_q: public C_multi_selection_callback{
                  C_mode_long_operation &mod;
                  virtual void Select(int option){
                     switch(option){
                     case 2:           //yes all
                        mod.del_rd_only = DEL_RD_ONLY_ALL;
                     case 0:           //yes
                        {
                           const wchar *fn1 = mod.filenames[mod.file_index];
                           C_file::SetAttributes(fn1, C_file::ATT_READ_ONLY, 0);
                        }
                        break;
                     case 3:           //no all
                        mod.del_rd_only = DEL_RD_ONLY_NO;
                     case 1:           //no
                                    //set atts back
                        if(++mod.file_index==(dword)mod.filenames.size())
                           mod.EndOfWork();
                        break;
                     }
                  }
                  virtual void Cancel(){
                     mod.app.CloseMode(mod);
                  }
               public:
                  C_q(C_mode_long_operation &_m):
                     mod(_m)
                  {}
               };
               CreateMultiSelectionMode(app, TXT_DELETE, s, options, can_be_multi_choice ? 4 : 2, new(true) C_q(*this), true);
               return false;
            }
            if(is_folder)
               ok = C_dir::RemoveDirectory(fn);
            else
               ok = C_file::DeleteFile(fn);
         }
      }
   }
#endif
   return ok;
}

//----------------------------

void C_client_file_mgr::C_mode_long_operation::EndOfWork(E_TEXT_ID err_title, const wchar *err){
   C_mode_file_browser &mod_fl = (C_mode_file_browser&)*GetParent();
                              //end of work
#ifdef EXPLORER
   if(operation==OPERATION_ZIP && err_title==TXT_NULL){
                              //close ZIP
      dword central_offset = fl_dst.Tell();

      dword num_files = 0;
                              //write central directory
      for(int i=0; i<zip_files.size(); i++){
         if(IsDirectory(filenames[i]))
            continue;
         const S_zip_file_info &fi = zip_files[i];

         fl_dst.WriteDword(0x02014b50); //signature
         fl_dst.WriteWord(0xb14); //versionUsed
         fl_dst.WriteWord(20); //versionNeeded
         fl_dst.WriteWord(0x802); //flags
         fl_dst.WriteWord(8); //compression
         fl_dst.WriteDword(fi.time_date); //lastModTimeDate (DOS format)
         fl_dst.WriteDword(fi.crc); //crc
         fl_dst.WriteDword(fi.size_compr); //compressed_size
         fl_dst.WriteDword(fi.size_uncompr); //uncompressed_size
         fl_dst.WriteWord((word)fi.filename_utf8.Length()); //filenameLen
         fl_dst.WriteWord(0);  //extraFieldLen
         fl_dst.WriteWord(0);  //commentLen
         fl_dst.WriteWord(0);  //diskStart
         fl_dst.WriteWord(0);  //internal
         fl_dst.WriteDword(0);  //external
         fl_dst.WriteDword(fi.local_hdr_offs);  //localOffset
         fl_dst.Write((const char*)fi.filename_utf8, fi.filename_utf8.Length());
         ++num_files;
      }
      dword central_dir_size = fl_dst.Tell() - central_offset;
                              //write end dir record
      fl_dst.WriteDword(0x06054b50); //signature
      fl_dst.WriteWord(0);  //thisDisk
      fl_dst.WriteWord(0);  //dirStartDisk
      fl_dst.WriteWord(word(num_files));  //numEntriesOnDisk
      fl_dst.WriteWord(word(num_files));  //numEntriesTotal
      fl_dst.WriteDword(central_dir_size); //size of the central directory
      fl_dst.WriteDword(central_offset); //dirStartOffset
      fl_dst.WriteWord(0); //commentLen

      C_file::E_WRITE_STATUS ws = fl_dst.WriteFlush();
      fl_dst.Close();
      if(ws){
         err_title = ws==C_file::WRITE_DISK_FULL ? TXT_ERR_WRITE_DISK_FULL : TXT_ERR_WRITE;
         err = fn_dst;
         C_file::DeleteFile(fn_dst);
      }
   }
#endif
   mod_fl.marked_files.clear();
   ((C_client_file_mgr&)app).FileBrowser_RefreshEntries(mod_fl);
                        //update all drives' info
   for(int i=mod_fl.entries.size(); i--; ){
      if(!mod_fl.entries[i].level)
         FileBrowser_UpdateDriveInfo(mod_fl, i);
   }
   AddRef();
   app.CloseMode(*this);

   if(err_title!=TXT_NULL)
      ((C_client_file_mgr&)app).ShowErrorWindow(err_title, err);
#ifdef EXPLORER
   else if(operation==OPERATION_ZIP){
                              //go to created Zip
      ((C_client_file_mgr&)app).FileBrowser_GoToPath(mod_fl, fn_dst, false);
                              //write about success
      Cstr_w s = app.GetText(TXT_ARCHIVE_WAS_CREATED);
      s<<L": " <<file_utils::GetFileNameNoPath(fn_dst);
      CreateInfoMessage(app, TXT_SUCCESS, s);
      if(zip_move){
         AddRef();
                              //prepare delete operation
         SetTitle(app.GetText(TXT_PROGRESS_DELETING));
         operation = OPERATION_DELETE;
                              //add folders to delete
         file_index = 0;
         progress_2.total = -1;
         progress_1.total = filenames.size();
         progress_1.pos = 0;
         last_prog_1 = -1;
         InitLayout();
         app.ActivateMode(*this);
      }
   }
#endif
   Release();
}

//----------------------------

void C_client_file_mgr::C_mode_long_operation::Tick(dword time, bool &redraw){

                              //sometimes don't do anything (so that system can live)
   ++frame_count;
   if(!(frame_count&31))
      return;
                              //do some task (watch spent time)
   dword end_time = GetTickTime() + 50;
   E_TEXT_ID err_title = TXT_NULL;
   Cstr_w err;
   const C_zip_package *arch = GetFileBrowser().arch;

   do{
      const Cstr_w &fn = filenames[file_index];

      switch(operation){
      case OPERATION_DELETE:
         {
            bool ok = false;
            if(IsDirectory(fn)){
               ok = DeleteFile(fn, true);
               if(this!=app.mode)
                  return;
               if(!ok){
                              //try to clear read-only flag
               }
            }else{
#ifdef EXPLORER
               if(arch && arch->GetArchiveType()>=1000)
                  ok = ((C_archive_simulator&)*arch).DeleteFile(fn);
               else
#endif
               {
                  ok = DeleteFile(fn, false);
                  if(this!=app.mode)
                     return;
               }
            }
            if(!ok){
               err_title = TXT_ERR_CANT_DELETE_FILE;
               err = fn;
            }
            ++file_index;
            progress_1.pos = file_index;
         }
         break;
      case OPERATION_COPY:
      case OPERATION_MOVE:
         {
            const Cstr_w &fn_dest = filenames_dst[file_index];
            if(IsDirectory(fn_dest)){
               if(fn_dest[0]=='.'){
                              //marker, end of folder
                  if(operation==OPERATION_MOVE){
                     C_dir::RemoveDirectory(fn_dest.RightFromPos(1));
                  }
                  ++file_index;
               }else{
                                 //create target directory
                  if(C_dir::MakeDirectory(fn_dest, true)){
                     ++file_index;
                  }else{
                     err_title = TXT_ERR_CANT_MAKE_DIR;
                     err = fn_dest;
                  }
               }
            }else{
                              //copy current file
               if(fl_src){
                              //continue in copying
                  int sz = fl_src->GetFileSize();
                  const int BUF_SIZE = copy_buf.Size();
                  int pos = fl_src->Tell();
                  int copy_size = Min(sz-pos, BUF_SIZE);
                  byte *buf = copy_buf.Begin();
                  if(!fl_src->Read(buf, copy_size)){
                     err_title = TXT_ERROR;
                     err = fn;
                  }else{
                     C_file::E_WRITE_STATUS st = fl_dst.Write(buf, copy_size);
                     if(!st)
                        st = fl_dst.WriteFlush();
                     if(st){
                                 //write error
                        err_title = (st==C_file::WRITE_DISK_FULL) ? TXT_ERR_WRITE_DISK_FULL : TXT_ERR_WRITE;
                        err = fn_dest;
                     }else{
                        progress_1.pos += copy_size;
                        progress_2.pos += copy_size;
                        if(fl_src->IsEof()){
                           //end of file, close and move to next
                           progress_1.pos = sz;
                           last_prog_1 = -1;

                           delete fl_src;
                           fl_src = NULL;
                           fl_dst.Close();
                           //copy write time from source to dest file
                           S_date_time dt;
                           if(C_file::GetFileWriteTime(fn, dt))
                              C_file::SetFileWriteTime(fn_dst, dt);
                           //copy also attributes
                           dword atts;
                           if(C_file::GetAttributes(fn, atts))
                              C_file::SetAttributes(fn_dst, dword(-1), atts);

                           fn_dst.Clear();
                           //delete source if moving
                           if(operation==OPERATION_MOVE){
#ifdef EXPLORER
                              if(arch && arch->GetArchiveType()>=1000){
                                 C_archive_simulator &as = (C_archive_simulator&)*arch;
                                 as.DeleteFile(fn);
                              }else
#endif
                                 C_file::DeleteFile(fn);
                           }
                           ++file_index;
                        }
                     }
                  }
               }else{
                  progress_1.pos = 0;
                  last_prog_1 = -1;
#ifdef EXPLORER
                  bool skip = false;
                  if(overwrite_mode!=OVERWRITE_YES){
                                             //check if dest file already exists
                     C_file f1;
                     if(f1.Open(fn_dest)){
                              //destination already exists, ask what to do
                        switch(overwrite_mode){
                        case OVERWRITE_ASK:
                           {
                              timer->Pause();
                              bool can_be_multi_choice = (filenames.size()>1);
                              const wchar *options[] = { app.GetText(can_be_multi_choice ? TXT_OVERWRITE_ALL : TXT_OVERWRITE), app.GetText(can_be_multi_choice ? TXT_SKIP_ALL : TXT_SKIP) };
                              Cstr_w s = fn_dest;
                              s<<L"\n\n" <<app.GetText(TXT_Q_OVERWRITE);
                              CreateMultiSelectionMode(app, TXT_FILE_ALREADY_EXISTS, s, options, 2, this);
                           }
                           return;
                        case OVERWRITE_NO:
                           {
                                       //skip the file, just count size
                              C_file *fl = CreateFile(fn, arch);
                              if(fl){
                                 progress_2.pos += fl->GetFileSize();
                                 delete fl;
                              }
                              ++file_index;
                              skip = true;
                           }
                           break;
                        default:
                           assert(0);
                        }
                     }
                  }
                  if(!skip)
#endif
                  {
                     bool rename_ok = false;
                     if(operation==OPERATION_MOVE){
                        C_file::DeleteFile(fn_dest);
#ifdef EXPLORER
                        if(arch && arch->GetArchiveType()>=1000){
                           C_archive_simulator &as = (C_archive_simulator&)*arch;
                           const wchar *real_fn = as.GetRealFileName(fn);
                           if(real_fn)
                              rename_ok = C_file::RenameFile(real_fn, fn_dest);
                        }else
#endif
                           rename_ok = C_file::RenameFile(fn, fn_dest);
                        if(rename_ok){
                                       //count progress
                           C_file fl;
                           if(fl.Open(fn_dest))
                              progress_2.pos += fl.GetFileSize();
                           ++file_index;
                        }
                     }
                     if(!rename_ok){
                                 //open source file
                        C_file *fl = CreateFile(fn, arch);
                        if(fl){
                           fl_src = fl;
                                 //open dest file
                           if(!fl_dst.Open(fn_dest, C_file::FILE_WRITE | C_file::FILE_WRITE_READONLY)){
                              err_title = TXT_ERR_CANT_OPEN_FILE;
                              err = fn_dest;
                           }else{
                              fn_dst = fn_dest;
                              progress_1.total = fl_src->GetFileSize();
                              if(!copy_buf.Size())
                                 copy_buf.Resize(0x10000);
                           }
                        }else{
                           //assert(0);
                                 //unexpected, silently ignore
                           ++file_index;
                        }
                     }
                  }
               }
            }
         }
         break;
#ifdef EXPLORER
      case OPERATION_ZIP:
         {
            if(IsDirectory(fn)){
                              //skip folders, they'll be just deleted
               ++file_index;
               break;
            }
            S_zip_file_info &fi = zip_files[file_index];
            if(fl_src){
                              //compress part of file
               byte *src_ptr = copy_buf.Begin();
               if(zs.avail_in){
                  if(src_ptr!=zs.next_in)
                     MemCpy(src_ptr, zs.next_in, zs.avail_in);
                  src_ptr += zs.avail_in;
               }
               const dword TMP_SIZE = compr_buf.Size();
               dword rl = Min(fl_src->GetFileSize()-fl_src->Tell(), TMP_SIZE-zs.avail_in);
               zs.avail_in += rl;
               zs.avail_out = TMP_SIZE;
               zs.next_in = copy_buf.Begin();
               zs.next_out = compr_buf.Begin();
               if(rl){
                  if(!fl_src->Read(src_ptr, rl)){
                     assert(0);
                  }
                                    //compute crc (usin zlib function)
                  fi.crc = crc32(fi.crc, src_ptr, rl);
                                    //progress display
                  progress_1.pos += rl;
                  progress_2.pos += rl;
               }
               int st = deflate(&zs, zs.avail_in ? Z_NO_FLUSH : Z_FINISH);
               assert(st>=0);
               dword num_wr = TMP_SIZE-zs.avail_out;
               C_file::E_WRITE_STATUS ws = fl_dst.Write(compr_buf.Begin(), num_wr);
               if(ws){
                              //write error
                  err_title = ws==C_file::WRITE_DISK_FULL ? TXT_ERR_WRITE_DISK_FULL : TXT_ERR_WRITE;
                  err = fn_dst;
               }else
               if(st==Z_STREAM_END){
                  delete fl_src;
                  fl_src = NULL;
                              //end of file compress, finish
                  fi.size_compr = zs.total_out;
                  st = deflateEnd(&zs);
                  assert(!st);

                                       //write back compressed size
                  dword offs = fl_dst.Tell();

                  fl_dst.Seek(fi.local_hdr_offs+0x12);
                  fl_dst.WriteDword(fi.size_compr);

                  fl_dst.Seek(fi.local_hdr_offs+0xe);
                  fl_dst.WriteDword(fi.crc);

                  fl_dst.Seek(offs);

                  ++file_index;
               }
            }else{
                              //open and prepare next file
               progress_1.pos = 0;
               last_prog_1 = -1;

               fl_src = CreateFile(fn);
               if(!fl_src){
                  assert(0);
                  ++file_index;
               }else{
                  dword src_size = fl_src->GetFileSize();
                  progress_1.pos = 0;
                  progress_1.total = src_size;
                  fi.crc = 0;
                  fi.local_hdr_offs = fl_dst.Tell();
                  fi.size_uncompr = src_size;

                  fi.time_date = 0;
                  S_date_time td;
                  if(C_file::GetFileWriteTime(fn, td)){
                     fi.time_date = (td.second) |
                        (td.minute<<5) |
                        (td.hour<<11) |
                        ((td.day+1)<<16) |
                        ((td.month+1)<<21) |
                        ((td.year-1980)<<25);
                  }

                              //write local file header
                  fl_dst.WriteDword(0x04034b50); //signature
                  fl_dst.WriteWord(20); //version_needed
                  fl_dst.WriteWord(0x802); //flags
                  fl_dst.WriteWord(8); //compression (deflate)
                  fl_dst.WriteDword(fi.time_date); //lastModTimeDate (DOS format)
                  fl_dst.WriteDword(0); //crc
                  fl_dst.WriteDword(0); //sz_compressed
                  fl_dst.WriteDword(src_size); //sz_uncompressed
                  fl_dst.WriteWord((word)fi.filename_utf8.Length());  //filenameLen
                  fl_dst.WriteWord(0);  //extraFieldLen
                  fl_dst.Write((const char*)fi.filename_utf8, fi.filename_utf8.Length());

                  MemSet(&zs, 0, sizeof(zs));
                  int err1 = deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 9, Z_DEFAULT_STRATEGY);
                  assert(!err1);

                  zs.avail_in = 0;
               }
            }
         }
         break;
#endif
      }

      if(file_index==(dword)filenames.size() || err_title!=TXT_NULL){
         EndOfWork(err_title, err);
         return;
      }
   }while(GetTickTime()<end_time);
                              //pause, draw progress and let system live
   DrawContent();
}

//----------------------------

void C_client_file_mgr::C_mode_long_operation::Draw() const{

   super::Draw();
   last_prog_1 = -1;
   last_prog_2 = -1;
   DrawContent();
}

//----------------------------

void C_client_file_mgr::C_mode_long_operation::DrawContent() const{

   dword col_text = app.GetColor(COL_TEXT_POPUP);

   S_rect rc_in(rc.x, rc.y+app.GetDialogTitleHeight(), rc.sx, app.fdb.line_spacing);
   if(last_prog_1!=progress_1.pos){
                              //draw first line
      Cstr_w fn = filenames[file_index];
      if(IsDirectory(fn)){
         fn = file_utils::GetFileNameNoPath(fn.Left(fn.Length()-1));
      }else
         fn = file_utils::GetFileNameNoPath(fn);
      app.DrawDialogBase(rc, false, &rc_in);
      app.SetClipRect(rc_in);
                              //file name
      app.DrawString(fn, rc_in.x+app.fdb.cell_size_x/2, rc_in.y, UI_FONT_BIG, 0, col_text, -(rc_in.sx-app.fdb.cell_size_x/2));
      switch(operation){
      case OPERATION_DELETE:
         break;
      case OPERATION_COPY:
      case OPERATION_MOVE:
         break;
      }
      app.ResetClipRect();
                              //draw total progress
      app.DrawDialogBase(rc, false, &progress_1.rc);
      app.DrawProgress(progress_1);
      last_prog_1 = progress_1.pos;
   }
   if(last_prog_2!=progress_2.pos && progress_2.total!=-1){
      switch(operation){
      case OPERATION_COPY:
      case OPERATION_MOVE:
#ifdef EXPLORER
      case OPERATION_ZIP:
#endif
         rc_in.y += app.fdb.line_spacing*2;
         app.DrawDialogBase(rc, false, &rc_in);
         app.DrawString(app.GetText(TXT_TOTAL_SIZE), rc_in.x+app.fdb.cell_size_x/2, rc_in.y, UI_FONT_BIG, 0, col_text);
                                 //draw total progress
         app.DrawDialogBase(rc, false, &progress_2.rc);
         app.DrawProgress(progress_2);
         break;
      }
      last_prog_2 = progress_2.pos;
   }
}

//----------------------------

void C_client_file_mgr::FileBrowser_DeleteSelected(){

   C_mode_file_browser &mod = (C_mode_file_browser&)*mode;
   if(!mod.entries.size())
      return;
   mod.img_thumbnail = NULL;
#ifdef AUDIO_PREVIEW
   mod.audio_preview.Close();
#endif
   C_mode_long_operation &mod_lop = *new(true) C_mode_long_operation(*this, C_mode_long_operation::OPERATION_DELETE, GetText(TXT_PROGRESS_DELETING));
   dword save_flags = mod.op_flags;
   mod.op_flags |= GETDIR_HIDDEN;
   FileBrowser_CollectMarkedOrSelectedFiles(mod, mod_lop.filenames, true, false);
   mod.op_flags = save_flags;
   if(!mod_lop.filenames.size()){
      assert(0);
      mod_lop.Release();
      return;
   }
   mod_lop.progress_1.total = mod_lop.filenames.size();
   mod_lop.InitLayout();
   CreateTimer(mod_lop, 50);
   ActivateMode(mod_lop, (mod_lop.filenames.size()>1));
}

//----------------------------

void C_client_file_mgr::FileBrowser_BeginDelete(C_mode_file_browser &mod){

   if(!(mod.flags&mod.FLG_ALLOW_DELETE))
      return;

   class C_question: public C_question_callback{
      C_client_file_mgr &app;
      virtual void QuestionConfirm(){
         app.FileBrowser_DeleteSelected();   
      }
   public:
      C_question(C_client_file_mgr &_a): app(_a){}
   };
   if(mod.marked_files.size()){
      Cstr_w s; s<<GetText(TXT_MARK_SELECTED) <<L": " <<mod.marked_files.size();
      CreateQuestion(*this, TXT_Q_DELETE_SELECTED, s, new(true) C_question(*this), true);
   }else{
      if(!mod.entries.size())
         return;

      const C_mode_file_browser::S_entry &e = mod.entries[mod.selection];
      switch(e.type){
      case C_mode_file_browser::S_entry::DIR: CreateQuestion(*this, TXT_Q_DELETE_DIR, e.name, new(true) C_question(*this), true); break;
      case C_mode_file_browser::S_entry::FILE: CreateQuestion(*this, TXT_Q_DELETE_FILE, e.name, new(true) C_question(*this), true); break;
      }
   }
}

//----------------------------

int C_client_file_mgr::FileBrowser_GetDirEntry(C_mode_file_browser &mod, int indx){

   if(indx==-1)
      indx = mod.selection;
   const C_mode_file_browser::S_entry &e = mod.entries[indx];
   if(e.type==e.FILE && e.level){
                              //step up to parent dir
      while(indx--){
         if(mod.entries[indx].level < e.level)
            break;
      }
   }
   return indx;
}

//----------------------------

int C_client_file_mgr::FileBrowser_GetDriveEntry(C_mode_file_browser &mod, int indx){

   if(indx==-1)
      indx = mod.selection;
   while(mod.entries[indx].level)
      --indx;
   return indx;
}

//----------------------------

void C_client_file_mgr::FileBrowser_UpdateDriveInfo(C_mode_file_browser &mod, int indx){

   C_mode_file_browser::S_entry &ed = mod.entries[FileBrowser_GetDriveEntry(mod, indx)];
   if((ed.type==ed.DRIVE
#ifdef NO_FILE_SYSTEM_DRIVES
      || ed.level==0
#endif
      ) && ed.type!=ed.FILE
      ){
      dword fl, fh, tl, th;
      if(GetDiskSpace(ed, fl, fh, tl, th)){
         ed.size.drive.free[0] = fl;
         ed.size.drive.free[1] = fh;
         ed.size.drive.total[0] = tl;
         ed.size.drive.total[1] = th;
      }else{
         MemSet(&ed.size.drive, 0, sizeof(ed.size.drive));
      }
   }
}

//----------------------------

void C_client_file_mgr::FileBrowser_BeginMakeNewFile(C_mode_file_browser &mod, bool dir){

   if(!(mod.flags&mod.FLG_ALLOW_MAKE_DIR))
      return;
   assert(mod.flags&mod.FLG_ALLOW_RENAME);
   if(!mod.entries.size())
      return;
   mod.img_thumbnail = NULL;
#ifdef AUDIO_PREVIEW
   mod.audio_preview.Close();
#endif
   mod.selection = FileBrowser_GetDirEntry(mod);
   FileBrowser_ExpandDir(mod);
   C_mode_file_browser::S_entry &e = mod.entries[mod.selection];
   e.expand_status = e.EXPANDED;
   ++mod.selection;
   C_mode_file_browser::S_entry en;
   en.type = dir ? en.DIR : en.FILE;
   en.level = e.level+1;
   en.size.file_size = 0;
   en.expand_status = en.NEVER_EXPAND;
   mod.entries.insert(&mod.entries[mod.selection], &en, &en+1);
   mod.sb.total_space += mod.entry_height;
   mod.sb.SetVisibleFlag();
   mod.EnsureVisible();
   mod.BeginRename();
}

//----------------------------

bool C_client_file_mgr::CopyFile(C_file &fl_src, const wchar *src_name, const wchar *dst_name, E_TEXT_ID dlg_title, int total_prog_size, int total_prog_pos){

   C_file ck_dst;
   if(!ck_dst.Open(dst_name, C_file::FILE_WRITE | C_file::FILE_WRITE_READONLY)){
      Cstr_w fn = dst_name;
#ifdef UNIX_FILE_SYSTEM
      fn = file_utils::ConvertPathSeparatorsToUnix(fn);
#endif
      ShowErrorWindow(TXT_ERR_CANT_OPEN_FILE, fn);
      return false;
   }
   int sz = fl_src.GetFileSize();
   const int BUF_SIZE = 16384;

   S_progress_display_help ip(this);
   ip.progress1.total = total_prog_size;
   ip.progress1.pos = total_prog_pos;
   PrepareProgressBarDisplay(dlg_title, src_name, ip, Max(1, sz/BUF_SIZE), total_prog_size==-1 ? TXT_NULL : TXT_TOTAL_SIZE);
   if(total_prog_size!=-1){
      ip.progress1.total = total_prog_size;
      ip.progress1.pos = total_prog_pos;
   }
                           //copy the file
   byte *buf = new(true) byte[BUF_SIZE];
   C_file::E_WRITE_STATUS err = C_file::WRITE_OK;
   dword last_progress_draw = 0;
   for(int i=0; i<sz; ){
      int copy_size = Min(sz-i, BUF_SIZE);
      dword tm = GetTickTime();
      if(total_prog_size!=-1){
         ip.progress1.pos += copy_size;
         if((tm-last_progress_draw) > 100){
            S_rect rc = ip.progress1.rc; rc.Expand();
            FillRect(rc, GetColor(COL_GREY));
            DrawProgress(ip.progress1);
            UpdateScreen();
         }
      }
      ++ip.progress.pos;
      if((tm-last_progress_draw) > 100){
         last_progress_draw = tm;
         DisplayProgressBar(ip.progress.pos, &ip);
      }
      fl_src.Read(buf, copy_size);
      err = ck_dst.Write(buf, copy_size);
      if(err)
         break;
      i += copy_size;
   }
   delete[] buf;
   if(err || (err=ck_dst.WriteFlush(), err)){
      ck_dst.Close();
      C_file::DeleteFile(dst_name);
      ShowErrorWindow(err==C_file::WRITE_DISK_FULL ? TXT_ERR_WRITE_DISK_FULL : TXT_ERR_WRITE, dst_name);
      return false;
   }
   return true;
}

//----------------------------

void C_client_file_mgr::FileBrowser_FinishGetSaveFileName(C_mode_file_browser &mod, bool ask_overwrite){

   assert(mod.browser_mode==mod.MODE_GET_SAVE_FILENAME);
                              //get name of destination file
   Cstr_w dst_name;
   int dir_sel = FileBrowser_GetDirEntry(mod);
   FileBrowser_GetFullName(mod, dst_name, dir_sel);
   dst_name<<L"\\" <<mod.save_as_name;

                              //check if dst exists
   if(ask_overwrite){
      C_file ck_dst;
      if(ck_dst.Open(dst_name)){
         class C_question: public C_question_callback{
            C_mode_file_browser &mod;
            virtual void QuestionConfirm(){
               ((C_client_file_mgr&)mod.C_mode::app).FileBrowser_FinishGetSaveFileName(mod, false);
            }
         public:
            C_question(C_mode_file_browser &_m): mod(_m){}
         };
         CreateQuestion(*this, TXT_Q_OVERWRITE, dst_name, new(true) C_question(mod), true);
         return;
      }
   }
   C_mode_file_browser::t_OpenCallback cb = mod._OpenCallback;
   C_file_browser_callback *open_callback = mod.open_callback;
   FileBrowser_Close(mod);
   if(cb)
      (this->*cb)(&dst_name, NULL);
   else if(open_callback)
      open_callback->FileSelected(dst_name);
}

//----------------------------
#ifdef EXPLORER
void C_client_file_mgr::FileBrowser_RefreshEntries1(void *c){
   C_client_file_mgr *_this = (C_client_file_mgr*)c;
   if(_this->mode && _this->mode->Id()==C_client_file_mgr::ID){
      _this->FileBrowser_RefreshEntries((C_mode_file_browser&)*_this->mode);
      _this->RedrawScreen();
   }
}
#endif
//----------------------------

void C_client_file_mgr::FileBrowser_RefreshEntries(C_mode_file_browser &mod){

                              //can't be in rename mode when doing this
   if(mod.te_rename)
      //FileBrowser_CancelRename(mod);
      return;
                              //check all entries
   for(int i=0; i<mod.entries.size(); i++){
      C_mode_file_browser::S_entry &e = mod.entries[i];
      Cstr_w full_name;
      FileBrowser_GetFullName(mod, full_name, i, true);
      switch(e.type){
      case C_mode_file_browser::S_entry::DRIVE:
                              //update size
         FileBrowser_UpdateDriveInfo(mod, i);
                              //flow...
      case C_mode_file_browser::S_entry::DIR:
      case C_mode_file_browser::S_entry::ARCHIVE:
         if(e.expand_status==e.EXPANDED){
                              //get folder content again, update children
            C_vector<C_mode_file_browser::S_entry> entries;
            entries.reserve(mod.entries.size()+2); //optimization: be prepared for content
            FileBrowser_GetDirectoryContents(&mod, full_name, &e, &entries, mod.op_flags, mod.extension_filter);

            int e_level = e.level;
                              //check if all subsequent entries on same level are found in the list
            for(int j=i+1; j<mod.entries.size(); j++){
               C_mode_file_browser::S_entry &e1 = mod.entries[j];
               if(e1.level <= e_level)
                  break;
               if(e1.level!=e_level+1)
                  continue;
               int ii;
               for(ii=0; ii<entries.size(); ii++){
                  const C_mode_file_browser::S_entry &e2 = entries[ii];
                  if(e1.name==e2.name){
                     e1.size = e2.size;
                     e1.modify_date = e2.modify_date;
                     e1.atts = e2.atts;
                     break;
                  }
               }
               if(ii==entries.size()){
                              //entry disappeared, remove from list
                  int lev = e1.level;
                  if(e1.type!=e1.FILE)
                     FileBrowser_CollapseDir(mod, j);
                  mod.entries.erase(&mod.entries[j]);
                  mod.sb.total_space -= mod.entry_height;
                  mod.sb.SetVisibleFlag();
                  if(mod.selection==j){
                     if(j>=mod.entries.size() || mod.entries[j].level!=lev)
                        --mod.selection;
                  }else
                  if(mod.selection > j)
                     --mod.selection;
                  --j;
               }else{
                  //entries.erase(&entries[ii]);
                              //optimized remove
                  entries[ii] = entries.back();
                  entries.pop_back();
               }
            }
            if(entries.size()){
                              //refresh dir contents, so that new entries will appear (correctly sorted)
               FileBrowser_CollapseDir(mod, i);
               FileBrowser_ExpandDir(mod, i);
            }else{
                              //if the dir has no more entries, make it never expand
               if(i==mod.entries.size()-1 || mod.entries[i+1].level<=e_level)
                  mod.entries[i].expand_status = C_mode_file_browser::S_entry::NEVER_EXPAND;
            }
         }else{
                              //just update epand status COLLAPSED/NEVER_EXPAND
            if(FileBrowser_GetDirectoryContents(&mod, full_name, &e, NULL, mod.op_flags | GETDIR_CHECK_IF_ANY, mod.extension_filter))
               e.expand_status = e.COLLAPSED;
            else
               e.expand_status = e.NEVER_EXPAND;
         }
         break;
#ifdef NO_FILE_SYSTEM_DRIVES
      case C_mode_file_browser::S_entry::FILE:
         if(!mod.arch){
                              //check if the file in root exists
            dword atts;
            if(!C_file::GetAttributes(full_name, atts)){
                              //remove the entry
               mod.entries.remove_index(i);
               mod.sb.total_space -= mod.entry_height;
               mod.sb.SetVisibleFlag();
               if(mod.selection==i){
                  if(i>=mod.entries.size())
                     --mod.selection;
               }else
               if(mod.selection > i)
                  --mod.selection;
               --i;
            }
         }
         break;
#endif
      }
   }
                              //make sure all marked files still exist
   for(int i=mod.marked_files.size(); i--; ){
      Cstr_w &fn = mod.marked_files[i];
      dword atts;
      if(!C_file::GetAttributes(fn, atts))
         mod.marked_files.erase(&fn);
   }

   mod.ChangeSelection(Min(Max(mod.selection, 0), mod.GetNumEntries()-1));
   mod.EnsureVisible();
}

//----------------------------
#if defined MAIL || defined WEB || defined EXPLORER

//----------------------------

void C_client_file_mgr::CollectCopyOrMoveFolder(C_mode_file_browser &mod, C_mode_long_operation &mod_lop, const wchar *src_base, const wchar *src_name, const wchar *dst){

   Cstr_w src_full; src_full<<src_base <<src_name;
   Cstr_w dst_full; dst_full<<dst <<src_name;
                              //don't copy/move to self
   if(src_full==dst_full)
      return;

   if(*src_full){
      mod_lop.filenames.push_back(src_full) <<'\\';
      mod_lop.filenames_dst.push_back(dst_full) <<'\\';
   }
                              //get contents of this folder
   C_vector<C_mode_file_browser::S_entry> entries;
   FileBrowser_GetDirectoryContents(&mod, src_full, NULL, &entries, GETDIR_DIRECTORIES | GETDIR_FILES | GETDIR_HIDDEN | GETDIR_SYSTEM);
   for(int i=0; i<entries.size(); i++){
      const C_mode_file_browser::S_entry &e = entries[i];
      Cstr_w s;
      if(*src_name)
         s<<src_name <<'\\';
      s<<e.name;
      switch(e.type){
      case C_mode_file_browser::S_entry::DIR:
                              //recursively call on this directory
         CollectCopyOrMoveFolder(mod, mod_lop, src_base, s, dst);
         break;
      case C_mode_file_browser::S_entry::FILE:
         {
            Cstr_w fn_src;
            if(src_full.Length())
               fn_src<<src_full <<'\\';
            fn_src <<e.name;
            Cstr_w fn_dst; fn_dst<<dst_full;
            if(*src_name)
               fn_dst <<'\\';
            fn_dst <<e.name;
            mod_lop.filenames.push_back(fn_src);
            mod_lop.filenames_dst.push_back(fn_dst);
            C_file *fl = CreateFile(fn_src, mod.arch);
            if(fl){
               mod_lop.progress_2.total += fl->GetFileSize();
               delete fl;
            }//else assert(0);
         }
         break;
      }
   }
   if(*src_full){
                              //add folder termination (for possible deletion)
      Cstr_w fn; fn<<'.' <<src_full <<'\\';
      mod_lop.filenames.push_back(fn);
      mod_lop.filenames_dst.push_back(fn);
   }
}

//----------------------------

C_client_file_mgr::C_mode_long_operation *C_client_file_mgr::SetModeCopyMove(C_mode_file_browser &mod_e, const C_vector<Cstr_w> &src_files, const Cstr_w &dst_path, bool move){

   C_mode_long_operation &mod_lop = *new(true) C_mode_long_operation(*this, move ? C_mode_long_operation::OPERATION_MOVE : C_mode_long_operation::OPERATION_COPY, GetText(move ? TXT_MOVING : TXT_COPYING));
   mod_lop.progress_2.total = 0;
                           //copy/move files now
   for(int i=0; i<src_files.size(); i++){
      Cstr_w fn_src = src_files[i];
      Cstr_w fn_base = fn_src.RightFromPos(fn_src.FindReverse('\\')+1);
      Cstr_w fn_dst;
      if(src_files.size()==1 && !IsDirectory(dst_path))
         fn_dst = dst_path;
      else
         fn_dst<<dst_path <<fn_base;

      if(IsDirectory(fn_src)){
                           //dir or drive
         if(fn_src.Length())
            fn_src = fn_src.Left(fn_src.Length()-1);
                           //check if target dir is not inside of this
         if(!IsFileNameInFolder(dst_path, fn_src)){
            int slash = fn_src.FindReverse('\\');
            Cstr_w src_base = fn_src.Left(slash+1);
            Cstr_w src_name = fn_src.Right(fn_src.Length()-slash-1);
            CollectCopyOrMoveFolder(mod_e, mod_lop, src_base, src_name, fn_dst);
         }
      }else{
                        //file
         if(mod_e.arch || fn_dst!=fn_src){
            mod_lop.filenames.push_back(fn_src);
            mod_lop.filenames_dst.push_back(fn_dst);
            C_file *fl = CreateFile(fn_src, mod_e.arch);
            if(fl){
               mod_lop.progress_2.total += fl->GetFileSize();
               delete fl;
            }
         }
      }
   }
   if(!mod_lop.filenames.size()){
      mod_lop.Release();
      mod_e.marked_files.clear();
      RedrawScreen();
      return NULL;
   }
   if(mod_lop.filenames.size()<=1){
      mod_lop.progress_2.total = -1;
   }

   mod_lop.InitLayout();
   CreateTimer(mod_lop, 50);
   ActivateMode(mod_lop);
   return &mod_lop;
}

//----------------------------

void C_client_file_mgr::FileBrowser_CopyMove(C_mode_file_browser &mod, bool move, bool ask_dest){

   if(ask_dest){
      FileBrowser_SaveLastPath(mod);
      C_mode_file_browser &mod_cm = SetModeFileBrowser(move ? C_mode_file_browser::MODE_MOVE : C_mode_file_browser::MODE_COPY, false,
         NULL, move ? TXT_MOVE_MARKED_TO : TXT_COPY_MARKED_TO);

      mod_cm.flags = mod.FLG_ACCEPT_DIR | mod.FLG_ACCEPT_FILE | mod.FLG_ALLOW_MAKE_DIR | mod.FLG_ALLOW_RENAME | mod.FLG_ALLOW_SHOW_SUBMENU | mod.FLG_SELECT_OPTION;
                              //copy selected files to new mode, so that they're shown
      mod_cm.marked_files = mod.marked_files;
      FileBrowser_CleanMarked(mod_cm);
      RedrawScreen();
   }else{
                              //destination selected
      Cstr_w dst_path;
      int dir_sel = FileBrowser_GetDirEntry(mod);
      FileBrowser_GetFullName(mod, dst_path, dir_sel);
      dst_path<<'\\';

      C_mode_file_browser &mod_e = (C_mode_file_browser&)*mod.GetParent();
      mod_e.marked_files = mod.marked_files;
      CloseMode(mod);

      C_vector<Cstr_w> src_files = mod_e.marked_files;
      if(!src_files.size()){
         Cstr_w full_path;
         FileBrowser_GetFullName(mod_e, full_path, -1, true);
                           //append '\' to dirs or drives
         switch(mod_e.entries[mod_e.selection].type){
         case C_mode_file_browser::S_entry::DIR: case C_mode_file_browser::S_entry::DRIVE:
            full_path<<'\\';
            break;
         }
         src_files.push_back(full_path);
      }
      mod_e.img_thumbnail = NULL;
#ifdef AUDIO_PREVIEW
      mod_e.audio_preview.Close();
#endif
      SetModeCopyMove(mod_e, src_files, dst_path, move);
   }
}

//----------------------------

bool C_client_file_mgr::FileBrowser_SendBy(C_mode_file_browser &mod, C_obex_file_send::E_TRANSFER tr){

   C_vector<Cstr_w> filenames;
   FileBrowser_CollectMarkedOrSelectedFiles(mod, filenames);
   return C_client_file_send::CreateMode(this, filenames, tr);
}

#endif //MAIL || WEB || EXPLORER
//----------------------------

void C_client_file_mgr::FileBrowser_CollectFolderFiles(C_mode_file_browser &mod, const wchar *folder, C_vector<Cstr_w> &filenames, bool add_folders, bool folders_first){

   if(add_folders && folders_first)
      filenames.push_back(folder) <<'\\';
                              //get contents of this folder
   C_vector<C_mode_file_browser::S_entry> entries;
   FileBrowser_GetDirectoryContents(&mod, folder, NULL, &entries, mod.op_flags | GETDIR_DIRECTORIES | GETDIR_FILES);
   for(int i=0; i<entries.size(); i++){
      const C_mode_file_browser::S_entry &e = entries[i];
      Cstr_w fn;
      fn<<folder;
      if(fn.Length())
         fn <<'\\';
      fn<<e.name;
      switch(e.type){
      case C_mode_file_browser::S_entry::FILE:
         filenames.push_back(fn);
         break;
      default:
         FileBrowser_CollectFolderFiles(mod, fn, filenames, add_folders, folders_first);
      }
   }
   if(add_folders && !folders_first)
      filenames.push_back(folder) <<'\\';
}

//----------------------------

void C_client_file_mgr::FileBrowser_CollectMarkedOrSelectedFiles(C_mode_file_browser &mod, C_vector<Cstr_w> &filenames, bool add_folders, bool folders_first) const{

   FileBrowser_CleanMarked(mod);
   if(mod.marked_files.size()){
      for(int i=0; i<mod.marked_files.size(); i++){
         const Cstr_w &fn = mod.marked_files[i];
         if(IsDirectory(fn)){
            Cstr_w dir;
            if(fn.Length())
               dir = fn.Left(fn.Length()-1);
            FileBrowser_CollectFolderFiles(mod, dir, filenames, add_folders, folders_first);
         }else{
            filenames.push_back(fn);
         }
      }
   }else{
      const C_mode_file_browser::S_entry &e = mod.entries[mod.selection];
      Cstr_w full_name;
      FileBrowser_GetFullName(mod, full_name);
      if(e.type==e.FILE){
         filenames.push_back(full_name);
      }else{
         FileBrowser_CollectFolderFiles(mod, full_name, filenames, add_folders, folders_first);
      }
   }
}

//----------------------------

bool C_client_file_mgr::FileBrowser_OpenFile(C_mode_file_browser &mod){

#ifdef AUDIO_PREVIEW
   if(mod.audio_preview.plr)
      mod.audio_preview.plr = NULL;
#endif
   Cstr_w full_name;
   FileBrowser_GetFullName(mod, full_name, -1);

   const C_mode_file_browser::S_entry &e = mod.entries[mod.selection];

   if(mod._OpenCallback || mod.open_callback){
      C_file_browser_callback *open_callback = mod.open_callback;
      if(mod.marked_files.size()){
         FileBrowser_CleanMarked(mod);
         C_vector<Cstr_w> list = mod.marked_files;
         if(mod._OpenCallback)
            return (this->*mod._OpenCallback)(NULL, &list);
         FileBrowser_Close(mod);
         open_callback->FilesSelected(list);
         return true;
      }
      switch(e.type){
      case C_mode_file_browser::S_entry::FILE:
         if(mod.flags&mod.FLG_ACCEPT_FILE){
            if(mod._OpenCallback)
               return (this->*mod._OpenCallback)(&full_name, NULL);
            FileBrowser_Close(mod);
            open_callback->FileSelected(full_name);
            return true;
         }
         break;
      default:
         if(mod.flags&mod.FLG_ACCEPT_DIR){
            full_name<<'\\';
            if(mod._OpenCallback)
               return (this->*mod._OpenCallback)(&full_name, NULL);
            FileBrowser_Close(mod);
            open_callback->FileSelected(full_name);
            return true;
         }
      }
      return false;
   }
   if(e.type!=e.FILE)
      return false;

#if defined MAIL || defined WEB || defined EXPLORER || defined MESSENGER
   E_TEXT_CODING coding = COD_DEFAULT;
#ifdef EXPLORER
   coding = reinterpret_cast<C_explorer_client*>(this)->config_xplore.default_text_coding;
#endif
   if(mod.browser_mode==mod.MODE_ARCHIVE_EXPLORER){
      if(C_client_viewer::OpenFileForViewing(this, full_name, e.name, mod.arch, NULL, NULL, &mod, 0, coding, false))
         return true;
   }else{
#ifdef EXPLORER
      if(reinterpret_cast<C_explorer_client*>(this)->OpenFileByAssociationApp(full_name))
         return true;
#endif
      if(C_client_viewer::OpenFileForViewing(this, full_name, e.name, NULL, NULL, NULL, &mod, 0, coding, true))
         return true;
   }
#endif//MAIL||WEB||EXPLORER
   if(!mod.arch){
      if(OpenFileBySystem(full_name)){
         RedrawScreen();
         return true;
      }
   }
#ifdef EXPLORER
   else{
                              //try to open file from archive in temp dir
      //return reinterpret_cast<C_explorer_client*>(this)->FileBrowser_ExtractToTempAndOpen(mod);
      Cstr_w s;
      s<<full_name;
#ifdef UNIX_FILE_SYSTEM
      s = file_utils::ConvertPathSeparatorsToUnix(s);
#endif
      s<<L"\n\n" <<GetText(TXT_EXTRACT_TO_TEMP_AND_OPEN);
      CreateQuestion(*this, TXT_ERR_NO_VIEWER, s, new(true) C_explorer_client::C_question_extract(mod), true);
      return false;
   }
#endif
#ifdef UNIX_FILE_SYSTEM
   full_name = file_utils::ConvertPathSeparatorsToUnix(full_name);
#endif
   ShowErrorWindow(TXT_ERR_NO_VIEWER, full_name);
   return false;
}

//----------------------------

bool C_client_file_mgr::C_mode_file_browser::GetPreviousNextFile(bool next, Cstr_w *filename, Cstr_w *file_title){

   if(!entries.size())
      return false;
   int sel = selection;
   S_entry &e = entries[sel];
   if(e.type!=e.FILE)
      return false;

   Cstr_w ext = text_utils::GetExtension(e.name);
   ext.ToLower();
   E_FILE_TYPE ft = DetermineFileType(ext);
                              //find next/previous of the same type, in the same folder
   while(true){
      if(!next){
         if(!sel)
            break;
         --sel;
      }else{
         if(sel==entries.size()-1)
            break;
         ++sel;
      }
                              //check if it's on same level
      S_entry &e1 = entries[sel];
      if(e1.level != e.level)
         break;
                              //check its file type
      ext = text_utils::GetExtension(e1.name);
      ext.ToLower();
      if(DetermineFileType(ext)==ft){
                              //found it
         if(filename){
            app.FileBrowser_GetFullName(*this, *filename, sel, true);
            *file_title = e1.name;
            ChangeSelection(sel);
            EnsureVisible();
         }
         return true;
      }
   }
   return false;
}

//----------------------------
#if defined MAIL || defined WEB || defined EXPLORER

bool C_client_file_mgr::FileBrowser_ExtractCallback(const Cstr_w *file, const C_vector<Cstr_w> *files){

   C_mode_file_browser &mod = (C_mode_file_browser&)*mode;
   if(mod.browser_mode!=mod.MODE_ARCHIVE_EXPLORER){
      assert(0);
   }else{
      Cstr_w fn_src;
      FileBrowser_GetFullName(mod, fn_src);

      C_vector<Cstr_w> src_files;
      src_files.push_back(fn_src);

      C_mode_long_operation *mod_lop = SetModeCopyMove(mod, src_files, *file, false);
      if(mod_lop)
         mod_lop->overwrite_mode = C_mode_long_operation::OVERWRITE_YES;
   }
   RedrawScreen();
   return true;
}

//----------------------------

void C_client_file_mgr::FileBrowser_ExtractFile(C_mode_file_browser &mod){

   if(!mod.entries.size())
      return;
   C_mode_file_browser::S_entry &e = mod.entries[mod.selection];
   E_TEXT_ID tid = TXT_EXTRACT;
#ifdef EXPLORER
   if(mod.arch && mod.arch->GetArchiveType()>=1000)
      tid = TXT_SAVE_AS;
#endif
   if(mod.marked_files.size() || e.type!=e.FILE){
      FileBrowser_CopyMove(mod, false);
   }else
      SetModeFileBrowser_GetSaveFileName(e.name, (C_mode_file_browser::t_OpenCallback)&C_client_file_mgr::FileBrowser_ExtractCallback, tid);
}

#endif
//----------------------------

bool C_client_file_mgr::FileBrowser_GetStatistics(const C_mode_file_browser &mod, const wchar *dir, dword &num_d, dword &num_f, longlong &total_size){

   C_vector<C_mode_file_browser::S_entry> entries;
   FileBrowser_GetDirectoryContents(&mod, dir, NULL, &entries, GETDIR_DIRECTORIES | GETDIR_FILES | GETDIR_HIDDEN | GETDIR_SYSTEM);
   for(int i=entries.size(); i--; ){
      const C_mode_file_browser::S_entry &e = entries[i];
      switch(e.type){
      case C_mode_file_browser::S_entry::FILE:
         ++num_f;
         if(e.size.file_size!=dword(-1))
            total_size += (longlong)e.size.file_size;
         break;
      default:
         /*
         if(igraph->GetBufferedKey()==K_RIGHT_SOFT)
            return false;
            */
         ++num_d;
         Cstr_w fn; fn<<dir <<L"\\" <<e.name;
         if(!FileBrowser_GetStatistics(mod, fn, num_d, num_f, total_size))
            return false;
      }
   }
   return true;
}

//----------------------------

void C_client_file_mgr::FileBrowser_EnterSaveAsFilename(C_mode_file_browser &mod){

   mod.selection = FileBrowser_GetDirEntry(mod);
   FileBrowser_ExpandDir(mod);
   C_mode_file_browser::S_entry en;
   en.type = en.FILE;
   en.level = mod.entries[mod.selection].level;
   if(mod.entries[mod.selection].type!=C_mode_file_browser::S_entry::FILE)
      ++en.level;
   en.name = mod.save_as_name;
   ++mod.selection;
   mod.entries.insert(&mod.entries[mod.selection], &en, &en+1);
   mod.sb.total_space += mod.entry_height;
   mod.sb.SetVisibleFlag();
   mod.EnsureVisible();
   mod.BeginRename();
   mod.entries[mod.selection].name.Clear();
}

//----------------------------

static void MakeLongFileSizeText(const dword size[2], Cstr_w &str){

   if(!size[1])
      str = text_utils::MakeFileSizeText(size[0], true, false);
   else{
      dword mb = (size[0]>>20) | (size[1]<<12);
      assert(mb>=1000);
      str.Format(L"%.%GB") <<(mb/1024) <<((mb%1024+5)/103);
   }
}

//----------------------------

void C_client_file_mgr::FileBrowser_ShowDetails(C_mode_file_browser &mod){

   if(!(mod.flags&mod.FLG_ALLOW_DETAILS))
      return;

   E_TEXT_ID tid;
   C_vector<char> buf;
   S_text_style ts;

   union{
      dword dw[2];
      longlong sz;
   } total_size = { {0} };
   if(mod.marked_files.size()){
      FileBrowser_CleanMarked(mod);

      bool wait_shown = false;

      bool files = false, dirs = false;
      dword num_files = 0, num_dirs = 0;

      for(int i=mod.marked_files.size(); i--; ){
         Cstr_w fn = mod.marked_files[i];
         if(IsDirectory(fn)){
            if(!wait_shown){
               CreateInfoMessage(*this, TXT_PLEASE_WAIT, L"...", NULL);
               CloseMode(*mode);
               UpdateScreen();
               wait_shown = true;
            }
            dirs = true;
            fn = fn.Left(fn.Length()-1);
            dword nd = 0, nf = 0;
            longlong tsz = 0;
            if(!FileBrowser_GetStatistics(mod, fn, nd, nf, tsz)){
               RedrawScreen();
               return;
            }
            num_files += nf;
            num_dirs += nd + 1;
            total_size.sz += tsz;
         }else{
            files = true;
            C_file_zip fl;
            if(fl.Open(fn, mod.arch))
               total_size.sz += (longlong)fl.GetFileSize();
            ++num_files;
         }
      }
      tid = TXT_FILE_DETAILS;

      Cstr_w tmp, size_text;
      MakeLongFileSizeText(total_size.dw, size_text);

      if(num_dirs){
         ts.AddWideStringToBuf(buf, GetText(TXT_NUM_DIRECTORIES), FF_BOLD);
         tmp.Format(L" %\n") <<num_dirs;
         ts.AddWideStringToBuf(buf, tmp);
      }
      ts.AddWideStringToBuf(buf, GetText(TXT_NUM_FILES), FF_BOLD);
      tmp.Format(L" %\n") <<num_files;
      ts.AddWideStringToBuf(buf, tmp);

      ts.AddWideStringToBuf(buf, GetText(TXT_TOTAL_SIZE), FF_BOLD);
      tmp.Format(L" %") <<size_text;
      if(!total_size.dw[1])
         tmp.AppendFormat(L" (% %)") <<total_size.dw[0] <<GetText(TXT_BYTES);
      tmp<<'\n';
      ts.AddWideStringToBuf(buf, tmp);
   }else{
      Cstr_w full_name;
      FileBrowser_GetFullName(mod, full_name);
      wchar sep = '\\';
#ifdef UNIX_FILE_SYSTEM
      full_name = file_utils::ConvertPathSeparatorsToUnix(full_name);
      sep = '/';
#endif
      const C_mode_file_browser::S_entry &e = mod.entries[mod.selection];
      switch(e.type){
      case C_mode_file_browser::S_entry::DRIVE:
      case C_mode_file_browser::S_entry::DIR:
         {
            E_TEXT_ID tt;
            if(e.type==e.DRIVE){
               tid = TXT_DRIVE_DETAILS;
               tt = TXT_DRIVE;
            }else{
               tid = TXT_DIRECTORY_DETAILS;
               tt = TXT_DIRECTORY;
            }
            Cstr_w tmp;
                              //Drive | Directory:
            tmp.Format(L"%: ") <<GetText(tt);
            ts.AddWideStringToBuf(buf, tmp, FF_BOLD);
                        //<name> \n
            ts.AddWideStringToBuf(buf, full_name);
            ts.AddWideCharToBuf(buf, sep);
            ts.AddWideCharToBuf(buf, '\n');

            CreateInfoMessage(*this, tid, GetText(TXT_PLEASE_WAIT), NULL);
            CloseMode(*mode);
            UpdateScreen();

            dword num_dirs = 0, num_files = 0;
            if(e.type==e.DIR)
               ++num_dirs;
            if(!FileBrowser_GetStatistics(mod, full_name, num_dirs, num_files, total_size.sz)){
               RedrawScreen();
               return;
            }

            Cstr_w size_text;
            MakeLongFileSizeText(total_size.dw, size_text);

            ts.AddWideStringToBuf(buf, GetText(TXT_NUM_DIRECTORIES), FF_BOLD);
            tmp.Format(L" %\n") <<num_dirs;
            ts.AddWideStringToBuf(buf, tmp);

            ts.AddWideStringToBuf(buf, GetText(TXT_NUM_FILES), FF_BOLD);
            tmp.Format(L" %\n") <<num_files;
            ts.AddWideStringToBuf(buf, tmp);

            ts.AddWideStringToBuf(buf, GetText(TXT_TOTAL_SIZE), FF_BOLD);
            tmp.Format(L" %") <<size_text;
            if(!total_size.dw[1])
               tmp.AppendFormat(L" (% %)") <<total_size.dw[0] <<GetText(TXT_BYTES);
            tmp<<'\n';
            ts.AddWideStringToBuf(buf, tmp);
         }
         break;
      case C_mode_file_browser::S_entry::FILE:
         {
            tid = TXT_FILE_DETAILS;
            Cstr_w tmp;
                        //File:
            tmp.Format(L"%: ") <<GetText(TXT_FILE);
            ts.AddWideStringToBuf(buf, tmp, FF_BOLD);
                        //<name> \n
            ts.AddWideStringToBuf(buf, full_name);
            if(e.size.file_size!=dword(-1)){
               ts.AddWideStringToBuf(buf, L"\n");
                        //<size> \n
               Cstr_w size_text = text_utils::MakeFileSizeText(e.size.file_size, false, true);
               ts.AddWideStringToBuf(buf, GetText(TXT_SIZE), FF_BOLD);
               tmp.Format(L": % (% %)") <<size_text <<e.size.file_size <<GetText(TXT_BYTES);
               ts.AddWideStringToBuf(buf, tmp);
            }
            if(e.modify_date.GetSeconds()){
                        //Date:
               ts.AddWideStringToBuf(buf, L"\n");
               ts.AddWideStringToBuf(buf, GetText(TXT_MODIFY_DATE), FF_BOLD);
               ts.AddWideStringToBuf(buf, L": ");

                                 //<date>
               Cstr_w ds, tms;
               GetDateString(e.modify_date, ds);
               tms = text_utils::GetTimeString(e.modify_date, true);
               ts.AddWideStringToBuf(buf, ds);
               ts.AddWideStringToBuf(buf, L", ");
               ts.AddWideStringToBuf(buf, tms);
            }
#ifndef UNIX_FILE_SYSTEM
                                 //hidden / readonly
            if(e.atts&C_file::ATT_HIDDEN){
               ts.AddWideStringToBuf(buf, L"\n");
               ts.AddWideStringToBuf(buf, GetText(TXT_HIDDEN));
            }
            if(e.atts&C_file::ATT_READ_ONLY){
               ts.AddWideStringToBuf(buf, L"\n");
               ts.AddWideStringToBuf(buf, GetText(TXT_READ_ONLY));
            }
#endif
   #ifdef EXPLORER
            if(mod.arch && mod.arch->GetArchiveType()>=1000){
               const C_archive_simulator &as = (C_archive_simulator&)*mod.arch;
               const wchar *fn = as.GetRealFileName(full_name);
               if(fn){
                  ts.AddWideStringToBuf(buf, L"\n");
                  ts.AddWideStringToBuf(buf, GetText(TXT_FILE), FF_BOLD);
                  ts.AddWideStringToBuf(buf, L": ");
                  ts.AddWideStringToBuf(buf, fn);
               }
            }
   #endif
         }
         break;
      default:
         assert(0);
         return;
      }
#ifdef UNIX_FILE_SYSTEM
      {
         ts.AddWideStringToBuf(buf, L"\n");
         ts.AddWideStringToBuf(buf, L"Permissions:\n", FF_BOLD);
         //ts.AddWideStringToBuf(buf, L"usr   grp   oth\n");
         //ts.AddWideStringToBuf(buf, L"rwx   rwx   rwx\n");
         static const char rwx[] = "rwx";
         static const wchar *const type[3] = { L"usr", L"grp", L"oth" };
         for(int i=0; i<3; i++){
            ts.AddWideStringToBuf(buf, type[i]);
            ts.AddWideStringToBuf(buf, L": ");
            for(int b=0; b<3; b++){
               int bit = C_file::ATT_P_USR_R>>(i*3+b);
               bool set = (e.atts&bit);
               //buf.push_back(set ? '1' : '0');
               buf.push_back(set ? rwx[b] : '-');
            }
            //ts.AddWideStringToBuf(buf, L"   ");
            ts.AddWideStringToBuf(buf, L"   ");
         }
      }
#endif
   }
   CreateFormattedMessage(*this, GetText(tid), buf);
}

//----------------------------

bool C_client_file_mgr::FileBrowser_IsFileMarked(const C_mode_file_browser &mod, const C_mode_file_browser::S_entry &e, int *sel_indx){

   int ei = &e - mod.entries.begin();

   for(int i=mod.marked_files.size(); i--; ){
      const wchar *sel_n = mod.marked_files[i];
      int e_len = e.name.Length();
      if(mod.browser_mode==mod.MODE_ARCHIVE_EXPLORER && !e.level)
         e_len = 0;

      int sel_l = mod.marked_files[i].Length();

      if(e.type == C_mode_file_browser::S_entry::DIR || e.type == C_mode_file_browser::S_entry::DRIVE){
         --sel_l;
         if(sel_n[sel_l]!='\\')
            continue;
      }else{
         if(sel_n[sel_l-1]=='\\')
            continue;
      }

      if(sel_l < e_len)
         continue;
      if(MemCmp(e.name, sel_n+sel_l-e_len, e_len*sizeof(wchar)))
         continue;
      sel_l -= e_len;

      int level = e.level;
      int ei1 = ei;
      while(level--){
                              //must end with '\'
         if(!sel_l || sel_n[sel_l-1]!='\\')
            break;
         --sel_l;
                              //find upper level
         while(ei1--){
            if(mod.entries[ei1].level==level)
               break;
         }
         assert(ei1>=0);
         const C_mode_file_browser::S_entry &e1 = mod.entries[ei1];
         e_len = e1.name.Length();
         if(sel_l < e_len)
            break;
         if(MemCmp(e1.name, sel_n+sel_l-e_len, e_len*sizeof(wchar)))
            break;
                              //name matches, continue
         sel_l -= e_len;
      }
      if(mod.browser_mode==mod.MODE_ARCHIVE_EXPLORER && level>=0)
         --level;
      if(level==-1 && !sel_l){
         if(sel_indx) *sel_indx = i;
         return true;
      }
   }
   return false;
}

//----------------------------

void C_client_file_mgr::FileBrowser_MarkCurrFile(C_mode_file_browser &mod, bool toggle, bool mark){

   const C_mode_file_browser::S_entry *e = !mod.entries.size() ? NULL : &mod.entries[mod.selection];
   if(!e || e->type==e->DRIVE || e->type==e->ARCHIVE)
      return;

   switch(e->type){
   case C_mode_file_browser::S_entry::FILE:
      if(!(mod.flags&mod.FLG_ACCEPT_FILE))
         return;
      break;
   default:
      if(!(mod.flags&mod.FLG_ACCEPT_DIR))
         return;
   }
                              //check if selected
   int sel_i;
   if(!FileBrowser_IsFileMarked(mod, *e, &sel_i)){
      if(toggle || mark){
                              //add to selection
         Cstr_w full_path;
         FileBrowser_GetFullName(mod, full_path, -1, true);
                              //append '\' to dirs or drives
         switch(mod.entries[mod.selection].type){
         case C_mode_file_browser::S_entry::DIR:
         case C_mode_file_browser::S_entry::DRIVE:
            full_path<<'\\';
            break;
         }
         mod.marked_files.push_back(full_path);
      }
   }else{
      if(toggle || !mark){
                              //remove from selection
         mod.marked_files[sel_i] = mod.marked_files.back();
         mod.marked_files.pop_back();
      }
   }
}

//----------------------------

void C_client_file_mgr::FileBrowser_MarkOnSameLevel(C_mode_file_browser &mod){

                              //mark all on the same level
   int curr_sel = mod.selection;
   int level = mod.entries[curr_sel].level;
   for(mod.selection=curr_sel; mod.selection<mod.entries.size(); mod.selection++){
      const C_mode_file_browser::S_entry &e = mod.entries[mod.selection];
      if(e.level < level)
         break;
      if(e.level == level)
         FileBrowser_MarkCurrFile(mod, false, true);
   }
   for(mod.selection=curr_sel; mod.selection--; ){
      const C_mode_file_browser::S_entry &e = mod.entries[mod.selection];
      if(e.level < level)
         break;
      if(e.level == level)
         FileBrowser_MarkCurrFile(mod, false, true);
   }
   mod.selection = curr_sel;
}

//----------------------------

void C_client_file_mgr::FileBrowser_CleanMarked(C_mode_file_browser &mod){

   for(int i=mod.marked_files.size(); i--; ){
      const Cstr_w s = mod.marked_files[i];
                              //check if any other files are children of this file, or this file is child of others
      for(int j=mod.marked_files.size(); --j > i; ){
         const Cstr_w &s1 = mod.marked_files[j];
         if(s.Length() < s1.Length()){
            if(IsFileNameInFolder(s1, s)){
                              //remove s1
               mod.marked_files[j] = mod.marked_files.back();
               mod.marked_files.pop_back();
            }
         }else{
            if(IsFileNameInFolder(s, s1)){
                              //remove s
               mod.marked_files[i] = mod.marked_files.back();
               mod.marked_files.pop_back();
               break;
            }
         }
      }
   }
}

//----------------------------
#if defined USE_MOUSE && (defined MAIL || defined EXPLORER)
//----------------------------

void C_client_file_mgr::FileBrowser_SetButtonsState(C_mode_file_browser &mod) const{

   mod.enabled_buttons = 0;
   if((mod.flags&(mod.FLG_ALLOW_COPY_MOVE|mod.FLG_ALLOW_DELETE)) && mod.entries.size()){
      const C_mode_file_browser::S_entry &e = mod.entries[mod.selection];
      bool marked = (mod.marked_files.size()!=0);
      mod.enabled_buttons |= 3;
      if((marked || e.type!=e.DRIVE) && (mod.flags&mod.FLG_ALLOW_DELETE))
         mod.enabled_buttons |= 8;
   }
}

//----------------------------
#endif
//----------------------------
#ifdef EXPLORER

bool C_client_file_mgr::FileBrowserEditFile(C_mode_file_browser &mod, bool is_new_file){

   if(mod.browser_mode!=mod.MODE_EXPLORER)
      return false;
   const C_mode_file_browser::S_entry *e = !mod.entries.size() ? NULL : &mod.entries[mod.selection];
   if(!e || e->type!=e->FILE)
      return false;
   Cstr_w fn;
   FileBrowser_GetFullName(mod, fn, -1, true);
   ((C_explorer_client&)*this).CreateTextEditMode(fn, e->name, is_new_file);
   return true;
}

//----------------------------

bool C_client_file_mgr::FileBrowser_OpenInHexViewer(C_mode_file_browser &mod){

   const C_mode_file_browser::S_entry &e = mod.entries[mod.selection];
   if(e.type!=e.FILE)
      return false;
   Cstr_w full_name;
   FileBrowser_GetFullName(mod, full_name);
   return reinterpret_cast<C_explorer_client*>(this)->SetModeHexEdit(full_name, mod.arch, false);
}

//----------------------------

void C_client_file_mgr::StartCreateZip(C_mode_file_browser &mod, bool move){

   FileBrowser_SaveLastPath(mod);
   FileBrowser_CleanMarked(mod);
   Cstr_w suggest_name;
   bool no_sel = (!mod.marked_files.size());
   if(no_sel){
      FileBrowser_MarkCurrFile(mod, false, true);
      if(!mod.marked_files.size())
         return;
   }
   if(mod.marked_files.size()<=1){
      suggest_name = mod.marked_files.front();
      if(suggest_name[suggest_name.Length()-1]=='\\')
         suggest_name = suggest_name.Left(suggest_name.Length()-1);
      suggest_name = file_utils::GetFileNameNoPath(suggest_name);
      const wchar *ext = text_utils::GetExtension(suggest_name);
      if(ext)
         suggest_name = suggest_name.Left(suggest_name.Length()-StrLen(ext)-1);
      suggest_name<<L".zip";
   }else
      suggest_name = L"Files.zip";

   C_mode_file_browser &mod_1 = SetModeFileBrowser_GetSaveFileName(suggest_name,
      (C_mode_file_browser::t_OpenCallback)(!move ? &C_client_file_mgr::CopyFilesToZip : &C_client_file_mgr::MoveFilesToZip),
      TXT_CHOOSE_ZIP_LOCATION, NULL);
   mod_1.flags &= ~(mod.FLG_ALLOW_DELETE);
   mod_1.marked_files = mod.marked_files;
   if(no_sel)
      mod.marked_files.clear();
   RedrawScreen();
}

//----------------------------

#endif//EXPLORER
//----------------------------

#if defined __SYMBIAN32__ || defined _DEBUG
struct S_thumbnail{
   Cstr_w fname;
   S_point sz;
};

// Search for all file's thumbnails inside of _PAlbTN folder.
static void GetImageThumbnails(const wchar *dir, const Cstr_w &fname, C_vector<S_thumbnail> &thumb_info){

   C_dir d;
   if(d.ScanBegin(dir)){
      while(true){
         dword atts;
         Cstr_w n = d.ScanGet(&atts);
         if(!n.Length())
            break;
         if(atts&C_file::ATT_DIRECTORY){
            Cstr_w dir1; dir1<<dir <<'\\' <<n;
            GetImageThumbnails(dir1, fname, thumb_info);
         }else{
            if(fname.Length()<n.Length() && fname==n.Left(fname.Length())){
               Cstr_c rest; rest.Copy(n+fname.Length());
               const char *cp = rest;
               if(*cp++=='_'){
                  S_thumbnail t;
                  if(text_utils::ScanDecimalNumber(cp, t.sz.x) && ToLower(*cp++)=='x' && text_utils::ScanDecimalNumber(cp, t.sz.y)){
                     t.fname<<dir <<'\\' <<n;
                     thumb_info.push_back(t);
                  }
               }
            }
         }
      }
   }
}

#endif
//----------------------------

C_smart_ptr<C_image> C_client_file_mgr::CreateImageThumbnail(const wchar *fn, int limit_sx, int limit_sy, const C_zip_package *dta) const{

   C_smart_ptr<C_image> img = C_image::Create(*this);
   img->Release();
   {
                              //open from exif thumbnail
      Cstr_w ext = text_utils::GetExtension(fn);
      ext.ToLower();
      C_file_zip fl;
      if(ext==L"jpg" && fl.Open(fn, dta)){
                              //try to find exif data
         for(dword i=0; i<0x800ul; i++){
            fl.Seek(i);
            dword id;
            if(!fl.ReadDword(id))
               break;
            if(id!=FOUR_CC('E','x','i','f'))
               continue;
            word tmp;
            if(fl.ReadWord(tmp) && !tmp && fl.ReadWord(tmp) && tmp==0x4949){
                              //exif found
               fl.Seek(fl.Tell()+2);
               dword offs;
               fl.ReadDword(offs);
               fl.Seek(fl.Tell()+offs-8);
               word num_entries;
               fl.ReadWord(num_entries);
               fl.Seek(fl.Tell()+num_entries*12);
               fl.ReadDword(offs);
               offs += i + 6;
               if(offs<fl.GetFileSize()){
                  fl.Seek(offs);
                  fl.ReadWord(num_entries);
                  dword thumbnail_offset = 0;
#ifdef _DEBUG
                  dword thumbnail_length, thumbnail_width;
#endif

                  while(num_entries--){
                     word tag, format;
                     dword components;
                     fl.ReadWord(tag);
                     fl.ReadWord(format);
                     fl.ReadDword(components);
                     dword id1;
                     fl.ReadDword(id1);
                     switch(tag){
                     case 0x201: //TAG_THUMBNAIL_OFFSET
                        if(format==4)
                           thumbnail_offset = id1 + i+6;
                        break;
#ifdef _DEBUG
                     case 0x202: //TAG_THUMBNAIL_LENGTH
                        if(format==4)
                           thumbnail_length = id1;
                        break;
                     case 0x100: //TAG_THUMBNAIL_WIDTH
                        if(format==4)
                           thumbnail_width = id1;
                        break;
#endif
                     }
                  }
                  if(thumbnail_offset)
                     img->Open(fn, limit_sx, limit_sy, C_fixed::One(), dta, NULL, thumbnail_offset);
               }
            }
            break;
         }
      }
   }

#if defined __SYMBIAN32__// || defined _DEBUG
   if(!img->SizeX() && !dta){
                              //try to load from system thumbnail
      Cstr_w s_fn(fn);
      Cstr_w dir = file_utils::GetFilePath(s_fn);
      dir<<L"_PAlbTN";
      dword atts;
      if(C_file::GetAttributes(dir, atts)){
                                 //determine possible thumbnails
         C_vector<S_thumbnail> thumb_info;
         GetImageThumbnails(dir, file_utils::GetFileNameNoPath(s_fn), thumb_info);
         while(thumb_info.size()){
                                 //try to choose dimension best matching our max rect
            int best_i = -1, best_delta = 0x7fffffff;
            for(int i=thumb_info.size(); i--; ){
               const S_thumbnail &t = thumb_info[i];
               int d = t.sz.x*t.sz.y - limit_sx*limit_sy;
               if(d<0)
                  d = -d*4;
               if(best_delta>d){
                  best_delta = d;
                  best_i = i;
               }
            }
            if(best_i!=-1){
                              //try to load the image
               const S_thumbnail &t = thumb_info[best_i];
               if(!img->Open(t.fname, limit_sx, limit_sy))
                  break;
                              //failed, try with other files
               thumb_info.remove_index(best_i);
            }
         }
      }
   }
#endif
   if(!img->SizeX()){
                              //open directly from image
      img->Open(fn, limit_sx, limit_sy, dta);
   }
   if(!img->SizeX())
      img = NULL;
   return img;
}

//----------------------------

C_smart_ptr<C_image> C_client_file_mgr::CreateImageThumbnail(C_mode_file_browser &mod, const wchar *fn){

   return CreateImageThumbnail(fn, fdb.cell_size_x*7, fdb.cell_size_x*6, mod.arch);
}

//----------------------------

void C_client_file_mgr::C_mode_file_browser::Tick(dword time, bool &redraw){

   if(timer){
      if(timer_need_refresh//>0
         ){
         app.FileBrowser_RefreshEntries(*this);
         redraw = true;
         timer_need_refresh = false;
      }
      if(timer_file_details){
         timer_file_details = false;
         if(entries.size()){
            const C_mode_file_browser::S_entry &e = entries[selection];
            switch(e.type){
            case S_entry::FILE:
               {
                  E_FILE_TYPE ft = app.DetermineFileType(text_utils::GetExtension(e.name));
                  switch(ft){
                  case FILE_TYPE_IMAGE:
                     {
                        Cstr_w fn;
                        app.FileBrowser_GetFullName(*this, fn);
                        img_thumbnail = app.CreateImageThumbnail(*this, fn);
                        if(img_thumbnail)
                           redraw = true;
                     }
                     break;
#ifdef AUDIO_PREVIEW
                  case FILE_TYPE_AUDIO:
                     if(!audio_preview.active && (flags&FLG_AUDIO_PREVIEW)){
                        AudioPreviewEnable();
                        redraw = true;
                     }
                     break;
#endif
                  }
               }
               break;
            }
         }
      }
#ifdef AUDIO_PREVIEW
      if(audio_preview.plr){
         if(audio_preview.plr->IsDone()){
            audio_preview.plr = NULL;
            redraw = true;
         }else{
            int p = audio_preview.plr->GetPlayPos();
            if(audio_preview.progress.pos!=p){
               audio_preview.progress.pos = p;
               redraw = true;
            }
         }
      }else
#endif
         DeleteTimer();
   }
}

//----------------------------

void C_client_file_mgr::ProcessMenu(C_mode_file_browser &mod, int itm, dword menu_id){

#ifdef EXPLORER
   C_explorer_client *this_explorer = reinterpret_cast<C_explorer_client*>(this);
#endif
   switch(itm){
#ifdef EXPLORER
   case TXT_FILE:
      {
         mod.menu = mod.CreateMenu();
         const C_mode_file_browser::S_entry *e = !mod.entries.size() ? NULL : &mod.entries[mod.selection];

         mod.menu->AddItem((mod.flags&mod.FLG_SELECT_OPTION) ? TXT_SELECT : TXT_OPEN, 0, ok_key_name);
         if(!(mod.flags&mod.FLG_SELECT_OPTION)){
            mod.menu->AddItem(TXT_OPEN_BY_SYSTEM, (e && e->type==e->FILE) ? 0 : C_menu::DISABLED);
            if(mod.browser_mode==mod.MODE_EXPLORER){
               mod.menu->AddItem(TXT_HEX_VIEWER, (e && e->type==e->FILE) ? 0 : C_menu::DISABLED, "[3]", "[H]");
               mod.menu->AddItem(TXT_EDIT, (e && e->type==e->FILE) ? 0 : C_menu::DISABLED, "[8]", "[I]");
            }
         }
                           //extract
         if(mod.browser_mode==mod.MODE_ARCHIVE_EXPLORER){
            if(!(mod.flags&mod.FLG_ALLOW_COPY_MOVE))
               mod.menu->AddItem(mod.arch->GetArchiveType()>=1000 ? TXT_SAVE_AS : TXT_EXTRACT, (e && (e->type==e->FILE || (mod.flags&mod.FLG_ALLOW_MARK_FILES))) ? 0 : C_menu::DISABLED, "[4]", "[X]");
            mod.menu->AddItem(TXT_HEX_VIEWER, (e && e->type==e->FILE) ? 0 : C_menu::DISABLED, "[3]");
         }
         if(mod.flags&mod.FLG_ALLOW_DETAILS){
                           //file details
            bool allow_details = (e || mod.marked_files.size());
            mod.menu->AddItem(TXT_SHOW_DETAILS, allow_details ? 0 : C_menu::DISABLED, "[5]", "[E]");
         }
#ifndef UNIX_FILE_SYSTEM
         if(mod.flags&mod.FLG_ALLOW_ATTRIBUTES){
                           //attributes
            bool allow_atts = (e && (mod.marked_files.size() || (e->type==e->DIR || e->type==e->FILE)));
            mod.menu->AddItem(TXT_SHOW_ATTRIBUTES, allow_atts ? 0 : C_menu::DISABLED, "[6]", "[T]");
         }
#endif
         if(mod.flags&mod.FLG_ALLOW_DELETE){
                              //delete
            bool allow_delete = (e && e->type!=e->DRIVE && e->type!=e->ARCHIVE);
            mod.menu->AddItem(TXT_DELETE, allow_delete ? 0 : C_menu::DISABLED, delete_key_name, NULL, BUT_DELETE);
         }

         if(mod.flags&mod.FLG_ALLOW_RENAME){
                              //rename
            bool allow_rename = (e && e->type!=e->DRIVE);
            mod.menu->AddItem(TXT_RENAME, allow_rename ? 0 : C_menu::DISABLED, "[7]", "[R]");
            if(mod.flags&mod.FLG_ALLOW_MAKE_DIR){
                                 //make dir
               mod.menu->AddItem(TXT_MAKE_DIR, 0, "[9]", "[F]");
            }
         }
         if(mod.browser_mode==mod.MODE_EXPLORER && !(mod.flags&mod.FLG_SELECT_OPTION)){
            mod.menu->AddItem(TXT_NEW_TEXT_FILE);
         }
      }
      PrepareMenu(mod.menu);
      break;

   case TXT_FIND:
      mod.menu = mod.CreateMenu();
      mod.menu->AddItem(TXT_FIND_FILE);
      mod.menu->AddItem(TXT_FIND_NEXT, this_explorer->config_xplore.find_files_text.Length() ? 0 : C_menu::DISABLED);
      PrepareMenu(mod.menu);
      break;

   case TXT_FIND_FILE:
      {
         class C_text_entry: public C_text_entry_callback{
            C_explorer_client &app;
            C_client_file_mgr::C_mode_file_browser &mod;

            virtual void TextEntered(const Cstr_w &txt){
               app.FileBrowserFindTextEntered(mod, txt);
            }
         public:
            C_text_entry(C_explorer_client &a, C_client_file_mgr::C_mode_file_browser &m): app(a), mod(m){}
         };
         CreateTextEntryMode(*this, TXT_FIND_FILE, new(true) C_text_entry((C_explorer_client&)*this, mod), true, 50, this_explorer->config_xplore.find_files_text);
      }
      return;

   case TXT_FIND_NEXT:
      this_explorer->FileBrowserFindTextEntered(mod, this_explorer->config_xplore.find_files_text);
      return;

   case TXT_ZIP:
      {
         mod.menu = mod.CreateMenu();
         const C_mode_file_browser::S_entry *e = !mod.entries.size() ? NULL : &mod.entries[mod.selection];
         bool allow_zip = (mod.marked_files.size() || (e && !(e->type==e->DRIVE || e->type==e->ARCHIVE)));
         mod.menu->AddItem(TXT_COPY_TO_ZIP, allow_zip ? 0 : C_menu::DISABLED
#ifdef S60
            , GetShiftShortcut("[%+Send]")
#endif
            );
         mod.menu->AddItem(TXT_MOVE_TO_ZIP, allow_zip ? 0 : C_menu::DISABLED);
         PrepareMenu(mod.menu);
      }
      break;

   case TXT_COPY_TO_ZIP:
   case TXT_MOVE_TO_ZIP:
      StartCreateZip(mod, itm==TXT_MOVE_TO_ZIP);
      return;

#ifndef UNIX_FILE_SYSTEM
   case TXT_SHOW_ATTRIBUTES:
      this_explorer->SetModeEditAttributes(mod);
      return;
#endif

   case TXT_NEW_TEXT_FILE:
      FileBrowser_BeginMakeNewFile(mod, false);
      break;

   case TXT_HEX_VIEWER:
      if(FileBrowser_OpenInHexViewer(mod))
         return;
      break;

   case TXT_TOOLS:
      mod.menu = mod.CreateMenu();
      mod.menu->AddItem(TXT_CONFIGURATION, 0, "[0]", "[G]");
#if !(defined UIQ && !defined __SYMBIAN_3RD__) && !(defined _WIN32_WCE)
      mod.menu->AddItem(TXT_MESSAGES, 0, "[4]", "[X]");
#endif
#ifdef S60
      mod.menu->AddItem(TXT_FILE_ASSOCIATIONS);
#endif
      mod.menu->AddItem(TXT_DEVICE_INFO);
      if(system::CanRebootDevice())
         mod.menu->AddItem(TXT_RESTART_DEVICE);
#if !(defined __SYMBIAN_3RD__)
      mod.menu->AddItem(TXT_PROCESSES);
#if !(defined _WIN32_WCE)
      mod.menu->AddItem(TXT_TASKS);
#endif
#endif
      mod.menu->AddItem(TXT_ABOUT);
      PrepareMenu(mod.menu);
      break;

   case TXT_CONFIGURATION:
      this_explorer->SetModeConfig();
      return;

   case TXT_DEVICE_INFO:
      this_explorer->SetModeDeviceInfo();
      return;

   case TXT_RESTART_DEVICE:
      CreateQuestion(*this, TXT_RESTART_DEVICE, GetText(TXT_Q_ARE_YOU_SURE), new(true) C_explorer_client::C_question_restart((C_explorer_client&)*this), true);
      return;

   case TXT_PROCESSES:
      this_explorer->SetModeItemList(C_explorer_client::C_mode_item_list::PROCESSES);
      return;

   case TXT_TASKS:
      this_explorer->SetModeItemList(C_explorer_client::C_mode_item_list::TASKS);
      return;

   case TXT_FILE_ASSOCIATIONS:
      {
         const C_mode_file_browser::S_entry *e = !mod.entries.size() ? NULL : &mod.entries[mod.selection];
         this_explorer->SetModeFileAssociations(e && e->type==e->FILE ? text_utils::GetExtension(e->name) : NULL);
      }
      return;

   case TXT_GO_TO_FILE:
      this_explorer->FileBrowser_GoToRealFile(mod);
      return;

   case TXT_MESSAGES:
      FileBrowser_SaveLastPath(mod);
      C_client_file_mgr::SetModeFileBrowser_Archive(this, L"*msgs*", GetText(TXT_MESSAGES));
      return;

   case TXT_QUICK_FOLDERS:
      {
         mod.menu = mod.CreateMenu();
         this_explorer->LoadQuickFolders();
         for(int i=0; i<10; i++){
            Cstr_w s = this_explorer->quick_folders[i];
#ifdef UNIX_FILE_SYSTEM
            s = file_utils::ConvertPathSeparatorsToUnix(s);
#endif
            Cstr_c sh;
            sh.Format("[%%+%]") <<((i+1)%10);
            sh = GetShiftShortcut(sh);
            mod.menu->AddItem(s.Length() ? (const wchar*)s : GetText(TXT_NOT_SET), s.Length() ? 0 : C_menu::DISABLED, sh);
         }
         mod.menu->AddSeparator();
         mod.menu->AddItem(TXT_EDIT_FOLDERS);
         PrepareMenu(mod.menu);
      }
      break;

   case TXT_EDIT_FOLDERS:
      this_explorer->SetModeItemList(C_explorer_client::C_mode_item_list::QUICK_DIRS);
      return;

   default:
      if(itm>=0x10000){
         itm -= 0x10000;
         if(itm<10)
            FileBrowser_GoToPath(mod, this_explorer->quick_folders[itm], true);
      }
      break;

   case TXT_ABOUT:
      CreateModeAbout(*this, TXT_ABOUT, app_name, L"logo.png", VERSION_HI, VERSION_LO, VERSION_BUILD, this);
      return;

#endif//EXPLORER

   case TXT_OPEN:
      {
         const C_mode_file_browser::S_entry &e = mod.entries[mod.selection];
         switch(e.type){
         case C_mode_file_browser::S_entry::DIR:
         case C_mode_file_browser::S_entry::DRIVE:
         case C_mode_file_browser::S_entry::ARCHIVE:
            switch(e.expand_status){
            case C_mode_file_browser::S_entry::COLLAPSED: FileBrowser_ExpandDir(mod); break;
            case C_mode_file_browser::S_entry::EXPANDED: FileBrowser_CollapseDir(mod); break;
            }
            break;
         default:
            FileBrowser_OpenFile(mod);
            return;
         }
      }
      break;

   case TXT_SELECT:
      switch(mod.browser_mode){
      case C_mode_file_browser::MODE_GET_SAVE_FILENAME:
         FileBrowser_EnterSaveAsFilename(mod);
         break;
#if defined MAIL || defined WEB || defined EXPLORER
      case C_mode_file_browser::MODE_COPY:
      case C_mode_file_browser::MODE_MOVE:
         FileBrowser_CopyMove(mod, (mod.browser_mode==C_mode_file_browser::MODE_MOVE), false);
         return;
#endif
      default:
         if(FileBrowser_OpenFile(mod))
            return;
      }
      break;

#if defined MAIL || defined WEB || defined EXPLORER
   case TXT_OPEN_BY_SYSTEM:
      {
#ifdef EXPLORER
         if(mod.arch){
            this_explorer->FileBrowser_ExtractToTempAndOpen(mod);
            return;
         }
#endif//EXPLORER
         Cstr_w full_name;
         FileBrowser_GetFullName(mod, full_name);
         if(!OpenFileBySystem(full_name)){
#ifdef UNIX_FILE_SYSTEM
            full_name = file_utils::ConvertPathSeparatorsToUnix(full_name);
#endif
            ShowErrorWindow(TXT_ERR_NO_VIEWER, full_name);
            return;
         }
      }
      break;
#endif

   case TXT_SHOW_DETAILS:
      FileBrowser_ShowDetails(mod);
      return;

   case TXT_RENAME:
      mod.BeginRename();
      break;
   case TXT_DELETE:
      FileBrowser_BeginDelete(mod);
      return;
   case TXT_MAKE_DIR:
      FileBrowser_BeginMakeNewFile(mod, true);
      break;

#if defined MAIL || defined WEB || defined EXPLORER
   case TXT_EXTRACT:
#ifdef EXPLORER
   case TXT_SAVE_AS:
#endif
      FileBrowser_ExtractFile(mod);
      return;
#endif

   case TXT_EDIT:
      {
#ifdef EXPLORER
         if(!mod.menu){
            if(FileBrowserEditFile(mod, false))
               return;
            break;
         }
#else
         const C_mode_file_browser::S_entry *e = !mod.entries.size() ? NULL : &mod.entries[mod.selection];
#endif
         //bool can_cc = (e && e->type==e->FILE);
         mod.menu = mod.CreateMenu();

#ifndef EXPLORER
         if(mod.flags&mod.FLG_ALLOW_DELETE){
            bool allow_delete = (mod.marked_files.size() || (e && e->type!=e->DRIVE));
            mod.menu->AddItem(TXT_DELETE, allow_delete ? 0 : C_menu::DISABLED, delete_key_name, NULL, BUT_DELETE);
         }
#endif
#if defined MAIL || defined WEB || defined EXPLORER
         if(mod.flags&mod.FLG_ALLOW_COPY_MOVE){
            mod.menu->AddItem(_TXT_COPY, 0, "[1]", "[C]", BUT_COPY);
            mod.menu->AddItem(TXT_MOVE, 0, "[2]", "[M]", BUT_MOVE);
         }
#endif
         PrepareMenu(mod.menu);
      }
      break;

#if defined MAIL || defined WEB || defined EXPLORER
   case _TXT_COPY:
   case TXT_MOVE:
      FileBrowser_CopyMove(mod, (itm==TXT_MOVE));
      return;
#endif

   case TXT_MARK:
      {
         const C_mode_file_browser::S_entry *e = !mod.entries.size() ? NULL : &mod.entries[mod.selection];

         mod.menu = mod.CreateMenu();

         bool allow_mark = (e && e->type!=e->DRIVE && e->type!=e->ARCHIVE);
         mod.menu->AddItem(TXT_MARK_SELECTED, allow_mark ? 0 : C_menu::DISABLED, GetShiftShortcut("[%+OK]"));
         mod.menu->AddItem(TXT_MARK_ALL, allow_mark ? 0 : C_menu::DISABLED, GetShiftShortcut("[%+Right]"));
         mod.menu->AddItem(TXT_MARK_NONE, mod.marked_files.size() ? 0 : C_menu::DISABLED, GetShiftShortcut("[%+Left]"));
         PrepareMenu(mod.menu);
      }
      break;

   case TXT_MARK_ALL:
      FileBrowser_MarkOnSameLevel(mod);
      break;

   case TXT_MARK_SELECTED:
      FileBrowser_MarkCurrFile(mod, true, false);
      break;

   case TXT_MARK_NONE:
      mod.marked_files.clear();
      break;

      /*
   case TXT_SCROLL:
      mod.menu = mod.CreateMenu();
      mod.menu->AddItem(TXT_PAGE_UP, 0, "[*]", "[Q]");
      mod.menu->AddItem(TXT_PAGE_DOWN, 0, "[#]", "[A]");
      PrepareMenu(mod.menu);
      break;

   case TXT_PAGE_UP:
   case TXT_PAGE_DOWN:
      FileBrowser_ScrollPage(mod, (itm==TXT_PAGE_UP), false);
      break;
      */

#if (defined MAIL || defined WEB) && !defined UNIX_FILE_SYSTEM
   case TXT_SHOW:
      mod.menu = mod.CreateMenu();
      mod.menu->AddItem(TXT_HIDDEN_FILES, (config.flags&config.CONF_BROWSER_SHOW_HIDDEN) ? C_menu::MARKED : 0);
      mod.menu->AddItem(TXT_ROM_DRIVES, (config.flags&config.CONF_BROWSER_SHOW_ROM) ? C_menu::MARKED : 0);
      PrepareMenu(mod.menu);
      break;

   case TXT_HIDDEN_FILES:
   case TXT_ROM_DRIVES:
      {
         config.flags ^= (itm==TXT_HIDDEN_FILES) ? config.CONF_BROWSER_SHOW_HIDDEN : config.CONF_BROWSER_SHOW_ROM;
         SaveConfig();
                           //restart browser
         FileBrowser_SaveLastPath(mod);
         C_mode_file_browser::E_MODE bm = mod.browser_mode;
         CloseMode(mod, false);
         SetModeFileBrowser(bm);
         return;
      }
      break;
#endif

#if defined MAIL || defined WEB || defined EXPLORER
   case TXT_SEND_BY:
      mod.menu = mod.CreateMenu();
      if(C_obex_file_send::CanSendByBt())
         mod.menu->AddItem(TXT_SEND_BY_BT, 0, send_key_name);
      if(C_obex_file_send::CanSendByIr())
         mod.menu->AddItem(TXT_SEND_BY_IR);
      PrepareMenu(mod.menu);
      break;

   case TXT_SEND_BY_BT:
   case TXT_SEND_BY_IR:
      if(FileBrowser_SendBy(mod, itm==TXT_SEND_BY_BT ? C_obex_file_send::TR_BLUETOOTH : C_obex_file_send::TR_INFRARED))
         return;
      break;
#endif//MAIL || WEB

   case TXT_BACK:
   case TXT_EXIT:
      FileBrowser_Close(mod);
      return;
   }
}

//----------------------------

dword C_client_file_mgr::C_mode_file_browser::GetPageSize(bool up){

   int max = C_mode_list<C_client_file_mgr>::GetPageSize(up);
   int want_sel = selection;
   const int level = entries[selection].level;
   if(up){
      while(max-- && want_sel && entries[want_sel-1].level==level)
         --want_sel;
   }else{
      while(max-- && want_sel<entries.size()-1 && entries[want_sel+1].level==level)
         ++want_sel;
   }
   return Max(1, Abs(want_sel-selection));
}

//----------------------------

void C_client_file_mgr::C_mode_file_browser::SelectionChanged(int prev_selection){

   if((shift_mark!=-1 || drag_mark!=-1) && (flags&FLG_ALLOW_MARK_FILES)){
      bool mark = shift_mark!=-1 ? bool(shift_mark) : bool(drag_mark);
      int new_sel = selection;
      selection = prev_selection;
      while(selection!=new_sel){
         app.FileBrowser_MarkCurrFile(*this, false, mark);
         if(selection<new_sel)
            ++selection;
         else
            --selection;
      }
      app.FileBrowser_MarkCurrFile(*this, false, mark);
   }
   ChangeSelection(selection);
}

//----------------------------

void C_client_file_mgr::C_mode_file_browser::ProcessInput(S_user_input &ui, bool &redraw){

   if(ui.key_bits&GKEY_SHIFT){
      if(shift_mark==-1 && entries.size()){
         shift_mark = !app.FileBrowser_IsFileMarked(*this, entries[selection]);
      }
   }else
      shift_mark = -1;

   if(!te_rename){
#ifdef USE_MOUSE
      if(drag_mark!=-1 && (ui.mouse_buttons&MOUSE_BUTTON_1_DRAG)){
         if(ui.CheckMouseInRect(rc)){
            int line = (ui.mouse.y + sb.pos - rc.y) / entry_height;
            if(line < entries.size() && line!=selection){
                              //mark files while dragging
               int ps = selection;
               selection = line;
               SelectionChanged(ps);
               redraw = true;
            }
         }
      }
#endif
#ifdef AUDIO_PREVIEW
      if(audio_preview.active && ui.mouse_buttons && ui.CheckMouseInRect(audio_preview.rc)){
         if(!audio_preview.err){
            for(int i=0; i<audio_preview.BUTTON_LAST; i++){
               C_button &but = audio_preview.buttons[i];
               if(but.ProcessMouse(ui, redraw, &app)){
                  switch(i){
                  case C_audio_preview::BUTTON_PLAY:
                     AudioPreviewTogglePlay();
                     break;
                  case C_audio_preview::BUTTON_VOL_DOWN:
                  case C_audio_preview::BUTTON_VOL_UP:
                     ChangeAudioPreviewVolume((i==C_audio_preview::BUTTON_VOL_UP));
                     break;
                  }
               }
            }
         }
      }else
#endif
      if(drag_mark==-1)
         ProcessInputInList(ui, redraw);
   }

#ifdef USE_MOUSE
   if(app.HasMouse())
   if(!app.ProcessMouseInSoftButtons(ui, redraw, false)){
      if(te_rename){
         if(app.ProcessMouseInTextEditor(*te_rename, ui))
            redraw = true;
      }else{
#if (defined MAIL || defined EXPLORER)
         app.FileBrowser_SetButtonsState(*this);
         int but = app.TickBottomButtons(ui, redraw, enabled_buttons);
         if(but!=-1){
            switch(but){
            case 0: app.FileBrowser_CopyMove(*this, false); return;
            case 1: app.FileBrowser_CopyMove(*this, true); return;
            case 3: app.FileBrowser_BeginDelete(*this); return;
            }
         }else
#endif
#ifdef AUDIO_PREVIEW
         if(audio_preview.active && ui.mouse_buttons && ui.CheckMouseInRect(audio_preview.rc)){
            if(audio_preview.err && (ui.mouse_buttons&MOUSE_BUTTON_1_DOWN)){
               audio_preview.Close();
               redraw = true;
            }
         }else
#endif
         if(ui.CheckMouseInRect(rc)){
            int line = (ui.mouse.y + sb.pos - rc.y) / entry_height;
            if(line < entries.size()){
               const S_entry &e = entries[line];
               if(ui.mouse_buttons&MOUSE_BUTTON_1_DOWN){
                  if((flags&FLG_ALLOW_MARK_FILES) && drag_mark==-1){
                     const int mark_x = GetMaxX() - app.fdb.cell_size_x*3;
                     if(ui.mouse.x >= mark_x){
                        drag_mark = !app.FileBrowser_IsFileMarked(*this, entries[selection]);
                        app.FileBrowser_MarkCurrFile(*this, false, drag_mark);
                        redraw = true;
                     }
                  }
                  /*
                  const int name_x = rc.x + 2 + e.level * LEVEL_OFFSET + expand_box_size + icon_sx + app.fdb.letter_size_x/2;
                  if(ui.mouse.x < name_x){
                     ChangeSelection(line);
                     if(flags&FLG_ALLOW_MARK_FILES){
                        app.FileBrowser_MarkCurrFile(*this, true, false);
                        sel_dragging = app.FileBrowser_IsFileMarked(*this, entries[selection]);
                        redraw = true;
                     }else{
                        //expand/collapse entry
                        switch(e.type){
                        case S_entry::DIR:
                        case S_entry::DRIVE:
                           switch(e.expand_status){
                        case S_entry::COLLAPSED: ui.key = K_CURSORRIGHT; break;
                        case S_entry::EXPANDED: ui.key = K_CURSORLEFT; break;
                           }
                           break;
                        }
                     }
                  }else{
                     sel_dragging = -1;
                     touch_down_selection = selection;
                     if(selection != line){
                        ChangeSelection(line);
                        redraw = true;
                     }
                     */
#if defined MAIL || defined EXPLORER
                     if(e.type!=e.ARCHIVE && !arch && drag_mark==-1){
                        menu = app.CreateTouchMenu();
                        switch(e.type){
                        case S_entry::DRIVE:
                           menu->AddItem(TXT_MAKE_DIR);
                           menu->AddSeparator();
                           menu->AddSeparator();
                           menu->AddSeparator();
                           if(flags&FLG_SELECT_OPTION)
                              menu->AddItem(TXT_SELECT);
                           break;
                        case S_entry::DIR:
                           {
                              menu->AddItem(TXT_MAKE_DIR);
                              menu->AddItem(TXT_DELETE, 0, 0, 0, BUT_DELETE);
                              if(!marked_files.size())
                                 menu->AddItem(TXT_RENAME);
                              else
                                 menu->AddSeparator();

#if defined MAIL || defined WEB || (defined EXPLORER && !defined _WIN32_WCE)
                              if((flags&FLG_ALLOW_SEND_BY) && (C_obex_file_send::CanSendByBt() || C_obex_file_send::CanSendByIr()))
                                 menu->AddItem(TXT_SEND_BY, C_menu::HAS_SUBMENU);
                              else
#endif
                                 menu->AddSeparator();
                              if(flags&FLG_SELECT_OPTION)
                                 menu->AddItem(TXT_SELECT);
                              else
                                 menu->AddItem(TXT_SHOW_DETAILS);
                           }
                           break;
                        case S_entry::FILE:
                           if(!marked_files.size())
                              menu->AddItem(TXT_OPEN_BY_SYSTEM);
                           else
                              menu->AddSeparator();
                           menu->AddItem(TXT_DELETE, 0, 0, 0, BUT_DELETE);
                           if(!marked_files.size())
                              menu->AddItem(TXT_RENAME);
                           else
                              menu->AddSeparator();
#if defined MAIL || defined WEB || (defined EXPLORER && !defined _WIN32_WCE)
                           if((flags&FLG_ALLOW_SEND_BY) && (C_obex_file_send::CanSendByBt() || C_obex_file_send::CanSendByIr()))
                              menu->AddItem(TXT_SEND_BY, C_menu::HAS_SUBMENU);
                           else
#endif
                              menu->AddSeparator();
                           menu->AddItem(TXT_SHOW_DETAILS);
                           break;
                        }
                        app.PrepareTouchMenu(menu, ui);
                     }
#endif
                  //}
               }
               if(ui.mouse_buttons&MOUSE_BUTTON_1_UP){
                  if(drag_mark!=-1){
                              //stop drag marking
                     drag_mark = -1;
                     ui.key = 0;
                  }else
                  if(ui.key==K_ENTER){
                     //if((flags&FLG_SELECT_OPTION) && (flags&FLG_ALLOW_MARK_FILES)){
                     const int name_x = rc.x + 2 + e.level * LEVEL_OFFSET + expand_box_size + icon_sx + app.fdb.letter_size_x/2;
                     if(ui.mouse.x < name_x){
                              //expand/collapse when clicked on +- sign
                        switch(e.type){
                        case S_entry::DIR:
                        case S_entry::DRIVE:
                           switch(e.expand_status){
                           case S_entry::COLLAPSED: ui.key = K_CURSORRIGHT; break;
                           case S_entry::EXPANDED: ui.key = K_CURSORLEFT; break;
                           }
                           break;
                        }
                     }
                  }
                  title_touch_down = false;
               }
            }
         }else if(ui.mouse.y < rc.y){
                              //mouse at top bar
            if(ui.mouse_buttons&MOUSE_BUTTON_1_DOWN){
               title_touch_down = true;
            }
            if((ui.mouse_buttons&MOUSE_BUTTON_1_UP) && title_touch_down){
               ui.key = K_CURSORLEFT;
            }
         }
      }
   }
#endif

   if(te_rename){
      C_text_editor &te = *te_rename;
      S_entry &e = entries[selection];
      switch(ui.key){
      case K_RIGHT_SOFT:
      case K_BACK:
      case K_ESC:
         app.FileBrowser_CancelRename(*this);
         redraw = true;
         break;

      case K_ENTER:
      case K_LEFT_SOFT:
         {
            Cstr_w old;
            app.FileBrowser_GetFullName(*this, old);
            Cstr_w txt_buf = te.GetText();
                              //remove dot at end
            while(txt_buf.Length() && (txt_buf[txt_buf.Length()-1]=='.' || text_utils::IsSpace(txt_buf[txt_buf.Length()-1])))
               txt_buf = txt_buf.Left(txt_buf.Length()-1);
            if(!txt_buf.Length())
               break;

            switch(browser_mode){
            case MODE_GET_SAVE_FILENAME:
               if(e.type==e.FILE && !e.name.Length()){
                              //file name entered, try to finish operation
                  save_as_name = txt_buf;
                  app.FileBrowser_FinishGetSaveFileName(*this);
                  return;
               }
                              //flow...
            default:
               {
                  Cstr_w nw;
                  nw = old;
                  int i = nw.FindReverse('\\');
#ifdef NO_FILE_SYSTEM_DRIVES
                  if(i!=0)
#endif
                     ++i;
                  if(i!=(int)nw.Length())
                     nw.At(i) = 0;
                  nw = (const wchar*)nw;
                  nw <<txt_buf;
                  E_TEXT_ID err = TXT_NULL;
                  Cstr_w err_detail;
                  if(!e.name.Length()){
                              //unnamed file, it means we're creating new
                     if(e.type==e.DIR){
                        if(!C_dir::MakeDirectory(nw, false))
                           err = TXT_ERR_CANT_MAKE_DIR;
                     }else{
#ifdef EXPLORER
                              //add txt extension, if not specified
                        bool add_ext = (!text_utils::GetExtension(txt_buf));
                        if(add_ext)
                           nw<<L".txt";
                        C_file fl;
                              //try to open existing file
                        if(fl.Open(nw)){
                           fl.Close();
                           app.FileBrowser_CancelRename(*this);
                           ((C_explorer_client&)app).CreateTextEditMode(nw, file_utils::GetFileNameNoPath(nw), false);
                           return;
                        }
                              //try to create file
                        if(!fl.Open(nw, C_file::FILE_WRITE)){
                           err = TXT_ERROR;
                           err_detail.Format(app.GetText(TXT_ERR_CANT_WRITE_FILE)) <<te.GetText();
                        }else{
                           fl.Close();
                           e.name = txt_buf;
                           e.size.file_size = 0;
                           if(add_ext)
                              e.name<<L".txt";
                           te_rename = NULL;
                           app.FileBrowserEditFile(*this, true);
                           return;
                        }
#else
                        assert(0);
#endif
                     }
                  }else{
                     for(int i=marked_files.size(); i--; ){
                        Cstr_w &m = marked_files[i];
                        if(m==old){
                           m = nw;
                           break;
                        }
                     }
                     img_thumbnail = NULL;
#ifdef AUDIO_PREVIEW
                     audio_preview.Close();
#endif
                     if(!C_file::RenameFile(old, nw))
                        err = TXT_ERR_CANT_RENAME;
                  }
                  if(err==TXT_NULL){
                     e.name = txt_buf;
                     te_rename = NULL;
                     ChangeSelection(selection);
                     app.RedrawScreen();
                  }else
                     app.ShowErrorWindow(err, err_detail.Length() ? (const wchar*)err_detail : te.GetText());
               }
            }
         }
         return;
      }
   }else{
#ifdef EXPLORER
      if((ui.key_bits&GKEY_SHIFT) && text_utils::IsDigit(wchar(ui.key))){
                              //gov to quick folder
         int ii = ui.key-'0';
         if(!ii--) ii = 9;
         C_explorer_client *ec = reinterpret_cast<C_explorer_client*>(&app);
         ec->LoadQuickFolders();
         if(ec->quick_folders[ii].Length()){
            app.FileBrowser_GoToPath(*this, ec->quick_folders[ii], true);
            redraw = true;
         }else{
            //C_explorer_client::C_mode_item_list &mod = 
               ec->SetModeItemList(C_explorer_client::C_mode_item_list::QUICK_DIRS, false);
            selection = ii;
            app.RedrawScreen();
            return;
         }
      }else
#endif
      switch(ui.key){
      case K_RIGHT_SOFT:
      case K_BACK:
      case K_ESC:
#if defined EXPLORER
         if(!GetParent() && (app.config.flags&C_explorer_client::S_config_xplore::CONF_ASK_TO_EXIT)){
            CreateQuestion(app, TXT_EXIT, app.GetText(TXT_Q_ARE_YOU_SURE), new(true) C_explorer_client::C_question_exit((C_explorer_client&)app), true);
            return;
         }
#endif
         app.FileBrowser_Close(*this);
         return;

      case K_LEFT_SOFT:
      case K_MENU:
         {
            const S_entry *e = !entries.size() ? NULL : &entries[selection];
            menu = CreateMenu();

            bool add_sep = true;
#ifdef EXPLORER
            menu->AddItem(TXT_FILE, C_menu::HAS_SUBMENU | (e ? 0 : C_menu::DISABLED));
            if(!(flags&FLG_SELECT_OPTION)){
               if(arch && arch->GetArchiveType()==1001)
                  menu->AddItem(TXT_GO_TO_FILE, (e && e->type==e->FILE) ? 0 : C_menu::DISABLED);
               if(browser_mode==MODE_EXPLORER){
                  menu->AddItem(TXT_FIND, C_menu::HAS_SUBMENU);
                  menu->AddItem(TXT_ZIP, C_menu::HAS_SUBMENU);
               }
            }
            if(flags&FLG_ALLOW_COPY_MOVE){
               bool allow_edit = (marked_files.size() || (e && !(e->type==e->DRIVE || e->type==e->ARCHIVE)));
               menu->AddItem(TXT_EDIT, (allow_edit ? 0 : C_menu::DISABLED) | C_menu::HAS_SUBMENU);
            }
            if(flags&FLG_ALLOW_MARK_FILES)
               menu->AddItem(TXT_MARK, (e ? 0 : C_menu::DISABLED) | C_menu::HAS_SUBMENU);
#else
            if(flags&FLG_SELECT_OPTION){
               bool en = false;
               if(marked_files.size())
                  en = true;
               else
               if(e){
                  if(e->type==e->FILE)
                     en = (flags&FLG_ACCEPT_FILE);
                  else
                     en = (flags&FLG_ACCEPT_DIR);
               }
               menu->AddItem(TXT_SELECT, en ? 0 : C_menu::DISABLED, ok_key_name);
            }else{
               menu->AddItem(TXT_OPEN, 0, ok_key_name);
#if defined MAIL || defined WEB || defined EXPLORER
               if(browser_mode==MODE_EXPLORER)
                  menu->AddItem(TXT_OPEN_BY_SYSTEM, (e && e->type==e->FILE) ? 0 : C_menu::DISABLED);
#endif
            }
            menu->AddSeparator();
            add_sep = false;
#if defined MAIL || defined WEB || defined EXPLORER
            if((flags&FLG_ALLOW_COPY_MOVE) && (flags&FLG_ALLOW_DELETE)){
               bool allow_edit = (marked_files.size() || (e && e->type!=e->DRIVE));
               menu->AddItem(TXT_EDIT, (allow_edit ? 0 : C_menu::DISABLED) | C_menu::HAS_SUBMENU);
               add_sep = true;
            }
#endif
                              //mark menu
            if(flags&FLG_ALLOW_MARK_FILES){
               menu->AddItem(TXT_MARK, (e ? 0 : C_menu::DISABLED) | C_menu::HAS_SUBMENU);
               add_sep = true;
            }
#if defined MAIL || defined WEB || defined EXPLORER
                              //extract
            if(browser_mode==MODE_ARCHIVE_EXPLORER){
               menu->AddItem(TXT_EXTRACT, (e && e->type==e->FILE) ? 0 : C_menu::DISABLED, "[4]", "[X]");
               add_sep = true;
            }
#endif
                              //file details
            if(flags&FLG_ALLOW_DETAILS){
               bool allow_details = (e || marked_files.size());
               menu->AddItem(TXT_SHOW_DETAILS, allow_details ? 0 : C_menu::DISABLED, "[5]", "[E]");
               add_sep = true;
            }
                              //delete
            if(
#if defined MAIL || defined WEB || defined EXPLORER
               !(flags&FLG_ALLOW_COPY_MOVE) &&
#endif
               (flags&FLG_ALLOW_DELETE)){
               bool allow_delete = (e && e->type!=e->DRIVE);
               menu->AddItem(TXT_DELETE, allow_delete ? 0 : C_menu::DISABLED, app.delete_key_name, NULL, BUT_DELETE);
               add_sep = true;
            }
                              //rename
            if(flags&FLG_ALLOW_RENAME){
               bool allow_rename = (e && e->type!=e->DRIVE);
               menu->AddItem(TXT_RENAME, allow_rename ? 0 : C_menu::DISABLED, "[7]", "[R]");
               add_sep = true;
            }
                              //make dir
            if(flags&FLG_ALLOW_MAKE_DIR){
               menu->AddItem(TXT_MAKE_DIR, 0, "[9]", "[F]");
               add_sep = true;
            }
            if(add_sep){
               menu->AddSeparator();
               add_sep = false;
            }
#endif//!EXPLORER

#if defined MAIL && !defined UNIX_FILE_SYSTEM
                              //Show... submenu
            if(flags&FLG_ALLOW_SHOW_SUBMENU){
               menu->AddItem(TXT_SHOW, C_menu::HAS_SUBMENU);
               add_sep = true;
            }
#endif
#if defined MAIL || defined WEB || (defined EXPLORER && !defined _WIN32_WCE)
                              //Send by
            if((flags&FLG_ALLOW_SEND_BY) && (C_obex_file_send::CanSendByBt() || C_obex_file_send::CanSendByIr())){
               bool can_send = (e && e->type!=e->DRIVE);
               menu->AddItem(TXT_SEND_BY, (can_send ? 0 : C_menu::DISABLED) | C_menu::HAS_SUBMENU);
               add_sep = true;
            }
#endif
            /*
            if(!app.HasMouse()){
                              //scroll
               menu->AddItem(TXT_SCROLL, C_menu::HAS_SUBMENU);
               add_sep = true;
            }
            */
#ifdef EXPLORER
            if(browser_mode==MODE_EXPLORER && !(flags&FLG_SELECT_OPTION)){
               menu->AddItem(TXT_QUICK_FOLDERS, C_menu::HAS_SUBMENU);
               menu->AddItem(TXT_TOOLS, C_menu::HAS_SUBMENU);
                  add_sep = true;
            }else if(browser_mode==MODE_COPY || browser_mode==MODE_MOVE || browser_mode==MODE_GET_SAVE_FILENAME){
               menu->AddItem(TXT_QUICK_FOLDERS, C_menu::HAS_SUBMENU);
            }
#else
            if(IsStandalone()){
               menu->AddItem(TXT_ABOUT);
               add_sep = true;
            }
#endif
            if(add_sep)
               menu->AddSeparator();
            menu->AddItem(!IsStandalone() ? TXT_BACK : TXT_EXIT);
            app.PrepareMenu(menu);
         }
         return;

#ifdef AUDIO_PREVIEW
      case K_VOLUME_DOWN:
      case K_VOLUME_UP:
         if(audio_preview.active && ChangeAudioPreviewVolume((ui.key==K_VOLUME_UP)))
            redraw = true;
         break;
#endif

#if defined MAIL || defined WEB || defined EXPLORER
      case K_SEND:
#ifdef EXPLORER
         if(ui.key_bits&GKEY_SHIFT){
            if(!(flags&FLG_SELECT_OPTION))
               app.StartCreateZip(*this, false);
         }else
#endif//EXPLORER
         if(flags&FLG_ALLOW_SEND_BY){
            if(C_obex_file_send::CanSendByBt() && app.FileBrowser_SendBy(*this, C_obex_file_send::TR_BLUETOOTH))
               return;
         }
         break;
#endif

#ifdef EXPLORER
      case '0':
      case 'g':
         if(browser_mode==MODE_EXPLORER && !(flags&FLG_SELECT_OPTION)){
            reinterpret_cast<C_explorer_client*>(&app)->SetModeConfig();
            return;
         }
         break;
#ifdef _DEBUG
      case 'z':
         app.MakeZip(L"D:\\1\\test.zip", false);
         break;

         /*
      case 'a':
         if(ui.key_bits&GKEY_CTRL){
            C_client_about::CreateMode(this, L"About", L"logo.png");
            return;
         }
         break;
         */
#endif
#endif

#if defined MAIL || defined WEB || defined EXPLORER
      case 'c':
      case '1':                  //copy
      case '2':                  //move
      case 'm':
         if(flags&FLG_ALLOW_COPY_MOVE){
            app.FileBrowser_CopyMove(*this, (ui.key=='2' || ui.key=='m'));
            return;
         }
         break;

      case '3':
      case 'h':
      case 'p':
#ifdef AUDIO_PREVIEW
         if(ui.key=='3' || ui.key=='p'){
            if(!audio_preview.active && entries.size()){
                              //try to enable
               const C_mode_file_browser::S_entry &e = entries[selection];
               if(e.type==S_entry::FILE){
                  E_FILE_TYPE ft = app.DetermineFileType(text_utils::GetExtension(e.name));
                  if(ft==FILE_TYPE_AUDIO)
                     AudioPreviewEnable();
               }
            }
            if(audio_preview.active){
               AudioPreviewTogglePlay();
               redraw = true;
               break;
            }
         }
#endif
#if defined EXPLORER
         if((browser_mode==MODE_EXPLORER || browser_mode==MODE_ARCHIVE_EXPLORER) && (ui.key=='3' || ui.key=='h')){
            if(app.FileBrowser_OpenInHexViewer(*this))
               return;
         }
#endif
         break;

      case '4':
      case 'x':
#if defined EXPLORER && !defined _WIN32_WCE

#if !(defined UIQ && !defined __SYMBIAN_3RD__)
         if(browser_mode==MODE_EXPLORER){
            app.FileBrowser_SaveLastPath(*this);
            app.SetModeFileBrowser_Archive(L"*msgs*", app.GetText(TXT_MESSAGES));
            return;
         }
#endif
#endif
         if(arch && !(flags&FLG_ALLOW_COPY_MOVE)){
            const S_entry *e = !entries.size() ? NULL : &entries[selection];
            if(e){
               if(e->type==e->FILE || (flags&FLG_ALLOW_MARK_FILES)){
                  app.FileBrowser_ExtractFile(*this);
                  return;
               }
            }
         }
         break;
#endif

      case '5':
      case 'e':
         if(entries.size()){
            app.FileBrowser_ShowDetails(*this);
            return;
         }
         break;

#ifdef EXPLORER
#ifndef UNIX_FILE_SYSTEM
      case '6':
      case 't':
         if((flags&FLG_ALLOW_ATTRIBUTES) && entries.size()// && !marked_files.size()
            ){
            reinterpret_cast<C_explorer_client&>(app).SetModeEditAttributes(*this);
            return;
         }
         break;
#endif
#endif//EXPLORER

      case '7':
      case 'r':
         BeginRename();
         redraw = true;
         break;

#ifdef EXPLORER
      case '8':
      case 'i':
         if(app.FileBrowserEditFile(*this, false))
            return;
         break;
#endif

      case '9':
      case 'f':
         if(flags&FLG_ALLOW_MAKE_DIR){
            app.FileBrowser_BeginMakeNewFile(*this, true);
            redraw = true;
         }
         break;

      case ' ':
         app.FileBrowser_MarkCurrFile(*this, true, false);
         redraw = true;
         break;

#ifdef _DEBUG
      case 'y':
         app.FileBrowser_RefreshEntries(*this);
         redraw = true;
         break;
#endif

      /*
      case K_CURSORUP:
      case K_CURSORDOWN:
         if(entries.size()){
            redraw = true;
            int ns = selection;
            if(ui.key==K_CURSORUP){
               if(!ns)
                  ns = entries.size();
               --ns;
            }else{
               if(++ns == entries.size())
                  ns = 0;
            }
            if((ui.key_bits&GKEY_SHIFT) && (flags&FLG_ALLOW_MARK_FILES)){
                                 //epxand/clear selection
               app.FileBrowser_MarkCurrFile(*this, false, shift_mark);
               selection = ns;
               app.FileBrowser_MarkCurrFile(*this, false, shift_mark);
            }
            ChangeSelection(ns);
            EnsureVisible();
         }
         break;
         */

      case K_ENTER:
         if(!entries.size())
            break;
         if((ui.key_bits&GKEY_SHIFT) && (flags&FLG_ALLOW_MARK_FILES)){
            app.FileBrowser_MarkCurrFile(*this, true, false);
            redraw = true;
         }else
         switch(browser_mode){
         case MODE_GET_SAVE_FILENAME:
                                 //switch to mode entering filename
            app.FileBrowser_EnterSaveAsFilename(*this);
            redraw = true;
            break;
#if defined MAIL || defined WEB || defined EXPLORER
         case MODE_COPY:
         case MODE_MOVE:
            app.FileBrowser_CopyMove(*this, (browser_mode==MODE_MOVE), false);
            return;
#endif
         default:
            {
               C_client_file_mgr &app1 = this->app;
               if(app1.FileBrowser_OpenFile(*this) || app1.mode->Id()!=Id())
                  return;
               const S_entry &e = entries[selection];
               if(e.type!=e.FILE){
                  switch(e.expand_status){
                  case S_entry::COLLAPSED: redraw = app1.FileBrowser_ExpandDir(*this); break;
                  case S_entry::EXPANDED: redraw = app1.FileBrowser_CollapseDir(*this); break;
                  }
               }
               /*
               const S_entry &e = entries[selection];
               switch(e.type){
               case S_entry::DIR: case S_entry::DRIVE: case S_entry::ZIP:
                  if(!(flags&FLG_ACCEPT_DIR)){
                     switch(e.expand_status){
                     case S_entry::COLLAPSED: redraw = FileBrowser_ExpandDir(mod); break;
                     case S_entry::EXPANDED: redraw = FileBrowser_CollapseDir(mod); break;
                     }
                     break;
                  }
                              //flow...
               case S_entry::FILE:
                                    //default action (open/view/return)
                  if(FileBrowser_OpenFile(mod))
                     return;
               }
               */
            }
         }
         break;

      case K_DEL:
#ifdef _WIN32_WCE
      case 'd':
#endif
         if(flags&FLG_ALLOW_DELETE){
            app.FileBrowser_BeginDelete(*this);
            return;
         }
         break;

      case K_CURSORRIGHT:
         if(ui.key_bits&GKEY_SHIFT){
                              //mark all
            app.FileBrowser_MarkOnSameLevel(*this);
            redraw = true;
         }else
         if(!app.FileBrowser_ExpandDir(*this)){
            if(selection < entries.size()-1){
               ChangeSelection(selection+1);
               EnsureVisible();
            }
         }
         redraw = true;
         break;

      case K_CURSORLEFT:
         if(ui.key_bits&GKEY_SHIFT){
                              //mark none
            marked_files.clear();
            redraw = true;
         }else
         if(!app.FileBrowser_CollapseDir(*this)){
                                 //go to selection with upper level than ours
            const S_entry &e = entries[selection];
            for(int i=selection; i--; ){
               if(entries[i].level < e.level){
                  ChangeSelection(i);
                  EnsureVisible();
                  break;
               }
            }
         }
         redraw = true;
         break;
      }
   }
#if defined USE_MOUSE && (defined MAIL || defined EXPLORER)
   if(redraw && app.HasMouse())
      app.FileBrowser_SetButtonsState(*this);
#endif
}

//----------------------------

void C_client_file_mgr::C_mode_file_browser::DrawContents() const{

   const dword col_text = app.GetColor(COL_TEXT);
   app.ClearToBackground(S_rect(0, 0, app.ScrnSX(), rc.y));
   {
                              //draw title bar
      int x = app.fdb.letter_size_x/2;
      int y = 1;
      if(title!=TXT_NULL){
         app.DrawString(app.GetText(title), x, y, UI_FONT_BIG, 0, col_text);
         y += app.fdb.line_spacing;
      }

      Cstr_w full_name;
      app.FileBrowser_GetFullName(*this, full_name, -1, false);
#ifdef EXPLORER
                              //for find results, write path at top
      if(arch && arch->GetArchiveType()==1001){
         const S_entry &e = entries[selection];
         if(e.type==e.FILE){
            const C_archive_simulator &as = (C_archive_simulator&)*arch;
            const wchar *fn = as.GetRealFileName(e.name);
            if(fn){
               Cstr_w n = fn;
               full_name = n.Left(n.FindReverse('\\')+1);
            }
         }
      }
#endif
      wchar sep = '\\';
      const wchar *fn_w = full_name;
#ifdef UNIX_FILE_SYSTEM
      sep = '/';
      full_name = file_utils::ConvertPathSeparatorsToUnix(full_name);
      fn_w = full_name;
#endif

                              //draw nice path: entire last name, and partial or entire path
      int i = full_name.FindReverse(sep);
      i = Max(i, 0);
      int right_width = app.GetTextWidth(&fn_w[i], UI_FONT_BIG, FF_BOLD);
      int left_width = !i ? 0 : app.GetTextWidth(fn_w, UI_FONT_BIG, 0, i) + 1;
      int max_w = app.ScrnSX() - x*2;
#ifdef S60
      if(app.GetScreenRotation()==ROTATION_90CW && !app.HasMouse())
         max_w -= app.fdb.letter_size_x*8;
#endif
      if(marked_files.size()){
         int xx1 = app.ScrnSX()-2, y1 = 1;
#ifdef S60
         if(app.GetScreenRotation()==ROTATION_90CW && !app.HasMouse()){
            xx1 = app.ScrnSX()/4;
            y1 = app.ScrnSY()-app.fdb.cell_size_y-1;
         }
#endif
         const int frame_size = app.fdb.cell_size_y;
         Cstr_w s; s<<marked_files.size();
         int w = app.DrawString(s, xx1, y1, UI_FONT_BIG, FF_RIGHT, col_text);
         app.DrawCheckbox(xx1 - w - frame_size - 1, y1 + app.fdb.cell_size_y/2, frame_size, true, false);
         if(title==TXT_NULL)
            max_w -= w + frame_size + 2;
      }
      int max_left = 0;
      if(left_width+right_width > max_w){
         app.DrawString(&fn_w[i], x+max_w, y, UI_FONT_BIG, FF_BOLD|FF_RIGHT, col_text);
         max_left = -(max_w - right_width);
      }else{
         app.DrawString(&fn_w[i], x+left_width, y, UI_FONT_BIG, FF_BOLD, col_text);
         max_left = i*sizeof(wchar);
      }
      if(right_width < max_w && max_left)
         app.DrawString(fn_w, x, y, UI_FONT_BIG, 0, col_text, max_left);
   }
   app.ClearWorkArea(rc);
   app.DrawEtchedFrame(rc);

   if(!entries.size())
      return;
                              //draw entries

   const int x = rc.x + 1;//fd.letter_size_x/2;

                           //process lines, visible and invisible (invisible due to drawing lines)
   S_rect rc_item;
   int item_index = -1;
#ifdef NO_FILE_SYSTEM_DRIVES
   bool internal_info = false;
#endif
   while(BeginDrawNextItem(rc_item, item_index)){
      const S_entry &e = entries[item_index];

      dword color = col_text;
                           //draw selection
      if(item_index==selection){
         //S_rect rc(this->rc.x, y, this->rc.sx, app.fdb.line_spacing);
         //if(sb.total_space > sb.visible_space)
            //rc.sx = sb.rc.x - rc.x - 1;
         app.DrawSelection(rc_item);
         color = app.GetColor(COL_TEXT_HIGHLIGHTED);
      }

      int xx = x + Min(e.level, int(S_entry::MAX_DRAW_LEVEL)) * LEVEL_OFFSET;
      if(e.type!=e.FILE){
         int line_start = xx-2, line_end = xx;
         int y_mid = rc_item.y+entry_height/2;
                           //expansion mark
         if(e.expand_status!=e.NEVER_EXPAND){
            int yy = rc_item.y+(entry_height-expand_box_size)/2;
            app.DrawPlusMinus(xx, yy, expand_box_size, !(e.expand_status==e.EXPANDED));
         }else
            line_end += expand_box_size;
         xx += expand_box_size + 1;
         if(e.type==e.DIR){
                           //draw line connection to above level
            app.DrawHorizontalLine(line_start, y_mid, line_end-line_start, 0xff808080);
            int i;
            for(i=item_index; i--; ){
               const S_entry &ex = entries[i];
               if(ex.level == e.level-1)
                  break;
            }
            int line_y = y_mid - (item_index-i)*entry_height;
            line_y += expand_box_size/2+1;
            line_y = Max(line_y, rc.y);
            y_mid = Min(y_mid, rc.Bottom());
            app.DrawVerticalLine(line_start, y_mid, line_y-y_mid, 0xff808080);
         }
      }else
         xx += expand_box_size + 1;

      int max_x = sb.rc.x-3;
      int si = -1;
      switch(e.type){
      case S_entry::DRIVE: si = SPR_BROWSER_ICON_DRIVE; break;
      case S_entry::DIR:
         si = SPR_BROWSER_ICON_DIR;
#ifdef NO_FILE_SYSTEM_DRIVES
         if(e.atts&e.ATT_MEMORY_CARD)
            si = SPR_BROWSER_ICON_DRIVE;
#endif
         break;
      case S_entry::ARCHIVE:
#ifdef EXPLORER
         if(arch->GetArchiveType()==1000)
            si = SPR_BROWSER_ICON_MESSAGING;
         else
         if(arch->GetArchiveType()==1001)
            ;
         else
#endif
         si = SPR_BROWSER_ICON_FILE_ARCHIVE;
         break;
      default:
         {
            E_FILE_TYPE ft = app.DetermineFileType(text_utils::GetExtension(e.name));
            if(ft<=FILE_TYPE_EMAIL_MESSAGE)
               si = (SPR_BROWSER_ICON_FILE_ARCHIVE + ft);
            else
               si = SPR_BROWSER_ICON_FILE_DEFAULT;
         }
      }
      if(si!=-1){
         int yy = rc_item.y+(entry_height-(int)icons->SizeY())/2;
         C_fixed alpha(1);
#ifdef UNIX_FILE_SYSTEM
         if(!(e.atts&C_file::ATT_ACCESS_READ) || (e.atts&C_file::ATT_HIDDEN))
#else
         if(e.atts&C_file::ATT_HIDDEN)
#endif
            alpha = C_fixed::Percent(30);
         app.FileBrowser_DrawIcon(*this, xx, yy, si, alpha);
#ifdef UNIX_FILE_SYSTEM
         if(!(e.atts&C_file::ATT_ACCESS_WRITE) && !arch)
            app.DrawString(L"!", xx+icons->SizeY()*7/8, yy+Max(0, (int)icons->SizeY()-app.fds.line_spacing), app.UI_FONT_SMALL, FF_BOLD|FF_ITALIC, 0x80ff0000);
#endif
         xx += icon_sx + app.fdb.letter_size_x/2;
      }
                        //draw entry
      dword flg = 0;
      switch(e.type){
      case S_entry::DIR: flg = FF_BOLD; break;
      case S_entry::DRIVE: flg = FF_BOLD; break;
      }
#ifdef UNIX_FILE_SYSTEM
      if(!(e.atts&C_file::ATT_ACCESS_READ) || (e.atts&C_file::ATT_HIDDEN))
#else
      if(e.atts&C_file::ATT_HIDDEN)
#endif
         color = MulAlpha(color, 0x4000);
      if(marked_files.size()){
         if(app.FileBrowser_IsFileMarked(*this, e)){
            color = !(color&0xffffff) ? 0xffff0000 : 0xffe0e000;
            int SZ = app.fdb.cell_size_y;
            app.DrawCheckbox(rc_item.Right()-SZ-app.fdb.cell_size_x/2, rc_item.y+entry_height/2-SZ/2, SZ, true, false);
            max_x -= SZ;
         }
      }
      Cstr_w tmp;
      const wchar *name = e.name;
#ifdef NO_FILE_SYSTEM_DRIVES
      if(*name=='\\')
         ++name;
#endif
      bool is_renaming = (te_rename && item_index==selection);
      switch(e.type){
      case S_entry::DIR:
         if(e.level)
            break;
                        //flow...
      case S_entry::DRIVE:
         {
#ifdef NO_FILE_SYSTEM_DRIVES
            bool draw = (e.atts&e.ATT_MEMORY_CARD);
            if(!draw && !internal_info){
               internal_info = true;
               draw = true;
            }
            if(draw && (e.size.drive.total[0] || e.size.drive.total[1]))
#endif
            {
                        //draw drive info
               Cstr_w sf, st;
               MakeLongFileSizeText(e.size.drive.free, sf);
               MakeLongFileSizeText(e.size.drive.total, st);
               Cstr_w buf;
               dword font_index = UI_FONT_SMALL;
#ifdef _WIN32_WCE
               buf.Format(L"%/%");
               font_index = UI_FONT_SMALL;
#else
               buf.Format(L"(% % / %)") <<app.GetText(TXT_FREE);
#endif
               buf<<sf <<st;
               app.DrawString(buf, max_x, rc_item.y+1, font_index, FF_RIGHT, color);
            }
         }
         break;
      case S_entry::ARCHIVE:
#ifdef EXPLORER
         if(arch->GetArchiveType()>=1000)
            break;
#endif
                        //flow...
      case S_entry::FILE:
         if(!is_renaming){
            Cstr_w ss;
            if(e.size.file_size!=dword(-1))
               ss = text_utils::MakeFileSizeText(e.size.file_size, false, true);
            else
               ss = L"?";
            int yy = rc_item.y+1 + app.fdb.cell_size_y - app.fds.cell_size_y;
            max_x -= app.DrawString(ss, max_x, yy, UI_FONT_SMALL, FF_RIGHT, color);
            max_x -= app.fds.letter_size_x;
         }
         break;
      }
      if(is_renaming){
                        //don't draw edited entry
      }else
         app.DrawString(name, xx, rc_item.y+1+(entry_height-app.fdb.line_spacing)/2, UI_FONT_BIG, flg, color, -(max_x-xx));
      if(op_flags&OP_FLAGS_TOUCH_FRIENDLY)
         app.DrawHorizontalLine(rc_item.x+app.fdb.cell_size_x, rc_item.Bottom()-1, rc_item.sx-app.fdb.cell_size_x*2, MulAlpha(col_text, 0x4000));
   }

   ++item_index;
   for(int min_level_line_drawn = 1000; item_index<entries.size() && --min_level_line_drawn>1; item_index++){
      const S_entry &e = entries[item_index];

      if(e.type!=e.FILE){
         int xx = x + Min(e.level, int(S_entry::MAX_DRAW_LEVEL)) * LEVEL_OFFSET;
         int line_start = xx-2;
         int y_mid = rc_item.Bottom()+entry_height/2;
         if(e.type==e.DIR){
            int i;
            for(i=item_index; i--; ){
               const S_entry &ex = entries[i];
               if(ex.level == e.level-1)
                  break;
            }
            int line_y = y_mid - (item_index-i)*entry_height;
            line_y += expand_box_size/2+1;
            line_y = Max(line_y, rc.y);
            y_mid = Min(y_mid, rc.Bottom());
            app.DrawVerticalLine(line_start, y_mid, line_y-y_mid, 0xff808080);
         }
      }
      rc_item.y += entry_height;
   }
   if(img_thumbnail){
      const S_entry &e = entries[selection];
      int y_pos = selection*entry_height - sb.pos;
      S_rect rc1(0, 0, img_thumbnail->SizeX(), img_thumbnail->SizeY());
      rc1.x = this->rc.x + expand_box_size + 2 + Min(e.level, int(S_entry::MAX_DRAW_LEVEL)) * LEVEL_OFFSET + app.fdb.cell_size_x;
      rc1.y = this->rc.y + y_pos+entry_height + app.fdb.line_spacing/8;
      if(rc1.Bottom() > this->rc.Bottom()-app.fdb.line_spacing/4)
         rc1.y -= rc1.sy + entry_height + app.fdb.line_spacing/2;
      if(img_thumbnail->IsTransparent())
         app.FillRect(rc1, 0xffffffff);
      img_thumbnail->Draw(rc1.x, rc1.y);
      app.DrawOutline(rc1, 0xff000000);
      app.DrawShadow(rc1, true);
   }
#ifdef AUDIO_PREVIEW
   if(audio_preview.active){
      app.DrawDialogBase(audio_preview.rc, false);
      app.DrawOutline(audio_preview.rc, 0xff000000);
      app.DrawShadow(audio_preview.rc, true);
      if(audio_preview.err){
         app.DrawString(app.GetText(TXT_ERROR), audio_preview.rc.x+app.fdb.cell_size_x/2, audio_preview.rc.CenterY()-app.fdb.line_spacing/2, app.UI_FONT_SMALL, 0x80<<FF_SHADOW_ALPHA_SHIFT, 0xffff0000);
      }else{
         for(int i=0; i<audio_preview.BUTTON_LAST; i++){
            const C_button &but = audio_preview.buttons[i];
            app.DrawButton(but, true);
            S_rect rc1 = but.GetRect();
            rc1.Compact(rc1.sx/3);
            if(but.down)
               ++rc1.x, ++rc1.y;
            switch(i){
            case C_audio_preview::BUTTON_PLAY:
               if(audio_preview.plr && !audio_preview.paused){
                  int szx = rc1.sx/3;
                  app.FillRect(S_rect(rc1.x, rc1.y, szx, rc1.sy), col_text);
                  app.FillRect(S_rect(rc1.Right()-szx, rc1.y, szx, rc1.sy), col_text);
               }else
                  app.DrawArrowHorizontal(rc1.x, rc1.y, rc1.sx, col_text, true);
               if(app.HasKeyboard()){
                  const wchar *sh = L"[3]";  //S60 Release compiler doesn't recognize ? : statement? need this workaround
                  if(app.HasFullKeyboard())
                     sh = L"[P]";
                  //Info(sh, app.HasFullKeyboard());
                  app.DrawString(sh, but.GetRect().Right()-1, but.GetRect().y+1, app.UI_FONT_SMALL, FF_RIGHT, MulAlpha(col_text, 0x8000));
               }
               break;
            case C_audio_preview::BUTTON_VOL_UP:
            case C_audio_preview::BUTTON_VOL_DOWN:
               //app.DrawString(i==C_audio_preview::BUTTON_VOL_UP ? L"+" : L"-", rc1.CenterX(), rc1.CenterY()-app.fdb.line_spacing/2, app.UI_FONT_BIG, FF_CENTER, col_text);
               rc1.Compact(rc1.sx/4);
               app.DrawArrowVertical(rc1.x, rc1.y, rc1.sx, col_text, i!=C_audio_preview::BUTTON_VOL_UP);
               break;
            }
         }
         dword col = MulAlpha(col_text, 0x8000);
         app.DrawOutline(audio_preview.rc_volbar, col, col);
         //app.DrawProgress(audio_preview.progress, col_text, false);
         {
            S_rect rc1 = audio_preview.rc_volbar;
            int sz = audio_preview.volume*rc1.sy/10;
            rc1.y = rc1.Bottom()-sz;
            rc1.sy = sz;
            app.FillRect(rc1, col_text);
         }
         if(audio_preview.plr){
            //app.DrawProgress(audio_preview.progress, 0, false);
            app.DrawOutline(audio_preview.progress.rc, col, col);
            app.DrawProgress(audio_preview.progress, col_text, false);
         }
      }
   }
#endif
   EndDrawItems();
   if(sb.visible)
      app.DrawScrollbar(sb);
   
   if(te_rename)
      app.DrawEditedText(*te_rename);
}

//----------------------------

void C_client_file_mgr::C_mode_file_browser::Draw() const{

   DrawContents();

   app.ClearSoftButtonsArea(rc.Bottom() + 2);
   {
      dword lsk, rsk;
      if(te_rename){
         lsk = TXT_OK, rsk = TXT_CANCEL;
      }else
         lsk = TXT_MENU, rsk = !IsStandalone() ? TXT_BACK : TXT_EXIT;
      app.DrawSoftButtonsBar(*this, lsk, rsk, te_rename);
   }
#if defined USE_MOUSE && (defined MAIL || defined EXPLORER)
   if(!te_rename){
      static const char but_defs[] = { BUT_COPY, BUT_MOVE, BUT_NO, BUT_DELETE };
      static const dword tids[] = { _TXT_COPY, TXT_MOVE, 0, TXT_DELETE };
      app.DrawBottomButtons(*this, but_defs, tids, enabled_buttons);
   }
#endif
}

//----------------------------

bool C_client_file_mgr::CheckExtensionMatch(const wchar *fname, const char *exts){

                              //find name's extension
   int len = StrLen(fname);
   int dot_pos = len;
   while(dot_pos--){
      if(fname[dot_pos]=='.')
         break;
   }
                              //if file has no extension, it doesn't match any
   if(!++dot_pos)
      return false;
   len -= dot_pos;
   fname += dot_pos;
                              //check all extensions
   while(*exts){
      int ext_len = StrLen(exts);
      if(len==ext_len){
         int i;
         for(i=len; i--; ){
            if(ToLower(fname[i])!=exts[i])
               break;
         }
         if(i==-1)
            return true;
      }
      exts += ext_len+1;
   }
   return false;
}

//----------------------------
#include <Directory.h>

//----------------------------

bool C_client_file_mgr::GetDiskSpace(C_mode_file_browser::S_entry &e, dword &free_lo, dword &free_hi, dword &total_lo, dword &total_hi){

   C_dir::S_size total, free;
   if(!C_dir::GetDriveSpace(
#ifdef NO_FILE_SYSTEM_DRIVES
      e.name,
#else
      e.name[0]<256 ? (char)e.name[0] : 0,
#endif
      &total, &free)){
      free_lo = free_hi = total_lo = total_hi = 0;
      return false;
   }
   total_lo = total.lo;
   total_hi = total.hi;
   free_lo = free.lo;
   free_hi = free.hi;
   return true;
}

//----------------------------

bool C_client_file_mgr::FileBrowser_GetDirectoryContents(const C_mode_file_browser *mod, const wchar *dir, const C_mode_file_browser::S_entry *e,
   C_vector<C_mode_file_browser::S_entry> *entries, dword flags, const char *ext_filter){

   if((mod && mod->browser_mode==mod->MODE_ARCHIVE_EXPLORER) || (e && e->type==e->ARCHIVE))
      return FileBrowser_GetArchiveContents(*mod, dir, *entries, flags);

#ifdef _WIN32_WCE
   if(entries){
      if(!text_utils::CompareStringsNoCase(dir, L"\\windows"))
         entries->reserve(10000);
   }
#endif
   C_dir d;
   if(d.ScanBegin(dir)){
      while(true){
         dword attributes, size;
         S_date_time last_modify_time;
         const wchar *fn = d.ScanGet(&attributes, &size, &last_modify_time);
         if(!fn)
            break;
         if(!(flags&GETDIR_HIDDEN)){
#ifdef UNIX_FILE_SYSTEM
            if(!(attributes&C_file::ATT_ACCESS_READ) || (attributes&C_file::ATT_HIDDEN))
               continue;
#else
            if(attributes&C_file::ATT_HIDDEN)
               continue;
#endif
         }
         if((attributes&C_file::ATT_SYSTEM) && !(flags&GETDIR_SYSTEM))
            continue;

         C_mode_file_browser::S_entry e1;
         if(attributes&C_file::ATT_DIRECTORY){
            if(!(flags&GETDIR_DIRECTORIES))
               continue;
            e1.type = e1.DIR;
         }else{
            if(!(flags&GETDIR_FILES))
               continue;
            if(ext_filter && !CheckExtensionMatch(fn, ext_filter))
               continue;
            e1.type = e1.FILE;
            e1.size.file_size = size;
         }
         if(flags&GETDIR_CHECK_IF_ANY)
            return true;
         e1.atts = attributes;
         e1.name = fn;
         e1.modify_date = last_modify_time;
         e1.modify_date.MakeSortValue();
         entries->push_back(e1);
      }
   }
   return false;
}

//----------------------------

void C_client_file_mgr::GetAllDrives(C_mode_file_browser &mod, C_vector<C_mode_file_browser::S_entry> &entries, bool allow_rom_drives, bool allow_ram_drives) const{

#ifdef NO_FILE_SYSTEM_DRIVES
   FileBrowser_GetDirectoryContents(NULL, L"", NULL, &entries, GETDIR_DIRECTORIES | GETDIR_FILES | mod.op_flags);

   QuickSort(entries.begin(), entries.size(), sizeof(C_mode_file_browser::S_entry),
#ifdef EXPLORER
      &C_explorer_client::CompareFileEntries,
#else
      &mod.CompareEntries,
#endif
      &config.flags, &mod.SwapEntries);

   for(int i=entries.size(); i--; ){
      C_mode_file_browser::S_entry &en = entries[i];
      en.level = 0;
      Cstr_w n; n<<L"\\" <<en.name;
      en.name = n;

      if(!FileBrowser_GetDirectoryContents(NULL, en.name, &en, NULL, mod.op_flags | GETDIR_CHECK_IF_ANY, mod.extension_filter))
         en.expand_status = en.NEVER_EXPAND;
   }
   C_dir d;
   if(d.ScanMemoryCards()){
      for(const wchar *name; (name = d.ScanGet()); ){
         for(int i=entries.size(); i--; ){
            C_mode_file_browser::S_entry &en = entries[i];
            if(!StrCmp(en.name+1, name)){
               en.atts |= en.ATT_MEMORY_CARD;
               break;
            }
         }
      }
   }
#else
   //int num = C_dir::GetNumDrives();
   dword drives = C_dir::GetAvailableDrives();

#if !defined _DEBUG && defined MUSICPLAYER
#if defined S60
   drives &= ~8;               //D drive off
#endif
#endif
   for(int i=0; i<32; i++){
      if(!(drives&(1<<i)))
         continue;
      char d = char('a'+i);
      C_dir::E_DRIVE_TYPE t = C_dir::GetDriveType(d);
      if(t==C_dir::DRIVE_ERROR)
         continue;
      if(t==C_dir::DRIVE_ROM && !allow_rom_drives)
         continue;
      if(t==C_dir::DRIVE_RAMDISK && !allow_ram_drives)
         continue;
      Cstr_w s;
      s.Format(L"%:") <<wchar(ToUpper(d));

      C_mode_file_browser::S_entry e;
      e.type = e.DRIVE;
      e.name = s;
      e.level = 0;
      entries.push_back(e);
   }
   /*
#if defined EXPLORER && defined _DEBUG
                              //simulate Messages
   {
      C_mode_file_browser::S_entry e;
      e.name = L"<";
      e.type = e.DRIVE;
      e.level = 0;
      entries.push_back(e);
   }
#endif
   */
#endif
}

//----------------------------

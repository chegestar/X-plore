#ifdef __SYMBIAN32__
#include <e32std.h>
#endif
#include "..\Main.h"
#include "Main_Explorer.h"
#include <UI\FatalError.h>
#include <Ui\MultiQuestion.h>
#include "..\Viewer.h"
#include <UI\TextEntry.h>
#include <RndGen.h>
#include <Directory.h>
#include <Md5.h>
#include <Lang.h>

const int VERSION_HI = 1, VERSION_LO = 65;
const char VERSION_BUILD = 0;

#ifdef _DEBUG
//#define DEBUG_FORCE_STARTUP_SCREEN
#endif

//----------------------------

extern const wchar app_name[] = L"X-plore";

//----------------------------

const dword MAX_WRITE_TEXT_LEN = 256000;

//----------------------------

#ifdef __SYMBIAN32__
                              //Symbian UID
#ifdef __SYMBIAN_3RD__
#pragma data_seg(".SYMBIAN")
SYMBIAN_UID(0xa000b86e, 0x1e000)
#else
#pragma data_seg(".E32_UID")
SYMBIAN_UID(0x20009598)
#endif
#pragma data_seg()

#endif

//----------------------------

static struct{
   dword bgnd, selection, title, scrollbar, text_color;
} const color_themes[] = {
#ifdef USE_SYSTEM_SKIN
   { 0xffd0f0d0, 0xff80e080, 0xffc0c0f0, 0xffe0c080, 0xff000000 }, //blank
#endif
   { 0xffd0f0d0, 0xff80e080, 0xffc0c0f0, 0xffe0c080, 0xff000000 },
   { 0xffa0a0ff, 0xffe0c040, 0xff7070ff, 0xffc0c000, 0xff000000 },
   { 0xffe0e0e0, 0xff808080, 0xffa0a0a0, 0xffc0c0c0, 0xff000000 },
   { 0xfff0d0d0, 0xffe08080, 0xffe0c0ff, 0xffc080e0, 0xff000000 },
   { 0xfff0f0a0, 0xffa0a060, 0xffc0f0c0, 0xffc0c0f0, 0xff000000 },
   { 0xfff0f0c0, 0xff00ff00, 0xffffff00, 0xff00ff00, 0xff000000 },
   { 0xff2020c0, 0xffa06000, 0xff004080, 0xffc0c000, 0xffffffff },
   { 0xff000000, 0xff006db4, 0xff222222, 0xff404040, 0xffffffff },
};

static const int NUM_COLOR_THEMES = sizeof(color_themes)/sizeof(*color_themes);
//----------------------------

dword C_explorer_client::GetColor(E_COLOR col) const{

   switch(col){
   //case COL_HIGHLIGHT: return 0xffc0c0c0;
   //case COL_HIGHLIGHT: return 0x80ffffff;

   case COL_AREA:
   case COL_MENU:
   case COL_BACKGROUND: return color_themes[config.color_theme].bgnd;
   case COL_TITLE: return color_themes[config.color_theme].title;
   case COL_SCROLLBAR: return color_themes[config.color_theme].scrollbar;
   case COL_SELECTION: return color_themes[config.color_theme].selection;
   //case COL_TEXT: return color_themes[config.color_theme].text_color;
   case COL_TEXT:
   case COL_TEXT_HIGHLIGHTED:
   case COL_EDITING_STATE:
   case COL_TEXT_SOFTKEY:
   case COL_TEXT_TITLE:
   case COL_TEXT_POPUP:
   case COL_TEXT_MENU:
   case COL_TEXT_MENU_SECONDARY:
      if(config.color_theme)
         return color_themes[config.color_theme].text_color;
      break;
   }
   return C_client::GetColor(col);
}

//----------------------------

static bool IsNumericName(const wchar *name){

   while(true){
      wchar c = *name++;
      if(!c)
         break;
      if(!(c>='0' && c<='9'))
         return false;
   }
   return true;
}

//----------------------------

static int ConvertNameToNumber(const wchar *name){
   int r = 0;
   while(true){
      wchar c = *name++;
      if(!c)
         break;
      r *= 10;
      r += c-'0';
   }
   return r;
}

//----------------------------

int C_explorer_client::CompareFileEntries(const void *m1, const void *m2, void *context){

   C_client_file_mgr::C_mode_file_browser::S_entry &e1 = *(C_client_file_mgr::C_mode_file_browser::S_entry*)m1;
   C_client_file_mgr::C_mode_file_browser::S_entry &e2 = *(C_client_file_mgr::C_mode_file_browser::S_entry*)m2;
                              //first by type (drive, dir, file)
   if(e1.type != e2.type)
      return (e1.type < e2.type) ? -1 : 1;
                              //then by...
   S_config_xplore::E_SORT_MODE sm = *(S_config_xplore::E_SORT_MODE*)context;
   //const dword flags = *(dword*)context;
   switch(sm){
   case S_config_xplore::SORT_BY_NAME:
      break;
   case S_config_xplore::SORT_BY_DATE:
   case S_config_xplore::SORT_BY_DATE_REVERSE:
      if(e1.type==e1.FILE){
         int cmp = e1.modify_date.Compare(e2.modify_date);
         if(cmp){
            if(sm==S_config_xplore::SORT_BY_DATE_REVERSE)
               cmp = -cmp;
            return cmp;
         }
      }
      break;
   case S_config_xplore::SORT_BY_EXT:
      {
         const wchar *ex1 = text_utils::GetExtension(e1.name);
         const wchar *ex2 = text_utils::GetExtension(e2.name);
         if(ex1 && ex2){
            int cmp = text_utils::CompareStringsNoCase(ex1, ex2);
            if(cmp)
               return cmp;
         }else
         if(ex1 || ex2)
            return ex1 ? 1 : -1;
      }
      break;
   case S_config_xplore::SORT_BY_SIZE:
      if(e1.type==e1.FILE){
         if(e1.size.file_size!=e2.size.file_size)
            return (e1.size.file_size < e2.size.file_size) ? -1 : 1;
      }
      break;
   default:
      assert(0);
   }
                              //by name

                              //check if both names are numbers, then sort by number
   if(IsNumericName(e1.name) && IsNumericName(e2.name)){
      int i1 = ConvertNameToNumber(e1.name), i2 = ConvertNameToNumber(e2.name);
      return i1<i2 ? -1 : i1>i2 ? 1 : 0;
   }
   return text_utils::CompareStringsNoCase(e1.name, e2.name);
}

//----------------------------

C_explorer_client::S_config_xplore::S_config_xplore():
   default_text_coding(COD_WESTERN),
   sort_mode(SORT_BY_NAME),
   audio_volume(5)
{
   flags |= CONF_ASK_TO_EXIT;
   find_files_text = L"*.jpg";
}

//----------------------------

const S_config_store_type C_explorer_client::save_config_values[] = {
   { S_config_store_type::TYPE_WORD, OffsetOf(S_config,ui_font_size_delta), "ui_font_size" },
   { S_config_store_type::TYPE_CSTR_W, OffsetOf(S_config_xplore,app_password), "login" },
   { S_config_store_type::TYPE_BYTE, OffsetOf(S_config_xplore,sort_mode), "sort_mode" },
   { S_config_store_type::TYPE_BYTE, OffsetOf(S_config_xplore,default_text_coding), "text_coding" },
   { S_config_store_type::TYPE_BYTE, OffsetOf(S_config_xplore,audio_volume), "audio_volume" },
   { S_config_store_type::TYPE_CSTR_W, OffsetOf(S_config_xplore,find_files_text), "find_text" },

   { S_config_store_type::TYPE_NEXT_VALS, dword(&save_config_values_base) },
};

//----------------------------

const S_config_item C_explorer_client::config_options[] = {
   { CFG_ITEM_CHECKBOX, TXT_CFG_SHOW_HIDDEN, S_config_xplore::CONF_BROWSER_SHOW_HIDDEN, OffsetOf(S_config, flags) },
#ifndef NO_FILE_SYSTEM_DRIVES
   { CFG_ITEM_CHECKBOX, TXT_CFG_SHOW_ROM, S_config_xplore::CONF_BROWSER_SHOW_ROM, OffsetOf(S_config, flags) },
   { CFG_ITEM_CHECKBOX, TXT_CFG_SHOW_RAM, S_config_xplore::CONF_BROWSER_SHOW_RAM, OffsetOf(S_config, flags) },
#endif//!NO_FILE_SYSTEM_DRIVES
//#ifdef __SYMBIAN32__
   { CFG_ITEM_CHECKBOX, TXT_CFG_SHOW_SYSTEM, S_config_xplore::CONF_BROWSER_SHOW_SYSTEM, OffsetOf(S_config, flags) },
//#endif
   { CFG_ITEM_SORT_MODE, TXT_CFG_SORT_BY, 0, OffsetOf(S_config_xplore, sort_mode) },

   { CFG_ITEM_FONT_SIZE, TXT_CFG_UI_FONT_SIZE, 0, OffsetOf(S_config, ui_font_size_delta) },
   { CFG_ITEM_CHECKBOX, TXT_CFG_USE_SYSTEM_FONT, 0, OffsetOf(S_config, use_system_font) },
   { CFG_ITEM_TEXT_FONT_SIZE, TXT_CFG_TEXT_FONT_SIZE, 0, OffsetOf(S_config, viewer_font_index) },

#ifndef USE_ANDROID_TEXTS
   { CFG_ITEM_LANGUAGE, TXT_CFG_LANGUAGE, 0, },
#endif
   { CFG_ITEM_CHECKBOX, TXT_CFG_ASK_TO_EXIT, S_config_xplore::CONF_ASK_TO_EXIT, OffsetOf(S_config, flags) },
   { CFG_ITEM_CHECKBOX, TXT_CFG_VIEWER_FULLSCREEN, S_config_xplore::CONF_VIEWER_FULLSCREEN, OffsetOf(S_config, flags) },
   { CFG_ITEM_CHECKBOX, TXT_CFG_VIEWER_LANDSCAPE, S_config_xplore::CONF_VIEWER_LANDSCAPE, OffsetOf(S_config, flags) },
#if defined __SYMBIAN32__ || defined _DEBUG
   { CFG_ITEM_CHECKBOX, TXT_CFG_SYSTEM_APP, S_config_xplore::CONF_SYSTEM_APP, OffsetOf(S_config, flags) },
#endif
#ifdef _DEBUG
   //{ CFG_ITEM_ACCESS_POINT, TXT_CFG_ACCESS_POINT },
#endif
   { CFG_ITEM_APP_PASSWORD, TXT_CFG_PROGRAM_PASSWORD, 0, OffsetOf(S_config_xplore,app_password) },
   { CFG_ITEM_TEXT_CODING, TXT_CFG_TEXT_CODING, 0, OffsetOf(S_config_xplore, default_text_coding) },
   { CFG_ITEM_COLOR_THEME, TXT_CFG_COLOR_THEME, NUM_COLOR_THEMES },
};

//----------------------------

void C_explorer_client::SetModeConfig(){

   dword num = sizeof(config_options)/sizeof(*config_options);
   C_configuration_editing_xplore *ce = new C_configuration_editing_xplore(*this);
#ifndef USE_ANDROID_TEXTS
   ce->CollectLanguages();
#endif
   C_mode_config_explorer &mod = *new(true) C_mode_config_explorer(*this, config_options, num, ce);
   mod.InitLayout();
   ActivateMode(mod);
}

//----------------------------

void C_explorer_client::C_configuration_editing_xplore::OnClose(){

   if(modified && App().mode->Id()==C_client_file_mgr::ID){
      C_client_file_mgr::FileBrowser_SaveLastPath(&App(), (C_client_file_mgr::C_mode_file_browser&)*App().mode);
      App().mode = NULL;
      C_client_file_mgr::SetModeFileBrowser(&App());
   }
   super::OnClose();
   //app.RedrawScreen();
}

//----------------------------
static const wchar *const cod_names[] = {
   L"default", L"8859-1 (western)", L"8859-2 (central europe)", L"Windows-1250", L"Windows-1251 (cyrillic)", L"8859-9 (turkish)",
   L"8859-7 (greek)", L"utf-8", L"koi8-r", L"8859-5 (cyrillic iso)", L"8859-13 (Baltic)", L"Baltic (ISO)", L"Windows-1257 (Baltic)",
   L"8859-15", L"Big 5", L"GB 2312", L"GBK", L"Shift-JIS", L"iso-2022-jp", L"euc-kr (Korean)"
};

bool C_explorer_client::C_mode_config_explorer::GetConfigOptionText(const S_config_item &ec, Cstr_w &str, bool &draw_arrow_l, bool &draw_arrow_r) const{

   const S_config_xplore &config_xplore = (S_config_xplore&)App().config_xplore;
   switch(ec.ctype){
   case CFG_ITEM_TEXT_CODING:
      {
         dword ci = Min(dword(config_xplore.default_text_coding), dword(COD_EUC_KR));
         assert(COD_EUC_KR==19);
         str = cod_names[ci];
         draw_arrow_l = (config_xplore.default_text_coding>1);
         draw_arrow_r = (config_xplore.default_text_coding!=COD_EUC_KR);
      }
      break;

   case CFG_ITEM_SORT_MODE:
      draw_arrow_l = draw_arrow_r = true;
      switch(config_xplore.sort_mode){
      case S_config_xplore::SORT_BY_NAME:
         str = app.GetText(TXT_SORT_NAME);
         draw_arrow_l = false;
         break;
      case S_config_xplore::SORT_BY_DATE:
         str = app.GetText(TXT_SORT_DATE);
         break;
      case S_config_xplore::SORT_BY_EXT:
         str = app.GetText(TXT_SORT_EXT);
         break;
      case S_config_xplore::SORT_BY_SIZE:
         str = app.GetText(TXT_SORT_SIZE);
         draw_arrow_r = false;
         break;
      case S_config_xplore::SORT_BY_DATE_REVERSE:
      default:
         str = app.GetText(TXT_SORT_DATE);
         str << " (newer first)";
      }
      break;
   default:
      return C_mode_config_client::GetConfigOptionText(ec, str, draw_arrow_l, draw_arrow_r);
   }
   return true;
}

//----------------------------

bool C_explorer_client::C_mode_config_explorer::ChangeConfigOption(const S_config_item &ec, dword key, dword key_bits){

   S_config_xplore &config_xplore = (S_config_xplore&)App().config_xplore;
   bool changed = false;
   switch(ec.ctype){
   case CFG_ITEM_TEXT_CODING:
      if(key==K_CURSORRIGHT || key==K_CURSORLEFT){
         if(key==K_CURSORLEFT){
            if(config_xplore.default_text_coding>COD_DEFAULT+1){
               config_xplore.default_text_coding = E_TEXT_CODING(config_xplore.default_text_coding-1);
               changed = true;
            }
         }else{
            if(config_xplore.default_text_coding<COD_EUC_KR){
               config_xplore.default_text_coding = E_TEXT_CODING(config_xplore.default_text_coding+1);
               changed = true;
            }
         }
      }
      break;

   case CFG_ITEM_SORT_MODE:
      if(key==K_CURSORRIGHT || key==K_CURSORLEFT){
         byte &sm = (byte&)config_xplore.sort_mode;
         if(key==K_CURSORRIGHT){
            switch(sm){
            case S_config_xplore::SORT_BY_NAME: sm = S_config_xplore::SORT_BY_DATE; changed = true; break;
            case S_config_xplore::SORT_BY_DATE: sm = S_config_xplore::SORT_BY_DATE_REVERSE; changed = true; break;
            case S_config_xplore::SORT_BY_DATE_REVERSE: sm = S_config_xplore::SORT_BY_EXT; changed = true; break;
            case S_config_xplore::SORT_BY_EXT: sm = S_config_xplore::SORT_BY_SIZE; changed = true; break;
            }
         }else{
            switch(sm){
            case S_config_xplore::SORT_BY_SIZE: sm = S_config_xplore::SORT_BY_EXT; changed = true; break;
            case S_config_xplore::SORT_BY_EXT: sm = S_config_xplore::SORT_BY_DATE_REVERSE; changed = true; break;
            case S_config_xplore::SORT_BY_DATE_REVERSE: sm = S_config_xplore::SORT_BY_DATE; changed = true; break;
            case S_config_xplore::SORT_BY_DATE: sm = S_config_xplore::SORT_BY_NAME; changed = true; break;
            }
         }
      }
      break;
   default:
      changed = C_mode_config_client::ChangeConfigOption(ec, key, key_bits);
   }
   return changed;
}

//----------------------------

void C_explorer_client::C_configuration_editing_xplore::OnConfigItemChanged(const S_config_item &ec){

   S_config_xplore &config_xplore = (S_config_xplore&)App().config_xplore;
   switch(ec.ctype){
   case CFG_ITEM_CHECKBOX:
      switch(ec.param){
      case S_config_xplore::CONF_SYSTEM_APP:
         system::SetAsSystemApp(config_xplore.flags&config_xplore.CONF_SYSTEM_APP);
         break;
      }
      break;
   }
   super::OnConfigItemChanged(ec);
}

//----------------------------

void C_explorer_client::C_configuration_editing_xplore::OnClick(const S_config_item &ec){

   S_config_xplore &config_xplore = (S_config_xplore&)App().config_xplore;
   switch(ec.ctype){
   case CFG_ITEM_APP_PASSWORD:
      {
         const Cstr_w &s = config_xplore.app_password;
         if(!s.Length()){
            const int MAX_APP_PASSWORD = 20;
            class C_text_entry: public C_text_entry_callback{
               C_explorer_client &app;
               virtual void TextEntered(const Cstr_w &txt){
                  app.TxtNewPasswordEntered(txt);
               }
            public:
               C_text_entry(C_explorer_client &a): app(a){}
            };
            CreateTextEntryMode(App(), TXT_ENTER_NEW_PASSWORD, new(true) C_text_entry(App()), true, MAX_APP_PASSWORD, NULL, TXTED_ANSI_ONLY|TXTED_SECRET);
         }else{
            class C_text_entry: public C_text_entry_callback{
               C_explorer_client &app;
               virtual void TextEntered(const Cstr_w &txt){
                  app.TxtCurrPasswordEntered(txt);
               }
            public:
               C_text_entry(C_explorer_client &a): app(a){}
            };
            CreateTextEntryMode(App(), TXT_ENTER_CURRENT_PASSWORD, new(true) C_text_entry(App()), true, 1000, NULL, TXTED_ANSI_ONLY|TXTED_SECRET);
         }
      }
      break;
   }
   super::OnClick(ec);
}

//----------------------------

C_application_base *CreateApplication(){

   //Info("!");
   return new(true) C_explorer_client;
}

//----------------------------

void C_explorer_client::FocusChange(bool foreground){

#ifndef _DEBUG_
   if(foreground){
      if(mode)
      switch(mode->Id()){
      case C_client_file_mgr::ID:
         {
            C_client_file_mgr::C_mode_file_browser &mod_fb = (C_client_file_mgr::C_mode_file_browser&)*mode;
            if(!mod_fb.arch){
               /*
               if(mod_fb.timer_need_refresh==-1){
                                 //first focus, don't refresh now
                  mod_fb.timer_need_refresh = false;
               }else
               */
               {
                              //refresh contents (in next Tick)
                  //if(!mode->timer)
                     //CreateTimer(*mode, 1);
                  mode->DeleteTimer();
                  CreateTimer(*mode, 1);
                  mod_fb.timer_need_refresh = true;
               }
            }
         }
         break;
      }
      DeleteCachePath();
      /*
   }else{
      if(mode)
      switch(*mode){
      case C_client_file_mgr::ID:
         C_client_file_mgr::FileBrowser_CancelRename((C_client_file_mgr::C_mode_file_browser&)*mode);
         break;
      }
      */
   }
   C_client::FocusChange(foreground);
#endif
}

//----------------------------
static const wchar viewer_pos_filename[] = L"Explorer\\ViewPos.bin";
const dword VIEWPOS_SAVE_VERSION = 1;

void C_explorer_client::S_viewer_pos::Load(){
   assert(!read);
   C_file fl;
   if(fl.Open(viewer_pos_filename)){
      dword ver, num;
      if(fl.ReadDword(ver) && ver==VIEWPOS_SAVE_VERSION && fl.ReadDword(num)){
         entries.resize(num);
         dword i;
         for(i=0; i<num; i++){
            S_entry &e = entries[i];
            if(!file_utils::ReadString(fl, e.name))
               break;
            if(!fl.ReadDword(e.offs))
               break;
         }
         if(i!=num)
            entries.clear();
         else
            read = true;
      }
   }
}

//----------------------------

void C_explorer_client::S_viewer_pos::Save(){

   C_file fl;
   if(fl.Open(viewer_pos_filename, C_file::FILE_WRITE | C_file::FILE_WRITE_CREATE_PATH)){
      fl.WriteDword(VIEWPOS_SAVE_VERSION);
      dword num = entries.size();
      fl.WriteDword(num);
      for(dword i=0; i<num; i++){
         const S_entry &e = entries[i];
         file_utils::WriteString(fl, e.name);
         fl.WriteDword(e.offs);
      }
   }
}

//----------------------------

void C_explorer_client::S_viewer_pos::SaveTextPos(dword offs, const wchar *filename, const C_zip_package *arch){

   if(!read)
      Load();
   S_entry e;
   if(arch) e.name <<arch->GetFileName() <<'*';
   e.name <<filename;
   e.name.ToLower();
   int i;
   for(i=entries.size(); i--; ){
      if(entries[i].name==e.name){
         entries.erase(&entries[i]);
         read = true;
         break;
      }
   }
   if(offs){
      if(entries.size()==MAX_ENTRIES)
         entries.pop_front();
      e.offs = offs;
      entries.push_back(e);
      read = true;
   }
   Save();
}

//----------------------------

int C_explorer_client::S_viewer_pos::GetTextPos(const wchar *filename, const C_zip_package *arch){

   if(!read)
      Load();
   if(read){
      Cstr_w name;
      if(arch) name <<arch->GetFileName() <<'*';
      name <<filename;
      name.ToLower();
      for(int i=entries.size(); i--; ){
         if(entries[i].name==name)
            return entries[i].offs;
      }
   }
   return 0;
}

//----------------------------
static const wchar assoc_filename[] = L"Explorer\\Assoc.bin";

void C_explorer_client::LoadAssociations(){

   if(assocs_loaded)
      return;
   assocs_loaded = true;
   C_file fl;
   if(!fl.Open(assoc_filename))
      return;
   dword num;
   if(!fl.ReadDword(num))
      return;
   associations.Resize(num);
   dword i;
   for(i=0; i<num; i++){
      S_association &asc = associations[i];
      if(!file_utils::ReadString(fl, asc.ext) ||
         !file_utils::ReadString(fl, asc.app_name) ||
         !fl.ReadDword(asc.uid))
         break;
   }
   if(i!=num)
      associations.Resize(i);
}

//----------------------------

void C_explorer_client::SaveAssociations() const{

   C_file fl;
   if(!fl.Open(assoc_filename, C_file::FILE_WRITE))
      return;
   dword num = associations.Size();
   fl.WriteDword(num);
   for(dword i=0; i<num; i++){
      const S_association &asc = associations[i];
      file_utils::WriteString(fl, asc.ext);
      file_utils::WriteString(fl, asc.app_name);
      fl.WriteDword(asc.uid);
   }
}

//----------------------------

bool C_explorer_client::OpenFileByAssociationApp(const wchar *fname){

   LoadAssociations();
   if(associations.Size()){
      Cstr_w ext = text_utils::GetExtension(fname);
      ext.ToLower();
      for(int i=associations.Size(); i--; ){
         const S_association &asc = associations[i];
         if(asc.ext==ext){
            if(OpenFileBySystem(fname, asc.uid))
               return true;
            break;
         }
      }
   }
   return false;
}

//----------------------------

static const wchar quick_folders_filename[] = L"Explorer\\QuickDirs.bin";

void C_explorer_client::LoadQuickFolders(){

   if(quick_folders_loaded)
      return;
   quick_folders_loaded = true;
   C_file fl;
   if(!fl.Open(quick_folders_filename))
      return;
   for(dword i=0; i<10; i++){
      if(!file_utils::ReadString(fl, quick_folders[i]))
         break;
   }
}

//----------------------------

void C_explorer_client::SaveQuickFolders() const{

   C_file fl;
   if(!fl.Open(quick_folders_filename, C_file::FILE_WRITE))
      return;
   for(dword i=0; i<10; i++)
      file_utils::WriteString(fl, quick_folders[i]);
}

//----------------------------

void C_explorer_client::DeleteCachePath(){

   if(temp_path){
      Cstr_w tmp = temp_path;
      tmp = tmp.Left(tmp.Length()-1);
      C_file::GetFullPath(tmp, tmp);
      C_client_file_mgr::DeleteDirRecursively(tmp, NULL);
   }
}

//----------------------------

C_explorer_client::C_explorer_client():
   C_client(config_xplore),
   assocs_loaded(false),
   char_conv(NULL),
   quick_folders_loaded(false)
{
   temp_path =
#ifdef __SYMBIAN_3RD__
                              //save in non-protected folder
      L"\\System\\Data\\X-plore_temp\\";
#else
      L"Explorer\\tmp\\";
#endif
}

//----------------------------

C_explorer_client::~C_explorer_client(){

   DeleteCachePath();
}

//----------------------------

void C_explorer_client::Construct(){

   C_application_ui::Construct(L"Explorer\\");
   if(GetSystemLanguage()==LANG_CHINESE){
      config.app_language = "Chinese";
      config.use_system_font = true;
   }

   BaseConstruct(CreateDtaFile());
#if defined __SYMBIAN32__
   system::SetAsSystemApp(config_xplore.flags&config_xplore.CONF_SYSTEM_APP);
#endif

                              //request password
   if(config_xplore.app_password.Length()){
      class C_text_entry: public C_text_entry_callback{
         C_explorer_client &app;
         virtual void TextEntered(const Cstr_w &txt){
            app.TxtStartPasswordEntered(txt);
         }
      public:
         C_text_entry(C_explorer_client &a): app(a){}
      };
      CreateTextEntryMode(*this, TXT_ENTER_PASSWORD, new(true) C_text_entry(*this), true, 500, NULL, TXTED_ANSI_ONLY|TXTED_SECRET);
   }else
      FinishConstruct();
#ifdef _DEBUG
   //SetModeItemList(C_mode_item_list::TASKS);
   //SetModeItemList(C_mode_item_list::PROCESSES);
   //SetModeItemList(C_mode_item_list::ASSOCIATIONS);
   //SetModeItemList(C_mode_item_list::QUICK_DIRS);
   //static const wchar *options[] = { L"Overwrite", L"Skip all", L"Delete me", L"Four", L"Five", L"Six", L"Seven", L"Eight", L"Nine", L"Ten" }; C_client_multi_question::CreateMode(this, TXT_Q_OVERWRITE, L"123 g ksdfjegfoui fjeioerui fjslfhj fjkidf", options, 10, NULL);
   //ProgramUpdateCreate(*this);
   //{ S_user_input ui; ui.Clear(); ui.key = '7'; ProcessInput(ui); }
#endif
}

//----------------------------

void C_explorer_client::FinishConstruct(){

   if(DisplayLicenseAgreement())
      return;
   C_client_file_mgr::SetModeFileBrowser(this);
   UpdateScreen();
}

//----------------------------
//----------------------------

bool C_explorer_client::FileBrowser_ExtractToTempAndOpen(const C_client_file_mgr::C_mode_file_browser &mod){

   const C_client_file_mgr::C_mode_file_browser::S_entry &e = mod.entries[mod.selection];
   if(e.type!=e.FILE)
      return false;
   if(!mod.arch)
      return false;
   Cstr_w full_name;
   C_client_file_mgr::FileBrowser_GetFullName(mod, full_name);
   dword atype = mod.arch->GetArchiveType();
#ifdef __SYMBIAN_3RD__
   if(atype>=1001)
#else
   if(atype>=1000)
#endif
   {
                              //try to open by direct filename
      C_archive_simulator &as = (C_archive_simulator&)*mod.arch;
      const wchar *fn = as.GetRealFileName(full_name);
      if(fn){
         bool ok = OpenFileByAssociationApp(fn);
         if(!ok)
            ok = OpenFileBySystem(fn);;
         if(ok)
            return true;
         ShowErrorWindow(TXT_ERR_NO_VIEWER, full_name);
         return false;
      }
   }
   C_file *fp = mod.arch->OpenFile(full_name);
   if(!fp)
      return false;
   Cstr_w dst_n;
   dst_n<<temp_path <<full_name;
   C_file::MakeSurePathExists(dst_n);
   bool ok = C_client_file_mgr::CopyFile(this, *fp, e.name, dst_n);
   delete fp;
   if(ok){
      ok = OpenFileByAssociationApp(dst_n);
      if(!ok)
         ok = OpenFileBySystem(dst_n);
      if(!ok){
         C_file::DeleteFile(dst_n);
         ShowErrorWindow(TXT_ERR_NO_VIEWER, full_name);
      }else
         RedrawScreen();
   }
   return ok;
}

//----------------------------

void C_explorer_client::FileBrowser_GoToRealFile(C_client_file_mgr::C_mode_file_browser &mod){

   assert(mod.arch);
   Cstr_w full_name;
   C_client_file_mgr::FileBrowser_GetFullName(mod, full_name, -1, true);

   const C_archive_simulator &as = (C_archive_simulator&)*mod.arch;
   Cstr_w fn = as.GetRealFileName(full_name);
   if(!fn.Length())
      return;
                              //close this (search results) and goto given path
   mode = mod.saved_parent;
   if(mode->Id()==C_client_file_mgr::ID){
      C_client_file_mgr::FileBrowser_GoToPath(this, (C_client_file_mgr::C_mode_file_browser&)*mode, fn, false);
      RedrawScreen();
   }else
      assert(0);
}

//----------------------------

static const struct{
   E_TEXT_ID name;
   dword attrib;
} edit_attribs[] = {
   { TXT_HIDDEN, C_file::ATT_HIDDEN },
   { TXT_READ_ONLY, C_file::ATT_READ_ONLY },
   { TXT_SYSTEM, C_file::ATT_SYSTEM },
   //{ TXT_ARCHIVE, C_file::ATT_ARCHIVE },
};
const int NUM_ATTRIBS = sizeof(edit_attribs)/sizeof(*edit_attribs);

//----------------------------

void C_explorer_client::SetModeEditAttributes(C_client_file_mgr::C_mode_file_browser &mod_fb, int all_in_dir){

   C_vector<Cstr_w> filenames;
   if(mod_fb.marked_files.size()){
      filenames = mod_fb.marked_files;
   }else{
      const C_client_file_mgr::C_mode_file_browser::S_entry &e = mod_fb.entries[mod_fb.selection];
      if(e.type!=e.DIR && e.type!=e.FILE)
         return;
      if(e.type==e.DIR && all_in_dir==-1){
         class C_q: public C_question_callback{
            C_explorer_client &app;
            C_client_file_mgr::C_mode_file_browser &mod;

            virtual void QuestionConfirm(){
               app.SetModeEditAttributes(mod, true);
            }
            virtual void QuestionReject(){
               app.SetModeEditAttributes(mod, false);
            }
         public:
            C_q(C_explorer_client &_a, C_client_file_mgr::C_mode_file_browser &_m):
               app(_a),
               mod(_m)
            {}
         };
         CreateQuestion(*this, TXT_SHOW_ATTRIBUTES, GetText(TXT_Q_SET_ATTS_IN_ALL_FILES_IN_DIR), new(true) C_q(*this, mod_fb), true);
         return;
      }
      if(all_in_dir){
         C_client_file_mgr::GetThis(this)->FileBrowser_CollectMarkedOrSelectedFiles(mod_fb, filenames, true, false);
      }else{
         Cstr_w filename;
         C_client_file_mgr::FileBrowser_GetFullName(mod_fb, filename, -1, true);
         filenames.push_back(filename);
      }
   }
   mod_fb.img_thumbnail = NULL;
   C_mode_attributes &mod = *new(true) C_mode_attributes(*this);

   for(int i=0; i<filenames.size(); i++){
      dword attribs;
      if(!C_file::GetAttributes(filenames[i], attribs)){
         filenames.remove_index(i);
         --i;
      }else{
         C_mode_attributes::S_attrib at;
         for(int j=0; j<3; j++){
            at.not_same = false;
            at.state = bool((attribs&edit_attribs[j].attrib));
            if(!i)
               mod.atts[j] = at;
            else{
               if(!mod.atts[j].not_same){
                  if(mod.atts[j].state != at.state){
                     mod.atts[j].not_same = true;
                     mod.atts[j].state = 2;
                  }
               }
            }
         }
      }
   }
   if(!filenames.size()){
      mod.Release();
      ShowErrorWindow(TXT_ERROR, TXT_ERR_CANT_READ_ATTRIBS);
      return;
   }
   mod.filenames = filenames;
   mod.InitLayout();
   ActivateMode(mod);
}

//----------------------------

void C_explorer_client::C_mode_attributes::InitLayout(){

   const dword sx = app.ScrnSX();
   draw_bgnd = true;
   rc = S_rect(0, 0, Min(app.fdb.letter_size_x*25, app.ScrnSX()*7/8), app.GetDialogTitleHeight() + app.fdb.cell_size_x + app.fdb.line_spacing*NUM_ATTRIBS);
   rc.x = (sx-rc.sx)/2;
   rc.y = (app.ScrnSY()-rc.sy)/2;
}

//----------------------------

void C_explorer_client::C_mode_attributes::ProcessInput(S_user_input &ui, bool &redraw){

#ifdef USE_MOUSE
   {
      bool rd = false;
      app.ProcessMouseInSoftButtons(ui, rd, false);
      if(rd){
         redraw = true;
         draw_bgnd = true;
      }
      if(ui.mouse_buttons&1){
         S_rect rc1(rc.x, rc.y+app.GetDialogTitleHeight()+app.fdb.cell_size_x/2, rc.sx, app.fdb.line_spacing*NUM_ATTRIBS);
         if(ui.CheckMouseInRect(rc1)){
            int line = Min((ui.mouse.y-rc1.y)/app.fdb.line_spacing, NUM_ATTRIBS-1);
            if(selection!=line){
               selection = line;
               redraw = true;
            }//else
               ui.key = K_ENTER;
         }
      }
   }
#endif

   switch(ui.key){
   case K_LEFT_SOFT:
      if(!changed)
         break;
      {
         bool fail = false;
         for(int i=0; i<filenames.size(); i++){
            dword set_atts = 0, set_mask = 0;
            for(int j=0; j<3; j++){
               char a = atts[j].state;
               if(a!=2){
                  set_mask |= edit_attribs[j].attrib;
                  if(a)
                     set_atts |= edit_attribs[j].attrib;
               }
            }
            if(!C_file::SetAttributes(filenames[i], set_mask, set_atts)){
               fail = true;
            }
         }
         {
            C_client_file_mgr::C_mode_file_browser &mod_fb = (C_client_file_mgr::C_mode_file_browser&)*saved_parent;
            mod_fb.marked_files.clear();
            C_client_file_mgr::GetThis(&app)->FileBrowser_RefreshEntries(mod_fb);
         }
         if(fail){
            AddRef();
            app.mode = saved_parent;
            app.ShowErrorWindow(TXT_ERROR, TXT_ERR_CANT_CHANGE_ATTRIBS);
            Release();
            return;
         }
      }
                              //flow...
   case K_RIGHT_SOFT:
   case K_BACK:
   case K_ESC:
      AddRef();
      app.mode = saved_parent;
      app.RedrawScreen();
      Release();
      return;

   case K_CURSORUP:
   case K_CURSORDOWN:
      {
         int &sel = selection;
         if(ui.key == K_CURSORUP)
           sel += NUM_ATTRIBS-1;
         else
            ++sel;
         sel %= NUM_ATTRIBS;
         redraw = true;
      }
      break;

   case K_ENTER:
      {
         S_attrib &at = atts[selection];
         if(at.not_same){
            ++at.state %= 3;
         }else
            at.state = !at.state;
      }
      redraw = true;
      if(!changed){
         changed = true;
         draw_bgnd = true;
      }
      break;
   }
}

//----------------------------

void C_explorer_client::C_mode_attributes::Draw() const{

   if(draw_bgnd){
      DrawParentMode(true);

      app.DrawDialogBase(rc, true);
      Cstr_w s;
      if(filenames.size()==1){
         s = file_utils::GetFileNameNoPath(filenames[0]);
         if(!s.Length())
            s = filenames[0];
      }else
         s<<app.GetText(TXT_NUM_FILES) <<' ' <<filenames.size();
      app.DrawDialogTitle(rc, s);
   }
   const dword col_text = app.GetColor(COL_TEXT_POPUP);
   const int title_sy = app.GetDialogTitleHeight();
   int x = rc.x + app.fdb.cell_size_x/2;
   int y = rc.y + title_sy + app.fdb.cell_size_x/2;
   {
      S_rect rc1(rc.x, rc.y+title_sy, rc.sx, rc.sy-title_sy);
      app.DrawDialogBase(rc, false, &rc1);
   }
   for(int i=0; i<NUM_ATTRIBS; i++){
      dword col = col_text;
      if(i==selection){
         app.DrawSelection(S_rect(rc.x, y, rc.sx, app.fdb.line_spacing));
         col = app.GetColor(COL_TEXT_HIGHLIGHTED);
      }
      app.DrawString(app.GetText(edit_attribs[i].name), x, y, UI_FONT_BIG, 0, col);

      const int frame_size = app.fdb.cell_size_y-2;
      int xx = rc.Right() - frame_size - app.fdb.cell_size_x/2;
      //app.__DrawCheckboxFrame(xx, y+1, frame_size);
      app.DrawCheckbox(xx, y+1, frame_size, atts[i].state, true, atts[i].state==1 ? 255 : 128);
      /*
      int yy = y + app.fdb.line_spacing/2-1;
      if(atts[i].state){
         int x = xx+frame_size/2;
         if(atts[i].state==1)
            app.__DrawCheckbox(x, yy, frame_size);
         else{
            app.FillRect(S_rect(xx, y+1, frame_size, frame_size), 0x80808080);
            app.__DrawCheckbox(x, yy, frame_size, 0xff808080);
         }
      }
      */
      y += app.fdb.line_spacing;
   }
   if(draw_bgnd){
      app.DrawSoftButtonsBar(*this, changed ? TXT_CHANGE : TXT_NULL, TXT_CANCEL);
   }
   draw_bgnd = false;
   app.SetScreenDirty();
}

//----------------------------

void C_explorer_client::SetModeDeviceInfo(){

   S_device_info di;
   di.Read();

   C_vector<char> buf;
   S_text_style ts;
   Cstr_w tmp;
   const dword color = 0xff0000ff;

                              //memory
   if(di.total_ram){
      tmp.Format(L"%: ") <<GetText(TXT_TOTAL_RAM);
      ts.AddWideStringToBuf(buf, tmp, FF_BOLD);
      tmp = text_utils::MakeFileSizeText(di.total_ram, true, false);
      ts.AddWideStringToBuf(buf, tmp, -1, color);
      ts.AddWideStringToBuf(buf, L"\n");
   }
   tmp.Format(L"%: ") <<GetText(TXT_FREE_RAM);
   ts.AddWideStringToBuf(buf, tmp, FF_BOLD);
   tmp = text_utils::MakeFileSizeText(di.free_ram, true, false);
   ts.AddWideStringToBuf(buf, tmp, -1, color);
   ts.AddWideStringToBuf(buf, L"\n");

   if(di.cpu_speed!=-1){
      tmp.Format(L"%: ") <<GetText(TXT_CPU_SPEED);
      ts.AddWideStringToBuf(buf, tmp, FF_BOLD);
      tmp.Format(L"% MHz\n") <<di.cpu_speed;
      ts.AddWideStringToBuf(buf, tmp, -1, color);
   }
   CreateFormattedMessage(*this, GetText(TXT_DEVICE_INFO), buf);
}

//----------------------------
//----------------------------

class C_mode_text_editor: public C_mode_app<C_explorer_client>, public C_text_entry_callback{
   static int FindTextInText(const Cstr_w &what, const wchar *where, int offs, int where_len){

      int txt_len = what.Length();
      int num_finds = where_len - txt_len + 1;
      const wchar *cp = what;
      where += offs;
      for(int fi=offs; fi<num_finds; fi++){
         int i;
         for(i=0; i<txt_len; i++){
            wchar c1 = text_utils::LowerCase(cp[i]);
            wchar c2 = text_utils::LowerCase(where[i]);
            if(c1!=c2)
               break;
         }
         if(i==txt_len)
            return fi;
         ++where;
      }
      return -1;
   }
//----------------------------
                              //C_text_entry_callback:
   virtual void TextEntered(const Cstr_w &txt){
      SetFocus(ctrl_text);
      if(!txt.Length())
         return;
      app.last_find_text = txt;

      const wchar *cp = ctrl_text->GetText();
      int pos = ctrl_text->GetCursorPos();
      int len = ctrl_text->GetTextLength();
      int fi = FindTextInText(txt, cp, pos, len);
      if(fi==-1)
         fi = FindTextInText(txt, cp, 0, len);
      if(fi!=-1){
         ctrl_text->SetCursorPos(fi+txt.Length(), fi);
         //te.ResetCursor();
         Draw();
      }else{
         app.ShowErrorWindow(TXT_ERR_TEXT_NOT_FOUND, txt);
      }
   }
//----------------------------
   Cstr_w filename;
   Cstr_w display_name;
   bool wide;
   bool modified;
   C_ctrl_text_entry *ctrl_text;
//----------------------------
   bool Save(){
      C_file fl;
      if(!fl.Open(filename, C_file::FILE_WRITE | C_file::FILE_WRITE_READONLY)){
         Cstr_w s;
         s.Format(app.GetText(TXT_ERR_CANT_WRITE_FILE)) <<display_name;
         app.ShowErrorWindow(TXT_ERROR, s);
         return false;
      }
      const wchar *txt = ctrl_text->GetText();
      if(wide)
         fl.WriteWord(0xfeff);
      while(*txt){
         wchar c = *txt++;
         if(wide){
            if(c=='\n')
               fl.WriteWord('\r');
            fl.WriteWord(c);
         }else{
            if(c=='\n')
               fl.WriteByte('\r');
            if(c>=0x80){
               S_font::E_PUNKTATION p;
               c = wchar(C_application_base::S_internal_font::GetCharMapping(c, p));
            }
            fl.WriteByte(char(c));
         }
      }
      modified = false;
      UpdateTitle();
      return true;
   }
//----------------------------
   virtual void Close(bool redraw = true){
      SetFocus(NULL);
      if(modified){
                                 //ask to save changes
         class C_question: public C_question_callback{
            C_mode_text_editor &mod;
            virtual void QuestionConfirm(){
               if(!mod.Save())
                  return;
               if(mod.GetParent()->Id() == C_client_file_mgr::ID)
                  C_client_file_mgr::FileBrowser_RefreshEntries(&mod.app, (C_client_file_mgr::C_mode_file_browser&)*mod.GetParent());
               mod.modified = false;
               mod.Close();
            }
            virtual void QuestionReject(){
               mod.modified = false;
               mod.Close();
            }
         public:
            C_question(C_mode_text_editor &_m): mod(_m){}
         };
         CreateQuestion(app, TXT_Q_SAVE_CHANGES, display_name, new(true) C_question(*this), true);
         return;
      }
      C_mode::Close(redraw);
   }
//----------------------------
   virtual void TextEditNotify(bool cursor_moved, bool text_changed, bool &redraw){
      //if(cursor_moved)
         //adjust_scroll = true;
      if(text_changed){
         modified = true;
         UpdateTitle();
      }
      redraw = true;
   }
//----------------------------

public:

   C_mode_text_editor(C_explorer_client &_app, const wchar *fn, const wchar *dn, dword _font_index, bool _wide, const wchar *text):
      C_mode_app<C_explorer_client>(_app),
      //C_client_writer::C_mode_this(_app, _font_index),
      filename(fn),
      display_name(dn),
      wide(_wide),
      ctrl_text(NULL),
      modified(false)
   {
      ctrl_text = new(true) C_ctrl_text_entry(this, MAX_WRITE_TEXT_LEN, TXTED_ALLOW_PREDICTIVE, _font_index);
      AddControl(ctrl_text);
      ctrl_text->SetText(text);
      ctrl_text->SetCursorPos(0); 
      SetFocus(ctrl_text);
      /*
      if(!ctrl_text){
         te_write = app.CreateTextEditor(TXTED_ALLOW_PREDICTIVE | TXTED_ALLOW_EOL, _font_index, 0, text, MAX_WRITE_TEXT_LEN);
         te_write->Release();
         te_write->SetCursorPos(0, 0);
      }
      */
      UpdateTitle();
   }
//----------------------------
   void UpdateTitle(){
      Cstr_w title;
      if(modified)
         title <<L"* ";
      title <<display_name;
      title.AppendFormat(L" (%)") <<ctrl_text->GetTextLength();
      SetTitle(title);
   }
//----------------------------
   virtual void InitLayout(){
      C_mode::InitLayout();

      S_rect rc = GetClientRect();
      rc.Compact(1);
      ctrl_text->SetRect(S_rect(rc.x, rc.y, rc.sx, rc.sy));
   }
//----------------------------
   virtual void InitMenu(){
      app.AddEditSubmenu(menu);
      menu->AddItem(TXT_FIND);
      menu->AddItem(TXT_FIND_NEXT, app.last_find_text.Length() ? 0 : C_menu::DISABLED);
      menu->AddItem(TXT_SAVE, modified ? 0 : C_menu::DISABLED);
      menu->AddItem(TXT_SAVE_AS_UNICODE, wide ? C_menu::MARKED : 0);
      menu->AddSeparator();
      menu->AddItem(TXT_BACK);
   }
//----------------------------
   virtual void ProcessInput(S_user_input &ui, bool &redraw){
      C_mode::ProcessInput(ui, redraw);
   }
//----------------------------
   virtual void ProcessMenu(int itm, dword menu_id){
      switch(itm){
      case TXT_SAVE:
         Save();
         break;

      case TXT_EDIT:
         menu = app.CreateEditCCPSubmenu(ctrl_text->GetTextEditor(), menu);
         app.PrepareMenu(menu);
         break;

      case C_application_ui::SPECIAL_TEXT_CUT:
      case C_application_ui::SPECIAL_TEXT_COPY:
      case C_application_ui::SPECIAL_TEXT_PASTE:
         app.ProcessCCPMenuOption(itm, ctrl_text->GetTextEditor());
         break;

      case TXT_BACK:
         Close();
         return;

      case TXT_FIND:
         CreateTextEntryMode(app, TXT_FIND, this, false, app.MAX_FIND_TEXT_LEN, app.last_find_text);
         return;

      case TXT_FIND_NEXT:
         TextEntered(app.last_find_text);
         break;

      case TXT_SAVE_AS_UNICODE:
         wide = !wide;
         modified = true;
         break;
      }
   }
//----------------------------
   virtual void Draw() const{
      C_mode::Draw();
      app.DrawOutline(ctrl_text->GetRect(), app.GetColor(app.COL_SHADOW), app.GetColor(app.COL_HIGHLIGHT));
      //if(te_write)
         //C_mode_this::Draw(0xffffffff, true);
   }
};

//----------------------------
//----------------------------

void C_explorer_client::CreateTextEditMode(const wchar *filename, const wchar *display_name, bool is_new_file){

   C_file fl;
                              //check if the file may be opened for writing
   if(!fl.Open(filename, C_file::FILE_WRITE | C_file::FILE_WRITE_NOREPLACE)){
      Cstr_w s;
      s.Format(GetText(TXT_ERR_CANT_WRITE_FILE)) <<filename;
      ShowErrorWindow(TXT_ERROR, s);
      return;
   }

   if(!fl.Open(filename))
      return;
                              //detect wide
   bool wide = is_new_file, big_endian = false;
   E_TEXT_CODING coding = config_xplore.default_text_coding;
   dword file_size = fl.GetFileSize();
   if(file_size >= 2){
      word w;
      fl.ReadWord(w);
      if(w==0xfeff || w==0xfffe){
         if(file_size&1)
            return;
         wide = true;
         big_endian = (w==0xfffe);
         file_size -= 2;
      }else if(w==0xbbef){
         byte b;
         if(fl.ReadByte(b) && b==0xbf){
            coding = COD_UTF_8;
            file_size -= 3;
         }else
            fl.Seek(0);
      }else
         fl.Seek(0);
   }
   file_size = Min(file_size, MAX_WRITE_TEXT_LEN*(wide ? 2 : 1)+100);
   Cstr_c ext; ext.Copy(text_utils::GetExtension(filename)); ext.ToLower();
   bool is_txt = (ext=="txt");

   C_buffer<byte> buf;
   buf.Resize(file_size+1);
   fl.Read(buf.Begin(), file_size);
   buf[file_size] = 0;
   int num_chars = file_size;
   if(wide) num_chars/=2;

   if(!wide && encoding::IsMultiByteCoding(coding)){
      Cstr_w tmp;
      ConvertMultiByteStringToUnicode((const char*)buf.Begin(), coding, tmp);
      num_chars = tmp.Length();
      buf.Resize(num_chars*2);
      MemCpy(buf.Begin(), (const wchar*)tmp, num_chars*2);
      wide = true;
   }else
   if(big_endian){
      wchar *wp = (wchar*)buf.Begin();
      for(int i=0; i<num_chars; i++)
         wp[i] = ByteSwap16(wp[i]);
   }
   C_vector<wchar> tbuf;
   tbuf.reserve(num_chars+1);
   const void *str = buf.Begin();
   for(int i=0; i<num_chars; i++){
      word w = word(text_utils::GetChar(str, wide));
      if(w<' ')
      switch(w){
      case '\r':
         continue;
      case '\n':
         break;
      case '\t':
         w = ' ';
         tbuf.push_back(w);
         tbuf.push_back(w);
         break;
      default:
                           //invalid char
         if(!is_txt){
                           //can't edit binary file
            if(!SetModeHexEdit(filename, NULL, true))
               ShowErrorWindow(TXT_ERROR, TXT_ERR_CANT_EDIT_BINARY);
            return;
         }
                           //convert to valid char '?'
         w = '?';
      }else
      if(!wide)
         w = (word)encoding::ConvertCodedCharToUnicode(w, coding);
      tbuf.push_back(w);
   }
   tbuf.push_back(0);
   C_mode_text_editor &mod = *new(true) C_mode_text_editor(*this, filename, display_name, config.viewer_font_index, wide, tbuf.begin());
   mod.InitLayout();
   ActivateMode(mod);

   if(tbuf.size()-1 > int(MAX_WRITE_TEXT_LEN)){
      //mod.te_write->Activate(false);
      ShowErrorWindow(TXT_WARNING, TXT_ERR_EDITING_TRUNCATED);
   }
}

//----------------------------

int C_explorer_client::C_mode_item_list::CompareItems(const void *_i0, const void *_i1, void*){
   C_mode_item_list::S_item &i0 = *(C_mode_item_list::S_item*)_i0,
      &i1 = *(C_mode_item_list::S_item*)_i1;
   return StrCmp(i0.display_name, i1.display_name);
}

//----------------------------

void C_explorer_client::SetModeFileAssociations(const wchar *init_ext){

   C_mode_item_list &mod = SetModeItemList(C_mode_item_list::ASSOCIATIONS, false);
   if(init_ext){
      Cstr_w ext = init_ext;
      ext.ToLower();
      for(int i=mod.items.size(); i--; ){
         if(mod.items[i].display_name==ext){
            mod.selection = i;
            mod.EnsureVisible();
            break;
         }
      }
   }
   RedrawScreen();
}

//----------------------------

C_explorer_client::C_mode_item_list &C_explorer_client::SetModeItemList(C_mode_item_list::E_TYPE type, bool draw){

   C_mode_item_list &mod = *new(true) C_mode_item_list(*this, type, mode);

   switch(type){
   case C_mode_item_list::PROCESSES:
      EnumProcesses(mod.items);
      break;
   case C_mode_item_list::TASKS:
      EnumTasks(mod.items, true);
      break;
   case C_mode_item_list::CHOOSE_APP:
      EnumTasks(mod.items, false);
      break;
   case C_mode_item_list::ASSOCIATIONS:
      {
         LoadAssociations();
         for(dword i=associations.Size(); i--; ){
            const S_association &asc = associations[i];
            C_mode_item_list::S_item itm;
            itm.display_name = asc.ext;
            itm.internal_name = asc.app_name;
            itm.id = asc.uid;
            mod.items.push_back(itm);
         }
      }
      break;
   case C_mode_item_list::QUICK_DIRS:
      LoadQuickFolders();
      for(int i=0; i<10; i++){
         C_mode_item_list::S_item itm;
         const wchar *s = quick_folders[i];
         itm.id = 1;
         if(!*s){
            s = GetText(TXT_NOT_SET);
            --itm.id;
         }
         //int ii = (i+1)%10;
         //itm.display_name.Format(L"%. %") <<ii <<s;
         itm.display_name = s;
#ifdef UNIX_FILE_SYSTEM
         itm.display_name = file_utils::ConvertPathSeparatorsToUnix(itm.display_name);
#endif
         mod.items.push_back(itm);
      }
      break;
   default:
      assert(0);
   }
   if(type!=C_mode_item_list::QUICK_DIRS)
      QuickSort(mod.items.begin(), mod.items.size(), sizeof(C_mode_item_list::S_item), C_mode_item_list::CompareItems);
   mod.sb.total_space = mod.items.size();

   InitLayoutItemList(mod);
   ActivateMode(mod, draw);
   return mod;
}

//----------------------------

void C_explorer_client::InitLayoutItemList(C_mode_item_list &mod){

   const int border = 3;
   const int top = GetTitleBarHeight();
   mod.rc = S_rect(border, top, ScrnSX()-border*2, ScrnSY()-top);
   mod.rc.sy -= GetSoftButtonBarHeight();
   mod.entry_height = fdb.line_spacing+1;
   if(HasMouse())
      mod.entry_height *= 2;
   mod.sb.visible_space = mod.rc.sy/mod.entry_height;
   mod.rc.sy = mod.entry_height*mod.sb.visible_space;

   const int sb_width = GetScrollbarWidth();
   mod.sb.rc = S_rect(mod.rc.Right()-sb_width-1, mod.rc.y+3, sb_width, mod.rc.sy - 6);

   mod.sb.SetVisibleFlag();
   mod.EnsureVisible();
}

//----------------------------

void C_explorer_client::ItemList_ActivateTask(C_mode_item_list &mod){

   if(!mod.items.size())
      return;
   C_mode_item_list::S_item &itm = mod.items[mod.selection];
   itm.StartTask();
}

//----------------------------

bool C_explorer_client::ItemList_TerminateProcessOrTask(C_mode_item_list &mod, bool ask){

   if(!mod.items.size())
      return false;
   C_mode_item_list::S_item &itm = mod.items[mod.selection];
   bool is_task = (mod.type==mod.TASKS);
   if(ask){
      class C_question: public C_question_callback{
         C_mode_item_list &mod;
         virtual void QuestionConfirm(){
            ((C_explorer_client&)mod.app).ItemList_TerminateProcessOrTask(mod, false);
         }
      public:
         C_question(C_mode_item_list &_m): mod(_m){}
      };
      Cstr_w s; s.Format(GetText(is_task ? TXT_Q_TERMINATE_TASK : TXT_Q_TERMINATE_PROCESS)) <<itm.display_name;
      CreateQuestion(*this, TXT_Q_ARE_YOU_SURE, s, new(true) C_question(mod), true);
      return false;
   }
   bool ok;
   if(is_task)
      ok = itm.TerminateTask();
   else
      ok = itm.TerminateProcess();
   if(!ok){
      ShowErrorWindow(TXT_ERROR, is_task ? TXT_ERR_CANT_TERMINATE_TASK : TXT_ERR_CANT_TERMINATE_PROCESS);
      return false;
   }
                              //remove selection
   mod.items.erase(&itm);
   --mod.sb.total_space;
   mod.sb.SetVisibleFlag();
   mod.selection = Min(mod.selection, mod.items.size()-1);
   mod.EnsureVisible();
   return true;
}

//----------------------------

void C_explorer_client::ItemList_Close(C_mode_item_list &mod){

   mode = mod.saved_parent;
   RedrawScreen();
}

//----------------------------

void C_explorer_client::ItemList_DeleteAssociation(){

   C_mode_item_list &mod = (C_mode_item_list&)*mode;
   const C_mode_item_list::S_item &itm = mod.items[mod.selection];
   int i;
   int num_a = associations.Size();
   for(i=num_a; i--; ){
      if(associations[i].ext==itm.display_name){
         for(; i<num_a-1; i++)
            associations[i] = associations[i+1];
         associations.Resize(num_a-1);
         SaveAssociations();
         break;
      }
   }
   {
      mod.items.erase(&mod.items[mod.selection]);
      --mod.sb.total_space;
      mod.sb.SetVisibleFlag();
      mod.selection = Min(mod.selection, mod.items.size()-1);
      mod.EnsureVisible();
   }
}

//----------------------------

class C_mailbox_arch_sim_imp: public C_mailbox_arch_sim{

   C_smart_ptr<C_messaging_session> msg_session;

   virtual E_TYPE GetArchiveType() const{ return (E_TYPE)1000; }
   virtual const wchar *GetFileName() const{ return L"*Msgs"; }

//----------------------------

   virtual void *GetFirstEntry(const wchar *&name, dword &length) const{
      if(!items.size())
         return NULL;
      name = items.front().filename;
      length = items.front().size;
      return (void*)1;
   }

//----------------------------

   virtual void *GetNextEntry(void *handle, const wchar *&name, dword &length) const{
      dword indx = (dword)handle;
      if(indx==NumFiles())
         return NULL;
      name = items[indx].filename;
      length = items[indx].size;
      ++indx;
      return (void*)indx;
   }

//----------------------------

   virtual bool GetFileWriteTime(const wchar *filename, S_date_time &td) const{

      int fi = FindIndex(filename);
      if(fi==-1)
         return false;
      td = items[fi].modify_date;
      return true;
   }

//----------------------------

   virtual dword NumFiles() const{ return items.size(); }

   int FindIndex(const wchar *filename) const{
      for(int i=0; i<items.size(); i++){
         if(items[i].filename==filename)
            return i;
      }
      return -1;
   }

//----------------------------

   virtual bool DeleteFile(const wchar *filename){
      int i = FindIndex(filename);
      if(i==-1)
         return false;
      if(!items[i].DeleteMessage(msg_session))
         return false;
      items.erase(&items[i]);
      return true;
   }

//----------------------------

   virtual const wchar *GetRealFileName(const wchar *filename) const{
      int i = FindIndex(filename);
      if(i==-1)
         return NULL;
      return items[i].full_filename;
   }
//----------------------------
   void EnumMessages();
//----------------------------
   t_Change change_callback;
   void *change_context;
   static void Change(void *c){
      C_mailbox_arch_sim_imp *_this = (C_mailbox_arch_sim_imp*)c;
      _this->EnumMessages();
      (*_this->change_callback)(_this->change_context);
   }
//----------------------------
   virtual void SetChangeObserver(t_Change c, void *context){
      change_callback = c;
      change_context = context;
      msg_session->SetChangeObserver(&Change, this);
   }
public:
   C_mailbox_arch_sim_imp():
      change_callback(NULL)
   {}

   void Construct(){
      msg_session = C_messaging_session::Create();
      msg_session->Release();
      EnumMessages();
   }

   virtual C_file *OpenFile(const wchar *filename) const{
      int i = FindIndex(filename);
      if(i!=-1){
         C_file *fp = items[i].CreateFile(msg_session);
         if(fp){
            C_mailbox_arch_sim_imp *as = const_cast<C_mailbox_arch_sim_imp*>(this);
            as->items[i].SetMessageAsRead(as->msg_session);
         }
         return fp;
      }
      return NULL;
   }
};

//----------------------------

void C_mailbox_arch_sim_imp::EnumMessages(){
   C_mailbox_arch_sim::EnumMessages(msg_session);
                           //detect name duplicates
   for(int i=0; i<items.size(); i++){
      Cstr_w &n = items[i].filename;
      int fi = FindIndex(n);
      if(fi!=i){
                        //avoid name duplicates
         int ei = n.FindReverse('.');
         Cstr_w name = ei==-1 ? n : n.Left(ei);
         Cstr_w ext;
         if(ei!=-1)
            ext = n.Right(n.Length()-ei);
         for(int j=1; j<10000; j++){
            Cstr_w n1;
            n1.Format(L"% (#02%)%") <<name <<j <<ext;
            if(FindIndex(n1)==-1){
               n = n1;
               break;
            }
         }
      }
   }
}

//----------------------------

C_mailbox_arch_sim *CreateMessagingArchive(){
   C_mailbox_arch_sim_imp *a = new(true) C_mailbox_arch_sim_imp;
   a->Construct();
   return a;
}

//----------------------------

void C_explorer_client::ItemList_EditOrNewAssociation(C_mode_item_list &mod){

   C_mode_item_list::S_item itm;
   itm.id = 0;
   mod.selection = mod.items.size();
   mod.items.push_back(itm);
   ++mod.sb.total_space;
   mod.sb.SetVisibleFlag();
   mod.EnsureVisible();

   ItemList_BeginEditAssociation(mod);
}

//----------------------------

void C_explorer_client::ItemList_BeginEditAssociation(C_mode_item_list &mod){

   mod.te_edit = CreateTextEditor(0, UI_FONT_BIG, 0, NULL, 5);
   mod.te_edit->Release();
   C_text_editor &te = *mod.te_edit;
   te.SetCase(te.CASE_LOWER, te.CASE_LOWER);
   te.SetRect(S_rect(mod.rc.x+fdb.cell_size_x/2, mod.rc.y+(mod.selection-mod.top_line)*mod.entry_height, fdb.cell_size_x*5, fdb.line_spacing));

   C_mode_item_list::S_item &itm = mod.items[mod.selection];

   te.SetInitText(itm.display_name);
   te.SetCursorPos(te.GetTextLength(), te.GetTextLength());
}

//----------------------------

void C_explorer_client::ItemList_AssociationChooseApp(C_mode_item_list &mod){

   Cstr_w app_name1 = mod.items[mod.selection].internal_name;
   {
      C_mode_item_list &mod_il = SetModeItemList(C_mode_item_list::CHOOSE_APP, false);
      for(int i=mod_il.items.size(); i--; ){
         if(mod_il.items[i].display_name==app_name1){
            mod_il.selection = i;
            mod_il.EnsureVisible();
            break;
         }
      }
      RedrawScreen();
   }
}

//----------------------------

void C_explorer_client::ItemList_ConfirmChooseApp(C_mode_item_list &mod){

   const C_mode_item_list::S_item &itm = mod.items[mod.selection];
   Cstr_w app_name1 = itm.display_name;
   dword id = itm.id;
   mode = mod.saved_parent;
   {
      C_mode_item_list &mod_il = (C_mode_item_list&)*mode;
      if(mod_il.type==mod_il.ASSOCIATIONS){
         C_mode_item_list::S_item &itm1 = mod_il.items[mod_il.selection];
         itm1.internal_name = app_name1;
         itm1.id = id;
                                 //update/create association
         int i;
         if(mod_il.edited_assoc_ext.Length()){
            for(i=associations.Size(); i--; ){
               if(associations[i].ext == mod_il.edited_assoc_ext)
                  break;
            }
            if(i==-1){
               assert(0);
               return;
            }
         }else{
                                 //create new association
            i = associations.Size();
            associations.Resize(i+1);
         }
         associations[i].ext = itm1.display_name;
         S_association &asc = associations[i];
         asc.app_name = app_name1;
         asc.uid = id;
         SaveAssociations();
      }
   }
   RedrawScreen();
}

//----------------------------

void C_explorer_client::ItemList_SelectQuickFolder(C_mode_item_list &mod){

   Cstr_w s = quick_folders[mod.selection];
   if(!s.Length()){
      const C_client_file_mgr::C_mode_file_browser &mod_f = (C_client_file_mgr::C_mode_file_browser&)*mod.saved_parent;
      C_client_file_mgr::FileBrowser_GetFullName(mod_f, s, -1, true);
   }
   {
      C_client_file_mgr::C_mode_file_browser &mod_fb = C_client_file_mgr::SetModeFileBrowser(this, C_client_file_mgr::C_mode_file_browser::MODE_EXPLORER, true,
         (C_client_file_mgr::C_mode_file_browser::t_OpenCallback)&C_explorer_client::QuickFolders_SelectCallback,
         TXT_SELECT_FOLDER, s, C_client_file_mgr::GETDIR_DIRECTORIES);
      mod_fb.flags = mod_fb.FLG_ACCEPT_DIR | mod_fb.FLG_SELECT_OPTION | mod_fb.FLG_ALLOW_DETAILS | mod_fb.FLG_ALLOW_RENAME | mod_fb.FLG_AUTO_COLLAPSE_DIRS | mod_fb.FLG_ALLOW_DELETE | mod_fb.FLG_SAVE_LAST_PATH;
   }
}

//----------------------------

bool C_explorer_client::QuickFolders_SelectCallback(const Cstr_w *file, const C_vector<Cstr_w> *files){

   CloseMode(*mode, false);
   C_mode_item_list &mod = (C_mode_item_list&)*mode;
   Cstr_w &s = mod.items[mod.selection].display_name;
   mod.items[mod.selection].id = 1;
   s = *file;
   if(s.Length() && s[s.Length()-1]=='\\')
      s = s.Left(s.Length()-1);
   quick_folders[mod.selection] = s;
   SaveQuickFolders();
#ifdef UNIX_FILE_SYSTEM
   s = file_utils::ConvertPathSeparatorsToUnix(s);
#endif

   RedrawScreen();
   return true;
}

//----------------------------

void C_explorer_client::ItemListProcessMenu(C_mode_item_list &mod, int itm, dword menu_id){

   switch(itm){
   case TXT_TERMINATE:
   case TXT_CLOSE:
      ItemList_TerminateProcessOrTask(mod, true);
      return;

   case TXT_ACTIVATE:
      ItemList_ActivateTask(mod);
      break;

   case TXT_DELETE:
      switch(mod.type){
      case C_mode_item_list::ASSOCIATIONS:
         CreateQuestion(*this, TXT_Q_DELETE_ASSOCIATION, mod.items[mod.selection].display_name, new(true) C_del_assoc_question(*this), true);
         return;
      case C_mode_item_list::QUICK_DIRS:
         mod.items[mod.selection].display_name = GetText(TXT_NOT_SET);
         mod.items[mod.selection].id = 0;
         quick_folders[mod.selection].Clear();
         SaveQuickFolders();
         break;
      }
      break;

   case TXT_NEW:
      ItemList_EditOrNewAssociation(mod);
      break;

   case TXT_EDIT:
      ItemList_BeginEditAssociation(mod);
      return;

   case TXT_PAGE_UP:
   case TXT_PAGE_DOWN:
      {
         int s = mod.selection;
         int add = mod.sb.visible_space-1;
         if(itm==TXT_PAGE_UP)
            add = -add;
         s += add;
         s = Max(0, Min(mod.items.size()-1, s));
         if(mod.selection!=s){
            mod.selection = s;
            mod.EnsureVisible();
         }
      }
      break;

   case TXT_SCROLL:
      mod.menu = mod.CreateMenu();
      mod.menu->AddItem(TXT_PAGE_UP, 0, "[*]", "[q]");
      mod.menu->AddItem(TXT_PAGE_DOWN, 0, "[#]", "[a]");
      PrepareMenu(mod.menu);
      break;

   case TXT_CHANGE:
      ItemList_SelectQuickFolder(mod);
      return;

   case TXT_BACK:
      ItemList_Close(mod);
      return;
   }
}

//----------------------------

void C_explorer_client::ItemListProcessInput(C_mode_item_list &mod, S_user_input &ui, bool &redraw){

   MapScrollingKeys(ui);
   if(!mod.te_edit)
      mod.ProcessInputInList(ui, redraw);
#ifdef USE_MOUSE
   if(!ProcessMouseInSoftButtons(ui, redraw)){
      if(mod.te_edit){
         if(ProcessMouseInTextEditor(*mod.te_edit, ui))
            redraw = true;
      }
   }
#endif

   if(mod.te_edit){
      C_text_editor &te = *mod.te_edit;
      switch(ui.key){
      case K_LEFT_SOFT:
      case K_ENTER:
         {
                              //name confirmed
            Cstr_w ext = te.GetText();
            if(!ext.Length())
               break;
            ext.ToLower();
            C_mode_item_list::S_item &itm = mod.items[mod.selection];
            if(itm.display_name!=ext){
                              //changing extension, check if such extension doesn't exist yet
               int i;
               for(i=associations.Size(); i--; ){
                  if(associations[i].ext==ext)
                     break;
               }
               if(i!=-1)
                  break;
            }
            mod.edited_assoc_ext = itm.display_name;
            itm.display_name = ext;

            mod.te_edit = NULL;
            C_mode_item_list &mod_l = SetModeItemList(C_mode_item_list::CHOOSE_APP);
            for(int i=mod_l.items.size(); i--; ){
               if(mod_l.items[i].id==itm.id && mod_l.items[i].display_name==itm.internal_name){
                  mod_l.selection = i;
                  mod_l.EnsureVisible();
                  RedrawScreen();
                  break;
               }
            }
            return;
         }
         break;

      case K_RIGHT_SOFT:
      case K_ESC:
      case K_BACK:
         {
            mod.te_edit = NULL;
                              //cancel edit/create
            const C_mode_item_list::S_item &itm = mod.items[mod.selection];
            if(!itm.display_name.Length()){
               ItemList_DeleteAssociation();
            }else{
               //assert(0);
            }
            redraw = true;
         }
         break;
      }
   }else
   switch(ui.key){
   case K_LEFT_SOFT:
   case K_MENU:
      if(mod.type==mod.CHOOSE_APP){
         ItemList_ConfirmChooseApp(mod);
         return;
      }
      mod.menu = mod.CreateMenu();
      switch(mod.type){
      case C_mode_item_list::TASKS:
      case C_mode_item_list::PROCESSES:
         if(mod.items.size()){
            mod.menu->AddItem(mod.type==mod.TASKS ? TXT_CLOSE : TXT_TERMINATE);
            if(mod.type==mod.TASKS){
               mod.menu->AddItem(TXT_ACTIVATE, 0, "[Ok]");
            }
            mod.menu->AddSeparator();
         }
         break;
      case C_mode_item_list::ASSOCIATIONS:
         {
            bool en = (mod.items.size()!=0);
            mod.menu->AddItem(TXT_NEW, 0, "[9]", "[N]");
            mod.menu->AddItem(TXT_DELETE, en ? 0 : C_menu::DISABLED, delete_key_name);
            mod.menu->AddItem(TXT_EDIT, en ? 0 : C_menu::DISABLED, "[7]", "[E]");
            mod.menu->AddSeparator();
         }
         break;
      case C_mode_item_list::QUICK_DIRS:
         mod.menu->AddItem(TXT_CHANGE, 0, "[Ok]");
         mod.menu->AddItem(TXT_DELETE, mod.items[mod.selection].id ? 0 : C_menu::DISABLED, delete_key_name);
         mod.menu->AddSeparator();
         break;
      }
      if(mod.sb.visible && !HasMouse()){
         mod.menu->AddItem(TXT_SCROLL, C_menu::HAS_SUBMENU);
         mod.menu->AddSeparator();
      }
      mod.menu->AddItem(TXT_BACK);
      PrepareMenu(mod.menu);
      return;

   case K_RIGHT_SOFT:
   case K_BACK:
   case K_ESC:
                              //cancel
      if(mod.type==mod.CHOOSE_APP){
         mode = mod.saved_parent;
         C_mode_item_list &mod_il = (C_mode_item_list&)*mode;
         if(mod_il.type==mod_il.ASSOCIATIONS){
            C_mode_item_list::S_item &itm = mod_il.items[mod_il.selection];
                              //undo action - delete created, or rename back edited
            if(!itm.internal_name.Length())
               ItemList_DeleteAssociation();
            else
               itm.display_name = mod_il.edited_assoc_ext;
         }
         RedrawScreen();
         return;
      }
      ItemList_Close(mod);
      return;

   case '7':
   case 'e':
      if(mod.type==mod.ASSOCIATIONS && mod.items.size()){
         //ItemList_AssociationChooseApp(mod);
         ItemList_BeginEditAssociation(mod);
         redraw = true;
      }
      break;

   case '9':
   case 'n':
      if(mod.type==mod.ASSOCIATIONS){
         ItemList_EditOrNewAssociation(mod);
         redraw = true;
      }
      break;

   case K_DEL:
#ifdef _WIN32_WCE
   case 'd':
#endif
      switch(mod.type){
      case C_mode_item_list::TASKS:
      case C_mode_item_list::PROCESSES:
         ItemList_TerminateProcessOrTask(mod, true);
         return;
      case C_mode_item_list::ASSOCIATIONS:
         if(mod.items.size()){
            CreateQuestion(*this, TXT_Q_DELETE_ASSOCIATION, mod.items[mod.selection].display_name, new(true) C_del_assoc_question(*this), true);
            return;
         }
         break;
      case C_mode_item_list::QUICK_DIRS:
         if(mod.items[mod.selection].id){
            mod.items[mod.selection].display_name = GetText(TXT_NOT_SET);
            mod.items[mod.selection].id = 0;
            quick_folders[mod.selection].Clear();
            SaveQuickFolders();
            redraw = true;
         }
         break;
      }
      break;

   case K_ENTER:
      switch(mod.type){
      case C_mode_item_list::TASKS:
         ItemList_ActivateTask(mod);
         break;
      case C_mode_item_list::ASSOCIATIONS:
         break;
      case C_mode_item_list::CHOOSE_APP:
         ItemList_ConfirmChooseApp(mod);
         return;
      case C_mode_item_list::QUICK_DIRS:
         ItemList_SelectQuickFolder(mod);
         return;
      }
      break;
   }
}

//----------------------------

void C_explorer_client::DrawItemListContents(const C_mode_item_list &mod){

   const dword col_text = GetColor(COL_TEXT);
   ClearToBackground(mod.rc);

   S_rect rc_item;
   int ii = -1;
   int max_x = mod.sb.rc.x - fdb.cell_size_x/2;
   int x = mod.rc.x + fdb.cell_size_x/2;
   while(mod.BeginDrawNextItem(rc_item, ii)){
      const C_mode_item_list::S_item &itm = mod.items[ii];
      dword color = col_text;
      if(ii==mod.selection){
         DrawSelection(rc_item);
         color = GetColor(COL_TEXT_HIGHLIGHTED);
      }
      //if(i<mod.sb.visible_space-1)
      {
          int yy = rc_item.y+mod.entry_height;
          DrawSeparator(mod.rc.x, mod.rc.sx, yy);
      }
      int xx = x;
      int max_x1 = max_x;
      dword col = color;
      int txt_y = rc_item.y + (rc_item.sy-fdb.line_spacing)/2;
      switch(mod.type){
      case C_mode_item_list::ASSOCIATIONS:
         {
                              //draw app name
            int xx2 = xx+fdb.cell_size_x*4;
            DrawString(itm.internal_name, xx2, txt_y, UI_FONT_BIG, FF_BOLD, color, -(max_x1-xx));
            max_x1 = xx2;
         }
         break;
      case C_mode_item_list::QUICK_DIRS:
         {
            if(!mod.items[ii].id)
               col = MulAlpha(col, 0x8000);
            Cstr_w s;
            s.Format(L"%. ") <<((ii+1)%10);
            xx += DrawString(s, xx, txt_y, UI_FONT_BIG, 0, color);
         }
         break;
      }
      DrawString(itm.display_name, xx, txt_y, UI_FONT_BIG, 0, col, -(max_x1-xx));
   }
   DrawScrollbar(mod.sb);
}

//----------------------------

void C_explorer_client::DrawItemList(const C_mode_item_list &mod){

   {
      E_TEXT_ID ti;
      switch(mod.type){
      case C_mode_item_list::TASKS: ti = TXT_TASKS; break;
      case C_mode_item_list::PROCESSES: ti = TXT_PROCESSES; break;
      case C_mode_item_list::ASSOCIATIONS: ti = TXT_FILE_ASSOCIATIONS; break;
      case C_mode_item_list::QUICK_DIRS: ti = TXT_QUICK_FOLDERS; break;
      case C_mode_item_list::CHOOSE_APP: ti = TXT_SELECT_APPLICATION; break;
      default: assert(0); ti = TXT_NULL;
      }
      DrawTitleBar(GetText(ti));
   }

   const dword c0 = GetColor(COL_SHADOW), c1 = GetColor(COL_HIGHLIGHT);
   DrawOutline(mod.rc, c0, c1);
   {
      S_rect rc = mod.rc;
      rc.Expand();
      DrawOutline(rc, c1, c0);
   }
   DrawItemListContents(mod);
   ClearSoftButtonsArea(mod.rc.Bottom() + 2);

   {
      dword lsk, rsk;
      if(mod.te_edit){
         DrawEditedText(*mod.te_edit);
         lsk = TXT_OK, rsk = TXT_CANCEL;
      }else
      if(mod.type==mod.CHOOSE_APP)
         lsk = TXT_SELECT, rsk = TXT_CANCEL;
      else
         lsk = TXT_MENU, rsk = TXT_BACK;
      DrawSoftButtonsBar(mod, lsk, rsk, mod.te_edit);
   }
   SetScreenDirty();
}

//----------------------------

void C_explorer_client::S_config_xplore::SetPassword(const wchar *txt){

   int l = StrLen(txt);
   app_password = txt;
   while(l--)
      app_password.At(l) += 0x156;
}

//----------------------------

Cstr_w C_explorer_client::S_config_xplore::GetPassword() const{

   Cstr_w ret = app_password;
   for(int i=ret.Length(); i--; )
      ret.At(i) -= 0x156;
   return ret;
}

//----------------------------

void C_explorer_client::TxtNewPasswordEntered(const Cstr_w &txt){

   class C_text_entry: public C_text_entry_callback{
      C_explorer_client &app;
      Cstr_w pw;
      virtual void TextEntered(const Cstr_w &_txt){
         if(_txt==pw){
            app.config_xplore.SetPassword(_txt);
            CreateInfoMessage(app, TXT_INFORMATION, app.GetText(TXT_PASSWORD_SET));
            app.SaveConfig();
         }else
            app.ShowErrorWindow(TXT_ERROR, TXT_INVALID_PASSWORD);
      }
   public:
      C_text_entry(C_explorer_client &a, const Cstr_w &p): app(a), pw(p){}
   };
   if(!txt.Length()){
      config_xplore.SetPassword(txt);
      CreateInfoMessage(*this, TXT_INFORMATION, GetText(TXT_PASSWORD_CLEARED));
      SaveConfig();
   }else
      CreateTextEntryMode(*this, TXT_REPEAT_PASSWORD, new(true) C_text_entry(*this, txt), true, 1000, NULL, TXTED_ANSI_ONLY|TXTED_SECRET);
}

//----------------------------

void C_explorer_client::TxtCurrPasswordEntered(const Cstr_w &txt){

   Cstr_w pw = config_xplore.GetPassword();
   if(pw!=txt)
      ShowErrorWindow(TXT_ERROR, TXT_INVALID_PASSWORD);
   else{
      class C_text_entry: public C_text_entry_callback{
         C_explorer_client &app;
         virtual void TextEntered(const Cstr_w &_txt){
            app.TxtNewPasswordEntered(_txt);
         }
      public:
         C_text_entry(C_explorer_client &a): app(a){}
      };
      CreateTextEntryMode(*this, TXT_ENTER_NEW_PASSWORD, new(true) C_text_entry(*this), true, 1000, NULL, TXTED_ANSI_ONLY|TXTED_SECRET);
   }
}

//----------------------------

void C_explorer_client::TxtStartPasswordEntered(const Cstr_w &txt){
   Cstr_w pw = config_xplore.GetPassword();
   if(pw!=txt){
      system::Sleep(3000);
      ShowErrorWindow(TXT_ERROR, TXT_INVALID_PASSWORD);
   }else{
      FinishConstruct();
   }
}

//----------------------------

void C_explorer_client::TxtViewerFindEntered(C_mode &_mod, const Cstr_w &txt){

   if(!txt.Length())
      return;
   C_client_viewer::C_mode_this &mod = (C_client_viewer::C_mode_this&)_mod;
   last_find_text = txt;
                              //perform actual seek
   S_text_display_info &tdi = mod.text_info;
   int offs = tdi.byte_offs;
   if(tdi.mark_end){
      offs = Max(offs, tdi.mark_start);
      if(tdi.is_wide)
         offs += 2;
      else{
         if(S_text_style::IsControlCode(tdi.body_c[offs])){
            const char *cp = tdi.body_c+offs;
            S_text_style::SkipCode(cp);
            offs = cp-tdi.body_c;
         }else
            ++offs;
      }
   }
   int match_len;
   int fo = SearchText(tdi, offs, txt, true, match_len);
   if(fo==-1 && offs)
      fo = SearchText(tdi, 0, txt, true, match_len);
   if(fo!=-1){
      tdi.mark_start = fo;
      tdi.mark_end = fo + match_len;//txt.Length()*(tdi.is_wide?2:1);
                              //make sure the text is visible
      const S_font &fd = font_defs[tdi.ts.font_index];
      if(tdi.mark_start < int(tdi.byte_offs)){
         do{
            if(!ScrollText(tdi, -fd.line_spacing))
               break;
         }while(tdi.mark_start < int(tdi.byte_offs));
         ScrollText(tdi, -tdi.pixel_offs);
      }
                              //check if end mark is visible
      {
         const char *cp = tdi.is_wide ? (const char*)(const wchar*)tdi.body_w : (const char*)tdi.body_c;
         cp += tdi.byte_offs;
         int offs1 = tdi.byte_offs;
         int pix = tdi.pixel_offs;
         while(tdi.mark_end>offs1){
            int line_height;
            S_text_style ts = tdi.ts;
            int len = ComputeFittingTextLength(cp, tdi.is_wide, tdi.rc.sx, &tdi, ts, line_height, config.viewer_font_index);
            if(!len)
               break;
            offs1 += len;
            pix += line_height;
            (char*&)cp += len;
         }
         pix -= tdi.rc.sy;
         if(pix>0)
            ScrollText(tdi, pix);
      }
      mod.sb.pos = tdi.top_pixel;
      RedrawScreen();
   }else{
      ShowErrorWindow(TXT_ERR_TEXT_NOT_FOUND, txt);
   }
}

//----------------------------

void C_explorer_client::ViewerFindText(){

   class C_text_entry: public C_text_entry_callback{
      C_explorer_client &app;
      C_mode &mod;

      virtual void TextEntered(const Cstr_w &txt){
         app.TxtViewerFindEntered(mod, txt);
      }
   public:
      C_text_entry(C_explorer_client &a, C_mode &m): app(a), mod(m){}
   };
   CreateTextEntryMode(*this, TXT_FIND, new(true) C_text_entry(*this, *mode), true, MAX_FIND_TEXT_LEN, last_find_text);
}

//----------------------------

class C_find_files_arch_sim: public C_archive_simulator{
   struct S_file{
      Cstr_w display_name;
      Cstr_w full_name;
      dword size;
   };
   C_vector<S_file> files;
   virtual E_TYPE GetArchiveType() const{ return (E_TYPE)1001; }
   virtual const wchar *GetFileName() const{ return L"*FindResults"; }
   virtual void SetChangeObserver(t_Change c, void *context){}

//----------------------------

   virtual void *GetFirstEntry(const wchar *&name, dword &length) const{
      if(!files.size())
         return NULL;
      name = files.front().display_name;
      length = files.front().size;
      return (void*)1;
   }

//----------------------------

   virtual void *GetNextEntry(void *handle, const wchar *&name, dword &length) const{
      dword indx = (dword)handle;
      if(indx==NumFiles())
         return NULL;
      name = files[indx].display_name;
      length = files[indx].size;
      ++indx;
      return (void*)indx;
   }

//----------------------------

   virtual dword NumFiles() const{ return files.size(); }

   int FindIndex(const wchar *filename) const{
      for(int i=0; i<files.size(); i++){
         if(files[i].display_name==filename)
            return i;
      }
      return -1;
   }

//----------------------------

   virtual bool DeleteFile(const wchar *filename){
      int i = FindIndex(filename);
      if(i==-1)
         return false;
      bool ok = C_file::DeleteFile(files[i].full_name);
      files.erase(&files[i]);
      return ok;
   }

//----------------------------

   virtual const wchar *GetRealFileName(const wchar *filename) const{
      int i = FindIndex(filename);
      if(i==-1)
         return NULL;
      return files[i].full_name;
   }
public:
   C_find_files_arch_sim()
   {}

   void Construct(const C_vector<Cstr_w> &fls){
      files.reserve(fls.size());
      for(int i=0; i<fls.size(); i++){
         C_file fl;
         S_file f;
         f.full_name = fls[i];
         if(fl.Open(f.full_name)){
            f.size = fl.GetFileSize();
            f.display_name = f.full_name.Right(f.full_name.Length() - f.full_name.FindReverse('\\')-1);
            if(FindIndex(f.display_name)!=-1){
                              //avoid name duplicates
               int ei = f.display_name.FindReverse('.');
               Cstr_w name = ei==-1 ? f.display_name : f.display_name.Left(ei);
               Cstr_w ext;
               if(ei!=-1)
                  ext = f.display_name.Right(f.display_name.Length()-ei);
               for(int i1=1; i1<10000; i1++){
                  Cstr_w n1;
                  n1.Format(L"% (#02%)%") <<name <<i1 <<ext;
                  if(FindIndex(n1)==-1){
                     f.display_name = n1;
                     break;
                  }
               }
            }
            files.push_back(f);
         }
      }
   }

   virtual C_file *OpenFile(const wchar *filename) const{
      int i = FindIndex(filename);
      if(i!=-1){
         C_file *fp = new(true) C_file;
         if(fp->Open(files[i].full_name))
            return fp;
         delete fp;
      }
      return NULL;
   }
};

//----------------------------

struct S_search_pattern{
   Cstr_w name;
   Cstr_w ext;
};

//----------------------------
// Search wildcards:
// ? = one fixed char
// * = any number of chars
// other = actual char
static bool CheckIfNameMatchPattern(const wchar *name, const wchar *pattern){

#if 1
   while(*pattern){
      wchar cp = *pattern++;
      if(cp=='*'){
         if(!*pattern){
                              //matches everything in name till end
            return true;
         }
                              //recursively search remaining pattern in end of name
         for(int i=1; name[i]; i++){
            if(CheckIfNameMatchPattern(name+i, pattern))
               return true;
         }
      }else{
         wchar cn = *name++;
         if(!cn)              //if end of name, then no match
            return false;
         if(cp!='?'){         //? matches any single character in name
            if(cn!=cp)
               return false;
         }
      }
   }
#else
   while(*pattern){
      wchar c = *pattern++;
      wchar cn = *name++;
                              //check wild-cards
      if(c=='*')
         return true;
      if(!cn)
         return false;
      if(cn!=c){
         if(c!='?')
            return false;
      }
   }
#endif
   return (!*name);
}

//----------------------------

static bool CheckFileNameMatchPattern(const Cstr_w &fname, const S_search_pattern *patterns, dword num_patterns){

   Cstr_w name;
   const wchar *ext = NULL;
   name = fname.Right(fname.Length()-fname.FindReverse('\\') - 1);
   name.ToLower();
   int ei = name.FindReverse('.');
   if(ei!=-1){
      ext = (const wchar*)name+ei+1;
      name.At(ei) = 0;
   }else
      ext = L"";
   while(num_patterns--){
      const S_search_pattern &sp = *patterns++;

      bool match = true;
                              //check extension match
      if(sp.ext.Length())
         match = CheckIfNameMatchPattern(ext, sp.ext);
      if(match && CheckIfNameMatchPattern(name, sp.name))
         return true;
   }
   return false;
}

//----------------------------

void C_explorer_client::FileBrowserFindTextEntered(C_client_file_mgr::C_mode_file_browser &mod, const Cstr_w &txt){

   config_xplore.find_files_text = txt;
   SaveConfig();

   C_vector<S_search_pattern> search_patterns;
   {
                              //parse search pattern text
      Cstr_w s = txt;
      while(s.Length()){
         while(s.Length() && s[0]==' ')
            s = s.Right(s.Length()-1);
         int si = s.Find(' ');
         Cstr_w next;
         if(si!=-1){
            next = s.Right(s.Length()-si-1);
            s = s.Left(si);
         }
         S_search_pattern sf;
         int ei = s.FindReverse('.');
         if(ei!=-1){
            sf.ext = s.Right(s.Length()-ei-1);
            sf.ext.ToLower();
            s = s.Left(ei);
         }
         s.ToLower();
         sf.name = s;
         search_patterns.push_back(sf);
         s = next;
      }
   }
   if(!search_patterns.size() || !mod.entries.size())
      return;
   
   mod.selection = C_client_file_mgr::FileBrowser_GetDirEntry(mod);
   mod.EnsureVisible();
   Cstr_w dir;
   C_client_file_mgr::FileBrowser_GetFullName(mod, dir, -1, true);
                              //get all files in the dir
   C_vector<Cstr_w> files;
   C_client_file_mgr::FileBrowser_CollectFolderFiles(mod, dir, files);
                              //filter files by search text
   for(int i=files.size(); i--; ){
      if(!CheckFileNameMatchPattern(files[i], search_patterns.begin(), search_patterns.size())){
         files[i] = files.back();
         files.pop_back();
      }
   }

   if(!files.size()){
      Cstr_w s;
      s.Format(GetText(TXT_NO_FILES_FOUND_IN_FOLDER)) <<dir;
      ShowErrorWindow(TXT_NOT_FOUND, s);
      return;
   }
                              //feed into archive simulator, and browse it
   {
      C_client_file_mgr::C_mode_file_browser &mod_fb = C_client_file_mgr::SetModeFileBrowser(this, C_client_file_mgr::C_mode_file_browser::MODE_ARCHIVE_EXPLORER, false);
      mod_fb.flags &= ~(mod_fb.FLG_SAVE_LAST_PATH | mod_fb.FLG_ALLOW_SEND_BY | mod_fb.FLG_ALLOW_MAKE_DIR | mod_fb.FLG_ALLOW_RENAME | mod_fb.FLG_ALLOW_SHOW_SUBMENU | mod_fb.FLG_ALLOW_ATTRIBUTES);

      C_find_files_arch_sim *a = new(true) C_find_files_arch_sim;
      a->Construct(files);
      mod_fb.arch.SetPtr(a);
      a->Release();

      {
         C_client_file_mgr::C_mode_file_browser::S_entry e;
         e.type = e.ARCHIVE;
         e.name.Format(L"% (%)") <<GetText(TXT_SEARCH_RESULTS) <<files.size();
         e.level = 0;
         e.size.file_size = 0;
         mod_fb.entries.push_back(e);

         mod_fb.sb.total_space += mod_fb.entry_height;
         mod_fb.selection = 0;
         mod_fb.EnsureVisible();
      }
                              //expand top level
      C_client_file_mgr::FileBrowser_ExpandDir(this, mod_fb);
      RedrawScreen();
   }
}

//----------------------------
//----------------------------

bool C_explorer_client::SetModeHexEdit(const wchar *fn, const C_zip_package *arch, bool edit){

   C_file *fl = CreateFile(fn, arch);
   if(!fl)
      return false;
   bool can_edit = (!arch && fl->GetFileSize()<=C_mode_hex_edit::MAX_EDIT_FILE_SIZE);
   if(edit && !can_edit){
      delete fl;
      return false;
   }
   C_mode_hex_edit &mod = *new(true) C_mode_hex_edit(*this, fn, fl, can_edit);
   mod.display_name = mod.filename.Right(mod.filename.Length()-mod.filename.FindReverse('\\')-1);

   InitLayoutHexEdit(mod);

   if(edit && can_edit)
      HexEdit_BeginEdit(mod);
   ActivateMode(mod);
   return true;
}

//----------------------------

void C_explorer_client::HexEdit_BeginEdit(C_mode_hex_edit &mod){

   assert(!mod.IsEditMode());
   if(mod.file_size>mod.MAX_EDIT_FILE_SIZE)
      return;
   dword buf_sz = mod.file_size+16;
   mod.edit_buf = new byte[buf_sz];
   if(!mod.edit_buf)
      return;
   MemSet(mod.edit_buf, 0, buf_sz);
   mod.edit_buf_size = buf_sz;
   mod.fp->Seek(0);
   mod.fp->Read(mod.edit_buf, mod.file_size);

   const S_font &fd = font_defs[config.viewer_font_index];
   mod.te_edit = CreateTextEditor(TXTED_ANSI_ONLY, config.viewer_font_index, 0, NULL, 3);
   mod.te_edit->Release();
   C_text_editor &te = *mod.te_edit;
   te.SetRect(S_rect(2, 2, fd.cell_size_x*5, fd.line_spacing));
   te.SetCase(te.CASE_LOWER | te.CASE_UPPER, te.CASE_LOWER);
   te.SetInitText(L"**");
   te.SetCursorPos(1, 1);
#ifdef _DEBUG
   //mod.edit_chars = true;
#endif

   mod.EditEnsureVisible();
}

//----------------------------

void C_explorer_client::InitLayoutHexEdit(C_mode_hex_edit &mod){

   const int border = 1;

   mod.title_height = GetTitleBarHeight();
   mod.rc = S_rect(border, mod.title_height, ScrnSX()-border*2, ScrnSY()-mod.title_height);
   mod.rc.sy -= GetSoftButtonBarHeight()+1;
   mod.font_index = config.viewer_font_index;
   //mod.font_index = UI_FONT_SMALL;
   const S_font &fd = font_defs[mod.font_index];
   int num_l = mod.rc.sy/fd.line_spacing;
   mod.rc.sy = fd.line_spacing*num_l;
   if(system_font.use){
      mod.zero_char_width = system_font.GetCharWidth('0', false, false, mod.font_index);
   }else{
      mod.zero_char_width = internal_font.font_data[mod.font_index].width_empty_bits['0'-' '];
      if(mod.zero_char_width&0x80)
         mod.zero_char_width = (mod.zero_char_width&0x7f) - 1;
   }                        //init scrollbar
   int sb_size = GetScrollbarWidth();
   C_scrollbar &sb = mod.sb;
   sb.visible_space = mod.rc.sy;
   sb.rc = S_rect(mod.rc.Right()-sb_size-2, mod.rc.y+3, sb_size, mod.rc.sy-6);
   sb.visible_space = num_l;

   mod.x_bytes = mod.rc.x + fd.cell_size_x*6 + fd.cell_size_x/2;
   mod.sx_byte = fd.cell_size_x*2 + fd.cell_size_x/2;
   mod.sx_char = fd.letter_size_x+1;
   {
                              //compute number of fitting columns
      int w = mod.sb.rc.x - mod.x_bytes;

      w -= fd.cell_size_x/2;
      const int col_w = mod.sx_byte + mod.sx_char;
      mod.num_cols = w/col_w;
   }
   mod.x_chars = mod.x_bytes + mod.sx_byte*mod.num_cols + fd.cell_size_x/2;

   sb.total_space = (mod.Size()+mod.num_cols-1)/mod.num_cols;
   sb.SetVisibleFlag();
}

//----------------------------

void C_explorer_client::HexEditClose(C_mode_hex_edit &mod, bool ask){

   if(mod.te_edit)
      mod.te_edit->Activate(false);
   if(mod.modified && ask){
                              //ask to save changes
      class C_question: public C_question_callback{
         C_mode_hex_edit &mod;
         virtual void QuestionConfirm(){
            ((C_explorer_client&)mod.C_mode::app).HexEditClose(mod, false);
         }
         virtual void QuestionReject(){
            mod.modified = false;
            ((C_explorer_client&)mod.C_mode::app).HexEditClose(mod, false);
         }
      public:
         C_question(C_mode_hex_edit &_m): mod(_m){}
      };
      CreateQuestion(*this, TXT_Q_SAVE_CHANGES, mod.display_name, new(true) C_question(mod), true);
      return;
   }
   if(mod.modified && mod.IsEditMode()){
      mod.fp->Close();

      C_file fl;
      if(!fl.Open(mod.filename, C_file::FILE_WRITE)){
         Cstr_w s;
         s.Format(GetText(TXT_ERR_CANT_WRITE_FILE)) <<mod.display_name;
         ShowErrorWindow(TXT_ERROR, s);
         return;
      }
      fl.Write(mod.edit_buf, mod.Size());
      fl.Close();
                              //update file browser
      if(mod.saved_parent->Id() == C_client_file_mgr::ID){
         if(fl.Open(mod.filename)){
            C_client_file_mgr::C_mode_file_browser &mod_fb = (C_client_file_mgr::C_mode_file_browser&)*mod.saved_parent;
            C_client_file_mgr::C_mode_file_browser::S_entry &e = mod_fb.entries[mod_fb.selection];
            e.size.file_size = mod.Size();
         }
      }
   }
   mode = mod.saved_parent;
   RedrawScreen();
}

//----------------------------

static int FindTextInFile(const char *txt, dword len, C_file &fl, const byte *edit_buf, dword offs, int num_search, bool ignore_case){

   if(num_search==-1)
      num_search = fl.GetFileSize() - len + 1;
   while(num_search--){
      if(!edit_buf)
         fl.Seek(offs);
      dword i;
      for(i=0; i<len; i++){
         byte b1 = txt[i];
         byte b2;
         if(edit_buf)
            b2 = edit_buf[offs+i];
         else
         if(!fl.ReadByte(b2))
            return -1;
         if(ignore_case){
            b1 = ToLower(char(b1));
            b2 = ToLower(char(b2));
         }
         if(b1!=b2)
            break;
      }
      if(i==len)
         return offs;
      ++offs;
   }
   return -1;
}

//----------------------------

void C_explorer_client::TxtHexEditorFind(C_mode_hex_edit &mod, const Cstr_w &txt, bool hex){

   last_find_text = txt;
   mod.last_search_mode = hex ? mod.SEARCH_HEX : mod.SEARCH_CHARS;
   Cstr_c str; str.Copy(txt);

   const char *cp = str;
   dword cp_len = str.Length();
   C_vector<char> hex_buf;
   if(hex){
      while(*cp){
         text_utils::SkipWS(cp);
         if(!*cp)
            break;
         dword dw;
         if(!text_utils::ScanHexByte(cp, dw))
            break;
         hex_buf.push_back(char(dw));
      }
      cp = hex_buf.begin();
      cp_len = hex_buf.size();
      if(!cp_len)
         return;
   }

   int offs = mod.sb.pos * mod.num_cols;
   if(mod.mark_end)
      offs = Max(offs, int(mod.mark_start+1));
   const byte *edit_buf = mod.IsEditMode() ? mod.edit_buf : NULL;
   int fi = FindTextInFile(cp, cp_len, *mod.fp, edit_buf, offs, -1, !hex);
   if(fi==-1)
      fi = FindTextInFile(cp, cp_len, *mod.fp, edit_buf, 0, offs, !hex);
   if(fi!=-1){
      mod.mark_start = fi; mod.mark_end = fi+cp_len;
                              //make sure marked is visible
      mod.sb.pos = Min(mod.sb.pos, int(mod.mark_start/mod.num_cols));
      int end_line = (mod.mark_end-1)/mod.num_cols;
      mod.sb.pos = Max(mod.sb.pos, end_line-(mod.sb.visible_space-1));

      if(mod.IsEditMode()){
                              //set also cursor here
         mod.edit_y = mod.mark_start/mod.num_cols;
         mod.edit_x = (mod.mark_start - (mod.edit_y*mod.num_cols))*2;
         if(mod.te_edit)
            mod.te_edit->Activate(true);
      }

      RedrawScreen();
   }else
      ShowErrorWindow(TXT_ERR_TEXT_NOT_FOUND, txt);
}

//----------------------------

void C_explorer_client::C_mode_hex_edit::EditEnsureVisible(){

   if(edit_y<sb.pos)
      sb.pos = edit_y;
   else
   if(edit_y>=sb.pos+sb.visible_space)
      sb.pos = edit_y-(sb.visible_space-1);
}

//----------------------------

bool C_explorer_client::C_mode_hex_edit::EditCursorLeft(){

   if(edit_chars)
      edit_x &= ~1;
   if(!edit_x){
      if(!edit_y)
         return false;
      --edit_y;
      edit_x = num_cols*2;
   }
   edit_x -= (edit_chars ? 2 : 1);
   return true;
}

//----------------------------

void C_explorer_client::C_mode_hex_edit::EditCursorRight(){

   if(!Size())
      return;
   edit_x += (edit_chars ? 2 : 1);
   if(edit_x>=int(num_cols*2)){
      if(edit_y==sb.total_space-1){
         edit_x = num_cols*2;
      }else{
         edit_x = 0;
         ++edit_y;
         EditEnsureVisible();
      }
   }
   if(edit_y==sb.total_space-1){
      int r = Size()%num_cols;
      if(r)
         edit_x = Min(edit_x, r*2);
   }
}

//----------------------------

void C_explorer_client::C_mode_hex_edit::IncEditBuf(dword offs){

   if(file_size==edit_buf_size){
      dword new_sz = edit_buf_size+128;
      byte *nb = new byte[new_sz];
      if(!nb){
         insert_mode = false;
         return;
      }
      MemCpy(nb, edit_buf, offs);
      MemCpy(nb+offs+1, edit_buf+offs, file_size-offs);
      edit_buf_size = new_sz;
      delete[] edit_buf;
      edit_buf = nb;
   }else{
      for(int i=file_size; i>=int(offs); i--)
         edit_buf[i+1] = edit_buf[i];
   }
   ++file_size;
   sb.total_space = (Size()+num_cols-1)/num_cols;
   sb.SetVisibleFlag();
}

//----------------------------

void C_explorer_client::HexTextEditNotify(C_mode_hex_edit &mod, bool cursor_moved, bool text_changed, bool &redraw){

   redraw = true;
   C_text_editor &te = *mod.te_edit;
   const wchar *txt = te.GetText();
   if(cursor_moved || StrCmp(txt, L"**")){
      int ipos;
      if(te.GetInlineText(ipos))
         return;
      int len = te.GetTextLength();
      int pos = te.GetCursorPos();
      if(len==2){
         if(!pos){
            mod.EditCursorLeft();
            mod.EditEnsureVisible();
         }else
         if(pos>1){
            mod.EditCursorRight();
            mod.EditEnsureVisible();
         }
      }else
      if(len>=3){
         S_font::E_PUNKTATION punkt;
         dword c = S_internal_font::GetCharMapping(txt[1], punkt);
         if(mod.edit_chars){
         }else{
            c = ToLower(char(c));
            if(c>='0' && c<='9')
               c -= '0';
            else
               if(c>='a' && c<='f')
                  c = 10 + c - 'a';
               else
                  c = 0xffffffff;
         }
         if(c!=0xffffffff){
                              //write to buffer
            int offs = mod.edit_y*mod.num_cols + mod.edit_x/2;
            if(offs==int(mod.edit_buf_size) || (mod.insert_mode && !(mod.edit_x&1)))
               mod.IncEditBuf(offs);
            if(offs<int(mod.edit_buf_size)){

               byte &b = mod.edit_buf[offs];
               if(mod.edit_chars)
                  b = byte(c);
               else{
                  b &= (mod.edit_x&1) ? 0xf0 : 0xf;
                  b |= c << ((mod.edit_x&1) ? 0 : 4);
               }

               mod.modified = true;
            }
            mod.mark_start = mod.mark_end = 0;

            if(mod.edit_x==int(mod.num_cols*2)){
               ++mod.edit_y;
               mod.edit_x = 0;
               mod.EditEnsureVisible();
            }
            mod.EditCursorRight();
            mod.EditEnsureVisible();
         }
      }else
      if(len==1){
                              //delete byte
         bool can_del = true;
         if(!pos){
            //deleted back
            can_del = mod.EditCursorLeft();
         }
         if(can_del){
            dword offs = mod.edit_y*mod.num_cols + mod.edit_x/2;
            if(offs<mod.file_size){
               MemCpy(mod.edit_buf+offs, mod.edit_buf+offs+1, mod.file_size-offs);
               --mod.file_size;

               mod.modified = true;
               mod.mark_start = mod.mark_end = 0;

               mod.sb.total_space = (mod.Size()+mod.num_cols-1)/mod.num_cols;
               mod.sb.SetVisibleFlag();
               if(mod.edit_y>=mod.sb.total_space && mod.edit_y){
                  --mod.edit_y;
                  mod.edit_x = mod.num_cols*2;
               }
               mod.EditEnsureVisible();
               mod.edit_x &= ~1;
            }
         }
      }
      if(text_changed)
         te.SetInitText(L"**");
      te.SetCursorPos(1, 1);
   }
}

//----------------------------

void C_explorer_client::C_mode_hex_edit::FindChars(){

   class C_text_entry: public C_text_entry_callback{
      C_mode_hex_edit &mod;
      virtual void TextEntered(const Cstr_w &txt){
         mod.app.TxtHexEditorFindChars(mod, txt);
      }
   public:
      C_text_entry(C_mode_hex_edit &m): mod(m){}
   };
   CreateTextEntryMode(app, TXT_FIND_CHARS, new(true) C_text_entry(*this), true, MAX_FIND_TEXT_LEN, app.last_find_text);
} 

//----------------------------

void C_explorer_client::C_mode_hex_edit::FindHex(){

   class C_text_entry: public C_text_entry_callback{
      C_mode_hex_edit &mod;
      virtual void TextEntered(const Cstr_w &txt){
         mod.app.TxtHexEditorFindHex(mod, txt);
      }
   public:
      C_text_entry(C_mode_hex_edit &m): mod(m){}
   };
   CreateTextEntryMode(app, TXT_FIND_HEX, new(true) C_text_entry(*this), true, MAX_FIND_TEXT_LEN, app.last_find_text);
} 

//----------------------------

void C_explorer_client::HexEditProcessMenu(C_mode_hex_edit &mod, int itm, dword menu_id){

   switch(itm){

   case TXT_SCROLL:
      mod.menu = mod.CreateMenu();
      mod.menu->AddItem(TXT_PAGE_UP, 0, mod.IsEditMode() ? NULL : "[*]", "[q]");
      mod.menu->AddItem(TXT_PAGE_DOWN, 0, mod.IsEditMode() ? NULL : "[#]", "[a]");
      if(mod.IsEditMode()){
         mod.menu->AddItem(TXT_TOP, mod.edit_y ? 0 : C_menu::DISABLED);
         mod.menu->AddItem(TXT_BOTTOM, mod.edit_y<mod.sb.total_space-1 ? 0 : C_menu::DISABLED);
      }else{
         mod.menu->AddItem(TXT_TOP, mod.sb.pos ? 0 : C_menu::DISABLED);
         mod.menu->AddItem(TXT_BOTTOM, mod.sb.pos<mod.sb.total_space-mod.sb.visible_space ? 0 : C_menu::DISABLED);
      }
      PrepareMenu(mod.menu);
      break;

   case TXT_PAGE_UP:
   case TXT_PAGE_DOWN:
      {
         int step = mod.sb.visible_space-1;
         if(itm==TXT_PAGE_UP)
            step = -step;
         if(mod.IsEditMode()){
            mod.edit_y += step;
            mod.edit_y = Max(0, Min(mod.sb.total_space-1, mod.edit_y));
            if(mod.edit_y==mod.sb.total_space-1){
               int r = mod.Size()%mod.num_cols;
               if(r)
                  mod.edit_x = Min(mod.edit_x, r*2);
            }
            mod.EditEnsureVisible();
         }else{
            mod.sb.pos += step;
            mod.sb.pos = Max(0, Min(mod.sb.total_space-mod.sb.visible_space, mod.sb.pos));
         }
      }
      break;

   case TXT_TOP:
      if(mod.IsEditMode()){
         mod.edit_y = 0;
         mod.EditEnsureVisible();
      }else
         mod.sb.pos = 0;
      break;

   case TXT_BOTTOM:
      if(mod.IsEditMode()){
         mod.edit_y = Max(0, mod.sb.total_space-1);
         if(mod.edit_y==mod.sb.total_space-1){
            int r = mod.Size()%mod.num_cols;
            if(r)
               mod.edit_x = Min(mod.edit_x, r*2);
         }
         mod.EditEnsureVisible();
      }else
         mod.sb.pos = mod.sb.total_space-mod.sb.visible_space;
      break;

   case TXT_FIND_CHARS:
      mod.FindChars();
      return;

   case TXT_FIND_HEX:
      mod.FindHex();
      return;

   case TXT_FIND_NEXT:
      TxtHexEditorFind(mod, last_find_text, (mod.last_search_mode==mod.SEARCH_HEX));
      return;

   case TXT_INSERT_MODE:
      mod.insert_mode = !mod.insert_mode;
      break;

   case TXT_EDIT_HEX:
   case TXT_EDIT_CHARS:
      mod.edit_chars = !mod.edit_chars;
      break;

   case TXT_EDIT:
      HexEdit_BeginEdit(mod);
      break;

   case TXT_BACK:
      HexEditClose(mod);
      return;
   }
   if(!mod.menu && mod.te_edit)
      mod.te_edit->Activate(true);
}

//----------------------------

void C_explorer_client::HexEditProcessInput(C_mode_hex_edit &mod, S_user_input &ui, bool &redraw){

#ifdef USE_MOUSE
   if(!ProcessMouseInSoftButtons(ui, redraw)){
      C_scrollbar::E_PROCESS_MOUSE pm = ProcessScrollbarMouse(mod.sb, ui);
      switch(pm){
      case C_scrollbar::PM_PROCESSED:
      case C_scrollbar::PM_CHANGED:
         redraw = true;
         break;
      default:
         if(mod.IsEditMode())
            if(ui.mouse_buttons&MOUSE_BUTTON_1_DOWN)
         if(ui.CheckMouseInRect(mod.rc)){
            const S_font &fd = font_defs[mod.font_index];
            int l = (ui.mouse.y - mod.rc.y) / fd.line_spacing;
            l += mod.sb.pos;
            if(l>=0 && l<mod.sb.total_space){
               if(mod.edit_y!=l){
                  mod.edit_y = l;
                  redraw = true;
               }else
                  mod.te_edit->InvokeInputDialog();
               if(ui.mouse.x>=mod.x_chars){
                  int c = (ui.mouse.x-mod.x_chars) / mod.sx_char;
                  c = Max(0, Min(int(mod.num_cols), c));
                  int xx = c*2;
                  if(mod.edit_x!=xx){
                     mod.edit_x = xx;
                     redraw = true;
                  }
                  if(!mod.edit_chars){
                     mod.edit_chars = true;
                     redraw = true;
                  }
               }else{
                  int c = (ui.mouse.x-mod.x_bytes) / mod.sx_byte;
                  c = Max(0, Min(int(mod.num_cols), c));
                  int xx = c*2;
                  int x = ui.mouse.x - mod.x_bytes - c*mod.sx_byte;
                  if(x>mod.zero_char_width)
                     ++xx;
                  if(mod.edit_y==mod.sb.total_space-1){
                     int r = mod.Size()%mod.num_cols;
                     if(r)
                        xx = Min(xx, r*2);
                  }
                  if(mod.edit_x!=xx){
                     mod.edit_x = xx;
                     redraw = true;
                  }
                  if(mod.edit_chars){
                     mod.edit_chars = false;
                     redraw = true;
                  }
               }
               if(redraw)
                  mod.te_edit->ResetCursor();
            }
         }
      }
   }
#endif

   MapScrollingKeys(ui);
   switch(ui.key){
   case K_RIGHT_SOFT:
   case K_BACK:
   case K_ESC:
      HexEditClose(mod);
      return;

   case K_LEFT_SOFT:
   case K_MENU:
      if(mod.te_edit)
         mod.te_edit->Activate(false);
      mod.menu = mod.CreateMenu();
      mod.menu->AddItem(TXT_FIND_CHARS, 0, !mod.IsEditMode() ? "[1]" : NULL);
      mod.menu->AddItem(TXT_FIND_HEX, 0, !mod.IsEditMode() ? "[2]" : NULL);
      mod.menu->AddItem(TXT_FIND_NEXT, (last_find_text.Length() && mod.last_search_mode) ? 0 : C_menu::DISABLED, !mod.IsEditMode() ? "[3]" : NULL);
      mod.menu->AddSeparator();
      if(mod.IsEditMode()){
         mod.menu->AddItem(TXT_INSERT_MODE, mod.insert_mode ? C_menu::MARKED : 0);
         mod.menu->AddItem(mod.edit_chars ? TXT_EDIT_HEX : TXT_EDIT_CHARS, 0, send_key_name);
      }else
         mod.menu->AddItem(TXT_EDIT, mod.can_edit ? 0 : C_menu::DISABLED);
      mod.menu->AddSeparator();
      if(!HasMouse()){
         mod.menu->AddItem(TXT_SCROLL, C_menu::HAS_SUBMENU);
         mod.menu->AddSeparator();
      }
      mod.menu->AddItem(TXT_BACK);
      PrepareMenu(mod.menu);
      return;

   case '1':
      if(!mod.IsEditMode()){
         mod.FindChars();
         return;
      }
      break;

   case '2':
      if(!mod.IsEditMode()){
         mod.FindHex();
         return;
      }
      break;

   case '3':
      if(!mod.IsEditMode() && last_find_text.Length()){
         TxtHexEditorFind(mod, last_find_text, (mod.last_search_mode==mod.SEARCH_HEX));
         return;
      }
      break;

   case K_SEND:
      if(mod.IsEditMode()){
         mod.edit_chars = !mod.edit_chars;
         redraw = true;
      }
      break;

   case K_CURSORUP:
   case K_CURSORDOWN:
      if(mod.IsEditMode()){
         if(!mod.Size())
            break;
         int l = mod.edit_y;
         if(ui.key==K_CURSORUP){
            if(l)
               --l;
            else{
               mod.edit_x = 0;
               redraw = true;
               mod.te_edit->ResetCursor();
            }
         }else{
            if(l<mod.sb.total_space-1)
               ++l;
            else{
               int r = mod.Size()%mod.num_cols;
               if(!r) r = mod.num_cols;
               mod.edit_x = r*2;
               redraw = true;
               mod.te_edit->ResetCursor();
            }
         }
         if(mod.edit_y!=l){
            mod.edit_y = l;
            //mod.edit_x &= ~1;
            if(l==mod.sb.total_space-1){
               int r = mod.Size()%mod.num_cols;
               if(r)
                  mod.edit_x = Min(mod.edit_x, r*2);
            }else
               mod.edit_x = Min(mod.edit_x, int(mod.num_cols*2-1));
            mod.EditEnsureVisible();
            mod.te_edit->ResetCursor();
            redraw = true;
         }
         break;
      }
                              //flow...
   case '*':
   case '#':
   case 'q':
   case 'a':
      {
         int l = mod.sb.pos;
         int step = 1;
         if(ui.key=='*' || ui.key=='#' || ui.key=='q' || ui.key=='a')
            step = mod.sb.visible_space-1;
         if(ui.key==K_CURSORUP || ui.key=='*' || ui.key=='q')
            step = -step;
         l += step;
         l = Max(0, Min(mod.sb.total_space-mod.sb.visible_space, l));
         if(mod.sb.pos!=l){
            mod.sb.pos = l;
            redraw = true;
         }
      }
      break;
   default:
      if(mod.te_edit)
         mod.te_edit->Activate(true);
   }
}

//----------------------------

void C_explorer_client::DrawHexEdit(const C_mode_hex_edit &mod){

   const dword col_text = GetColor(COL_TEXT);

   ClearSoftButtonsArea(mod.rc.Bottom() + 1);
   //bool is_edit = mod.IsEditMode();
   {
      Cstr_w title;
      if(mod.modified){
         title <<L"* " <<mod.display_name;
      }else
         title = mod.display_name;
      DrawTitleBar(title);
   }
   DrawOutline(mod.rc, GetColor(COL_SHADOW), GetColor(COL_HIGHLIGHT));
   ClearToBackground(mod.rc);

   const S_font &fd = font_defs[mod.font_index];

   int y = mod.rc.y;
   const int x_offs = mod.rc.x+fds.cell_size_x/4;
   {
      dword col_line = MulAlpha(col_text, 0x3000);
      int x = mod.x_bytes - fd.cell_size_x/2, yb = mod.rc.Bottom()-1;
      DrawVerticalLine(x, y, yb-y, col_line);
      x = mod.x_chars-fd.cell_size_x/2;
      DrawVerticalLine(x, y, yb-y, col_line);
   }
   dword col_offs = MulAlpha(col_text, 0xc000);
   dword col_mark = ((((col_text>>16)&0xff) + ((col_text>>8)&0xff) + (col_text&0xff)) > 128*3) ? 0xffc0c0c0 : 0xff0000ff;

                              //alloc temp buffer for reading one line of data
   byte *tmp_buf = new(true) byte[mod.num_cols];
   if(!mod.IsEditMode()){
                              //seek to beginning of data
      mod.fp->Seek(mod.sb.pos*mod.num_cols);
   }

   if(!mod.sb.total_space){
                              //empty line, draw just offset
      DrawString(L"000000", x_offs, y, mod.font_index, 0, col_offs);
   }else
   for(int i=0; i<mod.sb.visible_space; i++){
      int li = mod.sb.pos + i;
      if(li>=mod.sb.total_space)
         break;
      dword offs = li*mod.num_cols;
                              //draw offset
      wchar s_offs[9];
      dword dw_tmp;
      for(int si=0; si<3; si++){
         const char *cp = text_utils::MakeHexString(byte((offs>>((2-si)*8))&0xff), dw_tmp, false);
         s_offs[si*2] = cp[0];
         s_offs[si*2+1] = cp[1];
      }
      s_offs[6] = 0;
      DrawString(s_offs, x_offs, y, mod.font_index, 0, col_offs);

      int xb = mod.x_bytes, xc = mod.x_chars;

      dword num_c = Min(mod.num_cols, mod.file_size-offs);
      if(!mod.IsEditMode())
         mod.fp->Read(tmp_buf, num_c);
      else
         MemCpy(tmp_buf, mod.edit_buf+li*mod.num_cols, num_c);
      for(dword ci=0; ci<num_c; ci++){
         byte b = tmp_buf[ci];
         const char *cp = text_utils::MakeHexString(b, dw_tmp, false);
         dword ct = col_text;
         bool marked = mod.IsMarked(offs);
         if(marked){
            int w = GetTextWidth(cp, false, COD_DEFAULT, mod.font_index);
            FillRect(S_rect(xb-1, y, w+2, fd.line_spacing), col_mark);
            ct ^= 0xffffff;
         }
         //DrawString2(cp, false, xb, y, COD_DEFAULT, mod.font_index, 0, ct, 0);
         DrawLetter(xb, y, cp[0], mod.font_index, 0, ct);
         DrawLetter(xb+mod.zero_char_width+fd.extra_space+1, y, cp[1], mod.font_index, 0, ct);
         xb += mod.sx_byte;

         dword col_chars = col_text;//(col_text&0xffffff) ? 0xffc0ffc0 : 0xff0000ff;
         if(b<' '){
            col_chars = col_offs;
            b = '.';
         }
         {
            int xx = xc;
            dword letter_width;
            if(system_font.use){
               letter_width = system_font.GetCharWidth(b, false, false, mod.font_index);
            }else{
               S_font::E_PUNKTATION punkt;
               dword char_map = S_internal_font::GetCharMapping(b, punkt);
               letter_width = internal_font.font_data[mod.font_index].width_empty_bits[char_map-' '];
            }
            if(letter_width&0x80)
               letter_width = (letter_width&0x7f) - 1;
            if(letter_width<dword(fd.letter_size_x))
               xx += (fd.letter_size_x - letter_width)/2;
            ct = col_chars;
            if(marked){
               ct ^= 0xffffff;
               FillRect(S_rect(xc, y, fd.letter_size_x+1, fd.line_spacing), col_mark);
            }
            DrawLetter(xx, y, b, mod.font_index, 0, ct);
         }
         xc += mod.sx_char;

         if(++offs==mod.Size())
            break;
      }
      y += fd.line_spacing;
   }
   delete[] tmp_buf;
   DrawScrollbar(mod.sb);

   if(mod.te_edit){
      const C_text_editor &te = *mod.te_edit;
                              //draw cursor
      int cursor_y = mod.edit_y - mod.sb.pos;
      if(cursor_y>=0 && cursor_y<mod.sb.visible_space){
         int x, y1 = mod.rc.y + cursor_y*fd.line_spacing;
         if(mod.edit_chars){
            x = mod.x_chars + mod.sx_char*(mod.edit_x/2);
         }else{
            x = mod.x_bytes + mod.sx_byte*(mod.edit_x/2);
            if(mod.edit_x&1)
               x += mod.zero_char_width + fd.extra_space+1;
         }
                              //draw inline edit text
         int ipos;
         int ilen = te.GetInlineText(ipos);
         //ilen = ipos = 1;
         if(ilen==1 && ipos==1){
                                 //draw inline edit text
            ClearToBackground(S_rect(x-1, y1, mod.zero_char_width+2, fd.cell_size_y+1));
            DrawLetter(x, y1, te.GetText()[ipos], mod.font_index, FF_UNDERLINE, 0xff0000ff);
         }

         if(te.IsCursorVisible()){
            if(mod.insert_mode)
               FillRect(S_rect(x-1, y1, 2, fd.cell_size_y+1), 0xffff0000);   
            else
               FillRect(S_rect(x-1, y1, mod.zero_char_width+2, fd.cell_size_y+1), 0x80ff0000);   
         }
      }
      //DrawEditedText(te);
   }
   DrawSoftButtonsBar(mod, TXT_MENU, TXT_BACK, mod.te_edit);
   SetScreenDirty();
}

//----------------------------

int C_explorer_client::SearchText(S_text_display_info &tdi, int offset, const Cstr_w &find_text, bool ignore_case, int &match_len) const{

   int t_len = tdi.Length();
   int f_len = find_text.Length();
   const wchar *cp_f = find_text;
   if(tdi.is_wide){
      offset /= 2;
      if(offset >= t_len)
         return -1;
      const wchar *cp_t = tdi.body_w;
      const wchar *cp = cp_t+offset;
      int cmp_len = t_len - offset - f_len + 1;
      while(cmp_len--){
         int i;
         for(i=0; i<f_len; i++){
            wchar c1 = cp[i];
            wchar c2 = cp_f[i];
            if(ignore_case){
               c1 = text_utils::LowerCase(c1);
               c2 = text_utils::LowerCase(c2);
            }
            if(c1!=c2)
               break;
         }
         if(i==f_len){
                              //found, return offset
            match_len = find_text.Length()*2;
            return (cp - cp_t) * sizeof(wchar);
         }
         ++cp;
      }
   }else{
      if(offset >= t_len)
         return -1;
      const char *cp_t = tdi.body_c;
      const char *cp = cp_t+offset;
      while(true){
         int i;
         const char *cp1 = cp;
         for(i=0; i<f_len; i++){
            char c = *cp1;
            if(!c){
               cp = NULL;
               break;
            }
            wchar c1;
            if(S_text_style::IsControlCode(c)){
               if(c==CC_WIDE_CHAR)
                  c1 = S_text_style::ReadWideChar(++cp1);
               else{
                  S_text_style::SkipCode(cp1);
                  c1 = 0;
               }
            }else{
               c1 = (wchar)encoding::ConvertCodedCharToUnicode(c, tdi.body_c.coding);
               ++cp1;
            }
            wchar c2 = cp_f[i];
            if(ignore_case){
               c1 = text_utils::LowerCase(c1);
               c2 = text_utils::LowerCase(c2);
            }
            if(c1!=c2)
               break;
         }
         if(i==f_len){
            match_len = cp1-cp;
            return cp - cp_t;
         }
         if(!cp)
            break;
                              //not found, try in next offset
         if(S_text_style::IsControlCode(*cp))
            S_text_style::SkipCode(cp);
         else
            ++cp;
      }
   }
   return -1;
}

//----------------------------
#ifdef __SYMBIAN32__
#include <hal.h>

void C_explorer_client::S_device_info::Read(){

   total_ram = free_ram = 0;
   HAL::Get(HAL::EMemoryRAM, (TInt&)total_ram);
   if(total_ram<16)           //bug of Symbian 6.1
      total_ram = 0;
   HAL::Get(HAL::EMemoryRAMFree, (TInt&)free_ram);
   if(!HAL::Get(HAL::ECPUSpeed, (TInt&)cpu_speed))
      cpu_speed /= 1000;
}

//----------------------------

void C_explorer_client::EnumProcesses(C_vector<C_mode_item_list::S_item> &items){

   TFindProcess fp;
   TFullName name;
   while(!fp.Next(name)){
      C_explorer_client::C_mode_item_list::S_item itm;
      itm.internal_name = (const wchar*)name.PtrZ();
      int ii = itm.internal_name.Find('[');
      if(ii!=-1)
         itm.display_name = itm.internal_name.Left(ii);
      else
         itm.display_name = itm.internal_name;
      items.push_back(itm);
   }
}

//----------------------------

bool C_explorer_client::C_mode_item_list::S_item::TerminateProcess(){

   RProcess p;
   if(p.Open(TPtrC((const word*)(const wchar*)internal_name, internal_name.Length())))
      return false;
   p.Kill(1);
   bool ret = (p.ExitReason()!=0);
   p.Close();
   return ret;
}

//----------------------------
#include <ApgCli.h>
#include <ApgTask.h>
#include <EikEnv.h>

void C_explorer_client::EnumTasks(C_vector<C_explorer_client::C_mode_item_list::S_item> &items, bool only_running){

   RApaLsSession as;
   as.Connect();
   as.GetAllApps();
   TApaTaskList tl(CCoeEnv::Static()->WsSession());

   TApaAppInfo ai;
   while(!as.GetNextApp(ai)){
      if(!ai.iCaption.Length())
         continue;
      if(ai.iCaption.Length()==1 && ai.iCaption[0]==' ')
         continue;
      if(only_running){
         TApaTask at = tl.FindApp(ai.iUid);
         if(!at.Exists())
            continue;
      }
      C_explorer_client::C_mode_item_list::S_item itm;
      itm.display_name = (const wchar*)ai.iCaption.PtrZ();
      itm.id = ai.iUid.iUid;
      //GetAppIcon()
      items.push_back(itm);
   }
   as.Close();
}

//----------------------------

bool C_explorer_client::C_mode_item_list::S_item::StartTask(){

   TUid uid = { id };

   TApaTaskList tl(CCoeEnv::Static()->WsSession());
   TApaTask at = tl.FindApp(uid);
   if(at.Exists()){
      at.BringToForeground();
      return true;
   }
   RApaLsSession as;
   as.Connect();

   TApaAppInfo ai;
   int err = as.GetAppInfo(ai, uid);
   if(err)
      return false;
   CApaCommandLine *cmd = CApaCommandLine::NewL();
#ifdef __SYMBIAN_3RD__
   cmd->SetExecutableNameL(ai.iFullName);
#else
   cmd->SetLibraryNameL(ai.iFullName);
#endif
   cmd->SetCommandL(EApaCommandOpen);
   err = as.StartApp(*cmd);
   delete cmd;
   as.Close();

   return (!err);
}

//----------------------------

bool C_explorer_client::C_mode_item_list::S_item::TerminateTask(){

   TApaTaskList tl(CCoeEnv::Static()->WsSession());
   TUid uid = { id };
   TApaTask at = tl.FindApp(uid);
   if(!at.Exists())
      return true;
   at.KillTask();
   return true;
}

//----------------------------
#include <MsvApi.h>
#include <MsvIds.h>
#ifdef S60
#include <BtMsgTypeUid.h>
#include <MmsConst.h>
#endif
#ifdef __SYMBIAN_3RD__
#ifdef S60
#include <IrMsgTypeUid.h>
#endif
#include <MMsvAttachmentManager.h>
#else
#ifdef S60
#include <irmsgtypeuid.h>
#include <btmsgtypeuid.h>
#endif
#endif

static void EnumMessageEntries(CMsvEntry *root, C_vector<C_mailbox_arch_sim::S_item> &items){

   CMsvEntrySelection *chlds = root->ChildrenL();
   for(int i=chlds->Count(); i--; ){
      TMsvId id = (*chlds)[i];
      CMsvEntry *e = root->ChildEntryL(id);
      const TMsvEntry &en = e->Entry();
      if(en.StandardFolder()){
         EnumMessageEntries(e, items);
      }else{
#ifdef S60
         if(
#ifdef __SYMBIAN_3RD__
            en.iMtm==KUidMsgTypeBt || en.iMtm==KUidMsgTypeIrUID || en.iMtm==KUidMsgTypeMultimedia
            //|| en.iMtm==KUidMsgTypeMultimedia
#ifdef _DEBUG
            || 1
#endif
#else
            en.iMtm.iUid==KUidMsgTypeBtTInt32 ||
            en.iMtm.iUid==KUidMsgTypeIr
#endif
            )
#endif
         {
            CMsvEntrySelection *chlds = e->ChildrenL();
            //Fatal("!", chlds->Count());
            if(chlds->Count()==1){
               CMsvEntry *ee = e->ChildEntryL((*chlds)[0]);
               if(ee->HasStoreL()){
                  //Fatal("!");
                  C_mailbox_arch_sim::S_item itm;
                  TFileName fn;
                  TTimeIntervalSeconds ti;
                  en.iDate.SecondsFrom(TTime(_L("19800000:000000")), ti);
                  int sec = ti.Int();
#ifdef __SYMBIAN_3RD__
                  CMsvStore *s = ee->ReadStoreL();
                  MMsvAttachmentManager &m = s->AttachmentManagerL();
                  if(m.AttachmentCount()==1){
                     CMsvAttachment *ai = m.GetAttachmentInfoL(0);
                     fn = ai->FilePath();
                     itm.full_filename = (const wchar*)fn.PtrZ();
                     fn = ai->AttachmentName();
                     itm.filename = (const wchar*)fn.PtrZ();
                     itm.size = ai->Size();
                     delete ai;
                  }
                  delete s;
                  sec += S_date_time::GetTimeZoneMinuteShift()*60;
#else
                  if(!ee->GetFilePath(fn)){
                     itm.full_filename = (const wchar*)fn.PtrZ();
                     fn = en.iDescription;
                     itm.filename = (const wchar*)fn.PtrZ();

                     itm.full_filename <<itm.filename;
                     itm.full_filename.Build();

                     itm.size = ee->Entry().iSize;
                  }
#endif
                  itm.modify_date.SetFromSeconds(sec);

                  itm.id = id;
                  items.push_back(itm);
               }
               delete ee;
            }
            delete chlds;
#ifdef _DEBUG
         }else{
            /*
            Info("!", en.iMtm.iUid);
            C_explorer_client::C_mode_item_list::S_item itm;
            TFileName fn = en.iDescription;
            itm.display_name = (const wchar*)fn.PtrZ();
            itm.id = id;
            items.push_back(itm);
            /**/
#endif
         }
      }
      delete e;
   }
   delete chlds;
}

//----------------------------

class C_messaging_session_imp: public C_messaging_session, public MMsvSessionObserver{

   virtual void HandleSessionEventL(TMsvSessionEvent aEvent, TAny* aArg1, TAny* aArg2, TAny* aArg3){
      if(lock)
         return;
      switch(aEvent){
      case EMsvEntriesCreated:
      case EMsvEntriesDeleted:
         if(change_callback)
            change_callback(change_context);
         break;
      case EMsvEntriesChanged:
         break;
      //default: Fatal("msg ev", aEvent);
      }
   }
   bool lock;
public:
   CMsvSession *ms;

   C_messaging_session_imp():
      ms(NULL),
      lock(false)
   {}
   void Construct(){
      ms = CMsvSession::OpenSyncL(*this);
   }
   ~C_messaging_session_imp(){
      delete ms;
   }
   void Lock(){ lock = true; }
   void Unlock(){ lock = false; }
};

C_messaging_session *C_messaging_session::Create(){
   C_messaging_session_imp *ms = new(true) C_messaging_session_imp;
   ms->Construct();
   return ms;
}

//----------------------------

void C_mailbox_arch_sim::EnumMessages(C_messaging_session *_ms){

   items.clear();
   C_messaging_session_imp *ms = (C_messaging_session_imp*)_ms;
   if(!ms->ms)
      return;
   ms->Lock();
   CMsvEntry *root = ms->ms->GetEntryL(KMsvRootIndexEntryId);
   EnumMessageEntries(root, items);
   delete root;
   ms->Unlock();
}

//----------------------------

static bool DeleteMessageEntry(CMsvEntry *root, TMsvId del_id){

   bool ok = false;
   CMsvEntrySelection *chlds = root->ChildrenL();
   for(int i=chlds->Count(); i-- && !ok; ){
      TMsvId id = (*chlds)[i];
      if(id==del_id){
         root->DeleteL(id);
         ok = true;
      }else
         ok = DeleteMessageEntry(root->ChildEntryL(id), del_id);
   }
   delete chlds;
   delete root;
   return ok;
}

//----------------------------

bool C_mailbox_arch_sim::S_item::DeleteMessage(C_messaging_session *_ms){

   bool ok = false;
   C_messaging_session_imp *ms = (C_messaging_session_imp*)_ms;
   if(ms->ms){
      ms->Lock();
      ok = DeleteMessageEntry(ms->ms->GetEntryL(KMsvRootIndexEntryId), id);
      ms->Unlock();
   }
   return ok;
}

//----------------------------

bool C_mailbox_arch_sim::S_item::SetMessageAsRead(C_messaging_session *_ms){

   bool ok = false;
   C_messaging_session_imp *ms = (C_messaging_session_imp*)_ms;
   if(ms->ms){
      ms->Lock();
      CMsvEntrySelection *sel = new(true) CMsvEntrySelection;
      sel->AppendL(id);
      ms->ms->ChangeAttributesL(*sel, 0, KMsvUnreadAttribute);
      delete sel;
      ms->Unlock();
   }
   return ok;
}

//----------------------------

#ifdef __SYMBIAN_3RD__

//----------------------------

class C_file_msg: public C_file
   /*
   , public MMsvSessionObserver{
   virtual void HandleSessionEventL(TMsvSessionEvent aEvent, TAny* aArg1, TAny* aArg2, TAny* aArg3){
   }
   */
{

   class C_imp: public C_file::C_file_base{
      enum{ CACHE_SIZE = 0x2000 };

      byte *top, *curr;
      byte base[CACHE_SIZE];
      RFile file;

      dword curr_pos;         //real file pos
      int file_size;

      dword ReadCache(){
         TPtr8 desc(curr=base, CACHE_SIZE);
         int err = file.Read(desc);
         if(err)
            Fatal("File read error", err);
         return desc.Size();
      }
   public:
      C_imp(MMsvAttachmentManager &mma):
         top(NULL), curr(NULL),
         curr_pos(0),
         file_size(-1)
      {
         file = mma.GetAttachmentFileL(0);
         if(file.Size(file_size)!=KErrNone)
            file_size = -1;
      }
      ~C_imp(){
         file.Close();
      }

      virtual E_FILE_MODE GetMode() const{ return FILE_READ; }

   //----------------------------

      virtual bool Read(void *mem, dword len){
         dword read_len = 0;
         const dword wanted_len = len;
         dword rl = 0;
         while(curr+len>top){
            dword rl1 = top-curr;
            MemCpy(mem, curr, rl1);
            len -= rl1;
            mem = (byte*)mem + rl1;
            dword sz = ReadCache();
            top = base + sz;
            curr_pos += sz;
            rl += rl1;
            if(!sz){
               read_len = rl;
               goto check_read_size;
               //return rl;
            }
         }
         MemCpy(mem, curr, len);
         curr += len;
         read_len = rl + len;
      check_read_size:
         return (read_len == wanted_len);
      }

   //----------------------------

      virtual bool ReadByte(byte &v){
         if(curr==top){
            dword sz = ReadCache();
            if(sz < sizeof(byte))
               return false;
            top = base + sz;
            curr_pos += sz;
         }
         v = *curr++;
         return true;
      }

   //----------------------------

      virtual bool IsEof() const{
         return (curr==top && (dword)file_size==GetCurrPos());
      }
      virtual dword GetCurrPos() const{ return curr_pos+curr-top; }
      virtual bool SetPos(dword pos){
         pos = Min(pos, file_size);
         if(pos<GetCurrPos()){
            if(pos >= curr_pos-(top-base)){
               curr = top-(curr_pos-pos);
            }else{
               curr = top = base;
               TInt ipos = pos;
               file.Seek(ESeekStart, ipos);
               curr_pos = pos;
            }
         }else
         if(pos > curr_pos){
            curr = top = base;
            TInt ipos = pos;
            file.Seek(ESeekStart, ipos);
            curr_pos = pos;
         }else
            curr = top + pos - curr_pos;
         return true;
      }
      virtual dword GetFileSize() const{ return file_size; }
   };

public:
   /*
   CMsvSession *ms;

   C_file_msg():
      ms(NULL)
   {}
   ~C_file_msg(){
      delete ms;
   }
   */
   void Construct(MMsvAttachmentManager &mma){
      imp = new(true) C_imp(mma);
   }
};

//----------------------------

static bool MessageOpenAttachment(CMsvEntry *root, dword msg_id, C_file_msg *fl){

   bool ok = false;
   CMsvEntrySelection *chlds = root->ChildrenL();
   for(int i=chlds->Count(); i-- && !ok; ){
      TMsvId id = (*chlds)[i];
      CMsvEntry *e = root->ChildEntryL(id);
      const TMsvEntry &en = e->Entry();
      if(en.StandardFolder()){
         ok = MessageOpenAttachment(e, msg_id, fl);
      }else{
         if(id==msg_id){
            CMsvEntrySelection *chlds = e->ChildrenL();
            CMsvEntry *ee = e->ChildEntryL((*chlds)[0]);
            CMsvStore *s = ee->ReadStoreL();
            MMsvAttachmentManager &m = s->AttachmentManagerL();
            if(m.AttachmentCount()==1){
               //Fatal("!1");
               fl->Construct(m);
               ok = true;
            }
            delete s;
            delete ee;
            delete chlds;
         }
      }
      delete e;
   }
   delete chlds;
   return ok;
}

//----------------------------

C_file *C_mailbox_arch_sim::S_item::CreateFile(const C_messaging_session *_ms) const{

   if(!CActiveScheduler::Current())
      return NULL;
   C_file_msg *fl = NULL;
   C_messaging_session_imp *ms = (C_messaging_session_imp*)_ms;
   if(ms->ms){
      ms->Lock();
      fl = new(true) C_file_msg;
#if 0
      CActiveScheduler *as_tmp = NULL, *as_curr = CActiveScheduler::Current();
      if(!as_curr){            //install AS if it's not (run in thread)
         as_tmp = new(ELeave) CActiveScheduler;
         CActiveScheduler::Install(as_tmp);
      }
      fl->ms = CMsvSession::OpenSyncL(*fl);
      Fatal("!aa");
      CMsvEntry *root = fl->ms->GetEntryL(KMsvRootIndexEntryId);
#else
      CMsvEntry *root = ms->ms->GetEntryL(KMsvRootIndexEntryId);
#endif
      if(!MessageOpenAttachment(root, id, fl)){
         delete fl;
         fl = NULL;
      }
      delete root;
      //if(!as_curr) CActiveScheduler::Install(NULL);
      //delete as_tmp;
      ms->Unlock();
   }
   return fl;
}

#else

C_file *C_mailbox_arch_sim::S_item::CreateFile(const C_messaging_session *_ms) const{

   C_file *fl = new(true) C_file;
   if(!fl->Open(full_filename)){
      delete fl;
      fl = NULL;
   }
   return fl;
}

#endif

//----------------------------

bool C_wallpaper_utils::CanSetWallpaper(){
#if defined S60 && defined __SYMBIAN_3RD__
   RProcess pr;
   return pr.HasCapability(ECapabilityWriteDeviceData);
#else
   return false;
#endif
}

//----------------------------
#if defined S60 && defined __SYMBIAN_3RD__
#include <AknsWallpaperUtils.h>
#endif

bool C_wallpaper_utils::SetWallpaper(const wchar *filename){

#if defined S60 && defined __SYMBIAN_3RD__
   return (!AknsWallpaperUtils::SetIdleWallpaper(TPtrC((const word*)filename, StrLen(filename)), CCoeEnv::Static()));
#else
   return false;
#endif
}

//----------------------------

#endif
//----------------------------
#if defined _WINDOWS || defined _WIN32_WCE
#include <String.h>
namespace win{
#include <Windows.h>
#include <Tlhelp32.h>
}
#undef CreateFile
#undef DeleteFile

void C_explorer_client::S_device_info::Read(){
   win::MEMORYSTATUS mem;

   mem.dwLength = sizeof(mem);
   win::GlobalMemoryStatus(&mem);
   total_ram = mem.dwTotalPhys;
   free_ram = mem.dwAvailPhys;
}

//----------------------------

void C_explorer_client::EnumProcesses(C_vector<C_mode_item_list::S_item> &items){

   win::PROCESSENTRY32 pe;
   pe.dwSize = sizeof(pe);
   win::HANDLE h_snap = win::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
   if(win::Process32First(h_snap, &pe)){
      do{
         C_explorer_client::C_mode_item_list::S_item itm;
         itm.id = pe.th32ProcessID;
         itm.display_name = pe.szExeFile;
         items.push_back(itm);
      }while(win::Process32Next(h_snap, &pe));
   }
   win::CloseHandle(h_snap);
#ifdef _DEBUG
   C_explorer_client::C_mode_item_list::S_item itm;
   itm.display_name = L"A very long process name quite long";
   items.push_back(itm);
#endif
}

//----------------------------

bool C_explorer_client::C_mode_item_list::S_item::TerminateProcess(){
   win::HANDLE h = win::OpenProcess(PROCESS_TERMINATE, false, id);
   if(!h){
      //int err = win::GetLastError();
      return false;
   }
   bool ret = win::TerminateProcess(h, 0);
   win::CloseHandle(h);
   return ret;
}

//----------------------------

void C_explorer_client::EnumTasks(C_vector<C_explorer_client::C_mode_item_list::S_item> &items, bool only_running){

   C_explorer_client::C_mode_item_list::S_item itm;
   itm.display_name = L"SmartMovie";
   itm.id = 0x10001234;
   items.push_back(itm);
   for(int i=0; i<20; i++){
      itm.display_name = L"ProfiMail ";
      itm.display_name<<i;
      itm.id = 0x10004321;
      items.push_back(itm);
   }
}

//----------------------------

bool C_explorer_client::C_mode_item_list::S_item::StartTask(){
   return false;
}

//----------------------------

bool C_explorer_client::C_mode_item_list::S_item::TerminateTask(){
   return false;
}

//----------------------------

C_messaging_session *C_messaging_session::Create(){
   return new(true) C_messaging_session;
}

//----------------------------

void C_mailbox_arch_sim::EnumMessages(class C_messaging_session *ms){

   items.clear();

   S_item itm;
   itm.filename = L"Shoping List.txt";
   itm.full_filename = L"Lang\\Explorer\\English.txt";
   itm.size = 124445;
   C_file::GetFileWriteTime(itm.full_filename, itm.modify_date);
   items.push_back(itm);

   itm.filename = L"Anim.gif";
   itm.full_filename = L"D:\\1\\Anim.gif";
   itm.size = 8934;
   C_file::GetFileWriteTime(itm.full_filename, itm.modify_date);
   items.push_back(itm);

   itm.filename = L"A.zip";
   itm.full_filename = L"D:\\Develope\\Mobile\\Apps\\Videos\\Zip\\Text.zip";
   itm.size = 124445;
   C_file::GetFileWriteTime(itm.full_filename, itm.modify_date);
   items.push_back(itm);

   itm.filename = L"Test.wav";
   itm.full_filename = L"D:\\Develope\\Mobile\\Apps\\Videos\\audio\\test.wav";
   itm.size = 1767664;
   C_file::GetFileWriteTime(itm.full_filename, itm.modify_date);
   items.push_back(itm);
}

//----------------------------

bool C_mailbox_arch_sim::S_item::DeleteMessage(C_messaging_session*){
   return true;
}

//----------------------------

bool C_mailbox_arch_sim::S_item::SetMessageAsRead(C_messaging_session *ms){
#ifdef _DEBUG
   ms->SimulateChangeCallback();
#endif
   return true;
}

//----------------------------

C_file *C_mailbox_arch_sim::S_item::CreateFile(const C_messaging_session*) const{

   C_file *fl = new(true) C_file;
                              //debug - open some file
   if(!fl->Open(full_filename)){
      delete fl;
      fl = NULL;
   }
   return fl;
}

//----------------------------

bool C_wallpaper_utils::CanSetWallpaper(){
#ifdef _DEBUG
   return true;
#else
   return false;
#endif
}

//----------------------------

bool C_wallpaper_utils::SetWallpaper(const wchar *filename){
   return true;
}
#endif//_WINDOWS

//----------------------------
#ifdef __SYMBIAN32__
#include <charconv.h>
#endif

void C_explorer_client::CloseCharConv(){
#ifdef __SYMBIAN32__
   delete (CCnvCharacterSetConverter*)char_conv;
   char_conv = NULL;
#endif
}

//----------------------------

void C_explorer_client::ConvertMultiByteStringToUnicode(const char *src, E_TEXT_CODING coding, Cstr_w &dst) const{

   switch(coding){
   case COD_BIG5:
   case COD_GB2312:
   case COD_GBK:
   case COD_SHIFT_JIS:
   case COD_JIS:
   case COD_EUC_KR:
   case COD_HEBREW:
      {
         C_buffer<wchar> tmp;
         dword src_len = StrLen(src);
         tmp.Resize(src_len);
         wchar *wt = tmp.Begin();
#ifdef __SYMBIAN32__
         if(!char_conv)
            char_conv = CCnvCharacterSetConverter::NewL();
         CCnvCharacterSetConverter *cnv = (CCnvCharacterSetConverter*)char_conv;
         dword cp;
         switch(coding){
         case COD_BIG5: cp = KCharacterSetIdentifierBig5; break;
         case COD_GB2312: cp = KCharacterSetIdentifierGb2312; break;
         case COD_GBK: cp = KCharacterSetIdentifierGbk; break;
         case COD_SHIFT_JIS: cp = KCharacterSetIdentifierShiftJis; break;
         case COD_JIS: cp = KCharacterSetIdentifierIso2022Jp; break;
         case COD_HEBREW: cp = KCharacterSetIdentifierIso88598; break;
         case COD_EUC_KR: cp = 0x2000e526; break; //KCharacterSetIdentifierEUCKR;
         default:
            dst.Copy(src);
            return;
         }
         CCnvCharacterSetConverter::TAvailability av = cnv->PrepareToConvertToOrFromL(cp, CCoeEnv::Static()->FsSession());
         if(av!=CCnvCharacterSetConverter::EAvailable){
            dst.Copy(src);
            return;
         }
         TInt state = CCnvCharacterSetConverter::KStateDefault;
         TPtr des((word*)wt, src_len);
         int n = cnv->ConvertToUnicode(des, TPtrC8((byte*)src, src_len), state);
         if(n<0){
            assert(0);
            dst.Copy(src);
            return;
         }
         assert(!n);
         n = des.Length();
#elif defined _WINDOWS || defined _WIN32_WCE
                              //codepages: msdn2.microsoft.com/en-us/library/ms776446.aspx
         int cp;
         switch(coding){
         default:
         case COD_BIG5: cp = 950; break;
         case COD_GB2312: cp = 936; break;
         case COD_GBK: cp = 936; break;
         case COD_SHIFT_JIS: cp = 932; break;
         case COD_JIS: cp = 50220; break;
         case COD_EUC_KR: cp = 949; break;
         case COD_HEBREW: cp = 1255; break;
         }
         dword n;
         if(!src_len)
            n = 0;
         else{
            n = win::MultiByteToWideChar(cp, 0, src, src_len, wt, src_len);
            if(!n){
               dst.Copy(src);
               return;
            }
         }
#else
         dst.Copy(src);
         dword n = 0;
         return;
#endif
         dst.Allocate(wt, n);
      }
      break;
   default:
      C_client::ConvertMultiByteStringToUnicode(src, coding, dst);
   }
}

//----------------------------

void C_explorer_client::OpenDocument(const wchar *fname){

   Cstr_w fn = fname;
   /*
   while(mode && mode->Id()!=C_client_file_mgr::ID){
      mode->Close();
   }
   */
   mode = NULL;
   config_xplore.last_browser_path = fname;
   C_client_file_mgr::C_mode_file_browser &mod = C_client_file_mgr::SetModeFileBrowser(this, C_client_file_mgr::C_mode_file_browser::MODE_EXPLORER, true, NULL, TXT_NULL, fn);
   //C_client_file_mgr::FileBrowser_OpenFile(mod);
   //C_client_file_mgr::FileBrowser_GoToPath(this, mod, fname, true);
   //C_client_viewer::OpenFileForViewing(this, fname, file_utils::GetFileNameNoPath(fn), NULL, NULL, NULL, &mod);

   //RedrawScreen();
   CreateImageViewer(*this, &mod, file_utils::GetFileNameNoPath(fn), fn);
   //UpdateScreen();
}

//----------------------------

#include <Util.h>
#include <C_file.h>
#include <RndGen.h>
#include <Directory.h>
#include <Md5.h>
#include "Main.h"

//----------------------------

#ifdef _DEBUG

#ifdef _WIN32_WCE
#else
#define DEBUG_SN "354864047281424"
//#define DEBUG_SN "359575040342642"
#endif

//----------------------------
#define DEBUG_NETWORK_LOG

//#define FONT_TEST

#endif

extern const int MAX_SYSTEM_FONT_SIZE_DELTA = 24;
const int MAX_INTERNAL_FONT_SIZE_DELTA = 2;

//----------------------------

C_client::S_config::S_config():
   flags(CONF_PRG_AUTO_UPDATE// | CONF_DRAW_COUNTER_TOTAL
      ),
   //img_ratio(C_fixed::Percent(60)),
   last_auto_update(-1),
   ui_font_size_delta(0),
   viewer_font_index(0),
   color_theme(0),
   total_data_sent(0),
   total_data_received(0),
   use_system_font(false),
   fullscreen(true),
   app_language("English")
{
}

//----------------------------

Cstr_w C_client::GetConfigFilename() const{

   Cstr_w fn; fn<<data_path <<L"config.txt";
   return fn;
}

//----------------------------

const S_config_store_type C_client::save_config_values_base[] = {
   { S_config_store_type::TYPE_DWORD, OffsetOf(S_config,flags), "flags1" },
   { S_config_store_type::TYPE_BYTE, OffsetOf(S_config,viewer_font_index), "viewer_font_index" },
   { S_config_store_type::TYPE_BYTE, OffsetOf(S_config,fullscreen), "fullscreen" },
   { S_config_store_type::TYPE_DWORD, OffsetOf(S_config,last_auto_update), "last_auto_update" },
   //{ S_config_store_type::TYPE_DWORD, OffsetOf(S_config,img_ratio), "img_ratio" },
   { S_config_store_type::TYPE_CSTR_C, OffsetOf(S_config,app_language), "app_language" },
   { S_config_store_type::TYPE_CSTR_W, OffsetOf(S_config,last_browser_path), "last_browser_path" },
   { S_config_store_type::TYPE_DWORD, OffsetOf(S_config,color_theme), "color_theme1" },
#if defined __SYMBIAN32__
   { S_config_store_type::TYPE_DWORD, OffsetOf(S_config,iap_id), "iap_id" },
   { S_config_store_type::TYPE_DWORD, OffsetOf(S_config,secondary_iap_id), "alt_iap_id" },
#else
   { S_config_store_type::TYPE_CSTR_W, OffsetOf(S_config,wce_iap_name), "iap_name" },
#endif
   { S_config_store_type::TYPE_DWORD, OffsetOf(S_config,total_data_sent), "total_data_sent" },
   { S_config_store_type::TYPE_DWORD, OffsetOf(S_config,total_data_received), "total_data_received" },
   { S_config_store_type::TYPE_DWORD, OffsetOf(S_config,connection_time_out), "connection_time_out" },
   { S_config_store_type::TYPE_BYTE, OffsetOf(S_config,use_system_font), "use_system_font" },
   { S_config_store_type::TYPE_NULL }
};

//----------------------------

static int CompareStrings(const void *a1, const void *a2, void *context){

   const Cstr_c &s1 = *(Cstr_c*)a1;
   const Cstr_c &s2 = *(Cstr_c*)a2;
   return StrCmp(s1, s2);
}

//----------------------------

C_client::C_mode_config_client::C_mode_config_client(C_client &_app, const S_config_item *opts, dword num_opts, C_configuration_editing *ce,
   dword title_id, void *_cfg_base):
   C_mode_configuration(_app, ce ? ce : new C_configuration_editing_client(_app))
{
   if(!_cfg_base)
      _cfg_base = &App().config;
   Init(opts, num_opts, _cfg_base, title_id);

                              //make sure config has valid IAP
   int i;
   for(i=num_opts; i--; ){
      if(opts[i].ctype==CFG_ITEM_ACCESS_POINT)
         break;
   }
   if(i!=-1){
                           //determine access points
#if defined _WINDOWS || defined _WIN32_WCE
      C_internet_connection::EnumAccessPoints(iaps, &app);
      if(App().config.wce_iap_name.Length()){
         int i;
         for(i=iaps.size(); i--; ){
            if(iaps[i].name==App().config.wce_iap_name)
               break;
         }
         if(i==-1){
            App().config.wce_iap_name = iaps.front().name;
            App().SaveConfig();
            App().CloseConnection();
         }
      }
#elif defined S60
      C_internet_connection::EnumAccessPoints(iaps, &app);
      if(App().config.iap_id!=-1){
         int i;
         for(i=iaps.size(); i--; ){
            if(iaps[i].id==(dword)App().config.iap_id)
               break;
         }
         if(i==-1){
            App().config.iap_id = -1;
            App().SaveConfig();
            App().CloseConnection();
         }
      }
#endif
   }
}

//----------------------------

bool C_client::C_mode_config_client::GetConfigOptionText(const S_config_item &ec, Cstr_w &str, bool &draw_arrow_l, bool &draw_arrow_r) const{

   const C_client::S_config &config = App().config;
   switch(ec.ctype){
#if defined MAIL || defined EXPLORER || defined MESSENGER
   case CFG_ITEM_TEXT_FONT_SIZE:
      str = app.GetText(!config.viewer_font_index ? TXT_CFG_SMALL : TXT_CFG_NORMAL);
      draw_arrow_l = (config.viewer_font_index>0);
      draw_arrow_r = (config.viewer_font_index<NUM_FONTS-1);
      break;
#endif
   case CFG_ITEM_FONT_SIZE:
      {
         int val = *(short*)(cfg_base + ec.elem_offset);
         if(val>0)
            str<<'+';
         str<<val;
         int min = 0, max =
            !config.use_system_font ? S_internal_font::NUM_INTERNAL_FONTS-2 :
            NUM_SYSTEM_FONTS;
         int fs = val + app.GetDefaultFontSize(
            config.use_system_font
            );
         const int MAX_DELTA =
            !config.use_system_font ? MAX_INTERNAL_FONT_SIZE_DELTA :
            MAX_SYSTEM_FONT_SIZE_DELTA;
         draw_arrow_l = (fs > min && val>-MAX_DELTA);
         draw_arrow_r = (fs < max && val<MAX_DELTA);
      }
      break;

#ifndef USE_ANDROID_TEXTS
   case CFG_ITEM_LANGUAGE:
      {
         const C_configuration_editing_client *cec = (C_configuration_editing_client*)configuration_editing;
         int i;
         for(i=cec->langs.size(); i--; ){
            if(cec->langs[i]==config.app_language)
               break;
         }
         const char *lname = (i==-1) ? "English" : (const char*)config.app_language;
         str.Copy(lname);
         draw_arrow_l = (i>0);
         draw_arrow_r = (i<cec->langs.size()-1);
      }
      break;
#endif

   case CFG_ITEM_ACCESS_POINT:
      {
         int i;
         for(i=iaps.size(); i--; ){
#if defined _WINDOWS || defined _WIN32_WCE
            const Cstr_w &iap_name = *(Cstr_w*)(cfg_base+ec.elem_offset);
            if(iaps[i].name==iap_name){
               draw_arrow_l = true;
#else
            dword iap_id = *(dword*)(cfg_base+ec.elem_offset);
            if(iaps[i].id==iap_id){
               draw_arrow_l = true;
#endif
               str = iaps[i].name;
               break;
            }
         }
         if(i==-1){
#ifndef USE_ALT_IAP
            str = app.GetText(TXT_ACCESS_POINT_DEFAULT);
#else
            str = app.GetText(!ec.param ? TXT_ACCESS_POINT_DEFAULT : TXT_CFG_DISABLED);
#endif
         }
         if(i!=iaps.size()-1)
            draw_arrow_r = true;
#if defined USE_ALT_IAP && defined __SYMBIAN32__
                              //no arrows for disabled secondary
         if(ec.param && config.iap_id==-1)
            draw_arrow_l = draw_arrow_r = false;
#endif
      }
      break;

   case CFG_ITEM_APP_PASSWORD:
      {
         const Cstr_w &s = *(Cstr_w*)(cfg_base+ec.elem_offset);
         str = s.Length() ? L"*******" : L"-";
      }
      break;

   default:
      return C_mode_configuration::GetConfigOptionText(ec, str, draw_arrow_l, draw_arrow_r);
   }
   return true;
}

//----------------------------

bool C_client::C_mode_config_client::ChangeConfigOption(const S_config_item &ec, dword key, dword key_bits){

   C_client::S_config &config = App().config;
   bool changed = false;
   switch(ec.ctype){
   case CFG_ITEM_TEXT_FONT_SIZE:
      if(key==K_CURSORLEFT){
         if(config.viewer_font_index){
            --config.viewer_font_index;
            changed = true;
         }
      }else
      if(key==K_CURSORRIGHT){
         if(config.viewer_font_index<NUM_FONTS-1){
            ++config.viewer_font_index;
            changed = true;
         }
      }
      break;

   case CFG_ITEM_FONT_SIZE:
      {
         short &val = *(short*)(cfg_base + ec.elem_offset);
         int min = 0, max =
            !config.use_system_font ? S_internal_font::NUM_INTERNAL_FONTS-2 :
            NUM_SYSTEM_FONTS;
         int dfs = app.GetDefaultFontSize(
            config.use_system_font
            );
         int fs = val+dfs;
         const int MAX_DELTA =
            !config.use_system_font ? MAX_INTERNAL_FONT_SIZE_DELTA :
            MAX_SYSTEM_FONT_SIZE_DELTA;
         if(key==K_CURSORLEFT){
            if(fs>min && val>-MAX_DELTA){
               --val;
               changed = true;
            }
         }else
         if(key==K_CURSORRIGHT){
            if(fs<max && val<MAX_DELTA){
               ++val;
               changed = true;
            }
         }
      }
      break;

#ifndef USE_ANDROID_TEXTS
   case CFG_ITEM_LANGUAGE:
      if(key==K_CURSORRIGHT || key==K_CURSORLEFT){
         const C_configuration_editing_client *cec = (C_configuration_editing_client*)configuration_editing;
         if(!cec->langs.size())
            break;
         int i;
         for(i=cec->langs.size(); i--; ){
            if(cec->langs[i]==config.app_language)
               break;
         }
         if(key==K_CURSORLEFT)
            i = Max(0, i-1);
         else
            i = Min(cec->langs.size()-1, i+1);
         config.app_language = cec->langs[i];
         changed = true;
      }
      break;
#endif //!USE_ANDROID_TEXTS

   case CFG_ITEM_COLOR_THEME:
      if(key==K_CURSORLEFT){
         if(config.color_theme){
            --config.color_theme;
            changed = true;
         }
      }else
      if(key==K_CURSORRIGHT){
         if(config.color_theme<ec.param-1){
            ++config.color_theme;
            changed = true;
         }
      }
      break;

   case CFG_ITEM_ACCESS_POINT:
      if(key==K_CURSORRIGHT || key==K_CURSORLEFT){
#if defined _WINDOWS || defined _WIN32_WCE
         Cstr_w &iap_name = *(Cstr_w*)(cfg_base+ec.elem_offset);
         int i;
         for(i=iaps.size(); i--; ){
            if(iaps[i].name==iap_name)
               break;
         }
         if(key==K_CURSORLEFT){
            if(i>0){
               --i;
               iap_name = iaps[i].name;
               changed = true;
            }else
            if(i==0){
               iap_name.Clear();
               changed = true;
            }
         }else{
            if(++i < iaps.size()){
               iap_name = iaps[i].name;
               changed = true;
            }
         }
#elif defined S60
#ifdef USE_ALT_IAP
                              //no change of secondary if primary is disabled
         if(ec.param!=0 && config.iap_id==-1)
            break;
#endif

         dword &iap_id = *(dword*)(cfg_base+ec.elem_offset);
         int i;
         for(i=iaps.size(); i--; ){
            if(iaps[i].id==dword(iap_id))
               break;
         }
         if(key==K_CURSORLEFT){
            if(i!=-1){
               if(--i==-1)
                  iap_id = i;
               else
                  iap_id = iaps[i].id;
               changed = true;
            }
         }else{
            if(++i < iaps.size()){
               iap_id = iaps[i].id;
               changed = true;
            }
         }
#ifdef USE_ALT_IAP
                              //if primary goes to <automatic>, disable secondary
         if(!ec.param && iap_id==-1)
            config.secondary_iap_id = -1;
#endif
#endif
      }
      break;

   default:
      changed = C_mode_configuration::ChangeConfigOption(ec, key, key_bits);
   }
   return changed;
}

//----------------------------
#ifndef USE_ANDROID_TEXTS

void C_client::C_configuration_editing_client::CollectLanguages(){

                              //determine languages
#if defined _DEBUG && !defined _WIN32_WCE
   Cstr_w fn = L"lang\\";
   fn<<app.data_path;
   C_dir d;
   if(d.ScanBegin(fn)){
      const wchar *fn;
      dword attributes;
      while((fn=d.ScanGet(&attributes), fn)){
         if(attributes&C_file::ATT_DIRECTORY)
            continue;
         Cstr_w ext = text_utils::GetExtension(fn);
         ext.ToLower();
         if(ext!=L"txt")
            continue;
         Cstr_c s;
         s.Copy(fn);
         s = s.Left(s.Length()-4);
         s.ToLower();
         s.At(0) = ToUpper(s[0]);
         langs.push_back(s);
      }
   }
#else
   const C_smart_ptr<C_zip_package> dta = app.CreateDtaFile();

   const wchar *cp;
   dword len;
   void *h = dta->GetFirstEntry(cp, len);
   if(h) do{
      Cstr_w n = cp;
      n.ToLower();
      if(n.Length()>5 && n.Left(5)==L"lang\\"){
         n = n.Right(n.Length()-5);
         if(n.Length()>=4 && n.Right(4)==L".txt"){
            n = n.Left(n.Length()-4);
            n.At(0) = ToUpper(n[0]);
            Cstr_c s; s.Copy(n);
            langs.push_back(s);
         }
      }
      h = dta->GetNextEntry(h, cp, len);
   }while(h);
#endif
#if defined S60 && !defined _DEBUG
   {
      Cstr_w fn; fn.Format(L"E:\\LCG Lang\\%") <<app.GetDataPath();
      C_dir d;
      if(d.ScanBegin(fn)){
         const wchar *fn;
         dword attributes;
         while((fn=d.ScanGet(&attributes), fn)){
            if(attributes&C_file::ATT_DIRECTORY)
               continue;
            Cstr_w ext = text_utils::GetExtension(fn);
            ext.ToLower();
            if(ext!=L"txt")
               continue;
            Cstr_c s;
            s.Copy(fn);
            s = s.Left(s.Length()-4);
            s.ToLower();
            s.At(0) = ToUpper(s[0]);
            int i;
            for(i=langs.size(); i--; ){
               if(langs[i]==s)
                  break;
            }
            if(i==-1)
               langs.push_back(s);
         }
      }
   }
#endif
   QuickSort(langs.begin(), langs.size(), sizeof(Cstr_c), CompareStrings);
}

#endif //!USE_ANDROID_TEXTS
//----------------------------

void C_client::C_configuration_editing_client::OnConfigItemChanged(const S_config_item &ec){

   modified = need_save = true;
   C_client::S_config &config = app.config;
   switch(ec.ctype){
   case CFG_ITEM_FONT_SIZE:
      app.internal_font.Close();
      app.InitAfterScreenResize();
      break;
#if defined USE_SYSTEM_SKIN || defined USE_OWN_SKIN
   case CFG_ITEM_COLOR_THEME:
      app.EnableSkinsByConfig();
      break;
#endif
   case CFG_ITEM_ACCESS_POINT:
      app.CloseConnection();
      break;
   case CFG_ITEM_CHECKBOX:
   case CFG_ITEM_CHECKBOX_INV:
      if(ec.elem_offset==OffsetOf(S_config, use_system_font)){
         config.ui_font_size_delta = 0;
         app.internal_font.Close();
         app.InitAfterScreenResize();
      }
      break;
#ifndef USE_ANDROID_TEXTS
   case CFG_ITEM_LANGUAGE:
      {
         bool was_internal = !config.use_system_font;
         app.LoadTexts(config.app_language, app.CreateDtaFile(), TXT_LAST);
         bool is_internal = app.CanUseInternalFont();
         if(was_internal != is_internal){
            config.use_system_font = false;
            if(!is_internal)
               config.use_system_font = true;
            config.ui_font_size_delta = 0;
            app.internal_font.Close();
            app.InitAfterScreenResize();
         }
      }
      break;
#endif
   }
}

//----------------------------

void C_client::C_configuration_editing_client::CommitChanges(){
   if(need_save){
      app.SaveConfig();
      need_save = false;
      LOG_RUN("Config saved");
   }
}

//----------------------------

void C_client::C_mode_config_client::DrawItem(const S_config_item &ec, const S_rect &rc_item, dword color) const{

#if defined MAIL || defined EXPLORER
   switch(ec.ctype){
   case CFG_ITEM_COLOR_THEME:
      {
         int m = rc_item.CenterX();
         int l = m - app.fdb.letter_size_x*6;
         int r = m + m - l;
         int yy = rc_item.CenterY();
         S_rect rc_l(l, yy, m-l, app.fdb.cell_size_y);
         S_rect rc_r(m+1, yy, r-m, app.fdb.cell_size_y);
         S_rect rc_a(r+2, yy, app.fdb.letter_size_x*3, app.fdb.cell_size_y);
         dword color1 = app.GetColor(COL_TEXT);
         app.DrawOutline(rc_l, color1);
         app.DrawOutline(rc_r, color1);
         app.DrawOutline(rc_a, color1);
         app.ClearToBackground(rc_l);
         app.DrawSelection(rc_r);
         app.FillRect(rc_a, app.GetColor(COL_SCROLLBAR));

         DrawArrows(rc_item, color1, (App().config.color_theme!=0), (App().config.color_theme!=ec.param-1));
      }
      return;
   }
#endif
   C_mode_configuration::DrawItem(ec, rc_item, color);
}

//----------------------------

void C_client::MapScrollingKeys(S_user_input &ui){

   if(ui.key_bits&GKEY_CTRL)
   switch(ui.key){
   case K_CURSORUP: ui.key = '*'; break;
   case K_CURSORDOWN: ui.key = '#'; break;
   }
}

//----------------------------

void C_client::ShowErrorWindow(E_TEXT_ID title, const wchar *err){
   CreateInfoMessage(*this, title, err, INFO_MESSAGE_IMG_EXCLAMATION);
}

//----------------------------

C_internet_connection *C_client::CreateConnection(){

   if(!connection){
      connection = ::CreateConnection(this,
#if defined __SYMBIAN32__
         config.iap_id,
#else
         config.wce_iap_name,
#endif
         config.connection_time_out*1000
#ifdef USE_ALT_IAP
         , config.secondary_iap_id
#endif
         );
      connection->Release();
   }
   return connection;
}

//----------------------------

void C_client::CloseConnection(){

   StoreDataCounters();
   super::CloseConnection();
}

//----------------------------

void C_client::StoreDataCounters(){

   if(connection){
      dword s = connection->GetDataSent(),
         r = connection->GetDataReceived();
      config.total_data_sent += s;
      config.total_data_received += r;
      if(s || r)
         SaveConfig();
      connection->ResetDataCounters();
   }
}

//----------------------------

void C_client::Exit(){
//   LOG_RUN("app.Exit");
   mode = NULL;
   CloseConnection();
   super::Exit();
}

//----------------------------

C_client::C_client(S_config &con):
   config(con),
   socket_log(SOCKET_LOG_NO),
   show_license(true)
{
}

//----------------------------

C_client::~C_client(){
   mode = NULL;
}

//----------------------------

const wchar *C_client::GetText(dword id) const{

   if(!id)
      return NULL;
   if(id>=0x8000)
   switch(id){
   case SPECIAL_TEXT_OK: id = TXT_OK; break;
   case SPECIAL_TEXT_SELECT: id = TXT_SELECT; break;
   case SPECIAL_TEXT_CANCEL: id = TXT_CANCEL; break;
   case SPECIAL_TEXT_SPELL: id = TXT_EDIT; break;
   case SPECIAL_TEXT_PREVIOUS: id = TXT_PREVIOUS; break;
   case SPECIAL_TEXT_CUT: id = _TXT_CUT; break;
   case SPECIAL_TEXT_COPY: id = _TXT_COPY; break;
   case SPECIAL_TEXT_PASTE: id = _TXT_PASTE; break;
   case SPECIAL_TEXT_YES: id = TXT_YES; break;
   case SPECIAL_TEXT_NO: id = TXT_NO; break;
   case SPECIAL_TEXT_BACK: id = TXT_BACK; break;
   case SPECIAL_TEXT_MENU: id = TXT_MENU; break;
   default: assert(0);
   }
#ifdef USE_ANDROID_TEXTS
   if(id>=TXT_LAST)
      return NULL;
   Cstr_w &cache = texts_cache[id];
   if(cache.Length())
      return cache;
   JNIEnv &env = jni::GetEnv();
   static jmethodID mid = env.GetMethodID(android_globals.java_app_class, "GetText", "(I)Ljava/lang/String;");
   jstring js = (jstring)env.CallObjectMethod(android_globals.java_app, mid, id);
   if(!env.ExceptionCheck()){
      cache = jni::GetString(js, true);
   }else{
      env.ExceptionClear();
      cache.Format(L"<%>") <<id;
   }
   return cache;
#else
   return C_application_ui::GetText(id);
#endif
}

//----------------------------

bool C_client::ProcessKeyCode(dword code){

   switch(code){
   case 123:
   case 124:
      socket_log = socket_log ? SOCKET_LOG_NO : (code==123 ? SOCKET_LOG_YES : SOCKET_LOG_YES_NOSHOW);
      return true;
   case 800:
      config.use_system_font = !config.use_system_font;
      config.ui_font_size_delta = 0;
      SaveConfig();
      internal_font.Close();
      InitAfterScreenResize();
      RedrawScreen();
      break;
#ifdef __SYMBIAN32___
   case 843:
      {
         Cstr_w s;
         text_utils::MakeHexString(
         s<<L"Device UID: 0x";
         C_client_info_message::CreateMode(this, TXT_OK, s, C_client_info_message::IMG_SUCCESS);
      }
      break;
#endif

   case 995:                  //intentional crash
      *(byte*)NULL = 1;
      break;
   }
   return false;
}

//----------------------------
#if defined USE_SYSTEM_SKIN || defined USE_OWN_SKIN

void C_client::EnableSkinsByConfig(){
   EnableSkins(config.color_theme==0);
}

#endif
//----------------------------

bool C_client::BaseConstruct(const C_zip_package *dta, dword _ig_flags){

#ifdef DEBUG_LOG
   { Cstr_c s; s.Format("% %.% (%)") <<Cstr_w(app_name).ToUtf8() <<VERSION_HI <<VERSION_LO <<__DATE__; LOG_RUN(s); }
#endif
   bool cfg_loaded = LoadConfig();
   
   if(!config.IsFullscreenMode())
      _ig_flags |= IG_SCREEN_USE_CLIENT_RECT;

#if defined USE_SYSTEM_SKIN || defined USE_OWN_SKIN
   EnableSkinsByConfig();
#endif
#ifdef UI_MENU_ON_RIGHT
   menu_on_right = false;
#endif

   if(!cfg_loaded)
      SaveConfig();
#ifndef USE_ANDROID_TEXTS
   LoadTexts(config.app_language, dta, TXT_LAST);
#endif
   InitGraphics(_ig_flags);

#ifdef DEBUG_NETWORK_LOG
   socket_log = SOCKET_LOG_YES_NOSHOW;
#endif
   return cfg_loaded;
}

//----------------------------

void C_client::ModifySettingsTextEditor(C_mode_settings &mod, const S_config_item &ec, byte *cfg_base){

   C_text_editor &te = *mod.text_editor;
   switch(ec.ctype){
   case CFG_ITEM_NUMBER:
   case CFG_ITEM_SIGNED_NUMBER:
      {
         int num;
         Cstr_w str = te.GetText();
         Cstr_c sc; sc.Copy(str);
         const char *cp = sc;
         word &n = *(word*)(cfg_base + ec.elem_offset);
         if(!*cp)
            n = 0;
         else{
            if(ec.ctype==CFG_ITEM_NUMBER){
               if(text_utils::ScanDecimalNumber(cp, num) && num<65536)
                  n = word(num);
            }else{
               if(text_utils::ScanDecimalNumber(cp, num) && num>=-32768 && num<32768)
                  n = short(num);
            }
         }
      }
      break;

   default:
      switch(ec.is_wide){
      case 0:
      case 2:
         {
            Cstr_c &str = *(Cstr_c*)(cfg_base + ec.elem_offset);
            if(ec.is_wide)
               str.ToUtf8(te.GetText());
            else
               str.Copy(te.GetText());
         }
         break;
      case 1:
         {
            Cstr_w &str = *(Cstr_w*)(cfg_base + ec.elem_offset);
            str = te.GetText();
         }
         break;
      }
   }
}

//----------------------------

void C_client::CopyTextToClipboard(const wchar *txt){

   C_text_editor::ClipboardCopy(txt);
}

//----------------------------
static const wchar license_filename[] = L"License.txt";

bool C_client::DisplayLicenseAgreement(){

   if(!show_license)
      return false;
   show_license = false;
   C_file fl;
   Cstr_w lfn;
   lfn<<data_path <<license_filename;
   if(fl.Open(lfn)){
      dword sz = fl.GetFileSize();
      C_vector<char> buf;
      buf.resize(sz);
      fl.Read(buf.begin(), sz);

      class C_question: public C_question_callback{
         C_client &app;
         virtual void QuestionConfirm(){
            Cstr_w lfn; lfn<<app.data_path <<license_filename;
            C_file::DeleteFile(lfn);
            app.FinishConstruct();
         }
         virtual void QuestionReject(){
            app.Exit();
         }
      public:
         C_question(C_client &_a): app(_a){}
      };
      CreateFormattedMessage(*this, L"License agreement", buf, new(true) C_question(*this), true, TXT_OK, TXT_CANCEL);
      UpdateScreen();
      return true;
   }
   return false;
}

//----------------------------

bool C_client::CopyFile(const wchar *src, const wchar *dst){
   C_file fl;
   if(fl.Open(src)){
      C_buffer<byte> buf;
      buf.Resize(fl.GetFileSize());
      fl.Read(buf.Begin(), buf.Size());
      if(fl.Open(dst, C_file::FILE_WRITE | C_file::FILE_WRITE_CREATE_PATH)){
         fl.Write(buf.Begin(), buf.Size());
         if(fl.WriteFlush()==fl.WRITE_OK)
            return true;
         fl.Close();
         C_file::DeleteFile(dst);
      }
   }
   return false;
}

//----------------------------

bool C_archive_simulator::GetFileWriteTime(const wchar *filename, S_date_time &td) const{
   filename = GetRealFileName(filename);
   if(!filename)
      return false;
   return C_file::GetFileWriteTime(filename, td);
}

//----------------------------

bool encoding::IsMultiByteCoding(E_TEXT_CODING coding){

   switch(coding){
   case COD_UTF_8:
   case COD_BIG5:
   case COD_GB2312:
   case COD_GBK:
   case COD_SHIFT_JIS:
   case COD_JIS:
   case COD_EUC_KR:
   case COD_HEBREW:
      return true;
   }
   return false;
}

//----------------------------

void C_client::ConvertMultiByteStringToUnicode(const char *src, E_TEXT_CODING coding, Cstr_w &dst) const{

   switch(coding){
   case COD_UTF_8:
      dst.FromUtf8(src);
      break;
   default:
      ConvertStringToUnicode(src, coding, dst);
   }
}

//----------------------------

void C_client::ConvertStringToUnicode(const char *src, E_TEXT_CODING coding, Cstr_w &dst) const{

   if(encoding::IsMultiByteCoding(coding))
      ConvertMultiByteStringToUnicode(src, coding, dst);
   else{
      dword len = StrLen(src);
      dst.Allocate(NULL, len);
      for(dword i=0; i<len; i++)
         dst.At(i) = (wchar)encoding::ConvertCodedCharToUnicode(byte(src[i]), coding);
   }
}

//----------------------------

Cstr_w C_client::GetErrorName(const Cstr_w &text) const{

   Cstr_w str = GetText(
#ifdef MUSICPLAYER
      TXT_ERR_CONNECTION_FAILED
#else
      TXT_ERR_CONNECT_FAIL
#endif
      );
   if(text.Length())
      str.AppendFormat(L"\n(%)") <<text;
   return str;
}

//----------------------------

#include <ImgLib.h>

const t_CreateLoader image_loaders[] = {
#ifndef IMAGE_NO_JPG
   &CreateImgLoaderJPG,
#endif
#ifndef IMAGE_NO_GIF
   &CreateImgLoaderGIF,
#endif
#ifndef IMAGE_NO_PNG
   &CreateImgLoaderPNG,
#endif
   &CreateImgLoaderPCX,
#ifndef IMAGE_NO_BMP
   &CreateImgLoaderBMP,
#endif
#ifndef IMAGE_NO_ICO
   &CreateImgLoaderICO,
#endif
   NULL
};

//----------------------------

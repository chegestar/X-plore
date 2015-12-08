#include "Main.h"
#ifdef EXPLORER
#include "Explorer\Main_Explorer.h"
#elif defined MAIL
#include "Email\Main_Email.h"
#endif
#if defined MAIL || defined EXPLORER
#include "AudioPreview.h"
#endif
#ifdef EXPLORER
#include "Explorer\VideoPreview.h"
#endif

#include "Viewer.h"
#include <Xml.h>

//----------------------------

void C_client::ViewerShowLink(C_text_viewer &viewer) const{

   const S_text_display_info &tdi = viewer.text_info;
   const S_hyperlink *href = tdi.GetActiveHyperlink();
   if(!href){
      viewer.link_show_count = 0;
      return;
   }
   Cstr_c tmp;
   const char *link = href->link;
   if(*link){
      text_utils::CheckStringBegin(link, "mailto:") ||
         text_utils::CheckStringBegin(link, "tel:") ||
         text_utils::CheckStringBegin(link, text_utils::HTTP_PREFIX) ||
         text_utils::CheckStringBegin(link, text_utils::HTTPS_PREFIX);
   }else
   if(href->image_index!=-1){
      tmp.Format("Image %") <<(href->image_index+1);
      link = tmp;
   }
   text_utils::SkipWS(link);
   viewer.link_show = link;
   viewer.link_show_count = 2000;
}

//----------------------------

void C_client::C_multi_item_loader::BeginNextEntry(){

   if(entries.size()){
      entries.erase(entries.begin());
      ++prog_all.pos;
   }
   phase = NOT_CONNECTED;
   if(!entries.size()){
      socket = NULL;
      phase = CLOSED;
   }
}

//----------------------------

void C_client::C_multi_item_loader::Suspend(){
   socket = NULL;
   phase = CLOSED;
}

//----------------------------

void C_client::C_text_viewer::SuspendLoader(){

   if(img_loader.phase==img_loader.CLOSED)
      return;
   if(img_loader.phase!=img_loader.NOT_CONNECTED){
      img_loader.data_saving.CancelOutstanding();
      img_loader.phase = img_loader.NOT_CONNECTED;
      img_loader.socket = NULL;
      img_loader.prog_curr.pos = 0;
      img_loader.prog_curr.total = 1;
   }
}

//----------------------------

bool C_client::ViewerProcessMouse(C_text_viewer &viewer, const S_user_input &ui, int &scroll_pixels, bool &redraw){

   if(!HasMouse())
      return false;
   if(viewer.mouse_drag){
      if(ui.mouse_buttons&MOUSE_BUTTON_1_UP){
         viewer.mouse_drag = false;
      }else
      if(ui.mouse_buttons&MOUSE_BUTTON_1_DRAG){
         scroll_pixels = viewer.drag_mouse_prev.y - ui.mouse.y;
         viewer.drag_mouse_prev = ui.mouse;
         int dy = viewer.drag_mouse_begin.y - ui.mouse.y;
         if(Abs(dy) >= fds.line_spacing){
            viewer.link_show_count = 0;
            viewer.ResetTouchInput();
         }
      }
   }
   if((ui.mouse_buttons&(MOUSE_BUTTON_1_DOWN|MOUSE_BUTTON_1_UP)) && ui.CheckMouseInRect(viewer.rc)){
      S_text_display_info &tdi = viewer.text_info;
      if(tdi.hyperlinks.size() && !tdi.is_wide){
                              //detect if hyperling was clicked
         int curr_link = -1, link_offs = 0;

         const char *cp = tdi.body_c + tdi.byte_offs;
         int y = ui.mouse.y - tdi.rc.y + tdi.pixel_offs;
         S_text_style ts = tdi.ts;
         while(true){
            int h;
            S_text_style ts1 = ts;
            int len = ComputeFittingTextLength(cp, false, tdi.rc.sx, &tdi, ts1, h, config.viewer_font_index);
            if(!len)
               break;
            if(y < h){
                              //this is our line, determine hyperlink on it
               int x = ui.mouse.x - tdi.rc.x;
               if(ts.href){
                              //line begins on hyperlink, determine which it is
                  S_text_style ts2 = ts;
                  const char *cp1 = cp;
                  while(cp1!=tdi.body_c){
                     --cp1;
                     if(S_text_style::IsControlCode(*cp1)){
                        if(*cp1==CC_HYPERLINK){
                           cp1 -= 3;
                           curr_link = byte(cp1[1]) | (byte(cp1[2]) << 8);
                           assert(curr_link < tdi.hyperlinks.size());
                           link_offs = cp1 - tdi.body_c;
                           break;
                        }
                        ++cp1;
                        ts2.ReadCodeBack(cp1);
                     }
                  }
               }
               const char *cp_beg = cp, *cp_end = cp + len;
               while(cp_beg<cp_end && S_text_style::IsStyleCode(*cp_beg))
                  ts.ReadCode(cp_beg);

               if(ts.font_flags&(FF_CENTER|FF_RIGHT)){
                  int w = GetTextWidth(cp, false, tdi.body_c.coding, ts.font_index, ts.font_flags, len, &tdi);
                  int dx = tdi.rc.sx - w;
                  if(ts.font_flags&FF_CENTER)
                     dx /= 2;
                  x -= dx;
               }
               if(x<=0){
                  curr_link = -1;
               }else
               while(cp < cp_end){
                  dword c = (byte)*cp;
                  int height = 0;
                  if(S_text_style::IsControlCode(c)){
                     switch(c){
                     case CC_HYPERLINK:
                        curr_link = byte(cp[1]) | (byte(cp[2]) << 8);
                        assert(curr_link < tdi.hyperlinks.size());
                        link_offs = cp - tdi.body_c;
                        cp += 4;
                        break;
                     case CC_HYPERLINK_END:
                        curr_link = -1;
                        cp += 4;
                        break;
                     case CC_IMAGE:
                        {
                           ++cp;
                           dword ii = ts.Read16bitIndex(cp);
                           const S_image_record &ir = tdi.images[ii];
                           x -= ir.sx;
                           height = ir.sy;
                        }
                        break;
#ifdef WEB_FORMS
                     case CC_FORM_INPUT:
                        assert(0);
                        break;
#endif
                     case CC_TEXT_FONT_FLAGS:
                        {
                           dword prev_ff = ts.font_flags;
                           ts.ReadCode(cp);
                           if(((prev_ff^ts.font_flags)&(FF_CENTER|FF_RIGHT)) && ts.font_flags&(FF_CENTER|FF_RIGHT)){
                              int w = GetTextWidth(cp_beg, false, tdi.body_c.coding, ts.font_index, ts.font_flags, len, &tdi);
                              int dx = tdi.rc.sx - w;
                              if(ts.font_flags&FF_CENTER)
                                 dx /= 2;
                              x -= dx;
                           }
                        }
                        break;
                     case CC_WIDE_CHAR:
                        ++cp;
                        c = S_text_style::ReadWideChar(cp);
                        break;
                     default:
                        if(S_text_style::IsStyleCode(c))
                           ts.ReadCode(cp);
                        else
                           ts.SkipCode(cp);
                     }
                  }else
                     ++cp;
                  if(c>=' '){
#ifdef USE_SYSTEM_FONT
#else
                     S_font::E_PUNKTATION punkt;
                     c = encoding::ConvertCodedCharToUnicode(c, tdi.body_c.coding);
                     dword map = S_internal_font::GetCharMapping(c, punkt);

                     int fi = ts.font_index;
                     if(fi==-1)
                        fi = config.viewer_font_index;
                     const S_font &fd = font_defs[fi];

                     int letter_width;
                     if(system_font.use){
                        letter_width = system_font.GetCharWidth(wchar(map), (ts.font_flags&FF_BOLD), (ts.font_flags&FF_ITALIC), fi==1);
                     }else
                     {
                        if(map==' ')
                           letter_width = fd.cell_size_x/2;
                        else{
                           letter_width = internal_font.font_data[fi].width_empty_bits[map-' '];
                           if(letter_width&0x80)
                              letter_width = (letter_width&0x7f) - 1;
                        }
                        if(ts.font_flags&FF_BOLD)
                           ++letter_width;
                        }
                     x -= letter_width + 1 + fd.extra_space;
                     height = fd.line_spacing;
#endif
                  }
                  if(x<=0){
                              //we're at correct x position
                              // check if click is not too high
                     if((h-y) >= height)
                        curr_link = -1;
                     break;
                  }
               }
               break;
            }
            ts = ts1;
            y -= h;
            cp += len;
         }
         if(curr_link==tdi.active_link_index){
            if(ui.mouse_buttons&MOUSE_BUTTON_1_UP)
            if(curr_link!=-1){
                              //open link
               return true;
            }
         }else if(ui.mouse_buttons&MOUSE_BUTTON_1_DOWN){
                              //disable prev link and activate new
            tdi.ts.font_flags &= ~FF_ACTIVE_HYPERLINK;

            tdi.active_link_index = curr_link;
            if(curr_link!=-1){
               tdi.active_object_offs = link_offs;
               if(tdi.active_object_offs < int(tdi.byte_offs))
                  tdi.ts.font_flags |= FF_ACTIVE_HYPERLINK;
               //viewer.rc_link_show.y = ui.mouse.y - 20;
               ViewerShowLink(viewer);
            }else{
               tdi.active_object_offs = -1;
               viewer.link_show_count = 0;
            }
            redraw = true;
         }
      }
      if(ui.mouse_buttons&MOUSE_BUTTON_1_DOWN){
                              //start scrolling text by mouse drag
         viewer.mouse_drag = true;
         viewer.drag_mouse_begin = ui.mouse;
         viewer.drag_mouse_prev = ui.mouse;
      }
   }
   return false;
}

//----------------------------

void C_client::ViewerProcessKeys(C_text_viewer &viewer, const S_user_input &ui, int &scroll_pixels, bool &redraw){

   S_text_display_info &tdi = viewer.text_info;

   switch(ui.key){
   case K_CURSORDOWN:
   case K_CURSORUP:
      if(ui.key_bits&GKEY_SHIFT){
         viewer.auto_scroll_time = (ui.key==K_CURSORUP) ? -256 : 256;
      }else
      if(ui.key_bits&GKEY_CTRL){
         viewer.auto_scroll_time = (viewer.sb.visible_space-font_defs[config.viewer_font_index].line_spacing) << 8;
         if(ui.key==K_CURSORUP)
            viewer.auto_scroll_time = -viewer.auto_scroll_time;
      }else{
         scroll_pixels = font_defs[config.viewer_font_index].line_spacing;
         if(tdi.hyperlinks.size()
#ifdef WEB_FORMS
            || tdi.form_inputs.size()
#endif
            ){
                              //try navigate on hyperlinks
            if(ui.key==K_CURSORDOWN){
               if(GoToNextActiveObject(tdi)){
                              //check if link is visible
                  int y = GetLineYAtOffset(tdi, tdi.active_object_offs) + font_defs[UI_FONT_BIG].line_spacing;
                  int bot = tdi.top_pixel + tdi.rc.sy;
                  if(y < bot || bot >= tdi.total_height){
                              //move to the link
#ifdef WEB
                     if(config.flags&S_config::CONF_AUTO_DISPLAY_HIPERLINKS)
#endif
                        ViewerShowLink(viewer);
                     scroll_pixels = 0;
                     redraw = true;
                  }else{
                                 //link invisible, do normal text scroll
                     if(!GoToPrevActiveObject(tdi)){
#ifdef WEB
                        if(config.flags&S_config::CONF_AUTO_DISPLAY_HIPERLINKS)
#endif
                           ViewerShowLink(viewer);
                     }
                  }
               }
            }else{
               if(GoToPrevActiveObject(tdi)){
                  int y = GetLineYAtOffset(tdi, tdi.active_object_offs);
                  if(y >= tdi.top_pixel){
                                 //move to the link
#ifdef WEB
                     if(config.flags&S_config::CONF_AUTO_DISPLAY_HIPERLINKS)
#endif
                        ViewerShowLink(viewer);
                     scroll_pixels = 0;
                     redraw = true;
                  }else{
                                 //link invisible, do normal text scroll
                     GoToNextActiveObject(tdi);
                  }
               }
            }
         }
         if(ui.key==K_CURSORUP)
            scroll_pixels = -scroll_pixels;
         viewer.auto_scroll_time = 0;
      }
      break;
   }
}

//----------------------------

void C_client::ViewerGoToVisibleHyperlink(C_text_viewer &viewer, bool up){

   S_text_display_info &tdi = viewer.text_info;
   if(tdi.hyperlinks.size()){
      bool show = false;
                              //make sure hyperlink is visible
      if(!up){
         do{
            int y = GetLineYAtOffset(tdi, tdi.active_object_offs);
            if(y >= tdi.top_pixel)
               break;
            show = true;
         } while(GoToNextActiveObject(tdi));
      }else{
         do{
            int y = GetLineYAtOffset(tdi, tdi.active_object_offs) + font_defs[UI_FONT_BIG].line_spacing;
            if(y < tdi.top_pixel+tdi.rc.sy)
               break;
            show = true;
         } while(GoToPrevActiveObject(tdi));
      }
      if(show)
         ViewerShowLink(viewer);
   }
}

//----------------------------

void C_client::ViewerTickCommon(C_text_viewer &viewer, int time, bool &redraw){

   if(viewer.auto_scroll_time){
      S_text_display_info &tdi = viewer.text_info;
      int scroll_pixels = viewer.auto_scroll_time >> 8;
      int add = time * 200;
      if(viewer.auto_scroll_time > 0){
         if((viewer.auto_scroll_time -= add) <= 0)
            viewer.auto_scroll_time = 0;
      }else{
         if((viewer.auto_scroll_time += add) >= 0)
            viewer.auto_scroll_time = 0;
      }
      scroll_pixels -= viewer.auto_scroll_time >> 8;
      if(ScrollText(tdi, scroll_pixels)){
         viewer.sb.pos = tdi.top_pixel;
         redraw = true;
         ViewerGoToVisibleHyperlink(viewer, (scroll_pixels<0));
      }
   }
   if(viewer.link_show_count && (viewer.link_show_count-=time) <= 0){
      viewer.link_show_count = 0;
      redraw = true;
   }
}

//----------------------------

C_client_viewer::C_mode_this::~C_mode_this(){

#ifdef MAIL
                              //delete all cached files
   for(int i=html_images.Size(); i--; )
      C_file::DeleteFile(html_images[i].filename.FromUtf8());
#endif
}

//----------------------------

C_client_viewer::C_mode_this *C_client_viewer::SetMode(){

   C_mode_this &mod = *new(true) C_mode_this(*this);
#ifdef EXPLORER
   mod.fullscreen = (config.flags&C_explorer_client::S_config_xplore::CONF_VIEWER_FULLSCREEN);
#endif
   mod.InitLayout();
   ActivateMode(mod, false);
   return &mod;
}

//----------------------------

void C_client_viewer::C_mode_this::InitLayout(){

   const int border = fullscreen ? 0 : 1;

   C_text_viewer::title_height = fullscreen ? 0 : app.fdb.line_spacing;
   rc = S_rect(border, C_text_viewer::title_height, app.ScrnSX()-border*2, app.ScrnSY()-C_text_viewer::title_height);
   if(!fullscreen)
      rc.sy -= app.GetSoftButtonBarHeight()+1;
   {
      int sb_size = app.GetScrollbarWidth();
      if(fullscreen)
         sb_size = Max(3, sb_size/2);
      text_info.rc = rc;
      text_info.rc.x += app.fdb.letter_size_x/2;
      text_info.rc.sx -= sb_size + app.fdb.letter_size_x;
      text_info.ts.font_index = app.config.viewer_font_index;
                              //init scrollbar
      sb.visible_space = rc.sy;
      sb.rc = S_rect(rc.Right()-sb_size-2, rc.y+3, sb_size, rc.sy-6);

      rc_link_show = S_rect(rc.x+app.fds.letter_size_x, Max(0, rc.y-app.fds.cell_size_y-4), rc.sx-app.fds.letter_size_x*2, app.fds.cell_size_y);
      sb.SetVisibleFlag();
   }
   {
      app.CountTextLinesAndHeight(text_info, app.config.viewer_font_index);
      sb.total_space = text_info.total_height;
      sb.SetVisibleFlag();
      if(!sb.visible){
         text_info.byte_offs = 0;
         text_info.pixel_offs = 0;
         text_info.top_pixel = 0;
      }
   }
}

//----------------------------

bool C_client_viewer::OpenFileForViewing1(const wchar *fname, const wchar *view_name, const C_zip_package *arch, const char *www_domain, C_mode_this *parent_viewer,
   C_viewer_previous_next *vpn, dword file_offset, E_TEXT_CODING coding, bool allow_delete){

   Cstr_w ext;
   ext = text_utils::GetExtension(fname);
   ext.ToLower();
   E_FILE_TYPE ft = DetermineFileType(ext);
#ifdef MUSICPLAYER
                              //support embedded images
   if(ft==FILE_TYPE_AUDIO && file_offset)
      ft = FILE_TYPE_IMAGE;
#endif

   C_mode_this *mod_v = NULL;
   switch(ft){
   case FILE_TYPE_IMAGE:
      if(CreateImageViewer(*this, vpn, view_name, fname, arch, file_offset, allow_delete))
         return true;
      break;
#if defined MAIL || defined EXPLORER
   case FILE_TYPE_WORD:
      {
         mod_v = C_client_viewer::CreateMode(this);
         if(OpenWordDoc(fname, mod_v->text_info, arch)){
            mod_v->text_info.ts.font_index = config.viewer_font_index;
            CountTextLinesAndHeight(mod_v->text_info, config.viewer_font_index);
            mod_v->text_info.bgnd_color = GetColor(COL_WHITE) & 0xffffff;
            mod_v->sb.total_space = mod_v->text_info.total_height;
            mod_v->sb.SetVisibleFlag();
         }else{
            CloseMode(*mod_v);
            mod_v = NULL;
         }
      }
      break;

#ifdef MAIL
   case FILE_TYPE_AUDIO:
      if(!arch && SetModeAudioPreview(*this, fname, reinterpret_cast<C_mail_client*>(this)->config_mail.audio_volume, view_name))
         return true;
      break;

   case FILE_TYPE_EMAIL_MESSAGE:
      if(reinterpret_cast<C_mail_client*>(this)->SetModeReadMail(fname))
         return true;
      break;
#endif

#ifdef EXPLORER
   case FILE_TYPE_AUDIO:
      {
#ifdef EXPLORER
         if(arch && arch->GetArchiveType()==1000){
            C_archive_simulator *arch_sim = (C_archive_simulator*)arch;
            fname = arch_sim->GetRealFileName(fname);
            arch = NULL;
         }
#endif
         if(!arch && SetModeAudioPreview(*this, fname, reinterpret_cast<C_explorer_client*>(this)->config_xplore.audio_volume))
            return true;
      }
      break;
   case FILE_TYPE_VIDEO:
      {
         if(arch && arch->GetArchiveType()==1000){
            C_archive_simulator *arch_sim = (C_archive_simulator*)arch;
            fname = arch_sim->GetRealFileName(fname);
            arch = NULL;
         }
         if(!arch){
            if(C_client_video_preview::CreateMode(this, fname, reinterpret_cast<C_explorer_client*>(this)->config_xplore.audio_volume))
               return true;
         }
      }
      break;
#endif

   case FILE_TYPE_ARCHIVE:
      return C_client_file_mgr::SetModeFileBrowser_Archive(this, fname, view_name, arch);

   case FILE_TYPE_CALENDAR_ENTRY:
   case FILE_TYPE_CONTACT_CARD:
   case FILE_TYPE_EXE:
   case FILE_TYPE_LINK:
      return OpenFileBySystem(fname);

   case FILE_TYPE_URL:
      {                       //try to parse url file
         C_file *fl = CreateFile(fname, arch);
         if(fl){
            bool opened = false;
            while(!fl->IsEof()){
               Cstr_c l = fl->GetLine();
               l.ToLower();
               const char *cp = l;
               if(text_utils::CheckStringBegin(cp, "url=")){
                  text_utils::SkipWS(cp);
                  system::OpenWebBrowser(*this, cp);
                  opened = true;
                  break;
               }
            }
            delete fl;
            if(opened)
               return true;
         }
      }
      break;

   default:
      {
                        //check if file may be opened in text mode
         C_file *ck = CreateFile(fname, arch);
         if(!ck)
            break;
         int size = ck->GetFileSize();
         bool wide = false;
         if(size>=2){
            word c;
            ck->ReadWord(c);
            if(c==0xfeff){
               wide = true;
               size = size/2 - 1;
            }else
               ck->Seek(0);
         }
         int i;
         for(i=size; i--; ){
            word c;
            if(wide)
               ck->ReadWord(c);
            else{
               c = 0;
               ck->ReadByte((byte&)c);
            }
            if(c<' '){
               if(c!='\n' && c!='\r' && c!='\t')
                  break;
            }
         }
         delete ck;
         if(i!=-1){
#ifdef EXPLORER
                              //check if it may be zip file
            if(C_client_file_mgr::SetModeFileBrowser_Archive(this, fname, view_name, arch))
               return true;
#endif
            break;
         }
                              //all characters are possibly printable, looks like text file
      }
                              //flow...
#endif

#if defined MAIL || defined EXPLORER || defined MESSENGER //|| defined MUSICPLAYER
   case FILE_TYPE_TEXT:
      mod_v = C_client_viewer::CreateMode(this);
#ifdef MAIL
      mod_v->img_loader.domain = www_domain;
#endif
      S_text_display_info &tdi = mod_v->text_info;
      tdi.justify = true;
      S_file_open_help oh;
      oh.arch = arch;
      oh.www_domain = www_domain;
      tdi.body_c.coding = coding;
      if(OpenTextFile(fname, -1, tdi, oh)){
         tdi.ts.font_index = config.viewer_font_index;
         if(tdi.html_title.Length())
            mod_v->title = tdi.html_title;
#ifdef MAIL
         {
                              //if opening html from another html viewer, adopt the previous viewer's images
            C_mail_client *cl_mail = reinterpret_cast<C_mail_client*>(this);
            if(cl_mail->config_mail.flags&C_mail_client::S_config_mail::CONF_DOWNLOAD_HTML_IMAGES)
               cl_mail->OpenHtmlImages1(*mod_v, parent_viewer);
         }
#endif
         if(tdi.HasDefaultBgndColor())
            tdi.bgnd_color = GetColor(COL_WHITE) & 0xffffff;

         mod_v->InitLayout();
      }else{
         CloseMode(*mod_v);
         mod_v = NULL;
      }
      break;
#endif//MAIL || EXPLORER
   }
   if(!mod_v)
      return false;

   if(!mod_v->filename.Length())
      mod_v->filename = fname;
   mod_v->arch.SetPtr(arch);

   if(!mod_v->title.Length())
      mod_v->title = view_name;
#ifdef EXPLORER
   {
      dword offs = reinterpret_cast<C_explorer_client*>(this)->viewer_pos.GetTextPos(fname, arch);
      if(offs){
                              //scroll to saved pos
         dword lpix = font_defs[config.viewer_font_index].line_spacing;
         S_text_display_info &tdi = mod_v->text_info;
         if(tdi.is_wide)
            offs *= 2;
         while(true){
            if(tdi.byte_offs >= offs)
               break;
            if(!ScrollText(tdi, lpix))
               break;
         }
         tdi.pixel_offs = 0;
         mod_v->sb.pos = tdi.top_pixel;
      }
   }
#endif
   RedrawScreen();
   return true;
}

//----------------------------

void C_client_viewer::Close(C_mode_this &mod){

#ifdef EXPLORER
   {
                              //save scroll position
      int offs = mod.text_info.byte_offs;
      if(mod.text_info.is_wide)
         offs /= 2;
      reinterpret_cast<C_explorer_client*>(this)->viewer_pos.SaveTextPos(offs, mod.filename, mod.arch);
   }
#endif
   CloseMode(mod);
}

//----------------------------

void C_client_viewer::C_mode_this::ProcessMenu(int itm, dword mid){

   switch(itm){
#ifdef MAIL
   case TXT_OPEN_LINK:
      reinterpret_cast<C_mail_client*>(&app)->Viewer_OpenLink(*this, (GetParent()->Id()==C_mail_client::C_mode_mailbox::ID) ? (C_mail_client::C_mode_mailbox*)GetParent() : NULL, false);
      return;

   case TXT_VIEW_IMAGE:
      reinterpret_cast<C_mail_client*>(&app)->Viewer_OpenLink(*this, NULL, false, true);
      break;
#endif

   case TXT_SCROLL:
      menu = CreateMenu();
      menu->AddItem(TXT_PAGE_UP, 0, "[*]", "[Q]");
      menu->AddItem(TXT_PAGE_DOWN, 0, "[#]", "[A]");
#ifdef EXPLORER
      menu->AddItem(TXT_TOP, text_info.top_pixel ? 0 : C_menu::DISABLED);
      menu->AddItem(TXT_BOTTOM, text_info.top_pixel<text_info.total_height-rc.sy ? 0 : C_menu::DISABLED);
#endif
      app.PrepareMenu(menu);
      break;

   case TXT_PAGE_UP:
   case TXT_PAGE_DOWN:
      auto_scroll_time = (sb.visible_space-app.font_defs[app.config.viewer_font_index].line_spacing) << 8;
      if(itm==TXT_PAGE_UP)
         auto_scroll_time = -auto_scroll_time;
      break;

#if defined MAIL || defined WEB
   case TXT_SHOW_LINK:
      app.ViewerShowLink(*this);
      break;
#endif

   case TXT_BACK:
   case TXT_EXIT:
      app.Close(*this);
      return;
#ifdef EXPLORER
   case TXT_FULLSCREEN:
      fullscreen = !fullscreen;
      InitLayout();
      break;
   case TXT_FIND:
      reinterpret_cast<C_explorer_client*>(&app)->ViewerFindText();
      return;

   case TXT_FIND_NEXT:
      {
         C_explorer_client *ec = reinterpret_cast<C_explorer_client*>(&app);
         ec->TxtViewerFindEntered(*this, ec->last_find_text);
      }
      return;

   case TXT_TOP:
      app.ScrollText(text_info, -text_info.top_pixel);
      sb.pos = text_info.top_pixel;
      break;

   case TXT_BOTTOM:
      app.ScrollText(text_info, text_info.total_height-text_info.top_pixel-rc.sy);
      sb.pos = text_info.top_pixel;
      break;

      /*
   case TXT_CFG_FONT_SIZE:
      mod.menu = mod.CreateMenu();
      mod.menu->AddItem(TXT_CFG_SMALL, config.viewer_font_index==2 ? C_menu::MARKED : 0);
      mod.menu->AddItem(TXT_CFG_NORMAL, config.viewer_font_index==1 ? C_menu::MARKED : 0);
      mod.menu->AddItem(TXT_CFG_BIG, config.viewer_font_index==0 ? C_menu::MARKED : 0);
      PrepareMenu(mod.menu);
      break;

   case TXT_CFG_SMALL:
   case TXT_CFG_NORMAL:
   case TXT_CFG_BIG:
      {
         int fi = itm==TXT_CFG_SMALL ? 2 : itm==TXT_CFG_NORMAL ? 1 : 0;
         if(config.viewer_font_index!=fi){
            config.viewer_font_index = fi;
            SaveConfig();

            InitLayoutViewer(mod);

            S_text_display_info &tdi = mod.text_info;
            tdi.ts.font_index = config.viewer_font_index;
            CountTextLinesAndHeight(tdi);
            tdi.pixel_offs = 0;
            tdi.top_pixel = 0;
            tdi.byte_offs = 0;
            mod.sb.pos = 0;
            mod.sb.total_space = tdi.total_height;
            mod.sb.SetVisibleFlag();
         }
      }
      break;
      */
#endif//EXPLORER
   }
}

//----------------------------

void C_client_viewer::C_mode_this::ProcessInput(S_user_input &ui, bool &redraw){

   int scroll_pixels = 0;      //positive = down; negative = up

#ifdef USE_MOUSE
   if(!app.ProcessMouseInSoftButtons(ui, redraw, false)){
      {
         C_scrollbar::E_PROCESS_MOUSE pm = app.ProcessScrollbarMouse(sb, ui);
         switch(pm){
         case C_scrollbar::PM_PROCESSED: redraw = true; break;
         case C_scrollbar::PM_CHANGED:
            scroll_pixels = sb.pos-text_info.top_pixel;
            break;
         default:
            if(app.ViewerProcessMouse(*this, ui, scroll_pixels, redraw)){
#ifdef MAIL
               if(reinterpret_cast<C_mail_client*>(&app)->Viewer_OpenLink(*this, (saved_parent->Id()==C_mail_client::C_mode_mailbox::ID) ? (C_mail_client::C_mode_mailbox*)(C_mode*)saved_parent : NULL, false))
                  return;
#endif
            }
         }
      }
   }
#endif

   switch(ui.key){
   case K_RIGHT_SOFT:
   case K_BACK:
   case K_ESC:
      app.Close(*this);
      return;
   case K_LEFT_SOFT:
   case K_MENU:
      menu = CreateMenu();
      {
#ifdef EXPLORER
         menu->AddItem(TXT_FIND, 0, "[1]");
         menu->AddItem(TXT_FIND_NEXT, (reinterpret_cast<C_explorer_client*>(&app)->last_find_text.Length() ? 0 : C_menu::DISABLED), "[2]");
#endif
         if(!app.HasMouse()){
            menu->AddItem(TXT_SCROLL, C_menu::HAS_SUBMENU);
            menu->AddSeparator();
         }
#if defined MAIL || defined WEB
         const S_hyperlink *href = text_info.GetActiveHyperlink();
         if(href){
            //dword flg = text_info.active_object_offs!=-1 ? 0 : C_menu::DISABLED;
            if(href->link.Length())
               menu->AddItem(TXT_OPEN_LINK, 0, ok_key_name);
            if(href->image_index!=-1)
               menu->AddItem(TXT_VIEW_IMAGE, 0, href->link.Length() ? NULL : ok_key_name);
            if(!app.HasMouse())
               menu->AddItem(TXT_SHOW_LINK, 0, "[0]");
            //if(href->link.Length())
               //menu->AddItem(TXT_SAVE_TARGET);
            menu->AddSeparator();
         }
#elif defined EXPLORER
         //menu->AddItem(TXT_CFG_FONT_SIZE, C_menu::HAS_SUBMENU);
#endif
      }
#ifdef EXPLORER
      menu->AddItem(TXT_FULLSCREEN, fullscreen ? C_menu::MARKED : 0, "[4]", "[F]");
      menu->AddSeparator();
#endif
      menu->AddItem(!GetParent() ? TXT_EXIT : TXT_BACK);
      app.PrepareMenu(menu);
      return;

   case K_CURSORUP:
   case K_CURSORDOWN:
      {
         app.ViewerProcessKeys(*this, ui, scroll_pixels, redraw);
#ifdef WEB_FORMS
                              //init text editor automatically
         S_html_form_input *fi = text_info.GetActiveFormInput();
         if(fi)
         switch(fi->type){
         case S_html_form_input::TEXT:
         case S_html_form_input::PASSWORD:
         case S_html_form_input::TEXTAREA:
            app.BrowserDoFormInputAction(mod, K_ENTER, redraw);
            break;
         }
#endif
//#endif
      }
      break;

#ifdef WEB_FORMS
   case K_CURSORLEFT:
   case K_CURSORRIGHT:
      {
         S_text_display_info &tdi = text_info;
         S_html_form_input *p_fi = tdi.GetActiveFormInput();
         if(p_fi && !p_fi->disabled){
            S_html_form_select_option &fsel = tdi.form_select_options[p_fi->form_select];
            switch(p_fi->type){
            case S_html_form_input::SELECT_SINGLE:
                              //change selected option
               if(ui.key==K_CURSORLEFT){
                  if(fsel.selected_option > 0){
                     --fsel.selected_option;
                     redraw = true;
                  }
               }else{
                  if(fsel.selected_option < (int)fsel.options.Size()-1){
                     ++fsel.selected_option;
                     redraw = true;
                  }
               }
               if(dropdown_select_expanded){
                  dropdown_select.selection = fsel.selected_option;
                  dropdown_select.EnsureVisible();
               }
               break;
            case S_html_form_input::SELECT_MULTI:
               if(ui.key==K_CURSORLEFT){
                  if(fsel.selection > 0){
                     --fsel.selection;
                     fsel.EnsureVisible();
                     redraw = true;
                  }
               }else{
                  if(fsel.selection < (int)fsel.options.Size()-1){
                     ++fsel.selection;
                     fsel.EnsureVisible();
                     redraw = true;
                  }
               }
               break;
            }
         }
      }
      break;
#endif

   case K_ENTER:
      {
                              //perform action
#ifdef MAIL
         if(reinterpret_cast<C_mail_client*>(&app)->Viewer_OpenLink(*this,
            (GetParent()->Id()==C_mail_client::C_mode_mailbox::ID) ? (C_mail_client::C_mode_mailbox*)GetParent() : NULL, false)){
            return;
         }
#endif
#ifdef WEB_FORMS
      if(BrowserDoFormInputAction(mod, ui.key, redraw))
         return;
#endif
      }
      break;

   case '#':
   case 'a':
   case 't':
   case K_PAGE_DOWN:
      {
         if(ui.key=='#' || ui.key=='a' || ui.key==K_PAGE_DOWN)
            auto_scroll_time = (sb.visible_space-app.font_defs[app.config.viewer_font_index].line_spacing) << 8;
      }
      break;

   case '*':
   case 'q':
   case 'r':
   case K_PAGE_UP:
      {
         if(ui.key=='*' || ui.key=='q' || ui.key==K_PAGE_UP)
            auto_scroll_time = -((sb.visible_space-app.font_defs[app.config.viewer_font_index].line_spacing) << 8);
      }
      break;

#ifdef EXPLORER
   case '1':
      reinterpret_cast<C_explorer_client*>(&app)->ViewerFindText();
      return;
#endif

   case 'o':
   case '0':
      {
         app.ViewerShowLink(*this);
         redraw = true;
      }
      break;

   case '2':
   case '3':
   case 'p':
   case 'n':
#ifdef EXPLORER
      if(ui.key=='2'){
         C_explorer_client *ec = reinterpret_cast<C_explorer_client*>(&app);
         if(ec->last_find_text.Length()){
            ec->TxtViewerFindEntered(*this, ec->last_find_text);
            return;
         }
      }
#endif
      break;

#ifdef EXPLORER
   case '4':
   case 'f':
      fullscreen = !fullscreen;
      InitLayout();
      redraw = true;
      break;
#endif
   }
   kinetic_movement.ProcessInput(ui, text_info.rc, 0, -1);

#ifdef WEB_FORMS
   S_html_form_input *fi = text_info.GetActiveFormInput();
   if(fi)
   switch(fi->type){
   case S_html_form_input::TEXT:
   case S_html_form_input::PASSWORD:
   case S_html_form_input::TEXTAREA:
      if(text_info.te_input){
         C_text_editor &te = *text_info.te_input;
                           //need to update the text editor
         bool text_changed;
         te.TickFull(ui, text_changed);
         if(text_changed){
            if(te.mask_password)
               te.mask_password = false;
            if(text_changed)
               fi->value = te.text.begin();
         }
         switch(ui.key){
         case '*':
         case '#':
            ui.key = 0;
            break;
         }
      }
      break;
   }
#endif
   if(scroll_pixels && app.ScrollText(text_info, scroll_pixels)){
      sb.pos = text_info.top_pixel;
      redraw = true;
   }
}

//----------------------------

void C_client_viewer::C_mode_this::Tick(dword time, bool &redraw){

   app.ViewerTickCommon(*this, time, redraw);
   {
      if(kinetic_movement.IsAnimating()){
         S_point p;
         kinetic_movement.Tick(time, &p);
         if(app.ScrollText(text_info, p.y)){
            sb.pos = text_info.top_pixel;
            redraw = true;
         }else
            kinetic_movement.Reset();
      }
   }
}

//----------------------------

void C_client_viewer::DownloadSocketEvent(C_mode_this &mod, C_socket_notify::E_SOCKET_EVENT ev, C_socket *socket, bool &redraw){

#ifdef MAIL
   reinterpret_cast<C_mail_client*>(this)->TickViewerImageLoader(mod, ev, mod, NULL, mod.html_images, redraw);
#endif
}

//----------------------------

void C_client_viewer::C_mode_this::Draw() const{

   dword sx = app.ScrnSX();
   if(!fullscreen){
      app.ClearTitleBar();
      app.ClearSoftButtonsArea(rc.Bottom() + 1);
   }

   {
      const dword col_text = app.GetColor(COL_TEXT_TITLE);
      int x = app.fdb.letter_size_x / 2;
      int max_w = sx - x;
      app.DrawString(title, x, 0, UI_FONT_BIG, 0, col_text, -max_w);
   }
   {
                              //text mode
      app.DrawFormattedText(text_info, &rc);
      app.DrawScrollbar(sb);
   }
   if(!fullscreen){
      const dword c0 = app.GetColor(COL_SHADOW), c1 = app.GetColor(COL_HIGHLIGHT);
      app.DrawOutline(rc, c0, c1);
   }

#ifdef MAIL
   if(img_loader.IsActive()){
      app.DrawOutline(img_loader.prog_curr.rc, 0xff000000);
      app.DrawOutline(img_loader.prog_all.rc, 0xff000000);
      app.DrawProgress(img_loader.prog_curr, 0xffff0000, false, 0x40000000);
      app.DrawProgress(img_loader.prog_all, 0xff00ff00, false, 0x40000000);
   }
#endif
   if(link_show_count){
      const S_rect &rc1 = rc_link_show;
      app.DrawOutline(rc1, 0xff000000);
      app.FillRect(rc1, 0xffffffff);
      app.DrawSimpleShadow(rc1, true);
      app.DrawNiceURL(link_show, rc1.x+app.fds.letter_size_x/2, rc1.y, UI_FONT_SMALL, 0xff000000, rc1.sx-app.fds.letter_size_x);
   }

   {
      if(!fullscreen)
         app.DrawSoftButtonsBar(*this, TXT_MENU, !GetParent() ? TXT_EXIT : TXT_BACK);
#ifdef WEB_FORMS
      if(text_info.te_input)
         app.DrawTextEditor(*text_info.te_input);
#endif
   }
   app.SetScreenDirty();
}

//----------------------------
#ifdef MAIL

static void DetectLinks(Cstr_c &body, S_text_display_info &tdi, bool detect_phone_numbers){

   const char *cp = body, *last_end = cp;
   Cstr_c addr, link;
   bool is_in_link = false;
   C_vector<char> tmp;
   bool is_beg_line = true;
   int block_quote_count = 0;
   const char *link_prefix = NULL;

   for(; *cp; ){
      const char *addr_cp = NULL;
      char c = *cp;
      if(S_text_style::IsControlCode(c)){
         S_text_style::SkipCode(cp);
         if(c==CC_HYPERLINK)
            is_in_link = true;
         else
         if(c==CC_HYPERLINK_END)
            is_in_link = false;
         continue;
      }
      if(is_in_link){
         ++cp;
         continue;
      }
      if(c=='@'){
         addr.Clear();
         link.Clear();
                                 //detect email address
         const char *cp1 = cp-1;
         for(; cp1>=last_end; --cp1){
            const char *cp2 = cp1;
            Cstr_c test;
            if(!ReadAddressSpec(cp2, test)){
               if(*cp2!='.')
                  break;
               continue;
            }
                              //must contain dot
            if(test.Find('.')==-1)
               break;
            addr = test;
            addr_cp = cp1;
         }
         link_prefix = "mailto:";
      }else if(detect_phone_numbers && (text_utils::IsDigit(c) || c=='+' || c=='(')){
                              //detect phone numbers
         char prev = 0;
         if(cp!=(const char*)body)
            prev = cp[-1];
         if(!prev || text_utils::IsSpace(prev) || prev==':' || S_text_style::IsControlCode(prev)){
            int last_good_i = 0;
            int num_plus = 0;
            bool in_braces = false;
            link.Clear();
            int i;
            for(i=0; ; i++){
               char c1 = cp[i];
               if(text_utils::IsDigit(c1)){
                  link<<c1;
                  last_good_i = i+1;
               }else if(c1=='+'){
                  if(link.Length() || ++num_plus>2)
                     break;
                  link<<c1;
               }else if(c1=='('){
                  if(in_braces)
                     break;
                  in_braces = true;
               }else if(c1==')'){
                  if(!in_braces)
                     break;
                  in_braces = false;
               }else if(c1=='-'){
               }else if(c1=='.'){
               }else if(c1==' '){
               }else{
                  if(c1=='@')
                     link.Clear();
                  break;
               }
            }
            if(link.Length()>=6){
               addr_cp = cp;
               addr.Allocate(cp, last_good_i);
               link_prefix = "tel:";
            }else
            if(link.Length())
               cp += link.Length()-1;
         }
      }else{
                              //detect url's
         bool is_https = false;
         bool has_http_prefix = false;
         if(text_utils::CheckStringBegin(cp, "www.", true, false) || (has_http_prefix=true, (text_utils::CheckStringBegin(cp, text_utils::HTTP_PREFIX, true, true) || (is_https=true, text_utils::CheckStringBegin(cp, text_utils::HTTPS_PREFIX, true, true))))){
            const char *cp1 = cp;
            if(text_utils::ReadToken(cp1, addr, " \r\n\t\\\"\x7f>]")){
               for(int i=addr.Length(); i--; ){
                  if(addr[i]!='.')
                     break;
                  addr.At(i)=0;
                  addr = (const char*)addr;
               }
               addr_cp = cp;
               if(has_http_prefix){
                  addr_cp -= is_https ? 8 : 7;
                  Cstr_c tmp1; tmp1<<(!is_https ? text_utils::HTTP_PREFIX : text_utils::HTTPS_PREFIX) <<addr;
                  addr = tmp1;
               }
            }
            link_prefix = !has_http_prefix ? text_utils::HTTP_PREFIX : NULL;
            link.Clear();
         }else{
            if(is_beg_line){
                              //detect qutoes, replace by style
               if(!tmp.size())
                  tmp.reserve(body.Length()*10/8);
               tmp.insert(tmp.end(), last_end, cp);

               int qc;
               for(qc=0; cp[qc]=='>'; ++qc);

               cp += qc;

               qc = Min(qc, 5);
               if(qc!=block_quote_count){
                              //quote count changed, update style
                  S_text_style::WriteCode(CC_BLOCK_QUOTE_COUNT, &block_quote_count, &qc, 1, tmp);
                  block_quote_count = qc;
               }
                              //remove possible space after > quotes for keeping nicer formatting
               last_end = cp;
               if(qc && *cp==' ')
                  ++last_end;
            }
            is_beg_line = (*cp=='\n');
         }
      }
      if(addr_cp){
         if(!tmp.size())
            tmp.reserve(body.Length()*10/8);
                              //copy previous text
         tmp.insert(tmp.end(), last_end, addr_cp);
                              //make hyperlink
         S_hyperlink href;
         if(link_prefix)
            href.link<<link_prefix;
         href.link<<(link.Length() ? link : addr);
         tdi.BeginHyperlink(tmp, href);
         
         tmp.insert(tmp.end(), addr, (const char*)addr+addr.Length());

         tdi.EndHyperlink(tmp);

                              //continue
         last_end = addr_cp+addr.Length();
         cp = last_end - 1;
      }
      ++cp;
   }
   if(tmp.size()){
      tmp.insert(tmp.end(), last_end, cp);
      body.Clear();
      body.Allocate(tmp.begin(), tmp.size());
   }
}
#endif//MAIL
//----------------------------

bool C_client_viewer::OpenTextFile(const wchar *filename, int is_html, S_text_display_info &tdi, S_file_open_help &oh) const{

   C_file *ck = CreateFile(filename, oh.arch);
   if(!ck){
      tdi.body_c = "\n";
      tdi.is_wide = false;
      return false;
   }
   dword sz = ck->GetFileSize();
                              //limitation of Cstr_c class
   sz = Min(sz, (dword)Cstr_w::MAX_STRING_LEN);
   bool wide = false, big_endian = false;

   if(sz >= 2){
      word mark;
      byte mark1;
      ck->ReadWord(mark);
      if(mark==0xfeff || mark==0xfffe){
                              //utf-16
         wide = true;
         big_endian = (mark==0xfffe);
         sz -= 2;
         sz /= sizeof(wchar);
      }else
      if(mark==0xbbef && ck->ReadByte(mark1) && mark1==0xbf){
                              //utf-8
         tdi.body_c.coding = COD_UTF_8;
         sz -= 3;
      }else
         ck->Seek(0);
   }
   if(!wide){
#if defined MAIL || defined EXPLORER || defined WEB_FORMS //|| defined MUSICPLAYER
      if(is_html==-1){
                              //auto-detect html
         is_html = false;
         Cstr_w ext; ext.Copy(text_utils::GetExtension(filename)); ext.ToLower();
         if(ext==L"htm" || ext==L"html")
            is_html = true;
         else{
            int pos = ck->Tell();
            int seek_sz = Min(512, (int)sz);
            while(seek_sz-- >= 6){
               char c;
               ck->ReadByte((byte&)c);
               if(c=='<'){
                  char buf[5];
                  for(int i=0; i<5; --seek_sz, i++){
                     ck->ReadByte((byte&)c);
                     buf[i] = ToLower(c);
                     if(c=='>')
                        break;
                  }
                  is_html = (!MemCmp(buf, "html>", 5) || !MemCmp(buf, "!doct", 5) || !MemCmp(buf, "head", 4));
               }
               if(is_html)
                  break;
               //if(c!=' ' && c!='\t' && c!='\n' && c!='\r')
                  //break;
            }
            ck->Seek(pos);
         }
      }
      if(is_html){
                              //html
         C_vector<char> src;
         src.resize(sz+1);
         ck->Read(src.begin(), sz);
         src[sz] = 0;
         const char *cp = src.begin();
         Cstr_c tmp;

         if(encoding::IsMultiByteCoding(tdi.body_c.coding) && tdi.body_c.coding!=COD_UTF_8){
                              //decode now
            Cstr_w s;
            ConvertStringToUnicode(cp, tdi.body_c.coding, s);
            src.clear();
            tmp.ToUtf8(s);
            cp = tmp;
            sz = tmp.Length();
            tdi.body_c.coding = COD_UTF_8;
         }
         CompileHTMLText(cp, sz, tdi, oh);
      }else
#endif//MAIL || EXPLORER
      {
                              //plain text
         tdi.body_c.Allocate(NULL, sz);
         tdi.is_wide = false;
         if(sz)
            ck->Read(&tdi.body_c.At(0), sz);
         if(encoding::IsMultiByteCoding(tdi.body_c.coding)){
                              //convert to unicode first
            Cstr_w s;
            ConvertStringToUnicode(tdi.body_c, tdi.body_c.coding, s);
#if 1
            C_vector<char> buf;
            dword s_len = s.Length();
            const wchar *wp = s;
            buf.reserve(s_len);
                              //now convert to single-byte coded text
            while(true){
               wchar c = *wp++;
               if(!c)
                  break;
               if(c>=256){
                  buf.push_back(CC_WIDE_CHAR);
                  buf.push_back(char(c&255));
                  buf.push_back(char(c>>8));
                  buf.push_back(CC_WIDE_CHAR);
               }else{
                  if(byte(c)<0x20)
                  switch(c){
                  case '\n':
                  case '\r':
                     break;
                  case '\t': c = ' '; break;
                  default:
                     c = '?';
                  }
                  if(c)
                     buf.push_back(char(c));
               }
            }
            tdi.body_c.Allocate(buf.begin(), buf.size());
#else
            tdi.body_w = s;
            tdi.is_wide = true;
            tdi.body_c.Clear();
#endif
            tdi.body_c.coding = COD_DEFAULT;
         }else{
                              //replace non-displayable characters
            sz = tdi.body_c.Length();
            for(int i=sz; i--; ){
               char &c = tdi.body_c.At(i);
               if(byte(c)<0x20){
                  switch(c){
                  case '\n':
                  case '\r':
                     break;
                  case '\t': c = ' '; break;
                  default:
                     c = '?';
                  }
               }
            }
         }
      }
#ifdef MAIL
      if(!oh.preview_mode && tdi.body_c.Length())
         DetectLinks(tdi.body_c, tdi, oh.detect_phone_numbers);
#endif
   }else{
      tdi.body_w.Allocate(NULL, sz);
      if(sz){
         wchar *wp = &tdi.body_w.At(0);
         ck->Read(wp, sz*sizeof(wchar));
         if(big_endian){
            for(dword i=0; i<sz; i++)
               wp[i] = ByteSwap16(wp[i]);
         }
      }
      for(int i=sz; i--; ){
         wchar &c = tdi.body_w.At(i);
         if(c<0x20){
            switch(c){
            case '\n':
            case '\r':
               break;
            case '\t': c = ' '; break;
            default:
               c = '?';
            }
         }
      }
      tdi.is_wide = true;
   }
   delete ck;
#ifdef _DEBUG
   //tdi.mark_start = 21; tdi.mark_end = 24;
#endif
   return true;
}

//----------------------------
#if defined MAIL || defined WEB || defined EXPLORER || defined MESSENGER

//----------------------------
static bool ReadHTMLColor(const Cstr_c &val, dword &color){

   if(!val.Length())
      return false;
   if(val=="black") color = 0xff000000;
   else if(val=="green") color = 0xff008000;
   else if(val=="silver") color = 0xffc0c0c0;
   else if(val=="lime") color = 0xff00ff00;
   else if(val=="gray") color = 0xff808080;
   else if(val=="olive") color = 0xff808000;
   else if(val=="white") color = 0xffffffff;
   else if(val=="yellow") color = 0xffffff00;
   else if(val=="maroon") color = 0xff800000;
   else if(val=="navy") color = 0xff000080;
   else if(val=="red") color = 0xffff0000;
   else if(val=="blue") color = 0xff0000ff;
   else if(val=="blue") color = 0xff0000ff;
   else if(val=="purple") color = 0xff800080;
   else if(val=="teal") color = 0xff008080;
   else if(val=="fuchsia") color = 0xffff00ff;
   else if(val=="aqua") color = 0xff00ffff;
   else
   if((val[0]=='#' && (val.Length()==7 || val.Length()==4)) || val.Length()==6){
      const char *cp = val;
      if(*cp=='#')
         ++cp;
                              //expect 6 digits now
      color = 0;
      bool sh = (StrLen(cp)==3);
      if(!text_utils::ScanHexNumber(cp, (int&)color))
         return false;
      if(sh)
         color = ((color<<12)&0xf00000) | ((color<<8)&0xf000) | ((color<<4)&0xf0);
/*      
      for(int i=6; i--; ){
         dword c = *cp++;
         if(IsDigit(c))
            c -= '0';
         else
         if(c>='a' && c<='f')
            c -= 'a'-10;
         else
         if(c>='A' && c<='F')
            c -= 'A'-10;
         else{
            //assert(0);
            return false;
         }
         color |= c << (i*4);
      }
         */
      color |= 0xff000000;
   }else{
      const char *cp = val;
      if(text_utils::CheckStringBegin(cp, "dark")){
                              //dark color
         Cstr_c n = cp;
         if(!ReadHTMLColor(n, color))
            return false;
         //color = MulColor(color, 0x8c00);
         color = 0xff000000 | ((color>>1)&0x7f7f7f);
      }else
      if(text_utils::CheckStringBegin(cp, "rgb")){
         text_utils::SkipWS(cp);
         if(*cp=='('){
            ++cp;
            S_rgb_x rgb;
            for(int i=3; i--; ){
               text_utils::SkipWS(cp);
               int ival;
               if(!text_utils::ScanDecimalNumber(cp, ival) || ival<0 || ival>255)
                  return false;
               rgb[i] = byte(ival);
               text_utils::SkipWS(cp);
               if(i){
                  if(*cp!=',')
                     return false;
                  ++cp;
               }
            }
            if(*cp!=')')
               return false;
            rgb.a = 0xff;
            color = rgb.To32bit();
         }else
            return false;
      }else{
         return false;
#ifdef _DEBUG
                              //unknown color name
         //Info(val);
#endif
      }
   }
   return true;
}
#endif

//----------------------------
#if defined MAIL || defined WEB || defined EXPLORER

static bool ReadWordDoc(const byte *hdr, C_file &ck, int offset, C_vector<char> &dst_buf, int font_index){

   enum{
      DOC_DOT = 1,
      DOC_GLSY = 2,
      DOC_COMPLEX = 4,
      DOC_PICTURES = 8,
      DOC_QUICKSAVE_MASK = 0xf0,
      DOC_ENCRYPTED = 0x100,
      DOC_RESERVED = 0x200,
      DOC_READONLY = 0x400,
      DOC_WRITE_RESERVED = 0x800,
      DOC_EXTCHAR = 0x1000,
   };
   enum{
                              //following symbols below 32 are allowed inside paragraph:
      SYM_FOOTNOTE_MARK = 2,
      SYM_CELL_MARK = 7,
      SYM_HARD_LINE_BREAK = 0xb,
      SYM_PAGE_BREAK = 0xc,
      SYM_PARA_END = 0xd,
      SYM_COLUMN_BREAK = 0xe,
      SYM_FIELD_BEGIN = 0x13,
      SYM_FIELD_SEPARATOR,
      SYM_FIELD_END,
      SYM_HARD_HYPHEN = 0x1e,
      SYM_SOFT_HYPHEN = 0x1f,
      SYM_HYPHEN = 0x96,
      SYM_HARD_SPACE = 0xa0,
   };
   dword flags = *(word*)(hdr+10);
   if(flags&DOC_ENCRYPTED)
      return false;

                              //skipping to textstart and computing textend
   dword textstart = *(dword*)(hdr+24);
   dword textlen = *(dword*)(hdr+28) - textstart;
   textstart += offset;

   if(ck.GetFileSize() - ck.Tell() < textstart)
      return false;

   ck.Seek(ck.Tell() + textstart);

   offset = 0;

   byte ext_buf[256];
   bool ext_buf_wide = false;

   S_text_style ts_last;
   S_text_style ts;
   ts_last.font_index = ts.font_index = font_index;

   //bool tab_mode = false;
   enum{
      PHASE_NORMAL,
      PHASE_FIELD,
   } phase = PHASE_NORMAL;

   while(offset<int(textlen)){
      wchar c;
      if(flags&DOC_EXTCHAR){
         int bi = offset&255;
         if(!bi){
                           //read extended chars into cache
            dword rd = Min(256ul, ck.GetFileSize()-ck.Tell());
            rd = Min(rd, textlen-offset);
            ck.Read(ext_buf, rd);
            MemSet(ext_buf+rd, 0, 256-rd);
                              //detect if block is single or multi byte (ugly approach)
            ext_buf_wide = false;
            if(!(rd&1)){
               ext_buf_wide = true;
               for(dword i=0; i<rd; i+=2){
                  //byte c0 = ext_buf[i];
                  byte c1 = ext_buf[i+1];
                  if(c1>0x40){
                     ext_buf_wide = false;
                     break;
                  }
               }
               if(!ext_buf_wide){
                              //count printable characters
                  dword nump = 0;
                  dword num_rd = rd;
                  for(int i=rd; i--; ){
                     byte c1 = ext_buf[i];
                     if(c1>=' ' && c1<128)
                        ++nump;
                     else
                     switch(c1){
                     case '\n':
                     case '\r':
                     case '\t':
                     case SYM_CELL_MARK:
                        ++nump;
                        break;
                     case 0:
                        --num_rd;
                        break;
                     }
                  }
                  if(num_rd && (nump*256/num_rd) < 0xe0){
                              //non-string block, skip
                     offset += 256;
                     continue;
                  }
               }
               /*
               for(int i=rd; i--; ){
                  byte c = ext_buf[i];
                  bool is_punc = ((c>=128) || (c>' '&&c<'0') || (c>=':'&&c<='@') || (c>='['&&c<'a'));
                  if((c==' ' || c==SYM_PARA_END || is_punc) && i<rd-1 && ext_buf[i+1]==0){
                  //if(!c){
                     ext_buf_wide = true;
                     break;
                  }
               }
               */
            }
         }
         if(ext_buf_wide){
            c = ext_buf[bi] | (ext_buf[bi+1]<<8);
            offset += 2;
         }else{
            c = ext_buf[bi];
            ++offset;
         }
      }else{
         ++offset;
         byte b;
         ck.ReadByte(b);
         c = b;

      }
      switch(phase){
      case PHASE_NORMAL:
         {
            /*
            if(tab_mode){
               tab_mode = false;
               if(c==SYM_CELL_MARK)
                  c = ' ';
               else
                  c = '\n';
            }
            */
            switch(c){
            case SYM_CELL_MARK:
               //tab_mode = true;
               {
                  for(int i=0; i<3; i++)
                     dst_buf.push_back(' ');
               }
               continue;
            case SYM_PARA_END:
            case SYM_COLUMN_BREAK:
            case SYM_HARD_LINE_BREAK:
            case SYM_PAGE_BREAK:
               dst_buf.push_back('\n');
               continue;
            case SYM_FOOTNOTE_MARK:
               continue;
            case SYM_SOFT_HYPHEN:
            case SYM_HARD_HYPHEN:
            case SYM_HYPHEN:
               c = '-';
               break;
            case '\t':
               c = ' ';
               break;
            case SYM_HARD_SPACE:
               c = ' ';
               break;

            case SYM_FIELD_SEPARATOR:
               continue;

            case SYM_FIELD_BEGIN:
               ts.text_color = 0xff0000ff;
               phase = PHASE_FIELD;
               continue;

            case SYM_FIELD_END:
               ts.text_color = 0xff000000;
               break;
            }
            if(c>=' ' && c < 0xfeff){
               ts_last.WriteAndUpdate(ts, dst_buf);
               if(c<256)
                  dst_buf.push_back(char(c));
               else{
                  dst_buf.push_back(CC_WIDE_CHAR);
                  dst_buf.push_back(char(c&255));
                  dst_buf.push_back(char(c>>8));
                  dst_buf.push_back(CC_WIDE_CHAR);
               }
            }
         }
         break;
      case PHASE_FIELD:
         switch(c){
         case SYM_FIELD_SEPARATOR:
         case SYM_FIELD_END:
            phase = PHASE_NORMAL;
            break;
         }
         break;
      }
   }
   return true;
}

//----------------------------

struct S_docx_parser{
   C_zip_package *zip;
   C_xml xml;
   C_xml xml_rels;
   S_text_style ts;
   C_vector<char> buf;
   const C_application_base &app;
   S_text_display_info &tdi;
   struct S_style: public S_text_style{
      Cstr_c id;
   };
   C_vector<S_style> styles;
   const S_style *FindStyle(const char *id) const{
      for(int i=styles.size(); i--; ){
         const S_style &st = styles[i];
         if(st.id==id)
            return &st;
      }
      return NULL;
   }
   typedef C_xml::C_element C_el;

   const char *FindRelationshipTarget(const char *id) const{
      for(const C_el *el_rel = xml_rels.Find("Relationships/Relationship"); el_rel; el_rel=el_rel->FindSameSibling()){
         const char *id1 = el_rel->FindAttributeValue("Id");
         if(!StrCmp(id, id1)){
            const char *t = el_rel->FindAttributeValue("Target");
            if(t)
               return t;
         }
      }
      return NULL;
   }

   void ParseStyle(const C_el &el_st, S_text_style &ts) const;

   S_docx_parser(const C_application_base &_app, S_text_display_info &t):
      app(_app),
      tdi(t),
      zip(NULL)
   {}
};

//----------------------------

void S_docx_parser::ParseStyle(const C_el &el_st, S_text_style &_ts) const{

   const C_el *el_ppr = el_st.Find("w:pPr");
   const C_el *el_rpr = el_st.Find("w:rPr");
   if(el_ppr){
                           //determine predefined style
      const C_el *el_style = el_ppr->Find("w:pStyle");
      if(el_style){
         const char *id = el_style->FindAttributeValue("w:val");
         if(id){
            const S_docx_parser::S_style *st = FindStyle(id);
            if(st)
               _ts = *st;
         }
      }
      const C_el *el_jc = el_ppr->Find("w:jc");
      if(el_jc){
         const char *val = el_jc->FindAttributeValue("w:val");
         if(val){
            if(!StrCmp(val, "center"))
               _ts.font_flags |= FF_CENTER;
         }
      }
      if(!el_rpr)
         el_rpr = el_ppr->Find("w:rPr");
   }
   if(el_rpr){
      if(el_rpr->Find("w:b"))
         _ts.font_flags |= FF_BOLD;
      if(el_rpr->Find("w:i"))
         _ts.font_flags |= FF_ITALIC;
      if(el_rpr->Find("w:u"))
         _ts.font_flags |= FF_UNDERLINE;
      const C_el *el_col = el_rpr->Find("w:color");
      if(el_col){
         const char *val = el_col->FindAttributeValue("w:val");
         if(val){
            ReadHTMLColor(val, _ts.text_color);
         }
      }
      /*
      const C_el *el_sz = el_rpr->Find("w:sz");
      if(el_sz){
         const char *sz = el_sz->FindAttributeValue("w:val");
         int size;
         if(sz && text_utils::ScanInt(sz, size)){
            //if(size>40)
               //_ts.font_index = 1;
         }
      }
      */
   }
}

//----------------------------

static void ParseDocXImage(S_docx_parser &parser, const char *id){

   const char *fn = parser.FindRelationshipTarget(id);
   if(fn){
      Cstr_w fn1;
      fn1.Copy(fn);
      while(fn1.Find('/')!=-1)
         fn1.At(fn1.Find('/')) = '\\';
      Cstr_w fn_img = L"word\\";
      fn_img<<fn1;
      C_image *img = C_image::Create(parser.app);
      if(img->Open(fn_img, parser.tdi.rc.sx, parser.tdi.rc.sy, parser.zip)==C_image::IMG_OK){
         int ii = parser.tdi.images.size();
         S_image_record &ir = parser.tdi.images.push_back(S_image_record());
         ir.img = img;
         ir.sx = img->SizeX();
         ir.sy = img->SizeY();
         
         parser.buf.push_back(CC_IMAGE);
         parser.buf.push_back(char(ii&0xff));
         parser.buf.push_back(char(ii>>8));
         parser.buf.push_back(CC_IMAGE);
      }
      img->Release();
   }
}

//----------------------------

static void ParseDocXResource(S_docx_parser &parser, const C_xml::C_element &el_r, const S_text_style &ts_p){

   typedef C_xml::C_element C_el;
   const C_vector<C_el> &r_children = el_r.GetChildren();
   for(int bi=0; bi<r_children.size(); ++bi){
      const C_el &el = r_children[bi];
      if(!StrCmp(el.GetName(), "w:t")){
         Cstr_w s = parser.xml.DecodeString(el.GetContent());
         parser.ts.AddWideStringToBuf(parser.buf, s);
      }else
      if(!StrCmp(el.GetName(), "w:br")){
         parser.buf.push_back('\n');
      }else
      if(!StrCmp(el.GetName(), "w:tab")){
         parser.buf.push_back(' ');
      }else
      if(!StrCmp(el.GetName(), "w:drawing")){
         const C_el *el_mode = el.Find("wp:inline");
         if(!el_mode)
            el_mode = el.Find("wp:anchor");
         if(el_mode){
            const C_el *el_img = el_mode->Find("a:graphic/a:graphicData/pic:pic/pic:blipFill/a:blip");
            if(el_img){
               const char *id = el_img->FindAttributeValue("r:embed");
               if(id)
                  ParseDocXImage(parser, id);
            }
         }
      }else
      if(!StrCmp(el.GetName(), "w:object")){
         const C_el *el_img = el.Find("v:shape/v:imagedata");
         if(el_img){
            const char *id = el_img->FindAttributeValue("r:id");
            if(id)
               ParseDocXImage(parser, id);
         }
      }
   }
}

//----------------------------

static void ParseDocXParagraph(S_docx_parser &parser, const C_xml::C_element &el, bool is_link = false){

   typedef C_xml::C_element C_el;

   S_text_style ts_p;
   parser.ParseStyle(el, ts_p);
   if(is_link)
      ts_p.font_flags |= FF_UNDERLINE;
   parser.ts.WriteAndUpdate(ts_p, parser.buf);

   const C_vector<C_el> &e_children = el.GetChildren();
   for(int ci=0; ci<e_children.size(); ci++){
      const C_el &el_c = e_children[ci];
      if(!StrCmp(el_c.GetName(), "w:r")){
         S_text_style ts = ts_p;
         parser.ParseStyle(el_c, ts);
         parser.ts.WriteAndUpdate(ts, parser.buf);
         ParseDocXResource(parser, el_c, ts_p);
      }else
      if(!StrCmp(el_c.GetName(), "w:hyperlink")){
         const char *link = NULL;
         const char *id = el_c.FindAttributeValue("r:id");
         if(id)
            link = parser.FindRelationshipTarget(id);
         if(link){
            S_hyperlink href;
            href.link = link;
            parser.tdi.BeginHyperlink(parser.buf, href);
         }
         ParseDocXParagraph(parser, el_c, true);
         if(link){
            parser.tdi.EndHyperlink(parser.buf);
         }
      }
   }
}

//----------------------------

static void ParseDocXElements(S_docx_parser &parser, const C_xml::C_element &el_root){

   typedef C_xml::C_element C_el;

   const C_vector<C_el> &body_children = el_root.GetChildren();
   for(int bi=0; bi<body_children.size(); ++bi){
      const C_el &el = body_children[bi];
      if(!StrCmp(el.GetName(), "w:p")){
                           //paragraph
         ParseDocXParagraph(parser, el);
         parser.buf.push_back('\n');
      }else
      if(!StrCmp(el.GetName(), "w:tbl")){
                           //table
         parser.buf.push_back('\n');
                           //rows
         for(const C_el *el_tr = el.Find("w:tr"); el_tr; el_tr=el_tr->FindSameSibling()){
                           //columns
            for(const C_el *el_tc = el_tr->Find("w:tc"); el_tc; el_tc=el_tc->FindSameSibling()){
               ParseDocXElements(parser, *el_tc);
            }
            parser.buf.push_back('\n');
         }
         parser.buf.push_back('\n');
      }
   }
}

//----------------------------

bool C_client_viewer::ReadWordDocX(const wchar *filename, S_text_display_info &tdi, const C_zip_package *arch) const{

   S_docx_parser parser(*this, tdi);
   {
      C_file *fl = CreateFile(filename, arch);
      if(!fl)
         return false;
      parser.zip = C_zip_package::Create(fl, true);
      if(!parser.zip)
         return false;
   }
   bool ok = false;

   C_file_zip fl;

   if(fl.Open(L"word\\document.xml", parser.zip))
   do{
      typedef C_xml::C_element C_el;
      if(!parser.xml.Parse(fl))
         break;
                              //parse relationships
      if(fl.Open(L"word\\_rels\\document.xml.rels", parser.zip))
         parser.xml_rels.Parse(fl);
                              //parse styles
      if(fl.Open(L"word\\styles.xml", parser.zip)){
         C_xml xml;
         if(xml.Parse(fl)){
            for(const C_el *el_st = xml.Find("w:styles/w:style"); el_st; el_st=el_st->FindSameSibling()){
               const char *id = el_st->FindAttributeValue("w:styleId");
               if(id){
                  S_docx_parser::S_style style;
                  style.id = id;
                  parser.ParseStyle(*el_st, style);
                  parser.styles.push_back(style);
               }
            }
         }
      }

      const C_el *el_body = parser.xml.Find("w:document/w:body");
      if(!el_body)
         break;
      ParseDocXElements(parser, *el_body);
      ok = true;
   }while(false);
   fl.Close();
   delete parser.zip;
   if(ok){
      tdi.body_c.Allocate(parser.buf.begin(), parser.buf.size());
   }
   return ok;
}

//----------------------------

bool C_client_viewer::OpenWordDoc(const wchar *filename, S_text_display_info &tdi, const C_zip_package *arch) const{

   bool ok = false;

   if(ToLower(filename[StrLen(filename)-1])=='x'){
      ok = ReadWordDocX(filename, tdi, arch);
   }else{
      C_vector<char> dst_buf;
      C_file *ck = CreateFile(filename, arch);
      if(!ck)
         return false;
      if(ck->GetFileSize() < 512){
         delete ck;
         return false;
      }
      dst_buf.reserve(ck->GetFileSize()/2);

      byte hdr[128];
      ck->Read(hdr, 4);

      if(hdr[1]==0xa5 && (hdr[0]&0x80)){
         ck->Read(hdr+4, 124);
         ok = ReadWordDoc(hdr, *ck, 0, dst_buf, config.viewer_font_index);
      }else{
         ck->Read(hdr+4, 4);
         static const byte ole_sign[] = {0xd0, 0xcf, 0x11, 0xe0, 0xa1, 0xb1, 0x1a, 0xe1};
         if(!MemCmp(hdr, ole_sign, 8)){
            ck->Read(hdr+8, 120);
            int offset = 128;
            while(ck->GetFileSize()-offset >= 128){
               ck->Read(hdr, 128);
               if(hdr[1]==0xa5 && (hdr[0]&0x80)){
                  ok = ReadWordDoc(hdr, *ck, -128, dst_buf, config.viewer_font_index);
                  break;
               }
               offset += 128;
            }
         }
      }
      if(ok){
                                 //copy buffer to body string
         dst_buf.push_back(0);
         tdi.body_c.Allocate(dst_buf.begin(), dst_buf.size());
         tdi.is_wide = false;
      }
      delete ck;
   }
   return ok;
}
#endif

//----------------------------
//----------------------------
#ifdef WEB_FORMS

bool C_client_viewer::BrowserDoFormInputAction(C_mode_this &mod, dword key, bool &redraw){

   S_text_display_info &tdi = mod.text_info;
   S_html_form_input *p_fi = tdi.GetActiveFormInput();
   if(!p_fi || p_fi->disabled)
      return false;

   switch(p_fi->type){
   case S_html_form_input::CHECKBOX:          //toggle check
      p_fi->checked = !p_fi->checked;
      redraw = true;
      if(mod.form_action && (this->*mod.form_action)(mod, p_fi, FORM_CHANGE, redraw))
         return true;
      break;

   case S_html_form_input::RADIO:
      if(!p_fi->checked){                        //this one is checked, all others not
         for(int i=tdi.form_inputs.size(); i--; ){
            S_html_form_input &fii = tdi.form_inputs[i];
            if(fii.form_index==p_fi->form_index && p_fi->name==fii.name && fii.type==fii.RADIO)
               fii.checked = false;
         }
         p_fi->checked = true;
         redraw = true;
         if(mod.form_action && (this->*mod.form_action)(mod, p_fi, FORM_CHANGE, redraw))
            return true;
      }
      break;

   case S_html_form_input::SELECT_MULTI:
      {
                              //toggle active selection
         S_html_form_select_option &fsel = tdi.form_select_options[p_fi->form_select];
         S_html_form_select_option::S_option &opt = fsel.options[fsel.selection];
         opt.selected = !opt.selected;
         redraw = true;
      }
      break;

   case S_html_form_input::SELECT_SINGLE:
      mod.dropdown_select_expanded = !mod.dropdown_select_expanded;
      if(mod.dropdown_select_expanded){
         S_html_form_select_option &fsel = tdi.form_select_options[p_fi->form_select];
         if(!fsel.options.Size()){
                              //no options, don't display
            mod.dropdown_select_expanded = false;
            break;
         }
                              //init dropdown list
         const S_font &fd = font_defs[tdi.ts.font_index];
         int h = fd.line_spacing;
         S_rect &rc = mod.dropdown_select.rc;
         C_scrollbar &sb = mod.dropdown_select.sb;

                              //compute position of rectangle
         rc.sx = p_fi->sx - fd.line_spacing;
         rc.sy = h * fsel.options.Size();
         sb.total_space = fsel.options.Size();
         sb.visible_space = sb.total_space;
                              //limit height
         int max_h = tdi.rc.sy*7/8;
         if(rc.sy > max_h){
            sb.visible_space = max_h/h;
            rc.sy = sb.visible_space * h;
         }
         sb.SetVisibleFlag();
         //sb.visible = true;

         if(sb.visible)
            rc.sx += GetScrollbarWidth()+3;
                              //center on sceen
         rc.x = mod.rc.x + (mod.rc.sx-rc.sx)/2;
         rc.y = mod.rc.y + (mod.rc.sy-rc.sy)/2;
                              //init scrollbar
         sb.rc = S_rect(rc.Right()+1-(GetScrollbarWidth()+3), rc.y+2, GetScrollbarWidth(), rc.sy-4);
         mod.dropdown_select.selection = fsel.selected_option;
         mod.dropdown_select.EnsureVisible();
      }
      redraw = true;
      break;

#ifdef WEB
   case S_html_form_input::RESET:
      {
         const S_html_form &form = tdi.forms[p_fi->form_index];

         if(form.js_onreset.Length()){
            RunJavaScript(mod.js_data, form.js_onreset, form.js_onreset.Length());
         }
                              //reset form's values to initial state
         for(int i=0; i<tdi.form_inputs.size(); i++){
            S_html_form_input &fii = tdi.form_inputs[i];
            if(fii.form_index==p_fi->form_index){
               switch(fii.type){
               case fii.SUBMIT:
               case fii.IMAGE:
               case fii.RESET:
                  break;
               case fii.TEXT:
               case fii.PASSWORD:
               case fii.TEXTAREA:
                  fii.value = fii.init_value;
                  fii.edit_sel = fii.edit_pos = (short)fii.value.Length();
                  break;
               case fii.CHECKBOX:
               case fii.RADIO:
                  fii.checked = fii.init_checked;
                  break;
               case fii.SELECT_SINGLE:
               case fii.SELECT_MULTI:
                  {
                     S_html_form_select_option &fsel = tdi.form_select_options[fii.form_select];
                     if(fii.type==fii.SELECT_SINGLE)
                        fsel.selected_option = fsel.init_selected_option;
                     else{
                        for(int i=fsel.options.Size(); i--; )
                           fsel.options[i].selected = fsel.options[i].init_selected;
                     }
                  }
                  break;
               default:
                  assert(0);
               }
            }
         }
         redraw = true;
      }
      break;
#endif//WEB

   case S_html_form_input::TEXT:
   case S_html_form_input::PASSWORD:
   case S_html_form_input::TEXTAREA:
                              //toggle text editor
      if(!tdi.te_input){
         bool is_pass = p_fi->type==S_html_form_input::PASSWORD;
         tdi.te_input = CreateTextEditor((is_pass ? TXTED_SECRET : TXTED_ALLOW_PREDICTIVE), mod.text_info.ts.font_index, 0, NULL, p_fi->max_length);
         C_text_editor &te = *tdi.te_input;
         te.Release();
         if(!is_pass)
            te.SetCase(te.CASE_CAPITAL, te.CASE_CAPITAL);
         else
            te.SetCase(te.CASE_ALL, te.CASE_LOWER);
         te.SetInitText(p_fi->value);
         if(p_fi->type!=S_html_form_input::PASSWORD)
            te.SetCursorPos(p_fi->edit_pos, p_fi->edit_sel);
         //else
            //te.SetCursorPos(0, p_fi->value.Length());
         te.rc.sx = p_fi->sx;
         MakeSure CursorIsVisible(te);
      }else{
         p_fi->edit_pos = tdi.te_input->GetCursorPos();
         p_fi->edit_sel = tdi.te_input->GetCursorSel();
         tdi.te_input = NULL;
      }
      redraw = true;
      break;
      /*
      {
                              //check if default button should be checked
         int form_i = p_fi->form_index;
         S_html_form_input *fi = p_fi+1;
         p_fi = NULL;
         for(; fi<tdi.form_inputs.end() && fi->form_index==form_i; fi++){
            if(fi->type==S_html_form_input::SUBMIT){
               p_fi = fi;
               break;
            }
         }
         if(!p_fi)
            break;
      }
                              //flow...
      */

   case S_html_form_input::SUBMIT:
   case S_html_form_input::IMAGE:
      {
         const S_html_form &form = tdi.forms[p_fi->form_index];
#ifdef WEB
         if(mod.page_loader.last_requested_domain.Length()){
                                 //perform submit main action - collect contributors and open link

            Cstr_c action_url = form.action;
            Cstr_c http_domain; http_domain<<mod.page_loader.LastRequestedPrefix() <<mod.page_loader.last_requested_domain;
            int slash_i = mod.page_loader.curr_file.FindReverse('/');
            http_domain<<(slash_i==-1 ? mod.page_loader.curr_file : mod.page_loader.curr_file.Left(slash_i+1));
            text_utils::MakeFullUrlName(http_domain, action_url);
            Cstr_c post_string;
            for(int i=0; i<tdi.form_inputs.size(); i++){
               const S_html_form_input &fii = tdi.form_inputs[i];
               if(fii.form_index==p_fi->form_index){
                  switch(fii.type){
                  case fii.SUBMIT:
                  case fii.IMAGE:
                     if(!fii.name.Length())
                        break;
                     if(&fii!=p_fi)
                        break;
                                 //flow...
                  case fii.TEXT:
                  case fii.PASSWORD:
                  case fii.HIDDEN:
                  case fii.TEXTAREA:
                                 //fields with null values may be ommited (rfc1866)
                     if(fii.value.Length()){
                        if(post_string.Length())
                           post_string<<'&';
                        UrlEncodeString((const char*)fii.name, false, post_string);
                        post_string<<'=';
                        UrlEncodeString((const wchar*)fii.value, true, post_string);
                     }
                     break;
                  case fii.CHECKBOX:
                  case fii.RADIO:
                                 //unselected radio buttons and checkboxes should not appear in the encoded data
                     if(fii.checked && fii.value.Length()){
                        if(post_string.Length())
                           post_string<<'&';
                        UrlEncodeString((const char*)fii.name, false, post_string);
                        post_string<<'=';
                        UrlEncodeString((const wchar*)fii.value, true, post_string);
                     }
                     break;
                  case fii.SELECT_SINGLE:
                  case fii.SELECT_MULTI:
                     {
                        const S_html_form_select_option &fsel = tdi.form_select_options[fii.form_select];
                        for(int i=0; i<fsel.options.Size(); i++){
                           const S_html_form_select_option::S_option &opt = fsel.options[i];
                                 //check if will be added
                           bool add;
                           if(fii.type==fii.SELECT_SINGLE){
                                 //add selected option (single)
                              add = (fsel.selected_option==i);
                           }else{
                                 //add all selected options
                              add = opt.selected;
                           }
                           if(add && (opt.value.Length() || opt.display_name.Length())){
                              if(post_string.Length())
                                 post_string<<'&';
                              UrlEncodeString((const char*)fii.name, false, post_string);
                              post_string<<'=';
                              if(opt.value.Length())
                                 post_string<<opt.value;
                              else
                                 UrlEncodeString((const wchar*)opt.display_name, true, post_string);
                           }
                        }
                     }
                     break;
                  case fii.RESET:
                     break;
                  default:
                     assert(0);
                  }
               }
            }
            if(!form.use_post && post_string.Length())
               action_url<<'?' <<post_string;
            BrowserOpenURL(mod, action_url, 0, form.use_post ? (const char*)post_string : NULL);
            return true;
         }
#else
         if(mod.form_action && (this->*mod.form_action)(mod, p_fi, FORM_CLICK, redraw))
            return true;
#endif
      }
      break;
   }
   return false;
}

#endif
//----------------------------
//----------------------------
// Convert character defined by &name; or &#xx techniques to char.
// The 'cp' points to string after initial '&' character.
wchar ConvertNamedHtmlChar(const char *&cp){

   dword c = *cp;
   if(!c)
      return 0;
   if(c=='#'){
      ++cp;
      int num;
      if((ToLower(*cp)=='x' && (++cp, text_utils::ScanHexNumber(cp, num))) || text_utils::ScanDecimalNumber(cp, num)){
         c = num;
      }else{
         //assert(0);
      }
   }else{
      const char *val_cp = cp;
      Cstr_c val;
      if(text_utils::ReadWord(cp, val, " =()<>@,;:\\\".[]\x7f")){

#define CMAP(n, x) n"\0"x"\0"
#define CMAP_W(n, hi, lo) n"\0."lo hi"\0"
//#define CMAPX(n, x) n"\0."x"\x7f\0"
#define CMAPX(n, x) CMAP(n, x)
         static const char char_map[] =
                              //2.
            CMAP("quot", "\"")
            CMAP("amp", "&")
            CMAP("apos", "\'")
                              //3.
            CMAP("lt", "<")
            CMAP("gt", ">")
                              //a.
            CMAP("nbsp", "\xa0")
            CMAP("iexcl", "\xa1")
            CMAP("cent", "\xa2")
            CMAP("pound", "\xa3")
            CMAP("curren", "\xa4")
            CMAP("yen", "\xa5")
            CMAP("brvbar", "\xa6")
            CMAP("sect", "\xa7")
            CMAP("uml", "\xa8")
            CMAP("copy", "\xa9")
            CMAP("ordf", "\xaa")
            CMAP("laquo", "\xab")
            CMAP("not", "\xac")
            CMAP("shy", "\xad")
            CMAP("reg", "\xae")
            //macr    macron -->
                              //b.
            CMAP("deg", "\xb0")
            CMAP("plusmn", "\xb1")
            //sup2    superscript two -->
            //sup3    superscript three -->
            CMAP("acute", "\xb4")
            CMAP("micro", "\xb5")
            //para    pilcrow (paragraph sign) -->
            CMAP("middot", "\xb7")
            CMAP("cedil", "\xb8")
            //sup1    superscript one -->
            CMAP("ordm", "\xba")
            CMAP("raquo", "\xbb")
            CMAP("frac14", "\xbc")
            CMAP("frac12", "\xbd")
            CMAP("frac34", "\xbe")
            CMAP("iquest", "\xbf")
                              //c.
            CMAPX("Agrave", "\xc0")
            CMAPX("Aacute", "\xc1")
            CMAPX("Acirc", "\xc2")
            CMAPX("Atilde", "\xc3")
            CMAPX("Auml", "\xc4")
            CMAPX("Aring", "\xc5")
            CMAP("AElig", "\xc6")
            CMAPX("Ccedil", "\xc7")
            CMAPX("Egrave", "\xc8")
            CMAPX("Eacute", "\xc9")
            CMAPX("Ecirc", "\xca")
            CMAPX("Euml", "\xcb")
            CMAPX("Igrave", "\xcc")
            CMAPX("Iacute", "\xcd")
            CMAPX("Icirc", "\xce")
            CMAPX("Iuml", "\xcf")
                              //d.
            CMAP("ETH", "\xd0")
            CMAPX("Ntilde", "\xd1")
            CMAPX("Ograve", "\xd2")
            CMAPX("Oacute", "\xd3")
            CMAPX("Ocirc", "\xd4")
            CMAPX("Otilde", "\xd5")
            CMAPX("Ouml", "\xd6")
            CMAP("times", "\xd7")
            CMAP("Oslash", "\xd8")
            CMAPX("Ugrave", "\xd9")
            CMAPX("Uacute", "\xda")
            CMAPX("Ucirc", "\xdb")
            CMAPX("Uuml", "\xdc")
            CMAPX("Yacute", "\xdd")
            CMAPX("THORN", "\xde")
            CMAP("szlig", "\xdf")
                              //e.
            CMAPX("agrave", "\xe0")
            CMAPX("aacute", "\xe1")
            CMAPX("acirc", "\xe2")
            CMAPX("atilde", "\xe3")
            CMAPX("auml", "\xe4")
            CMAPX("aring", "\xe5")
            CMAP("aelig", "\xe6")
            CMAPX("ccedil", "\xe7")
            CMAPX("egrave", "\xe8")
            CMAPX("eacute", "\xe9")
            CMAPX("ecirc", "\xea")
            CMAPX("euml", "\xeb")
            CMAPX("igrave", "\xec")
            CMAPX("iacute", "\xed")
            CMAPX("icirc", "\xee")
            CMAPX("iuml", "\xef")
                              //f.
            CMAPX("eth", "\xf0")
            CMAPX("ntilde", "\xf1")
            CMAPX("ograve", "\xf2")
            CMAPX("oacute", "\xf3")
            CMAPX("ocirc", "\xf4")
            CMAPX("otilde", "\xf5")
            CMAPX("ouml", "\xf6")
            CMAP("divide", "\xf7")
            CMAP("oslash", "\xf8")
            CMAPX("ugrave", "\xf9")
            CMAPX("uacute", "\xfa")
            CMAPX("ucirc", "\xfb")
            CMAPX("uuml", "\xfc")
            CMAPX("yacute", "\xfd")
            CMAPX("thorn", "\xfe")
            CMAPX("yuml", "\xff")
                              //15.
            CMAP_W("OElig", "\x1", "\x52")
            CMAP_W("oelig", "\x1", "\x53")
                              //16.
            CMAP_W("Scaron", "\x1", "\x60")
            CMAP_W("scaron", "\x1", "\x61")
                              //17.
            CMAP_W("Yuml", "\x1", "\x78")
                              //2c.
            CMAP_W("circ", "\x2", "\xc6")
                              //2d.
            CMAP_W("tilde", "\x2", "\xdc")
                              //200.
            CMAP_W("ensp", "\x20", "\x02")
            CMAP_W("emsp", "\x20", "\x03")
            CMAP_W("thinsp", "\x20", "\x09")
            CMAP_W("zwnj", "\x20", "\x0c")
            CMAP_W("zwj", "\x20", "\x0d")
            CMAP_W("lrm", "\x20", "\x0e")
            CMAP_W("rlm", "\x20", "\x0f")
                              //201.
            CMAP_W("ndash", "\x20", "\x13")
            CMAP_W("mdash", "\x20", "\x14")
            CMAP_W("lsquo", "\x20", "\x18")
            CMAP_W("rsquo", "\x20", "\x19")
            CMAP_W("sbquo", "\x20", "\x1a")
            CMAP_W("ldquo", "\x20", "\x1c")
            CMAP_W("rdquo", "\x20", "\x1d")
            CMAP_W("bdquo", "\x20", "\x1e")
                              //202.
            CMAP("dagger", "+")  //0x2020
            CMAP("Dagger", "+")  //0x2021
            CMAP_W("bull", "\x20", "\x22")
            CMAP_W("hellip", "\x20", "\x26")
                              //203.
            CMAP_W("permil", "\x20", "\x30")
            CMAP_W("lsaquo", "\x20", "\x39")
            CMAP_W("rsaquo", "\x20", "\x3a")
                              //20a.
            CMAP_W("euro", "\x20", "\xac")

            CMAP_W("trade", "\x21", "\x22")
            ;
         const char *cm = char_map;
         while(*cm){
            int len = StrLen(cm);
            const byte *cx = (byte*)cm + len + 1;
            if(val==cm){
               c = *cx;
               if(c=='.'){
                        //wide-char string
                  ++cx;
                  c = cx[0] | (cx[1]<<8);
               }
               break;
            }
            cm = (char*)cx;
            if(*cx=='.')
               cm += 2;
            cm += 2;
         }
         if(!*cm){
                        //leave as is
            cp = val_cp;
            c = cp[-1];
         }
      }else
         c = '&';
   }
   if(*cp==';')
      ++cp;
   return wchar(c);
}

//----------------------------
#if defined MAIL || defined WEB || defined EXPLORER || defined WEB_FORMS || defined MESSENGER //|| defined MUSICPLAYER

static const char html_tags[] =
   "center\0"
   "left\0"
   "right\0"
   "i\0"
   "u\0"
   "tt\0"
   "big\0"
   "table\0"
   "div\0"
   "b\0"
   "html\0"
   "xml\0"
   "head\0"
   "body\0"
   "tbody\0"
   "!doctype\0"
   "!\0"
   "title\0"
   "meta\0"
   "base\0"
   "font\0"
   "em\0"
   "strong\0"
   "style\0"
   "tr\0"
   "td\0"
   "th\0"
   "br\0"
   "a\0"
   "p\0"
   "span\0"
   "img\0"
   "ul\0"
   "li\0"
   "ol\0"
   "hr\0"
   "select\0"
   "option\0"
   "input\0"
   "form\0"
   "map\0"
   "area\0"
   "script\0"
   "noscript\0"
   "link\0"
   "h1\0"
   "h2\0"
   "h3\0"
   "h4\0"
   "h5\0"
   "h6\0"
   "blockquote\0"
   "q\0"
   "textarea\0"
   "pre\0"
   "frameset\0"
   "frame\0"
   "iframe\0"
   "noframes\0"
   "address\0"
   "dl\0"
   "dt\0"
   "dd\0"
   "dir\0"
   "menu\0"
   "caption\0"
   "applet\0"
   "ins\0"
   "del\0"
   ;

enum E_HTML_TAG{
   TAG_UNKNOWN = -1,
                              //these styles have stack of states
   TAG_CENTER,
   TAG_LEFT,
   TAG_RIGHT,
   TAG_I,
   TAG_U,
   TAG_TT,
   TAG_BIG,
   TAG_TABLE,
   TAG_DIV,
   TAG_B,

   TAG_STACKED_TAG_LAST = TAG_B,

   TAG_HTML,
   TAG_XML,
   TAG_HEAD,
   TAG_BODY,
   TAG_TBODY,
   TAG_DOCTYPE,
   TAG_COMMENT,
   TAG_TITLE,
   TAG_META,
   TAG_BASE,
   TAG_FONT,
   TAG_EM,
   TAG_STRONG,
   TAG_STYLE,
   TAG_TR,
   TAG_TD,
   TAG_TH,
   TAG_BR,                    //line break
   TAG_A,                     //anchor
   TAG_P,                     //paragraph
   TAG_SPAN,
   TAG_IMG,
   TAG_UL,
   TAG_LI,                    //list item
   TAG_OL,
   TAG_HR,                    //horizontal rule
   TAG_SELECT,
   TAG_OPTION,
   TAG_INPUT,
   TAG_FORM,
   TAG_MAP,
   TAG_AREA,
   TAG_SCRIPT,
   TAG_NOSCRIPT,
   TAG_LINK,
   TAG_H1,                    //headers
   TAG_H2,
   TAG_H3,
   TAG_H4,
   TAG_H5,
   TAG_H6,
   TAG_BLOCKQUOTE,
   TAG_Q,
   TAG_TEXTAREA,
   TAG_PRE,
   TAG_FRAMESET,
   TAG_FRAME,
   TAG_IFRAME,
   TAG_NOFRAMES,
   TAG_ADDRESS,
   TAG_DL,
   TAG_DT,
   TAG_DD,
   TAG_DIR,
   TAG_MENU,
   TAG_CAPTION,
   TAG_APPLET,
   TAG_INS,
   TAG_DEL,
};

//----------------------------
struct S_font_tag_state{
   S_text_style ts;
   E_HTML_TAG tag;
   //dword bgnd_color, text_color;
   //int font_index;
   S_font_tag_state(){}
   S_font_tag_state(const S_text_style &ts1, E_HTML_TAG t):
      ts(ts1),
      tag(t)
   {}
};
//----------------------------
static const char tag_delimiters[] = " \t\n=()<>@-,;:\\\".[]\x7f";
//----------------------------
static bool StrICmp(const char *&str, const char *kw, const char *separators){

   const char *str1 = str;
   while(*kw){
      char c1 = ToLower(*str1++);
      char c2 = *kw++;
      if(c1!=c2)
         return false;
   }
                              //str must end with a good separator
   char c = *str1;
   if(c){
      while(*separators){
         char s = *separators++;
         if(c==s)
            break;
      }   
      if(!*separators)
         return false;
   }
   str = str1;
   return true;
}
//----------------------------

static int FindHTMLKeyword(const char *&str, const char *keywords, const char *separators){

   for(int i=0; *keywords; i++){
      if(StrICmp(str, keywords, separators))
         return i;
      keywords += StrLen(keywords) + 1;
   }
   return -1;
}

//----------------------------

static void FindTagEnd(const char *&cp){
                              //find tag end
   while(true){
      char c = *cp++;
      if(!c){
         --cp;
         return;
      }
      if(c=='>')
         break;
   }
}

//----------------------------

static bool FindMatchingEndTag(const char *&cp, E_HTML_TAG tag){

   while(true){
      char c = *cp++;
      if(!c){
         --cp;
         return false;
      }
      if(c=='<'){
         text_utils::SkipWSAndEol(cp);
         bool tag_end = false;
         if(*cp=='/'){
            tag_end = true;
            ++cp;
         }
         E_HTML_TAG t = (E_HTML_TAG)FindHTMLKeyword(cp, html_tags, tag_delimiters);
         if(t==tag && tag_end)
            break;
      }
   }
   return true;
}

//----------------------------

static void AddEolIfNotPresent(C_vector<char> &buf){

   if(buf.size() && buf.back()!='\n')
      buf.push_back('\n');
}

//----------------------------

static void SkipHtmlComment(const char *&cp){

   if(cp[0]=='-' && cp[1]=='-'){
                     //comment
      cp += 2;
      while(true){
         char c = *cp++;
         if(!c){
            --cp;
            break;
         }
         if(c=='-' && cp[0]=='-')
            break;
      }
   }
}

//----------------------------
// For characters which cannot be represented as themselves, convert to character to different character,
// or return multi-character string.
const char *GetMulticharString(dword &c){

                        //detect special characters, and write them as multi-character
   switch(c){
   case 0x80: c = 0x20ac; break;
   case 0x85: return "...";
   case 0x99: return "[TM]";
   //case 0xa0: c = ' '; break;
   case 0xa2: return "cent";
   case 0xa4: c = '*'; break; //currency
   case 0xa5: return "Yen";
   case 0xa9: return "(c)";
   case 0xab: return "<<";
   case 0xae: return "(R)";
   case 0xb1: return "+-";
   case 0xb4: c = '\''; break;
   case 0xb7: c = 0x95; break;
   case 0xb9: return "No";
   case 0xbb: return ">>";
   case 0xbc: return "1/4";
   case 0xbd: return "1/2";
   case 0xbe: return "3/4";

   case 0x152: return "OE";
   case 0x153: return "oe";
   case 0x2c6: c = '^'; break;

   case 0x2013: c = '-'; break;
   case 0x2014: c = '-'; break;
   case 0x2019: c = '\''; break;
   case 0x2026: return "...";
   case 0x2116: return "No";
   }
   return NULL;
}

//----------------------------
// Convert html string into other string.
void C_client_viewer::ConvertHtmlString(const char *_cp, E_TEXT_CODING coding, Cstr_w *dst_w, Cstr_c *dst_c){

   Cstr_c src;
                              //decode &-encoded characters
   while(*_cp){
      char c = *_cp++;
      if(c=='&'){
         wchar c1 = ConvertNamedHtmlChar(_cp);
         if(c1){
            if(coding == COD_UTF_8){
               wchar tmp[2] = {c1, 0};
               Cstr_c ss; ss.ToUtf8(tmp);
               src<<ss;
            }else{
               if(c1<' ' || c1>=256)
                  c1 = '?';
               src<<c1;
            }
         }
      }else
      if(byte(c)>=' ' || c=='\n' || c=='\t')
         src<<c;
      else
         assert(0);
   }

   C_vector<wchar> buf;
   if(coding == COD_UTF_8){
                              //decode utf-8 text
      buf.reserve(src.Length());
      Cstr_w tmp;
      tmp.FromUtf8(src);
      buf.insert(buf.begin(), (const wchar*)tmp, (const wchar*)tmp+tmp.Length());
   }else{
      buf.reserve(src.Length()*2);
      const char *cp = src;
      while(*cp){
         dword c1 = byte(*cp++);
         if(coding==COD_DEFAULT){
            const char *spec = GetMulticharString(c1);
            if(spec){
               while(*spec)
                  buf.push_back(*spec++);
            }else
               buf.push_back(wchar(c1));
         }else{
            c1 = encoding::ConvertCodedCharToUnicode(c1, coding);
            buf.push_back(wchar(c1));
         }
      }
   }
   if(dst_w){
      buf.push_back(0);
      *dst_w = buf.begin();
   }else{
      dst_c->Allocate(NULL, buf.size());
      for(int i=0; i<buf.size(); i++){
         wchar c1 = buf[i];
         if(c1>=256)
            c1 = '?';
         dst_c->At(i) = char(c1);
      }
   }
}

//----------------------------

static void ParseStyle(const char *cp, S_text_style &ts, bool preview_mode){

   while(*cp){
      text_utils::SkipWSAndEol(cp);
      Cstr_c attr, val;
      if(!text_utils::ReadToken(cp, attr, " :"))
         break;
      attr.ToLower();
      text_utils::SkipWSAndEol(cp);
      if(*cp!=':'){
         //assert(0);
         break;
      }
      text_utils::SkipWSAndEol(++cp);
      if(!text_utils::ReadToken(cp, val, ";")){
         //assert(0);
         break;
      }
      text_utils::SkipWSAndEol(cp);
      if(*cp==';')
         ++cp;
      /*
      if(!ReadQuotedString(cp, val)){
         if(!ReadQuotedString(cp, val, '\'')){
            if(!ReadToken(cp, val, " >"))
               return false;
         }
      }
      */
      if(attr=="background-color"){
         if(!preview_mode){
            if(ReadHTMLColor(val, ts.bgnd_color))
               ts.bgnd_color |= 0xff000000;
         }
      }else if(attr=="background"){
         if(!preview_mode){
            if(ReadHTMLColor(val, ts.bgnd_color))
               ts.bgnd_color |= 0xff000000;
         }
      }else if(attr=="color"){
         if(!preview_mode){
            if(ReadHTMLColor(val, ts.text_color))
               ts.text_color |= 0xff000000;
         }
      }else if(attr=="font-weight"){
         if(val=="normal" || val=="lighter")
            ts.font_flags &= ~FF_BOLD;
         else if(val=="bold")
            ts.font_flags |= FF_BOLD;
      }
      /*
#ifdef _DEBUG
      else if(attr=="text-align"){
      }else if(attr=="line-height"){
      }else if(attr=="width"){
      }else if(attr=="padding-top"){
      }else if(attr=="padding-bottom"){
      }else if(attr=="padding-left"){
      }else if(attr=="padding-right"){
      }else if(attr=="padding"){
      }else if(attr=="border"){
      }else if(attr=="margin"){
      }else if(attr=="margin-left"){
      }else if(attr=="margin-right"){
      }else if(attr=="margin-top"){
      }else if(attr=="margin-bottom"){
      }else if(attr=="cursor"){
      }else if(attr=="display"){
      }else if(attr=="float"){
      }else if(attr=="clear"){
      }else if(attr=="font"){
      }else if(attr=="font-size"){
      }else if(attr=="font-style"){
      }else if(attr=="font-variant"){
      }else if(attr=="font-size-adjust"){
      }else if(attr=="font-stretch"){
      }else if(attr=="border-left"){
      }else if(attr=="border-right"){
      }else if(attr=="border-top"){
      }else if(attr=="border-bottom"){
      }else if(attr=="font-family"){
      }else if(attr=="direction"){
      }else if(attr=="unicode-bidi"){
      }else if(attr=="mso-margin-left-alt"){
      }else if(attr=="mso-margin-right-alt"){
      }else if(attr=="mso-margin-top-alt"){
      }else if(attr=="mso-margin-bottom-alt"){
#endif
      }else
         assert(0);
      */
   }
}

//----------------------------

static void PopSavedState(C_vector<S_font_tag_state> &saved_state_font, E_HTML_TAG tag, S_text_style &ts){

   for(int i=saved_state_font.size(); i--; ){
      if(saved_state_font[i].tag==tag){
         ts = saved_state_font[i].ts;
         saved_state_font.resize(i);
         break;
      }
   }
}

//----------------------------
/*
void text_utils::StripTrailingSpace(C_vector<wchar> &buf){

   while(buf.size()){
      wchar c = buf.back();
      if(!(c==' ' || c=='\t'))
         break;
      buf.pop_back();
   }
}
*/
//----------------------------
// Skip trailing spaces from character buffer.
static void StripTrailingSpace(C_vector<char> &buf){

   while(buf.size()){
      char c = buf.back();
      if(!(c==' ' || c=='\t'))
         break;
      buf.pop_back();
   }
}

//----------------------------

void C_client_viewer::CompileHTMLText(const char *cp, dword src_size, S_text_display_info &tdi, S_file_open_help &oh) const{

   if(!oh.append)
      tdi.Clear();
   bool is_space = true;
   const char *www_domain = oh.www_domain;

   Cstr_c base_domain;        //read from <base> tag
   Cstr_c coded_title;        //title in default coding
   S_text_style ts_last;
   S_text_style ts;
   ts_last.font_index = ts.font_index = config.viewer_font_index;
   C_vector<dword> saved_state[TAG_STACKED_TAG_LAST+1];
   C_vector<S_font_tag_state> saved_state_font;
   C_vector<int> list;        //ordered or unordered list (OL, UL tags); the value represents index of entry for OL, or 0 for UL
   //dword tag_saved_align = 0;
   const int LIST_TAB = 2;
   bool in_href = false;
   bool is_preformated = false;
   bool has_default_coding = (tdi.body_c.coding==COD_DEFAULT);
   int block_quote_count = 0;

#ifdef WEB_FORMS
   bool in_form = false;
   bool script_type_is_js = true;
   bool use_js = true;
   //use_js = (oh.js_data!=NULL && (config.flags&config.CONF_USE_JAVASCRIPT));
#ifdef WEB
   Cstr_c js_on_body_onload;
#endif

   if(has_default_coding)
      tdi.body_c.coding = COD_UTF_8;
#endif//WEB_FORMS
   C_smart_ptr<C_zip_package> dta;

   C_vector<char> dst_buf;
   dst_buf.reserve(src_size);

   while(true){
      dword c = byte(*cp++);
      if(!c)
         break;
      if(c=='<'){
         text_utils::SkipWSAndEol(cp);
         bool tag_end = false;
         if(*cp=='/'){
            tag_end = true;
            ++cp;
         }
                              //tag, read what it is
         E_HTML_TAG tag = (E_HTML_TAG)FindHTMLKeyword(cp, html_tags, tag_delimiters);

         Cstr_c attr, val;
         switch(tag){
         case TAG_HEAD:
         case TAG_DOCTYPE:
         case TAG_HTML:
         //case TAG_SPAN:
            break;

         case TAG_DEL:
                              //deleted text - do not render at all
            if(!tag_end)
               FindMatchingEndTag(cp, tag);
            break;

         case TAG_PRE:
            if(!tag_end){
               is_preformated = true;
                              //remove eol immediately following start tag
               FindTagEnd(cp);
               text_utils::SkipWS(cp);
               if(*cp=='\r')
                  ++cp;
               if(*cp=='\n')
                  ++cp;
               continue;
            }else{
               is_preformated = false;
               AddEolIfNotPresent(dst_buf);
               dst_buf.push_back('\n');
            }
            break;

         case TAG_BASE:
            if(tag_end)
               break;
            while(text_utils::GetNextAtrrValuePair(cp, attr, val)){
               if(attr=="href"){
                              //read base domain
                  if(val.Length() && val[val.Length()-1]!='/')
                     val<<'/';
                  base_domain = val;
                  www_domain = base_domain;
                  if(!text_utils::CheckStringBegin(www_domain, text_utils::HTTP_PREFIX))
                     www_domain = NULL;
               }
                              //other attribs: target (used with framesets)
            }
            break;

         case TAG_TABLE:
            if(!tag_end){
               saved_state_font.push_back(S_font_tag_state(ts, tag));
               while(text_utils::GetNextAtrrValuePair(cp, attr, val)){
                  if(attr=="bgcolor"){
                     if(!oh.preview_mode)
                     if(ReadHTMLColor(val, ts.bgnd_color))
                        ts.bgnd_color |= 0xff000000;
                  }
#ifdef _DEBUG
                  else
                  if(attr=="align"){
                  }else
                  if(attr=="width"){
                  }else
                  if(attr=="border" || attr=="cellspacing" || attr=="cellpadding"){
                  }
#endif
               }
            }else{
               PopSavedState(saved_state_font, tag, ts);
               AddEolIfNotPresent(dst_buf);
            }
            break;

         case TAG_TD:
         case TAG_TH:
         case TAG_TR:
            if(!tag_end){
               if(saved_state_font.size() && saved_state_font.back().tag==tag)
                  ts = saved_state_font.back().ts;
               else{
                  saved_state_font.push_back(S_font_tag_state(ts, tag));
               }
               while(text_utils::GetNextAtrrValuePair(cp, attr, val)){
                  if(attr=="bgcolor"){
                     if(!oh.preview_mode)
                     if(ReadHTMLColor(val, ts.bgnd_color))
                        ts.bgnd_color |= 0xff000000;
                  }else
                  if(attr=="style"){
                     ParseStyle(val, ts, oh.preview_mode);
                  }
                              //todo: read other attibs (nowrap, rowspan=n, colspan=n, align=left/right/center, valign=top/middle/bottom, width=pixels, height=pixels)
               }
               if(tag==TAG_TR){
                  AddEolIfNotPresent(dst_buf);
               }
            }else{
               PopSavedState(saved_state_font, tag, ts);
            }
            break;

         case TAG_COMMENT:
            SkipHtmlComment(cp);
            break;

         case TAG_TBODY:
            break;

         case TAG_ADDRESS:
            AddEolIfNotPresent(dst_buf);
            break;

         case TAG_BLOCKQUOTE:
            AddEolIfNotPresent(dst_buf);
            if(!tag_end){
               ++block_quote_count;
            }else{
               if(block_quote_count)
                  --block_quote_count;
            }
            ts.block_quote_count = (byte)Min(block_quote_count, 5);
            break;

         case TAG_Q:
            dst_buf.push_back('\"');
            break;

         case TAG_META:
            //if(tdi.body.coding==COD_DEFAULT)
            {
               Cstr_c content, equiv;
               while(text_utils::GetNextAtrrValuePair(cp, attr, val)){
                  if(attr=="http-equiv")
                     equiv = val;
                  else
                  if(attr=="content")
                     content = val;
                  /*
#ifdef _DEBUG
                  else
                  if(attr=="name"){
                  }else
                  if(attr=="lang"){
                  }else
                     assert(0);
#endif
                  */
               }
               if(equiv=="content-type"){
                  const char *cp1 = content;
                  S_content_type cc;
                  E_TEXT_CODING cod;
                  cc.ParseWithCoding(cp1, cod);
                  if(has_default_coding)
                     tdi.body_c.coding = cod;
               }
#ifdef WEB
               else
               if(equiv=="refresh"){
                  const char *cp = val;
                  if(text_utils::ScanDecimalNumber(cp, tdi.refresh_delay)){
                     text_utils::SkipWS(cp);
                     tdi.refresh_delay *= 1000;
                     if(*cp++==';'){
                        text_utils::SkipWS(cp);
                        if(CheckStringBeginWs(cp, "url")){
                           if(*cp++=='='){
                              tdi.refresh_link = cp;
                           }
                        }
                     }
                  }
               }else
               if(equiv=="content-script-type"){
                  content.ToLower();
                  if(content=="text/javascript")
                     script_type_is_js = true;
                  else{
                     script_type_is_js = false;
                     assert(0);
                  }
               }
#endif//WEB
            }
            break;

         case TAG_BODY:
            while(text_utils::GetNextAtrrValuePair(cp, attr, val, false)){
               if(attr=="background"){
                              //specifies a URL for an image that will be used to tile the document background.
                  if(!oh.preview_mode && val.Length()){
                     S_image_record img_rec;
                     img_rec.src = val;
                     tdi.bgnd_image = tdi.images.size();
                     tdi.images.push_back(img_rec);
                  }
               }else
               if(attr=="bgcolor"){
                           //Specifies the background color for the document body.
                  if(!oh.preview_mode)
                     ReadHTMLColor(val, tdi.bgnd_color);
               }else
               if(attr=="text"){
                  if(!oh.preview_mode){
                     val.ToLower();
                     ReadHTMLColor(val, ts.text_color);
                  }
               }else
               if(attr=="link"){
                  val.ToLower();
                  ReadHTMLColor(val, tdi.link_color);
               }else
               if(attr=="alink"){
                  val.ToLower();
                  ReadHTMLColor(val, tdi.alink_color);
               }else
               if(attr=="vlink"){
               }else
               if(attr=="style"){
                  ParseStyle(val, ts, oh.preview_mode);
               }
#ifdef WEB
               else
               if(attr=="onload"){
                  if(use_js) js_on_body_onload = val;
               }else
               if(attr=="onunload"){
                  if(use_js) tdi.js.on_body_unload = val;
               }
#endif
#ifdef _DEBUG
               else
               if(attr=="topmargin" || attr=="bottommargin" || attr=="leftmargin" || attr=="rightmargin" || attr=="marginwidth" || attr=="marginheight"){
               }else
               if(attr=="onclick" || attr=="lang" || attr=="bgproperties" || attr=="id" || attr=="scroll" || attr=="orgxpos" || attr=="orgypos" || attr=="sigcolor"){
               }else
               if(attr[0]=='x' && attr[1]=='-'){
               }
               //else assert(0);
#endif
            }
            break;

         case TAG_EM:
         case TAG_I:
         case TAG_STRONG:
         case TAG_B:
         case TAG_U:
         case TAG_TT:
         case TAG_BIG:
         case TAG_CENTER:     //equivalent of <div align=center>
         case TAG_LEFT:
         case TAG_RIGHT:
            {
               dword flag = 0;
               switch(tag){
               case TAG_EM: tag = TAG_I;
               case TAG_I:
                  flag = FF_ITALIC;
                  break;
               case TAG_BIG:
               case TAG_STRONG:
                  tag = TAG_B;
                              //flow...
               case TAG_B:
                  flag = FF_BOLD;
                  break;
               case TAG_U: flag = FF_UNDERLINE; break;
               case TAG_CENTER:
                  flag = FF_CENTER;
                              //closing center also makes line break
                  if(tag_end)
                     dst_buf.push_back('\n');
                  break;
               case TAG_LEFT: 
                  //assert(0);
                  break;
               case TAG_RIGHT: flag = FF_RIGHT; break;
               }
               C_vector<dword> &stk = saved_state[tag];
               if(!tag_end){
                  stk.push_back(ts.font_flags&flag);
                  ts.font_flags |= flag;
               }else{
                  if(stk.size()){
                     ts.font_flags &= ~flag;
                     ts.font_flags |= stk.back();
                     stk.pop_back();
                  }
               }
            }
            break;

         case TAG_FONT:
            if(!tag_end){
               saved_state_font.push_back(S_font_tag_state(ts, tag));
               int font_size = 3;
               while(text_utils::GetNextAtrrValuePair(cp, attr, val)){
                  if(attr=="color"){
                     if(!oh.preview_mode){
                        val.ToLower();
                        ReadHTMLColor(val, ts.text_color);
                     }
                  }else
                  if(attr=="size"){
                     const char *cp1 = val;
                     text_utils::ScanDecimalNumber(cp1, font_size);
                  }else
                  if(attr=="style"){
                     ParseStyle(val, ts, oh.preview_mode);
                  }
#ifdef _DEBUG
                  else
                  if(attr=="ptsize"){
                  }else
                  if(attr=="face"){
                  }else
                  if(attr=="class" || attr=="id"){
                  }else{
                     //assert(0);
                  }
#endif
               }
               const int n = !config.viewer_font_index ? 5 : 2;
               if(font_size<n)
                  ts.font_index = 0;
               else{
                  ts.font_index = 1;
               }
            }else{
               if(saved_state_font.size()){
                  ts = saved_state_font.back().ts;
                  saved_state_font.pop_back();
               }
            }
            break;

         case TAG_H1:
         case TAG_H2:
         case TAG_H3:
         case TAG_H4:
         case TAG_H5:
         case TAG_H6:
            if(!tag_end){
               while(text_utils::GetNextAtrrValuePair(cp, attr, val)){
#ifdef _DEBUG
                  if(attr=="align"){
                  }else
                  if(attr=="id" || attr=="class" || attr=="style" || attr=="xmlns"){
                  }else
                     assert(0);
#endif
               }
            }else{
            }
            break;

         case TAG_P:
         case TAG_SPAN:
            {
                              //the paragraph element requires a start tag, but the end tag can always be omitted
                              // use the ALIGN attribute to set the text alignment within a paragraph, e.g. <P ALIGN=RIGHT>
               if(tag==TAG_P)
                  StripTrailingSpace(dst_buf);
               ts.font_flags &= ~(FF_CENTER|FF_RIGHT);
               if(!tag_end){
                  saved_state_font.push_back(S_font_tag_state(ts, tag));

                  dword flag = 0;
                  while(text_utils::GetNextAtrrValuePair(cp, attr, val)){
                     if(attr=="align"){
                        if(val=="center")
                           flag |= FF_CENTER;
                        else
                        if(val=="right")
                           flag |= FF_RIGHT;
                     }else if(attr=="style"){
                        ParseStyle(val, ts, oh.preview_mode);
                     }
   #ifdef _DEBUG
                     else
                     if(attr=="class"){
                     }
                     //else assert(0);
   #endif
                  }
                  //tag_saved_align = ts.font_flags&(FF_CENTER|FF_RIGHT);
                  ts.font_flags |= flag;

                  if(tag==TAG_P){
                     AddEolIfNotPresent(dst_buf);
                     if(dst_buf.size()) dst_buf.push_back('\n');
                     is_space = true;
                  }
               }else{
                  //ts.font_flags |= tag_saved_align;
                  //tag_saved_align = 0;
                  if(saved_state_font.size()){
                     ts = saved_state_font.back().ts;
                     saved_state_font.pop_back();
                  }
                  if(tag==TAG_P){
                     AddEolIfNotPresent(dst_buf);
                     //dst_buf.push_back('\n');
                     is_space = true;
                  }
               }
            }
            break;

         case TAG_DL:
            break;

         case TAG_DT:
            AddEolIfNotPresent(dst_buf);
            break;

         case TAG_DD:
            {
               AddEolIfNotPresent(dst_buf);
               for(int i=0; i<3; i++)
                  dst_buf.push_back(' ');
            }
            break;

         case TAG_DIV:
            /*
            if(tag_end){
               StripTrailingSpace(dst_buf);
               dst_buf.push_back('\n');
               is_space = true;
            }
            */
            AddEolIfNotPresent(dst_buf);
            {
               C_vector<dword> &stk = saved_state[tag];
               if(!tag_end){
                  saved_state_font.push_back(S_font_tag_state(ts, tag));
                  dword flag = 0;
                  while(text_utils::GetNextAtrrValuePair(cp, attr, val)){
                     if(attr=="align"){
                        if(val=="center")
                           flag = FF_CENTER;
                        else
                        if(val=="right")
                           flag = FF_RIGHT;
                     }else if(attr=="style"){
                        ParseStyle(val, ts, oh.preview_mode);
                     }

#ifdef _DEBUG
                     //else assert(0);
#endif
                  }
                  stk.push_back(ts.font_flags&FF_CENTER);
                  ts.font_flags |= flag;
               }else{
                  PopSavedState(saved_state_font, tag, ts);
                  if(stk.size()){
                     ts.font_flags &= ~(FF_CENTER|FF_RIGHT);
                     ts.font_flags |= stk.back();
                     stk.pop_back();
                  }
               }
            }
            break;

         case TAG_BR:
            StripTrailingSpace(dst_buf);
            dst_buf.push_back('\n');
            is_space = true;
            break;

         case TAG_NOFRAMES:
            if(!tag_end)
               FindMatchingEndTag(cp, tag);
            break;

         case TAG_FRAMESET:
#ifdef WEB
            while(text_utils::GetNextAtrrValuePair(cp, attr, val, false)){
               if(attr=="onload"){
                  if(use_js)
                     js_on_body_onload = val;
               }else
               if(attr=="onunload"){
                  if(use_js)
                     tdi.js.on_body_unload = val;
               }
            }
#endif//WEB
            break;

         case TAG_IFRAME:
            break;

         case TAG_FRAME:
            if(!tag_end){
               S_file_open_help::S_frame frm;
               while(text_utils::GetNextAtrrValuePair(cp, attr, val, false)){
                  if(attr=="src")
                     frm.src = val;
                  else
                  if(attr=="name")
                     frm.name = val;
#ifdef _DEBUG
                              //other attribs: longdesc, noresize, scrolling=auto/yes/no, frameborder=1/0, marginwidth=pixels, marginheight=pixels
                  //else
                     //assert(0);
#endif
               }
               if(frm.src.Length()){
                  Cstr_c link = frm.src;
                  if(www_domain)
                     text_utils::MakeFullUrlName(www_domain, link);
#if defined WEB && 1
                  frm.src = link;
                  oh.frames.push_back(frm);
#else
                              //add text with hyperlink
                  S_hyperlink href;
                  href.link = link;
                  AddEolIfNotPresent(dst_buf);
                  dword ff = ts.font_flags;

                  tdi.BeginHyperlink(dst_buf, href);
                  ts.font_flags |= FF_UNDERLINE | FF_BOLD;

                  ts_last.WriteAndUpdate(ts, dst_buf);

                  dst_buf.insert(dst_buf.end(), frm.src, (const char*)frm.src+frm.src.Length());

                  tdi.EndHyperlink(dst_buf);

                  ts.font_flags = ff;
#endif
               }
            }
            break;

         case TAG_IMG:
            if(!oh.preview_mode){
               int img_sx=-1, img_sy=-1;
               Cstr_c img_src;
               Cstr_w img_alt;
               S_image_record::t_align_mode align_mode = 0;
                              //border is by default '1' when in hyperlink, otherwise there's no
               int border = in_href ? 1 : 0;
               int num_hspaces = 0;

               while(text_utils::GetNextAtrrValuePair(cp, attr, val, false)){
                  const char *cp2 = val;
                  if(attr=="width"){
                     text_utils::ScanDecimalNumber(cp2, img_sx);
                  }else
                  if(attr=="height"){
                     text_utils::ScanDecimalNumber(cp2, img_sy);
                  }else
                  if(attr=="src"){
                     ConvertHtmlString(val, COD_DEFAULT, NULL, &img_src);
                  }else
                  if(attr=="alt"){
                     ConvertHtmlString(val, tdi.body_c.coding, &img_alt, NULL);
                  }else
                  if(attr=="align" || attr=="valign"){
                     val.ToLower();
                     if(val=="top")
                        align_mode = S_image_record::ALIGN_V_TOP;
                     else
                     if(val=="middle" || val=="absmiddle")
                        align_mode = S_image_record::ALIGN_V_CENTER;
                     else
                     if(val=="bottom" || val=="absbottom")
                        align_mode = S_image_record::ALIGN_V_BOTTOM;
#ifdef _DEBUG_
                     else
                     if(val=="left" || val=="right" || val=="2" || val=="center" || val=="baseline" || val==""){
                     }else
                        assert(0);
#endif
                  }else
                  if(attr=="border"){
                     const char *cp1 = val;
                     text_utils::ScanDecimalNumber(cp1, border);
                  }else
                  if(attr=="hspace"){
                     const char *cp1 = val;
                     int space;
                     if(text_utils::ScanDecimalNumber(cp1, space)){
                        //space = C_fixed(space)*config.img_ratio;
                        int space_width = (font_defs[ts.font_index].cell_size_x+1)/2;
                        num_hspaces = (space+space_width/2) / space_width;
                     }
                  }
#ifdef _DEBUG
                  else
                  if(attr=="vspace"){
                  }else
                  if(attr=="usemap"){
                  }else
                  if(attr=="ismap"){
                  }else
                  if(attr=="v:shapes"){
                  }else
                  if(attr=="nosend"){
                  }else
                  if(attr=="style"){
                     //assert(0);
                  }else
                  if(attr=="title" || attr=="id" || attr=="u1:shapes" || attr=="class" || attr=="v:src" || attr=="name"){
                  }else{
                     //assert(0);
                  }
#endif
               }
               if(!img_src.Length())
                  break;

               ts_last.WriteAndUpdate(ts, dst_buf);
               int i;
               for(i=0; i<num_hspaces; i++)
                  dst_buf.push_back(' ');

               if(in_href){
                  tdi.hyperlinks.back().image_index = tdi.images.size();
               }else{
                  S_hyperlink href;
                  href.image_index = tdi.images.size();
                  tdi.BeginHyperlink(dst_buf, href);
               }
               InitHtmlImage(img_src, img_alt, img_sx, img_sy, www_domain, tdi, dst_buf, align_mode, border);

               if(!in_href)
                  tdi.EndHyperlink(dst_buf);
               for(i=0; i<num_hspaces; i++)
                  dst_buf.push_back(' ');
               /*
               const char *cp = img_src;
               if(text_utils::CheckStringBegin(cp, "dta://")){
                  S_image_record &irc = tdi.images[ii];
                  if(!dta)
                     dta = CreateDtaFile();
                  C_image *img = C_image::Create();
                  Cstr_w fn; fn.Copy(cp);
                  if(C_image::IMG_OK==img->Open(fn, irc.sx, irc.sy, dta)){
                     irc.sx = img->SizeX();
                     irc.sy = img->SizeY();
                     irc.img = img;
                  }
                  img->Release();
               }
               */
            }
            break;

         case TAG_A:
            if(!oh.preview_mode){
               if(!tag_end){
                  Cstr_c bookmark;
                  S_hyperlink href;
                  while(text_utils::GetNextAtrrValuePair(cp, attr, val, false)){
                     if(attr=="href"){
                        ConvertHtmlString(val, COD_DEFAULT, NULL, &href.link);
                     }else
                     if(attr=="name"){
                        bookmark = val;
                     }
#ifdef WEB_FORMS
                     else
                     if(use_js)
                        href.ParseAttribute(attr, val);
#endif
                     /*
#ifdef _DEBUG
                     else
                     if(attr=="target" || attr=="class"){
                     }else
                     if(attr=="linkarea" || attr=="linkid" || attr=="newsid" || attr=="id" || attr=="style" || attr=="title"){
                     }else
                        assert(0);
#endif
                        */
                  }
                  if(href.link.Length() && !in_href){
                     if(href.link[0]=='#')
                        href.link.ToLower();
                     else
                     if(www_domain)
                        text_utils::MakeFullUrlName(www_domain, href.link);

                     tdi.BeginHyperlink(dst_buf, href);
                     ts.font_flags |= FF_UNDERLINE;
                     in_href = true;
                  }
                  if(bookmark.Length()){
                     tdi.bookmarks.push_back(S_text_display_info::S_bookmark());
                     S_text_display_info::S_bookmark &b = tdi.bookmarks.back();
                     bookmark.ToLower();
                     b.name = bookmark;
                     b.byte_offset = dst_buf.size();
                  }
               }else{
                  if(in_href){
                     tdi.EndHyperlink(dst_buf);
                     in_href = false;
                     ts.font_flags &= ~FF_UNDERLINE;
                  }
               }
            }
            break;

#ifdef WEB_FORMS
         case TAG_FORM:
            if(!tag_end){
               tdi.forms.push_back(S_html_form());
               S_html_form &f = tdi.forms.back();
               while(text_utils::GetNextAtrrValuePair(cp, attr, val, false)){
                  if(attr=="action"){
                     f.action = val;
                  }else
                  if(attr=="name"){
                     //f.name = val;
                  }else
                  if(attr=="method"){
                     val.ToLower();
                     if(val=="get"){
                              //default
                     }else
                     if(val=="post")
                        f.use_post = true;
                     else
                        assert(0);
                  }else
                  if(attr=="enctype"){
                     if(val=="application/x-www-form-urlencoded"){
                     }else
                        assert(0);
                  }else
                  if(attr=="onsubmit"){
                     if(use_js) f.js_onsubmit = val;
                  }else
                  if(attr=="onreset"){
                     if(use_js) f.js_onreset = val;
                  }
#ifdef _DEBUG
                  else
                  if(attr=="target"){
                  }else
                  if(attr=="style"){
                  }else
                  if(attr=="id"){
                  }else
                  if(attr=="class" || attr=="accept-charset"){
                  }else
                     assert(0);
#endif
               }
               in_form = true;
            }else{
               in_form = false;
            }
            break;

         case TAG_INPUT:
         case TAG_SELECT:
         case TAG_TEXTAREA:
            if(in_form && !tag_end){
               S_html_form_input fi;
               if(tag==TAG_SELECT)
                  fi.type=fi.SELECT_SINGLE;
               else
               if(tag==TAG_TEXTAREA)
                  fi.type=fi.TEXTAREA;
               fi.form_index = tdi.forms.size() - 1;
               int img_sx=-1, img_sy=-1;
               Cstr_c img_src;
               Cstr_w img_alt;
               int form_size = 10;
               int rows = 10, cols = 3;

               while(text_utils::GetNextAtrrValuePair(cp, attr, val, false)){
                  const char *cp = val;
                  if(attr=="type"){
                     val.ToLower();
                     if(tag==TAG_INPUT){
                        if(val=="text") fi.type = fi.TEXT;
                        else if(val=="password") fi.type = fi.PASSWORD;
                        else if(val=="checkbox") fi.type = fi.CHECKBOX;
                        else if(val=="radio") fi.type = fi.RADIO;
                        else if(val=="image") fi.type = fi.IMAGE;
                        else if(val=="hidden") fi.type = fi.HIDDEN;
                        else if(val=="submit") fi.type = fi.SUBMIT;
                        else if(val=="reset"){
                           fi.type = fi.RESET;
                           fi.value = L"Reset";//GetText(TXT_RESET);
                        }else if(val=="button") fi.type = fi.SUBMIT;
                        //else if(val=="textarea") fi.type = fi.TEXTAREA;
                        else if(val=="file") fi.type = fi.FILE;
                        else
                           assert(0);
                     }
                  }else
                  if(attr=="name"){
                     fi.name = val;
                  }else
                  if(attr=="value"){
                     ConvertHtmlString(val, tdi.body_c.coding, &fi.value, NULL);
                     fi.init_value = fi.value;
                  }else
                  if(attr=="checked"){
                     fi.checked = fi.init_checked = true;
                  }else
                  if(attr=="maxlength"){
                     text_utils::ScanDecimalNumber(cp, fi.max_length);
                  }else
                  if(attr=="rows"){
                     text_utils::ScanDecimalNumber(cp, rows);
                  }else
                  if(attr=="cols"){
                     text_utils::ScanDecimalNumber(cp, cols);
                  }else
                  if(attr=="size"){
                     text_utils::ScanDecimalNumber(cp, form_size);
                  }else
                  if(attr=="width"){
                     text_utils::ScanDecimalNumber(cp, img_sx);
                  }else
                  if(attr=="height"){
                     text_utils::ScanDecimalNumber(cp, img_sy);
                  }else
                  if(attr=="src"){
                     ConvertHtmlString(val, COD_DEFAULT, NULL, &img_src);
                  }else
                  if(attr=="alt"){
                     ConvertHtmlString(val, COD_DEFAULT, &img_alt, NULL);
                  }else
                  if(attr=="multiple"){
                     if(tag==TAG_SELECT)
                        fi.type=fi.SELECT_MULTI;
                  }else
                  if(attr=="disabled"){
                     fi.disabled = true;
                  }else
                  if(attr=="id"){
                     fi.id = val;
                  }
#ifdef WEB_FORMS
                  else
                  if(use_js)
                     fi.ParseAttribute(attr, val);
#endif//WEB_FORMS
#ifdef _DEBUG
                  else
                  if(attr=="class" || attr=="align" || attr=="tabindex" || attr=="border" ||
                     attr=="vspace" || attr=="hspace" || attr=="autocomplete" || attr=="/" ||
                     attr=="onchange" ||
                     attr=="style" || attr=="accesskey" || attr=="readonly" || attr=="maxsize" ||
                     attr=="title" || attr=="wrap" || attr=="onfocus"){
                  }else
                     assert(0);
#endif
               }
               if(tag==TAG_SELECT){
                  FindTagEnd(cp);
                              //read next tags until </select>
                  fi.form_select = tdi.form_select_options.size();
                  tdi.form_select_options.push_back(S_html_form_select_option());
                  S_html_form_select_option &fsel = tdi.form_select_options.back();
                  C_vector<S_html_form_select_option::S_option> options;
                  S_html_form_select_option::S_option *opt = NULL;
                  Cstr_c display_name;
                  while(true){
                     dword c = byte(*cp);
                     if(!c)
                        break;
                     ++cp;
                     if(c=='<'){
                        SkipWSAndEol(cp);
                        bool tag_end = false;
                        if(*cp=='/'){
                           tag_end = true;
                           ++cp;
                        }
                        if(opt){
                                             //convert previous display name
                           ConvertHtmlString(display_name, tdi.body_c.coding, &opt->display_name, NULL);
                           opt = NULL;
                           display_name = NULL;
                        }
                                             //tag, read what it is
                        E_HTML_TAG tag = (E_HTML_TAG)FindHTMLKeyword(cp, html_tags, tag_delimiters);
                        if(tag==TAG_SELECT){
                           assert(tag_end);
                           break;
                        }
                        if(tag==TAG_OPTION){
                           if(!tag_end){
                              //begin option
                              options.push_back(S_html_form_select_option::S_option());
                              opt = &options.back();
                              while(text_utils::GetNextAtrrValuePair(cp, attr, val, false)){
                                 if(attr=="value"){
                                    //ConvertHtmlString(val, tdi.body_c.coding, &opt.value, NULL);
                                    opt->value = val;
                                 }else
                                 if(attr=="selected"){
                                    if(fi.type==fi.SELECT_SINGLE)
                                       fsel.selected_option = options.size()-1;
                                    else{
                                       opt->selected = true;
                                       opt->init_selected = true;
                                    }
                                 }
#ifdef _DEBUG
                                 else
                                 if(attr=="class" || attr=="style"){
                                 }else
                                 if(attr=="disabled" || attr=="label" || attr=="selectedvalue"){
                                 }else
                                    assert(0);
#endif
                              }
                           }else
                           if(opt){
                              ConvertHtmlString(display_name, tdi.body_c.coding, &opt->display_name, NULL);
                              opt = NULL;
                              display_name = NULL;
                           }
                        }else
                        if(tag==TAG_COMMENT){
                           SkipHtmlComment(cp);
                        }else
                           assert(0);
                        FindTagEnd(cp);
                     }else
                     if(opt){
                        if(c>=' ')
                           display_name<<(char)c;
                     }
                  }
                              //clean options, compute size
                  const S_font &fd = font_defs[config.viewer_font_index];
                  int max_w = fd.letter_size_x;
                  for(int i=0; i<options.size(); i++){
                     int w = GetTextWidth(options[i].display_name, config.viewer_font_index);
                     max_w = Max(max_w, w);
                  }
                  fi.sx = max_w + 6;
                  if(fi.type==fi.SELECT_SINGLE){
                     fi.sx += fd.line_spacing;
                     fi.sy = fd.line_spacing + 4;
                     fsel.selected_option = Min(Max(0, fsel.selected_option), (int)options.size()-1);
                     fsel.init_selected_option = fsel.selected_option;
                  }else{
                     fsel.select_visible_lines = Max(1, Min(form_size, Min(10, options.size())));
                     fi.sy = fd.line_spacing*fsel.select_visible_lines + 4;
                     fsel.sb.visible_space = fsel.select_visible_lines;
                     fsel.sb.total_space = options.size();
                     fsel.sb.SetVisibleFlag();
                     fsel.sb.rc = S_rect(0, 0, GetScrollbarWidth(), fi.sy-6);
                     if(fsel.sb.visible){
                        fi.sx += fsel.sb.rc.sx + 3;
                     }
                  }
                  fi.sx = Min(fi.sx, tdi.rc.sx);
                              //validate selection
                  fsel.options.Assign(options.begin(), options.end());
               }else
               if(tag==TAG_TEXTAREA){
                  FindTagEnd(cp);

                  Cstr_c str;
                  while(true){
                     dword c = byte(*cp);
                     if(!c)
                        break;
                     ++cp;
                     if(c=='<'){
                        SkipWSAndEol(cp);
                        bool tag_end = false;
                        if(*cp=='/'){
                           tag_end = true;
                           ++cp;
                        }
                                             //tag, read what it is
                        E_HTML_TAG tag = (E_HTML_TAG)FindHTMLKeyword(cp, html_tags, tag_delimiters);
                        if(tag==TAG_TEXTAREA){
                           assert(tag_end);
                           break;
                        }else
                        if(tag==TAG_COMMENT){
                           SkipHtmlComment(cp);
                        }
                        FindTagEnd(cp);
                     }else{
                        if(c=='\t')
                           str<<"   ";
                        else
                        if(c>=' ' || c=='\n')
                           str<<(char)c;
                     }
                  }
                  ConvertHtmlString(str, tdi.body_c.coding, &fi.value, NULL);
                  fi.init_value = fi.value;
               }
               if(fi.type!=fi.HIDDEN){
                  if(!is_space)
                     dst_buf.push_back(' ');

                  const S_font &fd1 = font_defs[config.viewer_font_index];
                              //compute size of form
                  switch(fi.type){
                  case S_html_form_input::SUBMIT:
                  case S_html_form_input::RESET:
                     fi.sx = GetTextWidth(fi.value, config.viewer_font_index) + 6;
                     fi.sx = Min(fi.sx, tdi.rc.sx);
                     fi.sy = fd1.line_spacing + 2;
                     break;
                  case S_html_form_input::TEXT:
                  case S_html_form_input::PASSWORD:
                     fi.sx = (fd1.letter_size_x+1) * (form_size+2) + 2;
                     fi.sx = Min(fi.sx, tdi.rc.sx);
                     fi.sy = fd1.line_spacing + 2;
                     fi.edit_pos = (short)fi.value.Length();
                     fi.edit_sel = 0;
                     break;
                  case S_html_form_input::TEXTAREA:
                     rows = Max(1, Min(10, rows));
                     cols = Max(4, cols);
                     fi.sx = (fd1.letter_size_x+1) * (cols+2) + 2;
                     fi.sy = fd1.line_spacing*rows + 2;
                     fi.sx = Min(fi.sx, tdi.rc.sx);
                     break;
                  case S_html_form_input::IMAGE:
                     InitHtmlImage(img_src, img_alt, img_sx, img_sy, www_domain, tdi, dst_buf, 0, 0);
                     break;
                  case S_html_form_input::CHECKBOX:
                     fi.sx = fi.sy = fd1.cell_size_y;
                     break;
                  case S_html_form_input::RADIO:
                     fi.sx = fi.sy = fd1.cell_size_y;
                     break;
                  case S_html_form_input::SELECT_SINGLE:
                  case S_html_form_input::SELECT_MULTI:
                     break;
                  default:
                     assert(0);
                  }
                              //enter code into text stream for active object (form input)
                  dst_buf.push_back(CC_FORM_INPUT);
                  int indx = tdi.form_inputs.size();
                  dst_buf.push_back(indx&0xff);
                  dst_buf.push_back(indx>>8);
                  dst_buf.push_back(CC_FORM_INPUT);

                  ts_last.WriteAndUpdate(ts, dst_buf);

                  dst_buf.push_back(' ');
                  is_space = true;
               }
               tdi.form_inputs.push_back(fi);
            }else{
               if(tag==TAG_SELECT && !tag_end){
                  FindMatchingEndTag(cp, tag);
               }
            }
            break;
#else
         case TAG_SELECT:
         case TAG_TEXTAREA:
            if(!tag_end)
               FindMatchingEndTag(cp, tag);
            break;
#endif//WEB_FORMS


         case TAG_UL:
         case TAG_DIR:
         case TAG_MENU:
         case TAG_OL:
            if(!tag_end){
               AddEolIfNotPresent(dst_buf);
               list.push_back(tag==TAG_OL ? 1 : 0);
            }else{
               if(list.size())
                  list.pop_back();
               dst_buf.push_back('\n');
            }
            break;

         case TAG_LI:
            if(!tag_end && list.size()){
               ts_last.WriteAndUpdate(ts, dst_buf);
               dst_buf.push_back('\n');
               for(int i=list.size()*LIST_TAB; i--; )
                  dst_buf.push_back(' ');
               int &li = list.back();
               if(!li){
                              //unordered, add dot
                  dst_buf.push_back((byte)0x95);
               }else{
                              //ordered, add number
                  Cstr_c s; s.Format("%.") <<li++;
                  dst_buf.insert(dst_buf.end(), s, s+s.Length());
               }
               dst_buf.push_back(' ');
            }
            break;

         case TAG_LINK:
            {
#ifdef WEB_FORMS
               cp = cp;
               //assert(0);
#endif
            }
            break;

         case TAG_OPTION:
         case TAG_MAP:
         case TAG_AREA:
         case TAG_CAPTION:
         case TAG_APPLET:
            break;

         case TAG_HR:
            {
               AddEolIfNotPresent(dst_buf);
               dword flg = ts.font_flags;
               ts.font_flags &= ~(FF_CENTER|FF_RIGHT);
               ts.font_flags |= FF_CENTER;
               while(text_utils::GetNextAtrrValuePair(cp, attr, val, false)){
                  if(attr=="align"){
                     ts.font_flags &= ~(FF_CENTER|FF_RIGHT);
                     if(val=="center")
                        ts.font_flags |= FF_CENTER;
                     else
                     if(val=="right")
                        ts.font_flags |= FF_RIGHT;
                  }
               }
               ts_last.WriteAndUpdate(ts, dst_buf);
               for(int i=0; i<15; i++)
                  dst_buf.push_back('-');
               dst_buf.push_back('\n');
               ts.font_flags = flg;
            }
            break;

         case TAG_XML:
         case TAG_STYLE:
            if(!tag_end)
               FindMatchingEndTag(cp, tag);
            break;

         case TAG_SCRIPT:
#ifdef WEB
            if(!tag_end){
               if(use_js){
                  bool is_js = script_type_is_js;
                  while(text_utils::GetNextAtrrValuePair(cp, attr, val, false)){
                     if(attr=="src"){
                        //assert(0);
                     }else
                     if(attr=="type"){
                        val.ToLower();
                        if(val=="text/javascript")
                           is_js = true;
                        else{
                           is_js = false;
                           assert(0);
                        }
                     }
#ifdef _DEBUG
                     else
                     if(attr=="language" || attr=="defer" || attr=="charset"){
                        //assert(0);
                     }else
                        assert(0);
#endif
                  }
                  if(is_js){
                     FindTagEnd(cp);
                     const char *beg = cp;
                     FindMatchingEndTag(cp, tag);
                              //execute the script code
                     const char *end = cp;
                     while(end>beg){
                        if(*end=='<'){
                              //create script class if not yet
                           if(!oh.js_data->eng)
                              ((C_web_client*)this)->InitJavaScript(*oh.js_data);
                           if(oh.js_data->eng){
                              dword len = end - beg;
                              oh.js_data->eng->ProcessString(beg, len);
                           }
                           break;
                        }
                        --end;
                     }
                  }else
                     FindMatchingEndTag(cp, tag);
               }else
                  FindMatchingEndTag(cp, tag);
            }
#else
            if(!tag_end)
               FindMatchingEndTag(cp, tag);
#endif
            break;

         case TAG_NOSCRIPT:
#ifdef WEB
            if(!tag_end){
                              //if JavaScript is enabled, ignore everything up to closing tag
               if(use_js)
                  FindMatchingEndTag(cp, tag);
            }
#endif
            break;

         case TAG_TITLE:
            if(!tag_end){
               FindTagEnd(cp);
               const char *beg = cp;
               while(*cp){
                  char c1 = *cp;
                  if(!c1 || c1=='\n' || c1=='<')
                     break;
                  ++cp;
               }
               coded_title.Allocate(beg, cp-beg);
               for(int i=coded_title.Length(); i--; ){
                  char &c1 = coded_title.At(i);
                  if(dword(c1)<' ')
                     c1 = ' ';
               }
               FindMatchingEndTag(cp, tag);
            }
            break;
         //default: assert(0);
         }
         FindTagEnd(cp);
         continue;
      }

                              //keep only single white-space
      switch(c){
      case ' ':
      case '\n':
      case '\t':
         if(!is_preformated){
            if(is_space)
               continue;
            is_space = true;
            if(!dst_buf.size() || dst_buf.back()=='\n')
               continue;
            c = ' ';
         }else{
            if(c=='\t')
               c = ' ';
            is_space = true;
         }
         break;
      default:
         if(c<' ')
            continue;
         is_space = false;
      }
      ts_last.WriteAndUpdate(ts, dst_buf);

      if(tdi.body_c.coding==COD_UTF_8 && (c&0xc0)==0xc0){
         --cp;
         c = text_utils::DecodeUtf8Char(cp);
      }
      bool allow_spec = false;
      if(c=='&'){
         c = ConvertNamedHtmlChar(cp);
         if(!c)
            break;
         allow_spec = true;
      }else
      if(c>=0x80){
         if(encoding::ConvertCodedCharToUnicode(c, tdi.body_c.coding)==c)
            allow_spec = true;
      }
      if(allow_spec || tdi.body_c.coding==COD_DEFAULT || tdi.body_c.coding==COD_WESTERN || tdi.body_c.coding==COD_UTF_8){
                              //detect special characters, and write them as multi-character
         const char *spec = GetMulticharString(c);
         if(spec){
            dst_buf.insert(dst_buf.end(), spec, spec+StrLen(spec));
            continue;
         }
      }
      if(c>=' ' || c=='\n'){
         if(c<256){
            dst_buf.push_back(char(c));
         }else{
            dst_buf.push_back(CC_WIDE_CHAR);
            dst_buf.push_back(char(c&255));
            dst_buf.push_back(char(c>>8));
            dst_buf.push_back(CC_WIDE_CHAR);
         }
      }
   }
//finish:
#ifdef WEB
   if(js_on_body_onload.Length())
      ((C_web_client*)this)->RunJavaScript(*oh.js_data, js_on_body_onload, js_on_body_onload.Length());
#endif

                              //copy buffer to body string
   if(!oh.append)
      tdi.body_c.Allocate(dst_buf.begin(), dst_buf.size());
   else{
      Cstr_c tmp = tdi.body_c;
      int l1 = tmp.Length();
      int l2 = dst_buf.size();
      tdi.body_c.Allocate(NULL, l1+1+l2);
      char *cp1 = &tdi.body_c.At(0);
      MemCpy(cp1, tmp, l1);
      cp1[l1] = '\n';
      MemCpy(cp1+l1+1, dst_buf.begin(), l2);
   }
   tdi.is_wide = false;

   if(!tdi.html_title.Length())
      ConvertHtmlString(coded_title, tdi.body_c.coding, &tdi.html_title, NULL);
}

#endif //MAIL || WEB || EXPLORER
//----------------------------

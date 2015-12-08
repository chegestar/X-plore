#include "Main.h"
#include <C_math.h>

#ifdef _DEBUG
//#define DEBUG_DISPLAY_SPACE //display space character as dot
#endif

//----------------------------

void C_client::DrawNiceFileName(const wchar *fn, int x, int y, dword font_index, dword color, int width){

   int tw = GetTextWidth(fn, font_index);
   if(tw > width){
      const wchar *ext = text_utils::GetExtension(fn);
      if(ext){
         --ext;
         int ew = GetTextWidth(ext, font_index);
         DrawString(ext, x+width, y, font_index, FF_RIGHT, color);
         width -= ew - 1;
      }
   }
                              //entire fits, just draw it
   if(width > 0)
      DrawString(fn, x, y, font_index, 0, color, -width);
}

//----------------------------

void C_client::DrawNiceURL(const char *url, int x, int y, dword font_index, dword color, int width){

   const S_font &fd = font_defs[font_index];
   S_rect rc(x, y, width, fd.line_spacing);
   SetClipRect(rc);
   int tw = GetTextWidth(url, false, COD_DEFAULT, font_index);
   DrawString2(url, false, Min(x, x+width-tw), y, font_index, 0, color, 0);
   ResetClipRect();
}

//----------------------------

C_image *C_client::OpenHtmlImage(const Cstr_w &fname, const S_text_display_info &tdi, S_image_record &ir) const{

   C_image *img = C_image::Create(*this);
   C_image::E_RESULT r;
   if(ir.size_set){
                  //size set explicitly by html, respect it (but don't enlarge)
      r = img->Open(fname, ir.sx, ir.sy);
   }else{
                  //size not specified, adopt from image
      int max_sx = tdi.rc.sx;
      int max_sy = tdi.rc.sy;
#ifdef MUSICPLAYER
      max_sx /= 2;
      max_sy /= 2;
#endif
      if(ir.draw_border){
         max_sx -= 2;
         max_sy -= 2;
      }
      r = img->Open(fname, max_sx, max_sy,
         /*
#if defined MAIL || defined WEB
         config.img_ratio
#else
         */
         C_fixed::One()
//#endif
         );
   }
   switch(r){
   case C_image::IMG_OK:
      ir.img = img;
      ir.filename = fname;
                              //adopt real image size, even if html specified it, image ratio in html may be incorrect
      ir.sx = img->SizeX();
      ir.sy = img->SizeY();
      break;
   default:
      const wchar *err = L"<load error>";
      switch(r){
      case C_image::IMG_NOT_ENOUGH_MEMORY: err = L"<not enough memory>"; break;
      case C_image::IMG_NO_FILE: err = L"<file not found>"; break;
      }
      ir.alt = err;
   }
   return img;
}

#if defined MAIL || defined WEB_FORMS
//----------------------------
#ifdef WEB_FORMS

int C_client::DrawHtmlFormInput(const S_text_display_info &tdi, const S_html_form_input &fi, bool active, const S_text_style &ts,
   const S_draw_fmt_string_data *fmt, int x, int y, dword bgnd_pix, const char *text_stream){

   S_rect rc(x, y-fi.sy, fi.sx, fi.sy);
   const S_font &fd = font_defs[ts.font_index];
   const dword col_white = GetColor(COL_WHITE), col_black = GetColor(COL_BLACK),
      col_light_grey = GetColor(COL_LIGHT_GREY), col_dark_grey = GetColor(COL_DARK_GREY), col_grey = GetColor(COL_GREY);
   dword text_color = !fi.disabled ? 0xff000000 : col_grey;

                              //fill rectangle not covered by the control by background color
   if(ts.bgnd_color){
      FillPixels(S_rect(x, y-fmt->line_height, fi.sx, fmt->line_height-fi.sy), bgnd_pix);
   }

   switch(fi.type){
   case S_html_form_input::SUBMIT:
   case S_html_form_input::RESET:
      rc.Compact();
      FillRect(rc, col_light_grey);
      DrawOutline(rc, col_white, col_black);
      break;

   case S_html_form_input::SELECT_SINGLE:
   case S_html_form_input::SELECT_MULTI:
      {
         rc.Compact();
         DrawOutline(rc, col_light_grey, col_black);
         rc.Compact();
         DrawOutline(rc, col_black, col_light_grey);
         FillRect(rc, fi.disabled ? col_light_grey : col_white);
         rc.Expand();

         if(fi.type==fi.SELECT_SINGLE){
                              //draw drop-down arrow
            S_rect rc_arrow(rc.Right()-fd.line_spacing-1, rc.y+1, fd.line_spacing, fd.line_spacing);
            rc_arrow.Compact();
            DrawOutline(rc_arrow, col_white, col_black);
            FillRect(rc_arrow, col_light_grey);
            //sset_mail->DrawSprite(rc_arrow.CenterX(), rc_arrow.CenterY(), SPR_HTML_DROPDOWN);
         }
      }
      break;

   case S_html_form_input::TEXT:
   case S_html_form_input::PASSWORD:
   case S_html_form_input::TEXTAREA:
      rc.Compact();
      FillRect(rc, fi.disabled ? col_light_grey : col_white);
      DrawOutline(rc, col_dark_grey, col_light_grey);
      break;

   case S_html_form_input::CHECKBOX:
   case S_html_form_input::RADIO:
      {
         if(ts.bgnd_color)
            FillPixels(rc, bgnd_pix);
         int sz = fd.cell_size_y-2;
         rc.y -= (fd.line_spacing-rc.sy)/2;
         DrawCheckboxFrame(rc.x, rc.y, sz);
         if(fi.checked)
            DrawCheckbox(rc.x+sz/2, rc.y+sz/2, sz);
         rc.Compact();
      }
      break;

   case S_html_form_input::IMAGE:
                              //draw only outline of active image
      if(!active)
         return 0;
                              //read image size from image entry right before this
      {
         text_stream -= 8;
         char c = *text_stream++;
         assert(c==CC_IMAGE);
         int ii = ts.Read16bitIndex(text_stream);
         assert(ii < fmt->tdi->images.size());
         const S_image_record &ir = fmt->tdi->images[ii];
         rc = S_rect(x-ir.sx, y-ir.sy, ir.sx, ir.sy);
         rc.Compact();
      }
      break;
   }
   y -= fd.line_spacing;
                              //draw selection around active form input
   if(active){
      const dword col0 = 0xffff0000, col1 = 0xff000000;
      DrawDashedOutline(rc, col0, col1);
      rc.Compact();
      DrawDashedOutline(rc, col0, col1);
      rc.Expand();
   }
                              //draw texts
   switch(fi.type){
   case S_html_form_input::PASSWORD:
   case S_html_form_input::TEXT:
   case S_html_form_input::TEXTAREA:
      if(active && fmt->tdi->te_input){
         C_text_editor &te = *(C_text_editor*)(const C_text_editor*)fmt->tdi->te_input;
         te.rc = rc;
         ++te.rc.y;
         ++te.rc.x;
         te.rc.sx -= 2;
         DrawEditedText(te, false);
         break;
      }
                              //flow...
   case S_html_form_input::SUBMIT:
   case S_html_form_input::RESET:
      {
         Cstr_w s = fi.value;
         if(fi.type==fi.PASSWORD)
            MaskPassword(s);
         DrawString(s, x+3, y, ts.font_index, 0, text_color, -(fi.sx-6));
      }
      break;

   case S_html_form_input::SELECT_SINGLE:
      {
         const S_html_form_select_option &fsel = tdi.form_select_options[fi.form_select];
         if(fsel.selected_option!=-1){
            const Cstr_w &s = fsel.options[fsel.selected_option].display_name;
            int max_w = fi.sx-fd.line_spacing-6;
            DrawString(s, x+3, y, ts.font_index, 0, text_color, -max_w);
         }
      }
      break;

   case S_html_form_input::SELECT_MULTI:
      {
         const S_html_form_select_option &fsel = tdi.form_select_options[fi.form_select];
         int y = rc.y+1;
         int txt_w = fi.sx-4;
         if(fsel.sb.visible)
            txt_w -= fsel.sb.rc.sx+3;
         for(int i=0; i<fsel.select_visible_lines; i++){
            dword indx = fsel.sb.pos+i;
            if(indx>=fsel.options.Size())
               break;
            const S_html_form_select_option::S_option &opt = fsel.options[indx];
            const Cstr_w &s = opt.display_name;
            dword color = text_color;
            if(opt.selected){
               FillRect(S_rect(x+2, y, txt_w, fd.line_spacing), 0xff0000c0);
               color ^= 0xffffff;
            }
            if(indx==fsel.selection && active && !fi.disabled){
                              //draw outline around item
               S_rect rc(x+3, y+1, txt_w-2, fd.line_spacing-2);
               DrawDashedOutline(rc, 0xffffffff, 0xff000000);
            }
            DrawString(s, x+3, y+1, ts.font_index, 0, color, -(txt_w-2));
            y += fd.line_spacing;
         }
                              //draw scrollbar
         if(fsel.sb.visible){
            C_scrollbar sb = fsel.sb;
            sb.rc.x = rc.Right()-sb.rc.sx-2;
            sb.rc.y = rc.y+2;
            DrawScrollbar(sb);
         }
      }
      break;
   }
   return fi.sx;
}

#endif//WEB_FORMS

#endif //MAIL || WEB_FORMS
//----------------------------

void C_client::ToggleUseClientRect(){

   C_application_skinned::ToggleUseClientRect();
   config.fullscreen = !(GetGraphicsFlags()&IG_SCREEN_USE_CLIENT_RECT);
   SaveConfig();
}

//----------------------------

void C_client::GetDateString(const S_date_time &dt, Cstr_w &str, bool force_short) const{

   str.Clear();
   int m = dt.month + 1;
   int d = dt.day + 1;
   int y = dt.year % 100;
   if(!force_short){
      str.Format(L"%.%.'#02%") <<(int)d <<(int)m <<(int)y;
   }else
      str<<d <<L'.' <<m <<L'.';
}

//----------------------------

int C_client::GetLineYAtOffset(const S_text_display_info &tdi, int byte_offs) const{

   int y = 0;
   bool wide = tdi.is_wide;
   const void *cp = wide ? (const char*)(const wchar*)tdi.body_w : (const char*)tdi.body_c;

   S_text_style ts;
   ts.font_index = config.viewer_font_index;
   while(true){
      int h;
      int len = ComputeFittingTextLength(cp, wide, tdi.rc.sx, &tdi, ts, h, config.viewer_font_index);
      if(!len)
         break;
      if((byte_offs -= len) < 0)
         break;
      y += h;
      (char*&)cp += len;
   }
   return y;
}

//----------------------------

void C_client::ProcessCCPMenuOption(int option, C_text_editor *te){
   assert(te);
   switch(option){
   case SPECIAL_TEXT_CUT: te->Cut(); break;
   case SPECIAL_TEXT_COPY: te->Copy(); break;
   case SPECIAL_TEXT_PASTE: te->Paste(); break;
   default:
      assert(0);
   }
}

//----------------------------
/*
void C_client::DrawFloatDataCounters(C_smart_ptr<C_image> &data_cnt_bgnd, bool save_bgnd, int force_right, int force_y){

#ifdef _DEBUG__
   int sent=100, recv=3000;
#else
   if(!connection)
      return;

   int sent = 0, recv = 0;
   switch(config.flags&config.CONF_DRAW_COUNTER_MASK){
   case S_config::CONF_DRAW_COUNTER_TOTAL:
      sent = config.total_data_sent;
      recv = config.total_data_received;
                              //flow...
   case S_config::CONF_DRAW_COUNTER_CURRENT:
      sent += connection->GetDataSent();
      recv += connection->GetDataReceived();
      break;
   default:
      return;
   }
#endif
   if(!sent && !recv)
      return;

   const dword txt_color = 0xff000000;
   int width;
   if(save_bgnd && data_cnt_bgnd){
      width = data_cnt_bgnd->SizeX();
   }else{
      width = GetTextWidth(GetText(TXT_STAT_SENT), UI_FONT_SMALL);
      width = Max(width, GetTextWidth(GetText(TXT_STAT_RECEIVED), UI_FONT_SMALL));
      width += GetTextWidth(L": 98.87MB", UI_FONT_SMALL);
      width += fds.letter_size_x*4;
   }

   S_rect rc(force_right!=-1 ? force_right-width : width/12, force_y!=-1 ? force_y : (ScrnSY()-fds.line_spacing*4-width/12), width, fds.line_spacing*2+fds.cell_size_y);
   int tx = rc.x+fds.letter_size_x*2, ty = rc.y + fds.line_spacing/2;

   if(!data_cnt_bgnd){
                              //create transparent bgnd for 1st time, and save it
      DrawOutline(rc, 0xff000000);
      FillRect(rc, 0x80ffffff);
      DrawShadow(rc);

      if(save_bgnd){
                              //save image below counters
         C_image *img = C_image::Create(*this);
         data_cnt_bgnd = img;
         img->Release();
         img->CreateFromScreen(S_rect(tx, ty, width, fds.line_spacing*2));
      }
   }else{
      data_cnt_bgnd->Draw(tx, ty);
   }

                              //draw text one pixel right due to font filtering
   ++tx;
   {
      Cstr_w sz;
      {
         Cstr_w s;
         sz = text_utils::MakeFileSizeText(sent, false, false);
         s<<GetText(TXT_STAT_SENT) <<L' ' <<sz;
         DrawString(s, tx, ty, UI_FONT_SMALL, 0, txt_color);
      }
      ty += fds.line_spacing;
      {
         Cstr_w s;
         sz = text_utils::MakeFileSizeText(recv, false, false);
         s<<GetText(TXT_STAT_RECEIVED) <<L' ' <<sz;
         DrawString(s, tx, ty, UI_FONT_SMALL, 0, txt_color);
      }
   }
   AddDirtyRect(rc);
}
*/
//----------------------------

void C_client::PrepareProgressBarDisplay(E_TEXT_ID title, const wchar *txt, S_progress_display_help &pd, dword progress_total, E_TEXT_ID progress1_text){

   int sy = fdb.line_spacing*3;
   if(txt)
      sy += fdb.line_spacing;
   if(progress1_text!=TXT_NULL)
      sy += fdb.line_spacing*3;
   dword sx = ScrnSX();
   int sz_x = Min((int)sx-6, fdb.letter_size_x * 30);

   S_rect rc((sx-sz_x)/2, (ScrnSY()-sy)/2, sz_x, sy);
   RedrawScreen();
   DrawWashOut();

   DrawDialogBase(rc, true);
   DrawDialogTitle(rc, GetText(title));

   pd.progress.rc = S_rect(rc.x+fdb.letter_size_x, rc.y+fdb.line_spacing*7/4, rc.sx-fdb.letter_size_x*2, GetProgressbarHeight());
   if(txt){
      pd.progress.rc.y += fdb.line_spacing;
      DrawString(txt, rc.x+fdb.letter_size_x/2, rc.y+GetDialogTitleHeight(), UI_FONT_BIG, 0, 0xff000000, -(rc.sx-fdb.letter_size_x));
   }
   DrawProgress(pd.progress);
   pd.progress.total = progress_total;

   if(progress1_text!=TXT_NULL){
      pd.progress1.rc = pd.progress.rc;
      pd.progress1.rc.y += fdb.line_spacing*3;
      DrawProgress(pd.progress1);
      DrawString(GetText(progress1_text), rc.x+fdb.letter_size_x/2, pd.progress1.rc.y-fdb.line_spacing*3/2, UI_FONT_BIG, 0, 0xff000000, -(rc.sx-fdb.letter_size_x));
   }
}

//----------------------------

bool C_client::DisplayProgressBar(dword pos, void *context){

   S_progress_display_help *pp = (S_progress_display_help*)context;
   pp->progress.pos = pos;
   pp->_this->DrawProgress(pp->progress);
   if(pp->need_update_whole_screen){
      pp->need_update_whole_screen = false;
      pp->_this->SetScreenDirty();
   }else
      pp->_this->AddDirtyRect(pp->progress.rc);
   pp->_this->UpdateScreen();
   return true;
}

//----------------------------


// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <UI/Config.h>
#include <TextUtils.h>

//----------------------------

C_mode_configuration::C_mode_configuration(C_application_ui &_app, C_configuration_editing *ce):
   C_mode_app<C_application_ui>(_app),
   configuration_editing(ce),
   modified(false)
{
   last_sk[0] = 0;
   last_sk[1] = app.SPECIAL_TEXT_BACK;
}

//----------------------------

void C_mode_configuration::PositionTextEditor(){

   assert(text_editor);

   C_text_editor &te = *text_editor;
   te.SetRect(S_rect(rc.x+app.fdb.letter_size_x, rc.y + 1 + app.fdb.line_spacing + selection*entry_height-sb.pos, max_textbox_width, app.fdb.cell_size_y+1));
   app.MakeSureCursorIsVisible(te);
}

//----------------------------

void C_mode_configuration::InitLayout(){

   const int border = 2;
   const int top = app.GetTitleBarHeight();
   rc = S_rect(border, top+border, app.ScrnSX()-border*2, app.ScrnSY()-top-app.GetSoftButtonBarHeight()-border*2);
                              //space for help
   if(app.IsWideScreen())
      rc.sx -= rc.sx/3;
   else
      rc.sy -= app.fds.line_spacing*9/2+border*2;

   entry_height = app.fdb.line_spacing * 2 + 2;
   sb.visible_space = (rc.sy-border*2);
   rc.y += (rc.sy - sb.visible_space)/2;
   rc.sy = sb.visible_space;

                              //init scrollbar
   const int width = app.GetScrollbarWidth();
   sb.rc = S_rect(rc.Right()-width-1, rc.y+1, width, rc.sy-2);
   sb.total_space = options.Size() * entry_height;
   sb.SetVisibleFlag();

   max_textbox_width = sb.rc.x-rc.x - app.fdb.letter_size_x*3;
   if(app.IsWideScreen()){
      max_textbox_width = Min(max_textbox_width, app.fdb.letter_size_x*30);
   }
   SetSelection(selection);
}

//----------------------------

void C_mode_configuration::SetSelection(int sel){

   selection = Abs(sel);
   text_editor = NULL;
   const S_config_item &ec = options[selection];
   switch(ec.ctype){
   case CFG_ITEM_NUMBER:
   case CFG_ITEM_SIGNED_NUMBER:
   case CFG_ITEM_TEXTBOX_CSTR:
   case CFG_ITEM_PASSWORD:
      {
         text_editor = app.CreateTextEditor(
            (!ec.is_wide ? TXTED_ANSI_ONLY : 0) |
            (ec.ctype==CFG_ITEM_NUMBER ? TXTED_NUMERIC : ec.ctype==CFG_ITEM_PASSWORD ? TXTED_SECRET : (ec.param2&8) ? TXTED_ALLOW_PREDICTIVE : 0),
            app.UI_FONT_BIG, 0, NULL, ec.param ? ec.param : 100);
         text_editor->Release();
         C_text_editor &te = *text_editor;
         S_rect trc = S_rect(rc.x, 0, max_textbox_width, app.fdb.line_spacing);
         if(app.IsWideScreen())
            trc.x += app.fdb.letter_size_x*10;
         else
            trc.x += app.fdb.letter_size_x;
         te.SetRect(trc);

         switch(ec.ctype){
         case CFG_ITEM_TEXTBOX_CSTR:
         case CFG_ITEM_PASSWORD:
            {
               te.SetCase(C_text_editor::CASE_ALL, ec.param2&C_text_editor::CASE_ALL);
               Cstr_w str;
               void *vp = (cfg_base + ec.elem_offset);
               switch(ec.is_wide){
               case 0: str.Copy(*(Cstr_c*)vp); break;
               case 1: str = *(Cstr_w*)vp; break;
               case 2: str = ((Cstr_c*)vp)->FromUtf8(); break;
               default: assert(0);
               }
               te.SetInitText(str);
            }
            break;

         case CFG_ITEM_NUMBER:
         case CFG_ITEM_SIGNED_NUMBER:
            {
               int n;
               if(ec.ctype==CFG_ITEM_NUMBER)
                  n = *(word*)(cfg_base + ec.elem_offset);
               else
                  n = *(short*)(cfg_base + ec.elem_offset);
               Cstr_w s;
               if(n)
                  s<<n;
               te.SetInitText(s);
            }
            break;
         default:
            assert(0);
         }
      }
      break;
   }
   if(sel>=0)
      EnsureVisible();
   if(text_editor){
      text_editor->SetCursorPos(text_editor->GetTextLength());
      PositionTextEditor();
   }
}

//----------------------------

void C_mode_configuration::SelectionChanged(int old_sel){
   SetSelection(-selection);
}

//----------------------------

void C_mode_configuration::Close(bool redraw){

   //AddRef();
   text_editor = NULL;
   C_mode::Close(redraw);
   if(configuration_editing)
      configuration_editing->OnClose();
   /*
   app.mode = GetParent();
   if(app.mode && app.mode->timer)
      app.mode->timer->Resume();
   app.RedrawScreen();
   Release();
   */
}

//----------------------------

void C_mode_configuration::TextEditNotify(bool cursor_moved, bool text_changed, bool &redraw){

   if(text_changed){
      if(EnsureVisible()){
         PositionTextEditor();
      }
      C_text_editor &te = *text_editor;
      const S_config_item &ec = options[selection];
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
            {
               Cstr_c &str = *(Cstr_c*)(cfg_base + ec.elem_offset);
               str.Copy(te.GetText());
            }
            break;
         case 1:
            {
               Cstr_w &str = *(Cstr_w*)(cfg_base + ec.elem_offset);
               str = te.GetText();
            }
            break;
         case 2:
            {
               Cstr_c &str = *(Cstr_c*)(cfg_base + ec.elem_offset);
               str.ToUtf8(te.GetText());
            }
            break;
         default:
            assert(0);
         }
      }
      modified = true;
      if(configuration_editing)
         configuration_editing->OnConfigItemChanged(ec);
   }
   redraw = true;
}

//----------------------------

void C_mode_configuration::ProcessInput(S_user_input &ui, bool &redraw){

   ProcessInputInList(ui, redraw);

    const S_config_item &ec = options[selection];
#ifdef USE_MOUSE
   if(app.HasMouse())
   if(!app.ProcessMouseInSoftButtons(ui, redraw)){
      if(text_editor && app.ProcessMouseInTextEditor(*text_editor, ui))
         redraw = true;
      else
      if(ui.mouse_buttons&MOUSE_BUTTON_1_UP){
         if(ui.key==K_ENTER){
            switch(ec.ctype){
            case CFG_ITEM_CHECKBOX:
            case CFG_ITEM_CHECKBOX_INV:
               //ui.key = K_ENTER;
               break;
            default:
               if(ui.mouse.x<rc.sx/3){
                  ui.key = K_CURSORLEFT;
               }else
               if(ui.mouse.x>=rc.sx*2/3){
                  ui.key = K_CURSORRIGHT;
               }else
                  ui.key = K_ENTER;
               if(configuration_editing && ui.key!=K_ENTER)
                  configuration_editing->OnClick(ec);
            }
         }
      }
   }
#endif

   if(ui.key){
      AddRef();
      if(ChangeConfigOption(ec, ui.key, ui.key_bits)){
         modified = true;
         redraw = true;
         ui.key = 0;
         if(configuration_editing)
            configuration_editing->OnConfigItemChanged(ec);
      }
      if(!Release())
         return;
   }
   switch(ui.key){
   case K_SOFT_SECONDARY:
   case K_BACK:
   case K_ESC:
      Close();
      return;
   case K_ENTER:
      if(configuration_editing)
         configuration_editing->OnClick(ec);
      break;

   default:
      if(text_editor)
         text_editor->Activate(true);
   }
}

//----------------------------

bool C_mode_configuration::ChangeConfigOption(const S_config_item &ec, dword key, dword key_bits){

   bool changed = false;
   switch(ec.ctype){

   case CFG_ITEM_WORD_NUMBER:
      {
         word &val = *(word*)(cfg_base + ec.elem_offset);
         int min = ec.param>>16, max = ec.param&0xffff;
         const int step = ec.param2 ? ec.param2 : 1;
         if(key==K_CURSORLEFT){
            if(val>min){
               val = (word)Max(min, int(val)-step);
               changed = true;
            }
         }else
         if(key==K_CURSORRIGHT){
            if(val<max){
               val = (word)Min(max, int(val)+step);
               changed = true;
            }
         }
      }
      break;

   case CFG_ITEM_CHECKBOX:
   case CFG_ITEM_CHECKBOX_INV:
      if(key==K_ENTER){
         if(ec.param){
            dword &flags = *(dword*)(cfg_base + ec.elem_offset);
            flags ^= ec.param;
         }else{
            bool &b = *(bool*)(cfg_base + ec.elem_offset);
            b = !b;
         }
         changed = true;
      }
      break;
   }
   return changed;
}

//----------------------------

bool C_mode_configuration::GetConfigOptionText(const S_config_item &ec, Cstr_w &str, bool &draw_arrow_l, bool &draw_arrow_r) const{

   switch(ec.ctype){
   case CFG_ITEM_WORD_NUMBER:
      {
         word val = *(word*)(cfg_base + ec.elem_offset);
         str<<dword(val);
         int min = ec.param>>16, max = ec.param&0xffff;
         draw_arrow_l = (val > min);
         draw_arrow_r = (val < max);
      }
      return true;
   }
   return false;
}

//----------------------------

void C_mode_configuration::DrawItemTitle(const S_config_item &ec, const S_rect &rc_item, dword text_color) const{

                        //draw name
   int xx = rc_item.x;
   if(app.IsWideScreen())
      xx += app.fdb.letter_size_x*5;
   else
      xx += app.fdb.letter_size_x;
   const wchar *item_name;
   if(ec.txt_id<0x10000)
      item_name = app.GetText(ec.txt_id);
   else
      item_name = (const wchar*)ec.txt_id;
   app.DrawString(item_name, xx, rc_item.y + 2, app.UI_FONT_BIG, FF_BOLD, text_color, -(rc_item.Right()-xx-app.fdb.letter_size_x/2));
}

//----------------------------

void C_mode_configuration::CalcCheckboxPosition(const S_rect &rc_item, int &x, int &y, int &size) const{

   size = app.fdb.cell_size_y;
   if(app.IsWideScreen())
      x = rc_item.x+rc_item.sx*3/4;
   else
      x = rc_item.x + rc_item.sx - size - app.fdb.cell_size_x;
   x -= size/2;
   y = rc_item.y + 1 + app.fdb.line_spacing;
}

//----------------------------

void C_mode_configuration::DrawItem(const S_config_item &ec, const S_rect &rc_item, dword text_color) const{

   bool draw_arrow_l = false, draw_arrow_r = false;
                              //draw additional item info
   int xx = rc_item.x, yy = rc_item.y + 1 + app.fdb.line_spacing;
   if(app.IsWideScreen())
      xx += app.fdb.letter_size_x*5;
   else
      xx += app.fdb.letter_size_x;
   Cstr_w txt;
   if(GetConfigOptionText(ec, txt, draw_arrow_l, draw_arrow_r)){
      app.DrawString(txt, rc_item.CenterX(), yy, app.UI_FONT_BIG, FF_CENTER, text_color);
   }else
   switch(ec.ctype){
   case CFG_ITEM_TEXTBOX_CSTR:
   case CFG_ITEM_PASSWORD:
   //case CFG_ITEM_TEXT_NUMBER:
      {
         Cstr_w sw;
         switch(ec.is_wide){
         case 0:
            {
               const Cstr_c &str = *(Cstr_c*)(cfg_base + ec.elem_offset);
               sw.Copy(str);
            }
            break;
         case 1:
            sw = *(Cstr_w*)(cfg_base + ec.elem_offset);
            break;
         case 2:
            {
               const Cstr_c &str = *(Cstr_c*)(cfg_base + ec.elem_offset);
               sw.FromUtf8(str);
            }
            break;
         default:
            assert(0);
         }
         if(ec.ctype==CFG_ITEM_PASSWORD)
            text_utils::MaskPassword(sw);
         app.DrawString(sw, xx, yy, app.UI_FONT_BIG, 0, text_color, -max_textbox_width);
      }
      break;

   case CFG_ITEM_NUMBER:
   case CFG_ITEM_SIGNED_NUMBER:
      {
         int n;
         if(ec.ctype==CFG_ITEM_NUMBER)
            n = *(word*)(cfg_base + ec.elem_offset);
         else
            n = *(short*)(cfg_base + ec.elem_offset);
         if(n){
            Cstr_w s;
            s<<n;
            app.DrawString(s, xx, yy, app.UI_FONT_BIG, 0, text_color, -max_textbox_width);
         }
      }
      break;

   case CFG_ITEM_CHECKBOX:
   case CFG_ITEM_CHECKBOX_INV:
      {
         int x1, y1, size;
         CalcCheckboxPosition(rc_item, x1, y1, size);
         bool on;
         if(ec.param){
            const dword &flags = *(dword*)(cfg_base + ec.elem_offset);
            on = (flags&ec.param);
         }else{
            on = *(bool*)(cfg_base + ec.elem_offset);
         }
         if(ec.ctype==CFG_ITEM_CHECKBOX_INV)
            on = !on;
         app.DrawCheckbox(x1, yy, size, on);
      }
      break;
   }
   DrawArrows(rc_item, text_color, draw_arrow_l, draw_arrow_r);
}

//----------------------------

void C_mode_configuration::DrawArrows(const S_rect &rc_item, dword color, bool left, bool right) const{

   const int arrow_size = (app.fdb.line_spacing/2) | 1;
   int y = rc_item.Bottom()-entry_height/4-arrow_size/2;
   if(left){
      int xx = rc_item.x + app.fdb.letter_size_x;
      app.DrawArrowHorizontal(xx, y, arrow_size, 0xff000000|color, false);
   }
   if(right){
      int xx = rc_item.Right() - app.fdb.letter_size_x - arrow_size;
      app.DrawArrowHorizontal(xx, y, arrow_size, 0xff000000|color, true);
   }
}

//----------------------------

void C_mode_configuration::DrawContents() const{

   {
                              //check if softkeys changed
      const S_config_item &ec = options[selection];
      dword sk0 = GetLeftSoftKey(ec);
      dword sk1 = GetRightSoftKey(ec);
      if(last_sk[0]!=sk0 || last_sk[1]!=sk1){
         last_sk[0] = sk0;
         last_sk[1] = sk1;
         app.ClearSoftButtonsArea();
         app.DrawSoftButtonsBar(*this, sk0, sk1, text_editor);
      }
   }

   const dword col_text = app.GetColor(app.COL_TEXT);
   const dword sx = app.ScrnSX();

   app.ClearToBackground(S_rect(0, rc.Bottom(), sx, app.ScrnSY()-app.GetSoftButtonBarHeight()-rc.Bottom()));

   const int border = 2;
   S_text_display_info tdi_help;
   if(app.IsWideScreen()){
      int xx = rc.Right()+border*2;
      tdi_help.rc = S_rect(xx, rc.y+1, sx-xx-border-1, rc.sy);
      app.ClearToBackground(S_rect(xx-border-1, rc.y, border, rc.sy+2));
      app.ClearToBackground(S_rect(tdi_help.rc.Right()+1, rc.y, border, rc.sy+2));
   }else
      tdi_help.rc = S_rect(border, rc.Bottom()+app.fds.line_spacing/2-1, sx-border*2, app.fds.line_spacing*9/2);

   app.DrawEtchedFrame(rc);
   app.DrawPreviewWindow(tdi_help.rc);

   app.ClearWorkArea(rc);
   S_rect rc_item;
   int item_index = -1;

   while(BeginDrawNextItem(rc_item, item_index)){
      const S_config_item &ec = options[item_index];

      dword color = col_text;
      if(item_index==selection){
         app.DrawSelection(rc_item);
         color = app.GetColor(app.COL_TEXT_HIGHLIGHTED);
      }
      if(rc_item.y > rc.y && (item_index<selection || item_index>selection+1))
         app.DrawSeparator(rc_item.x+app.fdb.letter_size_x*1, rc_item.sx-app.fdb.letter_size_x*2, rc_item.y);

      DrawItemTitle(ec, rc_item, color);
      if(item_index==selection && text_editor)
         app.DrawEditedText(*text_editor);
      else
         DrawItem(ec, rc_item, color);
   }
   EndDrawItems();
   app.DrawScrollbar(sb);

   const S_config_item &ec = options[selection];
   {
                              //draw help
      tdi_help.rc.Compact();
      tdi_help.rc.Compact();

      const wchar *s;
      if(ec.txt_id<0x10000){
         dword hlp_id = ec.hlp_id;
         if(!hlp_id)
            hlp_id = ec.txt_id+1;
         tdi_help.body_w = app.GetText(hlp_id);
      }else{
         s = (const wchar*)ec.txt_id;
         tdi_help.body_w = s + StrLen(s)+1;
      }
      tdi_help.is_wide = true;
      tdi_help.ts.font_index = app.UI_FONT_SMALL;
      tdi_help.ts.text_color = app.GetColor(app.COL_TEXT_POPUP);
      app.DrawFormattedText(tdi_help);
   }
}

//----------------------------

void C_mode_configuration::Draw() const{

   app.DrawTitleBar(app.GetText(title_text_id), rc.y);
   app.ClearSoftButtonsArea();

   DrawContents();

   const S_config_item &ec = options[selection];
   app.DrawSoftButtonsBar(*this, GetLeftSoftKey(ec), GetRightSoftKey(ec), text_editor);
   app.SetScreenDirty();
}

//----------------------------

static void WriteConfigLine(C_file &fl, const char *attr, const char *val){
   fl.Write(attr, StrLen(attr));
   fl.WriteByte('=');
   fl.Write(val, StrLen(val));
   fl.WriteByte('\n');
}
//----------------------------
static void WriteConfigLine(C_file &fl, const char *attr, int val){
   Cstr_c s; s<<val;
   WriteConfigLine(fl, attr, s);
}
//----------------------------
static void WriteConfigLine(C_file &fl, const char *attr, const wchar *val){
   Cstr_c tmp; tmp.ToUtf8(val);
   WriteConfigLine(fl, attr, tmp);
}

//----------------------------

void SaveConfiguration(const wchar *fname, const void *struct_base, const S_config_store_type *vals){

   C_file fl;
   if(!fl.Open(fname, C_file::FILE_WRITE|C_file::FILE_WRITE_CREATE_PATH)){
      return;
   }

   const byte *base = (byte*)struct_base;
   while(vals->type){
      switch(vals->type){
      case S_config_store_type::TYPE_DWORD: WriteConfigLine(fl, vals->name, *(dword*)(base+vals->offset)); break;
      case S_config_store_type::TYPE_WORD: WriteConfigLine(fl, vals->name, *(word*)(base+vals->offset)); break;
      case S_config_store_type::TYPE_BYTE: WriteConfigLine(fl, vals->name, *(base+vals->offset)); break;
      case S_config_store_type::TYPE_CSTR_W: WriteConfigLine(fl, vals->name, *(Cstr_w*)(base+vals->offset)); break;
      case S_config_store_type::TYPE_CSTR_C: WriteConfigLine(fl, vals->name, *(Cstr_c*)(base+vals->offset)); break;
      case S_config_store_type::TYPE_NEXT_VALS: vals = (S_config_store_type*)vals->offset; continue;
      default: assert(0);
      }
      ++vals;
   }
}

//----------------------------

bool LoadConfiguration(const wchar *fname, void *struct_base, const S_config_store_type *vals){

   C_file fl;
   if(!fl.Open(fname))
      return false;
   byte *base = (byte*)struct_base;
   const S_config_store_type *last_val = vals;
   while(!fl.IsEof()){
      Cstr_c line = fl.GetLine();
      const char *cp = line;
      Cstr_c attr;
      if(text_utils::ReadToken(cp, attr, "=")){
         assert(*cp=='=');
         ++cp;
                              //find config value
         const S_config_store_type *val = last_val;
         while(true){
            if(attr==val->name)
               break;
            if(val->type==val->TYPE_NEXT_VALS)
               val = (S_config_store_type*)val->offset;
            else
               ++val;
            if(!val->type)
               val = vals;
            if(val==last_val){
               val = NULL;
               break;
            }
         }
         if(val){
            last_val = val;
            switch(val->type){
            case S_config_store_type::TYPE_DWORD: text_utils::ScanInt(cp, *(int*)(base+val->offset)); break;
            case S_config_store_type::TYPE_WORD:
               {
                  int v;
                  if(text_utils::ScanInt(cp, v))
                     *(word*)(base+val->offset) = word(v);
               }
               break;
            case S_config_store_type::TYPE_BYTE:
               {
                  int v;
                  if(text_utils::ScanInt(cp, v))
                     *(byte*)(base+val->offset) = byte(v&0xff);
               }
               break;
            case S_config_store_type::TYPE_CSTR_W: (*(Cstr_w*)(base+val->offset)).FromUtf8(cp); break;
            case S_config_store_type::TYPE_CSTR_C: (*(Cstr_c*)(base+val->offset)) = cp; break;
            default: assert(0);
            }
         }
      }
   }
   return true;
}

//----------------------------

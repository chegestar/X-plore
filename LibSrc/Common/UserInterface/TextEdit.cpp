//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include "TextEditorCommon.h"

//----------------------------

void C_application_ui::TextEditNotify(C_text_editor *te, bool cursor_blink, bool cursor_moved, bool text_changed){

   bool redraw = false;
   if(text_changed)
      cursor_moved = true;
   if(cursor_moved){
      if(!(te->GetInitFlags()&TXTED_ALLOW_EOL))
         MakeSureCursorIsVisible(*te);
   }
   if(mode)// && (cursor_moved || text_changed))
      mode->TextEditNotify(cursor_moved, text_changed, redraw);
   if(redraw || (mode && mode->GetMenu())){
                              //not very optimal to redraw entire screen, but works in all cases (menu invoked, etc)
      RedrawScreen();
   }
   else
   if(mode)
      mode->DrawDirtyControls();
   UpdateScreen();
}

//----------------------------

C_text_editor *C_application_ui::CreateTextEditor(dword flags, int font_index, dword font_flags, const wchar *init_text, dword max_length, C_text_editor::C_text_editor_notify *notify){

   //if(!(flags&TXTED_ALLOW_EOL))
      //flags |= TXTED_ALLOW_UP_DOWN_MARK;  //add this functionality in single-line editors

   const int sx = ScrnSX(), sy = ScrnSY();

   C_text_editor *te = CreateTextEditorInternal(flags, font_index, font_flags, init_text, max_length, notify);
   //if(ScrnSX()!=(int)igraph->_Scrn_SX() || ScrnSY()!=(int)igraph->_Scrn_SY())
   if(sx!=ScrnSX() || sy!=ScrnSY()){
      ResetClipRect();
   }
   return te;
}

//----------------------------

void C_text_editor_common::ReplaceSelection(const wchar *txt){

   ResetCursor();
                              //erase selection, and add new char
   dword sz = text.size() + StrLen(txt);
   if(!sz)
      return;
   wchar *buf = new(true) wchar[sz];

   int selmin = cursor_pos;
   int selmax = cursor_sel;
   if(selmin > selmax)
      Swap(selmin, selmax);
   //if(selmin<0 || selmax>text.size()) Fatal("bad sel");

   MemCpy(buf, GetText(), selmin*2);
   int di = selmin;
   dword len_end = StrLen(&text[selmax]);
                              //insert new string
   for(dword i=0; txt[i]; i++){
      if(di+len_end >= max_len)
         break;
      buf[di++] = txt[i];
   }
   len_end = Min(len_end, max_len-di);
   cursor_pos = cursor_sel = di;
   txt_first_char = Min(txt_first_char, cursor_pos);
   //if(di<0 || di>=sz) Fatal("bad di");
   MemCpy(buf+di, &text[selmax], len_end*2);
   di += len_end;

   text.clear();
   text.insert(text.begin(), buf, buf+di);
   text.push_back(0);
   delete[] buf;
}

//----------------------------

void C_text_editor_common::SetInitText(const Cstr_w &init_text){

   text.reserve(max_len+1);
   const wchar *wp = init_text;
   cursor_pos = Min(max_len, init_text.Length());
   text.clear();
   text.insert(text.begin(), wp, wp+cursor_pos);
   text.push_back(0);
                              //expand selection to entire text
   txt_first_char = 0;
   cursor_sel = 0;
}

//----------------------------

void C_application_ui::DrawEditedText(const C_text_editor &_te, bool draw_rect){

   const C_text_editor_common &te = (C_text_editor_common&)_te;

   const S_font &fd = font_defs[te.font_index];
   if(draw_rect)
      DrawEditedTextFrame(te.GetRect());

   int selmin = te.GetCursorPos() - te.txt_first_char;
   int selmax = te.GetCursorSel() - te.txt_first_char;
   if(selmin > selmax)
      Swap(selmin, selmax);

   Cstr_w tmp;
   const wchar *wp = te.GetText();
   if(te.IsSecret()){
      tmp = wp;
      text_utils::MaskPassword(tmp, te.mask_password ? -1 : (tmp.Length()-1));
      wp = tmp;
   }

   wp += te.txt_first_char;
   int maxw = te.GetRect().sx - 2;

   selmin = Max(0, selmin);
   selmax = Max(0, selmax);

   int inline_pos = 0, inline_len = te.GetInlineText(inline_pos);

   int x = te.GetRect().x + 1;
   dword col_text = GetColor(COL_TEXT_INPUT);
                     //draw part left from selection
   if(selmin > 0){
      int w;
      if(inline_len && inline_pos<selmin){
         assert(inline_pos+inline_len <= selmin);
         w = 0;
         if(inline_pos)
            w = DrawString(wp, x, te.GetRect().y, te.font_index, te.font_flags, col_text, inline_pos*sizeof(wchar)) + 1;
         w += DrawString(wp+inline_pos, x+w, te.GetRect().y, te.font_index, te.font_flags | FF_UNDERLINE, col_text, (selmin-inline_pos)*sizeof(wchar));
      }else
         w = DrawString(wp, x, te.GetRect().y, te.font_index, te.font_flags, col_text, selmin*sizeof(wchar)) + 1;
      wp += selmin;
      x += w;
      maxw -= w;
   }
   int cursor_x = x;

                     //draw selection
   if(selmax != selmin){
      int sw = GetTextWidth(wp, te.font_index, te.font_flags, selmax-selmin) + 1;
      if(sw > maxw)
         sw = maxw;
      FillRect(S_rect(x, te.GetRect().y, sw, fd.cell_size_y+1), GetColor(COL_TEXT_SELECTED_BGND));
      DrawString(wp, x, te.GetRect().y, te.font_index, te.font_flags, GetColor(COL_TEXT_SELECTED), -maxw);
      wp += selmax-selmin;
      x += sw;
      maxw -= sw;
      if(te.GetCursorPos() > te.GetCursorSel())
         cursor_x = x;
   }
                  //draw part right from selection
   if(*wp && maxw){
      if(inline_len && inline_pos+inline_len > selmax){
         assert(0);
      }
      DrawString(wp, x, te.GetRect().y, te.font_index, te.font_flags, col_text, -maxw);
   }
                  //draw flashing cursor
   if(te.IsCursorVisible()){
      dword size = (1+fd.extra_space)*2;
      FillRect(S_rect(cursor_x-1, te.GetRect().y, size, fd.cell_size_y+1), 0xffff0000);
   }
}

//----------------------------

void C_application_ui::MakeSureCursorIsVisible(C_text_editor &_te) const{

   C_text_editor_common &te = (C_text_editor_common&)_te;

   assert(te.font_index<NUM_FONTS);
   const S_font &fdi = font_defs[te.font_index];
   te.txt_first_char = Max(0, Min(te.txt_first_char, te.GetCursorPos()-4));
   Cstr_w tmp;
   const wchar *wp = te.GetText();
   if(te.IsSecret()){
      tmp = wp;
      text_utils::MaskPassword(tmp, te.mask_password ? -1 : (tmp.Length()-1));
      wp = tmp;
   }
   while(wp[te.txt_first_char]){
      int max_l = te.GetCursorPos() - te.txt_first_char;
      int w = (!max_l ? 0 : GetTextWidth(wp+te.txt_first_char, te.font_index, te.font_flags, max_l)) + fdi.letter_size_x;
      if(w < te.GetRect().sx-fdi.letter_size_x)
         break;
      ++te.txt_first_char;
   }
}

//----------------------------

bool C_application_ui::ProcessMouseInTextEditor(C_text_editor &_te, const S_user_input &ui){

#ifdef USE_MOUSE
   C_text_editor_common &te = (C_text_editor_common&)_te;

   te.ProcessInput(ui);

   if(!(te.GetInitFlags()&TXTED_ALLOW_EOL)){
      bool set_pos = false, set_sel = false, sel_word = false;

      if(te.mouse_selecting){
         if(ui.mouse_buttons&MOUSE_BUTTON_1_DRAG){
            set_pos = true;
         }
         if(ui.mouse_buttons&MOUSE_BUTTON_1_UP){
            te.mouse_selecting = false;
            if(ui.CheckMouseInRect(te.GetRect())){
               if(te.GetCursorPos()==te.GetCursorSel() || te.mask_password){
                  te.InvokeInputDialog();
               }
            }
         }
      }else
      if(ui.mouse_buttons&MOUSE_BUTTON_1_DOWN){
         if(ui.CheckMouseInRect(te.GetRect())){
                                 //set active selection, and start multi-selection
            if(ui.is_doubleclick){
               te.mouse_selecting = false;
               sel_word = true;
            }else{
               te.mouse_selecting = true;
               set_pos = set_sel = true;
            }
         }
      }
      if(set_pos || set_sel || sel_word){
                                 //determine character offset
         int xx = ui.mouse.x - te.GetRect().x - 2;
                                 //ignore clicks out of left border - MakeSure_CursorIsVisible will automatically put left part into view
         xx = Max(0, xx);
         const wchar *txt = te.GetText();
         const wchar *wp = txt + te.txt_first_char;
         int best_i = 0, best_w = 0x7fffffff;
         for(int i=0; ; i++){
            int w = !i ? 0 : GetTextWidth(wp, te.font_index, te.font_flags, i);
            int delta = xx-w;
            if(delta>0){
               if(delta>best_w)
                  break;
            }else
               delta = -delta;
            if(best_w>delta){
               best_w = delta;
               best_i = i;
            }
            if(!wp[i])
               break;
         }
         best_i += te.txt_first_char;
         if(sel_word){
            wp = txt;
            int n, x;
            for(n=best_i; n && wp[n-1]!=' '; --n);
            for(x=best_i; wp[x] && wp[x++]!=' '; );
            te.SetCursorPos(x, n);
         }else{
            assert(set_pos);
            te.SetCursorPos(best_i, set_sel ? best_i : te.GetCursorSel());
         }
         te.ResetCursor();
         MakeSureCursorIsVisible(te);
         return true;
      }
   }
   bool ret = false;

#ifdef _WIN32_WCE
#ifdef _WIN32_WCE
   if(!IsWMSmartphone())
#else
   if(HasMouse())
#endif
   {
      bool in = ui.CheckMouseInRect(but_keyboard.GetRect());
      if(ui.mouse_buttons&MOUSE_BUTTON_1_DOWN){
         but_keyboard.clicked = but_keyboard.down = in;
         if(in)
            ret = true;
      }
      if(ui.mouse_buttons&MOUSE_BUTTON_1_DRAG){
         but_keyboard.down = in;
         if(in)
            ret = true;
      }
      if(ui.mouse_buttons&MOUSE_BUTTON_1_UP){
         if(but_keyboard.clicked){
            but_keyboard.clicked = but_keyboard.down = false;
            if(in){
#ifdef _WIN32_WCE
               C_text_editor_common &tew = (C_text_editor_common&)te;
               tew.last_sip_on = !tew.last_sip_on;
               UpdateGraphicsFlags(tew.last_sip_on ? IG_SCREEN_SHOW_SIP : 0, IG_SCREEN_SHOW_SIP);
               tew.want_keyboard = tew.last_sip_on;
#endif
            }
            ret = true;
         }
      }
   }
#endif
   return ret;
#else
   return false;
#endif//USE_MOUSE
}


//----------------------------

//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include "..\Common\UserInterface\TextEditorCommon.h"

//----------------------------

namespace win{
#include <Windows.h>
#include <ctype.h>
#undef GetCharWidth
#ifdef _WIN32_WCE
//#define USE_RICHINK

#include <TpcShell.h>
#include <WinUserM.h>
#include <AygShell.h>
#ifdef USE_RICHINK
#include <RichInk.h>
#endif
#endif
}

//----------------------------

void C_text_editor::ClipboardCopy(const wchar *text){
   if(win::OpenClipboard(NULL)){
      win::EmptyClipboard();
      dword sz = StrLen(text)+1;
      win::HGLOBAL hg = win::GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, sz*sizeof(wchar));
#ifdef UNDER_CE
      void *mem = hg;
#else
      void *mem = win::GlobalLock(hg);
#endif
      MemCpy(mem, text, sz*sizeof(wchar));
#ifndef UNDER_CE
      win::GlobalUnlock(hg);
#endif
      win::SetClipboardData(CF_UNICODETEXT, hg);
      win::CloseClipboard();
   }
}

//----------------------------

Cstr_w C_text_editor::ClipboardGet(){

   Cstr_w ret;
   if(win::OpenClipboard(NULL)){
      win::HANDLE h = win::GetClipboardData(CF_UNICODETEXT);
      if(h){
#ifdef UNDER_CE
         const wchar *mem = (wchar*)h;
#else
         const wchar *mem = (wchar*)win::GlobalLock(h);
#endif
         dword sz = win::GlobalSize(h)/sizeof(wchar);
         while(sz && !mem[sz-1])
            --sz;
         ret.Allocate(mem, sz);
#ifndef UNDER_CE
         win::GlobalUnlock(h);
#endif
      }
      win::CloseClipboard();
   }
   return ret;
}

//----------------------------

class C_text_editor_win32: public C_text_editor_common{
   //static const int WM_REFRESH = WM_USER+101;

   win::HWND hwnd_ig;
   win::HWND hwnd_ctrl;
   dword timer_id;
   bool activated;
   bool text_dirty;

   win::WNDPROC save_ig_proc;
   win::WNDPROC save_ctrl_proc;

   bool high_res;

   static C_text_editor_win32 *active_this;

   dword saved_ig_flags;
   bool shift_down;

//----------------------------

   bool SetSelFromWin(){

      int sel = 0, pos = 0;
      win::SendMessage(hwnd_ctrl, EM_GETSEL, (win::WPARAM)&sel, (win::LPARAM)&pos);
      if((cursor_sel!=sel || cursor_pos!=pos) && !mask_password){
         if(cursor_sel==pos || cursor_pos==sel)
            Swap(sel, pos);
         if(cursor_sel!=sel || cursor_pos!=pos){
            cursor_sel = sel;
            cursor_pos = pos;
            ResetCursor();
            return true;
         }
      }
      return false;
   }

//----------------------------

   void CorrectText(bool detect){

      dword len = win::SendMessage(hwnd_ctrl, WM_GETTEXTLENGTH, 0, 0);
      if(mask_password && !len)
         return;
#ifdef USE_RICHINK
      if(detect && len==dword(text.size()-1)){
         C_buffer<wchar> tmp;
         tmp.Resize(len+1);
         SendMessageW(hwnd_ctrl, WM_GETTEXT, len, (win::LPARAM)tmp.Begin());
         if(!MemCmp(tmp.Begin(), text.begin(), len*sizeof(wchar)))
            return;
         MemCpy(text.begin(), tmp.Begin(), len*sizeof(wchar));
      }else{
         text.resize(len+1);
         win::SendMessage(hwnd_ctrl, WM_GETTEXT, len, (win::LPARAM)text.begin());
      }
      text[len] = 0;
#else
      if(detect && len==dword(text.size()-1)){
         C_buffer<wchar> tmp;
         tmp.Resize(len+1);
         SendMessageW(hwnd_ctrl, WM_GETTEXT, len+1, (win::LPARAM)tmp.Begin());
         if(!MemCmp(tmp.Begin(), text.begin(), (len+1)*sizeof(wchar)))
            return;
         MemCpy(text.begin(), tmp.Begin(), (len+1)*sizeof(wchar));
      }else{
         text.resize(len+1);
         win::SendMessage(hwnd_ctrl, WM_GETTEXT, len+1, (win::LPARAM)text.begin());
         text.begin()[len] = 0;
      }
#endif
      mask_password = false;
      text_dirty = true;
      SetSelFromWin();
      ResetCursor();
   }

//----------------------------

   win::LRESULT IgraphWindowProc(win::HWND hwnd, win::UINT msg, win::WPARAM wParam, win::LPARAM lParam){

      win::WNDPROC save_ig_proc = this->save_ig_proc; //keep local, this class may be destroyed in this func
      switch(msg){
      case WM_COMMAND:
         switch(word(wParam>>16)){
         case EN_UPDATE:
            if(activated){
               CorrectText(false);
               text_dirty = false;
               TextEditNotify(false, false, true);
            }
            break;
         }
         break;
#ifdef _WIN32_WCE
      case WM_HOTKEY:
                           //special care for back key - send to control
         if(IsWMSmartphone())
         if(word(lParam>>16)==VK_ESCAPE){
            //return win::SendMessage(hwnd_ctrl, msg, wParam, lParam);
            //return win::SendMessage(hwnd_ctrl, WM_KEYDOWN, VK_BACK, 0);
            win::SHSendBackToFocusWindow(msg, wParam, lParam);
            return 0;
         }
         break;
#endif
      }
      win::LRESULT ret = win::CallWindowProc(save_ig_proc, hwnd, msg, wParam, lParam);

      switch(msg){
      case WM_ACTIVATE:
         if(word(wParam&0xffff)!=WA_INACTIVE)
            win::SetFocus(hwnd_ctrl);
         break;
      }
      return ret;
   }
   static win::LRESULT CALLBACK IgraphWindowProcThunk(win::HWND hwnd, win::UINT msg, win::WPARAM wParam, win::LPARAM lParam){
      //return DefWindowProc(hwnd, msg, wParam, lParam);
      return active_this->IgraphWindowProc(hwnd, msg, wParam, lParam);
   }

//----------------------------

   win::LRESULT CtrlWindowProc(win::HWND hwnd, win::UINT msg, win::WPARAM wParam, win::LPARAM lParam){

      switch(msg){
      case WM_KEYDOWN:
      case WM_KEYUP:
         switch(wParam){
         case VK_RETURN:
            if(!(te_flags&TXTED_ALLOW_EOL) || IsReadOnly())
               return CallWindowProc(save_ig_proc, hwnd_ig, msg, wParam, lParam);
            return 0;
         case VK_SHIFT:
         case VK_LSHIFT:
         case VK_RSHIFT:
            shift_down = (msg==WM_KEYDOWN);
            return CallWindowProc(save_ig_proc, hwnd_ig, msg, wParam, lParam);

         case VK_UP:
         case VK_DOWN:
            if(!(te_flags&TXTED_ALLOW_EOL)){
               if(shift_down){
                  SetCursorPos(wParam==VK_UP ? 0 : text.size()-1, cursor_sel);
                  ResetCursor();
                  TextEditNotify(false, true, false);
                  break;
               }
            }
            if(te_flags&TXTED_REAL_VISIBLE){
               dword sel0, sel1;
               win::SendMessage(hwnd, EM_GETSEL, (win::WPARAM)&sel0, (win::LPARAM)&sel1);
               if(sel0!=sel1)
                  break;
               int num_lines = win::SendMessage(hwnd, EM_GETLINECOUNT, 0, 0);
               if(wParam==VK_UP){
                              //check if we're on 1st line
                  if(num_lines>1 && int(sel0)>=win::SendMessage(hwnd, EM_LINELENGTH, 0, 0))
                     break;
               }else{
                  if(num_lines>1 && int(sel0)<win::SendMessage(hwnd, EM_LINELENGTH, num_lines-1, 0))
                     break;
               }
            }
         case VK_F1: case VK_F2: case VK_F3: case VK_F4: case VK_F5: case VK_F6: case VK_F7: case VK_F8: case VK_F9: case VK_F10: case VK_F11: case VK_F12:
#ifdef _WINDOWS
         case VK_ESCAPE:
         case VK_PRIOR:
         case VK_NEXT:
         case VK_TAB:
#endif
            return CallWindowProc(save_ig_proc, hwnd_ig, msg, wParam, lParam);
         case VK_DELETE:
            if(mask_password){
               mask_password = false;
               SetInitText(NULL);
            }
            /*
            else
            if(!GetTextLength())
               return CallWindowProc(save_ig_proc, hwnd_ig, msg, wParam, lParam);
               */
            break;
            /*
         case VK_LEFT:
         case VK_RIGHT:
            if(!GetTextLength())
               return CallWindowProc(save_ig_proc, hwnd_ig, msg, wParam, lParam);
            break;
            */
         }
         break;
#ifdef _DEBUG
      case WM_SYSKEYDOWN:
      case WM_SYSKEYUP:
         if(wParam==VK_F10){
            return CallWindowProc(save_ig_proc, hwnd_ig, msg, wParam, lParam);
         }
         break;
#endif
         //*
      case WM_CHAR:
                              //process end-of-line here, because Windows enters \r\n as eol
         if(wParam=='\r'){
            if((te_flags&TXTED_ALLOW_EOL) && !IsReadOnly()){
               if(!(te_flags&TXTED_REAL_VISIBLE)){
                  ReplaceSelection(L"\n");
                  TextEditNotify(false, false, true);
                  return 0;
               }else
                  return CallWindowProc(save_ctrl_proc, hwnd, msg, wParam, lParam);
            }
            return CallWindowProc(save_ig_proc, hwnd_ig, msg, wParam, lParam);
         }else
         if(wParam=='\t'){
            return 0;
         }
         break;
         /**/

      case WM_TIMER:
         {
               /*
#ifdef _WIN32_WCE
            if(!IsWMSmartphone() && want_keyboard!=-1){
               //win::SetWindowPos(hwnd_ig, b ? HWND_NOTOPMOST : HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
               //UpdateWindow(hwnd_ig);
               if(want_keyboard){
                  win::SHSipPreference(hwnd_ig, C_text_editor_win32::last_sip_on ? win::SIP_UP : win::SIP_DOWN);
               }else{
                  win::SHSipPreference(hwnd_ig, win::SIP_DOWN);
               }
               want_keyboard = -1;
            }
#endif
            /**/
            if(win::GetFocus()!=hwnd_ctrl)
               win::SetFocus(hwnd_ctrl);
            bool cm = SetSelFromWin();
            //if(cm)
               CorrectText(true);
            bool cf = TickCursor();
            if(cm || cf || text_dirty){
               TextEditNotify(cf, cm, text_dirty);
               text_dirty = false;
            }
         }
         break;
      }
      return CallWindowProc(save_ctrl_proc, hwnd, msg, wParam, lParam);
   }
   static win::LRESULT CALLBACK CtrlWindowProcThunk(win::HWND hwnd, win::UINT msg, win::WPARAM wParam, win::LPARAM lParam){
      return active_this->CtrlWindowProc(hwnd, msg, wParam, lParam);
   }

//----------------------------

   static void CALLBACK CloseTimerProc(win::HWND hwnd, win::UINT uMsg, win::UINT idEvent, win::DWORD dwTime){
   }

//----------------------------

   virtual void Activate(bool b){

      if(b==activated)
         return;

      const int CHANGE_IG_FLAGS = IG_SCREEN_USE_DSA|IG_SCREEN_SHOW_SIP;
      if(b){
         assert(!active_this);
         active_this = this;

         saved_ig_flags = app->GetGraphicsFlags();
         dword flg = CHANGE_IG_FLAGS & ~IG_SCREEN_USE_DSA;
#ifdef _WIN32_WCE
         if(!last_sip_on)
            flg &= ~IG_SCREEN_SHOW_SIP;
#endif
         app->UpdateGraphicsFlags(flg, CHANGE_IG_FLAGS);

         save_ig_proc = (win::WNDPROC)win::SetWindowLong(hwnd_ig, GWL_WNDPROC, (win::LPARAM)IgraphWindowProcThunk);
         save_ctrl_proc = (win::WNDPROC)win::SetWindowLong(hwnd_ctrl, GWL_WNDPROC, (win::LPARAM)CtrlWindowProcThunk);
         //EnableWindow(hwnd_ctrl, true);
         ResetCursor();
         if(!(te_flags&TXTED_REAL_VISIBLE)){
            timer_id = win::SetTimer(hwnd_ctrl, 1, 50, NULL);
         }else{
            win::EnableWindow(hwnd_ctrl, true);
         }
         win::SetFocus(hwnd_ctrl);
      }else{
         if(!(te_flags&TXTED_REAL_VISIBLE)){
            win::KillTimer(hwnd_ctrl, timer_id);
         }else{
            win::EnableWindow(hwnd_ctrl, false);
         }
#ifdef _WIN32_WCE
         win::SIPINFO si;
         MemSet(&si, 0, sizeof(si));
         si.cbSize = sizeof(si);
         //if(want_keyboard==-1)
         //if(win::SipGetInfo(&si))
            //last_sip_on = (si.fdwFlags&SIPF_ON);
            last_sip_on = (app->GetGraphicsFlags()&IG_SCREEN_SHOW_SIP);
#endif
         assert(active_this);
         win::SetWindowLong(hwnd_ctrl, GWL_WNDPROC, (win::LPARAM)save_ctrl_proc);
         save_ctrl_proc = NULL;
         win::SetWindowLong(hwnd_ig, GWL_WNDPROC, (win::LPARAM)save_ig_proc);
         save_ig_proc = NULL;
         active_this = NULL;
         //EnableWindow(hwnd_ctrl, false);

         app->UpdateGraphicsFlags(saved_ig_flags & ~IG_SCREEN_SHOW_SIP, CHANGE_IG_FLAGS);

         win::SetFocus(hwnd_ig);
#ifdef _WIN32_WCE
         //if(!IsWMSmartphone())
            //win::SHSipPreference(hwnd_ig, win::SIP_DOWN);
#endif
         //win::SetTimer(NULL, 0, 500, CloseTimerProc);
      }
#ifdef _WIN32_WCE
      want_keyboard = b;
#endif
      activated = b;
      C_text_editor_common::Activate(b);
   }

//----------------------------

   virtual void SetRect(const S_rect &_rc){
      C_text_editor_common::SetRect(_rc);
      if(te_flags&TXTED_REAL_VISIBLE){
         if(app->system_font.use){
            win::HFONT hf = (win::HFONT)app->system_font.font_handle[0][0][0][font_index];
            win::SendMessage(hwnd_ctrl, WM_SETFONT, win::WPARAM(hf), false);
         }
         win::SetWindowPos(hwnd_ctrl, 0, rc.x, rc.y, rc.sx, rc.sy, SWP_NOREPOSITION|SWP_NOZORDER);
      }
   }

//----------------------------

   virtual void SetCursorPos(int pos, int sel){

      if(mask_password)
         return;
      C_text_editor_common::SetCursorPos(pos, sel);
      if(!mask_password){
         win::SendMessage(hwnd_ctrl, EM_SETSEL, sel, pos);
      }
   }

//----------------------------

   virtual void SetCase(dword allowed, dword curr){
      C_text_editor_common::SetCase(allowed, curr);
#if defined _WIN32_WCE
      dword flg;
      if(te_flags&TXTED_NUMERIC){
         flg = EIM_NUMBERS;
      }else
      if(te_flags&TXTED_ALLOW_PREDICTIVE)
         flg = EIM_TEXT;
      else
         flg = EIM_SPELL;
      switch(curr){
      case CASE_UPPER: flg |= IMMF_SETCLR_CAPSLOCK; break;
      case CASE_CAPITAL: flg |= IMMF_SETCLR_SHIFT; break;
      }
      win::SendMessage(hwnd_ctrl, EM_SETINPUTMODE, 0, flg);
#endif
   }

//----------------------------
#ifdef _DEBUG_
   static int CALLBACK EditWordBreakProc(win::LPTSTR lpch, int ichCurrent, int cch, int code){
      return true;
   }
#endif
//----------------------------
public:

   C_text_editor_win32(C_application_ui *app, dword flg, int fi, dword ff, dword ml, C_text_editor::C_text_editor_notify *_not):
      C_text_editor_common(app, flg, fi, ff, _not),
      activated(false),
      text_dirty(false),
      shift_down(false),
      hwnd_ctrl(NULL)
   {
      max_len = ml;
#ifdef _WIN32_WCE
      want_keyboard = -1;
#endif
#ifdef USE_RICHINK
      win::InitRichInkDLL();
#endif
      high_res = (app->ScrnSX()>=480 || app->ScrnSY()>=480);
      hwnd_ig = (win::HWND)app->GetIGraph()->GetHWND();
      dword wstyle = WS_BORDER | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL;
      if(te_flags&TXTED_REAL_VISIBLE){
         wstyle |= WS_VISIBLE;
         if(te_flags&TXTED_ALLOW_EOL)
            wstyle |= WS_VSCROLL;
      }
      if(te_flags&TXTED_ALLOW_EOL)
         wstyle |= ES_AUTOVSCROLL | ES_MULTILINE | ES_WANTRETURN;
      if(te_flags&TXTED_NUMERIC)
         wstyle |= ES_NUMBER;
      if(te_flags&TXTED_READ_ONLY)
         wstyle |= ES_READONLY;
      hwnd_ctrl = CreateWindowEx(
         0,
#ifdef USE_RICHINK
         WC_RICHINK,
#else
         L"EDIT",
#endif
         L"", wstyle,
#if defined _DEBUG && 0
         10, 40,
#else
         10, 10000,
#endif
         200, 40,
         hwnd_ig,
         NULL,
         NULL,
         NULL);
      /*
#ifdef _WIN32_WCE
      {
         dword eim = EIM_SPELL;
         if(te_flags&TXTED_NUMERIC)
            eim = EIM_NUMBERS;
         else if(te_flags&TXTED_ALLOW_PREDICTIVE)
            eim = EIM_TEXT;
         SendMessage(hwnd_ctrl, EM_SETINPUTMODE, 0, eim);
      }
#endif
      */
      win::SendMessage(hwnd_ctrl, EM_LIMITTEXT, ml, 0);
#ifdef _DEBUG_
      win::SendMessage(hwnd_ctrl, EM_SETWORDBREAKPROC, 0, win::LPARAM(&EditWordBreakProc));
#endif
      /*
      if(te_flags&TXTED_REAL_VISIBLE){
         if(app->system_font.use){
            win::HFONT hf = (win::HFONT)app->system_font.font_handle[0][0][0][fi];
            win::SendMessage(hwnd_ctrl, WM_SETFONT, win::WPARAM(hf), false);
         }
      }
      */
      if(!(flg&TXTED_CREATE_INACTIVE))
         Activate(true);
      SetCase(CASE_ALL, CASE_CAPITAL);
   }

//----------------------------

   ~C_text_editor_win32(){
      Activate(false);
      if(hwnd_ctrl)
         DestroyWindow(hwnd_ctrl);
   }

//----------------------------

   void SetWinText(){

      const wchar *wp = text.begin();
#if 0
                              //convert new-lines
      int len = text.size()-1;
      wchar *buf = new(true) wchar[len*2+1];
      int di = 0;
      while(true){
         wchar c = *wp++;
         if(!c)
            break;
         if(c=='\n'){
            buf[di++] = '\r';
         }
         buf[di++] = c;
      }
      buf[di] = 0;
      win::SendMessage(hwnd_ctrl, WM_SETTEXT, 0, (win::LPARAM)buf);
      delete[] buf;
#else
      win::SendMessage(hwnd_ctrl, WM_SETTEXT, 0, (win::LPARAM)wp);
#endif
   }

//----------------------------

   virtual void SetInitText(const Cstr_w &init_text){

      bool a = activated;
      activated = false;
      C_text_editor_common::SetInitText(init_text);
      int cp = cursor_pos, cs = cursor_sel;
      if(!mask_password)
         SetWinText();
      SetCursorPos(cp, cs);
      activated = a;
   }

//----------------------------

   virtual void ReplaceSelection(const wchar *txt){
      C_text_editor_common::ReplaceSelection(txt);
      SetWinText();
      SetCursorPos(cursor_pos, cursor_sel);
      text_dirty = false;
   }

//----------------------------

   void *GetCtrlHwnd() const{ return hwnd_ctrl; }

//----------------------------

#ifdef _WIN32_WCE

   static win::BOOL CALLBACK CCPDlg(win::HWND hwnd, win::UINT msg, win::WPARAM wParam, win::LPARAM lParam){

      switch(msg){
      case WM_INITDIALOG:
         win::SetWindowLong(hwnd, GWL_USERDATA, lParam);
         return 0;

      case WM_ERASEBKGND:
         {
            assert(0);
            C_text_editor_win32 *_this = (C_text_editor_win32*)win::GetWindowLong(hwnd, GWL_USERDATA);
            win::HDC hdc = (win::HDC)wParam;
            win::RECT rc = {0, 0, !_this->high_res ? 88:180, !_this->high_res ? 24:48 };
            win::HBRUSH hbr = win::CreateSolidBrush(win::GetSysColor(COLOR_3DFACE));
            win::FillRect(hdc, &rc, hbr);
            win::DeleteObject(hbr);
         }
         return 1;

      case WM_DRAWITEM:
         {
            assert(0);
            win::DRAWITEMSTRUCT *ds = (win::DRAWITEMSTRUCT*)lParam;
            int x = 0;
            if(ds->itemState&ODS_SELECTED)
               ++x;
            win::HBRUSH hbr = win::CreateSolidBrush(win::GetSysColor(COLOR_3DFACE));
            win::FillRect(ds->hDC, &ds->rcItem, hbr);
            win::DeleteObject(hbr);

            int ii = wParam;
            if(ds->itemState&ODS_DISABLED)
               ii += 100;
            C_text_editor_win32 *_this = (C_text_editor_win32*)win::GetWindowLong(hwnd, GWL_USERDATA);
            if(_this->high_res)
               ii += 200;
            win::HICON hicon = win::LoadIcon(win::GetModuleHandle(NULL), (const wchar*)ii);
            win::DrawIcon(ds->hDC, x, x, hicon);
         }
         break;
      case WM_COMMAND:
         {
            C_text_editor_win32 *_this = (C_text_editor_win32*)win::GetWindowLong(hwnd, GWL_USERDATA);
            int id = 0;
            switch(word(wParam)){
            case 1000: case 1200: id = WM_CUT; break;
            case 1001: case 1201: id = WM_COPY; break;
            case 1002: case 1202: id = WM_PASTE; break;
            case 1003: id = WM_UNDO; break;
            }
            if(id)
               win::SendMessage(_this->hwnd_ctrl, id, 0, 0);
         }
         break;
      }
      return 0;
   }

//----------------------------
#endif//_WIN32_WCE

   virtual void Cut(){
      if(!IsSecret())
         win::SendMessage(hwnd_ctrl, WM_CUT, 0, 0);
   }
   virtual void Copy(){
      if(!IsSecret())
         win::SendMessage(hwnd_ctrl, WM_COPY, 0, 0);
   }
   virtual bool CanPaste() const{
      if(mask_password)
         return false;
      return true;
   }
   virtual void Paste(){ win::SendMessage(hwnd_ctrl, WM_PASTE, 0, 0); }

//----------------------------

   virtual void ProcessInput(const S_user_input &ui){
   }

};

C_text_editor_win32 *C_text_editor_win32::active_this;

//----------------------------

#ifdef _WIN32_WCE

bool C_text_editor_common::last_sip_on = false;

#endif

//----------------------------

C_text_editor *C_application_ui::CreateTextEditorInternal(dword flags, int font_index, dword font_flags, const wchar *init_text, dword max_length, C_text_editor::C_text_editor_notify *notify){

   C_text_editor *te = new(true) C_text_editor_win32(this, flags, font_index, font_flags, max_length, notify);
   if(init_text)
      te->SetInitText(init_text);
   return te;
}

//----------------------------

void C_application_ui::DrawTextEditorState(const C_text_editor &te, dword &soft_button_menu_txt_id, dword &soft_button_secondary_txt_id, int ty){

   if(!GetSoftButtonBarHeight())
      return;
#if (defined _WIN32_WCE || defined _DEBUG)

   if(!text_edit_state_icons){
      C_image *img = C_image::Create(*this);
      text_edit_state_icons = img;
      img->Open(L"txt_icons.png", 0, fdb.cell_size_y+1, C_fixed::One(), CreateDtaFile());
      img->Release();
   }

   dword color = GetColor(ty==-1 ? COL_EDITING_STATE : COL_TEXT_POPUP);
#ifdef _WIN32_WCE
#ifdef _WIN32_WCE
   if(!IsWMSmartphone())
#else
   if(HasMouse())
#endif
   {
      DrawSoftButtonRectangle(but_keyboard);
      const S_rect &rc = but_keyboard.GetRect();
      int SZ = text_edit_state_icons->SizeY();
      S_rect rc_icon(SZ*7, 0, SZ, SZ);
      int x = rc.x + (rc.sx-SZ)/2;
      int y = rc.y + (rc.sy-SZ)/2;
      if(but_keyboard.down)
         ++x, ++y;
#ifdef _WIN32_WCE
      C_text_editor_common &tew = (C_text_editor_common&)te;
      if(!tew.last_sip_on)
         color = BlendColor(color, GetColor(COL_TITLE), 0x8000);
#endif
      text_edit_state_icons->DrawSpecial(x, y, &rc_icon, C_fixed::One(), color);
      return;
   }
#endif

   int x = ScrnSX()/2;
   int y = (ty!=-1) ? ty : (ScrnSY()-fdb.line_spacing+1);
   const wchar *cp = NULL;
   dword fflg = FF_ITALIC;

   S_rect rc_icon(0, 0, 0, text_edit_state_icons->SizeY());
   S_rect rc_icon1 = rc_icon;
#ifdef _WIN32_WCE
   C_text_editor_win32 &tew = (C_text_editor_win32&)te;
   win::HWND hc = (win::HWND)tew.GetCtrlHwnd();
   dword im = win::SendMessage(hc, EM_GETINPUTMODE, 0, true);
   bool shift = (im&IMMF_SHIFT);
   bool caps = (im&IMMF_CAPSLOCK);
   switch(im&EIM_MASK){
   case EIM_SPELL:
      rc_icon.sx = rc_icon.sy*2;
      cp = shift||caps ? L"ABC" : L"abc";
      if(caps)
         fflg |= FF_UNDERLINE;
      break;
   case EIM_AMBIG:
      rc_icon.sx = rc_icon.sy*2;
      rc_icon.x += rc_icon.sx;
      cp = shift||caps ? L"ABC" : L"abc";
      if(caps)
         fflg |= FF_UNDERLINE;
      break;
   case EIM_NUMBERS:
      rc_icon.sx = rc_icon.sy*2;
      cp = L"123";
      break;
   }
#elif defined _DEBUG && defined _WINDOWS
   cp = L"Abc";
   rc_icon.sx = rc_icon.sy*2;
#endif
   if(cp){
      DrawString(cp, x, y, UI_FONT_BIG, fflg, color);
   }else
   if(rc_icon1.sx)
      text_edit_state_icons->DrawSpecial(x, y, &rc_icon1, C_fixed::One(), color);
   if(rc_icon.sx)
      text_edit_state_icons->DrawSpecial(x-2-rc_icon.sx, y, &rc_icon, C_fixed::One(), color);
#endif
}

//----------------------------

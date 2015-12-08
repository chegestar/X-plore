#include <UI\UserInterface.h>
#include <TextUtils.h>

//----------------------------

#ifdef _DEBUG
#define IDLE_COUNTDOWN 1000 * 2
#else
#define IDLE_COUNTDOWN 1000 * 10
#endif

//----------------------------

class C_text_editor_common: public C_text_editor{
   dword cursor_flash_time;
   dword allowed_cases;
   int cursor_flash_count;
   bool cursor_flash_phase;
protected:
   static const int CURSOR_FLASH_COUNT = 300;

   dword max_len;
                              //edited text, including eol '\0' char
   C_vector<wchar> text;
   int cursor_pos;            //cursor position (in characters)
   int cursor_sel;            //cursor selection beginning (in characters)
   dword te_flags;
   bool cursor_on;
   bool secret;
   int idle_countdown;        //counter for being idle, so that we may stop cursor flashing

   C_application_ui *app;
   C_text_editor::C_text_editor_notify *notify;

   void TextEditNotify(bool cursor_blink, bool cursor_moved, bool text_changed){
      AddRef();
      if(notify){
         if(cursor_blink)
            notify->CursorBlinked();
         if(cursor_moved)
            notify->CursorMoved();
         if(text_changed)
            notify->TextChanged();
      }
      app->TextEditNotify(this, cursor_blink, cursor_moved, text_changed);
      Release();
   }

   C_text_editor_common(C_application_ui *_app, dword tf, int _font_index, dword _font_flags, C_text_editor::C_text_editor_notify *_nf):
      C_text_editor(tf, _font_index, _font_flags),
      notify(_nf),
      max_len(1000),
      allowed_cases(CASE_ALL),
      cursor_on(true),
      cursor_pos(0),
      cursor_sel(0),
      te_flags(tf),
      mouse_selecting(false),
      txt_first_char(0),
      secret(tf&TXTED_SECRET),
      mask_password(tf&TXTED_SECRET),
      app(_app)
   {
      ResetCursor();
      SetInitText(Cstr_w());
      rc = S_rect(10, 0, app->ScrnSX()-20, 20);
   }

//----------------------------
// Tick cursor. Returns true if screen should be redrawn.
   bool TickCursor(){
#ifndef DEBUG_EDWIN
      if(!idle_countdown)
         return false;

      dword t = GetTickTime();
      dword time = t - cursor_flash_time;
      cursor_flash_time = t;
      if((idle_countdown -= time) <= 0){
                                 //too long idle, disable cursor flash
                                 // ... and leave it visible, and reset after re-activation
         ResetCursor();
         idle_countdown = 0;
         return true;
      }
      if(cursor_on){
                                 //flash cursor
         if((cursor_flash_count -= time) <= 0){
            cursor_flash_count = CURSOR_FLASH_COUNT;
            cursor_flash_phase = !cursor_flash_phase;
            return true;
         }
      }
#endif
      return false;
   }

//----------------------------

   virtual bool IsReadOnly() const{ return (GetInitFlags()&TXTED_READ_ONLY); }
public:
   void ResetCursor(){
      cursor_flash_count = CURSOR_FLASH_COUNT;
      cursor_flash_phase = true;
      idle_countdown = IDLE_COUNTDOWN;
      cursor_flash_time = GetTickTime();
   }

//----------------------------

   virtual void Activate(bool b){
      if(!b)
         cursor_flash_phase = false;
   }

//----------------------------
   virtual bool IsSecret() const{ return secret; }
   virtual dword GetMaxLength() const{ return max_len; }

   virtual void ReplaceSelection(const wchar *txt);
   virtual void SetInitText(const Cstr_w &init_text);
   virtual void SetInitText(const char *init_text){
      Cstr_w tmp;
      tmp.Copy(init_text);
      SetInitText(tmp);
   }
   virtual void SetCase(dword allowed, dword curr){
      allowed_cases = allowed;
   }
   virtual bool IsCursorVisible() const{ return (cursor_on && cursor_flash_phase); }
   virtual int GetCursorPos() const{ return cursor_pos; }
   virtual int GetCursorSel() const{ return cursor_sel; }
   void SetCursorPos(int pos, int sel){

      assert(dword(pos)<=GetTextLength());
      assert(dword(sel)<=GetTextLength());
      cursor_pos = pos;
      cursor_sel = sel;
      ResetCursor();
   }
   virtual dword GetInitFlags() const{ return te_flags; }

   virtual const wchar *GetText() const{
      return text.begin();
   }

   virtual dword GetTextLength() const{
      return text.size()-1;
   }

//----------------------------

   bool mouse_selecting;
                              //used for single-line editor:
   int txt_first_char;        //first visible character
   bool mask_password;

#ifdef _WIN32_WCE
   int want_keyboard;
   static bool last_sip_on;
#endif
};

//----------------------------

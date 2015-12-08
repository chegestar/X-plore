// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <UI\FatalError.h>

//----------------------------
                              //specially encoded strings, so that they're not found in executable

                              //Damaged executable. Please re-install the application, or contact software vendor.
static const char msg_damage[] = "D7a4mbargge3dw Gelx8eYcru,t[a6bylled.b !P4lEeeaFsbe, 'r3e$- ifnbsdtraalcl4 GtHhhef qalp^p7lOihcnantyiYo4nR,t ,o:r' -c|ojnhttaecrtv bstoyf6t3w4atrteg jvdewn4d5o6rh.n";

                              //Fatal system exception occured. Device will be restarted.\n\nReason:
static const char msg_fatal[] = "Fla9t$adle %sby set$ermg heOxycEe2pQtdirognH jotcrc[u]r]e+di.g R"
#if !defined __SYMBIAN_3RD__ && !defined ANDROID
   "DgeEveifccee hwjiel2ld fbber erFers*t6aerHteesdg.e"
#endif
   "\n)\n&RkeTaDsfognn:?";

//----------------------------

class C_mode_fatal_error: public C_mode_app<>{
   virtual bool WantInactiveTimer() const{
      return true;
   }
   static void DecodeSecretText(const char *cp, Cstr_c &str){

      int len = StrLen(cp) / 2;
      str.Allocate(NULL, len);
      for(int i=0; i<len; i++)
         str.At(i) = cp[i*2];
   }
public:
   int reset_counter;
   dword err_code;

   C_mode_fatal_error(t_type &_app, dword ec):
      C_mode_app<>(_app),
      reset_counter(10000),
      err_code(ec)
   {
      app.CreateTimer(*this, 1000);
      app.ActivateMode(*this);
   }
   virtual void InitLayout(){ }
   virtual void ProcessInput(S_user_input &ui, bool &redraw){ }
   virtual void Tick(dword time, bool &redraw){
      if((reset_counter -= time) <= 0 || !app.IsFocused()){
         DeleteTimer();
         if(system::CanRebootDevice())
            system::RebootDevice();
#ifdef _DEBUG
         Fatal("reset");
#endif
      }

   }
   virtual void Draw() const;
};

//----------------------------

void CreateModeFatalError(C_application_ui *cl, dword err_code){
   new(true) C_mode_fatal_error(*cl, err_code);
}

//----------------------------

void C_mode_fatal_error::Draw() const{

   app.ClearScreen(0xff0000ff);

   S_text_display_info tdi;
   tdi.rc = S_rect(app.fdb.letter_size_x/2, app.fdb.line_spacing/2, app.ScrnSX()-app.fdb.letter_size_x, app.ScrnSY());
   Cstr_c s;
   DecodeSecretText(msg_fatal, s);
   tdi.body_c = s;
   tdi.is_wide = false;
   tdi.body_c<<"\n\n";

   DecodeSecretText(msg_damage, s);
   tdi.body_c<<s;

   tdi.body_c<<"\n\nError code: ";
   tdi.body_c<<(char)CC_TEXT_COLOR <<"aaa" <<(char)0 <<(char)0 <<(char)0 <<(char)CC_TEXT_COLOR;
   tdi.body_c<<err_code;
   tdi.body_c.Build();

   tdi.ts.font_index = app.UI_FONT_BIG;
   tdi.ts.text_color = 0xffffffff;
   app.CountTextLinesAndHeight(tdi, app.UI_FONT_BIG);
   tdi.rc.sy = tdi.total_height;

   app.DrawFormattedText(tdi);

   app.SetScreenDirty();
}

//----------------------------

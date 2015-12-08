//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//
// Display About screen.
// Optionally this mode processes 3-digit key codes which perform secret or testing functions of application.
//----------------------------

#ifndef __ABOUT_H__
#define __ABOUT_H__

#include <UI\UserInterface.h>

//----------------------------
                              //class used to process secret key code presses (3 digits) in About mode for controlling special application behavior
class C_about_keystroke_callback{
public:
//----------------------------
// Process actual key code.
// If returned value is true, then this mode is closed after call.
   virtual bool ProcessKeyCode(dword code) = 0;
};

//----------------------------

void CreateModeAbout(C_application_ui &app, dword txt_id_about, const wchar *title, const wchar *logo_filename, int app_version_hi, int app_version_lo,
   char app_build, C_about_keystroke_callback *ksc = NULL, const char *_build_date = __DATE__);

//----------------------------
#endif

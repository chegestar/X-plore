//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//
// Popup window with formatted informational text.
// Showing two softkey buttons.
// Optional image is shown in right top corner.
//----------------------------

#ifndef __INFO_MESSAGE_H_
#define __INFO_MESSAGE_H_

#include <UI\UserInterface.h>

enum E_INFO_MESSAGE_IMAGE{
   INFO_MESSAGE_IMG_SUCCESS,
   INFO_MESSAGE_IMG_EXCLAMATION,
   INFO_MESSAGE_IMG_QUESTION,
};

class C_mode_info_message: public C_mode_dialog_base{
   typedef C_mode_dialog_base super;
   //virtual dword GetMenuSoftKey() const{ return 0; }
   //virtual dword GetSecondarySoftKey() const{ return 0; }
   C_ctrl_image *ctrl_img;
protected:
public:
   static const char *GetImageName(E_INFO_MESSAGE_IMAGE img);
   static const dword ID = FOUR_CC('I','M','S','G');

   S_text_display_info tdi;

   C_mode_info_message(C_application_ui &app, const Cstr_w &_title, const wchar *msg, dword ls, dword rs, const char *img_name);
   virtual void InitLayout();
   virtual void ProcessInput(S_user_input &ui, bool &redraw);
   virtual void Draw() const;
};

//----------------------------
// Create info message with predefined color and icon.
void CreateInfoMessage(C_application_ui &app, dword title_txt_id, const wchar *msg, E_INFO_MESSAGE_IMAGE img = INFO_MESSAGE_IMG_SUCCESS);

//----------------------------
// Create info message with custom color and icon. Icon image is specified by filename (without .png extension) with alpha channel.
void CreateInfoMessage(C_application_ui &app, dword title_txt_id, const wchar *msg, const char *img_name);


#endif

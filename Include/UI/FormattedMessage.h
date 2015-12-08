//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//
// Popup window with rich text, with scrollbar.
// Showing one softkey buttons.
// Possible callbacks when user confirms or rejects the dialog.
//----------------------------

#ifndef __FORMATTED_MESSAGE_H_
#define __FORMATTED_MESSAGE_H_

#include <UI\InfoMessage.h>
#include <UI\Question.h>

//----------------------------

class C_mode_formatted_message: public C_mode_dialog_base{
   typedef C_mode_dialog_base super;
   C_question_callback *cb;
   bool del_cb;
protected:
   C_ctrl_rich_text *ctrl;
public:

   C_mode_formatted_message(C_application_ui &app, const Cstr_w &t, dword _lsb_txt_id, dword _rsb_txt_id, C_question_callback *cb, bool del_cb);
   ~C_mode_formatted_message();
   virtual void InitLayout();
   virtual void ProcessInput(S_user_input &ui, bool &redraw);

   static void Create(C_application_ui &app, const Cstr_w &title, const C_vector<char> &buf, C_question_callback *cb, bool del_cb, dword lsb_txt_id, dword rsb_txt_id);
};

//----------------------------

void CreateFormattedMessage(C_application_ui &app, const Cstr_w &title, const C_vector<char> &buf,
   C_question_callback *cb = NULL, bool del_cb = false, dword lsb_txt_id = C_application_ui::SPECIAL_TEXT_OK, dword rsb_txt_id = 0);

//----------------------------
#endif

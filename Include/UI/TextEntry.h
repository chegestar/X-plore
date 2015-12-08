//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//
// One-line text entry dialog (in small popup window).
// Showing OK and Cancel soft buttons.
// Callback functions t_TextEntered and t_Canceled are called with currently active mode.
//----------------------------

#ifndef __TEXT_ENTRY_H_
#define __TEXT_ENTRY_H_

#include <UI\UserInterface.h>

class C_text_entry_callback{
public:
   virtual ~C_text_entry_callback(){}
//----------------------------
// Function called when text was successfully entered.
   virtual void TextEntered(const Cstr_w &txt) = 0;
// Called when text entry dialog is canceled.
   virtual void Canceled(){}
};

//----------------------------
// cl = pointer to this
// title_txt_id = text id for title
// cb = callback class on which result will be called
// max_text_len = maximal length of text allowed to be entered
// init_text = text used to initialize text entry
// text_edit_flags = TXTED_* flags
C_mode &CreateTextEntryMode(C_application_ui &app, dword title_txt_id, C_text_entry_callback *cb, bool delete_cb, dword max_text_len, const wchar *init_text = NULL,
   dword text_edit_flags = 0, dword mode_id = 0);

//----------------------------
#endif

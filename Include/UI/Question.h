//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//
// Popup window with question (formatted text) and possibility to answer yes or no.
// Showing two softkey buttons.
//----------------------------

#ifndef __UI_QUESTION_H_
#define __UI_QUESTION_H_

#include <UI\InfoMessage.h>

//----------------------------

class C_question_callback{
public:
   virtual ~C_question_callback(){}
//----------------------------
// Functions called when Yes/No chosen for question.
   virtual void QuestionConfirm() = 0;
   virtual void QuestionReject(){}
};

//----------------------------

void CreateQuestion(C_application_ui &app, dword title_txt_id, const wchar *msg, C_question_callback *cb, bool delete_cb = false, dword lsb_txt_id = C_application_ui::SPECIAL_TEXT_YES, dword rsb_txt_id = C_application_ui::SPECIAL_TEXT_NO);

//----------------------------
#endif

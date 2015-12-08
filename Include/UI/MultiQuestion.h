//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//
// Dialog with multiple options for selecting.
// Showing OK and Cancel soft buttons.
// Callback function is called when selection is made with selection index. Or Cancel is called if dialog is canceled.

//----------------------------
#ifndef __UI_MULTI_SELECTION_H_
#define __UI_MULTI_SELECTION_H_


class C_multi_selection_callback{
public:
   virtual ~C_multi_selection_callback(){}
//----------------------------
// Functions called when user chooses selection.
   virtual void Select(int option) = 0;

//----------------------------
// User canceled the dialog.
   virtual void Cancel(){}
};

//----------------------------
// Create mode for selecting one option of multiple options.
// title_txt_id = text id of title
// msg = additional message displayed on dialog
// options = array of options texts
// num_options = number of options in array
// cb = callback class which methods will be called after selection or canceling dialog
// del_cb = true if cb has to be deleted by multi-question mode after it closes
void CreateMultiSelectionMode(C_application_ui &app, dword title_txt_id, const wchar *msg, const wchar *const options[], dword num_options, C_multi_selection_callback *cb, bool del_cb = false);

//----------------------------
#endif

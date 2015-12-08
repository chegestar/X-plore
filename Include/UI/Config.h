//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//
// Configuration mode.
// This displays configuration screen with options. It modifies fields in a structure.
// Saving of configuration may be done in destructor in child class.
//----------------------------

#ifndef __CONFIG_H_
#define __CONFIG_H_

#include <UI/UserInterface.h>

//----------------------------
                              //configuration item type
enum E_CONFIG_ITEM_TYPE{
   CFG_ITEM_WORD_NUMBER,       //word value in given range, controlled by left/right keys; param = high 16 bits: min range, low 16 bits: max range; param2 = step
   CFG_ITEM_CHECKBOX,          //checkbox for setting either bit in dword, or bool value
   CFG_ITEM_CHECKBOX_INV,      //same as previous, but show inverted checkbox
   CFG_ITEM_ACCESS_POINT,      //                ''

                           //text types:
   CFG_ITEM_TEXTBOX_CSTR,      //Cstr_c or Cstr_w
   CFG_ITEM_PASSWORD,          //same as CFG_ITEM_TEXTBOX_CSTR, but with masked chars
   CFG_ITEM_NUMBER,            //unsigned short; param means number of decimal digits
   CFG_ITEM_SIGNED_NUMBER,     //signed short;    ''

   CFG_ITEM_LAST_BASIC
};

//----------------------------
                              //single item in configuration items array.
struct S_config_item{
   dword ctype;               //E_CONFIG_ITEM_TYPE
   dword txt_id;              //item text (id if < 0x10000, wchar* otherwise)
                              //param:
                              // CFG_ITEM_TEXTBOX, CFG_ITEM_PASSWORD, CFG_ITEM_NUMBER: max length of entered text
                              // CFG_ITEM_CHECKBOX: bit to toggle in dword at offset 'elem_offset'
   int param;
   int elem_offset;           //offset of item in cfg_base
   dword param2;              //text mode: initial case CASE_* + bit3 (0x8) = use TXTED_ALLOW_PREDICTIVE flag; CFG_ITEM_WORD_NUMBER: step or 0=default
   byte is_wide;              //text field; 0=Cstr_c, 1=Cstr_w, 2=Cstr_c in utf-8
   dword hlp_id;              //text (id if < 0x10000, wchar* otherwise) of help text; if set to zero, then help text id = txt_id+1 (help text follows item text)
};

//----------------------------
                              //interface for configuration editor to report changes in configuration
class C_configuration_editing{
public:
   virtual ~C_configuration_editing(){}

   virtual void OnConfigItemChanged(const S_config_item &ec) = 0;

   virtual void OnClose() = 0;

   virtual void OnClick(const S_config_item &ec){}

   virtual void CommitChanges(){}
};
//----------------------------

class C_mode_configuration: public C_mode_app<C_application_ui>, public C_list_mode_base{
   virtual C_application_ui &AppForListMode() const{ return app; }
   virtual bool IsPixelMode() const{ return true; }
   C_smart_ptr<C_text_editor> text_editor;
   int max_textbox_width;  //max width of single-line text editor (in pixels)

   mutable dword last_sk[2];  //last drawn soft keys

protected:
   dword title_text_id;       //title id displayed at title bar
   byte *cfg_base;            //pointer to structure where configuration settings are stored
   C_buffer<S_config_item> options; //items descriptors
   bool modified;             //set if some item was modified (flagged to save the config)
   C_configuration_editing *configuration_editing; //set in constructor, this class doesn't delete it

   virtual int GetNumEntries() const{ return options.Size(); }

//----------------------------
   void PositionTextEditor();

//----------------------------
// Set selection. If 'sel' is negative, EnsureVisible is not called.
   void SetSelection(int sel);

//----------------------------
// Configuration is closed by this method.
   virtual void Close(bool redraw = true);

//----------------------------
// Draw single item's title.
   virtual void DrawItemTitle(const S_config_item &ec, const S_rect &rc_item, dword text_color) const;

//----------------------------
// Draw single item's content.
   virtual void DrawItem(const S_config_item &ec, const S_rect &rc_item, dword text_color) const;

//----------------------------
// Draw left/right arrows on the item.
   void DrawArrows(const S_rect &rc_item, dword color, bool left, bool right) const;

//----------------------------
// Calculate position of checkbox.
   void CalcCheckboxPosition(const S_rect &rc_item, int &x, int &y, int &size) const;

//----------------------------
// Get config item text, and set if left/right arrows should be drawn (false by default).
   virtual bool GetConfigOptionText(const S_config_item &ec, Cstr_w &str, bool &draw_left_arrow, bool &draw_right_arrow) const;

//----------------------------
// Change configuration item from key input. Return true if item was modified.
   virtual bool ChangeConfigOption(const S_config_item &ec, dword key, dword key_bits);

//----------------------------
// Get names (text id) of softkeys,
   virtual dword GetLeftSoftKey(const S_config_item &ec) const{ return 0; }
   virtual dword GetRightSoftKey(const S_config_item &ec) const{ return app.SPECIAL_TEXT_BACK; }
public:
   C_mode_configuration(C_application_ui &app, C_configuration_editing *ce);

   void Init(const S_config_item *opts, dword num_opts, void *_cfg_base, dword tit){
      options.Assign(opts, opts+num_opts);
      cfg_base = (byte*)_cfg_base;
      title_text_id = tit;
   }

   inline const S_config_item *GetOptions() const{ return options.Begin(); }
   virtual void InitLayout();
   virtual void ProcessInput(S_user_input &ui, bool &redraw);
   virtual void TextEditNotify(bool cursor_moved, bool text_changed, bool &redraw);
   virtual void Draw() const;
   virtual void DrawContents() const;
   virtual void SelectionChanged(int old_sel);
   virtual void ScrollChanged(){ if(text_editor) PositionTextEditor(); }

   inline C_text_editor *GetTextEditor(){ return text_editor; }
};

//----------------------------
                              //item type specifier, used for saving item of configuration structure to file
struct S_config_store_type{
   enum E_TYPE{
      TYPE_NULL,              //terminator, last item in array
      TYPE_DWORD,
      TYPE_WORD,
      TYPE_BYTE,
      TYPE_CSTR_C,
      TYPE_CSTR_W,
      TYPE_NEXT_VALS,         //special type, used for chained arrays of items; offset is pointer to next S_config_store_type array
   } type;
   dword offset;              //offset in structure
   const char *name;          //name of item stored in file
};

//----------------------------
// Save configuration from structure to file.
void SaveConfiguration(const wchar *fname, const void *struct_base, const S_config_store_type *vals);

//----------------------------
// Load config from file to structure.
// Returns true if file was successfully opened.
bool LoadConfiguration(const wchar *fname, void *struct_base, const S_config_store_type *vals);

//----------------------------
#endif

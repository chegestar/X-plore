#include "FileBrowser.h"

//----------------------------
class C_client_file_send: 
   //public C_application_ui{
   public C_client{
   class C_mode_this;
   friend class C_mode_this;

   bool SetMode(const C_vector<Cstr_w> &filenames, C_client_file_mgr::C_obex_file_send::E_TRANSFER tr);
   void InitLayout(C_mode_this &mod);
   void Tick(C_mode_this &mod, dword time, bool &redraw);
   void ProcessInput(C_mode_this &mod, S_user_input &ui, bool &redraw);
   void Draw(const C_mode_this &mod);

//----------------------------
                              //file send
   class C_mode_this: public C_mode{
   public:
      C_client_file_mgr::C_obex_file_send *obex_send;
      C_client_file_mgr::C_obex_file_send::E_STATUS last_status;
      mutable S_rect rc;
      mutable C_progress_indicator progress;
      C_vector<Cstr_w> filenames;
      int curr_display_name_index;
      bool is_ir;
      mutable bool redraw_bgnd;

      C_mode_this(C_application_ui &_app, C_mode *sm, C_client_file_mgr::C_obex_file_send *fs):
         C_mode(_app, sm),
         obex_send(fs),
         last_status(C_client_file_mgr::C_obex_file_send::ST_INITIALIZING),
         curr_display_name_index(0),
         redraw_bgnd(true)
      {}
      ~C_mode_this(){
         delete obex_send;
      }
      virtual void InitLayout(){ static_cast<C_client_file_send&>(app).InitLayout(*this); }
      virtual void ProcessInput(S_user_input &ui, bool &redraw){ static_cast<C_client_file_send&>(app).ProcessInput(*this, ui, redraw); }
      virtual void Tick(dword time, bool &redraw){ static_cast<C_client_file_send&>(app).Tick(*this, time, redraw); }
      virtual void Draw() const{ static_cast<C_client_file_send&>(app).Draw(*this); }
   };

   void DrawDialogText(const S_rect &rc, const wchar *txt);
public:
   static bool CreateMode(C_client *cl, const C_vector<Cstr_w> &filenames, C_client_file_mgr::C_obex_file_send::E_TRANSFER tr){
      C_client_file_send *_this = (C_client_file_send*)cl;
      return _this->SetMode(filenames, tr);
   }
};
//----------------------------

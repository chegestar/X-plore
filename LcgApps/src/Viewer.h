#ifndef __VIEWER_H_
#define __VIEWER_H_

//----------------------------

class C_client_viewer: public C_client{
public:
   class C_mode_this;

#ifdef WEB_FORMS
   enum E_FORM_ACTION{
      FORM_CHANGE,
      FORM_CLICK,
   };
// Callback function called on form script commands (onchange, onclick, etc) with associated param. If func returns true the mode was destroyed and should not process furhter.
   typedef bool (C_client::*t_FormAction)(C_mode_this &mod, S_html_form_input *fi, E_FORM_ACTION action, bool &redraw);
#endif

   struct S_file_open_help{
                              //input parameters:
      const char *www_domain;
      bool preview_mode;      //no bgnd color, no images, no hyperlinks
      const C_zip_package *arch;
      bool append;            //append to already opened text
      bool use_https;         //use https:// for relative hyperlinks
      bool detect_phone_numbers;

                              //output params:
      struct S_frame{
         Cstr_c name, src;
      };
#ifdef WEB_FORMS
      C_vector<S_frame> frames;
#ifdef WEB
      class C_javascript_data *js_data;
#endif
      //C_js_engine::t_ErrorReport *js_err_fnc;
      //void *js_err_context;
#endif

      S_file_open_help():
         www_domain(NULL),
         preview_mode(false),
         append(false),
         use_https(false),
#ifdef WEB
         js_data(NULL), //js_err_fnc(NULL),
#endif
         arch(NULL)
      {
      }
   };

private:
   friend class C_mode_this;

   void DownloadSocketEvent(C_mode_this &mod, C_socket_notify::E_SOCKET_EVENT ev, C_socket *socket, bool &redraw);

//----------------------------
public:
   class C_mode_this: public C_mode_app<C_client_viewer>, public C_text_viewer{
      virtual bool WantInactiveTimer() const{ return false; }
   public:
      Cstr_w title;

      Cstr_w filename;
      C_smart_ptr<C_zip_package> arch;

      S_point drag_mouse;
      bool fullscreen;

      C_kinetic_movement kinetic_movement;

#ifdef MAIL
                              //cached images of html document
      C_buffer<S_attachment> html_images;
#endif
#ifdef WEB_FORMS
      C_list_mode_base dropdown_select;
      bool dropdown_select_expanded;
      t_FormAction form_action;
#endif

      C_mode_this(C_client_viewer &_app):
         C_mode_app<C_client_viewer>(_app),
#ifdef WEB_FORMS
         dropdown_select_expanded(false),
         form_action(NULL),
#endif
         fullscreen(false)
      {
         img_loader.mod_socket_notify = this;
         CreateTimer(30);
      }
      ~C_mode_this();

      virtual void InitLayout();
      virtual void ProcessInput(S_user_input &ui, bool &redraw);
      virtual void ProcessMenu(int itm, dword menu_id);
      virtual void Tick(dword time, bool &redraw);
      virtual void Draw() const;
      virtual void SocketEvent(C_socket_notify::E_SOCKET_EVENT ev, C_socket *socket, bool &redraw){ static_cast<C_client_viewer&>(app).DownloadSocketEvent(*this, ev, socket, redraw); }
   };
private:
   C_mode_this *SetMode();

   void Close(C_mode_this &mod);

   bool OpenFileForViewing1(const wchar *fname, const wchar *show_name, const C_zip_package *arch, const char *www_domain, C_mode_this *parent_viewer,
      C_viewer_previous_next *vpn, dword file_offset, E_TEXT_CODING coding, bool allow_delete);
   bool OpenTextFile(const wchar *filename, int is_html, S_text_display_info &text_info, S_file_open_help &oh) const;
#if defined MAIL || defined EXPLORER || defined WEB_FORMS
   bool ReadWordDocX(const wchar *filename, S_text_display_info &ti, const C_zip_package *arch) const;
   bool OpenWordDoc(const wchar *filename, S_text_display_info &ti, const C_zip_package *arch = NULL) const;
#endif
#if defined MAIL || defined EXPLORER || defined WEB_FORMS || defined MESSENGER //|| defined MUSICPLAYER
   // Convert html string into other string.
public:
   static void ConvertHtmlString(const char *src, E_TEXT_CODING coding, Cstr_w *dst_w, Cstr_c *dst_c);
   void CompileHTMLText(const char *cp, dword src_size, S_text_display_info &ti, S_file_open_help &oh) const;
private:
#endif

#ifdef WEB_FORMS
// Returns true if mode is destroyed.
   bool BrowserDoFormInputAction(C_mode_this &mod, dword key, bool &redraw);
#endif

public:
   static C_mode_this *CreateMode(C_client *cl){
      C_client_viewer *_this = (C_client_viewer*)cl;
      return _this->SetMode();
   }

   static bool OpenFileForViewing(C_client *cl, const wchar *fname, const wchar *show_name, const C_zip_package *arch = NULL, const char *www_domain = NULL, C_mode_this *parent_viewer = NULL,
      C_viewer_previous_next *vpn = NULL, dword file_offset = 0, E_TEXT_CODING coding = COD_DEFAULT, bool allow_delete = false){
      C_client_viewer *_this = (C_client_viewer*)cl;
      return _this->OpenFileForViewing1(fname, show_name, arch, www_domain, parent_viewer, vpn, file_offset, coding, allow_delete);
   }
   static bool OpenTextFile(const C_client *cl, const wchar *filename, int is_html, S_text_display_info &text_info, S_file_open_help &oh){
      const C_client_viewer *_this = (C_client_viewer*)cl;
      return _this->OpenTextFile(filename, is_html, text_info, oh);
   }
};

//----------------------------

bool CreateImageViewer(C_client &app, C_viewer_previous_next *vpn, const Cstr_w &title, const Cstr_w &fn, const C_zip_package *arch = NULL, dword img_file_offset = 0,
   bool allow_delete = true);

//----------------------------
#endif

//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//
// Network related utility functions and definitions.
//----------------------------

#ifndef __HTTP_H_
#define __HTTP_H_

//----------------------------
#include <Socket.h>
#include <TimeDate.h>
#include <UI\UserInterface.h>

//----------------------------

enum E_CONTENT_TYPE{
   CONTENT_TEXT,
   CONTENT_IMAGE,
   CONTENT_AUDIO,
   CONTENT_VIDEO,
   CONTENT_APPLICATION,
   CONTENT_MESSAGE,
   CONTENT_MULTIPART,
};

//----------------------------

struct S_content_type{
   E_CONTENT_TYPE type;
   Cstr_c subtype;             //text specifying type (e.g. plain, html, gif, etc...)
   S_content_type():
      type(CONTENT_TEXT)
   {}

//----------------------------
// Parse content type / subtype from text string.
   bool Parse(const char *&cp);

//----------------------------
// Same as Parse, but also read additional parameters after type/subtype (separated by ';')
   bool ParseWithCoding(const char *&cp, E_TEXT_CODING &cod);
};

//----------------------------
                              //class used for saving downloaded data (messages, attachments, http data) to file
class C_data_saving{
public:
   C_file fl;
   Cstr_w filename;

   ~C_data_saving(){
      CancelOutstanding();
   }
//----------------------------
// Cancel any data saving, delete saved file if it was opened.
   void CancelOutstanding();

//----------------------------
// Check if data saving was started.
   inline bool IsStarted() const{ return (filename.Length()!=0); }

//----------------------------
// Start data saving into given filename.
   bool Start(const Cstr_w &full_name);

//----------------------------
// Finish downloading - close file being written, and let this class not further to care about file (not to delete it in destructor).
   void Finish(){
      fl.Close();
      filename.Clear();
   }
};

//----------------------------
                              //class used as namespace for http loader
class C_application_http: public C_application_ui{
public:
                              //http protocol header
   struct S_http_header{
   private:

      static bool ReadPhrase(const char *&cp, Cstr_c &str);
   //----------------------------
   // Format: Sun Nov  6 08:49:37 1994 (not implemented yet)
      bool ReadDateTime_ANSI_C(const char *&cp, S_date_time &ret_date){ return false; }

   //----------------------------
   // rfc822_date | rfc850_date | ansic_c_date
      bool ReadHTTPDateTime(const char *&cp, S_date_time &date);
   public:
      S_content_type content;
      E_TEXT_CODING content_coding;

      dword date, last_modify_date, expire_date;
      dword content_length;
      bool chunked;
      //bool keep_alive;
      bool connection_close;
      Cstr_c location;
      Cstr_c etag;
      int http_ver;              //hi<<8 + lo
      int response_code;
      enum{
         ENCODING_IDENTITY,
         ENCODING_DEFLATE,
         ENCODING_GZIP,
         ENCODING_COMPRESS,
      } content_encoding;
      enum{
         FLG_CACHE_NO_CACHE = 1,
         FLG_CACHE_MUST_REVALIDATE = 2,
         FLG_AUTH_BASIC = 4,
      };
      dword flags;

      S_http_header(){
         Reset();
      }

   //----------------------------
   // Reset to default values.
      void Reset();

   //----------------------------
   // Parse complete (possibly multi-line) http headers.
      bool Parse(const char *cp);
   };

                                 //class for loading file over http protocol
   class C_http_data_loader{
   public:
                              //internal members:
      Cstr_c domain;
      C_smart_ptr<C_socket> socket;
      Cstr_c curr_domain;
      Cstr_c curr_file;
      S_http_header hdr;
      int http_code;          //http response code returned by server
      dword length_to_read;
      int chunk_size;         //used with chunked transfer encoding
      bool secure;            //using https (set in Loader_ConnectTo)
      enum E_PHASE{
         CLOSED,
         NOT_CONNECTED,
         CONNECTING,
         //SEND_REQUEST,
         WAIT_RESPONSE_HDR,
         POST_DATA,
         READ_HDR,
         READ_HDR_REDIRECT,
         READ_DATA,
         READ_DATA_CHUNK_BEGIN,
         READ_DATA_CHUNK,
         READ_DATA_CHUNK_END,
      } phase;


      Cstr_w system_error_text;  //socket's error text in case of failure
      Cstr_c userinfo;           //username/pass parsed from url, passed as Authorization: Basic header field

                              //dest filename set by caller
                              //when not set, then random filename is constructed in app's temp dir
                              //if only path is speciffied (last char is '\\') then random filename is created in that folder
      Cstr_w dst_filename;
      Cstr_c add_header_fields;  //additional header fields
      Cstr_c post_data;       //data to POST; when set, it uses POST, otherwise it uses GET; cleared when these are sent
      class C_post_data: public C_unknown{
      public:
         virtual dword GetLength() const = 0;   //advertise how many data will be POST-ed
         virtual dword ReadData(byte *buf, dword max_length) = 0;   //provide data for HTTP POST, returns number of filled data to buffer
      };
      C_smart_ptr<C_post_data> binary_post_data; //when not NULL, it provides interface for retrieving POST data; HTTP POST is used in such case; can't be combined with post_data; set to NULL when these data are sent
      Cstr_c content_type;    //used with POST to set Content-Type; when not set, it defaults to "application/x-www-form-urlencoded"
      enum E_METHOD{
         METHOD_AUTO,         //automatic mode depending on if post data are setup
         METHOD_GET,
         METHOD_POST,
         METHOD_PUT,
         METHOD_DELETE,
      } http_method;          //desired HTTP command

      C_data_saving data_saving;
      C_progress_indicator prog_curr;
      C_mode *mod_socket_notify; //mode on which the loader works, and which is notified about socket events

      inline bool IsActive() const{ return (phase!=CLOSED); }

      C_http_data_loader()
         :phase(CLOSED)
         ,secure(false)
         ,http_code(0)
         //,binary_post_data(NULL)
         ,http_method(METHOD_AUTO)
         ,mod_socket_notify(NULL)
      {}

      void Close(){
         data_saving.CancelOutstanding();
         socket = NULL;
         post_data.Clear();
         binary_post_data = NULL;
         phase = CLOSED;
      }
      void Reset();
   };

   enum E_HTTP_LOADER{
      LDR_CLOSED,
      LDR_PROGRESS,
      LDR_FAILED,
      LDR_NOT_FOUND,
      LDR_DONE,
      LDR_CORRUPTED,
   };

//----------------------------
// Connect http loader to download given resource over http protocol.
//    ldr = loader on which it operates
//    url = full network path
//    log_file = optional name where log file will be written
//    socket_notify = socket notification context; when not set, ldr's mod_socket_notify will be used
//    override_timeout = timeout to use, 0 = connection's default
   bool Loader_ConnectTo(C_http_data_loader &ldr, const Cstr_c &url, const wchar *log_file = NULL, void *socket_notify = NULL, int override_timeout = 0);

//----------------------------
// Process http data loader, should be called when socket notification happens on loader's socket.
   E_HTTP_LOADER TickDataLoader(C_http_data_loader &ldr, C_socket_notify::E_SOCKET_EVENT ev, bool &redraw);
};

//----------------------------

#endif

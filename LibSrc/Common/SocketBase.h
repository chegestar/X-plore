//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <Socket.h>

//#ifdef _DEBUG
#define LOG_FULL(n) LOG(n)
#define LOG_FULL_N(n) LOG_N(n)
/*
#else
#define LOG_FULL(n)
#define LOG_FULL_N(n)
#endif
*/

//----------------------------

class C_socket_base;

class C_internet_connection_base: public C_internet_connection{
   dword last_use_time;       //last tick time when connection was used (for detecting and disconnecting idle connection)
public:
   Cstr_c last_resolved_hostname;
   word last_resolved_port;
   int time_out;
   dword data_sent, data_received;
   C_vector<C_socket_base*> sockets;

   C_socket_notify *socket_notify;

   C_internet_connection_base(C_socket_notify *notify, int to):
      socket_notify(notify),
      time_out(to),
      data_sent(0), data_received(0),
      last_resolved_port(0)
   {
      ResetIdle();
   }

   void ResetIdle(){
      last_use_time = GetTickTime();
   }

   virtual dword GetLastUseTime() const{ return last_use_time; }

   virtual void ResetDataCounters(){
      data_sent = 0;
      data_received = 0;
   }

   virtual dword GetDataSent() const{ return data_sent; }
   virtual dword GetDataReceived() const{ return data_received; }

   virtual dword NumSockets() const{ return sockets.size(); }
};

//----------------------------

class C_socket_base: public C_socket{
protected:
#ifdef _DEBUG
   dword socket_index;
#endif

   int time_out;              //inherited from connection, may be overriden; -1 = no timeout
   bool use_ssl;

   dword dst_ip;              //ip address where we connect
   word dst_port;

   Cstr_w system_error_text;

   typedef C_buffer<byte> t_recv_block;
   C_vector<t_recv_block*> blocks;
   bool getline_has_new_data;

   int auto_receive_timeout;

   Cstr_w log_file_name;

   void *event_context;

   C_internet_connection_base *con;

   dword max_cache_data;

//----------------------------

   void Notify(C_socket_notify *socket_notify, C_socket_notify::E_SOCKET_EVENT ev){
      if(socket_notify)
         socket_notify->SocketEvent(ev, this, event_context);
   }

//----------------------------

   void AddToLog(const void *buf, dword len) const;

//----------------------------

   C_socket_base(C_internet_connection_base *con, void *event_context, const wchar *log_fn, int to, bool ssl);
   ~C_socket_base();

   void DeleteAllBlocks();
public:
   inline bool IsLogging() const{
      return (log_file_name.Length()!=0);
   }

   inline void LOG(const char *cp) const{
      if(IsLogging())
         AddToLog(cp, StrLen(cp));
   }
   inline void LOGB(const void *buf, dword len) const{
      if(IsLogging())
         AddToLog(buf, len);
   }

//----------------------------

   inline void LOG_N(int n) const{

      if(IsLogging()){
         Cstr_c s;
         s<<n;
         AddToLog(s, s.Length());
      }
   }

   virtual void SetCacheSize(dword sz){ max_cache_data = sz; }
   virtual void SetTimeout(int t);
   virtual bool IsSSL() const{ return use_ssl; }
   virtual dword GetDestIp() const{ return dst_ip; }
   virtual word GetDestPort() const{ return dst_port; }
   virtual const Cstr_w &GetSystemErrorText() const{ return system_error_text; }

   virtual dword GetNumAvailableData() const;
   virtual void DiscardData(dword size);
   virtual bool PeekData(t_buffer &buf, dword min_size, dword max_size) const;
   virtual bool GetData(t_buffer &buf, dword min_size, dword max_size);
   virtual bool GetLine(t_buffer &line);

//----------------------------
// Encode url by RFC 3492. url is utf8 encoded.
   static Cstr_c PunyEncode(const Cstr_c &url);
};

//----------------------------

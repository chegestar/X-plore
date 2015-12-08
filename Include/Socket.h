//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <SmartPtr.h>
#include <Cstr.h>
#include <C_buffer.h>
#include <C_vector.h>
#include <IGraph.h>

//----------------------------
class C_socket;

#if defined __SYMBIAN32__ || defined BADA
#define CONNECTION_HAVE_ALTERNATIVE_IAP
#endif

class C_socket_notify{
public:
   enum E_SOCKET_EVENT{
      SOCKET_CONNECTED,
      SOCKET_DATA_SENT,
      SOCKET_DATA_RECEIVED,
      SOCKET_ERROR,
      SOCKET_FINISHED,
      SOCKET_SSL_HANDSHAKE,
   };

   virtual void SocketEvent(E_SOCKET_EVENT event, C_socket *socket, void *context) = 0;
};

//----------------------------

class C_internet_connection: public C_unknown{
public:
//----------------------------
// Get last time (measured by GetTickTime()) when this connection was actively used by some socket.
   virtual dword GetLastUseTime() const = 0;

//----------------------------
// Get / reset data counters.
   virtual dword GetDataSent() const = 0;
   virtual dword GetDataReceived() const = 0;
   virtual void ResetDataCounters() = 0;

//----------------------------
// Determine which access point is being used (0=primary, 1=secondary).
   virtual dword GetIapIndex() const = 0;

//----------------------------
// Get number of active sockets using this connection.
   virtual dword NumSockets() const = 0;

//----------------------------
// Enumerate access points.
   struct S_access_point{
      Cstr_w name;            //name of IAP
      dword id;
      C_smart_ptr<C_image> img;  //optional image representing access point
   };
   static void EnumAccessPoints(C_vector<S_access_point> &aps, C_application_base *app);
};

//----------------------------
// Create new connection.
// time_out is in ms.
C_internet_connection *CreateConnection(C_socket_notify *notify,
#if defined _WINDOWS || defined _WIN32_WCE
   const wchar *iap_name,
#else
   int iap_id,
#endif
   int time_out
#ifdef CONNECTION_HAVE_ALTERNATIVE_IAP
   , int secondary_iap_id = -1      //-1 = disabled
#endif
   );

//----------------------------

class C_socket: public C_unknown{
public:
   enum{                      //default ports
      PORT_SMTP = 25,
      PORT_HTTP = 80,
      PORT_POP3 = 110,
      PORT_IMAP4 = 143,

      PORT_HTTP_SSL = 443,
      PORT_POP3_SSL = 995,
      PORT_IMAP4_SSL = 993,
   };

   typedef C_buffer<char> t_buffer;

//----------------------------
// Connect to host/port.
   virtual void Connect(const char *host_name, word port, int override_timeout = 0) = 0;

//----------------------------
// Set internal cache size. Data are buffered up to this size is encountered or exceeded, then no more data are received. By default there's limit dependent on platform.
   virtual void SetCacheSize(dword bytes) = 0;

//----------------------------
// Send data to socket.
   virtual void SendData(const void *data, dword len, int override_timeout = 0, bool no_log = false) = 0;
   void SendString(const Cstr_c &s, int override_timeout = 0, bool no_log = false);
   void SendCString(const char *str, int override_timeout = 0, bool no_log = false);

//----------------------------
// Cancel current time-out.
   virtual void CancelTimeOut() = 0;

//----------------------------
// Get number of data available for reading.
   virtual dword GetNumAvailableData() const = 0;

//----------------------------
// Remove data from beginning of read buffer.
   virtual void DiscardData(dword size) = 0;

//----------------------------
// Read data, do not remove from buffers.
// 'min_size' specifies minimum buffer size that is accepted, 'max_size' specifies maimum number of data that is to be retrieved.
// If there's less than 'min_size' data available, then function returns false.
   virtual bool PeekData(t_buffer &buf, dword min_size, dword max_size) const = 0;

//----------------------------
// Get binary data.
   virtual bool GetData(t_buffer &buf, dword min_size, dword max_size) = 0;

//----------------------------
// Get single line of data. Returns true if 
// Single text line is returned. '\0' is appended to the buffer.
// Line end is determined by \n or \r\n characters. To determine if line was terminated by \r, check last character of returned buffer, if it is \n, then \r\n was used, otherwise \n was used.
// The buffer size is always such big, how many characters were read from stream.
   virtual bool GetLine(t_buffer &line) = 0;

//----------------------------
// Get error text (returnet by system).
   virtual const Cstr_w &GetSystemErrorText() const = 0;

//----------------------------
// Check if socket operates in SSL mode.
   virtual bool IsSSL() const = 0;

//----------------------------
// Get IP/port of computer to which this socket is connected.
   virtual dword GetDestIp() const = 0;
   virtual word GetDestPort() const = 0;

//----------------------------
// Get own (local) IP address.
   virtual dword GetLocalIp() const = 0;

//----------------------------
// Begin SSL socket operation, if it was initialized in deferred SSL mode.
   virtual bool BeginSSL() = 0;

//----------------------------
// Override time-out value used by this socket.
// If time_out is 0, then default connection's time-out will be used.
   virtual void SetTimeout(int time_out) = 0;
};

//----------------------------

C_socket *CreateSocket(C_internet_connection*, void *event_context = NULL, bool use_ssl = false, bool def_ssl = false, const wchar *log_file_name = NULL);

//----------------------------
#if defined _DEBUG && defined _WINDOWS

C_socket *CreateSocketPcap(const wchar *fn, C_internet_connection *con, void *event_context = NULL, const char *filter_ip = NULL);

#endif
//----------------------------

#endif

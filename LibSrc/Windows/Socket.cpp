//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include "..\Common\SocketBase.h"

#ifdef _WIN32_WCE
namespace win{
#include <WinSock2.h>
#include <ConnMgr.h>
#include <SslSock.h>
#include <PmPolicy.h>
#include <Ws2tcpip.h>
#include <Pm.h>
#include <PmPolicy.h>
}
#pragma comment(lib,"winsock")
#pragma comment(lib,"Ws2")
#pragma comment(lib,"cellcore")

#else//_WIN32_WCE

#include <stdio.h>
namespace win{
#include <WinSock2.h>
#include <Ws2tcpip.h>
}
#ifdef USE_OPENSSL
namespace openssl{
#include "openssl\ssl.h"
}
#pragma comment(lib,"libeay32MTd.lib")
#pragma comment(lib,"ssleay32MTd.lib")
#endif
#pragma comment(lib,"wsock32.lib")
#pragma comment(lib,"ws2_32.lib")

#endif//!_WIN32_WCE
#undef SOCKET_ERROR
const int SOCKET_ERROR = -1;

#if defined _DEBUG && defined _WINDOWS
#define USE_SOCKET_PCAP_SIM
#endif

enum{
   WM_SOCKET_SET_ERROR = WM_USER+200,
   WM_SOCKET_CONNECTED,
   WM_SSL_HANDSHAKE,
   WM_CONNECT_EVENT,
   WM_SOCKET_FINISHED,
#ifdef USE_SOCKET_PCAP_SIM
   WM_SOCKET_DATA_SENT_SIM,
   WM_SOCKET_DATA_RECEIVE_SIM,
#endif
};

//----------------------------
class C_socket_win32;

class C_connection_Win32: public C_internet_connection_base{
public:
   win::sockaddr last_resolved_address;
#ifdef _WIN32_WCE
   win::HANDLE h_con;              //handle to connection (ConnMgrEstablishConnection)
#endif
   bool is_initiated, is_initiating;
   Cstr_w iap_name;           //empty = "automatic"

   dword next_socket_id;      //id value for next socket that will be created

//----------------------------
#ifdef USE_OPENSSL
   openssl::SSL_CTX *ssl_ctx;
   void InitSslLib(){
      if(!ssl_ctx){
         openssl::SSL_library_init();
         ssl_ctx = openssl::SSL_CTX_new(openssl::SSLv23_client_method());
         // needed for mail1.firedupgroup.co.uk:993
         openssl::SSL_CTX_set_options(ssl_ctx, SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS);
      }
   }
#endif

//----------------------------

   win::HWND hwnd_async;

   win::LRESULT WndProc(win::HWND hwnd, win::UINT msg, win::WPARAM wParam, win::LPARAM lParam);

//----------------------------

   static win::LRESULT CALLBACK WndProc_thunk(win::HWND hwnd, win::UINT msg, win::WPARAM wParam, win::LPARAM lParam){
      if(msg==WM_CREATE){
         win::LPCREATESTRUCT cs = (win::LPCREATESTRUCT)lParam;
         win::SetWindowLong(hwnd, GWL_USERDATA, (win::LPARAM)cs->lpCreateParams);
      }
      C_connection_Win32 *_this = (C_connection_Win32*)win::GetWindowLong(hwnd, GWL_USERDATA);
      if(!_this)
         return win::DefWindowProc(hwnd, msg, wParam, lParam);
      return _this->WndProc(hwnd, msg, wParam, lParam);
   }

//----------------------------

   C_connection_Win32(C_socket_notify *notify, const wchar *iap_n, int to):
      C_internet_connection_base(notify, to),
      next_socket_id(0),
#ifdef _WIN32_WCE
      h_con(NULL),
#endif
#ifdef USE_OPENSSL
      ssl_ctx(NULL),
#endif
      is_initiated(false),
      is_initiating(false),
      iap_name(iap_n)
   {
      win::WSADATA data;
      //version = MAKEWORD(2, 2);
      win::WSAStartup(0x202, &data);

      win::WNDCLASS wc = { 0, &WndProc_thunk, 0, 0, win::GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"sock_notify" };
      win::RegisterClass(&wc);
      hwnd_async = win::CreateWindow(L"sock_notify", L"", WS_DISABLED, 0, 0, 0, 0, NULL, NULL, win::GetModuleHandle(NULL), this);
      assert(hwnd_async);

      win::SetTimer(hwnd_async, 1, 50, NULL);
#ifdef _WIN32_WCE
      SetPowerState(true);
#endif
   }

//----------------------------

   ~C_connection_Win32(){
#ifdef _WIN32_WCE
      SetPowerState(false);
#endif
      if(hwnd_async){
         win::KillTimer(hwnd_async, 1);
         win::DestroyWindow(hwnd_async);
      }
      win::UnregisterClass(L"sock_notify", NULL);
#ifdef USE_OPENSSL
      openssl::SSL_CTX_free(ssl_ctx);
#endif
      win::WSACleanup();
   }

//----------------------------

   virtual dword GetIapIndex() const{ return 0; }

//----------------------------
#ifdef _WIN32_WCE

   bool StartInitiate(){
      win::CONNMGR_CONNECTIONINFO ci;
      MemSet(&ci, 0, sizeof(ci));
      ci.cbSize = sizeof(ci);
      ci.dwPriority = CONNMGR_PRIORITY_USERINTERACTIVE;
      ci.hWnd = hwnd_async;
      ci.uMsg = WM_CONNECT_EVENT;
      ci.lParam = (win::LPARAM)this;

                           //request specific connection (identified by name)
      for(int i=0; ; i++){
         win::CONNMGR_DESTINATION_INFO ii;
         win::HRESULT hr = win::ConnMgrEnumDestinations(i, &ii);
         if(hr<0)
            break;
         bool match;
         if(iap_name.Length())
            match = (!StrCmp(iap_name, ii.szDescription));
         else
            match = (!StrCmp(ii.szDescription, L"The Internet") || !StrCmp(ii.szDescription, L"Internet"));
         if(match){
            ci.guidDestNet = ii.guid;
            ci.dwParams |= CONNMGR_PARAM_GUIDDESTNET;
            break;
         }
      }
      win::HANDLE hc;
      win::HRESULT hr = win::ConnMgrEstablishConnection(&ci, &hc);
      if(hr<0)
         return false;
      h_con = hc;
      is_initiating = true;
      return true;
   }

//----------------------------

   void CloseHcon(){
      if(h_con){
         win::ConnMgrReleaseConnection(h_con, 1);
         h_con = NULL;
         is_initiating = false;
         is_initiated = false;
      }
   }

//----------------------------

   void FinishInitiate();

//----------------------------

   //win::HANDLE power_handle;   //doesn't work

   void SetPowerState(bool b){

      win::PowerPolicyNotify(PPN_UNATTENDEDMODE, b);
         /*
      if(b){
         if(power_handle==win::HANDLE(-1)){
#ifdef _DEBUG
            power_handle = win::SetPowerRequirement(L"NDL1:", win::D0, 1, NULL, 0);
#endif
         }
      }else{
         if(power_handle!=win::HANDLE(-1)){
            win::ReleasePowerRequirement(power_handle);
            power_handle = win::HANDLE(-1);
         }
      }
      */
   }

#endif
};

C_internet_connection *CreateConnection(C_socket_notify *notify, const wchar *iap_name, int time_out){
   return new(true) C_connection_Win32(notify, iap_name, time_out);
}

//----------------------------
//----------------------------

class C_socket_win32: public C_socket_base{
protected:
   bool deferred_ssl_handshake;
   bool is_finished;
   bool send_in_progress;
   bool initiating;
   dword connecting_beg_time; //0 = not connecting

//----------------------------
   C_smart_ptr<C_connection_Win32> con;

   dword socket_id;           //unique socket identificator, used for WM messages (socket pointer isn't enough, since memory of destroyed socket may be reused for a new socket)

   win::SOCKET socket;
   friend class C_connection_Win32;
   bool timer_set;

   dword receive_buf_data;

//----------------------------

   void SetTimeOutTimer(int ms){
      if(ms>0){
         win::SetTimer(con->hwnd_async, dword(this), ms, NULL);
         timer_set = true;
      }else
         CancelTimeOutTimer();
   }

//----------------------------

   void CancelTimeOutTimer(){
      if(timer_set){
         bool b;
         b = win::KillTimer(con->hwnd_async, dword(this));
         assert(b);
         timer_set = false;
      }
   }

//----------------------------
// This call may destroy socket.
   void SetError(){
      //if(_status!=ST_ERROR){
         //_status = ST_ERROR;
         Notify(con->socket_notify, C_socket_notify::SOCKET_ERROR);
      //}
   }

//----------------------------

   void SetSocketError(const char *err){

      CancelTimeOutTimer();
      system_error_text.Copy(err);
      SetError();
   }

//----------------------------

   void TimeOut(){
      LOG("Time-out\n");
      SetSocketError("Time-out");
   }

//----------------------------

   Cstr_c requested_host_name;

   int curr_temp_timeout;

//----------------------------
#ifdef _WIN32_WCE

   static int CALLBACK SslCertHook(win::DWORD dwType, win::LPVOID pvArg, win::DWORD dwChainLen, win::LPBLOB pCertChain, win::DWORD dwFlags){
      return SSL_ERR_OKAY;
   }

#elif defined USE_OPENSSL
   openssl::SSL *ssl_object;
#endif
//----------------------------

   void MakeErrorText(int code){

      system_error_text.Clear();
      switch(code){
      case 10049: system_error_text = L"The requested network address is not valid."; break;
      case 10050: system_error_text = L"Network is down."; break;
      case 10051: system_error_text = L"Network is unreachable."; break;
      case 10053: system_error_text = L"Connection aborted."; break;
      case 10054: system_error_text = L"Connection reset by peer."; break;
      case 10060: system_error_text = L"Time-out."; break;
      case 10061: system_error_text = L"Connection refused."; break;
      case 10065: system_error_text = L"Host unreachable."; break;
      case 11001: system_error_text = L"Host not found."; break;
      }
      system_error_text.AppendFormat(L" Error number %") <<code;
      system_error_text.Build();
   }

//----------------------------
#if 1

   dword Resolve(win::sockaddr &addr, const Cstr_c &host_name, word port){

      win::addrinfo hints;
      MemSet(&hints, 0, sizeof(hints));
      hints.ai_family = AF_INET;
      hints.ai_socktype = SOCK_STREAM;
#ifdef _WIN32_WCE
      hints.ai_protocol = IPPROTO_TCP;
#else
      hints.ai_protocol = win::IPPROTO_TCP;
#endif
      win::addrinfo *list = NULL;

      Cstr_c p; p<<int(port);
      Cstr_c url = PunyEncode(host_name);

      dword ret = win::getaddrinfo(url, p, &hints, &list);
      if(ret || !list)
         return ret;
      addr = *list->ai_addr;

      freeaddrinfo(list);
      return 0;
   }

#else
   static bool ResloveAddress(win::sockaddr_in &addr, const char *host_name, word port){
      MemSet(&addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = win::inet_addr(host_name);

      if(addr.sin_addr.s_addr == INADDR_NONE){
         const win::HOSTENT *pHost = win::gethostbyname(host_name);
         if(!pHost)
            return false;
         addr.sin_addr = *(win::IN_ADDR*)pHost->h_addr_list[0];
      }
      addr.sin_port = win::htons(port);
      return true;
   }
#endif
//----------------------------

   void ConnectByHost(){

      initiating = false;
      con->ResetIdle();
      Close();
      if(con->last_resolved_hostname!=requested_host_name || con->last_resolved_port!=dst_port){
                              //resolve DNS
#if 1
         dword err = Resolve(con->last_resolved_address, requested_host_name, dst_port);
         if(err){
            MakeErrorText(err);
            win::PostMessage(con->hwnd_async, WM_SOCKET_SET_ERROR, win::WPARAM(this), socket_id);
            return;
         }
#else
         win::sockaddr_in ad;
         if(!ResloveAddress(ad, requested_host_name, dst_port)){
            MakeErrorText(win::WSAGetLastError());
            win::PostMessage(con->hwnd_async, WM_SOCKET_SET_ERROR, win::WPARAM(this), socket_id);
            return;
         }
         con->last_resolved_address = *(win::sockaddr*)&ad;
#endif
         con->last_resolved_hostname = requested_host_name;
         con->last_resolved_port = dst_port;
      }
      int err = 0;

      socket = win::socket(AF_INET, SOCK_STREAM,
#ifdef _WIN32_WCE
         IPPROTO_TCP);
#else
         win::IPPROTO_TCP);
#endif
      if(socket == win::SOCKET(~0)){
         MakeErrorText(win::WSAGetLastError());
         win::PostMessage(con->hwnd_async, WM_SOCKET_SET_ERROR, win::WPARAM(this), socket_id);
         return;
      }

#ifdef _WIN32_WCE
      if(use_ssl || deferred_ssl_handshake){
         dword optval = SO_SEC_SSL;
         err = win::setsockopt(socket, SOL_SOCKET, SO_SECURE, (const char*)&optval, sizeof(optval));
         if(!err){
            win::SSLVALIDATECERTHOOK fnc = { &SslCertHook, this};
            dword br;
            err = win::WSAIoctl(socket, SO_SSL_SET_VALIDATE_CERT_HOOK, &fnc, sizeof(fnc), NULL, 0, &br, NULL, NULL);
            if(deferred_ssl_handshake && !err){
               dword flg = SSL_FLAG_DEFER_HANDSHAKE;
               err = win::WSAIoctl(socket, SO_SSL_SET_FLAGS, &flg, sizeof(flg), NULL, 0, &br, NULL, NULL);
               //if(!deferred_ssl_handshake)
                  //request = REQ_SSL_HANDSHAKE_1;
            }
         }
      }
#endif
      if(err != SOCKET_ERROR){
         const win::sockaddr &sa = con->last_resolved_address;
         dst_ip = byte(sa.sa_data[5]) | (byte(sa.sa_data[4])<<8) | (byte(sa.sa_data[3])<<16) | (byte(sa.sa_data[2])<<24);
         if(IsLogging()){
            Cstr_c s; s.Format("Connect to IP %.%.%.%\n") <<int((dst_ip>>24)&0xff) <<int((dst_ip>>16)&0xff) <<int((dst_ip>>8)&0xff) <<int(dst_ip&0xff);
            LOG(s);
         }
#ifdef _WIN32_WCE
         if(use_ssl || deferred_ssl_handshake){
            err = win::connect(socket, &sa, sizeof(win::sockaddr_in));
         }else
#endif
         {
                              //set the socket in non-blocking
            dword mode = 1;
            const long _FIONBIO = _IOW('f', 126, win::u_long);
            err = win::ioctlsocket(socket, _FIONBIO, &mode);
            if(!err){
               win::connect(socket, &sa, sizeof(win::sockaddr_in));
               win::WSASetLastError(0);
                                 //back to blocking mode
               mode = 0;
               err = win::ioctlsocket(socket, _FIONBIO, &mode);
            }
         }
      }

      if(err == SOCKET_ERROR){
         socket = win::SOCKET(~0);
         err = win::WSAGetLastError();
         LOG("connect error: ");
         LOG_N(err);
         LOG("\n");
         MakeErrorText(err);
         win::PostMessage(con->hwnd_async, WM_SOCKET_SET_ERROR, win::WPARAM(this), socket_id);
      }else{
         connecting_beg_time = GetTickTime();
      }
   }

//----------------------------

public:
   C_socket_win32(C_connection_Win32 *c, void *event_context, bool ssl, bool def_ssl, const wchar *log_file_name):
      C_socket_base(c, event_context, log_file_name, c->time_out, ssl),
      con(c),
      curr_temp_timeout(0),
      socket(win::SOCKET(~0)),
      timer_set(false),
      receive_buf_data(0),
#ifdef USE_OPENSSL
      ssl_object(NULL),
#endif
      is_finished(false),
      initiating(false),
      connecting_beg_time(0),
      send_in_progress(false),
      deferred_ssl_handshake(def_ssl)
   {
      socket_id = con->next_socket_id++;
   }

//----------------------------

   ~C_socket_win32(){

      Close();
      CancelTimeOutTimer();
      for(int i=con->sockets.size(); i--; ){
         if(con->sockets[i]==this){
#ifdef _DEBUG
            C_socket_win32 *s = (C_socket_win32*)(C_socket*)con->sockets[i];
            assert(s->socket_id==socket_id);
#endif
            con->sockets.remove_index(i);
            break;
         }
      }
   }

//----------------------------

   void Close(){

      CancelTimeOutTimer();
#ifdef USE_OPENSSL
      if(ssl_object){
         openssl::SSL_free(ssl_object);
         ssl_object = NULL;
      }
#endif
      if(socket != win::SOCKET(~0)){
         win::shutdown(socket, SD_BOTH);
         win::closesocket(socket);
         socket = win::SOCKET(~0);
      }
      getline_has_new_data = false;
      DeleteAllBlocks();
   }

//----------------------------

   virtual void Connect(const char *hname, word port, int override_timeout){

      dst_port = port;
      requested_host_name = hname;
      LOG("connecting to host: "); LOG(requested_host_name); LOG(":"); LOG_N(port); LOG("\n");

      curr_temp_timeout = override_timeout ? override_timeout : time_out;
#ifdef _WIN32_WCE
      if(con->h_con){
         dword st;
         win::HRESULT hr = win::ConnMgrConnectionStatus(con->h_con, &st);
         LOG("Con st: ");
         LOG_N(st);
         LOG("\n");
         if(hr<0 || st!=CONNMGR_STATUS_CONNECTED)
            con->CloseHcon();
      }
                              //create connection
      if(//con->iap_name.Length() &&
         !con->is_initiated){
         initiating = true;
         if(!con->is_initiating){
            LOG("Starting connection initiator\n");
            if(con->StartInitiate()){
               if(curr_temp_timeout>=0)
                  SetTimeOutTimer(curr_temp_timeout);
            }else{
               system_error_text = L"Can't initiate connection";
               win::PostMessage(con->hwnd_async, WM_SOCKET_SET_ERROR, win::WPARAM(this), socket_id);
            }
         }else{
            LOG("Connection is initiating, waiting...\n");
         }
         return;
      }else{
         LOG(con->is_initiated ? "Connection already initiated\n" : "Using automatic access point\n");
      }
#endif
      ConnectByHost();
   }

//----------------------------

   bool IsReadReady() const{

      win::fd_set fds;
      fds.fd_array[0] = socket; fds.fd_count=1;

      win::timeval tv = {0, 0};
      win::WSASetLastError(0);
      int result = win::select(0, &fds, NULL, NULL, &tv);

      if(result == 0 || result == SOCKET_ERROR){
#ifdef USE_OPENSSL
         if(ssl_object)
            return (openssl::SSL_pending(ssl_object)!=0);
#endif
         return false;
      }
      return true;
   }

//----------------------------

   void ReceivePoll(){

#ifdef _WIN32_WCE
                              //note: setting buf to 1024 caused select() not reporting new data on WM (sometimes)...
      const int BUF_SIZE = 1007;
#else
      const int BUF_SIZE = 2048;
#endif
      char *buf = new(true) char[BUF_SIZE];
      bool has_new_data = false;
      const dword MAX_SIZE = 1024*16;
      dword size = 0;
                              //read all message blocks
      while(size<MAX_SIZE){
         if(!IsReadReady()){
            int wsa_err = win::WSAGetLastError();
            if(wsa_err){
               switch(wsa_err){
               case WSAETIMEDOUT:
                  system_error_text = L"Time-out";
                  break;
               default:
                  MakeErrorText(wsa_err);
               }
               SetError();
               has_new_data = false;
            }
            break;
         }
         CancelTimeOutTimer();
         int len;
#ifdef USE_OPENSSL
         if(ssl_object){
            len = openssl::SSL_read(ssl_object, buf, BUF_SIZE);
            assert(len>=0);
         }else
#endif
         {
            len = win::recv(socket, buf, BUF_SIZE, 0);
         }
         if(!len){
                              //socket closed
            is_finished = true;
            if(!blocks.size()){
                              //no more data, we can post finish notify now
               delete[] buf;
               Notify(con->socket_notify, C_socket_notify::SOCKET_FINISHED);
               return;
            }
                              //wait till client collects data, send notify later
            getline_has_new_data = true;
            win::PostMessage(con->hwnd_async, WM_SOCKET_FINISHED, win::WPARAM(this), socket_id);
            break;
         }
         if(len == SOCKET_ERROR){
            int wsa_err = win::WSAGetLastError();
            if(wsa_err){
               switch(wsa_err){
               case WSAETIMEDOUT:
                  system_error_text = L"Time-out";
                  break;
               default:
                  MakeErrorText(wsa_err);
               }
               SetError();
               has_new_data = false;
            }
            break;
         }
         con->data_received += len;
         con->ResetIdle();
         has_new_data = true;

         t_recv_block *bl = new(true) t_recv_block;
         bl->Resize(len);
         blocks.push_back(bl);
         MemCpy(bl->Begin(), buf, len);
         if(log_file_name.Length()){
            LOGB(buf, len);
         }
         getline_has_new_data = true;
         size += len;

         receive_buf_data = GetNumAvailableData();
         if(!max_cache_data || receive_buf_data<max_cache_data)
            SetTimeOutTimer(curr_temp_timeout);
      }
      delete[] buf;
      if(has_new_data)
         Notify(con->socket_notify, C_socket_notify::SOCKET_DATA_RECEIVED);
   }

//----------------------------

   virtual void DiscardData(dword size){

      dword prev_data = !max_cache_data ? 0 : GetNumAvailableData();
      C_socket_base::DiscardData(size);
      receive_buf_data = GetNumAvailableData();
      if(max_cache_data && prev_data>=max_cache_data && receive_buf_data<max_cache_data)
         _Receive(0);
   }

//----------------------------

   virtual void _Receive(int override_timeout){

      con->ResetIdle();
      getline_has_new_data = false;
      curr_temp_timeout = override_timeout ? override_timeout : time_out;
      SetTimeOutTimer(curr_temp_timeout);
   }

//----------------------------

   virtual void SetTimeout(int t){

      C_socket_base::SetTimeout(t);
      curr_temp_timeout = time_out;
      SetTimeOutTimer(time_out);
   }

//----------------------------

   virtual void CancelTimeOut(){
      CancelTimeOutTimer();
      curr_temp_timeout = 0;
   }

//----------------------------

   virtual void SendData(const void *buf, dword len, int override_timeout, bool disable_logging){

      send_in_progress = true;
      CancelTimeOutTimer();
      con->ResetIdle();

      if(log_file_name.Length()){
         LOG(":> ");
#ifndef _DEBUG
         if(disable_logging){
            LOG("<login command sent>\n");
         }else
#endif
         {
            LOGB(buf, len);
            /*
                              //copy to temp buf, add null
            C_buffer<char> tmp; tmp.Resize(len+1);
            MemCpy(tmp.Begin(), buf, len);
            tmp[len] = 0;
            LOG(tmp.Begin());
            */
         }
      }
      int nsent;
#ifdef USE_OPENSSL
      if(ssl_object)
         nsent = openssl::SSL_write(ssl_object, buf, len);
      else
#endif
         nsent = win::send(socket, (const char*)buf, len, 0);
      if(nsent == SOCKET_ERROR){
         MakeErrorText(win::WSAGetLastError());
         win::PostMessage(con->hwnd_async, WM_SOCKET_SET_ERROR, win::WPARAM(this), socket_id);
      }
      con->data_sent += len;
      SetTimeOutTimer(override_timeout ? override_timeout : time_out);
   }

//----------------------------

   void Tick(){

      if(initiating || socket == win::SOCKET(~0))
         return;

      if(connecting_beg_time){
                              //monitor connection status

                              //check time-out
         if(curr_temp_timeout && GetTickTime()>connecting_beg_time+curr_temp_timeout){
            TimeOut();
            return;
         }

                              //now wait for actual connection
         int err = 0;
         win::fd_set fd_wr, fd_err;
         fd_wr.fd_array[0] = socket; fd_wr.fd_count=1;
         fd_err = fd_wr;
         win::timeval tv = {0, 0};
         int result = win::select(0, NULL, &fd_wr, &fd_err, &tv);
         //LOG_RUN_N("result", result);
         if(result){
            if(__WSAFDIsSet(socket, &fd_wr)){
                           //connected ok
#ifdef USE_OPENSSL
               if(!err && use_ssl){
                  con->InitSslLib();
                  ssl_object = openssl::SSL_new(con->ssl_ctx);
                  openssl::SSL_set_fd(ssl_object, socket);
                  err = openssl::SSL_connect(ssl_object);
                  err = err==1 ? 0 : -1;
               }
#endif//USE_OPENSSL
               if(!err){
                           //finally connected
                  connecting_beg_time = 0;
                  _Receive(0);
                  win::PostMessage(con->hwnd_async, WM_SOCKET_CONNECTED, win::WPARAM(this), socket_id);
               }
            }else
            if(__WSAFDIsSet(socket, &fd_err)){
               err = -1;
               win::WSASetLastError(10061);
            }
         }
         if(err){
            socket = win::SOCKET(~0);
            err = win::WSAGetLastError();
            LOG("connect error: ");
            LOG_N(err);
            LOG("\n");
            MakeErrorText(err);
            win::PostMessage(con->hwnd_async, WM_SOCKET_SET_ERROR, win::WPARAM(this), socket_id);
         }
         return;
      }
                              //monitor sending
      if(send_in_progress){
         win::fd_set fds;
         fds.fd_array[0] = socket; fds.fd_count=1; //FD_ZERO(&fds); FD_SET(socket, &fds);
         win::timeval tv = {0, 0};
         int result = win::select(0, NULL, &fds, NULL, &tv);
         if(result==1){
            CancelTimeOutTimer();
            //status = ST_SUCCESS;
            send_in_progress = false;
            AddRef();
            Notify(con->socket_notify, C_socket_notify::SOCKET_DATA_SENT);
            if(Release()!=0)
               _Receive(0);
         }else
         if(result == SOCKET_ERROR){
            CancelTimeOutTimer();
            MakeErrorText(win::WSAGetLastError());
            SetError();
            return;
         }
      }
                              //check for receive
      if(!is_finished){
         if(!max_cache_data || receive_buf_data<max_cache_data)
            ReceivePoll();
      }
   }

//----------------------------

   void AsyncProcess(dword msg){

      switch(msg){
      case WM_SOCKET_SET_ERROR:
         SetError();
         return;

      case WM_SOCKET_CONNECTED: Notify(con->socket_notify, C_socket_notify::SOCKET_CONNECTED); break;
      case WM_SOCKET_FINISHED: Notify(con->socket_notify, C_socket_notify::SOCKET_FINISHED); break;

      case WM_SSL_HANDSHAKE:
         Notify(con->socket_notify, C_socket_notify::SOCKET_SSL_HANDSHAKE);
         break;

#ifdef USE_SOCKET_PCAP_SIM
      case WM_SOCKET_DATA_SENT_SIM:
         Notify(con->socket_notify, C_socket_notify::SOCKET_DATA_SENT);
         break;

      case WM_SOCKET_DATA_RECEIVE_SIM:
         Notify(con->socket_notify, C_socket_notify::SOCKET_DATA_RECEIVED);
         break;
#endif
      }
   }

//----------------------------

   virtual bool BeginSSL(){

      CancelTimeOutTimer();
      use_ssl = true;
      assert(deferred_ssl_handshake);
#ifdef _WIN32_WCE
      dword br;
      int err = win::WSAIoctl(socket, SO_SSL_PERFORM_HANDSHAKE, NULL, 0, NULL, 0, &br, NULL, NULL);
#elif defined USE_OPENSSL
      con->InitSslLib();
      ssl_object = openssl::SSL_new(con->ssl_ctx);
      openssl::SSL_set_fd(ssl_object, socket);
      int err = (openssl::SSL_connect(ssl_object)==1) ? 0 : SOCKET_ERROR;
#else
      int err = 1;
#endif
      if(err){
         MakeErrorText(win::WSAGetLastError());
         win::PostMessage(con->hwnd_async, WM_SOCKET_SET_ERROR, win::WPARAM(this), socket_id);
         return false;
      }
      win::PostMessage(con->hwnd_async, WM_SSL_HANDSHAKE, win::WPARAM(this), socket_id);
      //status = ST_SUCCESS;
      //SetTimeOutTimer(time_out);
      return true;
   }

//----------------------------

   virtual dword GetLocalIp() const{

      win::sockaddr sa;
      int len = sizeof(sa);
      if(getsockname(socket, &sa, &len)!=0)
         return 0;
      return (byte(sa.sa_data[2])<<24) | (byte(sa.sa_data[3])<<16) | (byte(sa.sa_data[4])<<8) | byte(sa.sa_data[5]);
   }

//----------------------------
};

C_socket *CreateSocket(C_internet_connection *con, void *event_context, bool use_ssl, bool deferred_ssl_handshake, const wchar *log_file_name){

   return new(true) C_socket_win32((C_connection_Win32*)con, event_context, use_ssl, deferred_ssl_handshake, log_file_name);
}

//----------------------------
//----------------------------

#ifdef _WIN32_WCE
void C_connection_Win32::FinishInitiate(){

                           //now continue in connection of other waiting sockets
   for(int i=0; i<sockets.size(); i++){
      C_socket_win32 *s = (C_socket_win32*)sockets[i];
      if(s->initiating){
         s->CancelTimeOutTimer();
         s->ConnectByHost();
      }
   }
}
#endif

//----------------------------
#ifdef USE_SOCKET_PCAP_SIM
#include <C_file.h>

class C_socket_pcap_sim: public C_socket_win32{

   static dword HostToIp(const char *cp){

      dword ip = 0;
      int di = 4;
      dword n = 0;
      while(true){
         char c = *cp++;
         if(c=='.' || c==0){
            ip |= dword(n)<<(--di*8);
            if(!c)
               return (di==0) ? ip : 0;
            n = 0;
         }else
         if(c>='0' && c<='9'){
            n = n*10 + (c-'0');
            if(n>=256)
               return 0;
         }else
            return 0;
      }
   }
public:

   C_socket_pcap_sim(C_connection_Win32 *_con, const wchar *fn, const char *filter_host, void *event_context):
      C_socket_win32(_con, event_context, false, false, NULL)
   {
      C_file fl;
      if(!fl.Open(fn))
         return;
                              //read pcap header
      struct{
         dword magic;
         word major, minor;
         int thiszone;
         dword sigfigs;
         dword snaplen;
         dword network;
      } hdr;
      if(!fl.Read(&hdr, sizeof(hdr)))
         return;
      dword filter_ip = 0;
      if(filter_host)
         filter_ip = HostToIp(filter_host);
      while(!fl.IsEof()){
         struct{
            dword ts_sec;
            dword ts_usec;
            dword incl_len;
            dword orig_len;
         } hdr;
         if(!fl.Read(&hdr, sizeof(hdr)))
            return;
         fl.Seek(fl.Tell()+14+12);
         dword src_ip;
         fl.ReadDword(src_ip);
         src_ip = ByteSwap32(src_ip);
         fl.Seek(fl.Tell()+4+20);
         dword sz = hdr.incl_len-0x36;
         if(src_ip==1 || (filter_ip && src_ip!=filter_ip))
            fl.Seek(fl.Tell()+sz);
         else{
            t_recv_block *bl = new t_recv_block;
            bl->Resize(sz);
            fl.Read(bl->Begin(), sz);
            blocks.push_back(bl);
         }
      }
   }
   void Connect(const char *hname, word port, int override_timeout = 0){
      win::PostMessage(con->hwnd_async, WM_SOCKET_CONNECTED, win::WPARAM(this), socket_id);
   }
   virtual void _Receive(int override_timeout = 0){
      win::PostMessage(con->hwnd_async, WM_SOCKET_DATA_RECEIVE_SIM, win::WPARAM(this), socket_id);
   }
   virtual void SendData(const void *str, dword len, int override_timeout = 0, bool is_login = false){
                              //just discard
      win::PostMessage(con->hwnd_async, WM_SOCKET_DATA_SENT_SIM, win::WPARAM(this), socket_id);
   }
   virtual dword GetLocalIp() const{ return 0; }
   virtual bool BeginSSL(){ return false; }
   virtual void CancelTimeOut(){}
};

//----------------------------

C_socket *CreateSocketPcap(const wchar *fn, C_internet_connection *con, void *event_context, const char *filter_ip){
   return new C_socket_pcap_sim((C_connection_Win32*)con, fn, filter_ip, event_context);
}

#endif//_DEBUG

//----------------------------

win::LRESULT C_connection_Win32::WndProc(win::HWND hwnd, win::UINT msg, win::WPARAM wParam, win::LPARAM lParam){

   switch(msg){
   case WM_SOCKET_SET_ERROR:
   case WM_SOCKET_CONNECTED:
   case WM_SOCKET_FINISHED:
   case WM_SSL_HANDSHAKE:
#ifdef USE_SOCKET_PCAP_SIM
   case WM_SOCKET_DATA_SENT_SIM:
   case WM_SOCKET_DATA_RECEIVE_SIM:
#endif
      AddRef();               //keep ref, for case that client releases this in callback
      for(int i=sockets.size(); i--; ){
         C_socket_win32 *s = (C_socket_win32*)sockets[i];
         if(win::WPARAM(s)==wParam && s->socket_id==dword(lParam)){
            s->AsyncProcess(msg);
            break;
         }
      }
      Release();
      return 0;

   case WM_TIMER:
      AddRef();               //keep ref, for case that client releases this in callback
      if(wParam==1){
                              //update connections on all sockets
                              //count upwards, always check sockets size
         for(int i=0; i<sockets.size(); i++){
            C_socket_win32 *s = (C_socket_win32*)sockets[i];
            s->Tick();
         }
      }else{
                              //time-out elapsed for some socket
         int i;
         for(i=sockets.size(); i--; ){
            C_socket_win32 *s = (C_socket_win32*)sockets[i];
            if((win::WPARAM)s==wParam){
               s->TimeOut();
               break;
            }
         }
         assert(i!=-1);
      }
      Release();
      return 0;

#ifdef _WIN32_WCE
   case WM_CONNECT_EVENT:
      if(h_con){
         dword st;
         win::HRESULT hr = win::ConnMgrConnectionStatus(h_con, &st);
         if(hr<0)
            st = CONNMGR_STATUS_DISCONNECTED;

         if(sockets.size()){
            C_socket_base *s = (C_socket_base*)sockets.front();
            s->LOG("Connect event: ");
            s->LOG_N(st);
            s->LOG("\n");
         }
         const char *err = NULL;
         switch(st){
         case CONNMGR_STATUS_CONNECTED:
            if(is_initiating){
               is_initiating = false;
               is_initiated = true;
               FinishInitiate();
            }
            break;
         case CONNMGR_STATUS_DISCONNECTED: err = "Connection is disconnected"; break;
         case CONNMGR_STATUS_NOPATHTODESTINATION: err = "No path could be found to destination"; break;
         case CONNMGR_STATUS_CONNECTIONCANCELED: err = "User aborted connection"; break;
         case CONNMGR_STATUS_CONNECTIONDISABLED: err = "Connection is ready to connect but disabled"; break;
         case CONNMGR_STATUS_CONNECTIONFAILED: err = "Connection failed and cannot not be reestablished"; break;
         }
         if(err){
            for(int i=0; i<sockets.size(); i++){
               C_socket_win32 *s = (C_socket_win32*)sockets[i];
               s->SetSocketError(err);
            }
            CloseHcon();
         }
      }//else
         //assert(0);
      return 0;
#endif
   }
   return win::DefWindowProc(hwnd, msg, wParam, lParam);
}

//----------------------------
//----------------------------
#ifdef _WIN32_WCE
#pragma optimize("g", off)  //override bug on WM5, ConnMgrEnumDestinations corrupts r4
#endif

void C_internet_connection::EnumAccessPoints(C_vector<S_access_point> &aps, C_application_base *app){

   aps.clear();
#ifdef _WIN32_WCE
   for(int i=0; ; i++){
      win::CONNMGR_DESTINATION_INFO ci;
      MemSet(&ci, 0, sizeof(ci));
      win::HRESULT hr = win::ConnMgrEnumDestinations(i, &ci);
      if(hr<0)
         break;
      aps.push_back(S_access_point());
      S_access_point &ap = aps.back();
      ap.name = ci.szDescription;
      ap.id = i;
   }
#else
                              //just for test
   {
      aps.push_back(S_access_point());
      S_access_point &ap = aps.back();
      ap.name = L"Internet access";
      ap.id = 14;
   }
   {
      aps.push_back(S_access_point());
      S_access_point &ap = aps.back();
      ap.name = L"WAP IAP";
      ap.id = 11;
   }
#endif
}
#ifdef _WIN32_WCE
#pragma optimize("", on)
#endif

//----------------------------
//----------------------------

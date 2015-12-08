//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <E32Std.h>
#include <es_sock.h>
#include <in_sock.h>
#include <CommDb.h>
#include <EikEnv.h>
#include <SecureSocket.h>
#include <Hal.h>
#include <EikEnv.h>

#include "..\Common\SocketBase.h"

//----------------------------

#define USE_RCONNECTION       //use RConnection class
#include <CommDbConnPref.h>
#include <es_enum.h>

#if (defined S60 && defined __SYMBIAN_3RD__) && !defined _DEBUG
#define USE_DESTINATIONS
#endif

//----------------------------
class C_socket_symbian;

class C_connection_symbian: public C_internet_connection_base{
public:
   dword last_resolved_address;  //IP
   int iap_id[2];             //access-point to use (id)
   int iap_index, used_iap_index;

   RSocketServ sock_srv;
   bool sock_srv_inited;
#ifdef USE_RCONNECTION
   RConnection r_con;
   TCommDbConnPref con_prefs;
#ifdef USE_DESTINATIONS
   TConnPref con_pref_dest;
#endif
#endif
   bool is_initiating;
   bool is_initiated;         //true if initiation process is finished

//----------------------------

   C_connection_symbian(C_socket_notify *notify, int id, int sec_id, int to):
      C_internet_connection_base(notify, to),
      iap_index(0),
      used_iap_index(0),
      is_initiating(false),
      is_initiated(false),
      last_resolved_address(0),
      sock_srv_inited(false)
   {
      iap_id[0] = id;
      iap_id[1] = sec_id;
   }

//----------------------------

   void Init(){
      if(!sock_srv_inited){
         sock_srv.Connect(16);
         sock_srv_inited = true;
#ifdef USE_RCONNECTION
         r_con.Open(sock_srv);
         //r_con.Open(sock_srv, KAfInet);
         //r_con.Start();
#endif
      }
   }

//----------------------------

   ~C_connection_symbian(){
      assert(!sockets.size());
      Close();
   }

//----------------------------

   virtual dword GetIapIndex() const{ return used_iap_index; }

//----------------------------

   void Close(){
      if(sock_srv_inited){
#ifdef USE_RCONNECTION
         r_con.Close();
#endif
         sock_srv.Close();
         sock_srv_inited = false;
      }
      is_initiated = false;
      is_initiating = false;
   }

//----------------------------
// Close connection if all sockets are in initiation phase.
// If alternative iap id is defined, initiate connection with this and return true.
   bool SafeClose(C_socket_symbian *s);

//----------------------------

   void FinishInitiate();
};

//----------------------------

C_internet_connection *CreateConnection(C_socket_notify *notify, int iap_id, int time_out, int secondary_iap_id){

   return new(true) C_connection_symbian(notify, iap_id, secondary_iap_id, time_out);
}

//----------------------------

class C_socket_symbian: public C_socket_base{
   friend class C_connection_symbian;

   bool was_connecting;
   bool socket_opened;

//----------------------------

   class C_time_out_timer: public CTimer{
      C_socket_symbian *notify;
      virtual void RunL(){
         notify->TimerExpired();
      }
   public:
      C_time_out_timer(C_socket_symbian *n):
         CTimer(EPriorityStandard),
         notify(n)
      {}
      void ConstructL(){
         CTimer::ConstructL();
         CActiveScheduler::Add(this);
      }
      ~C_time_out_timer(){
#ifdef _DEBUG
         //notify->LOG("~timer\n");
#endif
         Cancel();
      }
      void After(int t){
         Cancel();
         if(t>0)
            CTimer::After(t*1000);
      }
   } *timer;
   friend class C_time_out_timer;

//----------------------------

   void NotifyError(int st){

      if(IsLogging())
         DisplayErrorInLog(st);
      socket_opened = false;
      MakeErrorText(st);
      Notify(con->socket_notify, C_socket_notify::SOCKET_ERROR);
   }

//----------------------------

   class C_scheduler: public CActive{
   protected:
      C_socket_symbian *notify;
   public:
      C_scheduler(C_socket_symbian *n):
         /*
#ifdef _DEBUG
         CActive(EPriorityUserInput+1),   //must have this priority on S60 emulator, so that it's prioritized over main ticker (otherwise we don't get data)
#else
         */
         CActive(EPriorityStandard),
//#endif
         notify(n)
      {
         CActiveScheduler::Add(this);
      }
      ~C_scheduler(){
         Deque();
      }
      void SetActive(){
         CActive::SetActive();
      }
   };

//----------------------------
   class C_scheduler_send;
   friend class C_scheduler_send;

   void CancelSend(){

      timer->Cancel();
      if(ssl_sock)
         ssl_sock->CancelSend();
      else
         socket.CancelSend();
   }

//----------------------------

   void DataSent(int st){

      if(SchedulerProcessCommon(st))
         return;
      //LOG("data sent\n");

      if(st==KErrNone){
         C_scheduler_send &ss = *scheduler_send;
         con->data_sent += ss.send_buf.Size();
         ss.send_buf.Clear();
         if(ss.send_queue.size()){
                              //send queued data
            const char *bp = ss.send_queue.front();
            ss.send_queue.pop_front();
            SendData(bp+8, *(dword*)bp, *(int*)(bp+4), false);
            delete[] bp;
         }else{
                              //set timer for future receive operation
            int to = time_out;
            if(to>=0)
               timer->After(to);

            Notify(con->socket_notify, C_socket_notify::SOCKET_DATA_SENT);
         }
      }else{
         NotifyError(st);
      }
   }

//----------------------------

   class C_scheduler_send: public C_scheduler{
   public:
      virtual void DoCancel(){
         notify->CancelSend();
      }
      virtual void RunL(){
         notify->DataSent(iStatus.Int());
      }
   public:
      C_scheduler_send(C_socket_symbian *n):
         C_scheduler(n)
      {}
      ~C_scheduler_send(){
         Cancel();
         ClearSendQueue();
      }

      C_buffer<char> send_buf;
      TPtrC8 send_ptr;

      C_vector<char*> send_queue;   //used if sending is in progress while sending another data
      void ClearSendQueue(){
         for(int i=send_queue.size(); i--; )
            delete[] send_queue[i];
         send_queue.clear();
      }
   } *scheduler_send;

//----------------------------
   class C_scheduler_receive;
   friend class C_scheduler_receive;

   void CancelReceive(){

      if(ssl_sock)
         ssl_sock->CancelRecv();
      else
         socket.CancelRecv();
   }

//----------------------------

   void DataReceived(int st){

      if(SchedulerProcessCommon(st))
         return;

      if(st==KErrNone){
         int len = scheduler_receive->recv_buf.Length();
         assert(len);
         const byte *src = scheduler_receive->recv_buf.Ptr();
         if(IsLogging())
            AddToLog(src, len);
         con->data_received += len;

         t_recv_block *bl = new(true) t_recv_block;
         bl->Resize(len);
         blocks.push_back(bl);

         getline_has_new_data = true;

         MemCpy(bl->Begin(), src, len);

         //if(auto_receive_mode)
         AddRef();
         Notify(con->socket_notify, C_socket_notify::SOCKET_DATA_RECEIVED);
         if(Release()!=0){
                           //receive next data
            if(scheduler_receive && (!max_cache_data || GetNumAvailableData()<max_cache_data)){
               ReceiveBlock(save_override_timeout);
            }
         }
      }else{
         NotifyError(st);
      }
   }

//----------------------------

   class C_scheduler_receive: public C_scheduler{
      virtual void DoCancel(){
         notify->CancelReceive();
      }
      virtual void RunL(){
         notify->DataReceived(iStatus.Int());
      }
   public:
      C_scheduler_receive(C_socket_symbian *n):
         C_scheduler(n)
      {}
      ~C_scheduler_receive(){
         Cancel();
      }

      TSockXfrLength rd_len;     //redundand, needed for CSocked::RecvOneOrMore, but not used
#ifdef _DEBUG_
      TBuf8<40> recv_buf;
#else
      TBuf8<2048> recv_buf;
#endif
   } *scheduler_receive;

//----------------------------
   class C_scheduler_lookup;
   friend class C_scheduler_lookup;

//----------------------------

   static Cstr_c MakeIpString(dword ip){
      Cstr_c s;
      s.Format("%.%.%.%") <<int((ip>>24)&0xff) <<int((ip>>16)&0xff) <<int((ip>>8)&0xff) <<int(ip&0xff);
      return s;
   }

//----------------------------

   void LookupDone(int st){

      if(SchedulerProcessCommon(st))
         return;

      if(st==KErrNone){
                           //DNS look up successful
         const TNameRecord &nr = scheduler_lookup->iNameEntry();
                           //antihack: don't allow result from hosts file, possible trickery
         if(nr.iFlags&EDnsHostsFile){
            st = -5122; //DndInternalError
            dword ip = TInetAddr::Cast(nr.iAddr).Address();
            if(IsLogging()){
               Cstr_c s; s<<"DNS: " <<MakeIpString(ip) <<'\n';
               LOG(s);
            }
         }
      }
      if(st==KErrNone){

         LOG_FULL("Lookup ok\n");
         con->iap_index = 0;

         const TNameRecord &nr = scheduler_lookup->iNameEntry();
                           //extract domain name and IP address from name record
         TBuf<15> ipAddr;
         TInetAddr::Cast(nr.iAddr).Output(ipAddr);

         dword ip = TInetAddr::Cast(nr.iAddr).Address();

         TBuf8<256> hn;
         hn.Copy(host_name);
         hn.Append(0);
         con->last_resolved_hostname = (char*)hn.Ptr();
         con->last_resolved_address = ip;

         delete scheduler_lookup; scheduler_lookup = NULL;

         ConnectByIP(ip);
#ifndef USE_RCONNECTION
         if(con->is_initiating)
            con->FinishInitiate();
#endif
      }else{
         delete scheduler_lookup; scheduler_lookup = NULL;

         LOG_FULL("Lookup failed\n");
         assert(!ssl_sock);
         if(con->SafeClose(this))
            return;
         NotifyError(st);
      }
   }

//----------------------------

   class C_scheduler_lookup: public C_scheduler{
      virtual void DoCancel(){
         resolver.Cancel();
      }
      virtual void RunL(){
         notify->LookupDone(iStatus.Int());
      }
   public:
      C_scheduler_lookup(C_socket_symbian *n):
         C_scheduler(n)
      {}
      ~C_scheduler_lookup(){
         Cancel();
         resolver.Close();
      }

      RHostResolver resolver;
      TNameEntry iNameEntry;
   } *scheduler_lookup;

//----------------------------
   class C_scheduler_initiate;
   friend class C_scheduler_initiate;

   void InitiateCancel(){
#if defined USE_RCONNECTION
      con->r_con.Stop();
#endif
   }

//----------------------------

   void InitiateDone(int st){

      if(SchedulerProcessCommon(st))
         return;

      if(st>=0){
         LOG_FULL("Initiate ok\n");
         con->iap_index = 0;

         delete scheduler_initiate; scheduler_initiate = NULL;
#if defined USE_RCONNECTION
         socket.Open(con->sock_srv, KAfInet, KSockStream, KProtocolInetTcp, con->r_con);
         socket_opened = true;
#endif
         ConnectByHost();
         if(con->is_initiating)
            con->FinishInitiate();
      }else{
                              //error
         LOG_FULL("Initiate failed\n");
         if(con->SafeClose(this))
            return;
                              //notify error on this and all other initiating sockets
         C_vector<C_smart_ptr<C_socket_symbian> > sockets;
         for(int i=con->sockets.size(); i--; ){
            C_socket_symbian *s = (C_socket_symbian*)con->sockets[i];
            if(s==this || s->scheduler_initiate)
               sockets.push_back(s);
         }
         for(int i=sockets.size(); i--; )
            sockets[i]->NotifyError(st);
      }
   }

//----------------------------

   class C_scheduler_initiate: public C_scheduler{
      virtual void DoCancel(){
         notify->InitiateCancel();
      }
      virtual void RunL(){
         notify->InitiateDone(iStatus.Int());
      }
   public:
      C_scheduler_initiate(C_socket_symbian *n):
         C_scheduler(n)
      {}
      ~C_scheduler_initiate(){
         Cancel();
      }
   } *scheduler_initiate;

//----------------------------
   class C_scheduler_connect;
   friend class C_scheduler_connect;

   void ConnectCancel(){
      socket.CancelConnect();
   }

//----------------------------

   void ConnectDone(int st){

      if(SchedulerProcessCommon(st))
         return;

      if(st==KErrNone){
         LOG("Connected: ");
         LOG(con->last_resolved_hostname);
         LOG(":");
         LOG_N(dst_port);

         con->iap_index = 0;

         delete scheduler_connect; scheduler_connect = NULL;
         if(use_ssl){
            LOG("\nInit SSL\n");
            InitSSL(false);
            return;
         }

         ReceiveBlock();
         Notify(con->socket_notify, C_socket_notify::SOCKET_CONNECTED);
#ifndef USE_RCONNECTION
         if(con->is_initiating)
            con->FinishInitiate();
#endif
      }else{
         socket_opened = false;
         LOG_FULL("Connect failed\n");
         if(con->SafeClose(this)){
            LOG_FULL("No SafeClose\n");
            return;
         }
         LOG_FULL("SafeClose OK\n");
         NotifyError(st);
      }
   }

//----------------------------

   class C_scheduler_connect: public C_scheduler{
      virtual void DoCancel(){
         notify->ConnectCancel();
      }
      virtual void RunL(){
         notify->ConnectDone(iStatus.Int());
      }
   public:
      C_scheduler_connect(C_socket_symbian *n):
         C_scheduler(n)
      {}
      ~C_scheduler_connect(){
         Cancel();
      }

      TInetAddr addr;
   } *scheduler_connect;

//----------------------------
   class C_scheduler_ssl_init;
   friend class C_scheduler_ssl_init;

   void SslInitCancel(){
      LOG_FULL("cancel SSL handshake\n");
      ssl_sock->CancelAll();
      LOG_FULL(" done\n");
   }

//----------------------------

   void SslInitDone(int st, bool deferred_handshake){

      if(SchedulerProcessCommon(st))
         return;

      AddRef();               //keep ref for case we get destroyed in notify callback
      if(st==KErrNone){
         LOG_FULL("SSL handshake done\n");
         ReceiveBlock();
         Notify(con->socket_notify, deferred_handshake ? C_socket_notify::SOCKET_SSL_HANDSHAKE : C_socket_notify::SOCKET_CONNECTED);
      }else{
         LOG_FULL("SSL handshake failed\n");
         NotifyError(st);
      }
      delete scheduler_ssl_init; scheduler_ssl_init = NULL;
                              //now SSL-init another pending socket (fight Symbian error -11118)
      for(int i=con->sockets.size(); i--; ){
         C_socket_symbian *ss = (C_socket_symbian*)con->sockets[i];
         if(ss->scheduler_ssl_init && !ss->scheduler_ssl_init->started){
            ss->StartSSLHandshake();
            break;
         }
      }
      Release();
   }

//----------------------------

   class C_scheduler_ssl_init: public C_scheduler{
      bool deferred_handshake;

      virtual void DoCancel(){
         notify->SslInitCancel();
      }
      virtual void RunL(){
         notify->SslInitDone(iStatus.Int(), deferred_handshake);
      }
   public:
      bool started;
      C_scheduler_ssl_init(C_socket_symbian *n, bool defer):
         C_scheduler(n),
         deferred_handshake(defer),
         started(false)
      {}
      ~C_scheduler_ssl_init(){
         Cancel();
      }
   } *scheduler_ssl_init;

//----------------------------

   C_smart_ptr<C_connection_symbian> con;

   Cstr_c requested_host_name;

   RSocket socket;

                              //SSL socket
   CSecureSocket *ssl_sock;

//----------------------------

   static const char *GetErrorText(int err){
      static const struct{
         int err;
         const char *text;
      } errs[] = {
         -3, "Operation canceled",
         -4, "Not enough memory",
         -18, "Not ready",
         -21, "Access denied",
         -33, "Time out",
         -34, "Failed to connect",
         -36, "Disconnected",
         -39, "Aborted",
         -191, "Can't connect to server",
         -4161, "Service not subscribed",
         -5120, "No response from DNS server",
         -5121, "Host not found",
         -5122, "DNS internal error",
         -7510, "Invalid SSL message received",
         -17210, "Connection terminated",
         -30180, "WLAN network not found",
         0
      };
      for(int i=0; errs[i].err; i++){
         if(errs[i].err==err){
            return errs[i].text;
         }
      }
      return NULL;
   }

//----------------------------

   void MakeErrorText(int err){
      const char *text = GetErrorText(err);
      if(!text){
         system_error_text.Format(L" Error number %") <<err;
         system_error_text.Build();
      }else{
         system_error_text.Copy(text);
      }
   }

//----------------------------

   void DisplayErrorInLog(int err) const{
      Cstr_c s;
      s.Format("Error: %") <<err;
      const char *text = GetErrorText(err);
      if(text)
         s.AppendFormat(" (%)") <<text;
      s<<'\n';
      AddToLog(s, s.Length());
   }

//----------------------------

   virtual dword GetLocalIp() const{

#ifdef __WINS__
      return 0xc0a80001;
#else
      TInetAddr na;
      const_cast<RSocket&>(socket).LocalName(na);
      return na.Address();
#endif
   }

//----------------------------
// Process common tasks for all scheduler. Return true if processing should not continue.
   bool SchedulerProcessCommon(int st){

      con->ResetIdle();
      timer->Cancel();
      if(st==KErrEof){
         Notify(con->socket_notify, C_socket_notify::SOCKET_FINISHED);
         return true;
      }
      return false;
   }

//----------------------------

   virtual void DiscardData(dword size){

      C_socket_base::DiscardData(size);
      //if(scheduler_receive && scheduler_receive->IsActive() && (!max_cache_data || GetNumAvailableData()<max_cache_data)){
      if(max_cache_data && scheduler_receive && !scheduler_receive->IsActive() && GetNumAvailableData()<max_cache_data){
                              //resume receive
         //Fatal("resume");
         ReceiveBlock(save_override_timeout);
      }
   }

//----------------------------

   void TimerExpired(){

      LOG("TimerExpired\n");
      delete scheduler_send; scheduler_send = NULL;
      delete scheduler_receive; scheduler_receive = NULL;
      NotifyError(KErrTimedOut);
   }
private:
//----------------------------

   TBuf<256> host_name;

   int save_override_timeout;

//----------------------------

   void ReceiveBlock(int override_timeout = 0){

      if(!scheduler_receive)
         scheduler_receive = new(true) C_scheduler_receive(this);
      C_scheduler_receive &sr = *scheduler_receive;
      if(!sr.IsActive()){
         *(int*)sr.rd_len.Ptr() = 0;
         sr.recv_buf.SetLength(0);
         if(ssl_sock)
            ssl_sock->RecvOneOrMore(sr.recv_buf, sr.iStatus, sr.rd_len);
         else
            socket.RecvOneOrMore(sr.recv_buf, 0, sr.iStatus, sr.rd_len);
         sr.SetActive();
      }
      int to = override_timeout ? override_timeout : time_out;
      if(to>=0)
         timer->After(to);
   }

//----------------------------

   void StartSSLHandshake(){
      assert(ssl_sock);
      assert(scheduler_ssl_init);
      ssl_sock->StartClientHandshake(scheduler_ssl_init->iStatus);
      scheduler_ssl_init->SetActive();
      scheduler_ssl_init->started = true;

      timer->After(time_out);
      LOG_FULL("SSL inited\n");
   }

//----------------------------

   void InitSSL(bool deferred_handshake){

                              //workaround for bug -11118
      int num_ssl_conns = 0;
      for(int i=con->sockets.size(); i--; ){
         C_socket_symbian *ss = (C_socket_symbian*)con->sockets[i];
         if(ss->scheduler_ssl_init)
            ++num_ssl_conns;
      }
      //socket.SetOpt(KSoDialogMode, KSSLDialogUnattendedMode);
      ssl_sock = CSecureSocket::NewL(socket, _L("TLS1.0"));
#ifdef __SYMBIAN_3RD__
      //ssl_sock->FlushSessionCache();
      ssl_sock->SetOpt(KSoSSLDomainName, KSolInetSSL, TPtrC8((const byte*)(const char*)requested_host_name));
      ssl_sock->SetDialogMode(EDialogModeAttended);
#endif
      assert(!scheduler_ssl_init);
      scheduler_ssl_init = new(true) C_scheduler_ssl_init(this, deferred_handshake);

      if(num_ssl_conns<3)
         StartSSLHandshake();
   }

//----------------------------
public:
   C_socket_symbian(C_connection_symbian *c, void *event_context, bool ssl, const wchar *log_file_name):
      C_socket_base(c, event_context, log_file_name, c->time_out, ssl),
      socket_opened(false),
      was_connecting(false),
      con(c),
      save_override_timeout(0),
      scheduler_send(NULL),
      scheduler_receive(NULL),
      scheduler_lookup(NULL),
      scheduler_initiate(NULL),
      scheduler_connect(NULL),
      scheduler_ssl_init(NULL),
      ssl_sock(NULL)
   {
   }

//----------------------------

   ~C_socket_symbian(){

      LOG_FULL("Destroying socket\n");
      delete scheduler_send;
      delete scheduler_receive;
      delete scheduler_lookup;
      delete scheduler_initiate;
      delete scheduler_connect;
      delete scheduler_ssl_init;
      if(socket_opened && con->is_initiated){
         LOG_FULL("Socket cancel");
         socket.CancelAll();
         LOG_FULL(": OK\n");
      }
      if(ssl_sock){
         LOG_FULL("SSLsock close");
         ssl_sock->Close();
         delete ssl_sock;
         LOG_FULL(": OK\n");
      }
      if(socket_opened && con->is_initiated){
         LOG_FULL("Socket close\n");
         socket.Close();
      }
      delete timer;

      for(int i=con->sockets.size(); i--; ){
         if(con->sockets[i]==this){
            con->sockets.remove_index(i);

            if(!con->sockets.size()){
               if(con->is_initiating || !was_connecting){
                                       //connection was not initialized, close it so that other sockets don't stall
                  LOG_FULL("Closing uninitiated connection\n");
                  con->Close();
               }
            }
            break;
         }
      }
      LOG_FULL("Ok\n");
   }

//----------------------------

   inline bool IsSocketOpened() const{ return socket_opened; }

//----------------------------

   void Close(){
                              //cancel any outstanding operation, and close socket
      delete scheduler_send; scheduler_send = NULL;
      delete scheduler_receive; scheduler_receive = NULL;
      delete scheduler_lookup; scheduler_lookup = NULL;
      delete scheduler_initiate; scheduler_initiate = NULL;
      delete scheduler_connect; scheduler_connect = NULL;
      delete scheduler_ssl_init; scheduler_ssl_init = NULL;

      timer->Cancel();
      if(ssl_sock){
         ssl_sock->Close();
         delete ssl_sock;
         ssl_sock = NULL;
      }
      if(socket_opened){
         socket.Close();
         socket_opened = false;
      }
      DeleteAllBlocks();
   }

//----------------------------

   void Construct(){

      timer = new(true) C_time_out_timer(this);
      timer->ConstructL();
   }

//----------------------------

   void ConnectByHost();

//----------------------------

   void ConnectByIP(dword ip);

//----------------------------

   void ConnectInternal(){

      if(!con->sock_srv_inited){
         con->Init();
         LOG_FULL("Connection inited\n");
      }
      assert(!socket_opened);
      int err = 0;
#ifdef USE_RCONNECTION
      if(!con->is_initiated){
         assert(!scheduler_initiate);
         scheduler_initiate = new(true) C_scheduler_initiate(this);

         if(!con->is_initiating){
            con->is_initiating = true;

            scheduler_initiate->iStatus = KRequestPending;

#ifdef __SYMBIAN_3RD__
            con->con_prefs.SetBearerSet(ECommDbBearerWcdma | ECommDbBearerCSD | ECommDbBearerCdma2000 | ECommDbBearerWLAN | ECommDbBearerLAN | ECommDbBearerVirtual);
#endif
            con->con_prefs.SetDirection(ECommDbConnectionDirectionOutgoing);

            con->used_iap_index = con->iap_index;
            int iap_id = con->iap_id[con->iap_index];
            TConnPref *pref = &con->con_prefs;
            if(iap_id!=-1){
               LOG_FULL("Using predefined IAP\n");
                                 //use predefined IAP, and don't display dialog
#ifdef USE_DESTINATIONS
               if(iap_id&0x80000000){
                  RLibrary lib;
                  if(!lib.Load(_L("z:esock"))){
                     typedef void (*t_SetSnap)(TConnPref*, TUint32 aSnap); t_SetSnap SetSnap = (t_SetSnap)lib.Lookup(310);
                     typedef void (*t_TConnSnapPref_Ctr)(TConnPref*); t_TConnSnapPref_Ctr TConnSnapPref_Ctr = (t_TConnSnapPref_Ctr)lib.Lookup(312);
                     if(SetSnap && TConnSnapPref_Ctr){
                        TConnSnapPref_Ctr(&con->con_pref_dest);
                        SetSnap(&con->con_pref_dest, iap_id&0x7fffffff);
                        pref = &con->con_pref_dest;
                     }
                     lib.Close();
                  }
               }else
#endif
               {
                  con->con_prefs.SetIapId(iap_id);
                  con->con_prefs.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
               }
            }else{
               LOG_FULL("Using automatic connection\n");
                              //force dialog
               con->con_prefs.SetDialogPreference(ECommDbDialogPrefPrompt);
            }
            con->r_con.Start(*pref, scheduler_initiate->iStatus);
            scheduler_initiate->SetActive();
         }else{
                              //some other socket is initiating connection, pause this socket until initiated
            LOG_FULL("Waiting for init\n");
         }
                                 //request time out
         int to = save_override_timeout ? save_override_timeout : time_out;
         if(to>=0)
            timer->After(to);
         return;
      }else{
         LOG_FULL("Connection already active, using it\n");
                              //connection already started, just use the connection
         err = socket.Open(con->sock_srv, KAfInet, KSockStream, KProtocolInetTcp, con->r_con);
         if(err){
                              //the connection is dead, close it and try again
            LOG_FULL("Connection error ");
            LOG_FULL_N(err);
            LOG_FULL(", close and retry\n");
            con->Close();
            Connect(requested_host_name, dst_port, save_override_timeout);
            return;
         }
      }
#else
      {
         err = socket.Open(con->sock_srv, KAfInet, KSockStream, KProtocolInetTcp);
      }
#endif
      if(err){
         //con->Close();
         LOG("Open socket failed\n");
                              //schedule error asynchronously in RunL
         scheduler_connect = new(true) C_scheduler_connect(this);
         scheduler_connect->SetActive();
         TRequestStatus *rs = &scheduler_connect->iStatus;
         User::RequestComplete(rs, err);
         return;
      }
      socket_opened = true;

      //socket.SetOpt(KSoTcpKeepAlive, KSolInetTcp, 1);

      LOG_FULL("Socket opened\n");
#ifndef USE_RCONNECTION
      if(!con->is_initiated){
         if(!con->is_initiating){
            con->is_initiating = true;
         }else{
            return;
         }
      }
#endif
      ConnectByHost();
   }

//----------------------------

   virtual void Connect(const char *hname, word port, int override_timeout){

      LOG("Connect\n");
      Close();
      con->ResetIdle();
                              //save host name
      requested_host_name = PunyEncode(hname);
      dst_port = port;
      save_override_timeout = override_timeout;

      ConnectInternal();
   }

//----------------------------
   /*
   virtual void _Receive(int override_timeout){

      con->ResetIdle();
      getline_has_new_data = false;

      save_override_timeout = override_timeout;
      //ReceiveBlock(override_timeout);

      int to = override_timeout ? override_timeout : time_out;
      if(to>=0)
         timer->After(to);
   }
   */

//----------------------------

   virtual void SetTimeout(int t){

      save_override_timeout = 0;
      C_socket_base::SetTimeout(t);
      if(time_out>0)
         timer->After(time_out);
      else
         CancelTimeOut();
   }

//----------------------------

   virtual void CancelTimeOut(){

      timer->Cancel();
      save_override_timeout = -1;
   }

//----------------------------

   virtual void SendData(const void *buf, dword len, int override_timeout, bool disable_logging){

      con->ResetIdle();
      if(scheduler_send){
         if(scheduler_send->IsActive()){
                              //buffer data
            assert(scheduler_send->send_buf.Size());
            char *bp = new(true) char[len+8];
            *(dword*)bp = len;
            *(int*)(bp+4) = override_timeout;
            MemCpy(bp+8, buf, len);
            scheduler_send->send_queue.push_back(bp);
            return;
         }
      }else
         scheduler_send = new(true) C_scheduler_send(this);

      C_scheduler_send &ss = *scheduler_send;

      assert(!ss.send_buf.Size());

      if(IsLogging()){
         LOG(":> ");
         if(!disable_logging){
            //Cstr_c s; s.Allocate((const char*)buf, len);
            //LOG(s);
            LOGB(buf, len);
         }else
            LOG("<login command sent>\n");
      }

      ss.send_buf.Resize(len);
      MemCpy(ss.send_buf.Begin(), buf, len);
      ss.send_ptr.Set((byte*)ss.send_buf.Begin(), len);

      if(ssl_sock)
         ssl_sock->Send(ss.send_ptr, ss.iStatus);
      else
         socket.Send(ss.send_ptr, 0, ss.iStatus);
      ss.SetActive();

      int to = override_timeout ? override_timeout : time_out;
      if(to>=0)
         timer->After(to);
   }

//----------------------------

   virtual bool BeginSSL(){

                              //cancel current operation
      delete scheduler_receive; scheduler_receive = NULL;
      delete scheduler_send; scheduler_send = NULL;
      timer->Cancel();
      use_ssl = true;
      InitSSL(true);
      return true;
   }
};

//----------------------------

static bool HostToIp(const char *cp, dword &ip){

   ip = 0;
   int di = 4;
   dword n = 0;
   while(true){
      char c = *cp++;
      if(c=='.' || c==0){
         ip |= dword(n)<<(--di*8);
         if(!c)
            return (di==0);
         n = 0;
      }else
      if(c>='0' && c<='9'){
         n = n*10 + (c-'0');
         if(n>=256)
            return false;
      }else
         return false;
   }
}

//----------------------------

void C_socket_symbian::ConnectByHost(){

   LOG("Connect to host ");
   LOG(requested_host_name);
   LOG(":");
   LOG_N(dst_port);
   LOG("\n");
   if(con->last_resolved_hostname==requested_host_name){
      ConnectByIP(con->last_resolved_address);
   }else{
                              //check if host name is directly specified IP
      dword ip;
      if(HostToIp(requested_host_name, ip)){
         ConnectByIP(ip);
         return;
      }
                              //initiate DNS
      assert(!scheduler_lookup);
      scheduler_lookup = new(true) C_scheduler_lookup(this);
      int err;
#ifdef USE_RCONNECTION
      err = scheduler_lookup->resolver.Open(con->sock_srv, KAfInet, KProtocolInetUdp, con->r_con);
#else
      err = scheduler_lookup->resolver.Open(con->sock_srv, KAfInet, KProtocolInetUdp);
#endif
      if(err){
         //con->Close();
         socket_opened = false;
         scheduler_lookup->SetActive();
         TRequestStatus *rs = &scheduler_lookup->iStatus;
         User::RequestComplete(rs, err);
         return;
      }
      LOG("Starting resolver\n");
                              //DNS request for name resolution
      host_name.Copy(TPtrC8((byte*)(const char*)requested_host_name, requested_host_name.Length()));
      scheduler_lookup->resolver.GetByName(host_name, scheduler_lookup->iNameEntry, scheduler_lookup->iStatus);
      scheduler_lookup->SetActive();

                              //request time out
      int to = save_override_timeout ? save_override_timeout : time_out;
      if(to>=0)
         timer->After(to);
   }
}

//----------------------------

void C_socket_symbian::ConnectByIP(dword ip){

   if(IsLogging()){
      Cstr_c s;
      s<<"Connect to IP " <<MakeIpString(ip) <<'\n';
      LOG(s);
   }
   dst_ip = ip;
   was_connecting = true;

   assert(!scheduler_connect);
   scheduler_connect = new(true) C_scheduler_connect(this);

   scheduler_connect->addr.SetPort(dst_port);
   scheduler_connect->addr.SetAddress(ip);

   socket.Connect(scheduler_connect->addr, scheduler_connect->iStatus);
   scheduler_connect->SetActive();
   int to = save_override_timeout ? save_override_timeout : time_out;
   if(to>=0)
      timer->After(to);
}

//----------------------------
//----------------------------

C_socket *CreateSocket(C_internet_connection *con, void *event_context, bool use_ssl, bool deferred_ssl_handshake, const wchar *log_file_name){

   C_socket_symbian *sp = new(true) C_socket_symbian((C_connection_symbian*)con, event_context, use_ssl, log_file_name);
   sp->Construct();
   return sp;
}

//----------------------------
//----------------------------

void C_connection_symbian::FinishInitiate(){

   assert(is_initiating);
   is_initiating = false;
   is_initiated = true;
                           //now continue in connection of other waiting sockets
   for(int i=sockets.size(); i--; ){
      C_socket_symbian *s = (C_socket_symbian*)sockets[i];
      if(s->scheduler_initiate && !s->IsSocketOpened()){
#ifdef USE_RCONNECTION
         s->ConnectInternal();
#else
         s->ConnectByHost();
#endif
      }
   }
}

//----------------------------

bool C_connection_symbian::SafeClose(C_socket_symbian *s1){

                              //check if we may close connection (all other sockets are in initiation phase)
   for(int i=sockets.size(); i--; ){
      const C_socket_symbian *s = (C_socket_symbian*)sockets[i];
      if(s==s1)
         continue;
      if(!s->scheduler_initiate)
         return false;
   }
                              //close this socket
   s1->Close();
                              //close connection
   Close();
                              //check if we may try alternative connection
   if(iap_index==1 || iap_id[1]==-1){
                              //reset iap index to primary for next use
      iap_index = 0;
      return false;
   }
                              //yes, use it
   ++iap_index;
   s1->ConnectInternal();
   return true;
}

//----------------------------
//----------------------------
#ifdef USE_DESTINATIONS
/*
#include <AppBase.h>
#include <GulIcon.h>

static void SymbianBitmapToImage(C_image *img, const CFbsBitmap *bmp, const CFbsBitmap *mask){

   if(bmp->SizeInPixels().iWidth)
      Fatal("iWidth", bmp->SizeInPixels().iWidth);
   S_point sz = (S_point&)bmp->SizeInPixels();
   img->Create(sz.x, sz.y);
   S_rgb_x *pixs = new(true) S_rgb_x[sz.x*sz.y];

   for(int y=0; y<sz.y; y++){
      for(int x=0; x<sz.x; x++){
         TRgb p;
         bmp->GetPixel(p, TPoint(x, y));
         pixs[y*sz.x+x].From32bit(p.Internal());
      }
   }
   img->SetPixels(pixs);
   delete[] pixs;

   if(mask && mask->SizeInPixels()==bmp->SizeInPixels()){
      byte *alph = new(true) byte[sz.x*sz.y];
      for(int y=0; y<sz.y; y++){
         for(int x=0; x<sz.x; x++){
            TRgb p;
            mask->GetPixel(p, TPoint(x, y));
            alph[y*sz.x+x] = p.Internal()&0xff;
         }
      }
      img->SetAlphaChannel(alph);
      delete[] alph;
   }
}
*/
#endif//USE_DESTINATIONS
//----------------------------

void C_internet_connection::EnumAccessPoints(C_vector<S_access_point> &aps, C_application_base *app){

   aps.clear();

   CCommsDatabase *cdb = CCommsDatabase::NewL(EDatabaseTypeIAP);
#if (defined S60 && defined __SYMBIAN_3RD__) && 0
   CApSelect *as = CApSelect::NewL(*cdb, KEApIspTypeAll, EApBearerTypeAll, KEApSortUidAscending,
      EIPv4|EIPv6, EVpnFilterBoth, true);
   bool ok = as->MoveToFirst();
   for(int i=0; ok && i<as->Count(); i++){
      aps.push_back(S_access_point());
      S_access_point &ap = aps.back();
      TFileName n = as->Name();
      ap.name = (const wchar*)n.PtrZ();
      ap.id = as->Uid();
      ok = as->MoveNext();
   }
   //CleanupStack::PopAndDestroy(as);
   delete as;
#else
   CCommsDbTableView *db_view = cdb->OpenIAPTableViewMatchingBearerSetLC(KCommDbBearerWcdma | KCommDbBearerCSD
#if defined S60 || defined __SYMBIAN_3RD__
      | KCommDbBearerCdma2000 | KCommDbBearerVirtual// | KCommDbBearerLAN //| KCommDbBearerPAN
#endif
#ifdef __SYMBIAN_3RD__
      | KCommDbBearerWLAN
#endif
      , ECommDbConnectionDirectionOutgoing);
   if(db_view->GotoFirstRecord() == KErrNone){
      do{
         TFileName iIapName;
         dword iap_id;
         db_view->ReadTextL(TPtrC(COMMDB_NAME), iIapName);
         db_view->ReadUintL(TPtrC(COMMDB_ID), iap_id);
         const wchar *name = (wchar*)iIapName.PtrZ();
                              //detect duplicates
         int i;
         for(i=aps.size(); i--; ){
            if(aps[i].name == name)
               break;
         }
         if(i!=-1)
            continue;
         aps.push_back(S_access_point());
         S_access_point &ap = aps.back();
         ap.name = name;
         ap.id = iap_id;
         //Info("id", iap_id);
      }while(db_view->GotoNextRecord() == KErrNone);
   }
   CleanupStack::PopAndDestroy(db_view);
#endif
   delete cdb;

#ifdef USE_DESTINATIONS
                              //add "Destinations" added in S60 3rd FP2
   class C_cm_man{
      NONSHARABLE_CLASS(RCmDestination){
         class CCmDestinationData* iDestinatonData;
      public:
         RCmDestination(): iDestinatonData(NULL){}
      };

      NONSHARABLE_CLASS(RCmManager){
         class CCmManagerImpl* iImplementation;
      public:
         inline RCmManager():iImplementation(NULL) {}
      };

      RLibrary lib;
      bool opened;
   public:
      RCmManager cm;
      RArray<TUint32> da;
      C_application_base *app;

      C_cm_man():
         opened(false)
      {}
      ~C_cm_man(){
         da.Close();
         if(opened){
            typedef void (*t_Close)(RCmManager*);
            t_Close cl = (t_Close)lib.Lookup(11);
            if(cl)
               cl(&cm);
         }
         lib.Close();
      }
      void EnumDestinations(C_vector<S_access_point> &aps){
         if(!lib.Load(_L("z:cmmanager"))){
            //Info("Enum");

            class C_lf_OpenL: public C_leave_func{
            public:
               RCmManager *cm;
               typedef void (*t_OpenL)(RCmManager*);
               t_OpenL OpenL;

               virtual int Run(){
                  OpenL(cm);
                  return 0;
               }
            } lf_OpenL;
            lf_OpenL.OpenL = (C_lf_OpenL::t_OpenL)lib.Lookup(12);
            lf_OpenL.cm = &cm;

            if(lf_OpenL.OpenL){
               int err = lf_OpenL.Execute();
               //Info("Lib open");
               if(!err){
                  opened = true;

                  typedef void (*t_AllDestinationsL)(RCmManager*, RArray<TUint32>& aDestArray); t_AllDestinationsL AllDestinationsL = (t_AllDestinationsL)lib.Lookup(214);
                  typedef void (*t_DestinationL)(RCmDestination*, RCmManager*, TUint32 aDestinationId); t_DestinationL DestinationL = (t_DestinationL)lib.Lookup(213);
                  typedef void (*t_RCmDestination_ctr)(RCmDestination*); t_RCmDestination_ctr RCmDestination_ctr = (t_RCmDestination_ctr)lib.Lookup(305);
                  typedef void (*t_RCmDestination_dtr)(RCmDestination*); t_RCmDestination_dtr RCmDestination_dtr = (t_RCmDestination_dtr)lib.Lookup(52);
                  typedef HBufC *(*t_NameLC)(RCmDestination*); t_NameLC NameLC = (t_NameLC)lib.Lookup(254);
                  typedef TUint32 (*t_Id)(RCmDestination*); t_Id Id = (t_Id)lib.Lookup(252);
                  typedef void (*t_RCmDestination_Close)(RCmDestination*); t_RCmDestination_Close RCmDestination_Close = (t_RCmDestination_Close)lib.Lookup(49);
                  //typedef CGulIcon *(*t_IconL)(const RCmDestination*); t_IconL IconL = (t_IconL)lib.Lookup(253);

                  if(AllDestinationsL && DestinationL && RCmDestination_dtr && RCmDestination_ctr && NameLC && Id && RCmDestination_Close){
                     //Info("AllDests");
                     AllDestinationsL(&cm, da);
                     //Info("Count: ", da.Count());
                     for(int i=0; i<da.Count(); i++){

                        class C_lf: public C_leave_func{
                        public:
                           RCmDestination d;
                           RCmManager *cm;
                           TUint32 id;
                           t_DestinationL DestinationL;

                           virtual int Run(){
                              DestinationL(&d, cm, id);
                              return 0;
                           }
                        }lf;
                        lf.cm = &cm;
                        lf.id = da[i];
                        lf.DestinationL = DestinationL;
                        RCmDestination_ctr(&lf.d);

                        //Info("DestinationL");
                        int err = lf.Execute();
                        if(!err){
                           //Info("NameLC");
                           HBufC *b = NameLC(&lf.d);

                           S_access_point ap;
                           Cstr_w s;
                           s.Allocate((wchar*)b->Ptr(), b->Length());
                           ap.name<<L"* " <<s;
                                       //mark id with high bit set, which means that it is "destination"
                           ap.id = 0x80000000 | Id(&lf.d);
                           /*
                           if(IconL && app){
                              CGulIcon *gi = IconL(&d);
                              if(gi){
                                 C_image *img = C_image::Create(*app);
                                 SymbianBitmapToImage(img, gi->Bitmap(), gi->Mask());
                                 ap.img = img;
                                 ap.bmp = gi->Bitmap();
                                 img->Release();
                              }
                           }
                           */
                           aps.push_back(ap);

                           CleanupStack::PopAndDestroy();
                           //Info("Close");
                           RCmDestination_Close(&lf.d);
                        }
                        //else Info("DestinationL err: ", err);
                        RCmDestination_dtr(&lf.d);
                     }
                     //Info("Done");
                  }
               }
            }
         }
      }
   } cm_man;
   cm_man.app = app;
   cm_man.EnumDestinations(aps);
   //Info("EnumDestinations finish");
#endif
}

//----------------------------

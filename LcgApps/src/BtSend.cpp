//----------------------------

#if defined __SYMBIAN32__ && !defined _DEBUG

//----------------------------

#include <e32std.h>

#include <obex.h>
#include <btmanclient.h>
#include <btextnotifiers.h>
#include <btsdp.h>
#ifdef __SYMBIAN_3RD__
#include <ir_sock.h>
#ifdef S60
#include <FeatureInfo.h> 
#include <FeatDiscovery.h> 
#endif
#endif

#include "Main.h"
#include "FileBrowser.h"
#include <Util.h>

//----------------------------

#ifndef __SYMBIAN_3RD__
static int LoadIrdaDll(RLibrary &lib){
   const TUid irda_uid = { 0x10003d38 };
   const TUidType irda_dll_uid(KNullUid, irda_uid, KNullUid);
   return lib.Load(_L("irda.prt"), irda_dll_uid);
}
#endif//__SYMBIAN_3RD__

//----------------------------

bool C_client_file_mgr::C_obex_file_send::CanSendByBt(){
#ifdef S60
#ifdef __SYMBIAN_3RD__
   return CFeatureDiscovery::IsFeatureSupportedL(KFeatureIdBt);
#else
   return true;
#endif
#else
   return false;
#endif
}

//----------------------------

bool C_client_file_mgr::C_obex_file_send::CanSendByIr(){

#ifdef __SYMBIAN_3RD__
#ifdef S60
   return CFeatureDiscovery::IsFeatureSupportedL(KFeatureIdIrda);
#else
   return true;
#endif
#else
   RLibrary lib;
   int err = LoadIrdaDll(lib);
   if(!err){
      lib.Close();
      return true;
   }
   return false;
#endif
}

//----------------------------

class C_Sdp_attrib_notifier{
public:
   virtual void FoundElementL(int key, CSdpAttrValue &val) = 0;
};

//----------------------------
//----------------------------

class TSdpAttributeParser: public MSdpAttributeValueVisitor{
public:
   enum E_NODE_COMMAND{
      ECheckType,     
      ECheckValue,  
      ECheckEnd,      
      ESkip,        
      EReadValue,    
      EFinished
   };

   struct S_Sdp_attribute_node{
      E_NODE_COMMAND command;
      TSdpElementType type;
      int val;    
   };
private:
   C_Sdp_attrib_notifier &iObserver;
   const S_Sdp_attribute_node *nodes;
   int curr_node;
public:

//----------------------------

   TSdpAttributeParser(const S_Sdp_attribute_node *nl, C_Sdp_attrib_notifier &obs):
      iObserver(obs),
      nodes(nl),
      curr_node(0)
   {}

//----------------------------

   inline bool HasFinished() const{ return CurrentNode().command == EFinished; }

public:
   void VisitAttributeValueL(CSdpAttrValue &aValue, TSdpElementType aType){

      switch(CurrentNode().command){
      case ECheckType:
         CheckTypeL(aType);
         break;
         
      case ECheckValue:
         CheckTypeL(aType);
         CheckValueL(aValue);
         break;
         
      case ECheckEnd:
         User::Leave(KErrGeneral);   //  list element contains too many items
         break;
         
      case ESkip:
         break;  // no checking required
         
      case EReadValue:
         CheckTypeL(aType);
         ReadValueL(aValue);
         break;
         
      case EFinished:
         User::Leave(KErrGeneral);   // element is after value should have ended
         return;
      }
      AdvanceL();
   }

//----------------------------
   
   void StartListL(CSdpAttrValueList &aList){
   }

//----------------------------
   
   void EndListL(){
                              //check we are at the end of a list
      if(CurrentNode().command != ECheckEnd)
         User::Leave(KErrGeneral);
      AdvanceL();
   }
private:
   void CheckTypeL(TSdpElementType aElementType) const{
      if(CurrentNode().type != aElementType)
         User::Leave(KErrGeneral);
   }

//----------------------------
   
   void CheckValueL(CSdpAttrValue &aValue) const{

      switch(aValue.Type()){
      case ETypeNil:
         //Panic(ESdpAttributeParserNoValue);
         User::Panic(_L("Obex"), 1);
         break;
         
      case ETypeUint:
         if(aValue.Uint() != (dword)CurrentNode().val)
            User::Leave(KErrArgument);
         break;
         
      case ETypeInt:
         if(aValue.Int() != CurrentNode().val)
            User::Leave(KErrArgument);
         break;
         
      case ETypeBoolean:
         if(aValue.Bool() != CurrentNode().val)
            User::Leave(KErrArgument);
         break;
         
      case ETypeUUID:
         if(aValue.UUID() != TUUID(CurrentNode().val))
            User::Leave(KErrArgument);
         break;
         
         // these are lists, so have to check contents
      case ETypeDES:
      case ETypeDEA:
      default:
         //Panic(ESdpAttributeParserValueIsList);
         User::Panic(_L("Obex"), 2);
         break;
         
         // these aren't supported - use EReadValue and leave on error
      //case ETypeString:
      //case ETypeURL:
      //case ETypeEncoded:
      //default:
         //Panic(ESdpAttributeParserValueTypeUnsupported);
      }
   }

//----------------------------
   
   inline void ReadValueL(CSdpAttrValue &aValue) const{
      iObserver.FoundElementL(CurrentNode().val, aValue);
   }

//----------------------------
   
   inline const S_Sdp_attribute_node &CurrentNode() const{ return  nodes[curr_node]; }
   
//----------------------------

   inline void AdvanceL(){
                              //check not at end
      if(CurrentNode().command == EFinished)
         User::Leave(KErrEof);
                              //move to the next item
      ++curr_node;
   }
};

//----------------------------

class C_obex_file_send_imp: public C_client_file_mgr::C_obex_file_send, public CActive{
   enum E_STATE{
      STATE_NULL,
      STATE_GETTING_DEVICE,   //only BT, only S60
      STATE_GETTING_SERVICE,  //only BT
      STATE_GETTING_CONNECTION,  //ConnectToServerL
      STATE_SENDING,          //CObexClient::Put
      STATE_DISCONNECTING,    //CObexClient::Disconnect
      STATE_DONE,
      STATE_ERR_CANCELED,
      STATE_ERR_FAILED,
   };
   E_STATE state;

   E_TRANSFER transfer;

   dword bytes_sent_acc;
   const C_vector<Cstr_w> filenames;
   dword curr_file_indx;

//----------------------------
                              //class for searching BT device (on S60) and for discovering OBEX service (port)
   class C_obex_service_searcher: public CBase, public MSdpAgentNotifier, public C_Sdp_attrib_notifier{
      TUUID service_class;
      int port;

      TRequestStatus *status_observer;
      bool is_device_selector_connected;
      RNotifier device_selector;
      TBTDeviceResponseParamsPckg response;
      CSdpAgent *agent;
      CSdpSearchPattern *sdp_search_pattern;

      bool has_found_service;

      enum{
         KRfcommChannel = 1,
                              //see entry for OBEX in table 4.3 "Protocols" at the url www.bluetooth.org/assigned-numbers/sdp.htm
         KBtProtocolIdOBEX = 0x0008,
                              //see entry for OBEXObjectPush in table 4.4 "Service Classes" at the url www.bluetooth.org/assigned-numbers/sdp.htm
         KServiceClass = 0x1105,
      };

   public:
      C_obex_service_searcher():
         is_device_selector_connected(false),
         service_class(KServiceClass),
         port(-1)
      {}

      ~C_obex_service_searcher(){
         if(is_device_selector_connected){
            device_selector.CancelNotifier(KDeviceSelectionNotifierUid);
            device_selector.Close();
         }
         delete sdp_search_pattern;
         delete agent;
      }

   //----------------------------

      void SelectDeviceByDiscoveryL(TRequestStatus &aObserverRequestStatus){

         if(!is_device_selector_connected){
            User::LeaveIfError(device_selector.Connect());
            is_device_selector_connected = true;
         }
                              //request a device selection 
         TBTDeviceSelectionParamsPckg selection_filter;
         selection_filter().SetUUID(ServiceClass());
   
         device_selector.StartNotifierAndGetResponse(aObserverRequestStatus, KDeviceSelectionNotifierUid, selection_filter, response);
      }

   //----------------------------

      const TBTDevAddr &BTDevAddr(){
         return response().BDAddr();
      }

   //----------------------------

      void FindServiceL(TRequestStatus &aObserverRequestStatus){

         /*                   //display device BT address
         const TBTDevAddr &t = BTDevAddr();
         TBuf8<100> s;
         for(int i=0; i<6; i++)
            s.AppendFormat(_L8("%02x"), (int)t[i]);
         Fatal((const char*)s.PtrZ());
         */

         if(!response().IsValidBDAddr())
            User::Leave(KErrNotFound);
         has_found_service = false;
   
                              //delete any existing agent and search pattern
         delete sdp_search_pattern;
         delete agent;

         agent = CSdpAgent::NewL(*this, BTDevAddr());
         sdp_search_pattern = CSdpSearchPattern::NewL();
   
         sdp_search_pattern->AddL(ServiceClass()); 
                              //return code is the position in the list that the UUID is inserted at and is intentionally ignored
         agent->SetRecordFilterL(*sdp_search_pattern);
         status_observer = &aObserverRequestStatus;
         agent->NextRecordRequestL();
      }

   //----------------------------

      //const TBTDeviceResponseParams& ResponseParams(){ return response(); }

   //----------------------------

   private:

      void Finished(int aError = KErrNone){
         if(aError == KErrNone && !has_found_service)
            aError = KErrNotFound;
         User::RequestComplete(status_observer, aError);
      }

   //----------------------------

      bool HasFinishedSearching() const{
         return false;
      }

   //----------------------------

   public:
      void NextRecordRequestComplete(int aError, TSdpServRecordHandle aHandle, int aTotalRecordsCount){

         TRAPD(error, NextRecordRequestCompleteL(aError, aHandle, aTotalRecordsCount););
         if(error != KErrNone) 
            Fatal("NRRC", 0);
      }

   //----------------------------

      void AttributeRequestResult(TSdpServRecordHandle aHandle, TSdpAttributeID aAttrID, CSdpAttrValue* aAttrValue){

         TRAPD(error, AttributeRequestResultL(aHandle, aAttrID, aAttrValue); );
         if(error != KErrNone)
            Fatal("NRRC", 1);
                              //ownership has been transferred
         delete aAttrValue;
      }

   //----------------------------

      void AttributeRequestComplete(TSdpServRecordHandle aHandle, int aError){

         TRAPD(error, AttributeRequestCompleteL(aHandle, aError); );
         if(error != KErrNone)
            Fatal("NRRC", 3);
      }

   private:

      void NextRecordRequestCompleteL(int aError, TSdpServRecordHandle aHandle, int aTotalRecordsCount){

         if(aError == KErrEof){
            Finished();
            return;
         }
         if(aError != KErrNone){
            //iLog.LogL(_L("NRRC ERR"), aError);
            Finished(aError);
            return;
         }
         if(aTotalRecordsCount == 0){
            //iLog.LogL(_L("NRRC No records"));
            Finished(KErrNotFound);
            return;
         }
                              //request its attributes
         agent->AttributeRequestL(aHandle, KSdpAttrIdProtocolDescriptorList);
      }

   //----------------------------

      void AttributeRequestResultL(TSdpServRecordHandle aHandle, TSdpAttributeID aAttrID, CSdpAttrValue* aAttrValue){

         ASSERT(aAttrID == KSdpAttrIdProtocolDescriptorList);
   
         TSdpAttributeParser parser(ProtocolList(), *this);
   
                              //validate the attribute value, and extract the RFCOMM channel
         aAttrValue->AcceptVisitorL(parser);
   
         if(parser.HasFinished()){
                              //found a suitable record so change state
            has_found_service = true;
         }
      }

   //----------------------------

      void AttributeRequestCompleteL(TSdpServRecordHandle, int aError){

         if(aError != KErrNone){
            //iLog.LogL(_L("Can't get attribute "), aError);
         }else
         if(!HasFinishedSearching()){
                              //have not found a suitable record so request another
            agent->NextRecordRequestL();
         }else{
            //iLog.LogL(_L("AttrReqCom"));
            Finished();
         }
      }
   public:

   //----------------------------

      inline int GetPort() const{ return port; }

//----------------------------
   private:
      const TUUID &ServiceClass() const{ return service_class; }

   //----------------------------

      const TSdpAttributeParser::S_Sdp_attribute_node *ProtocolList() const{

         static const TSdpAttributeParser::S_Sdp_attribute_node obex_protocol_list[] = {
            { TSdpAttributeParser::ECheckType, ETypeDES },
            { TSdpAttributeParser::ECheckType, ETypeDES },
            { TSdpAttributeParser::ECheckValue, ETypeUUID, KL2CAP },
            { TSdpAttributeParser::ECheckEnd },
            { TSdpAttributeParser::ECheckType, ETypeDES },
            { TSdpAttributeParser::ECheckValue, ETypeUUID, KRFCOMM },
            { TSdpAttributeParser::EReadValue, ETypeUint, KRfcommChannel },
            { TSdpAttributeParser::ECheckEnd },
            { TSdpAttributeParser::ECheckType, ETypeDES },
            { TSdpAttributeParser::ECheckValue, ETypeUUID, KBtProtocolIdOBEX },
            { TSdpAttributeParser::ECheckEnd },
            { TSdpAttributeParser::ECheckEnd },
            { TSdpAttributeParser::EFinished }
         };
         return obex_protocol_list;
      }

   //----------------------------

      virtual void FoundElementL(int key, CSdpAttrValue& val){
         if(!(key == static_cast<int>(KRfcommChannel)))
            Fatal("Obex", 4);
         port = val.Uint();
      }
   } *service_searcher;

//----------------------------

   CObexClient *obex_client;
   CObexFileObject *obex_obj;

#ifndef __SYMBIAN_3RD__
   RLibrary lib_irda;         //irda.prt

//----------------------------

   class C_IrdaSockAddr: public TSockAddr{
      struct SIrdaAddr{
         TUint iHostDevAddr;
         TUint iRemoteDevAddr;
         TBool iSniff;
         TBool iSolicited;
         TUint8 iIrlapVersion;
         TUint8 iFirstServiceHintByte;
         TUint8 iSecondServiceHintByte;
         TUint8 iCharacterSet;
         TUint8 iServiceHintByteCount;
         // Extra stuff for MUX
         TUint8 iHomePort;
         TUint8 iRemotePort;
         TUint8 iSpare;
      };
      public:
         //IMPORT_C C_IrdaSockAddr();
         //IMPORT_C TIrdaSockAddr(const TSockAddr &aAddr);
         inline static TIrdaSockAddr &Cast(const TSockAddr &aAddr);
         inline static TIrdaSockAddr &Cast(const TSockAddr *aAddr);
         inline TUint GetRemoteDevAddr() const;
         inline void SetRemoteDevAddr(const TUint aRemote);
         inline TUint GetHostDevAddr() const;
         inline void SetHostDevAddr(const TUint aHost);
         inline TBool GetSniffStatus() const;
         inline void SetSniffStatus(const TBool aSniff);
         inline TBool GetSolicitedStatus() const;
         inline void SetSolicitedStatus(const TBool aSolicited);
         inline TUint8 GetIrlapVersion() const;
         inline void SetIrlapVersion(const TUint8 aIrlapVersion);
         inline TUint8 GetCharacterSet() const;
         inline void SetCharacterSet(const TUint8 aCharacterSet);
         inline TUint8 GetFirstServiceHintByte() const;
         inline void SetFirstServiceHintByte(const TUint8 aFirstServiceHintByte);
         inline TUint8 GetSecondServiceHintByte() const;
         inline void SetSecondServiceHintByte(const TUint8 aSecondServiceHintByte);
         inline TUint8 GetServiceHintByteCount() const;
         inline void SetServiceHintByteCount(const TUint8 aServiceHintByteCount);
         inline TUint8 GetHomePort() const;
         inline void SetHomePort(const TUint8 aHomePort);
         inline TUint8 GetRemotePort() const;
         inline void SetRemotePort(const TUint8 aRemotePort);
      private:
         inline SIrdaAddr *addrPtr() const
         {return (SIrdaAddr *)UserPtr();}
   };
#endif//!__SYMBIAN_3RD__

//----------------------------

   class C_ObexIrProtocolInfo: public TObexProtocolInfo{
   public:
#ifdef __SYMBIAN_3RD__
      TIrdaSockAddr iAddr;
#else
      C_IrdaSockAddr iAddr;
#endif
      TBuf8<KIASClassNameMax> iClassName;
      TBuf8<KIASAttributeNameMax> iAttributeName;
   };

//----------------------------

   void ConnectToServerL(){

      TObexProtocolInfo *pi = NULL;
      switch(transfer){
      case TR_BLUETOOTH:
         {
            TObexBluetoothProtocolInfo *pi_bt = new(true) TObexBluetoothProtocolInfo;
            pi_bt->iTransport.Copy(_L("RFCOMM"));
            pi_bt->iAddr.SetBTAddr(service_searcher->BTDevAddr());
            pi_bt->iAddr.SetPort(service_searcher->GetPort());
            delete service_searcher;
            service_searcher = NULL;
            pi = pi_bt;
         }
         break;
      case TR_INFRARED:
         {
            C_ObexIrProtocolInfo *pi_ir = new(true) C_ObexIrProtocolInfo;
#ifndef __SYMBIAN_3RD__
                              //call constructor of C_IrdaSockAddr, which is inside of irda.prt DLL
#ifdef _DEBUG
            typedef void (__fastcall t_Ctr)(C_IrdaSockAddr*);
#else
            typedef void (t_Ctr)(C_IrdaSockAddr*);
#endif
            t_Ctr *fp = (t_Ctr*)lib_irda.Lookup(7);
            (*fp)(&pi_ir->iAddr);
#endif//!__SYMBIAN_3RD__

            pi_ir->iTransport.Copy(_L("IrTinyTP"));
            //pi_ir->iAddr = 0;
            pi_ir->iClassName = _L8("OBEX");
            pi_ir->iAttributeName = _L8("IrDA:TinyTP:LsapSel");
            pi = pi_ir;
         }
         break;
      default:
         assert(0);
         return;
      }
      delete obex_client;
      obex_client = CObexClient::NewL(*pi);
      delete pi;
      obex_client->Connect(iStatus);
      state = STATE_GETTING_CONNECTION;
      SetActive();
   }

//----------------------------

   void RunL(){
      if(iStatus != KErrNone){
         //Fatal("err", iStatus.Int());
         switch(iStatus.Int()){
         case KErrCancel:
            state = STATE_ERR_CANCELED;
            break;
         case KErrNotSupported:
         case KErrNotFound:
         case KErrGeneral:
         case KErrNoMemory:
         default:
            state = STATE_ERR_FAILED;
            break;
         }
      }else{
         switch(state){
         case STATE_GETTING_DEVICE:
                              //found a device, now search for a suitable service
            //iLog.LogL(service_searcher->ResponseParams().DeviceName());
            state = STATE_GETTING_SERVICE;
                              //this means that the RunL can not be called until this program does something to iStatus
            iStatus = KRequestPending;
            service_searcher->FindServiceL(iStatus);
            SetActive();
            break;

         case STATE_GETTING_SERVICE:
            //iLog.LogL(_L("Found service"));
            ConnectToServerL();
            break;
            
         case STATE_GETTING_CONNECTION:
            //iLog.LogL(_L("Connected"));
            state = STATE_SENDING;
            obex_client->Put(*obex_obj, iStatus);
            SetActive();
            break;
            
         case STATE_SENDING:
            if(curr_file_indx==(dword)filenames.size()-1){
               //iLog.LogL(_L("Sent object"));
                  //iLog.LogL(_L("Disconnecting"));
               state = STATE_DISCONNECTING;
               obex_client->Disconnect(iStatus);
            }else{
               bytes_sent_acc += obex_obj->BytesSent();
               ++curr_file_indx;
               const wchar *fn1 = filenames[curr_file_indx];
               TPtrC fn((word*)fn1, StrLen(fn1));
               obex_obj->InitFromFileL(fn);
               obex_client->Put(*obex_obj, iStatus);
            }
            SetActive();
            break;
            
         case STATE_DISCONNECTING:
            //iLog.LogL(_L("Disconnected"));
            state = STATE_DONE;
            break;
            
         default:
            assert(0);
         };
      }
   }

//----------------------------

   void DoCancel(){}

//----------------------------
public:
   /*
   inline void *operator new(size_t sz, bool){
      void *vp = ::operator new(sz, true);
      MemSet(vp, 0, sz);
      return vp;
   }
   /**/
   C_obex_file_send_imp(const C_vector<Cstr_w> &fns, E_TRANSFER tr):
      CActive(CActive::EPriorityStandard),
      state(STATE_NULL),
      transfer(tr),
      filenames(fns),
      bytes_sent_acc(0),
      curr_file_indx(0),
      service_searcher(NULL),
      obex_client(NULL)
   {
      CActiveScheduler::Add(this);
   }

//----------------------------

   ~C_obex_file_send_imp(){
   
      if(obex_client && obex_client->IsConnected())
         obex_client->Abort();
      iStatus = KErrNone;
      Cancel();
      delete obex_obj;
      delete service_searcher;
      delete obex_client;
#ifndef __SYMBIAN_3RD__
      if(transfer==TR_INFRARED)
         lib_irda.Close();
#endif
   }

//----------------------------

   void ConstructL(){

      obex_obj = CObexFileObject::NewL();
      const wchar *fn1 = filenames[0];
      TPtrC fn((word*)fn1, StrLen(fn1));
      obex_obj->InitFromFileL(fn);

      switch(transfer){
      case TR_BLUETOOTH:
         {
            service_searcher = new(true) C_obex_service_searcher;
            service_searcher->SelectDeviceByDiscoveryL(iStatus);
            state = STATE_GETTING_DEVICE;
            SetActive();
         }
         break;
      case TR_INFRARED:
         {
#ifndef __SYMBIAN_3RD__
            LoadIrdaDll(lib_irda);
#endif
            ConnectToServerL();
         }
         break;
      default:
         assert(0);
      }
   }

//----------------------------

   virtual E_STATUS GetStatus() const{
      switch(state){
      default:
      case STATE_NULL:
         return ST_NULL;
      case STATE_GETTING_DEVICE:
      case STATE_GETTING_SERVICE:
      case STATE_GETTING_CONNECTION:
         return ST_INITIALIZING;
      case STATE_SENDING: return ST_SENDING;
      case STATE_DONE: return ST_DONE;
      case STATE_DISCONNECTING: return ST_CLOSING;
      case STATE_ERR_CANCELED: return ST_CANCELED;
      case STATE_ERR_FAILED: return ST_FAILED;
      }
   }

//----------------------------

   virtual dword GetBytesSent() const{

      if(obex_obj)
         return bytes_sent_acc + obex_obj->BytesSent();
      return bytes_sent_acc;
   }

//----------------------------

   virtual dword GetFileIndex() const{ return curr_file_indx; }
};

//----------------------------

C_client_file_mgr::C_obex_file_send *CreateObexFileSend(const C_vector<Cstr_w> &filenames, C_client_file_mgr::C_obex_file_send::E_TRANSFER tr){

   C_obex_file_send_imp *bts = new(true) C_obex_file_send_imp(filenames, tr);
   TRAPD(err, bts->ConstructL());
   if(err){
      delete bts;
      bts = NULL;
   }
   return bts;
}

//----------------------------
#elif defined WINDOWS_MOBILE
//----------------------------
#include "Main.h"
#include "FileBrowser.h"

bool C_client_file_mgr::C_obex_file_send::CanSendByBt(){ return false; }
bool C_client_file_mgr::C_obex_file_send::CanSendByIr(){ return false; }

//----------------------------

class C_obex_send_sim: public C_client_file_mgr::C_obex_file_send{
public:
   virtual E_STATUS GetStatus() const{
      return ST_INITIALIZING;
      //return ST_DONE;
   }
   virtual dword GetBytesSent() const{ return 0; }
   virtual dword GetFileIndex() const{ return 0; }
};

//----------------------------

C_client_file_mgr::C_obex_file_send *CreateObexFileSend(const C_vector<Cstr_w> &filenames, C_client_file_mgr::C_obex_file_send::E_TRANSFER tr){
   return new(true) C_obex_send_sim;
}

//----------------------------
#else
//----------------------------
#include "Main.h"
#include "FileBrowser.h"

bool C_client_file_mgr::C_obex_file_send::CanSendByBt(){ return true; }
bool C_client_file_mgr::C_obex_file_send::CanSendByIr(){ return true; }

//----------------------------

class C_obex_send_sim: public C_client_file_mgr::C_obex_file_send{
   const C_vector<Cstr_w> filenames;

   mutable dword last_time;
   mutable E_STATUS st;
   mutable dword bytes_sent;
   mutable int file_index;

   bool IsNext(dword pause) const{
      if(GetTickTime() < last_time+pause)
         return false;
      last_time = GetTickTime();
      return true;
   }
   void operator =(const C_obex_send_sim&);
public:
   C_obex_send_sim(const C_vector<Cstr_w> &fns):
      filenames(fns),
      last_time(GetTickTime()),
      st(ST_INITIALIZING),
      bytes_sent(0),
      file_index(0)
   {}
   virtual E_STATUS GetStatus() const{
      switch(st){
      case ST_INITIALIZING:
         if(IsNext(500))
            st = ST_SENDING;
            //st = ST_FAILED;
         break;
      case ST_SENDING:
         if(IsNext(3000)){
            if(file_index==filenames.size()-1)
               st = ST_CLOSING;
            else
               ++file_index;
         }else bytes_sent += 600;
         break;
      case ST_CLOSING: if(IsNext(300)) st = ST_DONE; break;
      }
      return st;
   }
   virtual dword GetBytesSent() const{ return bytes_sent; }
   virtual dword GetFileIndex() const{ return file_index; }
};

//----------------------------

C_client_file_mgr::C_obex_file_send *CreateObexFileSend(const C_vector<Cstr_w> &filenames, C_client_file_mgr::C_obex_file_send::E_TRANSFER tr){
   return new(true) C_obex_send_sim(filenames);
}

#endif

//----------------------------
#include "BtSend.h"

bool C_client_file_send::SetMode(const C_vector<Cstr_w> &_filenames, C_client_file_mgr::C_obex_file_send::E_TRANSFER tr){

   C_vector<Cstr_w> filenames = _filenames;
   C_file ck;
   dword total_size = 0;
   for(int i=filenames.size(); i--; ){
      if(ck.Open(filenames[i]))
         total_size += ck.GetFileSize();
      else
         filenames.erase(&filenames[i]);
   }
   if(!filenames.size())
      return false;
   ck.Close();
   
   C_client_file_mgr::C_obex_file_send *obex_send = CreateObexFileSend(filenames, tr);
   if(!obex_send)
      return false;

   C_mode_this &mod = *new(true) C_mode_this(*this, mode, obex_send);
   CreateTimer(mod, 50);

   mod.is_ir = (tr==C_client_file_mgr::C_obex_file_send::TR_INFRARED);
   mod.filenames = filenames;
   mod.progress.total = total_size;

   ActivateMode(mod);
   return true;
}

//----------------------------

void C_client_file_send::InitLayout(C_mode_this &mod){

   mod.redraw_bgnd = true;
}

//----------------------------

void C_client_file_send::DrawDialogText(const S_rect &rc, const wchar *txt){

   const int ICON_SX = 24;
   S_rect rc_fill = rc;
   rc_fill.y += fdb.line_spacing*2;
   rc_fill.sy = fdb.line_spacing;
   rc_fill.sx -= ICON_SX + 2;
   DrawDialogBase(rc, false, &rc_fill);

   DrawString(txt, rc.x+fdb.letter_size_x, rc.y+fdb.line_spacing*2, UI_FONT_BIG, 0, GetColor(COL_TEXT_POPUP), -(rc_fill.sx-4));
}

//----------------------------

void C_client_file_send::Tick(C_mode_this &mod, dword time, bool &redraw){

   bool redr_st = false;
   if(mod.curr_display_name_index != (int)mod.obex_send->GetFileIndex()){
      mod.curr_display_name_index = mod.obex_send->GetFileIndex();
      RedrawScreen();
      redr_st = true;
   }
   if(mod.last_status != mod.obex_send->GetStatus() || redr_st){
      mod.last_status = mod.obex_send->GetStatus();
      switch(mod.last_status){
      case C_client_file_mgr::C_obex_file_send::ST_DONE:
      case C_client_file_mgr::C_obex_file_send::ST_CANCELED:
      case C_client_file_mgr::C_obex_file_send::ST_FAILED:
         {
            mod.AddRef();
            CloseMode(mod);
            if(mod.last_status==C_client_file_mgr::C_obex_file_send::ST_DONE){
               Cstr_w s;
               if(mod.filenames.size()>1){
                  Cstr_w tmp = text_utils::MakeFileSizeText(mod.progress.total, true, true);
                  s.Format(L"% %\n% %") <<GetText(TXT_NUM_FILES) <<mod.filenames.size() <<GetText(TXT_TOTAL_SIZE) <<tmp;
               }else
                  s = mod.filenames.front();
               CreateInfoMessage(*this, TXT_FILE_SENT, s, NULL);
            }else
            if(mod.last_status==C_client_file_mgr::C_obex_file_send::ST_CANCELED){
            }else{
               ShowErrorWindow(TXT_ERR_CANT_SEND_FILE, mod.filenames[mod.obex_send->GetFileIndex()]);
            }
            mod.Release();
         }
         return;
      case C_client_file_mgr::C_obex_file_send::ST_SENDING:
         DrawDialogText(mod.rc, GetText(TXT_PROGRESS_SENDING));
         break;
      case C_client_file_mgr::C_obex_file_send::ST_CLOSING:
         DrawDialogText(mod.rc, GetText(TXT_DISCONNECTING));
         break;
      }
   }
   int pos = mod.obex_send->GetBytesSent();
   if(mod.progress.pos != pos){
      mod.progress.pos = pos;
      DrawDialogBase(mod.rc, false, &mod.progress.rc);
      DrawProgress(mod.progress);
      Cstr_w s = text_utils::MakeFileSizeText(mod.obex_send->GetBytesSent(), true, true);
      DrawString(s, mod.progress.rc.CenterX(), mod.progress.rc.y, UI_FONT_BIG, FF_CENTER);
   }
}

//----------------------------

void C_client_file_send::ProcessInput(C_mode_this &mod, S_user_input &ui, bool &redraw){

#ifdef USE_MOUSE
   ProcessMouseInSoftButtons(ui, redraw, false);
#endif
   if(ui.key==K_RIGHT_SOFT || ui.key==K_BACK){
      CloseMode(mod);
      return;
   }
}

//----------------------------

void C_client_file_send::Draw(const C_mode_this &mod){

   if(mod.redraw_bgnd){
      mod.redraw_bgnd = false;
      mod.DrawParentMode(true);
   }

   const dword sx = ScrnSX();
   int sz_x = sx - fdb.letter_size_x * 4;
   
   int sy = fdb.line_spacing*3;
   mod.rc = S_rect((sx-sz_x)/2, (ScrnSY()-sy)/2, sz_x, sy);

   mod.progress.rc = S_rect(mod.rc.x+fdb.letter_size_x*2, mod.rc.Bottom()+4, mod.rc.sx-fdb.letter_size_x*4, fdb.letter_size_x*2);
   mod.rc.sy += mod.progress.rc.sy + 8;

   DrawDialogBase(mod.rc, true);

   Cstr_w buf;
   buf<<GetText(TXT_SENDING) <<L' ' <<file_utils::GetFileNameNoPath(mod.filenames[mod.curr_display_name_index]);

   DrawDialogTitle(mod.rc, buf);
   DrawDialogText(mod.rc, GetText(TXT_PROGRESS_INITIALIZING));
   DrawDialogBase(mod.rc, false, &mod.progress.rc);
   DrawProgress(mod.progress);

   DrawSoftButtonsBar(mod, 0, TXT_CANCEL);
   SetScreenDirty();
}

//----------------------------

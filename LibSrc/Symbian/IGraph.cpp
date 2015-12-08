//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <EikEnv.h>
#include <CoeCntrl.h>
#include <EikDoc.h>
#include <EikAppUi.h>
#include <Hal.h>
#include <Cstr.h>
#include <C_vector.h>

//#define USE_LOG //!!!

#ifdef USE_LOG
#include <LogMan.h>
#define LOG(s) log.Add(s)
#define LOG_N(s, n) { Cstr_c t; t<<s <<':' <<n; log.Add(t); }
#else
#define LOG(s)
#define LOG_N(s, n)
#endif

#ifdef S60
#ifdef __SYMBIAN_3RD__
#include <aknsutils.h>
#include <aknsbasicbackgroundcontrolcontext.h>
#include <aknsdrawutils.h>
#include <PtiEngine.h>
//#include <PtiDefs.h>
#include <PtiKeyMappings.h>
#endif
#endif//S60

#include <IGraph.h>
#include <Util.h>
#include <AppBase.h>

#if defined __SYMBIAN_3RD__
#ifndef _DEBUG
#include <RemConCoreApiTarget.h>
#include <RemConCoreApiTargetObserver.h>
#include <RemConInterfaceSelector.h>
#define USE_HW_KEY_CATCH
#endif //!_DEBUG

#ifdef S60
#include <AknViewAppUi.h>
#ifndef __WINS___
#define USE_CDSB  //use Symbian's Direct screen bitmap for DSA
#endif
#endif //S60

#endif

#ifdef USE_CDSB
#include <Cdsb.h>
#endif

#if defined S60 && (defined __SYMBIAN_3RD__ || defined _DEBUG)
#define SHIFT_KEY_WORKAROUND
#endif

#ifdef __SYMBIAN_3RD__
//#define USE_HWRM_LIGHT   //doesn't work, not usable
#endif

#ifdef USE_HWRM_LIGHT
#include <hwrmlight.h>
#endif

//----------------------------

extern
#ifndef __WINS__
   const
#endif//__WINS__
#ifdef __SYMBIAN_3RD__
#ifdef __WINS__
   TEmulatorImageHeader uid;
#else
   dword uid;
#endif
#else//__SYMBIAN_3RD__
   TUid uid[];
#endif//!__SYMBIAN_3RD__

//----------------------------

#include "DeviceUid.h"

//----------------------------
class C_cc: public CCoeControl{
public:
   virtual void AddChild(CCoeControl*) = 0;
   virtual void RemoveChild(CCoeControl*) = 0;
};

//----------------------------

class IGraph_imp: public IGraph, public C_cc{
   C_application_base *app;

   dword key_bits;
   bool backlight_on;

#ifdef USE_HWRM_LIGHT
   CHWRMLight *hwrm_light;
#endif

//----------------------------
   C_vector<CCoeControl*> children;

   virtual void AddChild(CCoeControl *cc){
      children.push_back(cc);
   }
   virtual void RemoveChild(CCoeControl *cc){
      for(int i=children.size(); i--; ){
         if(children[i]==cc){
            children.remove_index(i);
            break;
         }
      }
   }
   virtual TInt CountComponentControls() const{ return children.size(); }
   virtual CCoeControl *ComponentControl(TInt i) const{ assert(i<CountComponentControls()); return children[i]; }

//----------------------------

#ifdef USE_HW_KEY_CATCH
   class C_hw_key_catch: public MRemConCoreApiTargetObserver{
      CRemConInterfaceSelector *selector;
      CRemConCoreApiTarget *target;
      IGraph_imp *igraph;
   public:
      C_hw_key_catch():
         selector(NULL),
         target(NULL),
         igraph(NULL)
      {}
      ~C_hw_key_catch(){
         Close();
      }
      void Construct(IGraph_imp *ig){
         igraph = ig;
         selector = CRemConInterfaceSelector::NewL();
         target = CRemConCoreApiTarget::NewL(*selector, *this);
         selector->OpenTargetL();
      }
      void Close(){
         if(selector){
            delete selector; selector = NULL;
            target = NULL;
         }
      }
      virtual void MrccatoCommand(TRemConCoreApiOperationId op, TRemConCoreApiButtonAction act){

         //Fatal("MM op", op);
         //Info("MM act", act);
         dword bits = igraph->key_bits;
         switch(act){
         case ERemConCoreApiButtonPress:
            //Info("MM key press", op);
            switch(op){
            case ERemConCoreApiVolumeDown: bits |= GKEY_VOLUME_DOWN; break;
            case ERemConCoreApiVolumeUp: bits |= GKEY_VOLUME_UP; break;
            case ERemConCoreApiRewind: bits |= GKEY_LEFT; break;
            case ERemConCoreApiFastForward: bits |= GKEY_RIGHT; break;
            //default: Info("MM key press", op);
            }
            break;
         case ERemConCoreApiButtonClick:
            //Info("MM key click", op);
            switch(op){
            case ERemConCoreApiVolumeDown: igraph->KeyReceived(K_VOLUME_DOWN); break;
            case ERemConCoreApiVolumeUp: igraph->KeyReceived(K_VOLUME_UP); break;
            case ERemConCoreApiStop: igraph->KeyReceived(K_MEDIA_STOP); break;
            case ERemConCoreApiForward: igraph->KeyReceived(K_MEDIA_FORWARD); break;
            case ERemConCoreApiBackward: igraph->KeyReceived(K_MEDIA_BACKWARD); break;
            case ERemConCoreApiF3:     //SE w950
            case ERemConCoreApiMute:   //Nokia headset
            case ERemConCoreApiPause:  //Jabra BT3030
            case ERemConCoreApiPlay:
            case ERemConCoreApiPausePlayFunction:
               igraph->KeyReceived(K_MEDIA_PLAY_PAUSE);
               break;
            }
            break;
         case ERemConCoreApiButtonRelease:
            switch(op){
            case ERemConCoreApiVolumeDown: bits &= ~GKEY_VOLUME_DOWN; break;
            case ERemConCoreApiVolumeUp: bits &= ~GKEY_VOLUME_UP; break;
            case ERemConCoreApiRewind: bits &= ~GKEY_LEFT; break;
            case ERemConCoreApiFastForward: bits &= ~GKEY_RIGHT; break;
            }
            break;
         }
         if(igraph->key_bits!=bits)
            igraph->ProcessInputInternal(K_NOKEY, false, igraph->key_bits = bits, S_point(0, 0), 0);
      }
   } hw_key_catch;
#endif

//----------------------------
                              //methods of CCoeControl:
   TCoeInputCapabilities InputCapabilities() const{
      return
         TCoeInputCapabilities::ENavigation |
         //TCoeInputCapabilities::ENonPredictive |
         //TCoeInputCapabilities::EWesternAlphabetic |
         //TCoeInputCapabilities::EWesternNumericIntegerPositive |
         TCoeInputCapabilities::EAllText |
         0;
   }

//----------------------------

   inline void ProcessInputInternal(IG_KEY key, bool auto_repeat, dword key_bits, const S_point &pen_pos, dword pen_state){
      app->ProcessInputInternal(key, auto_repeat, key_bits, pen_pos, pen_state);
   }

//----------------------------

   void KeyReceived(IG_KEY k, bool auto_repeat = false){
      ProcessInputInternal(k, auto_repeat, key_bits, S_point(0, 0), 0);
   }

//----------------------------

#ifdef SHIFT_KEY_WORKAROUND
                              //workaround for simulation of shift keys by hash keys on S60 devices that don't have shift key
   struct S_shift_key_workaround{
      bool enabled;
      bool any_key_down;
      bool key_down;
      S_shift_key_workaround():
         enabled(true),
         key_down(false),
         any_key_down(true)
      {
#ifdef __SYMBIAN_3RD__
                              //disable this only devices with pen key
         int machine_uid;
         HAL::Get(HALData::EMachineUid, machine_uid);
         switch(machine_uid){
         case UID_NOKIA_3250:
         case UID_NOKIA_5500:
         case UID_NOKIA_E50:
         case UID_NOKIA_E60:
         case UID_NOKIA_E65:
         case UID_NOKIA_N71:
         case UID_NOKIA_N73:
         case UID_NOKIA_N75:
         case UID_NOKIA_N76:
         case UID_NOKIA_N77:
         case UID_NOKIA_N80:
         case UID_NOKIA_N91:
         case UID_NOKIA_N93:
         case UID_NOKIA_N95:
         case UID_NOKIA_N95_8GB:
         //case 0x10005f62:        //emulator
            enabled = false;
            break;
         }
#endif
      }
   } shift_key_workaround;

#endif

//----------------------------
#if defined S60 && defined __SYMBIAN_3RD__
   class C_pti_system{
      CPtiEngine *eng;
      //CPtiKeyMappings *iPtiKeyMappings;
      CPtiQwertyKeyMappings *mappings;
   public:
      C_pti_system(bool has_full_kb)
         :eng(NULL),
         mappings(NULL)
      {
         if(has_full_kb){
                              //disable on some models
            int machine_uid;
            HAL::Get(HALData::EMachineUid, machine_uid);
            switch(machine_uid){
            case UID_NOKIA_E7_00: return;
            }
            eng = CPtiEngine::NewL(ETrue);
            //Fatal("lang", User::Language());
            TLanguage lang = ELangEnglish;   //User::Language();
            //eng->SetInputMode(EPtiEngineQwerty);
            CPtiCoreLanguage *core_lang = static_cast<CPtiCoreLanguage*>(eng->GetLanguage(lang));
            if(core_lang)
               mappings = static_cast<CPtiQwertyKeyMappings*>(core_lang->GetQwertyKeymappings());
            //else Fatal("no mappings");
         }
      }
      ~C_pti_system(){
         delete eng;
      }
      int GetKey(int sc){
         if(mappings && sc>=' ' && sc<0x80){
            TPtiKey key = (TPtiKey)sc;
            TBuf<12> iResult;
            mappings->GetDataForKey(key, iResult, EPtiCaseUpper);
            //Info((const wchar*)iResult.PtrZ(), sc);
            if(iResult.Length())
               sc = iResult[0];
         }
         return sc;
      }
   } pti_system;
#endif

//----------------------------

   virtual TKeyResponse OfferKeyEventL(const TKeyEvent &key_event, TEventCode type){

      switch(type){
      case EEventKeyDown:
         {
            dword bits = key_bits;
            //Fatal("key", key_event.iCode);
            //Fatal("scode", key_event.iScanCode);
            //Info("mod", key_event.iModifiers);
            switch(key_event.iScanCode){
            case EStdKeyLeftArrow: bits |= GKEY_LEFT; break;
            case EStdKeyRightArrow: bits |= GKEY_RIGHT; break;
            case EStdKeyUpArrow: bits |= GKEY_UP; break;
            case EStdKeyDownArrow: bits |= GKEY_DOWN; break;
            case EStdKeyYes: bits |= GKEY_SEND; break;
            //case EStdKeyNo: bits |= GKEY_END; break;
            case EStdKeyLeftShift:
            case EStdKeyRightShift:
               bits |= GKEY_SHIFT;
               break;
            //case EStdKeyBackspace: bits |= GKEY_CLEAR; break;
            //case EStdKeyNkpAsterisk:
            //case '*': bits |= GKEY_ASTERISK; break;
               /*
            case EStdKeyHash:
#ifdef SHIFT_KEY_WORKAROUND
               if(shift_key_workaround.enabled){
                                 //simulate shift key
                  shift_key_workaround.any_key_down = false;
                  shift_key_workaround.key_down = true;
                  bits |= GKEY_SHIFT;
               }else
#endif
               bits |= GKEY_HASH;
               break;
               */
            case EStdKeyEnter: bits |= GKEY_OK; break;
            case EStdKeyNkp0:
            case '0': bits |= GKEY_0; break;
            case EStdKeyNkp1:
            case '1': bits |= GKEY_1; break;
            case EStdKeyNkp2:
            case '2': bits |= GKEY_2; break;
            case EStdKeyNkp3:
            case '3': bits |= GKEY_3; break;
            case EStdKeyNkp4:
            case '4': bits |= GKEY_4; break;
            case EStdKeyNkp5:
            case '5': bits |= GKEY_5; break;
            case EStdKeyNkp6:
            case '6': bits |= GKEY_6; break;
            case EStdKeyNkp7:
            case '7': bits |= GKEY_7; break;
            case EStdKeyNkp8:
            case '8': bits |= GKEY_8; break;
            case EStdKeyNkp9:
            case '9': bits |= GKEY_9; break;
            case EStdKeyIncVolume: bits |= GKEY_VOLUME_UP; break;
            case EStdKeyDecVolume: bits |= GKEY_VOLUME_DOWN; break;

#if defined S60
            case EStdKeyDevice0: bits |= GKEY_SOFT_MENU; break;
            case EStdKeyDevice1: bits |= GKEY_SOFT_SECONDARY; break;
            case EStdKeyDevice3: bits |= GKEY_OK; break;
            case EStdKeyRightCtrl:
            case EStdKeyLeftCtrl: bits |= GKEY_CTRL; break;
#else
#error
#endif
            }
            /*
#if defined __SYMBIAN_3RD__ && defined S60
            if(bits == (GKEY_LEFT|GKEY_UP|GKEY_ASTERISK)){
                           //simulate portrait/landscape rotation by pressing Shift+3+*
               CAknViewAppUi *ap = (CAknViewAppUi*)GetGlobalData()->GetAppUi();
               ap->SetOrientationL(ap->Orientation()==CAknViewAppUi::EAppUiOrientationLandscape ? CAknViewAppUi::EAppUiOrientationPortrait : CAknViewAppUi::EAppUiOrientationLandscape);
            }
#endif
            /**/
            if(key_bits!=bits)
               ProcessInputInternal(K_NOKEY, false, key_bits = bits, S_point(0, 0), 0);
            if(key_event.iScanCode==EStdKeyYes)
               return EKeyWasConsumed;
         }
         break;
         //return EKeyWasConsumed;

      case EEventKey:
         {
            int iScanCode = key_event.iScanCode;
            //Info("key", key_event.iCode); Info("scode", iScanCode);
            //Info("mod", key_event.iModifiers);
#if defined S60 && defined __SYMBIAN_3RD__
            iScanCode = pti_system.GetKey(iScanCode);
            //Info("scode", iScanCode);
#endif
            IG_KEY k = K_NOKEY;
#if defined S60
            if(key_event.iModifiers&(EModifierLeftCtrl|EModifierRightCtrl)){
               if(iScanCode>=' ')
                  k = (IG_KEY)ToLower((char)iScanCode);
               else
               switch(iScanCode){
               case EStdKeyLeftArrow: k = K_CURSORLEFT; break;
               case EStdKeyRightArrow: k = K_CURSORRIGHT; break;
               case EStdKeyUpArrow: k = K_CURSORUP; break;
               case EStdKeyDownArrow: k = K_CURSORDOWN; break;
               case EStdKeyYes: k = K_SEND; break;
               case EStdKeyNo: k = K_HANG; break;
               case EStdKeyBackspace: k = K_DEL; break;
               case EStdKeyEnter: k = K_ENTER; break;
               case EStdKeyDevice0: k = K_LEFT_SOFT; break;
               case EStdKeyDevice3: k = K_RIGHT_SOFT; break;
               }
            }else
#endif
            if((key_event.iModifiers&(EModifierLeftShift|EModifierRightShift|EModifierShift))){
               //Fatal("scode", iScanCode);
               switch(iScanCode){
               case EStdKeyHash:
                  k = IG_KEY('#');
                  break;
               default:
                  if(iScanCode>='0' && iScanCode<='9')
                     k = IG_KEY(iScanCode);
                     //Info("sc", iScanCode);
               }
            }
#ifdef __SYMBIAN_3RD__
            else
            if(machine_uid==UID_NOKIA_E7_00){
                              //E7-00 returns numbers for top row, remap it here
               switch(iScanCode){
               case '0': k = IG_KEY('p'); break; case '1': k = IG_KEY('q'); break; case '2': k = IG_KEY('w'); break; case '3': k = IG_KEY('e'); break; case '4': k = IG_KEY('r'); break;
               case '5': k = IG_KEY('t'); break; case '6': k = IG_KEY('y'); break; case '7': k = IG_KEY('u'); break; case '8': k = IG_KEY('i'); break; case '9': k = IG_KEY('o'); break;
               }
            }
#endif
            //Info("k", k);
            if(!k){
               int c = key_event.iCode;
               if(key_event.iModifiers&(//EModifierLeftShift|EModifierRightShift|EModifierShift|
                  EModifierLeftFunc|EModifierRightFunc|EModifierFunc)){
                              //use c from iCode
               }else{
                              //use scancode for c
                  c = iScanCode;
                  //Info("sc", c);
#if (defined __SYMBIAN_3RD__ && defined S60)
                  switch(machine_uid){
                  case UID_NOKIA_E55:  //double-key keyboard, use code instead
                  //case UID_NOKIA_E61:
                  //case UID_NOKIA_E61i:
                  //case UID_NOKIA_E7_00:
                  case UID_NOKIA_N76:
                     c = key_event.iCode;
                     break;
                  }
#endif
               }
               if(c>=' ' && c<0x60){
                  if(key_event.iModifiers&(EModifierLeftShift|EModifierRightShift|EModifierShift))
                     c = ToUpper(char(c));
                  else
                     c = ToLower(char(c));
               }else
                  c = key_event.iCode;
               //Info("c", c);
               if(c>=' ' && c<127){
#ifdef SHIFT_KEY_WORKAROUND
                  if(c=='#' && !app->HasFullKeyboard() && shift_key_workaround.enabled && !shift_key_workaround.key_down){
                                    //simulate shift key
                     shift_key_workaround.any_key_down = false;
                     shift_key_workaround.key_down = true;
                     key_bits |= GKEY_SHIFT;
                  }
#endif
                  k = IG_KEY(c);
                  //Info("k", k);
               }else
               switch(c){
               case '\r':
               //case '\b':
               case '\t':
               case 27:
                  k = IG_KEY(c);
                  break;
               default:
                  switch(iScanCode){
                  case EStdKeySpace: k = IG_KEY(' '); break;
                  case EStdKeyComma: k = IG_KEY(','); break;
                  case EStdKeyHash: k = IG_KEY('#'); break;
                  case EStdKeyNkpAsterisk: k = IG_KEY('*'); break;
                  case EStdKeyFullStop: k = IG_KEY('.'); break;
                  case EStdKeyLeftArrow: k = K_CURSORLEFT; break;
                  case EStdKeyRightArrow: k = K_CURSORRIGHT; break;
                  case EStdKeyUpArrow: k = K_CURSORUP; break;
                  case EStdKeyDownArrow: k = K_CURSORDOWN; break;
                  case EStdKeyYes: k = K_SEND; break;
                  case EStdKeyNo: k = K_HANG; break;
                  case EStdKeyLeftShift:
                  case EStdKeyRightShift:
                     k = K_SHIFT; break;
                  case EStdKeyBackspace: k = K_DEL; break;
                  case EStdKeyEnter: k = K_ENTER; break;
                  case EStdKeyEscape: k = K_ESC; break;
                  case EStdKeyIncVolume: k = K_VOLUME_UP; break;
                  case EStdKeyDecVolume: k = K_VOLUME_DOWN; break;
                  case EStdKeyDictaphonePlay: k = K_MEDIA_PLAY_PAUSE; break;
                  case EStdKeyDictaphoneStop: k = K_MEDIA_STOP; break;
                  case EStdKeyMenu: k = K_MENU; break;
                  case EStdKeyHome: k = K_HOME; break;
                  case EStdKeyEnd: k = K_END; break;
                  case EStdKeyPageUp: k = K_PAGE_UP; break;
                  case EStdKeyPageDown: k = K_PAGE_DOWN; break;
#if defined S60
                  case EStdKeyDevice0: k = K_LEFT_SOFT; break;
                  case EStdKeyDevice1: k = K_RIGHT_SOFT; break;
                  case EStdKeyDevice3: k = K_ENTER; break;
#else
#error
#endif
                  }
               }
            }
            if(k){
#ifdef SHIFT_KEY_WORKAROUND
               if(shift_key_workaround.key_down){
                              //don't process hash key here
                  if(k=='#')
                     k = IG_KEY(0xff);   //post unused key
                  else
                     shift_key_workaround.any_key_down = true;
               }else
#endif
               {
                              //shift doesn't post keydown, so put it down by reading modifiers
                  if(key_event.iModifiers&(EModifierLeftShift|EModifierRightShift|EModifierShift))
                     key_bits |= GKEY_SHIFT;
                  else
                     key_bits &= ~GKEY_SHIFT;
               }
               KeyReceived(k, (key_event.iRepeats > 0));
            }
         }
         break;
         //return EKeyWasConsumed;

      case EEventKeyUp:
         {
            dword bits = key_bits;
            //Info("key up", key_event.iScanCode);
            switch(key_event.iScanCode){
            case EStdKeyLeftArrow: bits &= ~GKEY_LEFT; break;
            case EStdKeyRightArrow: bits &= ~GKEY_RIGHT; break;
            case EStdKeyUpArrow: bits &= ~GKEY_UP; break;
            case EStdKeyDownArrow: bits &= ~GKEY_DOWN; break;
            case EStdKeyYes: bits &= ~GKEY_SEND; break;
            //case EStdKeyNo: bits &= ~GKEY_END; break;
            case EStdKeyLeftShift:
            case EStdKeyRightShift:
               bits &= ~GKEY_SHIFT; break;
            //case EStdKeyBackspace: bits &= ~GKEY_CLEAR; break;
            //case EStdKeyNkpAsterisk:
            //case '*':
               //bits &= ~GKEY_ASTERISK; break;
            case EStdKeyHash:
#ifdef SHIFT_KEY_WORKAROUND
               if(shift_key_workaround.enabled && !app->HasFullKeyboard()){
                  bits &= ~GKEY_SHIFT;
                  shift_key_workaround.key_down = false;
                  key_bits = bits;
                              //post hash key now
                  if(!shift_key_workaround.any_key_down)
                     KeyReceived(IG_KEY('#'));
               }//else
#endif
                  //bits &= ~GKEY_HASH;
               break;
            case EStdKeyEnter: bits &= ~GKEY_OK; break;
            case EStdKeyNkp0:
            case '0': bits &= ~GKEY_0; break;
            case EStdKeyNkp1:
            case '1': bits &= ~GKEY_1; break;
            case EStdKeyNkp2:
            case '2': bits &= ~GKEY_2; break;
            case EStdKeyNkp3:
            case '3': bits &= ~GKEY_3; break;
            case EStdKeyNkp4:
            case '4': bits &= ~GKEY_4; break;
            case EStdKeyNkp5:
            case '5': bits &= ~GKEY_5; break;
            case EStdKeyNkp6:
            case '6': bits &= ~GKEY_6; break;
            case EStdKeyNkp7:
            case '7': bits &= ~GKEY_7; break;
            case EStdKeyNkp8:
            case '8': bits &= ~GKEY_8; break;
            case EStdKeyNkp9:
            case '9': bits &= ~GKEY_9; break;
            case EStdKeyIncVolume: bits &= ~GKEY_VOLUME_UP; break;
            case EStdKeyDecVolume: bits &= ~GKEY_VOLUME_DOWN; break;

#if defined S60
            case EStdKeyDevice0: bits &= ~GKEY_SOFT_MENU; break;
            case EStdKeyDevice1: bits &= ~GKEY_SOFT_SECONDARY; break;
            case EStdKeyDevice3: bits &= ~GKEY_OK; break;
            case EStdKeyRightCtrl:
            case EStdKeyLeftCtrl: bits &= ~GKEY_CTRL; break;
#else
#error
#endif
            }
            if(key_bits!=bits)
               ProcessInputInternal(K_NOKEY, false, key_bits = bits, S_point(0, 0), 0);
         }
         break;
         //return EKeyWasConsumed;
      }
      return CCoeControl::OfferKeyEventL(key_event, type);
      //return EKeyWasNotConsumed;
   }

//----------------------------

#if (defined __SYMBIAN_3RD__ && defined S60)
   virtual void HandlePointerEventL(const TPointerEvent &pe){

      CCoeControl::HandlePointerEventL(pe);
      dword cmd = 0;
      switch(pe.iType){
      case TPointerEvent::EButton1Down:
         cmd = MOUSE_BUTTON_1_DOWN;
         break;
      case TPointerEvent::EButton1Up:
         cmd = MOUSE_BUTTON_1_UP;
         break;
      case TPointerEvent::EDrag:
         cmd = MOUSE_BUTTON_1_DRAG;
         break;
      }
      if(cmd)
         ProcessInputInternal(K_NOKEY, 0, key_bits, S_point(pe.iPosition.iX, pe.iPosition.iY), cmd);
   }
#endif   //mouse control

//----------------------------
                              //window server handling
   CWsScreenDevice *screen_device;

#ifdef __SYMBIAN_3RD__
   void *GetBbufMem() const{
      backbuffer->LockHeap();
      void *bbuf_mem = backbuffer->DataAddress();
      backbuffer->UnlockHeap();
      return bbuf_mem;
   }
#endif

   int machine_uid;

   bool enable_dsa;

   inline const S_pixelformat &GetPixelFormat() const{ return app->GetPixelFormat(); }
   inline int ScrnSX() const{ return app->_scrn_sx; }
   inline int ScrnSY() const{ return app->_scrn_sy; }

//----------------------------
                              //direct screen access support
   class C_dsa_base: public MDirectScreenAccess{
   protected:
      CDirectScreenAccess *dsa;
      bool active;

      C_dsa_base():
         dsa(NULL),
         active(false),
         dsa_capable(false)
      {
#ifdef USE_LOG
         log.Open(L"E:\\dsa.txt");
#endif
      }

      void StartDsa(){
         LOG("StartDsa");
         class C_lf: public C_leave_func{
            virtual int Run(){
               dsa->StartL();
               return 0;
            }
         public:
            CDirectScreenAccess *dsa;
         } lf;
         lf.dsa = dsa;
         int err = lf.Execute();
         if(err)
            Fatal("Dsa start", err);
         dsa->Gc()->SetClippingRegion(dsa->DrawingRegion());
      }
   public:
#ifdef USE_LOG
      C_logman log;
#endif
      bool dsa_capable;          //true if dsa is possible

      inline bool IsActivated() const{ return active; }

      inline CDirectScreenAccess *GetDsa() const{ return dsa; }
   };

//----------------------------

#ifndef USE_CDSB
#ifdef __SYMBIAN_3RD___
#define DSA_HAS_BACKBUFFER

                              //problem: blinking on some devices, because bbuffer is rewritten as it's being displayed
   class C_direct_screen_access: public C_dsa_base{
      TAcceleratedBitmapInfo abi;
      IGraph_imp *igraph;

   //----------------------------

      virtual void Restart(RDirectScreenAccess::TTerminationReasons aReason){
         LOG("Restart");
         StartDsa();
         active = true;
      }
      virtual void AbortNow(RDirectScreenAccess::TTerminationReasons aReason){

         LOG("AbortNow");
                              //copy screen buffer back to backbuffer
         if(active && igraph->backbuffer)
         {
            const byte *src = (byte*)GetBackBuffer();
            byte *dst = (byte*)igraph->GetBbufMem();
            const S_pixelformat &pf = igraph->GetPixelFormat();
            dword bb_pitch = igraph->ScrnSX()*pf.bytes_per_pixel;
            for(int y=igraph->ScrnSY(); y--; ){
               MemCpy(dst, src, igraph->ScrnSX()*pf.bytes_per_pixel);
               src += GetPitch();
               dst += bb_pitch;
            }
         }
         active = false;
         if(dsa)
            dsa->Cancel();
      }
   public:
      CFbsScreenDevice *dev;

      C_direct_screen_access(IGraph_imp *ig):
         igraph(ig),
         dev(NULL)
      {
         int machine_uid;
         HAL::Get(HALData::EMachineUid, machine_uid);
         switch(machine_uid){
         case UID_SAMSUNG_I590:
            return;
         }

         //dev = CFbsScreenDevice::NewL(_L("scdv"), CCoeEnv::Static()->ScreenDevice()->DisplayMode());
         dev = CFbsScreenDevice::NewL(0, CCoeEnv::Static()->ScreenDevice()->DisplayMode());
         //TRect rc; dev->GetDrawRect(rc); Fatal("y", rc.iBr.iY);
         RHardwareBitmap hwb = dev->HardwareBitmap();
         if(!hwb.iHandle)
            return;
         hwb.GetInfo(abi);
         //Info("!p", dword(abi.iLinePitch));
         if(!abi.iAddress){
            delete dev;
            dev = NULL;
            return;
         }
                              //fix phone bugs
         switch(machine_uid){
         case UID_SAMSUNG_I8510:
         case UID_SAMSUNG_I7110:
            abi.iLinePitch *= 2;
            break;
         }
         dsa_capable = true;
      }
      ~C_direct_screen_access(){
         Stop();
         delete dev;
      }

   //----------------------------
   // Start drawing.
      void Start(const TRect &_rc, RWindow *win){
         LOG("Start");
         if(!dsa)
            dsa = CDirectScreenAccess::NewL(CEikonEnv::Static()->WsSession(), *CEikonEnv::Static()->ScreenDevice(), *win, *this);
         StartDsa();
         active = true;
         //dev->SetAutoUpdate(true);
      }
   //----------------------------
   // Stop drawing.
      void Stop(){
         AbortNow(RDirectScreenAccess::ETerminateCancel);
         delete dsa;
         dsa = NULL;
      }

   //----------------------------

      byte *BeginDraw(dword &pitch){
         //MemSet(GetBackBuffer(), 0, 32);
         dev->Update(*dsa->DrawingRegion());
         //dev->Update();
         return NULL;
      }

      void EndDraw(){
      }

      inline byte *GetBackBuffer() const{ return abi.iAddress; }
      inline dword GetPitch() const{ return abi.iLinePitch; }
   };
#else
   class C_direct_screen_access: public C_dsa_base{
#if !defined __SYMBIAN_3RD__
                              //raw event for forcing the update of screen
      TRawEvent redraw;
#endif
      int machine_uid;
      int pixel_offset, dst_pitch;

   //----------------------------

      virtual void Restart(RDirectScreenAccess::TTerminationReasons aReason){
         StartDsa();
         active = true;
      }
      virtual void AbortNow(RDirectScreenAccess::TTerminationReasons aReason){
         active = false;
         if(dsa)
            dsa->Cancel();
      }
   public:

      C_direct_screen_access(IGraph_imp *ig):
         pixel_offset(0)
      {
         HAL::Get(HALData::EMachineUid, machine_uid);
#if defined __SYMBIAN_3RD__
         dsa_capable = true;
#else
         switch(machine_uid){
#ifdef S60
         case UID_NOKIA_7650:
         case UID_NOKIA_3650:
         case UID_NOKIA_6600:
         case UID_NOKIA_6620:
         case UID_NOKIA_3230:
         case UID_NOKIA_6630:
         case UID_NOKIA_6680:
         case UID_NOKIA_6681:
         case UID_NOKIA_NGAGE:
         case UID_NOKIA_N70:
         case UID_SENDO_X:
         case UID_NOKIA_N90:
#ifdef _DEBUG
         //case 0x10005f62:        //emulator
#endif
#else
#error
#endif
            dsa_capable = true;
            break;
         }
         redraw.Set(TRawEvent::ERedraw);
#endif
         dst_pitch = 0;
         HAL::Get(HAL::EDisplayOffsetBetweenLines, dst_pitch);
         dst_pitch *= 2;
         HAL::Get(HAL::EDisplayOffsetToFirstPixel, pixel_offset);
      }
      ~C_direct_screen_access(){
         Stop();
      }

   //----------------------------
   // Start drawing.
      void Start(const TRect &_rc, RWindow *win){
         if(!dsa)
            dsa = CDirectScreenAccess::NewL(CEikonEnv::Static()->WsSession(), *CEikonEnv::Static()->ScreenDevice(), *win, *this);
         Restart(RDirectScreenAccess::ETerminateCancel);
      }
   //----------------------------
   // Stop drawing.
      void Stop(){
         //AbortNow(RDirectScreenAccess::ETerminateCancel);
         delete dsa;
         dsa = NULL;
      }

   //----------------------------

      byte *BeginDraw(dword &pitch){
         TScreenInfoV01 screenInfo;
         TPckg<TScreenInfoV01> sInfo(screenInfo);
         UserSvr::ScreenInfo(sInfo);
         byte *screen_addr = screenInfo.iScreenAddressValid ? (byte*)screenInfo.iScreenAddress : NULL;
         if(screen_addr){
#ifndef __SYMBIAN_3RD__
            switch(machine_uid){
            case UID_SONYERICSSON_P800:
            case UID_SONYERICSSON_P900:
            case UID_SENDO_X:
               break;
            default:
                                 //skip the palette data in the beginning of frame buffer (16 entries)
               screen_addr += 16 * sizeof(word);
            }
#else
            screen_addr += pixel_offset;
#endif
            pitch = dst_pitch;
         }
         return screen_addr;
      }

      void EndDraw(){
#if !defined __SYMBIAN_3RD__
         UserSvr::AddEvent(redraw);
#else
         dsa->ScreenDevice()->Update(*dsa->DrawingRegion());
#endif
      }
   };
#endif

#else//USE_CDSB
   class C_direct_screen_access: public CActive, public C_dsa_base{
      CDirectScreenBitmap *dsb;
      //TRect rc;

   //----------------------------
      void InitDsb(){
         LOG("InitDsb");
         if(!active)
            StartDsa();
         if(!dsb){
            LOG("New CDirectScreenBitmap");
            dsb = CDirectScreenBitmap::NewL();
         }
         LOG("dsb Create");
         TPixelsAndRotation por;
         CCoeEnv::Static()->ScreenDevice()->
#ifdef __SYMBIAN_3RD__
            GetScreenModeSizeAndRotation(CCoeEnv::Static()->ScreenDevice()->CurrentScreenMode(), por);
#else
            GetDefaultScreenSizeAndRotation(por);
#endif
         TRect rc(0, 0, por.iPixelSize.iWidth, por.iPixelSize.iHeight);
         //LOG_N("rc.x", rc.iTl.iX); LOG_N("rc.y", rc.iTl.iY); LOG_N("rc.sx", rc.Width()); LOG_N("rc.sy", rc.Height());
         int err = dsb->Create(rc, CDirectScreenBitmap::EDoubleBuffer);
         if(err){
                              //this happens (before screen is rotated), just close and wait for Stop/Start
            LOG_N("Create dsb err", err);
            CloseDsb();
         }else
            active = true;
      }
   //----------------------------
      void CloseDsb(){
         active = false;
         Cancel();
         if(dsb){
            dsb->Close();
            dsb = NULL;
         }
      }
   //----------------------------
      virtual void Restart(RDirectScreenAccess::TTerminationReasons aReason){
         LOG_N("Restart", aReason);
                              //restart request, but don't restart now, because if screen is rotated, it hangs phone; using timer helps
         //CTimer::After(1000*2000);
      }
   //----------------------------
      virtual void AbortNow(RDirectScreenAccess::TTerminationReasons aReason){
         LOG_N("AbortNow", aReason);
         CloseDsb();
      }
   public:
   //----------------------------
   // Start drawing.
      void Start(const TRect &_rc, RWindow *win){
         LOG("Start");
         //rc = _rc;
         if(!dsa)
            dsa = CDirectScreenAccess::NewL(CEikonEnv::Static()->WsSession(), *CEikonEnv::Static()->ScreenDevice(), *win, *this);
         InitDsb();
         LOG("Started");
      }
   //----------------------------
   // Stop drawing.
      void Stop(){
         LOG("Stop");
         CloseDsb();
         if(dsa) dsa->Cancel();
         delete dsb; dsb = NULL;
         delete dsa; dsa = NULL;
         LOG("Stopped");
      }

   //----------------------------

      C_direct_screen_access(IGraph_imp *ig):
         CActive(CActive::EPriorityHigh),
         //CTimer(CActive::EPriorityHigh),
         dsb(NULL)
      {
         CActiveScheduler::Add(this);
         dsa_capable = true;

         int machine_uid;
         HAL::Get(HALData::EMachineUid, machine_uid);
         switch(machine_uid){
         case UID_NOKIA_N82:
         case UID_NOKIA_N91:     //unknown bug in N91 firmware, can't use DSA here
         case UID_NOKIA_E90:
         case UID_SAMSUNG_I8510:
         case UID_SAMSUNG_I8910:
            dsa_capable = false;
            break;
         }
         LOG("-- Init --");
      }
      ~C_direct_screen_access(){
         LOG("~");
         Stop();
         Deque();
         //delete dsb;
         LOG("-- Closed --");
      }
   //----------------------------
      virtual void RunL(){
         /*
#ifdef USE_LOG
         if(iStatus.Int())
            LOG_N("RunL err", iStatus.Int());
#endif
         */
      }

   //----------------------------
      virtual void DoCancel(){}
   //----------------------------

      byte *BeginDraw(dword &dst_pitch){
         TAcceleratedBitmapInfo bi;
         int err = dsb->BeginUpdate(bi);
         if(err){
            //Fatal("BeginUpdate", err);
            //LOG_N("BeginUpdate err", err);
            return NULL;
         }
         LOG("BeginUpdate");
         dst_pitch = bi.iLinePitch;
         return bi.iAddress;
      }

      void EndDraw(){
         if(CActive::IsActive())
            CActive::Cancel();
         LOG("EU");
         CActive::SetActive();
         dsb->EndUpdate(iStatus);
         LOG("OK");
      }
   };
#endif
   mutable C_direct_screen_access dsa;
                              //back-buffer bitmap devide (for GDI drawing)
   CFbsBitmapDevice *bbuf_dev;
   mutable CFbsBitGc *bbuf_gc;
   CFbsBitmap *backbuffer;

   TPoint scrn_pt;
   bool is_drawing;


   dword full_scrn_sx, full_scrn_sy;

   friend IGraph *IGraphCreate(C_application_base *app, dword flags);

//----------------------------

   static TDisplayMode GetDisplayMode(dword bits_per_pixel){

      switch(bits_per_pixel){
      case 12: return EColor4K;
      case 16: return EColor64K;
      case 24: return EColor16M;
      case 32: return (TDisplayMode)11;   //EColor16MU
      }
      return ENone;
   }

//----------------------------

#ifdef CAPTURE_KEYS
   int h_cap_keys[4];

   void CaptureKeys(){
      RWindowGroup &wg = CEikonEnv::Static()->RootWin();
      h_cap_keys[0] = wg.CaptureKey(EKeyDevice3, 0, 0);
      h_cap_keys[1] = wg.CaptureKeyUpAndDowns(EStdKeyDevice3, 0, 0);
      h_cap_keys[2] = wg.CaptureKey(EKeyDevice9, 0, 0);
      h_cap_keys[3] = wg.CaptureKeyUpAndDowns(EStdKeyDevice9, 0, 0);
   }

//----------------------------

   void CancelCaptureKeys(){
      RWindowGroup &wg = CEikonEnv::Static()->RootWin();
      if(h_cap_keys[0]>=0){ wg.          CancelCaptureKey(h_cap_keys[0]); h_cap_keys[0] = -1; }
      if(h_cap_keys[1]>=0){ wg.CancelCaptureKeyUpAndDowns(h_cap_keys[1]); h_cap_keys[1] = -1; }
      if(h_cap_keys[2]>=0){ wg.          CancelCaptureKey(h_cap_keys[2]); h_cap_keys[2] = -1; }
      if(h_cap_keys[3]>=0){ wg.CancelCaptureKeyUpAndDowns(h_cap_keys[3]); h_cap_keys[3] = -1; }
   }

#endif

//----------------------------

   IGraph_imp(C_application_base *_app, dword flg):
#if defined S60 && defined __SYMBIAN_3RD__
      pti_system(_app->HasFullKeyboard()),
#endif
      app(_app),
      screen_device(NULL),
      key_bits(0),
      backlight_on(false),
      enable_dsa(false),
      machine_uid(0),
      bbuf_dev(NULL),
      bbuf_gc(NULL),
      backbuffer(NULL),
      dsa(this),
#ifdef USE_HWRM_LIGHT
      hwrm_light(NULL),
#endif
      is_drawing(false)
   {
      app->ig_flags = flg;
#ifdef CAPTURE_KEYS
      MemSet(h_cap_keys, 0xff, sizeof(h_cap_keys));
#endif
      HAL::Get(HALData::EMachineUid, machine_uid);

      SetBackLight((app->ig_flags&IG_ENABLE_BACKLIGHT));
   }

//----------------------------

   void UpdateDsaFlag(){
      enable_dsa = dsa.dsa_capable && (app->ig_flags&IG_SCREEN_USE_DSA);
      //*
#ifdef __SYMBIAN_3RD__
      if(enable_dsa){
         int machine_uid;
         HAL::Get(HALData::EMachineUid, machine_uid);
                              //some phones only in non-rotated mode
         switch(machine_uid){
         case UID_SAMSUNG_I8910:
            if(_GetRotation())
               enable_dsa = false;
            break;
         }
      }
#endif
      /**/
   }

//----------------------------

   void SetupScreenSize(){

      if(app->ig_flags&IG_SCREEN_USE_CLIENT_RECT){
         const S_global_data *gd = GetGlobalData();
         TRect rcc = gd->GetAppUi()->ClientRect();
#ifdef S60
                              //always fill screen fully at right and bottom
         rcc.iBr.iX = full_scrn_sx;
         rcc.iBr.iY = full_scrn_sy;
#endif
         app->_scrn_sx = rcc.iBr.iX-rcc.iTl.iX;
         app->_scrn_sy = rcc.iBr.iY-rcc.iTl.iY;
         scrn_pt.iX = rcc.iTl.iX;
         scrn_pt.iY = rcc.iTl.iY;
      }else{
         app->_scrn_sx = full_scrn_sx;
         app->_scrn_sy = full_scrn_sy;
         scrn_pt.iX = 0;
         scrn_pt.iY = 0;
      }
   }

//----------------------------
#ifdef _DEBUG_
   RWindowGroup wg;
#endif

   void Construct(){

      screen_device = CCoeEnv::Static()->ScreenDevice();

      byte bpp = (byte)TDisplayModeUtils::NumDisplayModeBitsPerPixel(screen_device->DisplayMode());
      if(bpp>16 && !(app->ig_flags&IG_SCREEN_ALLOW_32BIT)){
         bpp = 16;
         dsa.dsa_capable = false;
      }
      const_cast<S_pixelformat&>(app->GetPixelFormat()).Init(bpp);
      UpdateDsaFlag();

#ifdef _DEBUG_
      //RWindowGroup &wg = CEikonEnv::Static()->RootWin();
      wg = RWindowGroup(iCoeEnv->WsSession());
      wg.Construct((TUint32)&wg, true);
      CCoeControl::CreateWindowL(wg);

      TApaTaskList tl(iCoeEnv->WsSession());
      TApaTask tsk = tl.FindApp(TUid::Uid(GetSymbianAppUid()));
      tsk.SetWgId((TUint32)&wg);
#else
      CCoeControl::CreateWindowL();
#endif

      full_scrn_sx = screen_device->SizeInPixels().iWidth;
      full_scrn_sy = screen_device->SizeInPixels().iHeight;
      SetupScreenSize();
      InitBackBuffer();

      EnableDragEvents();

#ifdef USE_HW_KEY_CATCH
      if(app->ig_flags&IG_USE_MEDIA_KEYS)
         hw_key_catch.Construct(this);
#endif
      CCoeControl::ActivateL();
      CCoeControl::SetRect(TRect(scrn_pt.iX, scrn_pt.iY, scrn_pt.iX+app->_scrn_sx, scrn_pt.iY+app->_scrn_sy));
   }

//----------------------------

   ~IGraph_imp(){

      SetBackLight(false);
#ifdef USE_HW_KEY_CATCH
      hw_key_catch.Close();
#endif
      StopDrawing();
      CloseBackBuffer();
   }

//----------------------------

   void InitBackBuffer(){

      UpdateDsaFlag();
      S_rect rc(0, 0, app->_scrn_sx, app->_scrn_sy);
      TSize ssize(app->_scrn_sx, app->_scrn_sy);

      screen_pitch = app->_scrn_sx*GetPixelFormat().bytes_per_pixel;

                              //create the offscreen bitmap
      backbuffer = new(true) CFbsBitmap;

#if 0
                              //try to create hw backbuffer
      int err = 1;
      if(app->ig_flags&IG_SCREEN_USE_DSA){
                           //try to create hardware bitmap first
         TUid id =
#ifdef __SYMBIAN_3RD__
#ifdef __WINS__
            uid.iUids[2];
#else
            { uid };
#endif
#else
            uid[2];
#endif
         err = backbuffer->CreateHardwareBitmap(ssize, GetDisplayMode(pixel_format.bits_per_pixel), id);
         //Fatal("hwb", err);
      }
      if(err)
#endif
         backbuffer->Create(ssize, GetDisplayMode(GetPixelFormat().bits_per_pixel));

#ifdef __SYMBIAN_3RD__
      GetBbufMem();
#endif
      bbuf_dev = CFbsBitmapDevice::NewL(backbuffer);
      bbuf_dev->CreateContext(bbuf_gc);
   }

//----------------------------

   void CloseBackBuffer(){

      if(bbuf_gc){
         bbuf_gc->DiscardFont();
         delete bbuf_gc;
         bbuf_gc = NULL;
      }
      delete bbuf_dev;
      bbuf_dev = NULL;
      delete backbuffer;
      backbuffer = NULL;
   }

//----------------------------

   virtual void *_GetBackBuffer() const{
#ifdef DSA_HAS_BACKBUFFER
      if(enable_dsa && dsa.IsActivated())
         return dsa.GetBackBuffer();
#endif

#ifdef __SYMBIAN_3RD__
      return GetBbufMem();
#else
      return backbuffer->DataAddress();
#endif
   }

//----------------------------

   virtual const CFbsBitmap &GetBackBufferBitmap() const{ return *backbuffer; }
   virtual const class CFbsBitmapDevice &GetBackBufferDevice() const{ return *bbuf_dev; }
   virtual class CFbsBitGc &GetBitmapGc() const{ return *bbuf_gc; }
   virtual class CDirectScreenAccess *GetDsa() const{ return dsa.GetDsa(); }

//----------------------------

   virtual void Draw(const TRect &rc) const{

      if(enable_dsa){
         //SystemGc().SetBrushColor(TRgb(0xff)); SystemGc().Clear(rc);
         if(dsa.IsActivated()
#ifdef USE_CDSB
            && dsa_performance_counter>=DSA_PERFORMANCE_COUNT
#endif
            )
            return;
      }
#ifdef USE_LOG
      dsa.LOG("Draw");
#endif
      CWindowGc &gc = SystemGc();
      gc.BitBlt(rc.iTl, backbuffer, rc);
   }

//----------------------------
// Standard blitting by system.
   void UpdateScreenStandard(const S_rect *rects, dword num_rects) const{
#if defined __SYMBIAN_3RD__
                           //we can use only DrawNow method, BitBlt causes slowdowns
      if(num_rects){
         while(num_rects--){
            TRect rc = *(TRect*)rects++;
            rc.iBr += rc.iTl;
            DrawNow(rc);
         }
      }else
         DrawNow();
#else
#if 1
      DrawNow();
#else
      CWindowGc &gc = SystemGc();
      gc.Activate(Window());
      if(num_rects){
         while(num_rects--){
            TRect rc = *(TRect*)rects++;
            rc.iBr += rc.iTl;
            gc.BitBlt(rc.iTl, backbuffer, rc);
         }
      }else{
         TRect rc(0, 0, app->_scrn_sx, app->_scrn_sy);
         gc.BitBlt(rc.iTl, backbuffer, rc);
      }
      gc.Deactivate();
#endif
#endif
                              //update changes
      CEikonEnv::Static()->WsSession().Flush();
   }

//----------------------------
#if 0
   void DebugRect(dword color, int n) const{
      ((IGraph_imp*)this)->FillRect(S_rect(_scrn_sx-20, 0, 20, 20), color); //!!!
      Cstr_w s;
      s<<n;
      ((IGraph_imp*)this)->_DrawText(s, _scrn_sx-19, 19, 0, 11);
   }
#else
#define DebugRect(a, b)
#endif

//----------------------------

   bool UpdateScreenDsa() const{
                              //get screen address (may fail)
      dword dst_pitch;
      byte *screen_addr = dsa.BeginDraw(dst_pitch);
      if(!screen_addr)
         return false;

      //DebugRect(0xff00ff00, dsa_performance_counter); //!!!
                        //copy bitmap contents to frame buffer
#ifdef __SYMBIAN_3RD__
      backbuffer->LockHeap();
#endif
      const byte *bbuf = (byte*)backbuffer->DataAddress();
      const dword src_pitch = app->_scrn_sx*GetPixelFormat().bytes_per_pixel;
      if(dst_pitch && dst_pitch!=src_pitch){
         for(int y=app->_scrn_sy; y--; ){
            MemCpy(screen_addr, bbuf, src_pitch);
            screen_addr += dst_pitch;
            bbuf += src_pitch;
         }
      }else
         MemCpy(screen_addr, bbuf, app->_scrn_sx * app->_scrn_sy * GetPixelFormat().bytes_per_pixel);

#ifdef __SYMBIAN_3RD__
      backbuffer->UnlockHeap();
#endif
      dsa.EndDraw();
      return true;
   }

//----------------------------

   TRect GetViewport() const{
      const S_rect &clip_rect = app->GetClipRect();
      return TRect(clip_rect.x, clip_rect.y, clip_rect.Right(), clip_rect.Bottom());
   }

//----------------------------
#ifndef DSA_HAS_BACKBUFFER
   mutable int dsa_performance_counter;
   mutable dword dsa_times[2];        //[0]=no dsa, [1]=dsa
   static const int DSA_PERFORMANCE_COUNT = 30;

   void ResetDsaCounter(){
      dsa_performance_counter = 0;
      dsa_times[0] = dsa_times[1] = 0;
#if defined _DEBUG && 1
      dsa_performance_counter = DSA_PERFORMANCE_COUNT;
#endif
   }
#endif//!DSA_HAS_BACKBUFFER

//----------------------------

   virtual void _UpdateScreen(const S_rect *rects, dword num_rects) const{

      if(!enable_dsa){
         UpdateScreenStandard(rects, num_rects);
      }else{
#ifndef DSA_HAS_BACKBUFFER
         if(dsa_performance_counter<DSA_PERFORMANCE_COUNT){
                           //measure DSA/nonDSA speed
            dword tm = GetTickTime();
            if(dsa_performance_counter<DSA_PERFORMANCE_COUNT/2){
                           //non-DSA
               DebugRect(0xffffffff, dsa_performance_counter); //!!!
               UpdateScreenStandard(rects, num_rects);
               dsa_times[0] += GetTickTime() - tm;
               if(++dsa_performance_counter==DSA_PERFORMANCE_COUNT/2)
                  dsa.Start(GetViewport(), &CCoeControl::Window());
            }else{
               UpdateScreenDsa();
               dsa_times[1] += GetTickTime() - tm;
               ++dsa_performance_counter;
            }
            if(dsa_performance_counter==DSA_PERFORMANCE_COUNT){
               if(dsa_times[0]<dsa_times[1])
                  dsa.Stop();
               //Info("c0", dsa_times[0]); Info("c1", dsa_times[1]);
            }
         }else
#endif//!DSA_HAS_BACKBUFFER
         {
            if(!dsa.IsActivated()){
               DebugRect(0xff0000ff, 9999); //!!!
                                 //if DSA is suspended by system, use standard blit
               UpdateScreenStandard(rects, num_rects);
            }else{
               UpdateScreenDsa();
            }
         }
      }
      if(backlight_on && is_drawing)
         MakeBacklightOn();

#ifndef __SYMBIAN_3RD__
                              //legacy feature, keep for old Symbian
      CEikonEnv::Static()->WsSession().SetKeyboardRepeatRate(300000, 100000);
#endif
   }

//----------------------------
#ifdef DSA_HAS_BACKBUFFER

   void ActivateBbufGc(CFbsDevice *dev){
      bbuf_gc->Activate(dev);
      TPixelsAndRotation por;
      screen_device->GetScreenModeSizeAndRotation(screen_device->CurrentScreenMode(), por);
      bbuf_gc->SetOrientation(por.iRotation);
   }

#endif
//----------------------------

   virtual void StartDrawing(){

#ifdef USE_LOG
      dsa.LOG("StartDrawing");
#endif
      if(is_drawing)
         return;
                              //clear key-down bits, for case we miss keyup event while not focused
      key_bits = 0;

      if(enable_dsa){
         //CEikonEnv::Static()->WsSession().Flush();
         //dsa.Start(GetViewport(), &CCoeControl::Window());
#ifndef DSA_HAS_BACKBUFFER
         ResetDsaCounter();
#endif
         dsa.Start(GetViewport(), &CCoeControl::Window());
#ifdef DSA_HAS_BACKBUFFER
         screen_pitch = dsa.GetPitch();
         ActivateBbufGc(dsa.dev);
#endif
      }

      is_drawing = true;
#ifdef CAPTURE_KEYS
      CaptureKeys();
#endif
   }

//----------------------------

   virtual void StopDrawing(){

#ifdef USE_LOG
      dsa.LOG("StopDrawing");
#endif
      if(!is_drawing)
         return;

      is_drawing = false;

      bool dsa_act = dsa.IsActivated();
      dsa.Stop();
      if(dsa_act){
#ifdef DSA_HAS_BACKBUFFER
         screen_pitch = app->_scrn_sx*GetPixelFormat().bytes_per_pixel;
         ActivateBbufGc(bbuf_dev);
#endif
#ifdef S60
                              //update fresh screen contents, because dsb doesn't update Window server screen
         //DrawDeferred();
         //_UpdateScreen(NULL, 0);
#endif
      }
#ifdef CAPTURE_KEYS
      CancelCaptureKeys();
#endif
   }

//----------------------------

   void ReInitScreen(){

      bool dr = is_drawing;
      if(dr)
         StopDrawing();
      const int sx = app->_scrn_sx, sy = app->_scrn_sy;

      CloseBackBuffer();
      SetupScreenSize();
      CCoeControl::SetRect(TRect(scrn_pt.iX, scrn_pt.iY, scrn_pt.iX+app->_scrn_sx, scrn_pt.iY+app->_scrn_sy));
      InitBackBuffer();

      if(dr)
         StartDrawing();

      app->ScreenReinit((sx!=app->_scrn_sx || sy!=app->_scrn_sy));
   }

//----------------------------
#if defined S60 && defined __SYMBIAN_3RD__

   virtual void HandleResourceChange(TInt aType){
      switch(aType){
      case KAknsMessageSkinChange:
         ReInitScreen();
         break;
      }
   }

#endif
//----------------------------

   virtual void ScreenChanged(){
#ifdef __SYMBIAN_3RD__
      TPixelsTwipsAndRotation sz;
      screen_device->GetScreenModeSizeAndRotation(screen_device->CurrentScreenMode(), sz);
      if(full_scrn_sx != sz.iPixelSize.iWidth || full_scrn_sy != sz.iPixelSize.iHeight){
         full_scrn_sx = sz.iPixelSize.iWidth;
         full_scrn_sy = sz.iPixelSize.iHeight;
         ReInitScreen();
      }
#else
      ReInitScreen();
#endif
   }

//----------------------------

   virtual void _UpdateFlags(dword new_flags, dword flags_mask){

      flags_mask &= IG_SCREEN_USE_CLIENT_RECT|IG_SCREEN_USE_DSA|IG_ENABLE_BACKLIGHT|IG_USE_MEDIA_KEYS;
      new_flags &= flags_mask;
      const dword curr = app->ig_flags&flags_mask;
      if(new_flags!=curr){
         dword changed = curr ^ new_flags;
         app->ig_flags &= ~flags_mask;
         app->ig_flags |= new_flags;
         if(changed&IG_ENABLE_BACKLIGHT)
            SetBackLight((app->ig_flags&IG_ENABLE_BACKLIGHT));

         UpdateDsaFlag();
#ifdef USE_HW_KEY_CATCH
         if(changed&IG_USE_MEDIA_KEYS){
            if(app->ig_flags&IG_USE_MEDIA_KEYS)
               hw_key_catch.Construct(this);
            else
               hw_key_catch.Close();
         }
#endif
         if(changed&(IG_SCREEN_USE_CLIENT_RECT
#ifdef USE_CDSB
            |IG_SCREEN_USE_CLIENT_RECT
#endif
            |IG_SCREEN_USE_DSA
            )){
            ReInitScreen();
         }
      }
   }

//----------------------------

   void FixShiftKey(){
                              //detect shift up
#ifdef SHIFT_KEY_WORKAROUND
      if(!shift_key_workaround.key_down)
#endif
      if(!(CEikonEnv::Static()->WsSession().GetModifierState() & (EModifierLeftShift|EModifierRightShift|EModifierShift)))
         key_bits &= ~GKEY_SHIFT;
   }

//----------------------------

   virtual CCoeControl *GetCoeControl(){ return this; }

//----------------------------

   void MakeBacklightOn() const{

#ifdef USE_HWRM_LIGHT
      if(hwrm_light){
         //hwrm_light->LightOnL(CHWRMLight::EPrimaryDisplay);
      }else
#endif
         User::ResetInactivityTime();
   }

//----------------------------

   void SetBackLight(bool on){
      if(backlight_on!=on){
         backlight_on = on;
#ifdef USE_HWRM_LIGHT
         if(on){
            assert(!hwrm_light);
            struct S_lf: public C_leave_func{
               CHWRMLight *lp;
               virtual int Run(){
                  lp = CHWRMLight::NewL();
                  lp->LightOnL(CHWRMLight::EPrimaryDisplay);
                  return 0;
               }
            } lf;
            lf.lp = NULL;
            if(!lf.Execute() && lf.lp){
               hwrm_light = lf.lp;
            }
         }else{
            if(hwrm_light){
               struct S_lf: public C_leave_func{
                  CHWRMLight *lp;
                  int target;
                  virtual int Run(){
                     lp->LightOffL(CHWRMLight::EPrimaryKeyboard);
                     return 0;
                  }
               } lf;
               lf.lp = hwrm_light;
               lf.Execute();
               delete hwrm_light;
               hwrm_light = NULL;
            }
         }
#endif
         if(on)
            MakeBacklightOn();
      }
   }

//----------------------------
#if defined __SYMBIAN_3RD__ && defined S60
   virtual dword _GetRotation() const{

      switch(machine_uid){
      case UID_NOKIA_E90:
         if(app->_scrn_sx>600)   //softkeys at side, but probably doesn't report them correctly
            return 3;   //ROTATION_90CW
         break;
      case UID_NOKIA_N95_8GB:
      case UID_NOKIA_N96:
      case UID_NOKIA_6788:
                              //broken reporting of rotation on these devices
         return app->_scrn_sx==240 ? 0 : 3;
      case UID_LG_KT610:
         return 0;
      }
      TPixelsAndRotation por;
      screen_device->GetScreenModeSizeAndRotation(screen_device->CurrentScreenMode(), por);
      switch(por.iRotation){
      case CFbsBitGc::EGraphicsOrientationRotated90: return 3;
      case CFbsBitGc::EGraphicsOrientationRotated180: return 2;
      case CFbsBitGc::EGraphicsOrientationRotated270: return 1;
      }
      return 0;
   }
#endif
//----------------------------

   virtual S_point _GetFullScreenSize() const{
      TPixelsAndRotation por;
#ifdef __SYMBIAN_3RD__
      screen_device->GetScreenModeSizeAndRotation(screen_device->CurrentScreenMode(), por);
#else
      screen_device->GetDefaultScreenSizeAndRotation(por);
#endif
      return S_point(por.iPixelSize.iWidth, por.iPixelSize.iHeight);
   }
};

//----------------------------

C_application_base::E_LCD_SUBPIXEL_MODE C_application_base::DetermineSubpixelMode() const{

#ifdef __WINS__
                              //emulator
   return LCD_SUBPIXEL_RGB;
#else
                              //for tested phones, return correct RGB mode,
                              // otherwise return generic unknown
   switch(system::GetDeviceId()){
#ifdef __SYMBIAN_3RD__
   case UID_NOKIA_N73:
   case UID_NOKIA_N78:
   case UID_NOKIA_N80:
   case UID_NOKIA_N81:
   case UID_NOKIA_N82:
   case UID_NOKIA_N85:
   case UID_NOKIA_N86:
   case UID_NOKIA_N93:
   case UID_NOKIA_N95:
   case UID_NOKIA_N97:
   case UID_NOKIA_N97a:
   case UID_NOKIA_N97_mini:
   case UID_NOKIA_5320:
   case UID_NOKIA_6110:
   case UID_NOKIA_6120:
   case UID_NOKIA_E55:
   case UID_NOKIA_E52:
   case UID_NOKIA_X6:
      return LCD_SUBPIXEL_BGR;

   case UID_NOKIA_E50:
   case UID_NOKIA_E51:
   case UID_NOKIA_E60:
   case UID_NOKIA_E61:
   case UID_NOKIA_E61i:
   case UID_NOKIA_E62:
   case UID_NOKIA_E63:
   case UID_NOKIA_E65:
   case UID_NOKIA_E66:
   case UID_NOKIA_E70:
   case UID_NOKIA_E71:
   case UID_NOKIA_E71x:
   case UID_NOKIA_E72:
   case UID_NOKIA_E75:
   case UID_NOKIA_E90:
   case UID_NOKIA_N72:
   case UID_NOKIA_3250:
   case UID_NOKIA_5700:
   case UID_NOKIA_5800:
   case UID_SONYERICSSON_M600:
   case UID_SONYERICSSON_W950:
   case UID_SONYERICSSON_P990:
   case UID_SONYERICSSON_P1:
   case UID_SONYERICSSON_G900:
   case UID_SAMSUNG_I550:
      return LCD_SUBPIXEL_RGB;

   default:
      return LCD_SUBPIXEL_UNKNOWN;
#else
   case UID_NOKIA_3230:
   case UID_NOKIA_6260:
   case UID_NOKIA_6600:
   case UID_NOKIA_6620:
   case UID_NOKIA_6630:
   case UID_NOKIA_6670:
   case UID_NOKIA_6680:
   case UID_NOKIA_6681:
   case UID_NOKIA_6682:
   //case UID_NOKIA_7610:
   case UID_NOKIA_9x00:
   case UID_NOKIA_9300i:
   case UID_SONYERICSSON_P800:
   case UID_SONYERICSSON_P900:
   case UID_SONYERICSSON_P910:
   case UID_SENDO_X:
   case UID_NOKIA_N70:
   case UID_NOKIA_N90:
      return LCD_SUBPIXEL_RGB;
   case UID_NOKIA_3650:
   case UID_NOKIA_7650:
   case UID_NOKIA_NGAGE:
   case UID_NOKIA_NGAGE_QD:
   case UID_SIEMENS_SX1:
      return LCD_SUBPIXEL_BGR;
#endif
   }
   //return LCD_SUBPIXEL_UNKNOWN;
   return LCD_SUBPIXEL_RGB;
#endif
}

//----------------------------

IGraph *IGraphCreate(C_application_base *app, dword flags){

   IGraph_imp *ig = new(true) IGraph_imp(app, flags);
   ig->Construct();
   return ig;
}

//----------------------------
//----------------------------

C_application_base::S_system_font::S_system_font():
   use(false),
   last_used_font(NULL),
   last_text_color(0),
   font_width(NULL),
   ts(NULL),
   gc(NULL)
{
   MemSet(font_handle, 0, sizeof(font_handle));
}

//----------------------------
//----------------------------

void C_application_base::S_system_font::InitGc() const{

   gc = &igraph->GetBitmapGc();
   gc->SetPenColor(((last_text_color&0xff)<<16) | (last_text_color&0xff00) | ((last_text_color>>16)&0xff));
   gc->SetBrushStyle(CGraphicsContext::ENullBrush);
}

//----------------------------

void C_application_base::InitSystemFont(int size_delta, bool use_antialias, int small_font_size_percent){

   int size = Max(0, Min(NUM_SYSTEM_FONTS, GetDefaultFontSize(true)+size_delta));

   system_font.Close();
   system_font.igraph = igraph;

   system_font.use = true;
   system_font.use_antialias = use_antialias;

   font_defs = system_font.font_defs;
   CWsScreenDevice *sd = CCoeEnv::Static()->ScreenDevice();
   system_font.ts = CFbsTypefaceStore::NewL(sd);
#ifdef __SYMBIAN_3RD__
   //system_font.ts->SetDefaultBitmapType(use_antialias ? EAntiAliasedGlyphBitmap : EMonochromeGlyphBitmap);
   //system_font.ts->SetDefaultBitmapType(EDefaultGlyphBitmap);
#endif

   system_font.InitGc();

   system_font.font_width = new(true) byte[2*2*system_font.NUM_CACHE_WIDTH_CHARS];

   system_font.big_size = 11+size*3/2;
   system_font.small_size = Max(11, system_font.big_size*small_font_size_percent/100);  //note: rebooting phone on E61 chinese if using size <=10

                              //create fonts
   for(int fi=0; fi<NUM_FONTS; fi++){
      for(int b=0; b<2; b++)
         system_font.InitFont(0, b, false, fi!=0, !fi ? system_font.small_size : system_font.big_size);
      CFont *fnt = (CFont*)system_font.font_handle[0][0][fi];
      CFont *fnt_b = (CFont*)system_font.font_handle[1][0][fi];
      const byte *fw = system_font.font_width + fi*system_font.NUM_CACHE_WIDTH_CHARS;

      S_font &fd = system_font.font_defs[fi];
      fd.letter_size_x = fnt->WidthZeroInPixels()-1;
      fd.cell_size_x = fd.letter_size_x*12/8;
      fd.cell_size_y = fnt->HeightInPixels();
      fd.space_width = fw[0];
#ifdef __SYMBIAN_3RD__
      //fd.line_spacing = fnt->FontLineGap();
      //if(!fd.line_spacing)
#endif
         fd.line_spacing = fd.cell_size_y*10/8;
      fd.extra_space = 0;
      fd.bold_width_add = fnt_b->WidthZeroInPixels() - fnt->WidthZeroInPixels();
      fd.bold_width_subpix = fd.bold_width_add*3;
      fd.bold_width_pix = fd.bold_width_add;
   }
   fds = font_defs[UI_FONT_SMALL];
   fdb = font_defs[UI_FONT_BIG];
   SetClipRect(GetClipRect());
}

//----------------------------

void C_application_base::S_system_font::SelectFont(void *h) const{

   assert(h);
   last_used_font = h;
   gc->UseFont((CFont*)h);
}

//----------------------------

void C_application_base::S_system_font::SetClipRect(const S_rect &_rc){

   S_rect rc = _rc;
   rc.sx += rc.x;
   rc.sy += rc.y;
   gc->SetClippingRect((TRect&)rc);
}

//----------------------------

dword C_application_base::S_system_font::GetCharWidth(wchar c, bool bold, bool italic, bool big) const{

   if(c>=' ' && c<(' '+NUM_CACHE_WIDTH_CHARS)){
      const byte *fw = font_width + int(bold)*NUM_CACHE_WIDTH_CHARS*2 + int(big)*NUM_CACHE_WIDTH_CHARS;
      return fw[c-' '];
   }
   CFont *&fnt = (CFont*&)font_handle[bold][italic][big];
   if(!fnt)
      InitFont(0, bold, italic, big, !big ? small_size : big_size);
   /*
   CFont::TMeasureTextOutput mto;
   fnt->MeasureText(TPtrC((word*)&c, 1), NULL, &mto);
   */
   int w = fnt->CharWidthInPixels(c);
   if(w)
      --w;
   return w;
}

//----------------------------

void C_application_base::S_system_font::Close(){

   for(int i=0; i<8; i++){
      void *&h = font_handle[i>>2][(i>>1)&1][i&1];
      if(h){
#ifdef __SYMBIAN_3RD__
         ts->ReleaseFont((CFont*)h);
#endif
         h = NULL;
      }
   }
   gc = NULL;
   last_used_font = NULL;
   use = false;
   delete ts; ts = NULL;
   delete[] font_width; font_width = NULL;
}

//----------------------------

void C_application_base::S_system_font::InitFont(dword, bool bold, bool italic, bool big, int height) const{

   CFont *&fnt = (CFont*&)font_handle[bold][italic][big];
   assert(!fnt);
#ifdef __SYMBIAN_3RD__
   //TFontSpec fs(_L("Arial"), height);   // doesn't work good on Belle for Chinese
   TFontSpec fs;
   fs.iHeight = height;
   fs.iFontStyle.SetPosture(!italic ? EPostureUpright : EPostureItalic);
   fs.iFontStyle.SetStrokeWeight(!bold ? EStrokeWeightNormal : EStrokeWeightBold);
   fs.iFontStyle.SetBitmapType(use_antialias ? EAntiAliasedGlyphBitmap : EMonochromeGlyphBitmap);
   int err = ts->GetNearestFontInPixels(fnt, fs);
   if(err || !fnt){
      Fatal("GNFIP", err);
   }
#else
   if(height>17)
      fnt = (CFont*)CEikonEnv::Static()->TitleFont();
   else if(height>15)
      fnt = (CFont*)CEikonEnv::Static()->NormalFont();
   else if(height>13)
      fnt = (CFont*)CEikonEnv::Static()->SymbolFont();
   else
      fnt = (CFont*)CEikonEnv::Static()->DenseFont();
   //fnt = (CFont*)CEikonEnv::Static()->LegendFont();
   //fnt = (CFont*)CEikonEnv::Static()->AnnotationFont();
   /*
   TFontSpec fs(_L("Arial"), height);
   if(!italic){
      if(height<=12)
         fs.iTypeface.iName = _L("LatinPlain12");
      else
      if(height<=14)
         fs.iTypeface.iName = _L("LatinBold13");
      else
      if(height<=17)
         fs.iTypeface.iName = _L("LatinBold17");
      else
         fs.iTypeface.iName = _L("LatinBold19");
   }
   */
#endif
   assert(fnt);

   if(!italic){
      font_baseline[bold][big] = (byte)fnt->AscentInPixels()+1;

      byte *fw = font_width + int(bold)*NUM_CACHE_WIDTH_CHARS*2 + int(big)*NUM_CACHE_WIDTH_CHARS;
      for(int i=0; i<NUM_CACHE_WIDTH_CHARS; i++){
         word c = word(32+i);
         TOpenFontCharMetrics fm;
         const byte *b = NULL;
         TSize s(0,0);
         fnt->GetCharacterData(c, fm, b, s);

         int w;
         if(!i){
            //w = fm.HorizAdvance()-1;
            w = fnt->WidthZeroInPixels()/2;
         }else{
            w = fm.Width();
            int bx = fm.HorizBearingX();
            //if(bx<0)
               w += bx;
            //w = fm.HorizAdvance()-1;
            w = Min(w, fm.HorizAdvance()-1);
         }
         //if(c==' ' && big && !bold) Info("!", w);
         /*
         int w = fnt->CharWidthInPixels(c);
         if(w)
            --w;
            */
         fw[i] = (byte)w;
      }
   }
}

//----------------------------

void C_application_base::RenderSystemChar(int x, int y, wchar c, dword curr_font, dword font_flags, dword text_color){

   assert(curr_font<2);
   bool bold = (font_flags&FF_BOLD);
   bool italic = (font_flags&FF_ITALIC);

   CFont *&fnt = (CFont*&)system_font.font_handle[bold][italic][curr_font];
   if(!fnt)
      system_font.InitFont(0, bold, italic, curr_font, !curr_font ? system_font.small_size : system_font.big_size);
   if(system_font.last_used_font!=fnt)
      system_font.SelectFont(fnt);

   if(font_flags&FF_ACTIVE_HYPERLINK){
      dword w = system_font.GetCharWidth(c, bold, italic, curr_font)+1;
      if(bold)
         w += font_defs[curr_font].bold_width_add;
      FillRect(S_rect(x, y, w, font_defs[curr_font].line_spacing-1), text_color);
      text_color ^= 0xffffff;
   }
   const int baseline = system_font.font_baseline[bold][curr_font];

   dword rotation = (font_flags>>FF_ROTATE_SHIFT) & 3;
   switch(rotation){
   case 0:
      y += baseline;
      break;
   case 1:
      Swap(x, y);
      y = ScrnSY()-y;
      x += baseline;
      break;
   case 3:
      Swap(x, y);
      x = ScrnSX()-x;
      x -= baseline;
      break;
   }

   if(font_flags&FF_UNDERLINE){
      dword w = system_font.GetCharWidth(c, bold, italic, curr_font)+1;
      if(bold)
         w += font_defs[curr_font].bold_width_add;
      DrawHorizontalLine(x, y+1, w, text_color);
   }

   //text_color = 0xffff0000;
   if(system_font.last_text_color!=text_color){
      system_font.last_text_color = text_color;
      system_font.gc->SetPenColor(((text_color&0xff)<<16) | (text_color&0xff00) | ((text_color>>16)&0xff));
   }

   TPtrC desc((word*)&c, 1);
   TPoint pt(x, y);
   switch(rotation){
   case 0: system_font.gc->DrawText(desc, pt); break;
   case 1: system_font.gc->DrawTextVertical(desc, pt, true); break;
   case 3: system_font.gc->DrawTextVertical(desc, pt, false); break;
   }
}

//----------------------------
//----------------------------

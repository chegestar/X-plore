//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------
#include <E32Std.h>
#include "..\Common\UserInterface\TextEditorCommon.h"

#ifdef _DEBUG
//#define DEBUG_EDWIN
#endif

#define __FEPBASE_H__
#include <Eikappui.h>
#include <EikEdwin.h>
#include <CoeCntrl.h>

//----------------------------
                              //provide our own definition of MCoeFepAwareTextEditor (need to access private funcs using inline funcs)
class MCoeFepAwareTextEditor{
public:
   virtual void StartFepInlineEditL(const TDesC& aInitialInlineText, TInt aPositionOfInsertionPointInInlineText, TBool aCursorVisibility, const MFormCustomDraw* aCustomDraw, MFepInlineTextFormatRetriever& aInlineTextFormatRetriever, class MFepPointerEventHandlerDuringInlineEdit& aPointerEventHandlerDuringInlineEdit)=0;
   virtual void UpdateFepInlineTextL(const TDesC& aNewInlineText, TInt aPositionOfInsertionPointInInlineText)=0;
   virtual void SetInlineEditingCursorVisibilityL(TBool aCursorVisibility)=0;
   IMPORT_C void CommitFepInlineEditL(CCoeEnv& aConeEnvironment);
   virtual void CancelFepInlineEdit()=0;
   virtual TInt DocumentLengthForFep() const=0;
   virtual TInt DocumentMaximumLengthForFep() const=0;
   virtual void SetCursorSelectionForFepL(const TCursorSelection& aCursorSelection)=0;
   virtual void GetCursorSelectionForFep(TCursorSelection& aCursorSelection) const=0;
   virtual void GetEditorContentForFep(TDes& aEditorContent, TInt aDocumentPosition, TInt aLengthToRetrieve) const=0;
   virtual void GetFormatForFep(TCharFormat& aFormat, TInt aDocumentPosition) const=0;
   virtual void GetScreenCoordinatesForFepL(TPoint& aLeftSideOfBaseLine, TInt& aHeight, TInt& aAscent, TInt aDocumentPosition) const=0;
   IMPORT_C class MCoeFepAwareTextEditor_Extension1* Extension1();
                              //MB:
   inline MCoeFepAwareTextEditor_Extension1* Ext1(TBool& aSetToTrue){ return Extension1(aSetToTrue); }
   inline void DoCom(){ DoCommitFepInlineEditL(); }
private:
   virtual void DoCommitFepInlineEditL()=0;
   IMPORT_C virtual MCoeFepAwareTextEditor_Extension1* Extension1(TBool& aSetToTrue);
   IMPORT_C virtual void MCoeFepAwareTextEditor_Reserved_2();
};

//----------------------------

class MCoeFepAwareTextEditor_Extension1{
public:
   class CState: public CBase {
   protected:
      IMPORT_C CState();
      IMPORT_C void BaseConstructL();
   public:
      IMPORT_C virtual ~CState();
   private:
      IMPORT_C virtual void CState_Reserved_1();
      IMPORT_C virtual void CState_Reserved_2();
      IMPORT_C virtual void CState_Reserved_3();
      IMPORT_C virtual void CState_Reserved_4();
   private:
      TAny* iSpareForFutureUse;
   };
public:
   virtual void SetStateTransferingOwnershipL(class CState* aState, TUid aTypeSafetyUid)=0; // this function must only transfer ownership after it has successfully done everything that can leave
   virtual CState* State(TUid aTypeSafetyUid)=0; // this function does *not* transfer ownership
public:
   IMPORT_C virtual void StartFepInlineEditL(TBool& aSetToTrue, const TCursorSelection& aCursorSelection, const TDesC& aInitialInlineText, TInt aPositionOfInsertionPointInInlineText, TBool aCursorVisibility, const MFormCustomDraw* aCustomDraw, MFepInlineTextFormatRetriever& aInlineTextFormatRetriever, MFepPointerEventHandlerDuringInlineEdit& aPointerEventHandlerDuringInlineEdit);
   IMPORT_C virtual void SetCursorType(TBool& aSetToTrue, const TTextCursor& aTextCursor);
private:
   IMPORT_C virtual void MCoeFepAwareTextEditor_Extension1_Reserved_3();
   IMPORT_C virtual void MCoeFepAwareTextEditor_Extension1_Reserved_4();
};

//----------------------------

class MFepAttributeStorer{
public:
   IMPORT_C void ReadAllAttributesL(CCoeEnv& aConeEnvironment);
   IMPORT_C void WriteAttributeDataAndBroadcastL(CCoeEnv& aConeEnvironment, TUid aAttributeUid);
   IMPORT_C void WriteAttributeDataAndBroadcastL(CCoeEnv& aConeEnvironment, const TArray<TUid>& aAttributeUids);
   virtual TInt NumberOfAttributes() const=0;
   virtual TUid AttributeAtIndex(TInt aIndex) const=0;
   virtual void WriteAttributeDataToStreamL(TUid aAttributeUid, RWriteStream& aStream) const=0;
   virtual void ReadAttributeDataFromStreamL(TUid aAttributeUid, RReadStream& aStream)=0;
private:
   IMPORT_C virtual void MFepAttributeStorer_Reserved_1();
   IMPORT_C virtual void MFepAttributeStorer_Reserved_2();
   TInt NumberOfOccurrencesOfAttributeUid(TUid aAttributeUid) const;
};

class CCoeFep: public CBase, protected MFepAttributeStorer, public MCoeForegroundObserver, public MCoeFocusObserver, private MCoeMessageObserver{
public:
   enum TEventResponse{ EEventWasNotConsumed, EEventWasConsumed };
   class MDeferredFunctionCall{
   public:
      virtual void ExecuteFunctionL()=0;
   private:
      IMPORT_C virtual void MDeferredFunctionCall_Reserved_1();
      IMPORT_C virtual void MDeferredFunctionCall_Reserved_2();
   };
   class MKeyEventQueue{ // Internal to Symbian
   public:
      virtual void AppendKeyEventL(const TKeyEvent& aKeyEvent)=0;
   private:
      IMPORT_C virtual void MKeyEventQueue_Reserved_1();
      IMPORT_C virtual void MKeyEventQueue_Reserved_2();
   };
   class MModifiedCharacter{
   public:
      virtual TUint CharacterCode() const=0;
      virtual TUint ModifierMask() const=0;
      virtual TUint ModifierValues() const=0;
   private:
      IMPORT_C virtual void MModifiedCharacter_Reserved_1();
      IMPORT_C virtual void MModifiedCharacter_Reserved_2();
   };
public:
   IMPORT_C virtual ~CCoeFep();
   IMPORT_C TBool IsSimulatingKeyEvent() const;
   IMPORT_C TBool IsTurnedOnByL(const TKeyEvent& aKeyEvent) const;
   IMPORT_C TBool IsTurnedOffByL(const TKeyEvent& aKeyEvent) const;
   // new public virtual functions
   virtual void CancelTransaction()=0;
public: // Internal to Symbian
   // the following functions (and also those of MCoeForegroundObserver) are to be called by applications that do not always have an active-scheduler running, (e.g. OPLR)
   IMPORT_C TEventResponse OfferKeyEvent(TInt& aError, MKeyEventQueue& aKeyEventQueue, const TKeyEvent& aKeyEvent, TEventCode aEventCode);
   IMPORT_C TEventResponse OfferPointerEvent(TInt& aError, MKeyEventQueue& aKeyEventQueue, const TPointerEvent& aPointerEvent, const CCoeControl* aWindowOwningControl);
   IMPORT_C TEventResponse OfferPointerBufferReadyEvent(TInt& aError, MKeyEventQueue& aKeyEventQueue, const CCoeControl* aWindowOwningControl);
   IMPORT_C static TEventResponse OfferEventToActiveScheduler(CCoeFep* aFep, TInt& aError, MKeyEventQueue& aKeyEventQueue, TInt aMinimumPriority); // aFep may be NULL
public: // *** Do not use! API liable to change ***
   IMPORT_C void OnStartingHandlingKeyEvent_WithDownUpFilterLC();
   IMPORT_C void OnStartingHandlingKeyEvent_NoDownUpFilterLC();
   IMPORT_C TKeyResponse OnFinishingHandlingKeyEvent_WithDownUpFilterL(TEventCode aEventCode, const TKeyEvent& aKeyEvent, TKeyResponse aKeyResponse);
   IMPORT_C TKeyResponse OnFinishingHandlingKeyEvent_NoDownUpFilterL(TEventCode aEventCode, const TKeyEvent& aKeyEvent, TKeyResponse aKeyResponse);
protected:
   IMPORT_C CCoeFep(CCoeEnv& aConeEnvironment);
   IMPORT_C void BaseConstructL(const CCoeFepParameters& aFepParameters);
   IMPORT_C void ReadAllAttributesL();
   IMPORT_C void MakeDeferredFunctionCall(MDeferredFunctionCall& aDeferredFunctionCall);
   IMPORT_C void SimulateKeyEventsL(const TArray<TUint>& aArrayOfCharacters);
   IMPORT_C void SimulateKeyEventsL(const TArray<MModifiedCharacter>& aArrayOfModifiedCharacters);
   IMPORT_C void WriteAttributeDataAndBroadcastL(TUid aAttributeUid);
   IMPORT_C void WriteAttributeDataAndBroadcastL(const TArray<TUid>& aAttributeUids);
   IMPORT_C TBool IsOn() const;
private:
   enum {EUndefinedEventResponse=100};
   enum{
      EFlagDefaultIsOn =0x00000001,
      EFlagIsOn =0x00000002,
      EFlagIsHandlingKeyEvent =0x00000004,
      EFlagNoDownUpFilter =0x00000008
   };
   struct SKeyEvent{
      TEventCode iEventCode;
      TKeyEvent iKeyEvent;
      TKeyResponse iKeyResponse;
   };
   class CHighPriorityActive;
   class CLowPriorityActive;
private:
   void DoOnStartingHandlingKeyEventLC(TUint aFlagNoDownUpFilter);
   TKeyResponse DoOnFinishingHandlingKeyEventL(TEventCode aEventCode, const TKeyEvent& aKeyEvent, TKeyResponse aKeyResponse);
   //static TBool KeyEventMatchesOnOrOffKeyData(const TKeyEvent& aKeyEvent, const TFepOnOrOffKeyData& aOnOrOffKeyData);
   static void TurnOffKeyEventHandlingFlags(TAny* aFlags);
   // from MFepAttributeStorer
   IMPORT_C virtual void MFepAttributeStorer_Reserved_1();
   IMPORT_C virtual void MFepAttributeStorer_Reserved_2();
   // from MCoeForegroundObserver
   IMPORT_C virtual void MCoeForegroundObserver_Reserved_1();
   IMPORT_C virtual void MCoeForegroundObserver_Reserved_2();
   // from MCoeFocusObserver
   IMPORT_C virtual void MCoeFocusObserver_Reserved_1();
   IMPORT_C virtual void MCoeFocusObserver_Reserved_2();
   // from MCoeMessageObserver
   IMPORT_C virtual TMessageResponse HandleMessageL(TUint32 aClientHandleOfTargetWindowGroup, TUid aMessageUid, const TDesC8& aMessageParameters);
   IMPORT_C virtual void MCoeMessageObserver_Reserved_1();
   IMPORT_C virtual void MCoeMessageObserver_Reserved_2();
   // new private virtual functions
   virtual void IsOnHasChangedState()=0;
   virtual void OfferKeyEventL(TEventResponse& aEventResponse, const TKeyEvent& aKeyEvent, TEventCode aEventCode)=0;
   virtual void OfferPointerEventL(TEventResponse& aEventResponse, const TPointerEvent& aPointerEvent, const CCoeControl* aWindowOwningControl)=0;
   virtual void OfferPointerBufferReadyEventL(TEventResponse& aEventResponse, const CCoeControl* aWindowOwningControl)=0;
   IMPORT_C virtual void CCoeFep_Reserved_1();
   IMPORT_C virtual void CCoeFep_Reserved_2();
};

//----------------------------
#include <baclipb.h>

void C_text_editor::ClipboardCopy(const wchar *text){

   CClipboard *cb = CClipboard::NewForWritingLC(CCoeEnv::Static()->FsSession());
   cb->StreamDictionary().At(KClipboardUidTypePlainText);
   CPlainText *pt = CPlainText::NewL();
   pt->InsertL(0, TPtrC((const word*)text, StrLen(text)));
   pt->CopyToStoreL(cb->Store(), cb->StreamDictionary(), 0, pt->DocumentLength());
   delete pt;
   cb->CommitL();
   CleanupStack::PopAndDestroy();
}

//----------------------------

Cstr_w C_text_editor::ClipboardGet(){

   Cstr_w ret;
   CClipboard *cb = CClipboard::NewForReadingL(CCoeEnv::Static()->FsSession());
   CPlainText *pt = CPlainText::NewL();
   cb->StreamDictionary().At(KClipboardUidTypePlainText);
   pt->PasteFromStoreL(cb->Store(), cb->StreamDictionary(), 0);
   TPtrC des = pt->Read(0);
   const wchar *mem = (wchar*)des.Ptr();
   dword sz = des.Length();
   while(sz && !mem[sz-1] || mem[sz-1]==0x2029)
      --sz;
   ret.Allocate(mem, sz);
   delete pt;
   delete cb;
   return ret;
}

//----------------------------

#ifdef S60
#include <AknIndicatorContainer.h>
#include <AknEnv.h>
#include <avkon.hrh>
#include <AknEdSts.h>
#include <baclipb.h>

//----------------------------

enum TAknEditingState{
   EStateNone,
   ET9Upper,
   ET9Lower,
   ET9Shifted,
   ENumeric,
   EMultitapUpper,
   EMultitapLower,
   EMultitapShifted,
};

//----------------------------

class MAknEditingStateIndicator{
public:
   virtual void SetState(TAknEditingState aState) = 0;
   virtual CAknIndicatorContainer *IndicatorContainer() = 0;
};
#endif//S60

#ifdef DEBUG_EDWIN
#include <EikGTEd.h>
#endif

struct S_uid: public TUid{
   S_uid(int i){ iUid = i; }
};

//----------------------------

class C_cc: public CCoeControl{
public:
   virtual void AddChild(CCoeControl*) = 0;
   virtual void RemoveChild(CCoeControl*) = 0;
};

//----------------------------

class C_text_editor_Symbian: public C_text_editor_common,
#ifdef DEBUG_EDWIN_
   public CEikGlobalTextEditor
#else
   public CEikEdwin, public MCoeFepAwareTextEditor, public MEikEdwinObserver
#endif
{

   int inline_pos, inline_len;
   bool activated;
   bool cursor_moved, text_dirty;

   dword saved_ig_flags;

   mutable MCoeFepAwareTextEditor *fte;

   CPeriodic *timer;

//----------------------------

   void SetSelFromEdwin(){
      TCursorSelection sel = CEikEdwin::Selection();
      if(mask_password && !sel.iCursorPos)
         return;
      if(cursor_sel!=sel.iAnchorPos || cursor_pos != sel.iCursorPos){
                              //watch range - also this happens with FEP, that selection is ahead of text
         int len = text.size()-1;
         cursor_sel = Min(sel.iAnchorPos, len);
         cursor_pos = Min(sel.iCursorPos, len);
         //cursor_moved = true;
      }
   }

//----------------------------

   virtual int GetInlineText(int &pos) const{
      pos = inline_pos;
      return inline_len;
   }

//----------------------------

   virtual const wchar *GetText() const{
      const_cast<C_text_editor_Symbian*>(this)->CorrectText();
      return text.begin();
   }

//----------------------------

   virtual dword GetTextLength() const{
      const_cast<C_text_editor_Symbian*>(this)->CorrectText();
      return text.size()-1;
   }

//----------------------------

   virtual void SetCursorPos(int pos, int sel){

                              //no cursor movement if password is masked
      if(mask_password)
         return;

      /*
      CancelFepTransaction();    //this may remove inline text, and decrease text length!
      {
         int len = GetTextLength();
         pos = Min(pos, len);
         sel = Min(sel, len);
      }
      */
                              //fix edwin bug: it doesn't offer Copy option when we expand selection by up/down key
                              // so simulate left or right key before setting selection
      if(activated && pos!=sel && (te_flags&TXTED_ALLOW_EOL)){
         TKeyEvent e;
         e.iModifiers = EModifierShift;
         e.iRepeats = 0;
         if(pos>cursor_pos){
            e.iCode = EKeyRightArrow;
            e.iScanCode = EStdKeyRightArrow;
         }else{
            e.iCode = EKeyLeftArrow;
            e.iScanCode = EStdKeyLeftArrow;
         }
         CCoeEnv::Static()->SimulateKeyEventL(e, EEventKey);
      }
      if(inline_len){
         /*
                              //if our pos is ahead of inline pos, simulate Enter to accept inline text, otherwise just cancel fep
         if(pos>=inline_pos){
            static const TKeyEvent e = { EKeyEnter, EStdKeyEnter };
            CCoeEnv::Static()->SimulateKeyEventL(e, EEventKey);
         }else
         */
            CancelFepTransaction();
         int l = GetTextLength();
         pos = Min(pos, l);
         sel = Min(sel, l);
      }
      CEikEdwin::SetSelectionL(pos, sel);
      C_text_editor_common::SetCursorPos(pos, sel);
      if(inline_len){
         inline_len = 0;
         inline_pos = -1;
         cursor_on = true;
      }
   }

//----------------------------

   static void ConvertChars(wchar *wp, int len){
      while(len--){
         switch(*wp){
         case 0x2029:
            *wp = '\n';
            break;
         }
         ++wp;
      }
   }

//----------------------------

   void TimerProc(){
#ifdef USE_MOUSE
                                 //test user inactivity by system call, so that cursor flashes while typing with pen
      if(!idle_countdown && User::InactivityTime().Int() < 2)
         idle_countdown = IDLE_COUNTDOWN;
#endif
      bool td = CorrectText() || text_dirty;
      bool cf = TickCursor();
      if(td || cf || cursor_moved){
         SetSelFromEdwin();
         TextEditNotify(cf, cursor_moved, td);
         text_dirty = false;
         cursor_moved = false;
      }
   }

//----------------------------

   static TInt TimerProcThunk(TAny *c){
      ((C_text_editor_Symbian*)c)->TimerProc();
      return 1;
   }

//----------------------------

   void ResetFep(){
      CCoeFep *fep = CCoeEnv::Static()->Fep();
      if(fep)
         fep->HandleChangeInFocus();
   }

//----------------------------

   virtual void Activate(bool b){

      if(b==activated)
         return;
      const S_global_data *gd = GetGlobalData();
      if(b){
         saved_ig_flags = app->GetGraphicsFlags();
         app->UpdateGraphicsFlags(0, IG_SCREEN_USE_DSA);
         int pos = cursor_pos, sel = cursor_sel;

         gd->GetAppUi()->AddToStackL(this, ECoeStackPriorityDefault+1);
         if(te_flags&TXTED_REAL_VISIBLE){
            C_cc *ctrl = (C_cc*)app->GetIGraph()->GetCoeControl();
            ctrl->AddChild(this);
            DrawDeferred();
         }
         ActivateL();
         CEikEdwin::SetSelectionL(pos, sel); //ActivateL may change cursor position, so reset it here

         CEikEdwin::SetFocus(true);
         //CEikEdwin::SetSelectionL(cursor_pos, cursor_sel);

         ResetCursor();
         timer = CPeriodic::NewL(CActive::EPriorityUserInput);
         dword us = 1000 * 100;
         timer->Start(us, us, TCallBack(TimerProcThunk, this));
      }else{
         timer->Cancel();
         delete timer;
         timer = NULL;
         if(te_flags&TXTED_REAL_VISIBLE){
            C_cc *ctrl = (C_cc*)app->GetIGraph()->GetCoeControl();
            ctrl->RemoveChild(this);
         }
         gd->GetAppUi()->RemoveFromStack(this);
         CEikEdwin::SetFocus(false);

         app->UpdateGraphicsFlags(saved_ig_flags, IG_SCREEN_USE_DSA);
      }
      ResetFep();
#ifdef USE_SYSTEM_FONT
                              //re-init fonts, because backbuffer classes may be destroyed
      //app->InitSystemFont();
#endif
      activated = b;
      C_text_editor_common::Activate(b);
   }

//----------------------------

#ifdef S60
   bool IsT9() const{
      MAknEditingStateIndicator *ei = CAknEnv::Static()->EditingStateIndicator();
      if(!ei)
         return false;
      CAknIndicatorContainer *ic = ei->IndicatorContainer();
      if(!ic)
         return false;
      return (ic->IndicatorState(S_uid(EAknNaviPaneEditorIndicatorT9)));
   }
#endif//S60

//----------------------------

   bool CorrectText(){
      //if(CEikEdwin::TextLength()!=GetTextLength())
      {
         SetSelFromEdwin();
         int edwin_len = CEikEdwin::TextLength();
         if(mask_password && !edwin_len)
            return false;
         CPlainText *tp = CEikEdwin::Text();
         TPtrC tpp = tp->Read(0);
         const wchar *wp = (wchar*)tpp.Ptr();
         int tl = text.size()-1;
                              //check if they differ
         if(edwin_len==tl){
            const wchar *wp1 = text.begin();
            int i;
            for(i=tl; i--; ){
               if(*wp != *wp1){
                  if(*wp!=0x2029 || *wp1!='\n')
                     break;
               }
               ++wp, ++wp1;
            }
            if(i==-1)
               return false;
            wp = (wchar*)tpp.Ptr();
         }
         text.clear();
         text.insert(text.begin(), wp, wp+edwin_len);
         ConvertChars(text.begin(), text.size());
         text.push_back(0);
         text_dirty = true;
         cursor_moved = true;
         mask_password = false;
         ResetCursor();
         return true;
      }
   }

//----------------------------
#ifndef DEBUG_EDWIN_
                           //MEikEdwinObserver:
   virtual void HandleEdwinEventL(CEikEdwin *edwin, TEdwinEvent et){

      assert(edwin==this);
      switch(et){
      case EEventNavigation:
         SetSelFromEdwin();
         ResetCursor();
         if(inline_len){
            inline_len = 0;
            inline_pos = -1;
            cursor_on = true;
         }
         //TextEditNotify(false, true, false);
         cursor_moved = true;
         break;
      case EEventFormatChanged:
         //TextEditNotify(false, false, true);
         text_dirty = true;
         //cursor_moved = false;
         break;
      default:
         //et = et;
         break;
      }
   }

#endif
//----------------------------

   void EraseSelection(){

      if(cursor_pos==cursor_sel)
         return;
      int selmin = cursor_pos;
      int selmax = cursor_sel;
      if(selmin > selmax)
         Swap(selmin, selmax);
      text.erase(&text[selmin], &text[selmax]);
      cursor_pos = cursor_sel = selmin;
   }

//----------------------------
                           //MCoeFepAwareTextEditor:
   virtual void StartFepInlineEditL(const TDesC &init_text, TInt insert_pos, TBool cursor_vis, const MFormCustomDraw *cust_drw, MFepInlineTextFormatRetriever &tfr, MFepPointerEventHandlerDuringInlineEdit &peh){

      //if(C_text_editor_common::IsReadOnly()) return;
      fte->StartFepInlineEditL(init_text, insert_pos, cursor_vis, cust_drw, tfr, peh);

      EraseSelection();
      //inline_pos = insert_pos;
      inline_len = 0;
      //cursor_sel = cursor_pos;
      SetSelFromEdwin();
      inline_pos = cursor_pos;// - init_text.Length();
      cursor_on = cursor_vis;
      //bool text_dirty = false;
      if(init_text.Length()){
         inline_len = init_text.Length();
         text.insert(&text[inline_pos], (wchar*)init_text.Ptr(), (wchar*)init_text.Ptr()+inline_len);
         ConvertChars(text.begin()+inline_pos, inline_len);
         text_dirty = true;
         mask_password = false;
      }
      cursor_moved = true;
      //TextEditNotify(false, true, text_dirty);
   }
//----------------------------
   virtual void UpdateFepInlineTextL(const TDesC &new_text, TInt new_pos){

      //if(C_text_editor_common::IsReadOnly()) return;
      fte->UpdateFepInlineTextL(new_text, new_pos);

      if(inline_pos==-1){
         SetSelFromEdwin();
         inline_pos = cursor_pos - new_pos;
         inline_pos = Max(0, inline_pos);
         inline_len = (text.size()-1) - inline_pos;
      }
      text.erase(&text[inline_pos], &text[inline_pos+inline_len]);
      inline_len = new_text.Length();
      text.insert(&text[inline_pos], (wchar*)new_text.Ptr(), (wchar*)new_text.Ptr()+inline_len);
      ConvertChars(text.begin()+inline_pos, inline_len);
      SetSelFromEdwin();
      mask_password = false;
      //TextEditNotify(false, true, true);
      text_dirty = true;
   }
//----------------------------
   virtual void SetInlineEditingCursorVisibilityL(TBool vis){

      fte->SetInlineEditingCursorVisibilityL(vis);

      cursor_on = vis;
      ResetCursor();
   }
//----------------------------
   virtual void DoCommitFepInlineEditL(){

      fte->DoCom();
      mask_password = false;
      cursor_on = true;

      inline_pos = -1;
      inline_len = 0;
      //TextEditNotify(false, true, true);
      text_dirty = true;
   }
//----------------------------
   virtual void CancelFepInlineEdit(){

      fte->CancelFepInlineEdit();

      inline_pos = -1;
      inline_len = 0;
   }
//----------------------------
   virtual TInt DocumentLengthForFep() const{
      int len = fte->DocumentLengthForFep();
      if((text.size()-1)!=len && !mask_password){
         const_cast<C_text_editor_Symbian*>(this)->CorrectText();
         //TextEditNotify(false, true, true);
         assert((text.size()-1) == len);
      }
      return len;
   }
//----------------------------
   virtual TInt DocumentMaximumLengthForFep() const{
      return fte->DocumentMaximumLengthForFep();
   }
//----------------------------
   virtual void GetEditorContentForFep(TDes &txt, TInt pos, TInt len) const{
      fte->GetEditorContentForFep(txt, pos, len);
   }
//----------------------------
   virtual void GetFormatForFep(TCharFormat &fmt, TInt pos) const{
      fte->GetFormatForFep(fmt, pos);
   }
//----------------------------
   virtual void GetScreenCoordinatesForFepL(TPoint &bl, TInt &h, TInt &a, TInt pos) const{
      C_vector<C_application_ui::S_text_line> lines;
      lines.reserve((text.size()-1)/32);
      int cursor_line, cursor_col, cursor_x_pixel;
      app->DetermineTextLinesLengths(text.begin(), font_index, rc.sx, cursor_pos, lines, cursor_line, cursor_col, cursor_x_pixel);
      //cursor_line -= top_line;
      bl.iX = rc.x + cursor_x_pixel;
      const S_font &fd = app->GetFontDef(font_index);
      h = fd.line_spacing;
      a = fd.line_spacing - fd.cell_size_y;
      bl.iY = rc.y + h*(cursor_line+1)-(h-a) - scroll_y;
   }
//----------------------------
   virtual void SetCursorSelectionForFepL(const TCursorSelection &sel){
      fte->SetCursorSelectionForFepL(sel);

      if((sel.iAnchorPos!=cursor_sel || sel.iCursorPos != cursor_pos) && !mask_password){
         cursor_sel = sel.iAnchorPos;
         cursor_pos = sel.iCursorPos;
         //TextEditNotify(false, true, false);
         cursor_moved = true;
      }
   }
//----------------------------
   void GetCursorSelectionForFep1(TCursorSelection &sel){
      fte->GetCursorSelectionForFep(sel);
      if((cursor_sel!=sel.iAnchorPos || cursor_pos!=sel.iCursorPos) && !mask_password){
#ifdef S60
                              //fix bug when T9 FEP doesn't set inline edit selection when moving among words
         if(IsT9() && cursor_sel!=cursor_pos){
            //assert(cursor_sel<cursor_pos);
            inline_len = sel.iCursorPos-cursor_sel;
            if(inline_len){
               inline_pos = cursor_sel;
               //cursor_on = false;
            }
         }
#endif//S60
         cursor_sel = sel.iAnchorPos;
         cursor_pos = sel.iCursorPos;
                              //validate values, sometimes FEP sends out of range
         int tlen = text.size();
         cursor_sel = Max(0, Min(tlen, cursor_sel));
         cursor_pos = Max(0, Min(tlen, cursor_pos));
         //TextEditNotify(false, true, false);
         cursor_moved = true;
      }
   }
//----------------------------
   virtual void GetCursorSelectionForFep(TCursorSelection &sel) const{
      const_cast<C_text_editor_Symbian*>(this)->GetCursorSelectionForFep1(sel);
   }
//----------------------------
   virtual MCoeFepAwareTextEditor_Extension1 *Extension1(TBool &aSetToTrue){
      return fte ? fte->Ext1(aSetToTrue) : NULL;
   }

//----------------------------

   /*
   virtual void HandlePointerEventL(const TPointerEvent &pe){
      //Fatal("HPE", pe.iType);
   }
   */

//----------------------------
#ifndef DEBUG_EDWIN

#ifdef S60
public:
   CAknEdwinState *GetState(){
      if(!fte)
         return NULL;
      TBool t;
      MCoeFepAwareTextEditor_Extension1 *ext = Extension1(t);
      if(!ext)
         return NULL;
      return (CAknEdwinState*)ext->State(S_uid(0));
   }
private:
#endif

//----------------------------
                              //CCoeControl:
   virtual TCoeInputCapabilities InputCapabilities() const{
      TCoeInputCapabilities ic = CEikEdwin::InputCapabilities();
      //test: ic.MergeWith(TCoeInputCapabilities::ENavigation);
      if(!(te_flags&TXTED_READ_ONLY))
      {
         switch(system::GetDeviceId()){
#if !(defined USE_SYSTEM_FONT)
         case 0x20001858:  //E61
#endif
         case 0x20001859:  //E62
            break;
         default:
                              //replace fep to ours
            fte = ic.FepAwareTextEditor();
            //Fatal("!", (dword)fte);
            ((const MCoeFepAwareTextEditor**)&ic)[1] = this;
         }
      }
      return ic;
   }

//----------------------------

   virtual TKeyResponse OfferKeyEventL(const TKeyEvent &key_event, TEventCode type){

      //return EKeyWasConsumed;
      idle_countdown = IDLE_COUNTDOWN;
      //Info("k", type);
      switch(type){
         /*
      case EEventKeyDown:
      case EEventKeyUp:
         if(C_text_editor_common::IsReadOnly())
            return EKeyWasNotConsumed;
         break;
         */

      case EEventKey:
         //cursor_moved = true;
         //Info("k", key_event.iCode);
         switch(key_event.iCode){
         case EKeyUpArrow:
         case EKeyDownArrow:
            if(!(te_flags&TXTED_ALLOW_EOL)){
               if(key_event.iModifiers&(EModifierLeftShift|EModifierRightShift|EModifierShift)){
                  SetCursorPos(key_event.iCode==EKeyUpArrow ? 0 : (text.size()-1), cursor_sel);
                  ResetCursor();
                  //TextEditNotify(false, true, false);
                  text_dirty = true;
                  break;
               }
            }
         case 0x1b:           //esc
            return EKeyWasNotConsumed;
#if defined S60 && defined __SYMBIAN_3RD__
         case '7':
                              //detect Paste bug on E61/E62, which produces '7' instead of paste
            if(key_event.iModifiers&(EModifierLeftCtrl|EModifierRightCtrl)){
               switch(system::GetDeviceId()){
               case 0x20001858:  //E61
               case 0x20002d7f:  //E61i
               case 0x20001859:  //E62
               case 0x2000249b:  //E71
               case 0x20014dd8:  //E71x
               case 0x20014dd0:  //E72
                  Paste();
                  return EKeyWasConsumed;
               }
            }else
            if(C_text_editor_common::IsReadOnly())
               return EKeyWasNotConsumed;
            break;
#endif//S60
         case EKeyBackspace:
         case EKeyDelete:
            if(mask_password){
               mask_password = false;
               SetInitText(NULL);
            }else
            if(C_text_editor_common::IsReadOnly())
               return EKeyWasNotConsumed;
            break;

            /*
         case EKeyLeftArrow:
         case EKeyRightArrow:
            if(!GetTextLength())
               //return EKeyWasNotConsumed;
               Fatal("!");
            break;
            */

#ifdef S60
         case EKeyDevice3:
#else
#error
#endif
                              //OK
#ifdef _DEBUG
            //CCoeEnv::Static()->Fep()->HandleChangeInFocus();
            //Fatal("!");
            //return EKeyWasConsumed;
#endif
            if((te_flags&TXTED_ALLOW_EOL) && !C_text_editor_common::IsReadOnly()){
               ReplaceSelection(L"\n");
               text_dirty = true;
               return EKeyWasConsumed;
            }
            /*
            CorrectText();
            Info(C_text_editor::GetText());
            return EKeyWasConsumed;
            */
            break;

         case EKeyEnter:
            if(!(te_flags&TXTED_ALLOW_EOL) || C_text_editor_common::IsReadOnly()){
               return EKeyWasNotConsumed;
            }
            break;
         case EKeyLeftArrow:
         case EKeyRightArrow:
            break;
         default:
            if(C_text_editor_common::IsReadOnly())
               return EKeyWasNotConsumed;
            {
               //Info("k", key_event.iCode);
            }
         }
         break;
      }
      return CEikEdwin::OfferKeyEventL(key_event, type);
   }
#endif//!DEBUG_EDWIN

//----------------------------
// Note: CASE_* maps to EAknEditor*Case defined in uikon.hrh
   virtual void SetCase(dword allowed, dword curr){

      if(allowed&CASE_CAPITAL)
         allowed = CASE_ALL;
      C_text_editor_common::SetCase(allowed, curr);
#ifdef S60
#ifndef DEBUG_EDWIN
      CAknEdwinState *state = GetState();
      if(state){
         state->SetPermittedCases(allowed);
         state->SetCurrentCase(curr);
         state->SetDefaultCase(curr);
      }
#endif
#endif//S60
   }

//----------------------------

   virtual void Draw(const TRect &rc) const{
      CWindowGc &gc = SystemGc();
      gc.SetPenColor(KRgbBlack);
      gc.SetBrushStyle(CGraphicsContext::ENullBrush);
      TRect rect = rc;
      rect.Grow(1, 1);
      gc.DrawRect(rect);
      CEikEdwin::Draw(rc);
   }

//----------------------------

public:
   C_text_editor_Symbian(C_application_ui *app, dword tef, int fi, dword ff, dword ml, C_text_editor::C_text_editor_notify *_not):
      C_text_editor_common(app, tef, fi, ff, _not),
      fte(NULL),
      activated(false),
      text_dirty(false),
      cursor_moved(false),
      inline_len(0),
      inline_pos(-1)
   {
      max_len = ml;
   }
   ~C_text_editor_Symbian(){
      Activate(false);
      //CCoeEnv::Static()->Fep()->HandleLosingForeground();
   }

//----------------------------

   void ConstructL(){
                              //install the Symbian control, so that Windows Server send it keypresses (must do so before CEikEdwin::ConstructL)
#ifdef DEBUG_EDWIN_
      CEikGlobalTextEditor::SetContainerWindowL(*app->GetIGraph()->GetCoeControl());
      CEikGlobalTextEditor::ConstructL(app->GetIGraph()->GetCoeControl(),
         (te_flags&TXTED_ALLOW_EOL) ? 2 : 1,
         100,
         CEikEdwin::EWidthInPixels |
         CEikEdwin::ENoAutoSelection |
         //CEikEdwin::EOnlyASCIIChars |
         //CEikEdwin::EResizable |
         CEikEdwin::ENoWrap |
         CEikEdwin::EAllowUndo |
         //CEikEdwin::EAvkonEditor |
         ((te_flags&TXTED_ANSI_ONLY) ? CEikEdwin::EOnlyASCIIChars : 0) |
         ((te_flags&TXTED_ALLOW_EOL) ? 0 : CEikEdwin::ENoLineOrParaBreaks) |
         0, 0, 0);
#else
      CEikEdwin::SetContainerWindowL(*app->GetIGraph()->GetCoeControl());
      CEikEdwin::ConstructL(
         CEikEdwin::EWidthInPixels |
         CEikEdwin::ENoAutoSelection |
         //CEikEdwin::EOnlyASCIIChars |
         //CEikEdwin::EResizable |
         CEikEdwin::ENoWrap |
         CEikEdwin::EAllowUndo |
         ((te_flags&TXTED_ANSI_ONLY) ? CEikEdwin::EOnlyASCIIChars : 0) |
         ((te_flags&TXTED_ALLOW_EOL) ? 0 : CEikEdwin::ENoLineOrParaBreaks) |
         0, 100, 40, (te_flags&TXTED_ALLOW_EOL) ? 2 : 1);
#endif
      if(te_flags&TXTED_ANSI_ONLY)
         CEikEdwin::SetOnlyASCIIChars(true);
      if(te_flags&TXTED_READ_ONLY)
         CEikEdwin::SetReadOnly(true);
      CEikEdwin::SetTextLimit(GetMaxLength());
#ifdef S60
#ifndef DEBUG_EDWIN
      InputCapabilities();
      CAknEdwinState *state = GetState();
      if(state){
         if(te_flags&TXTED_NUMERIC){
            state->SetPermittedInputModes(EAknEditorNumericInputMode);
            state->SetCurrentInputMode(EAknEditorNumericInputMode);
         }else
         if(!(te_flags&TXTED_ALLOW_PREDICTIVE))
            state->SetFlags(EAknEditorFlagNoT9);
      }
#endif
#endif//S60

      //CEikEdwin::SetContainerWindowL(*app->GetIGraph()->GetCoeControl());
//#if defined _DEBUG && 1
#ifdef DEBUG_EDWIN
      CEikEdwin::SetRect(TRect(0, 0, 176, 64));
      CEikEdwin::SetParent(app->GetIGraph()->GetCoeControl());
#else
      CEikEdwin::SetEdwinObserver(this);
      CEikEdwin::SetRect(TRect(0, 1000, 20, 1020));
#endif
      if(te_flags&TXTED_REAL_VISIBLE){
         //int twp = iEikonEnv->ScreenDevice()->VerticalPixelsToTwips(app->font_defs[font_index].cell_size_y);

         CFont *fnt = (CFont*)app->system_font.font_handle[0][0][font_index];

         TCharFormat cf;//(_L("Arial"), twp);
         //CFont *fnt = (CFont*)CEikonEnv::Static()->DenseFont();
         cf.iFontSpec = fnt->FontSpecInTwips();

         //cf.iFontSpec.iFontStyle.SetBitmapType(EAntiAliasedGlyphBitmap);
         //cf.iFontSpec.iFontStyle.SetPosture(EPostureUpright);
         TCharFormatMask cfm;
         cfm.SetAttrib(EAttFontTypeface);
         cfm.SetAttrib(EAttFontHeight);
         cfm.SetAttrib(EAttFontPosture);

         CCharFormatLayer* cCharFL = CCharFormatLayer::NewL(cf, cfm);
         SetCharFormatLayer(cCharFL);

         //SetBorder(TGulBorder::EFlat | TGulBorder::EOneStep | TGulBorder::EAddOnePixel);
      }
      CEikEdwin::SetMaximumHeight(20);

      if(!(te_flags&TXTED_CREATE_INACTIVE))
         Activate(true);
      SetCase(CASE_ALL, CASE_CAPITAL);
   }

//----------------------------

   virtual void SetRect(const S_rect &_rc){
      C_text_editor_common::SetRect(_rc);
      if(te_flags&TXTED_REAL_VISIBLE){
         //Fatal("sy", rc.sy);
         CEikEdwin::SetMinimumHeight(rc.sy);
         CEikEdwin::SetRect(TRect(rc.x, rc.y-0, rc.Right(), rc.Bottom()));
      }
   }

//----------------------------

   virtual void InvokeInputDialog(){
#if defined S60 && defined __SYMBIAN_3RD__
                              //simulate pen down/up on same position; used on S60 5th ed. to invoke text input dialog
      int pos = cursor_pos, sel = cursor_sel;

      TPointerEvent pe = { TPointerEvent::EButton1Down };
      pe.iPosition = Rect().iTl;
      pe.iPosition += TPoint(10, 10);
      HandlePointerEventL(pe);
      //Fatal("pos", cursor_pos);
      SetCursorPos(pos, sel);
      pe.iType = pe.EButton1Up; HandlePointerEventL(pe);
      cursor_moved = true;
#endif
   }

//----------------------------

   void SetEdwinText(){

      CancelFepTransaction();
      const wchar *wp = text.begin();
                              //convert new-lines
      int len = text.size()-1;

      wchar *buf = new(true) wchar[len];
      for(int i=len; i--; ){
         wchar c = wp[i];
         switch(c){
         case '\n':
            c = 0x2029;
            break;
         }
         buf[i] = c;
      }
      TPtrC desc((word*)buf, len);
      CEikEdwin::SetTextL(&desc);
      delete[] buf;
   }

//----------------------------

   virtual void SetInitText(const Cstr_w &init_text){

#ifdef S60
      if(text.size()>1 && activated){
         ResetFep();
                              //this is needed to reset fep's state, otherwise in some fep transactions it can crash app
         //CEikEdwin::SetFocus(false);
         //CEikEdwin::SetFocus(true);
      }
#endif
      C_text_editor_common::SetInitText(init_text);
      if(!mask_password){
         SetEdwinText();
         CEikEdwin::SetSelectionL(cursor_pos, cursor_sel);
      }
      cursor_on = true;
   }

//----------------------------

   virtual void ReplaceSelection(const wchar *txt){
      C_text_editor_common::ReplaceSelection(txt);
      int pos = cursor_pos;
      SetEdwinText();
      SetCursorPos(pos, pos);
      cursor_on = true;
   }

//----------------------------

   inline int GetInlineTextLen() const{ return inline_len; }
   inline int GetInlineTextPos() const{ return inline_pos; }

//----------------------------

   virtual void Cut(){
      if(!IsSecret()){
         CEikEdwin::ClipboardL(ECut);
         CorrectText();
      }
   }
//----------------------------
   virtual void Copy(){
      if(!IsSecret())
         CEikEdwin::ClipboardL(ECopy);
   }
//----------------------------
   virtual bool CanPaste() const{
      if(mask_password)
         return false;
      return CcpuCanPaste();
   }
//----------------------------
   virtual void Paste(){
      if(CanPaste()){
         /*
         class C_lf: public C_leave_func{
            C_text_editor_Symbian &ted;
            virtual int Run(){
               ted.ClipboardL(EPaste);
               return 0;
            }
         public:
            C_lf(C_text_editor_Symbian &_t):
               ted(_t)
            {}
         } lf(*this);
         int err = lf.Execute();
         //Info("e", err);
         */
         ClipboardL(EPaste);
         CorrectText();
      }
   }
};

//----------------------------

C_text_editor *C_application_ui::CreateTextEditorInternal(dword flags, int font_index, dword font_flags, const wchar *init_text, dword max_length, C_text_editor::C_text_editor_notify *notify){

   C_text_editor_Symbian *tp = new(true) C_text_editor_Symbian(this, flags, font_index, font_flags, max_length, notify);
   tp->ConstructL();
   if(init_text)
      tp->SetInitText(init_text);
   return tp;
}

//----------------------------

void C_application_ui::DrawTextEditorState(const C_text_editor &te, dword &soft_button_menu_txt_id, dword &soft_button_secondary_txt_id, int ty){

   if(!text_edit_state_icons){
      C_image *img = C_image::Create(*this);
      text_edit_state_icons = img;
      img->Open(L"txt_icons.png", 0, fdb.cell_size_y+1, C_fixed::One(), CreateDtaFile());
      img->Release();
   }

   dword color = GetColor(ty==-1 ? COL_EDITING_STATE : COL_TEXT_POPUP);

   int x = ScrnSX()/2;
   int y = (ty!=-1) ? ty : (ScrnSY()-fdb.line_spacing+1);
   const wchar *cp = NULL;
   dword fflg = FF_ITALIC;

   S_rect rc_icon(0, 0, 0, text_edit_state_icons->SizeY());
   S_rect rc_icon1 = rc_icon;
#if defined __SYMBIAN32__ && defined S60
   MAknEditingStateIndicator *ei = CAknEnv::Static()->EditingStateIndicator();
   if(!ei)
      return;
   CAknIndicatorContainer *ic = ei->IndicatorContainer();
   if(!ic)
      return;

   if(ic->IndicatorState(S_uid(EAknNaviPaneEditorIndicatorT9))
      || ic->IndicatorState(S_uid(113))   //E71 autocomplete
      ){
      rc_icon.sx = rc_icon.sy*2;
      rc_icon.x += rc_icon.sx;
   }else
   if(ic->IndicatorState(S_uid(EAknNaviPaneEditorIndicatorQuery))){
      rc_icon.sx = rc_icon.sy*2;
   }
   enum{
      EAknNaviPaneEditorIndicatorFnKeyPressed = 500,
      EAknNaviPaneEditorIndicatorFnKeyLocked,
   };

   if(ic->IndicatorState(S_uid(114))) cp = L"Ctrl";
   else if(ic->IndicatorState(S_uid(EAknNaviPaneEditorIndicatorLowerCase))) cp = L"abc";
   else if(ic->IndicatorState(S_uid(EAknNaviPaneEditorIndicatorUpperCase))) cp = L"ABC";
   else if(ic->IndicatorState(S_uid(EAknNaviPaneEditorIndicatorTextCase))) cp = L"Abc";
   else if(ic->IndicatorState(S_uid(EAknNaviPaneEditorIndicatorNumberCase))) cp = L"123";
   else if(ic->IndicatorState(S_uid(EAknNaviPaneEditorIndicatorFnKeyPressed))){ cp = L"123"; rc_icon.sx = rc_icon.sy; rc_icon.x = rc_icon.sx*5; }
   else if(ic->IndicatorState(S_uid(EAknNaviPaneEditorIndicatorFnKeyLocked))){ cp = L"123"; rc_icon.sx = rc_icon.sy; rc_icon.x = rc_icon.sx*6; fflg |= FF_UNDERLINE; }
   else if(ic->IndicatorState(S_uid(EAknNaviPaneEditorIndicatorPinyin))) cp = L"\x62FC\x97F3";
   else if(ic->IndicatorState(S_uid(EAknNaviPaneEditorIndicatorZhuyin))) cp = L"\x6CE8\x97F3";
   else if(ic->IndicatorState(S_uid(EAknNaviPaneEditorIndicatorStroke))) cp = L"\x7B14\x753B";   //BiHua
#ifdef __SYMBIAN_3RD__
   else if(ic->IndicatorState(S_uid(EAknNaviPaneEditorIndicatorQwertyShift))){ rc_icon1.sx = rc_icon1.sy; rc_icon1.x = rc_icon1.sx*4; }
   else if(ic->IndicatorState(S_uid(EAknNaviPaneEditorIndicatorQwertyShift+1))){ rc_icon1.sx = rc_icon1.sy; rc_icon1.x = rc_icon1.sx*5; }
   else if(ic->IndicatorState(S_uid(EAknNaviPaneEditorIndicatorQwertyShift+2))){ rc_icon1.sx = rc_icon1.sy; rc_icon1.x = rc_icon1.sx*6; }
#endif

#ifndef DEBUG_EDWIN
                              //determine modified soft keys
   C_text_editor_Symbian &te_symb = (C_text_editor_Symbian&)te;
   CAknEdwinState *st = te_symb.GetState();
   if(st){
      CAknEdwinState::SEditorCbaState &cs = st->CbaState();
      switch(cs.iLeftSoftkeyCommandId){
      case 0x4001:
         soft_button_menu_txt_id = SPECIAL_TEXT_SPELL;
         break;
      default: assert(0);
      case 0: break;
      }
      switch(cs.iRightSoftkeyCommandId){
      case 0x4000:
         soft_button_secondary_txt_id = SPECIAL_TEXT_PREVIOUS;
         break;
      default: assert(0);
      case 0: break;
      }
   }
#endif
#endif
   if(cp){
      DrawString(cp, x, y, UI_FONT_BIG, fflg, color);
   }else
   if(rc_icon1.sx)
      text_edit_state_icons->DrawSpecial(x, y, &rc_icon1, C_fixed::One(), color);
   if(rc_icon.sx)
      text_edit_state_icons->DrawSpecial(x-2-rc_icon.sx, y, &rc_icon, C_fixed::One(), color);
}

//----------------------------

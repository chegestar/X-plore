#ifdef __SYMBIAN32__
#include <e32std.h>
#endif
#include "SimpleSoundPlayer.h"
#include <Fixed.h>
#include <Util.h>

//#define WCE_USE_PLAYSOUND

//----------------------------
#ifdef __SYMBIAN32__
#include <MdaAudioSamplePlayer.h>
#ifndef __SYMBIAN_3RD__
#define USE_OPTIONAL_POSITIONING
#endif

class C_simple_sound_player_imp: public C_simple_sound_player, public MMdaAudioPlayerCallback{
   CMdaAudioPlayerUtility *plr;
   bool inited, finished;
   dword init_volume;

#ifdef USE_OPTIONAL_POSITIONING
   RLibrary lib_mca;
   bool lib_loaded;
#ifdef _DEBUG
   typedef TInt (CMdaAudioPlayerUtility::*t_Pause)();
   typedef TInt (CMdaAudioPlayerUtility::*t_GetPosition)(TTimeIntervalMicroSeconds& aPosition);
   typedef void (CMdaAudioPlayerUtility::*t_SetPosition)(const TTimeIntervalMicroSeconds& aPosition);
#else
   typedef TInt (*t_Pause)(CMdaAudioPlayerUtility*);
   typedef TInt (*t_GetPosition)(CMdaAudioPlayerUtility*, TTimeIntervalMicroSeconds&);
   typedef void (*t_SetPosition)(CMdaAudioPlayerUtility*, const TTimeIntervalMicroSeconds& aPosition);
#endif
   t_Pause mca_Pause;
   t_GetPosition mca_GetPosition;
   t_SetPosition mca_SetPosition;

   void InitLib(){
      if(!lib_loaded){
         mca_Pause = NULL;
         mca_GetPosition = NULL;
         mca_SetPosition = NULL;
         const TUid ss_dll_uid = { 0x1000008d };
         const TUidType uid(KNullUid, ss_dll_uid, KNullUid);
         if(!lib_mca.Load(_L("mediaclientaudio.dll"), uid)){
            lib_loaded = true;
#ifdef _DEBUG
            *(TLibraryFunction*)&mca_Pause = lib_mca.Lookup(55);
            *(TLibraryFunction*)&mca_GetPosition = lib_mca.Lookup(30);
            *(TLibraryFunction*)&mca_SetPosition = lib_mca.Lookup(72);
#else
            mca_Pause = (t_Pause)lib_mca.Lookup(54);
            mca_GetPosition = (t_GetPosition)lib_mca.Lookup(29);
            mca_SetPosition = (t_SetPosition)lib_mca.Lookup(71);
#endif
         }
      }
   }
#endif

//----------------------------
   //CActiveSchedulerWait *asw;

   virtual void MapcInitComplete(TInt err, const TTimeIntervalMicroSeconds &aDuration){
      //Fatal("MapcInitComplete", err);
      if(err){
         finished = true;
      }else{
         inited = true;
         SetVolume(init_volume);
         plr->Play();
      }
      //if(asw) asw->AsyncStop();
   }

//----------------------------

   virtual void MapcPlayComplete(TInt err){
      finished = true;
   }

//----------------------------
public:
   C_simple_sound_player_imp(int vol):
      plr(NULL),
#ifdef USE_OPTIONAL_POSITIONING
      lib_loaded(false),
#endif
      //asw(NULL),
      inited(false),
      finished(false),
      init_volume(vol)
   {}
   ~C_simple_sound_player_imp(){
      //delete asw;
      delete plr;
#ifdef USE_OPTIONAL_POSITIONING
      if(lib_loaded)
         lib_mca.Close();
#endif
   }

//----------------------------

   bool Construct(const wchar *fname){

      //asw = new CActiveSchedulerWait;
      TRAPD(err, plr = CMdaAudioPlayerUtility::NewFilePlayerL(TPtrC((word*)fname, StrLen(fname)), *this));
      //asw->Start();
      //delete asw; asw = NULL;
      //if(!inited)
         //return false;
      //Fatal("Construct", err);
      return (!err);
   }

//----------------------------

   virtual bool IsDone() const{ return finished; }

//----------------------------

   virtual dword GetPlayTime() const{
      if(!inited)
         return 0;
#ifdef __SYMBIAN_3RD__
      return dword(plr->Duration().Int64()/TInt64(1000));
#else
      return (plr->Duration().Int64()/TInt64(1000)).Low();
#endif
   }

//----------------------------

   virtual bool HasPositioning() const{
#ifdef USE_OPTIONAL_POSITIONING
      const_cast<C_simple_sound_player_imp*>(this)->InitLib();
      return (mca_Pause!=NULL);
#else
      return true;
#endif
   }

//----------------------------

   virtual dword GetPlayPos() const{

      if(!inited)
         return 0;
      if(finished)
         return GetPlayTime();
      dword ret;
#ifdef USE_OPTIONAL_POSITIONING
      const_cast<C_simple_sound_player_imp*>(this)->InitLib();
      if(mca_GetPosition){
         TTimeIntervalMicroSeconds p;
#ifdef _DEBUG
         (plr->*mca_GetPosition)(p);
#else
         mca_GetPosition(plr, p);
#endif
         ret = (p.Int64()/TInt64(1000)).Low();
      }else
         ret = 0;
#else
      TTimeIntervalMicroSeconds p;
      plr->GetPosition(p);
      ret = dword(p.Int64()/TInt64(1000));
#endif
      return Min(ret, GetPlayTime());
   }

//----------------------------

   virtual void SetPlayPos(dword pos){
      if(inited){
         TTimeIntervalMicroSeconds p(TUint(pos*1000));
#ifdef USE_OPTIONAL_POSITIONING
         InitLib();
         if(mca_SetPosition){
#ifdef _DEBUG
            (plr->*mca_SetPosition)(p);
#else
            mca_SetPosition(plr, p);
#endif
         }
#else
         plr->SetPosition(p);
#endif
      }
   }

//----------------------------

   virtual void SetVolume(int vol){
      if(inited){
         dword v = (plr->MaxVolume() * vol) / 10;
         plr->SetVolume(v);
      }else
         init_volume = vol;
   }

//----------------------------
   virtual void Pause(){
      if(inited){
#ifdef USE_OPTIONAL_POSITIONING
         InitLib();
         if(mca_Pause){
#ifdef _DEBUG
            (plr->*mca_Pause)();
#else
            mca_Pause(plr);
#endif
         }
#else
         plr->Pause();
#endif
      }
   }

//----------------------------

   virtual void Resume(){
      if(inited){
         plr->Play();
         finished = false;
      }
   }
};

//----------------------------

C_simple_sound_player *C_simple_sound_player::Create(const wchar *fn, int vol){

   C_simple_sound_player_imp *plr = new(true) C_simple_sound_player_imp(vol);
   if(!plr->Construct(fn)){
      delete plr;
      plr = NULL;
   }
   return plr;
}

#endif
//----------------------------
#if defined _WINDOWS || defined _WIN32_WCE
#ifndef WCE_USE_PLAYSOUND
#include <SndPlayer.h>
#include <C_file.h>
#endif

class C_simple_sound_player_imp: public C_simple_sound_player{
#ifndef WCE_USE_PLAYSOUND
   C_sound_player *plr;
   C_sound *snd;
#endif
   int volume;                //0-10
   mutable dword end_play_time;       //time when we noticed that sound ended playing

//----------------------------

public:
   C_simple_sound_player_imp():
#ifndef WCE_USE_PLAYSOUND
      plr(NULL),
      snd(NULL),
#endif
      end_play_time(0)
   {}
   ~C_simple_sound_player_imp(){
#ifndef WCE_USE_PLAYSOUND
      if(snd)
         snd->Release();
      if(plr)
         plr->Release();
#endif
   }
   bool Construct(const wchar *fname, int vol){

#ifdef WCE_USE_PLAYSOUND
#else
      plr = CreateSoundPlayer(22050, 2);
      if(!plr)
         return false;
      C_sound_source *src = plr->CreateSoundSource();
      if(!src->Open(fname)){
         src->Release();
         return false;
      }
      snd = plr->CreateSound();
      snd->Open(src);
      src->Release();

      SetVolume(vol);
      snd->Play();
#endif
      return true;
   }

   virtual bool IsDone() const{
      if(!snd->IsPlaying() && GetPlayPos()==GetPlayTime()){
                              //finish with some delay, so that hw has time to play the sound
         if(!end_play_time)
            end_play_time = GetTickTime();
         if(GetTickTime()-end_play_time > 1000)
            return true;
      }
      return false;
   }
   virtual dword GetPlayTime() const{
      return snd->GetPlayTime();
   }
   virtual dword GetPlayPos() const{
      return snd->GetPlayPos();
   }
   virtual void SetPlayPos(dword p){
      snd->SetPlayPos(p);
   }
   virtual void SetVolume(int vol){
      snd->SetVolume(C_fixed(vol)*C_fixed::Percent(10));
   }
   virtual void Pause(){
      snd->Stop();
   }
   virtual void Resume(){
      snd->Play();
   }
   virtual bool HasPositioning() const{ return true; }
};

//----------------------------

C_simple_sound_player *C_simple_sound_player::Create(const wchar *fn, int vol){

   C_simple_sound_player_imp *plr = new(true) C_simple_sound_player_imp;
   if(!plr->Construct(fn, vol)){
      delete plr;
      plr = NULL;
   }
   //i
   //PlaySound(fn, NULL, SND_FILENAME | SND_ASYNC | SND_NOWAIT);
   return plr;
}

#endif
//----------------------------
//----------------------------

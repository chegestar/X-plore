//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <SndPlayer.h>
#include <mdaaudiooutputstream.h>
#include <mda\common\audio.h>
#include <Util.h>
#if defined __SYMBIAN_3RD__ && !defined __WINS__
#define USE_MMF
#include "mmf\Server\SoundDevice.h"
#else
#include <Hal.h>
#endif

#define USE_THREAD

//----------------------------
//#define USE_LOG               //!!!
//#define STRICT_CHECK          //show fatal error when unexpected errors occurs

#ifdef USE_LOG
#include <LogMan.h>
#include <Cstr.h>
#include <C_file.h>
#ifdef USE_THREAD
#define LOG(s) log.Add(s)
#define LOG_N(s, n) { Cstr_c t; t<<s <<':' <<n; log.Add(t); }
#else
#define LOG(s) _log.Add(s)
#define LOG_N(s, n) { Cstr_c t; t<<s <<':' <<n; _log.Add(t); }
#endif
#define _LOG(s) _log.Add(s)
#define _LOG_N(s, n) { Cstr_c t; t<<s <<':' <<n; _log.Add(t); }
#else
#define LOG(s)
#define LOG_N(s, n)
#define _LOG(s)
#define _LOG_N(s, n)
#endif


#ifdef _DEBUG
//#define DEBUG_PLAY_BUFFER_TICKS  //insert ticks into sound playback buffer in each Tick
#endif

//----------------------------
//----------------------------

class C_sound_player_imp: public C_sound_player{
   dword desired_frequency, desired_num_channels;
#ifdef USE_LOG
   C_logman _log;
#endif

//----------------------------

   class C_stream_thread_base{
   protected:
      dword init_flags;
      dword frequency, num_channels;

#ifdef USE_THREAD
      RThread thrd;
      bool thread_running;
      bool quit_request;
#endif
      int max_vol;
      dword curr_hw_volume, new_hw_volume;    //0 - max_vol; -1 = not set

      dword buffer_size;      //number of 16-bit values

   //----------------------------
#ifdef USE_THREAD
                              //override Error method of active scheduler
      class C_active_scheduler: public CActiveScheduler{

                              //this method is called when RunL method of scheduled object leaves
         virtual void Error(TInt err) const{
#ifdef STRICT_CHECK
            Fatal("AS err", err);
#endif
            Stop();
         }
      };
#endif
   //----------------------------

   public:
#ifdef USE_LOG
#ifdef USE_THREAD
      C_logman log;
#endif
      C_logman _log;
      bool buf_size_reported;
#endif
#ifdef USE_THREAD
      RSemaphore semaphore;
#endif
                              //linked list of sound writers
      C_sound_writer *first_sound_writer;
      dword num_sound_writers;

      C_stream_thread_base():
#ifdef USE_THREAD
         thread_running(false),
         quit_request(false),
#endif
         first_sound_writer(NULL), num_sound_writers(0),
         max_vol(0),
         curr_hw_volume(0xffffffff),
         new_hw_volume(0xffffffff),
         num_channels(0),
         frequency(0),
         buffer_size(0)
      {
#ifdef USE_THREAD
         semaphore.CreateLocal(1);
#endif
      }

      ~C_stream_thread_base(){
#ifdef USE_THREAD
         semaphore.Close();
#endif
         _LOG("~");
      }

   //----------------------------

      void Construct(dword flags){
         init_flags = flags;
         _LOG("Constructed");
      }

   //----------------------------

      void SetVolume(C_fixed vol){
         vol = C_fixed::Create(max_vol<<8) * vol;
                              //clamp up
         new_hw_volume = Min((vol.val+128) >> 8, max_vol);
      }

      dword GetMaxHardwareVolume() const{ return max_vol; }

      inline void SetHardwareVolume(dword val){ new_hw_volume = val; }
      inline dword GetHardwareVolume() const{ return new_hw_volume; }

      inline dword GetnumChannels() const{ return num_channels; }
      inline dword GetPlaybackRate() const{ return frequency; }

      inline dword GetBufSize() const{ return buffer_size; }
   };


#ifdef USE_MMF
//----------------------------

   class C_stream_thread: public C_stream_thread_base, public MDevSoundObserver{
      CMMFDevSound *dev_snd;
      enum{
         MODE_CLOSED,
         MODE_ACTIVATING,
         MODE_PLAYING,
      } mode;

   //----------------------------

      static TMMFSampleRate GetSampleRateFlags(dword freq){

         switch(freq){
         case 8000: return EMMFSampleRate8000Hz;
         case 11025: return EMMFSampleRate11025Hz;
         case 12000: return EMMFSampleRate12000Hz;
         case 16000: return EMMFSampleRate16000Hz;
         default:
         case 22050: return EMMFSampleRate22050Hz;
         case 24000: return EMMFSampleRate24000Hz;
         case 32000: return EMMFSampleRate32000Hz;
         case 44100: return EMMFSampleRate44100Hz;
         case 48000: return EMMFSampleRate48000Hz;
         }
      }

   //----------------------------

      virtual void ToneFinished(TInt err){}
      virtual void BufferToBeEmptied(CMMFBuffer *aBuffer){}
      virtual void RecordError(TInt err){}
      virtual void ConvertError(TInt err){}

   //----------------------------

      virtual void DeviceMessage(TUid aMessageType, const TDesC8 &aMsg){
         LOG_N("DeviceMessage: ", (int)aMessageType.iUid);
#ifdef STRICT_CHECK
         Fatal("DeviceMessage", aMessageType.iUid);
#endif
      }

   //----------------------------

      virtual void InitializeComplete(TInt err){

         __ASSERT_DEBUG(mode==MODE_ACTIVATING, Fatal("ISndDrv", 0));
         LOG("InitializeComplete");
         if(err)
            Fatal("Snd init err", err);
#ifdef USE_THREAD
         TMMFCapabilities caps;
         caps = dev_snd->Capabilities();

         caps.iEncoding = EMMFSoundEncoding16BitPCM;
                              //determine mono/stereo
         if(num_channels==1){
            caps.iChannels &= EMMFMono;
            if(!caps.iChannels)
               caps.iChannels = EMMFStereo;
         }else{
            caps.iChannels &= EMMFStereo;
            if(!caps.iChannels)
               caps.iChannels = EMMFMono;
         }
                              //determine desired buffer size
         const dword bufs_per_sec =
            (init_flags&SND_INIT_LOW_DELAY) ? 25 :
               3*num_channels;   //2010-12-27: changed from 4 to 3, since users of E72 reported cracks in music player
         caps.iBufferSize = frequency * sizeof(short) * num_channels / bufs_per_sec;
         caps.iBufferSize &= -64;
         LOG_N(" desired buffer size", caps.iBufferSize);
         buffer_size = caps.iBufferSize/2;
                              //determine frequency
                           //determine which frequency pattern we'll try to init
         static const struct S_freq_pattern{
            dword freq[7];
         } freq_patterns[] = {
            {8000, 11025, 16000, 22050, 32000, 44100},
            {11025, 16000, 8000, 22050, 32000, 44100},
            {16000, 22050, 11025, 8000, 32000, 44100},
            {22050, 16000, 11025, 8000, 32000, 44100},
            {32000, 22050, 16000, 11025, 8000, 44100},
            {44100, 32000, 22050, 48000, 16000, 11025},
            {48000, 44100, 32000, 22050, 16000, 11025},
            {0}
         };
         const S_freq_pattern *fp;
         for(fp = freq_patterns; *fp->freq; fp++){
            if(*fp->freq==frequency)
               break;
         }
         if(!*fp->freq)
            Fatal("SND no freq");
                              //find first supported frequency in the pattern
         frequency = 0;
         for(const dword *fpp = fp->freq; *fpp; ++fpp){
            dword flg = GetSampleRateFlags(*fpp);
            if(caps.iRate&flg){
               caps.iRate = flg;
               frequency = *fpp;
               break;
            }
         }
         dev_snd->SetConfigL(caps);
         LOG(" config set");

         max_vol = dev_snd->MaxVolume();
         LOG_N("Max hw volume", max_vol);
                              //reset hw vol to max initially
         if(new_hw_volume==0xffffffff)
            new_hw_volume = max_vol;

         /*
         TMMFPrioritySettings pset;
         pset.iPriority = 0;
         pset.iPref = EMdaPriorityPreferenceTime;
         pset.iState = EMMFStatePlaying;
         dev_snd->SetPrioritySettings(pset);
         */
#else
         dev_snd->SetConfigL(caps);
         LOG(" config set");
#endif

         curr_hw_volume = 0xffffffff;  //force volume re-apply, because our stream's volume is initially unspecified

         dev_snd->PlayInitL();
         LOG(" playback inited");
      }

   //----------------------------

      virtual void BufferToBeFilled(CMMFBuffer *buf){

#ifdef USE_THREAD
         if(quit_request){
            LOG("BufCopy - quit request");
            CActiveScheduler::Stop();
            LOG("AS Stop");
         }else
#endif
         {
            mode = MODE_PLAYING;
            int req_size = buf->RequestSize();
            buffer_size = req_size/sizeof(word);
#ifdef USE_LOG
            if(!buf_size_reported){
               buf_size_reported = true;
               LOG_N("Buffer size vals", buffer_size);
            }
#endif
            TDes8 &data = ((CMMFDataBuffer*)buf)->Data();
            data.SetLength(req_size);
            UpdateBuffer((short*)data.Ptr(), req_size/2);
            dev_snd->PlayData();
            ApplyVolume();
         }
      }

   //----------------------------

      virtual void PlayError(TInt err){

         LOG_N("PlayError", err);
#ifdef STRICT_CHECK
         Fatal("PlayError", err);
#endif
         switch(err){
         case KErrUnderflow:
                              //simply restart playback
            dev_snd->Stop();
            //curr_hw_volume = 0xffffffff;
            dev_snd->PlayInitL();
            LOG("Playback re-inited");
            break;

         default:
                              //after this our app can't seem to use this active scheduler anymore, just exit from it and request restart
            LOG("trying to restart");
#ifndef USE_THREAD
            CActiveScheduler *as = CActiveScheduler::Current();
            as->Stop();
            Stop();
#endif
                              //1 sec pause
            User::After(1000*1000);
#ifdef USE_THREAD
            CActiveScheduler::Stop();
            LOG("AS Stop");
#else
            Start(frequency, num_channels);
            as->Start();
            LOG("restarted");
#endif
            break;
         }
      }

#ifdef USE_THREAD
   //----------------------------
   // Stream thread proc.
      void ThreadProc(){

         thread_running = true;
         LOG("ThrdProc start");

#ifdef __SYMBIAN_3RD__
         this_static = this;
         User::SetExceptionHandler(ExceptionHandler, 0xffffffff);
#else
         {
            Dll::SetTls(this);
            RThread self;
            self.SetExceptionHandler(ExceptionHandler, 0xffffffff);
         }
#endif
#ifdef USE_LOG
         buf_size_reported = false;
#endif
         LOG_N("Desired freq", frequency);
         LOG_N("Desired chn", num_channels);

         mode = MODE_ACTIVATING;
         while(!quit_request){
            C_active_scheduler as;

            LOG("Installing AS");
            CActiveScheduler::Install(&as);

            LOG("CMMFDevSound::NewL");
            dev_snd = CMMFDevSound::NewL();

            LOG("Initializing dev");
            dev_snd->InitializeL(*this, EMMFStatePlaying);

            LOG("AS start");
            CActiveScheduler::Start();
            LOG("AS done");
            if(dev_snd){
               dev_snd->Stop();
               delete dev_snd;
               dev_snd = NULL;
               LOG("Deleted dev_snd");
            }
            CActiveScheduler::Install(NULL);
         }
         mode = MODE_CLOSED;
#ifdef __SYMBIAN_3RD__
         this_static = NULL;
#else
         Dll::SetTls(NULL);
#endif
         LOG("ThrdProc End");
         thread_running = false;
      }

   //----------------------------

      static TInt ThreadProc(TAny *aPtr){
         C_stream_thread *_this = (C_stream_thread*)aPtr;
#ifdef USE_LOG
         _this->log.Open(L"C:\\Isound.log", false);
         _this->log.Add("Snd thrd started");
#endif
         CTrapCleanup *tc = CTrapCleanup::New();
         TRAPD(err, _this->ThreadProc());
#ifdef USE_LOG
         //_this->log.Add("Snd thrd end");
         _this->log.Close();
#endif
         delete tc;
         return 0;
      }

//----------------------------

#ifdef __SYMBIAN_3RD__
      static C_stream_thread *this_static;
#endif

      static void ExceptionHandler(TExcType type){

#ifdef STRICT_CHECK
         Fatal("Exception", type);
#endif
#ifdef __SYMBIAN_3RD__
         C_stream_thread *_this = this_static;
#else
         C_stream_thread *_this = (C_stream_thread*)Dll::Tls();
#endif
#ifdef USE_LOG
         Cstr_c s; s<<"Exception " <<int(type);
         _this->log.Add(s);
#endif
         if(_this->dev_snd){
            _this->dev_snd->Stop();
            delete _this->dev_snd;
            _this->dev_snd = NULL;
         }
#ifdef USE_THREAD
         CActiveScheduler::Stop();
         _this->thread_running = false;
#endif
         _this->mode = MODE_CLOSED;
         User::Exit(type);
      }

#endif

//----------------------------

      void ApplyVolume(){
                              //don't apply always, as it causes breaks in audio on some hardware
         if(new_hw_volume!=0xffffffff && curr_hw_volume!=new_hw_volume){
            if(curr_hw_volume!=new_hw_volume)
               LOG_N("Update hw volume", new_hw_volume);
            dev_snd->SetVolume(new_hw_volume);
            curr_hw_volume = new_hw_volume;
         }
      }

//----------------------------
#ifdef USE_LOG
      dword last_t;
#endif

      void UpdateBuffer(short *sound_buf, dword buffer_size){

#if defined USE_LOG && 0
         {
            dword tm = GetTickTime();
            //dword tm = dev_snd->SamplesPlayed();
            LOG_N("UpdateBuffer", tm-last_t);
            last_t = tm;
         }
#endif
#ifdef USE_THREAD
                              //sync access to sound writers array
         semaphore.Wait();
#endif

                              //clear the buffer first
         if(!num_sound_writers || !(init_flags&SND_INIT_NO_BUF_CLEAR))
            MemSet(sound_buf, 0, buffer_size*sizeof(short));

         //ApplyVolume();
         C_fixed vol(1);
         //if(num_sound_writers)
            //vol = vol / C_fixed((int)num_sound_writers);
                                 //mix in all played sounds
         for(C_sound_writer *sw=first_sound_writer; sw; ){
            C_sound_writer *next = C_sound_player_imp::GetNextWriter(sw);
            sw->MixIntoBuffer(sound_buf, buffer_size, vol);
            sw = next;
         }
#ifdef USE_THREAD
         semaphore.Signal();
#endif

#ifdef DEBUG_PLAY_BUFFER_TICKS
                                 //write debugging mark into buffer, so that we hear clicks when buffer is filled
         for(dword i=0; i<4; i++) sound_buf[i] = (i&1) ? 0 : 0x8000;
#endif
      }

//----------------------------
      TMMFCapabilities caps;

   public:
      C_stream_thread():
         dev_snd(NULL),
         mode(MODE_CLOSED)
      {
      }

   //----------------------------

      virtual ~C_stream_thread(){
         Stop();
         __ASSERT_DEBUG(mode==MODE_CLOSED, Fatal("no stop"));
      }

//----------------------------

      void Start(dword desired_frequency, dword desired_num_channels){

#ifdef USE_THREAD
         if(!thread_running){
            _LOG_N("Start", desired_frequency);
                              //set to desired values, real will be set when initializing
            frequency = desired_frequency;
            num_channels = desired_num_channels;
            quit_request = false;

            _LOG("Create thread");
            thrd.Create(_L("Sound streaming"), ThreadProc, 8192, NULL, this);
            _LOG(" created");
            thrd.SetPriority(EPriorityAbsoluteHigh);
            _LOG(" priority set");
            thrd.Resume();
            _LOG(" resumed");

                              //3 sec time-out
            int time_out = 300;
                              //wait for stream to start playback
            while(mode!=MODE_PLAYING){
               User::After(10*1000);   //10ms
                              //kill thread if it fails to response in 1 second
               if(!--time_out){
                  _LOG("No response, kill");
                  thrd.Kill(0);
                  thrd.Close();
                  mode = MODE_CLOSED;
                  thread_running = false;
                  break;
               }
            }
            _LOG(" ok");
         }
#else
         if(!dev_snd){
            frequency = desired_frequency;
            num_channels = desired_num_channels;

#ifdef USE_LOG
            buf_size_reported = false;
#endif
            LOG_N("Desired freq", frequency);
            LOG_N("Desired chn", num_channels);

            mode = MODE_ACTIVATING;

            LOG("CMMFDevSound::NewL");
            TRAPD(err, dev_snd = CMMFDevSound::NewL());

            caps = dev_snd->Capabilities();

            caps.iEncoding = EMMFSoundEncoding16BitPCM;
                                 //determine mono/stereo
            if(num_channels==1){
               caps.iChannels &= EMMFMono;
               if(!caps.iChannels)
                  caps.iChannels = EMMFStereo;
            }else{
               caps.iChannels &= EMMFStereo;
               if(!caps.iChannels)
                  caps.iChannels = EMMFMono;
            }
                                 //determine desired buffer size
            const dword bufs_per_sec = (init_flags&SND_INIT_LOW_DELAY) ? 25 : 4*num_channels;
            caps.iBufferSize = frequency * sizeof(short) * num_channels / bufs_per_sec;
            caps.iBufferSize &= -64;
            LOG_N(" desired buffer size", caps.iBufferSize);
            buffer_size = caps.iBufferSize/2;
                                 //determine frequency
                              //determine which frequency pattern we'll try to init
            static const struct S_freq_pattern{
               dword freq[7];
            } freq_patterns[] = {
               {8000, 11025, 16000, 22050, 32000, 44100},
               {11025, 16000, 8000, 22050, 32000, 44100},
               {16000, 22050, 11025, 8000, 32000, 44100},
               {22050, 16000, 11025, 8000, 32000, 44100},
               {32000, 22050, 16000, 11025, 8000, 44100},
               {44100, 32000, 22050, 48000, 16000, 11025},
               {48000, 44100, 32000, 22050, 16000, 11025},
               {0}
            };
            const S_freq_pattern *fp;
            for(fp = freq_patterns; *fp->freq; fp++){
               if(*fp->freq==frequency)
                  break;
            }
            if(!*fp->freq)
               Fatal("SND no freq");
                                 //find first supported frequency in the pattern
            frequency = 0;
            for(const dword *fpp = fp->freq; *fpp; ++fpp){
               dword flg = GetSampleRateFlags(*fpp);
               if(caps.iRate&flg){
                  caps.iRate = flg;
                  frequency = *fpp;
                  break;
               }
            }
            max_vol = dev_snd->MaxVolume();
            LOG_N("Max hw volume", max_vol);
                                 //reset hw vol to max initially
            if(new_hw_volume==0xffffffff)
               new_hw_volume = max_vol;

            LOG("Initializing dev");
            dev_snd->InitializeL(*this, EMMFStatePlaying);
         }
#endif
      }

   //----------------------------

      void Stop(){

#ifdef USE_THREAD
         if(thread_running){
            _LOG_N("Stop", GetTickTime());
            quit_request = true;
            int time_out = 0;
            _LOG("Set quit request");
            while(mode!=MODE_CLOSED){
               User::After(10*1000);
                              //kill thread if it fails to respond in 2 seconds
               if(++time_out == 200){
                  _LOG(" no response, kill");
                  thrd.Kill(0);
                  mode = MODE_CLOSED;
                  break;
               }
            }
            thrd.Close();
            _LOG("thread close");
            thread_running = false;
            _LOG("thread close");
         }
#else
         if(dev_snd){
            dev_snd->Stop();
            delete dev_snd;
            dev_snd = NULL;
            LOG("Deleted dev_snd");
         }
         mode = MODE_CLOSED;
#endif
      }

//----------------------------

      const short *GetBuf() const{ return NULL; }

      /*
      inline dword GetHardwarePlayPosition() const{
         if(!dev_snd)
            return 0xffffffff;
         return dev_snd->SamplesPlayed()*num_channels;
      }
      */

   } stream;

//----------------------------
#else//USE_MMF
//----------------------------

   class C_stream_thread: public C_stream_thread_base, public MMdaAudioOutputStreamCallback{
   private:
      CMdaAudioOutputStream *strm;
      TMdaAudioDataSettings settings;

      short *sound_buf;
      TPtrC8 buf_desc;

      enum{
         MODE_CLOSED,
         MODE_ACTIVATING,
         MODE_PLAYING,
      } mode;
      int upd_buf_check;

   //----------------------------

      static dword GetSampleRateFlags(dword freq){

         switch(freq){
         case 8000: return TMdaAudioDataSettings::ESampleRate8000Hz;
         case 11025: return TMdaAudioDataSettings::ESampleRate11025Hz;
         case 16000: return TMdaAudioDataSettings::ESampleRate16000Hz;
         case 22050: return TMdaAudioDataSettings::ESampleRate22050Hz;
         case 32000: return TMdaAudioDataSettings::ESampleRate32000Hz;
         case 44100: return TMdaAudioDataSettings::ESampleRate44100Hz;
         case 48000: return TMdaAudioDataSettings::ESampleRate48000Hz;
         }
         return 0;
      }

   //----------------------------

      void CreateStream(){

         strm = CMdaAudioOutputStream::NewL(*this);
#ifndef USE_THREAD
         //strm->SetPriority(EMdaPriorityNormal, EMdaPriorityPreferenceNone); //crashes with panic
         strm->SetPriority(EMdaPriorityMin, EMdaPriorityPreferenceNone);
                        //determine which frequency pattern we'll try to init
         static const struct S_freq_pattern{
            dword freq[7];
         } freq_patterns[] = {
            {8000, 11025, 16000, 22050, 32000, 44100},
            {11025, 16000, 8000, 22050, 32000, 44100},
            {16000, 22050, 32000, 44100},
            {22050, 16000, 32000, 44100},
            {32000, 22050, 16000, 44100},
            {44100, 32000, 22050, 16000},
            {0}
         };
         const S_freq_pattern *fp;
         for(fp = freq_patterns; *fp->freq; fp++){
            if(*fp->freq==frequency)
               break;
         }
         if(!*fp->freq)
            Fatal("SND no freq");

         frequency = 0;
         for( ; num_channels>0; num_channels--){
            settings.iChannels = num_channels;
            dword caps_flags = (num_channels==1 ? TMdaAudioDataSettings::EChannelsMono : TMdaAudioDataSettings::EChannelsStereo);
                        //find first supported frequency in the pattern
            const dword *fpp;
            for(fpp = fp->freq; *fpp; ++fpp){
               frequency = *fpp;
               dword sample_rate_flags = GetSampleRateFlags(frequency);

               settings.iCaps = sample_rate_flags | caps_flags;
               LOG_N(" try init Hz", frequency);
               int err = 0;
               TRAP(err, strm->SetAudioPropertiesL(sample_rate_flags, caps_flags));
               if(!err)
                  break;
            }
            if(*fpp)
               break;
         }
         if(!num_channels){
                        //error: no matching frequency found
            //Fatal("can't init sound");
            num_channels = 1;
            frequency = 22050;
         }
         dword bufs_per_sec = (init_flags&SND_INIT_LOW_DELAY) ? 25 : 16;
         buffer_size = frequency * num_channels / bufs_per_sec;
         buffer_size &= -16;
         delete[] sound_buf;
         sound_buf = new(true) short[buffer_size];
         buf_desc.Set((byte*)sound_buf, buffer_size*sizeof(short));
         LOG_N(" buffer size", buffer_size);

                              //note that MaxVolume() is different in the emulator and the real device!
         max_vol = strm->MaxVolume();
                        //set to max hw volume initially
         if(new_hw_volume==0xffffffff)
            new_hw_volume = max_vol;

         strm->SetPriority(EMdaPriorityMin, EMdaPriorityPreferenceNone);
         upd_buf_check = 2;

         mode = MODE_ACTIVATING;
         LOG("Opening strm");
         strm->Open(&settings);
#endif
      }

   //----------------------------

      virtual void MaoscOpenComplete(TInt err){

         switch(err){
         case KErrNone:
            {
#ifdef USE_THREAD
               LOG("Open completed OK");
               strm->SetPriority(EMdaPriorityMin, EMdaPriorityPreferenceNone);
               __ASSERT_DEBUG(mode==MODE_ACTIVATING, Fatal("ISndDrv", 0));

                              //determine which frequency pattern we'll try to init
               static const struct S_freq_pattern{
                  dword freq[7];
               } freq_patterns[] = {
                  {8000, 11025, 16000, 22050, 32000, 44100},
                  {11025, 16000, 8000, 22050, 32000, 44100},
                  {16000, 22050, 32000, 44100},
                  {22050, 16000, 32000, 44100},
                  {32000, 22050, 16000, 44100},
                  {44100, 32000, 22050, 48000},
                  {48000, 44100, 32000, 22050},
                  {0}
               };
               const S_freq_pattern *fp;
               for(fp = freq_patterns; *fp->freq; fp++){
                  if(*fp->freq==frequency)
                     break;
               }
               if(!*fp->freq)
                  Fatal("SND no freq");

               frequency = 0;
               for( ; num_channels>0; num_channels--){
                  settings.iChannels = num_channels;
                  dword caps_flags = (num_channels==1 ? TMdaAudioDataSettings::EChannelsMono : TMdaAudioDataSettings::EChannelsStereo);
                              //find first supported frequency in the pattern
                  const dword *fpp;
                  for(fpp = fp->freq; *fpp; ++fpp){
                     frequency = *fpp;
                     dword sample_rate_flags = GetSampleRateFlags(frequency);

                     settings.iCaps = sample_rate_flags | caps_flags;
                     LOG_N(" try init Hz", frequency);
                     int err = 0;
                     TRAP(err, strm->SetAudioPropertiesL(sample_rate_flags, caps_flags));
                     if(!err)
                        break;
                  }
                  if(*fpp)
                     break;
               }
               if(!num_channels){
                              //error: no matching frequency found
                  //Fatal("can't init sound");
                  num_channels = 1;
                  frequency = 22050;
               }
               dword bufs_per_sec = (init_flags&SND_INIT_LOW_DELAY) ? 25 : 16;
               buffer_size = frequency * num_channels / bufs_per_sec;
               buffer_size &= -16;
               delete[] sound_buf;
               sound_buf = new(true) short[buffer_size];
               buf_desc.Set((byte*)sound_buf, buffer_size*sizeof(short));
               LOG_N(" buffer size", buffer_size);

                                    //note that MaxVolume() is different in the emulator and the real device!
               max_vol = strm->MaxVolume();
                              //set to max hw volume initially
               if(new_hw_volume==0xffffffff)
                  new_hw_volume = max_vol;

               strm->SetPriority(EMdaPriorityMin, EMdaPriorityPreferenceNone);
               upd_buf_check = 2;
#endif
               //curr_hw_volume = max_vol;
               curr_hw_volume = 0xffffffff;  //force volume re-apply, because our stream's volume is initially unspecified

                                 //fill first buffer and write it to the stream
               UpdateBuffer();
               LOG(" open compl - buf sent");
            }
            break;
         default:
            {
               LOG_N("Open complete failed", err);
#ifdef STRICT_CHECK
               Fatal("OpenFail", err);
#endif
            }
         }
      }

   //----------------------------

      virtual void MaoscBufferCopied(TInt err, const TDesC8& aBuffer){

#ifdef USE_THREAD
         if(quit_request){
            LOG("BufCopy - quit request");
            CActiveScheduler::Stop();
            LOG("AS Stop");
         }else
#endif
         {
            switch(err){
            case KErrNone:
            case KErrUnderflow:  //can it be set here?
                              //we can be sure that playback is ok if it gets here
               if(upd_buf_check && !--upd_buf_check){
                  mode = MODE_PLAYING;
                  LOG("BufCopy - marking PLAYING");
               }
               UpdateBuffer();
               break;
            case KErrAbort:
               LOG("BufCopy - abort");
               break;
            default:
               {
                  LOG_N("BufCopy fail", err);
#ifdef STRICT_CHECK
                  Fatal("BufCopy fail", err);
#endif
               }
            }
         }
      }

   //----------------------------

      virtual void MaoscPlayComplete(TInt err){

         switch(err){
         case KErrUnderflow:
            LOG("PlayCompl - underflow");
            if(strm){
               strm->Stop();
               delete strm;
            }
            CreateStream();
#ifdef USE_THREAD
            mode = MODE_ACTIVATING;
            strm->Open(&settings);
#endif
            LOG("PlayCompl - reopened");
#ifdef STRICT_CHECK
            Fatal("Underflow");
#endif
            break;
         default:
            LOG_N("PlayCompl err", err);
            if(strm){
               strm->Stop();
               delete strm;
               strm = NULL;
               mode = MODE_CLOSED;
            }
                              //1 sec pause
            User::After(1000*1000);
            CreateStream();
#ifdef USE_THREAD
            mode = MODE_ACTIVATING;
            strm->Open(&settings);
#endif
            LOG("PlayCompl - reopened");
            break;
         case KErrCancel:
            LOG("PlayCompl - cancel");
            break;
         }
      }

#ifdef USE_THREAD
   //----------------------------
   // Stream thread proc.
      void ThreadProc(){

         thread_running = true;
         LOG("ThrdProc start");

#ifdef __SYMBIAN_3RD__
         this_static = this;
         User::SetExceptionHandler(ExceptionHandler, 0xffffffff);
#else
         {
            Dll::SetTls(this);
            RThread self;
            self.SetExceptionHandler(ExceptionHandler, 0xffffffff);
         }
#endif
         LOG_N("Desired freq", frequency);
         LOG_N("Desired chn", num_channels);

         mode = MODE_ACTIVATING;
         while(!quit_request){
            C_active_scheduler as;

            LOG("Installing AS");
            CActiveScheduler::Install(&as);

            LOG("CMdaAudioOutputStream::NewL");
            CreateStream();

            LOG("Opening strm");
            strm->Open(&settings);

            LOG("AS start");
            CActiveScheduler::Start();
            LOG("AS done");
            if(strm){
               strm->Stop();
               delete strm;
               strm = NULL;
               LOG("Deleted strm");
            }
            CActiveScheduler::Install(NULL);
         }
         mode = MODE_CLOSED;
#ifdef __SYMBIAN_3RD__
         this_static = NULL;
#else
         Dll::SetTls(NULL);
#endif
         LOG("ThrdProc End");
         thread_running = false;
      }

   //----------------------------

      static TInt ThreadProc(TAny *aPtr){
         C_stream_thread *_this = (C_stream_thread*)aPtr;
#ifdef USE_LOG
         _this->log.Open(L"C:\\Isound.log", false);
         _this->log.Add("Snd thrd started");
#endif
         CTrapCleanup *tc = CTrapCleanup::New();
         int err = 0;
         TRAP(err, _this->ThreadProc());
#ifdef USE_LOG
         //_this->log.Add("Snd thrd end");
         _this->log.Close();
#endif
         delete tc;
         return 0;
      }

//----------------------------

#ifdef __SYMBIAN_3RD__
      static C_stream_thread *this_static;
#endif

      static void ExceptionHandler(TExcType type){

#ifdef STRICT_CHECK
         Fatal("Exception", type);
#endif
#ifdef __SYMBIAN_3RD__
         C_stream_thread *_this = this_static;
#else
         C_stream_thread *_this = (C_stream_thread*)Dll::Tls();
#endif
#ifdef USE_LOG
         Cstr_c s; s<<"Exception " <<int(type);
         _this->log.Add(s);
#endif
         if(_this->strm){
            _this->strm->Stop();
            delete _this->strm;
            _this->strm = NULL;
         }
         CActiveScheduler::Stop();
         _this->thread_running = false;
         _this->mode = MODE_CLOSED;
         User::Exit(type);
      }

#endif

//----------------------------
#ifdef USE_LOG
      dword last_t;
#endif

      void UpdateBuffer(){

#if defined USE_LOG && 0
         {
            dword tm = GetTickTime();
            LOG_N("UpdateBuffer", tm-last_t);
            last_t = tm;
         }
#endif
#ifdef USE_THREAD
                              //sync access to sound writers array
         semaphore.Wait();
#endif

                              //clear the buffer first
         if(!num_sound_writers || !(init_flags&SND_INIT_NO_BUF_CLEAR))
            MemSet(sound_buf, 0, buffer_size*sizeof(short));

                              //don't apply always, as it causes breaks in audio on some hardware
         if(new_hw_volume!=0xffffffff && curr_hw_volume!=new_hw_volume){
            LOG_N("Update hw volume", new_hw_volume);
            strm->SetVolume(new_hw_volume);
            curr_hw_volume = new_hw_volume;
         }

         C_fixed vol(1);
#ifdef __WINS__
         //vol = curr_volume;
#endif
         if(num_sound_writers>1)
            vol = vol / C_fixed((int)num_sound_writers);
                                 //mix in all played sounds
         for(C_sound_writer *sw=first_sound_writer; sw; ){
            C_sound_writer *next = C_sound_player_imp::GetNextWriter(sw);
            sw->MixIntoBuffer(sound_buf, buffer_size, vol);
            sw = next;
         }
#ifdef USE_THREAD
         semaphore.Signal();
#endif

#ifdef DEBUG_PLAY_BUFFER_TICKS
                                 //write debugging mark into buffer, so that we hear clicks when buffer is filled
         for(dword i=0; i<4; i++) sound_buf[i] = (i&1) ? 0 : 0x8000;
#endif
                                 //call WriteL with a descriptor pointing at the buffer
         strm->WriteL(buf_desc);
      }

//----------------------------

   public:
      C_stream_thread():
         strm(NULL),
         sound_buf(NULL),
         mode(MODE_CLOSED)
      {
         Mem::Fill(&settings, sizeof(settings), 0);
         settings.iVolume = 1;
         settings.iChannels = 0;
         settings.iCaps = 0;
      }

   //----------------------------

      virtual ~C_stream_thread(){
         Stop();
      }

//----------------------------

      void Start(dword desired_frequency, dword desired_num_channels){

#ifdef USE_THREAD
         if(!thread_running){
            _LOG_N("Start", desired_frequency);
                              //set to desired values, real will be set when initializing
            frequency = desired_frequency;
            num_channels = desired_num_channels;
            quit_request = false;

            _LOG("Create thread");
            thrd.Create(_L("Sound streaming"), ThreadProc, 8192, NULL, this);
            thrd.SetPriority(EPriorityAbsoluteHigh);
            thrd.Resume();
                              //3 sec time-out
            int time_out = 300;
                              //wait for stream to start playback
            while(mode!=MODE_PLAYING){
               User::After(10*1000);
                              //kill thread if it fails to response in 1 second
               if(!--time_out){
                  _LOG_N("No response, kill", GetTickTime());
                  thrd.Kill(0);
                  thrd.Close();
                  delete[] sound_buf;
                  sound_buf = NULL;
                  mode = MODE_CLOSED;
                  thread_running = false;
/*
#ifndef __SYMBIAN_3RD__
                              //try again with 8K mono (seems typical for BT headset)
                  if(desired_frequency!=8000 || desired_num_channels!=1){
                     _LOG("Try 8KHz");
                     Start(8000, 1);
                  }
#endif
                  */
                  break;
               }
            }
            _LOG(" Ok");
         }
#else
         if(!strm){
            frequency = desired_frequency;
            num_channels = desired_num_channels;
            LOG_N("Desired freq", frequency);
            LOG_N("Desired chn", num_channels);

            LOG("CMdaAudioOutputStream::NewL");
            CreateStream();
         }
#endif
      }

   //----------------------------

      void Stop(){

#ifdef USE_THREAD
         if(thread_running){
            _LOG("Stop");
            quit_request = true;
            int time_out = 0;
            _LOG("Set quit request");
            while(mode!=MODE_CLOSED){
               User::After(10*1000);
                              //kill thread if it fails to respond in 2 seconds
               if(++time_out == 200){
                  _LOG(" no response, kill");
                  thrd.Kill(0);
                  mode = MODE_CLOSED;
                  break;
               }
            }
            thrd.Close();
            _LOG("thread close");
            thread_running = false;
                              //free buffers
            delete[] sound_buf;
            sound_buf = NULL;
         }
         system::Sleep(10);   //needed for proper cleanup
#else
         if(strm){
            strm->Stop();
            delete strm;
            strm = NULL;
            LOG("Deleted strm");
         }
         mode = MODE_CLOSED;
#endif
      }

//----------------------------

      const short *GetBuf() const{ return sound_buf; }

      //inline dword GetHardwarePlayPosition() const{ return 0xffffffff; }

   } stream;

//----------------------------
#endif//!USE_MMF
//----------------------------

   virtual dword GetnumChannels() const{ return stream.GetnumChannels(); }
   virtual dword GetPlaybackRate() const{ return stream.GetPlaybackRate(); }

//----------------------------

public:
   C_sound_player_imp()
   {
#ifdef USE_LOG
#ifdef USE_THREAD
      C_file::DeleteFile(L"C:\\Isound.log");
#endif
      stream._log.Open(L"C:\\_Isound.log", true);
#endif
   }
   void Construct(dword frequency, dword num_channels, dword flags){
      stream.Construct(flags);
      desired_frequency = frequency;
      desired_num_channels = num_channels;

      stream.Start(desired_frequency, desired_num_channels);
   }

   virtual ~C_sound_player_imp(){
      stream.Stop();
      for(C_sound_writer *s=stream.first_sound_writer; s; s=s->next)
         s->drv = NULL;
   }

   static inline C_sound_writer *GetNextWriter(C_sound_writer *sw){ return sw->next; }

//----------------------------

   virtual C_sound_source *CreateSoundSource(){
      C_sound_source *CreateSoundSourceInternal();
      return CreateSoundSourceInternal();
   }

//----------------------------

   virtual C_sound *CreateSound(){
      C_sound *CreateSoundInternal(C_sound_player *plr);
      return CreateSoundInternal(this);
   }

//----------------------------

   virtual void Activate(bool b){

      if(b){
         stream.Start(desired_frequency, desired_num_channels);
      }else{
         stream.Stop();
      }
   }

//----------------------------

   virtual void SetVolume(C_fixed vol){
      stream.SetVolume(vol);
   }

//----------------------------

   virtual dword GetMaxHardwareVolume() const{ return stream.GetMaxHardwareVolume(); }
   virtual void SetHardwareVolume(dword val){ stream.SetHardwareVolume(val); }
   virtual dword GetHardwareVolume() const{ return stream.GetHardwareVolume(); }

//----------------------------

   virtual void RegisterSoundWriter(C_sound_writer *snd){

      if(snd->next || stream.first_sound_writer==snd){
#ifdef _DEBUG
         Fatal("Can't snd reg.");
#endif
         return;
      }
#ifdef USE_THREAD
                              //sync access to sound writers array
      stream.semaphore.Wait();
#endif

      snd->next = stream.first_sound_writer;
      stream.first_sound_writer = snd;
      ++stream.num_sound_writers;

#ifdef USE_THREAD
      stream.semaphore.Signal();
#endif
   }

//----------------------------

   virtual void UnregisterSoundWriter(C_sound_writer *snd, bool use_lock){

#ifdef USE_THREAD
                              //sync access to sound writers array
      if(use_lock)
         stream.semaphore.Wait();
#endif

      if(stream.first_sound_writer==snd)
         stream.first_sound_writer = snd->next;
      else{
         C_sound_writer *s;
         for(s=stream.first_sound_writer; s; s=s->next){
            if(s->next==snd){
               s->next = snd->next;
               break;
            }
         }
         assert(s);
      }
      snd->next = NULL;
      --stream.num_sound_writers;
#ifdef USE_THREAD
      if(use_lock)
         stream.semaphore.Signal();
#endif
   }

//----------------------------

   virtual dword GetHardwareBufferSize() const{
      return stream.GetBufSize();
   }
   /*
   virtual dword GetHardwarePlayPosition() const{
      return stream.GetHardwarePlayPosition();
   }
   */
};

//----------------------------
#ifdef __SYMBIAN_3RD__
#ifdef USE_THREAD
C_sound_player_imp::C_stream_thread *C_sound_player_imp::C_stream_thread::this_static = NULL;
#endif
#endif

//----------------------------

C_sound_player *CreateSoundPlayer(dword frequency, dword num_channels, dword flags){

   C_sound_player_imp *plr = new(true) C_sound_player_imp;
   plr->Construct(frequency, num_channels, flags);
   //plr->SetHardwareVolume(plr->GetMaxHardwareVolume());
   return plr;
}

//----------------------------
//----------------------------

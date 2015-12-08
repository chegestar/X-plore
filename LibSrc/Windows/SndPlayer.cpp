//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

namespace win{
#include <Windows.h>
#ifdef _WIN32_WCE
#include <Pm.h>
#include <PmPolicy.h>
#endif
}
#include <SndPlayer.h>
#include <Util.h>
#ifndef _WIN32_WCE
#pragma comment(lib, "winmm")
#endif

//----------------------------
//#define USE_LOG

#ifdef USE_LOG
#include <LogMan.h>
#include <Cstr.h>
#define LOG(s) log_man.Add(s)
#define LOG_N(s, n) { Cstr_c str; str<<s <<n; log_man.Add(str); }
#else
#define LOG(s)
#define LOG_N(s, n)
#endif

#ifndef _WIN32_WCE
                              //on PC don't change global mixer volume (waveOutSetVolume does that)
#define USE_SOFTWARE_VOLUME
#endif

//----------------------------

const int BUF_STEPS_LOW_DELAY = 25;     //how many time a buffer loops per second
#ifdef _DEBUG
const int BUF_STEPS_DEFAULT = 8;
#else
const int BUF_STEPS_DEFAULT = 16;
#endif
const int NUM_BUFFERS = 3;

//#define USE_THREAD

#ifdef _DEBUG

//#define DEBUG_PLAY_BUFFER_TICKS  //insert ticks into sound playback buffer in each Tick
#endif

//#define DEBUG_GEN_WAVE

//----------------------------
//----------------------------

class C_sound_player_imp: public C_sound_player{

   dword init_flags;
   dword desired_frequency;
   dword buffer_size;         //size of mixing buffer, in samples
   dword desired_num_channels;

#ifdef USE_LOG
   C_logman log_man;
#endif
//----------------------------

                           //we're having n buffers for playback, always having prepared next to be played
                           // immediately as soon as other is finished
   struct S_buffer{
      short *sound_buf;
      win::WAVEHDR wave_hdr;

      S_buffer():
         sound_buf(NULL)
      {}
      ~S_buffer(){
         delete[] sound_buf;
      }
   } buffer[NUM_BUFFERS];

   win::CRITICAL_SECTION semaphore;

                              //linked list of sound writers
   C_sound_writer *first_sound_writer;
   dword num_sound_writers;

   C_fixed curr_volume, new_volume;
   bool quit_request, active;

   enum{
      MODE_CLOSED,
      MODE_PLAYING,
   } mode;

   win::HWAVEOUT hwo;

#ifdef DEBUG_GEN_WAVE
   short debug_wave;
#endif

//----------------------------
                              //callback func for waveOut
   void waveOutProc(dword uMsg, dword dwParam1, dword dwParam2){

      switch(uMsg){
      case WOM_OPEN:
         mode = MODE_PLAYING;
         LOG("WOM_OPEN");
         break;
      case WOM_CLOSE:
         mode = MODE_CLOSED;
         LOG("WOM_CLOSE");
         break;
      case WOM_DONE:
         {
            win::WAVEHDR *header = (win::WAVEHDR*)dwParam1;
            int buf_i = header->dwUser;
#ifdef USE_THREAD
            //win::EnterCriticalSection(&semaphore);
            SetEvent(h_update_event[buf_i]);
            //win::LeaveCriticalSection(&semaphore);
#else
            if(quit_request)
               break;
            win::waveOutUnprepareHeader(hwo, &buffer[buf_i].wave_hdr, sizeof(win::WAVEHDR));
            UpdateBuffer(buf_i);
            win::waveOutWrite(hwo, &buffer[buf_i].wave_hdr, sizeof(win::WAVEHDR));
#endif
         }
         break;
      default:
         LOG_N("WOM_UNKNOWN", uMsg);
#ifdef _DEBUG
         //Fatal("waveOutProc", uMsg);
#endif
         assert(0);
      }
   }
   static void CALLBACK waveOutProcThunk(win::HWAVEOUT hwo, win::UINT uMsg, win::DWORD dwInstance, win::DWORD dwParam1, win::DWORD dwParam2){
      ((C_sound_player_imp*)dwInstance)->waveOutProc(uMsg, dwParam1, dwParam2);
   }

//----------------------------

#ifdef USE_THREAD
   HANDLE h_update_thread;
                              //signal handles for all buffers, plus last handle for quit request
   HANDLE h_update_event[NUM_BUFFERS+1];

   void ThreadProc(){

      LOG("Thread started");
      while(true){
         int buf_i = WaitForMultipleObjects(NUM_BUFFERS+1, h_update_event, false, INFINITE);

         //win::EnterCriticalSection(&semaphore);

         S_buffer &buf = buffer[buf_i];

         waveOutUnprepareHeader(hwo, &buf.wave_hdr, sizeof(WAVEHDR));
         if(buf_i==NUM_BUFFERS){
            assert(quit_request);
            LOG("Thread quit request");
            //win::LeaveCriticalSection(&semaphore);
            waveOutReset(hwo);
            for(int i=NUM_BUFFERS; i--; )
               waveOutUnprepareHeader(hwo, &buffer[i].wave_hdr, sizeof(WAVEHDR));
            break;
         }
         UpdateBuffer(buf_i);
         win::waveOutWrite(hwo, &buf.wave_hdr, sizeof(WAVEHDR));

         //++buf_i %= NUM_BUFFERS;
         ResetEvent(h_update_event[buf_i]);
         //win::LeaveCriticalSection(&semaphore);
      }
      LOG("Quitting thread");
   }

   static dword CALLBACK ThreadProcThunk(void *c){
      C_sound_player_imp *_this = (C_sound_player_imp*)c;
      _this->ThreadProc();
      return 0;
   }
#endif

//----------------------------

   void UpdateBuffer(int buf_i){

      win::EnterCriticalSection(&semaphore);
#ifdef USE_LOG_
      LOG_N("UpdateBuffer: ", buf_i);
      static int last = GetTickTime();
      int tc = GetTickTime();
      LOG_N(" ", tc-last);
      last = tc;
#endif

      S_buffer &buf = buffer[buf_i];
                              //clear the buffer first
      if(!num_sound_writers || !(init_flags&SND_INIT_NO_BUF_CLEAR))
         MemSet(buf.sound_buf, 0, buffer_size*sizeof(short));

      if(curr_volume!=new_volume){
         //Fatal("v", new_volume.val);
         curr_volume = new_volume;
#ifndef USE_SOFTWARE_VOLUME
         dword vol = Min(0xffff, curr_volume.val);
         waveOutSetVolume(hwo, vol | vol<<16);
#endif
      }
      /*
      C_fixed vol = 1;
      if(num_sound_writers)
         vol = vol / C_fixed((int)num_sound_writers);
         */
      C_fixed vol = C_fixed::Third();

                              //mix in all played sounds
      for(C_sound_writer *sw=first_sound_writer; sw; ){
         C_sound_writer *next = sw->next;
         sw->MixIntoBuffer(buf.sound_buf, buffer_size, vol);
         sw = next;
      }
#ifdef USE_SOFTWARE_VOLUME
      if(curr_volume!=C_fixed(1)){
         int vol_fp = curr_volume.val;
         for(dword i=0; i<buffer_size; i++){
            short &s = buf.sound_buf[i];
            s = short((s * vol_fp) >> 16);
         }
      }
#endif
#ifdef DEBUG_PLAY_BUFFER_TICKS
                              //write debugging mark into buffer, so that we hear clicks when buffer is filled
      for(dword i=0; i<4; i++) buf.sound_buf[i] = (i&1) ? 0 : 0x8000;
#endif
#ifdef DEBUG_GEN_WAVE
      for(dword i=0; i<buffer_size; i++){
         buf.sound_buf[i] = 0x1000 - (debug_wave&0x1fff);
         debug_wave += 0x100;
      }
#endif

      buf.wave_hdr.lpData = (char*)buf.sound_buf;
      buf.wave_hdr.dwBufferLength = buffer_size * sizeof(short);
      buf.wave_hdr.dwUser = buf_i;
      win::waveOutPrepareHeader(hwo, &buf.wave_hdr, sizeof(win::WAVEHDR));

      win::LeaveCriticalSection(&semaphore);
   }

//----------------------------

   virtual dword GetPlaybackRate() const{
      return desired_frequency;
   }

   virtual dword GetnumChannels() const{
      return desired_num_channels;
   }

//----------------------------

public:
   C_sound_player_imp(dword frequency, dword num_channels, dword flags):
      init_flags(flags),
      num_sound_writers(0),
      first_sound_writer(NULL),
      quit_request(false),
      active(false),
      curr_volume(2),
      new_volume(1),
      desired_frequency(frequency),
      desired_num_channels(num_channels),
#ifdef USE_THREAD
      h_update_thread(NULL),
#endif
#ifdef _WIN32_WCE
      power_handle(win::HANDLE(-1)),
#endif
      hwo(NULL)
   {
#ifdef USE_LOG
      log_man.Open(L"sound.log");
#endif
      buffer_size = (desired_frequency * desired_num_channels / ((init_flags&SND_INIT_LOW_DELAY) ? BUF_STEPS_LOW_DELAY : BUF_STEPS_DEFAULT));
      buffer_size &= -16;
      //buffer_size = 1380;
      InitializeCriticalSection(&semaphore);

#ifdef _WIN32_WCE
      SetPowerState(true);
#endif

      mode = MODE_CLOSED;
      win::WAVEFORMATEX fmt;
      memset(&fmt, 0, sizeof(fmt));
      fmt.wFormatTag = WAVE_FORMAT_PCM;
      fmt.nChannels = word(GetnumChannels());
      fmt.nSamplesPerSec = GetPlaybackRate();
      fmt.wBitsPerSample = 16;
      fmt.nBlockAlign = fmt.nChannels * (fmt.wBitsPerSample/8);
      fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;

      LOG("Open device");
      win::MMRESULT mmr;
      for(dword id=0; id<win::waveOutGetNumDevs(); id++){
         mmr = win::waveOutOpen(&hwo, id, &fmt, (dword)&waveOutProcThunk, (dword)this, CALLBACK_FUNCTION);
         if(mmr==MMSYSERR_NOERROR)
            break;
      }
      if(!hwo)
         return;
      while(mode!=MODE_PLAYING)
         win::Sleep(10);
      LOG(" opened");
#ifdef USE_THREAD
                              //signalling event
      for(int i=0; i<NUM_BUFFERS+1; i++)
         h_update_event[i] = win::CreateEvent(NULL, true, false, NULL);
      dword tid;
      h_update_thread = win::CreateThread(NULL, 0, ThreadProcThunk, this, 0, &tid);
      win::SetThreadPriority(h_update_thread, THREAD_PRIORITY_HIGHEST);
#endif
      win::waveOutPause(hwo);
                              //start playing buffers
      for(int i=0; i<NUM_BUFFERS; i++){
         MemSet(&buffer[i].wave_hdr, 0, sizeof(win::WAVEHDR));
         buffer[i].sound_buf = new short[buffer_size];
         UpdateBuffer(i);
         win::waveOutWrite(hwo, &buffer[i].wave_hdr, sizeof(win::WAVEHDR));
      }
      win::waveOutRestart(hwo);
      active = true;
   }

//----------------------------

   virtual ~C_sound_player_imp(){

      for(C_sound_writer *s=first_sound_writer; s; s=s->next)
         s->drv = NULL;

      if(mode!=MODE_CLOSED){
         LOG("Request quit");
         quit_request = true;
#ifdef USE_THREAD
         SetEvent(h_update_event[NUM_BUFFERS]);
#endif

#if 1
         win::waveOutReset(hwo);

         for(int i=NUM_BUFFERS; i--; ){
            if(buffer[i].wave_hdr.dwFlags&WHDR_PREPARED)
               win::waveOutUnprepareHeader(hwo, &buffer[i].wave_hdr, sizeof(win::WAVEHDR));
         }
#ifdef _WIN32_WCE
         if(active)
            SetPowerState(false);
#endif

#else
         if(!active)
            Activate(true);

         int time_out = 0;
         while(true){
            int i;
            for(i=NUM_BUFFERS; i--; ){
               if(buffer[i].wave_hdr.dwFlags&WHDR_PREPARED)
                  break;
            }
            if(i==-1)
               break;
            win::Sleep(10);
                           //kill thread if it fails to response in 1 second
            if(++time_out == 100){
               LOG("Quit timeout, break");
               mode = MODE_CLOSED;
               break;
            }
         }
#endif
         LOG(" ok");
      }
#ifdef USE_THREAD
      LOG("Terminate thread");
      TerminateThread(h_update_thread, 0);
      LOG("Close handles");
      CloseHandle(h_update_thread);
      for(int i=0; i<NUM_BUFFERS+1; i++)
         CloseHandle(h_update_event[i]);
#endif
      LOG("waveOutClose");
      waveOutClose(hwo);
      LOG(" OK");
      DeleteCriticalSection(&semaphore);
   }

//----------------------------

   virtual C_sound_source *CreateSoundSource(){
      C_sound_source *CreateSoundSourceInternal();
      return CreateSoundSourceInternal();
   }

//----------------------------

   virtual C_sound *CreateSound(){
      C_sound *CreateSoundInternal(C_sound_player*);
      return CreateSoundInternal(this);
   }

//----------------------------
#ifdef _WIN32_WCE
   win::HANDLE power_handle;

   void SetPowerState(bool b){
      if(b){
         if(power_handle==win::HANDLE(-1)){
            power_handle = win::SetPowerRequirement(L"WAV1:", win::D0, 1, NULL, 0);
            win::PowerPolicyNotify(PPN_UNATTENDEDMODE, true);
         }
      }else{
         if(power_handle!=win::HANDLE(-1)){
            win::PowerPolicyNotify(PPN_UNATTENDEDMODE, false);
            win::ReleasePowerRequirement(power_handle);
            power_handle = win::HANDLE(-1);
         }
      }
   }

#endif
//----------------------------

   virtual void Activate(bool b){

      if(hwo && active!=b){
         if(b){
            //win::SetPowerR
            win::waveOutRestart(hwo);
         }else{
            win::waveOutPause(hwo);
         }
         active = b;
#ifdef _WIN32_WCE
         SetPowerState(b);
#endif
      }
   }

//----------------------------

   virtual void SetVolume(C_fixed vol){
      new_volume = vol;
   }

//----------------------------

#ifdef _DEBUG
                              //simulate less volume steps in debug mode
   virtual dword GetMaxHardwareVolume() const{ return 8; }
   virtual void SetHardwareVolume(dword val){ new_volume.val = val<<13; }
   virtual dword GetHardwareVolume() const{ return new_volume.val>>13; }
#else
   virtual dword GetMaxHardwareVolume() const{ return 0x10000; }
   virtual void SetHardwareVolume(dword val){ new_volume.val = val; }
   virtual dword GetHardwareVolume() const{ return new_volume.val; }
#endif

//----------------------------

   virtual void RegisterSoundWriter(C_sound_writer *snd){

      if(snd->next || first_sound_writer==snd)
         Fatal("Can't snd reg.");
                              //sync access to sound writers array
      win::EnterCriticalSection(&semaphore);

      snd->next = first_sound_writer;
      first_sound_writer = snd;
      ++num_sound_writers;

      win::LeaveCriticalSection(&semaphore);
   }

//----------------------------

   virtual void UnregisterSoundWriter(C_sound_writer *snd, bool use_lock){

                              //sync access to sound writers array
      if(use_lock)
         win::EnterCriticalSection(&semaphore);

      if(first_sound_writer==snd)
         first_sound_writer = snd->next;
      else{
         C_sound_writer *s;
         for(s=first_sound_writer; s; s=s->next){
            if(s->next==snd){
               s->next = snd->next;
               break;
            }
         }
#ifdef _DEBUG
         if(!s)
            Fatal("Can't snd unreg.");
#endif
      }
      snd->next = NULL;
      --num_sound_writers;
      if(use_lock)
         win::LeaveCriticalSection(&semaphore);
   }

//----------------------------

   virtual dword GetHardwareBufferSize() const{
      return buffer_size;
   }
};

//----------------------------

C_sound_player *CreateSoundPlayer(dword frequency, dword num_channels, dword flags){
   return new(true) C_sound_player_imp(frequency, num_channels, flags);
}

//----------------------------
//----------------------------

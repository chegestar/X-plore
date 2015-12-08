//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#ifndef _SNDPLAYER_H_
#define _SNDPLAYER_H_

#include <Rules.h>
#include <SmartPtr.h>
#include <Fixed.h>

//----------------------------
class C_sound_player_imp;
                              //sound source that writes output to played audio buffer (in PCM format)
                              // there can be single sound writer rewriting the buffer, or multiple sound writers where every class adds its data to buffer
class C_sound_writer: public C_unknown{
   C_sound_writer *next;
protected:
   class C_sound_player *drv;

   C_sound_writer():
      next(NULL)
   {}

   friend class C_sound_player_imp;
public:
   virtual void MixIntoBuffer(short *buf, dword len, class C_fixed fp_volume) = 0;
};

//----------------------------
class C_sound_source: public C_unknown{
public:
   virtual bool Open(const wchar *filename) = 0;
   virtual bool Open(class C_file &fl) = 0;
   virtual dword GetPlayTime() const = 0;
};

class C_sound: public C_sound_writer{
public:
   virtual void Open(C_sound_source *src) = 0;
   virtual void Play() = 0;
   virtual void Stop() = 0;
   virtual bool IsPlaying() const = 0;
   virtual void SetVolume(C_fixed v) = 0;
   virtual C_fixed GetVolume() const = 0;
   virtual dword GetPlayTime() const = 0;
   virtual dword GetPlayPos() const = 0;
   virtual void SetPlayPos(dword) = 0;
};

//----------------------------

class C_sound_player: public C_unknown{
public:
   virtual C_sound_source *CreateSoundSource() = 0;
   virtual C_sound *CreateSound() = 0;

//----------------------------
// Register/unregister sound writer.
   virtual void RegisterSoundWriter(C_sound_writer*) = 0;
   virtual void UnregisterSoundWriter(C_sound_writer*, bool use_lock = true) = 0;

//----------------------------
// Get playback frequency.
   virtual dword GetPlaybackRate() const = 0;

//----------------------------
// Get number of channels.
   virtual dword GetnumChannels() const = 0;

//----------------------------
// Set system state (focus gained/lost).
   virtual void Activate(bool) = 0;

//----------------------------
// Set global sound volume.
   virtual void SetVolume(C_fixed volume) = 0;

//----------------------------
// Get maximal volume level applicable to hardware.
   virtual dword GetMaxHardwareVolume() const = 0;

//----------------------------
// Set volume in hardware level range (0 - GetMaxHardwareVolume).
   virtual void SetHardwareVolume(dword val) = 0;

//----------------------------
// Get current hardware volume.
   virtual dword GetHardwareVolume() const = 0;

//----------------------------
// Get size of hardware buffer, in sample values.
   virtual dword GetHardwareBufferSize() const = 0;

//----------------------------
// Get play position, in sample values.
   //virtual dword GetHardwarePlayPosition() const{ return 0xffffffff; }
};

//----------------------------

enum{
   SND_INIT_LOW_DELAY = 1,    //use smaller buffer, which produces lower delay, but have more chances for underflow (suitable for games)
   SND_INIT_NO_BUF_CLEAR = 2, //do not clear playback buffer if at least one sound writer is registered
};

C_sound_player *CreateSoundPlayer(dword frequency = 22050, dword num_channels = 2, dword flags = 0);

//----------------------------
//----------------------------
                              //sound writer that has internal temp buffer, and can perform sample frequency resampling and mixing mono > stereo, stereo > mono
class C_sound_writer_mixer: public C_sound_writer{
   virtual void MixIntoBuffer(short *buf, dword num_samples, C_fixed _mix_volume);
protected:
   dword src_num_channels, src_frequency;
   dword dst_num_channels, dst_frequency;
   dword DECODE_BUF_SIZE;     //must be 2^n

                              //temp decode buffer (PCM format of sound driver)
   short *decode_buf;
   dword pos_read, pos_write; //buffer read/write absolute position, in shorts
   dword pos_read_base_time;  //absolute time to which pos_read is relative to
   mutable dword last_time, last_mix_time;

   dword curr_time_system_base, pause_time_system_base;  //help for computing more exact time
   bool is_playing;

                              //resampling:
   C_fixed resample_ratio, last_resample_frac;

   C_fixed volume;

   C_sound_writer_mixer(C_sound_player *_drv);
   ~C_sound_writer_mixer(){
      delete[] decode_buf;
   }

//----------------------------
   void ResetBufferPos();

   void InitResampler();

//----------------------------
// Conpute time from sample count.
   dword GetTimeFromSamples(dword num_samples) const;

//----------------------------
// Resample mono or stereo input samples to destination format. Return number of src samples consumed, or 0 if temp buffer is full.
// If source is mono, only src0 is used.
// Source samples are in separate buffers of 32-bit data.
   //dword ResampleAndWriteBuffer(const int *src0, const int *src1, dword num_src_samples, int src_bit_depth);

//----------------------------
// Resample mono or stereo input samples to destination format. Return number of src samples consumed, or 0 if temp buffer is full.
// Source format is interleaved PCM 16-bit data.
   dword ResampleAndWriteBuffer(const short *src, dword num_src_samples);

public:
//----------------------------
// Compute time from samples output to audio hardware.
   virtual dword GetCurrTime() const;

   inline bool IsPlaying() const{ return is_playing; }

//----------------------------
// Start playback - register as sound writer.
   virtual void Play();

//----------------------------
// Pause playback. It may be resumed later by call to Play.
   virtual void Pause();

//----------------------------
   inline void SetVolume(C_fixed vol){
      volume = vol;
   }
};

//----------------------------

#endif

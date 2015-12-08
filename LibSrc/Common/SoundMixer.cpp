//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <SndPlayer.h>
#include <Util.h>

//----------------------------

C_sound_writer_mixer::C_sound_writer_mixer(C_sound_player *_drv):
   pos_read_base_time(0),
   is_playing(false),
   src_num_channels(0),
   src_frequency(0),
   dst_num_channels(0),
   dst_frequency(0),
   pos_read(0),
   pos_write(0),
   resample_ratio(0), last_resample_frac(0),
   volume(1),
   last_time(0xffffffff),
   last_mix_time(0xffffffff),
   decode_buf(NULL)
{
   drv = _drv;
   if(drv){
      dst_num_channels = drv->GetnumChannels();
      dst_frequency = drv->GetPlaybackRate();

      dword ideal_size = dst_num_channels * dst_frequency / 1;
      DECODE_BUF_SIZE = 1 << (FindHighestBit(ideal_size)+1);
      //Fatal("DECODE_BUF_SIZE", DECODE_BUF_SIZE);
      DECODE_BUF_SIZE = Max(DECODE_BUF_SIZE, 0x2000ul);    //must be at least 8KB
      decode_buf = new(true) short[DECODE_BUF_SIZE];
   }
}

//----------------------------

void C_sound_writer_mixer::ResetBufferPos(){

   pos_read = pos_write = 0;
   curr_time_system_base = pause_time_system_base = GetTickTime();
   last_time = 0xffffffff;
   last_mix_time = 0xffffffff;
}

//----------------------------

void C_sound_writer_mixer::InitResampler(){

   resample_ratio = C_fixed::Create(src_frequency) / C_fixed::Create(dst_frequency);
   last_resample_frac = 0;
}

//----------------------------

dword C_sound_writer_mixer::GetTimeFromSamples(dword num_samples) const{

   int div = dst_num_channels*dst_frequency;
   return int((longlong(num_samples)*longlong(1000)) / longlong(div));
}

//----------------------------

dword C_sound_writer_mixer::GetCurrTime() const{

   int pos = pos_read;
   pos = Max(0, int(pos)-int(drv->GetHardwareBufferSize()));
   dword t = pos_read_base_time + GetTimeFromSamples(pos);
   int delta = (!is_playing ? pause_time_system_base : GetTickTime()) - curr_time_system_base;
   delta = Max(0, Min(1000, delta));
   t += delta;
   if(last_time!=0xffffffff){
      if(t<last_time)
         t = last_time;
   }
   last_time = t;
   return t;
}

//----------------------------

void C_sound_writer_mixer::Play(){

   if(!is_playing){
      is_playing = true;
      drv->RegisterSoundWriter(this);
      //drv->Activate(true);
      curr_time_system_base += GetTickTime()-pause_time_system_base;
      last_time = 0xffffffff;
      last_mix_time = 0xffffffff;
   }
}

//----------------------------

void C_sound_writer_mixer::Pause(){

   if(is_playing){
      //drv->Activate(false);
      drv->UnregisterSoundWriter(this);
      pause_time_system_base = GetTickTime();
      is_playing = false;
   }
}

//----------------------------

void C_sound_writer_mixer::MixIntoBuffer(short *buf, dword num_samples, C_fixed _mix_volume){

                           //check that we're not ahead of write pointer
   dword copy_len = Min(num_samples, pos_write-pos_read);
   if(copy_len < num_samples){
                           //decoder doesn't have enough data, fill rest by zero
      MemSet(buf+copy_len, 0, (num_samples - copy_len)*sizeof(short));
      if(pos_write){
         LOG_DEBUG("Audio buffer underflow, mix silence");
      }
   }
   const dword DECODE_BUF_MASK = DECODE_BUF_SIZE-1;
                           //copy the temp buffer, and adjust read pointer
   dword end_pos = pos_read + copy_len;
   if((pos_read & ~DECODE_BUF_MASK) != ((end_pos-1) & ~DECODE_BUF_MASK)){
      dword num = (end_pos & ~DECODE_BUF_MASK) - pos_read;
      MemCpy(buf, &decode_buf[pos_read&DECODE_BUF_MASK], num*sizeof(short));
      pos_read += num;
      copy_len -= num;
      buf += num;
   }
   MemCpy(buf, &decode_buf[pos_read&DECODE_BUF_MASK], copy_len*sizeof(short));

   dword tick_time = GetTickTime();

   if(last_time!=0xffffffff){
                              //compensate for uneven time periods when hw device may call this function
      dword buf_time = GetTimeFromSamples(num_samples);
                              //proposed (ideal) time when this func is called
      dword proposed_time = curr_time_system_base + buf_time;
                              //get time delta (error)
      int time_delta = int(proposed_time)-int(tick_time);
                              //add part of delta to tick time for compensation
      tick_time += time_delta*3/4;
   }
   curr_time_system_base = tick_time;
   pos_read += copy_len;
}

//----------------------------

static inline short MakeSample(int sample, C_fixed volume){
   int s = (C_fixed::Create(sample) * volume).val;
   if(s>0x7fff)
      s = 0x7fff;
   if(s<-0x8000)
      s = -0x8000;
   //assert(s<=0x7fff);
   //assert(s>=-0x8000);
   return short(s);
}

//----------------------------

dword C_sound_writer_mixer::ResampleAndWriteBuffer(const short *src, dword num_src_samples){

   const dword DECODE_BUF_MASK = DECODE_BUF_SIZE-1;
   int max_fill = Min(pos_read+DECODE_BUF_SIZE, (pos_write+DECODE_BUF_SIZE) & ~DECODE_BUF_MASK);
   dword num_dst_samples = max_fill - pos_write;
                              //if buffer is full, do nothing
   if(!num_dst_samples)
      return 0;
   if(dst_num_channels==2)
      num_dst_samples /= 2;
   short *dst = decode_buf + (pos_write&DECODE_BUF_MASK);

   C_fixed vol = volume;
   if(resample_ratio == C_fixed::One()){
      dword num = Min(num_src_samples, num_dst_samples);
      num_src_samples = num;
      num_dst_samples = num;
      const short *dst_end = dst + num * dst_num_channels;
                              //direct copy
      if(dst_num_channels==1){
         if(src_num_channels==1){
                              //mono to mono
            do{
               *dst++ = MakeSample(*src++, vol);
            }while(dst != dst_end);
         }else{
                              //stereo to mono
            vol >>= 1;
            do{
               int s = *src++;
               s += *src++;
               *dst++ = MakeSample(s, vol);
            }while(dst != dst_end);
         }
      }else{
         if(src_num_channels==1){
                              //mono to stereo
            do{
               short s = MakeSample(*src++, vol);
               *dst++ = s;
               *dst++ = s;
            }while(dst != dst_end);
         }else{
                              //stereo to stereo
            do{
               *dst++ = MakeSample(*src++, vol);
               *dst++ = MakeSample(*src++, vol);
            }while(dst != dst_end);
         }
      }
   }else{
                              //process resampling
      C_fixed src_offs = Min(last_resample_frac, C_fixed(num_src_samples-1));
      const C_fixed src_end = num_src_samples;
      const short *dst_beg = dst, *dst_max = dst+num_dst_samples*dst_num_channels;

      if(dst_num_channels==1){
         if(src_num_channels==1){
                              //mono to mono
            for( ; dst<dst_max && src_offs<src_end; src_offs += resample_ratio){
               *dst++ = MakeSample(src[int(src_offs)], vol);
            }
         }else{
                              //stereo to mono
            vol >>= 1;
            for( ; dst<dst_max && src_offs<src_end; src_offs += resample_ratio){
               const short *s = src + int(src_offs)*2;
               *dst++ = MakeSample(s[0] + s[1], vol);
            }
         }
      }else{
         if(src_num_channels==1){
                              //mono to stereo
            for( ; dst<dst_max && src_offs<src_end; src_offs += resample_ratio){
               short s = MakeSample(src[int(src_offs)], vol);
               *dst++ = s;
               *dst++ = s;
            }
         }else{
                              //stereo to stereo
            for( ; dst<dst_max && src_offs<src_end; src_offs += resample_ratio){
               int src_i = int(src_offs)*2;
               *dst++ = MakeSample(src[src_i  ], vol);
               *dst++ = MakeSample(src[src_i+1], vol);
            }
         }
      }
      num_src_samples = Min(int(src_offs), int(num_src_samples));
      last_resample_frac = src_offs - C_fixed(num_src_samples);
      num_dst_samples = dst - dst_beg;
      if(dst_num_channels==2)
         num_dst_samples /= 2;
   }
   pos_write += num_dst_samples * dst_num_channels;
   return num_src_samples;
}

//----------------------------

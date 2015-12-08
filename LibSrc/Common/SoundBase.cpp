//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <SndPlayer.h>
#include <C_file.h>
#include <Fixed.h>

//----------------------------

class C_sound_source_imp: public C_sound_source{
   friend class C_sound_player_imp;
public:
   enum E_FORMAT{
      FORMAT_PCM = 1,
      FORMAT_ADPCM = 2,
      FORMAT_ALAW = 6,
      FORMAT_MULAW = 7,
   };
private:

   void *data;
   dword num_samples;
   byte num_channels;
   word sample_rate;
   E_FORMAT format;

                              //PCM:
   byte bytes_per_sample;

public:
   C_sound_source_imp():
      data(NULL),
      num_samples(0),
      bytes_per_sample(0),
      num_channels(0),
      sample_rate(0)
   {}
   virtual ~C_sound_source_imp(){
      Close();
   }

   inline E_FORMAT GetFormat() const{ return format; }
   inline const void *Data() const{ return data; }
   inline dword NumSamples() const{ return num_samples; }
   inline dword Bpp() const{ return bytes_per_sample; }
   inline dword SampleRate() const{ return sample_rate; }
   inline dword NumChannels() const{ return num_channels; }

//----------------------------

   void Close();

//----------------------------

   virtual bool Open(const wchar *filename){
      C_file fl;
      if(!fl.Open(filename))
         return false;
      return Open(fl);
   }
   virtual bool Open(C_file &fl);

//----------------------------

   virtual dword GetPlayTime() const{
      return num_samples*1000/sample_rate;
   }
};

//----------------------------

void C_sound_source_imp::Close(){

   delete[] (byte*)data;
   data = NULL;
   num_samples = 0;
}

//----------------------------

bool C_sound_source_imp::Open(C_file &fl){

   Close();

   if(fl.GetFileSize()<sizeof(dword))
      return false;

   dword id;
   if(!fl.ReadDword(id))
      return false;
   fl.Seek(0);
   if(id==0x46464952){     //'RIFF'

      struct{
         dword riff_id, riff_chunk_size;
         dword wave_id;
         dword fmt_id, fmt_chunk_size;
         word fmt_tag, num_channels;
         dword samples_per_sec;
         dword avg_bytes_per_sec;
      } header;

                           //load .wav
      if(!fl.Read(&header, sizeof(header)))
         return false;
                           //check params...
      if(header.riff_id==0x46464952 && //'RIFF'
         header.wave_id==0x45564157 && //'WAVE'
         header.fmt_id==0x20746d66)    //'fmt '
      {
         format = (E_FORMAT)header.fmt_tag;
         if(format==FORMAT_PCM
#ifndef __SYMBIAN32__
            || format==FORMAT_ADPCM
#endif
            //|| format==FORMAT_ALAW || format==FORMAT_MULAW
            ){
                              //valid wav header, check if it matches our expectations
            int bps = header.avg_bytes_per_sec/header.samples_per_sec;

            if((header.num_channels==1 || header.num_channels==2) && bps<=4){
               fl.Seek(header.fmt_chunk_size+20);

               while(!fl.IsEof()){
                  struct{
                     dword data_id, chunk_size;
                  } chk;
                  fl.Read(&chk, sizeof(chk));
                  dword pos = fl.Tell();
                  //*
                  if(chk.data_id==0x74636166){   //'fact'
                     if(chk.chunk_size>=4){
                        fl.ReadDword(num_samples);
                     }
                  }else /**/
                  if(chk.data_id==0x61746164){   //'data'
                     sample_rate = (word)header.samples_per_sec;
                     num_channels = byte(header.num_channels);
                     data = new(true) byte[chk.chunk_size];
                     fl.Read(data, chk.chunk_size);

                     switch(format){
                     case FORMAT_PCM:
                        bytes_per_sample = byte(bps/header.num_channels);
                        num_samples = chk.chunk_size / bps;
                        if(bps==1){
                                       //change sign of data
                           dword *dp = (dword*)data;
                           dword i;
                           for(i=num_samples/4; i--; )
                              *dp++ ^= 0x80808080;
                           for(i=num_samples&3; i--; )
                              ((byte*)dp)[i] ^= 0x80;
                        }
                        break;

#ifndef __SYMBIAN32__               //don't include in Symbian, this format is not used there so often, save space
                     case FORMAT_ADPCM:
                        {
                                       //decode ADPCM into temp buffer, then play as PCM
                           fl.Seek(0x26);
                           word samples_per_block;
                           fl.ReadWord(samples_per_block);

                           if(!num_samples){
                                                //fact chunk not present, determine num samples
                              int block_size = ((samples_per_block-2)/2 + 7) * num_channels;
                              num_samples = chk.chunk_size/block_size*samples_per_block;
                           }
                           const byte *bp = (byte*)data;
                           short *new_data = new(true) short[num_samples*num_channels];
                           short *dst = new_data, *dst_end = dst+num_samples*num_channels;

                           struct S_block_data{
                              int sample1, sample2;

                              int coef1, coef2;
                              int delta;

                              short DecodeSample(byte code){

                                 int s = ((sample1 * coef1) + (sample2 * coef2)) >> 8;

                                                         //add delta (from code)
                                 if(code&8)
                                    s += delta * (code-0x10);
                                 else
                                    s += delta * code;

                                                         //clamp sample
                                 if(s < -32768)
                                    s = -32768;
                                 else
                                 if(s > 32767)
                                    s = 32767;

                                 static const int adaptive[] = {
                                    230, 230, 230, 230, 307, 409, 512, 614,
                                    768, 614, 512, 409, 307, 230, 230, 230
                                 };
                                 int d = (delta * adaptive[code]) >> 8;
                                 if(d < 16)
                                    d = 16;
                                 delta = d;
                                 sample2 = sample1;
                                 sample1 = s;
                                 return short(s);
                              }
                           } block_data[2];
                           static const short coef_table[7][2] = {
                              { 256, 0},
                              { 512, -256},
                              { 0, 0},
                              { 192, 64},
                              { 240, 0},
                              { 460, -208},
                              { 392, -232},
                           };
                           struct S_hlp{
                              static inline short ReadShort(const byte *&bp1){
                                 byte lo = *bp1++;
                                 byte hi = *bp1++;
                                 return short((hi<<8) | lo);
                              }
                           };
                           while(dst<dst_end){
                              int i;
                                          //read header
                              for(i=0; i<num_channels; i++){
                                 byte pred_i = *bp++;
                                 assert(pred_i<7);
                                 if(pred_i>=7)
                                    pred_i = 0;
                                 block_data[i].coef1 = coef_table[pred_i][0];
                                 block_data[i].coef2 = coef_table[pred_i][1];
                              }
                              for(i=0; i<num_channels; i++)
                                 block_data[i].delta = S_hlp::ReadShort(bp);
                              for(i=0; i<num_channels; i++)
                                 block_data[i].sample1 = S_hlp::ReadShort(bp);
                              for(i=0; i<num_channels; i++)
                                 block_data[i].sample2 = S_hlp::ReadShort(bp);
                                                   //store first 2 samples
                              for(i=0; i<num_channels; i++)
                                 *dst++ = (short)block_data[i].sample2;
                              for(i=0; i<num_channels; i++)
                                 *dst++ = (short)block_data[i].sample1;

                              for(int n=(samples_per_block-2)*num_channels; n && dst<dst_end; n -= 2){
                                 byte b = *bp++;
                                 dword lo = b&0xf;
                                 dword hi = b>>4;

                                 *dst++ = short(block_data[0].DecodeSample(byte(hi)));
                                 if(dst==dst_end)
                                    break;
                                 *dst++ = short(block_data[num_channels-1].DecodeSample(byte(lo)));
                              }
                           }
                           delete[] (byte*)data;
                           data = new_data;
                           bytes_per_sample = 2;
                           format = FORMAT_PCM;
                        }
                        break;
#endif   //!__SYMBIAN32__
                     }
                     return true;
                  }
                  fl.Seek(pos+chk.chunk_size);
               }
            }
         }
      }
   }
   return false;
}

//----------------------------

C_sound_source *CreateSoundSourceInternal(){
   return new(true) C_sound_source_imp;
}

//----------------------------

class C_sound_imp: public C_sound{
                              //fixed-point values are in 24:8 format

   C_smart_ptr<C_sound_source_imp> src;
   bool is_playing;
   int play_pos_fp;
   C_fixed volume;
   int resample_ratio;        //ratio original_rate : device_rate

   int adpcm_last;

   friend class C_sound_player_imp;

//----------------------------

   virtual void MixIntoBuffer(short *buf, dword len, C_fixed mix_volume);

//----------------------------
public:
   C_sound_imp(C_sound_player *_d):
      is_playing(false),
      volume(1)
   {
      drv = _d;
   }
private:
   ~C_sound_imp(){
      Stop();
   }

   virtual void Open(C_sound_source *_src){
      Stop();
      src = (C_sound_source_imp*)_src;
      if(src!=NULL){
         C_fixed ss, dr;
         ss.val = src->SampleRate()<<12;
         dr.val = drv->GetPlaybackRate()<<12;
         C_fixed r = ss / dr;
         resample_ratio = r.val>>8;
      }
   }

//----------------------------

   virtual void Play(){

      if(src!=NULL && drv){
         if(!is_playing){
            is_playing = true;
            play_pos_fp = 0;
            adpcm_last = 0;
            drv->RegisterSoundWriter(this);
         }else{
            play_pos_fp = 0;
            adpcm_last = 0;
         }
      }
   }

//----------------------------

   virtual void Stop(){
      if(is_playing){
         if(drv)
            drv->UnregisterSoundWriter(this);
         is_playing = false;
      }
   }

//----------------------------

   virtual bool IsPlaying() const{
      return is_playing;
   }

//----------------------------

   virtual void SetVolume(C_fixed v){ volume = v; }
   virtual C_fixed GetVolume() const{ return volume; }

//----------------------------

   virtual dword GetPlayTime() const{
      return src->GetPlayTime();
   }
   virtual dword GetPlayPos() const{
      return (play_pos_fp>>8)*1000/src->SampleRate();
   }
   virtual void SetPlayPos(dword pos){
      play_pos_fp = Min(pos*src->SampleRate()/1000, src->NumSamples())<<8;
   }

};

//----------------------------

void C_sound_imp::MixIntoBuffer(short *buf, dword len, C_fixed mix_volume){

   int num_smp_fp = src->NumSamples() << 8;
   int raw_vol = (volume * mix_volume).val;
#ifdef _DEBUG
   //raw_vol = 0x10000;
#endif

   dword num_src_channels = src->NumChannels();
   dword num_dst_channels = drv->GetnumChannels();
   if(num_dst_channels==2)
      len /= 2;

   switch(src->GetFormat()){
   case C_sound_source_imp::FORMAT_PCM:
      switch(src->Bpp()){
      case 1:
         {
            const schar *src_data = (schar*)src->Data();
            while(len--){
               if(num_dst_channels==1){
                  if(num_src_channels==1){
                     int s = ((src_data[play_pos_fp>>8] * raw_vol) >> 8);
                     s += *buf;
                     *buf++ = short(s);
                  }else{
                     int s = ((byte(src_data[(play_pos_fp>>8)*2]) * raw_vol) >> 8);
                     s += ((byte(src_data[(play_pos_fp>>8)*2+1]) * raw_vol) >> 8);
                     s += *buf;
                     *buf++ = short(s);
                  }
               }else{
                  if(num_src_channels==1){
                     int s = ((src_data[play_pos_fp>>8] * raw_vol) >> 8);
                     s += *buf;
                     *buf++ = short(s);
                     s += *buf;
                     *buf++ = short(s);
                  }else{
                     int s = ((byte(src_data[(play_pos_fp>>8)*2]) * raw_vol) >> 8);
                     s += *buf;
                     *buf++ = short(s);
                     s = ((byte(src_data[(play_pos_fp>>8)*2+1]) * raw_vol) >> 8);
                     s += *buf;
                     *buf++ = short(s);
                  }
               }
               if((play_pos_fp += resample_ratio) >= num_smp_fp){
                  drv->UnregisterSoundWriter(this, false);
                  is_playing = false;
                  return;
               }
            }
         }
         break;
      case 2:
         {
            const short *src_data = (short*)src->Data();
            while(len--){
               if(num_dst_channels==1){
                  if(num_src_channels==1){
                     int s = ((src_data[play_pos_fp>>8] * raw_vol) >> 16);
                     s += *buf;
                     *buf++ = short(s);
                  }else{
                     int s = ((src_data[(play_pos_fp>>8)*2] * raw_vol) >> 16);
                     s += ((src_data[(play_pos_fp>>8)*2+1] * raw_vol) >> 16);
                     s += *buf;
                     *buf++ = short(s);
                  }
               }else{
                  if(num_src_channels==1){
                     int s = ((src_data[play_pos_fp>>8] * raw_vol) >> 16);
                     s += *buf;
                     *buf++ = short(s);
                     s += *buf;
                     *buf++ = short(s);
                  }else{
                     int s = ((src_data[(play_pos_fp>>8)*2] * raw_vol) >> 16);
                     s += *buf;
                     *buf++ = short(s);
                     s = ((src_data[(play_pos_fp>>8)*2+1] * raw_vol) >> 16);
                     s += *buf;
                     *buf++ = short(s);
                  }
               }
               if((play_pos_fp += resample_ratio) >= num_smp_fp){
                  drv->UnregisterSoundWriter(this, false);
                  is_playing = false;
                  return;
               }
            }
         }
         break;
      default:
         assert(0);
      }
      break;
/*
   case C_sound_source_imp::FORMAT_MULAW:
   case C_sound_source_imp::FORMAT_ALAW:
      if(bpp==1){
         static const short mulaw_tab[256] = {
            -32124,-31100,-30076,-29052,-28028,-27004,-25980,-24956, -23932,-22908,-21884,-20860,-19836,-18812,-17788,-16764, -15996,-15484,-14972,-14460,-13948,-13436,-12924,-12412, -11900,-11388,-10876,-10364, -9852, -9340, -8828, -8316,
            -7932, -7676, -7420, -7164, -6908, -6652, -6396, -6140, -5884, -5628, -5372, -5116, -4860, -4604, -4348, -4092, -3900, -3772, -3644, -3516, -3388, -3260, -3132, -3004, -2876, -2748, -2620, -2492, -2364, -2236, -2108, -1980,
            -1884, -1820, -1756, -1692, -1628, -1564, -1500, -1436, -1372, -1308, -1244, -1180, -1116, -1052,  -988,  -924, -876,  -844,  -812,  -780,  -748,  -716,  -684,  -652, -620,  -588,  -556,  -524,  -492,  -460,  -428,  -396,
            -372,  -356,  -340,  -324,  -308,  -292,  -276,  -260, -244,  -228,  -212,  -196,  -180,  -164,  -148,  -132, -120,  -112,  -104,   -96,   -88,   -80,   -72,   -64, -56,   -48,   -40,   -32,   -24,   -16,    -8,     0,
            32124, 31100, 30076, 29052, 28028, 27004, 25980, 24956, 23932, 22908, 21884, 20860, 19836, 18812, 17788, 16764, 15996, 15484, 14972, 14460, 13948, 13436, 12924, 12412, 11900, 11388, 10876, 10364,  9852,  9340,  8828,  8316,
            7932,  7676,  7420,  7164,  6908,  6652,  6396,  6140, 5884,  5628,  5372,  5116,  4860,  4604,  4348,  4092, 3900,  3772,  3644,  3516,  3388,  3260,  3132,  3004, 2876,  2748,  2620,  2492,  2364,  2236,  2108,  1980,
            1884,  1820,  1756,  1692,  1628,  1564,  1500,  1436, 1372,  1308,  1244,  1180,  1116,  1052,   988,   924, 876,   844,   812,   780,   748,   716,   684,   652, 620,   588,   556,   524,   492,   460,   428,   396,
            372,   356,   340,   324,   308,   292,   276,   260, 244,   228,   212,   196,   180,   164,   148,   132, 120,   112,   104,    96,    88,    80,    72,    64, 56,    48,    40,    32,    24,    16,     8,     0
         }, alaw_tab[256] = {
            -5504, -5248, -6016, -5760, -4480, -4224, -4992, -4736, -7552, -7296, -8064, -7808, -6528, -6272, -7040, -6784, -2752, -2624, -3008, -2880, -2240, -2112, -2496, -2368, -3776, -3648, -4032, -3904, -3264, -3136, -3520, -3392,
            -22016,-20992,-24064,-23040,-17920,-16896,-19968,-18944, -30208,-29184,-32256,-31232,-26112,-25088,-28160,-27136, -11008,-10496,-12032,-11520,-8960, -8448, -9984, -9472, -15104,-14592,-16128,-15616,-13056,-12544,-14080,-13568,
            -344,  -328,  -376,  -360,  -280,  -264,  -312,  -296, -472,  -456,  -504,  -488,  -408,  -392,  -440,  -424, -88,   -72,   -120,  -104,  -24,   -8,    -56,   -40, -216,  -200,  -248,  -232,  -152,  -136,  -184,  -168,
            -1376, -1312, -1504, -1440, -1120, -1056, -1248, -1184, -1888, -1824, -2016, -1952, -1632, -1568, -1760, -1696, -688,  -656,  -752,  -720,  -560,  -528,  -624,  -592, -944,  -912,  -1008, -976,  -816,  -784,  -880,  -848,
            5504,  5248,  6016,  5760,  4480,  4224,  4992,  4736, 7552,  7296,  8064,  7808,  6528,  6272,  7040,  6784, 2752,  2624,  3008,  2880,  2240,  2112,  2496,  2368, 3776,  3648,  4032,  3904,  3264,  3136,  3520,  3392,
            22016, 20992, 24064, 23040, 17920, 16896, 19968, 18944, 30208, 29184, 32256, 31232, 26112, 25088, 28160, 27136, 11008, 10496, 12032, 11520, 8960,  8448,  9984,  9472, 15104, 14592, 16128, 15616, 13056, 12544, 14080, 13568,
            344,   328,   376,   360,   280,   264,   312,   296, 472,   456,   504,   488,   408,   392,   440,   424, 88,    72,   120,   104,    24,     8,    56,    40, 216,   200,   248,   232,   152,   136,   184,   168,
            1376,  1312,  1504,  1440,  1120,  1056,  1248,  1184, 1888,  1824,  2016,  1952,  1632,  1568,  1760,  1696, 688,   656,   752,   720,   560,   528,   624,   592, 944,   912,  1008,   976,   816,   784,   880,   848
         };
         const short *tab = src->GetFormat()==C_sound_source_imp::FORMAT_MULAW ? mulaw_tab : alaw_tab;
         const byte *src_data = (byte*)src->Data();
         int s0, s1(0);
         while(len--){
            s0 = ((tab[src_data[(play_pos_fp>>8)*num_src_channels]] * raw_vol) >> 16);
            if(num_src_channels==2)
               s1 = (tab[src_data[(play_pos_fp>>8)*2+1]] * raw_vol) >> 16;
            if(num_dst_channels==1){
               if(num_src_channels==2)
                  s0 = (s0+s1)/2;
               *buf++ = short(*buf+s0);
            }else{
               if(num_src_channels==1)
                  s1 = s0;
               *buf++ = short(*buf+s0);
               *buf++ = short(*buf+s1);
            }
            if((play_pos_fp += resample_ratio) >= num_smp_fp){
               drv->UnregisterSoundWriter(this, false);
               is_playing = false;
               return;
            }
         }
      }else
         assert(0);
      break;
      */
   }
}

//----------------------------

C_sound *CreateSoundInternal(C_sound_player *plr){
   return new(true) C_sound_imp(plr);
}

//----------------------------

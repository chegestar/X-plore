//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#define SUPPORT_LOCK

#ifdef SUPPORT_LOCK
#if defined __SYMBIAN32__
#include <E32Base.h>
#elif defined BADA
#include <Util.h>
#include <FBaseRtThreadMutex.h>
#elif defined ANDROID
#else
namespace win{
#include <Windows.h>
#undef SOCKET_ERROR
}
#undef DrawText
#endif
#endif//SUPPORT_LOCK


#include <Profiler.h>
#include <IGraph.h>
#include <Util.h>
#include <Cstr.h>
#include <UI\UserInterface.h>

//----------------------------

static void TestFunc();

//----------------------------

C_profiler::C_profiler(const char *n, t_DrawFunc *df, void *dfc):
   sample_index(-1),
   num_samples(0),
   avg_count(0),
   names(n),
   lock(NULL),
   draw_func(df), draw_func_context(dfc),
   in_blocks(0)
{
#ifdef SUPPORT_LOCK
#ifdef __SYMBIAN32__
   RCriticalSection *smp = new(true) RCriticalSection;
   smp->CreateLocal();
   lock = smp;
#elif defined BADA
   lock = new(true) Osp::Base::Runtime::Mutex;
   ((Osp::Base::Runtime::Mutex*)lock)->Create();
#elif defined ANDROID
#else
   win::CRITICAL_SECTION *cs = new win::CRITICAL_SECTION;
   InitializeCriticalSection(cs);
   lock = cs;
#endif
#endif //SUPPORT_LOCK
   Reset();
   Start();
                              //calibrate
   for(int i=0; i<0x10000; i++){
      MarkS(0);
      TestFunc();
      MarkE(0);
   }
   calibrate_val = channels[0].avg_sum;
   Reset();
}

static void TestFunc(){}

//----------------------------

C_profiler::~C_profiler(){
#ifdef SUPPORT_LOCK
#ifdef __SYMBIAN32__
   RCriticalSection *smp = (RCriticalSection*)lock;
   smp->Close();
   delete smp;
#elif defined BADA
   delete ((Osp::Base::Runtime::Mutex*)lock);
#elif defined ANDROID
#else
   win::CRITICAL_SECTION *cs = (win::CRITICAL_SECTION*)lock;
   DeleteCriticalSection(cs);
   delete cs;
#endif
#endif//SUPPORT_LOCK
}

//----------------------------

void C_profiler::Lock(){
#ifdef SUPPORT_LOCK
#ifdef __SYMBIAN32__
   RCriticalSection *smp = (RCriticalSection*)lock;
   smp->Wait();
#elif defined BADA
   ((Osp::Base::Runtime::Mutex*)lock)->Acquire();
#elif defined ANDROID
#else
   win::CRITICAL_SECTION *cs = (win::CRITICAL_SECTION*)lock;
   EnterCriticalSection(cs);
#endif
#endif
}

void C_profiler::Unlock(){
#ifdef SUPPORT_LOCK
#ifdef __SYMBIAN32__
   RCriticalSection *smp = (RCriticalSection*)lock;
   smp->Signal();
#elif defined BADA
   ((Osp::Base::Runtime::Mutex*)lock)->Release();
#elif defined ANDROID
#else
   win::CRITICAL_SECTION *cs = (win::CRITICAL_SECTION*)lock;
   LeaveCriticalSection(cs);
#endif
#endif
}

//----------------------------

void C_profiler::MarkS(dword i, bool use_lock){

   if(use_lock) Lock();
   if(i>NUM_CHANNELS)
      Fatal("Prof: wrong index", i);
   if(in_blocks&(1<<i))
      Fatal("Prof: in block", i);

   ++channels[i].call_count;
   in_blocks |= 1<<i;
   channels[i].beg_time = GetTickTime();
}

//----------------------------

void C_profiler::MarkE(dword i, bool use_lock){

   int t = GetTickTime();
   if(i>NUM_CHANNELS)
      Fatal("Prof: wrong index", i);
   if(!(in_blocks&(1<<i)))
      Fatal("Prof: no block", i);
   t -= channels[i].beg_time;
   channels[i].samples[sample_index] += t;
   channels[i].avg_sum += t;
   in_blocks &= ~(1<<i);
   if(use_lock) Unlock();
}
//----------------------------

void C_profiler::End(dword show_mask, C_application_ui *app, dword color, bool show_max, bool show_avg){

   Lock();
   if(in_blocks){
      //Fatal("Prof: in blocks", in_blocks);
   }
                              //calibrate values
   const char *n = names;
   for(int i=0; i<NUM_CHANNELS && *n; i++, n += StrLen(n)+1){
      S_channel &chn = channels[i];
      if(chn.call_count){
         int c = (calibrate_val * chn.call_count) >> 16;
         int &s = chn.samples[sample_index];
         c = Min(c, s);
         s -= c;
         chn.avg_sum -= c;
      }
   }
   if(num_samples<S_channel::NUM_SAMPLES)
      ++num_samples;

   Write(app, show_mask, color, show_max, show_avg);
   Start();
   if(in_blocks){
      //Fatal("Prof: in blocks", in_blocks);
   }
   Unlock();
}

//----------------------------

void C_profiler::Start(){

   if(++sample_index==S_channel::NUM_SAMPLES)
      sample_index = 0;
   for(int i=NUM_CHANNELS; i--; ){
      channels[i].samples[sample_index] = 0;
      channels[i].call_count = 0;
   }
   ++avg_count;
}

//----------------------------

void C_profiler::Write(C_application_ui *app, dword show_mask, dword color, bool show_max, bool show_avg) const{

   const char *n = names;
   for(int i=0, y = 0; i<NUM_CHANNELS && *n; i++, n += StrLen(n)+1){
      if(show_mask&(1<<i)){
         int cnt = 0;
         const S_channel &chn = channels[i];
                              //don't display channels with zero count
         if(!chn.call_count && !chn.avg_sum && (!show_max || !chn.max_time))
            continue;
         for(int j=num_samples; j--; )
            cnt += chn.samples[j];
         cnt = cnt / num_samples;
         Cstr_w s; s.Copy(n);
                              //calculate maximums
         chn.max_time = Max(chn.max_time, cnt);

         s.AppendFormat(L": % (%)") <<cnt <<chn.call_count;
         if(show_max)
            s.AppendFormat(L" ^%") <<chn.max_time;
         if(show_avg && avg_count){
                              //ms * 10
            int a = chn.avg_sum*10 / avg_count;
            int d = a/10;
            s.AppendFormat(L" ~%.%") <<d <<(a-d*10);
         }
         if(app)
            app->DrawString(s, 0, app->GetFontDef(app->UI_FONT_SMALL).line_spacing*y, app->UI_FONT_SMALL, 0, color);
         else
         if(draw_func)
            (*draw_func)(s, 0, y, draw_func_context);
         y++;
      }
   }
}

//----------------------------

void C_profiler::WriteStatic(C_application_ui *app, dword show_mask){

   num_samples = 1;
   Write(app, show_mask, 0xffffffff, false, false);
}

//----------------------------

void C_profiler::Reset(){
   MemSet(channels, 0, sizeof(channels));
   num_samples = 0;
   avg_count = 0;
   sample_index = 0;
}

//----------------------------

void C_profiler::ResetMax(){
   for(int i=0; i<NUM_CHANNELS; i++)
      channels[i].max_time = 0;
}

//----------------------------

void C_profiler::ResetAverage(){
   avg_count = 0;
   for(int i=0; i<NUM_CHANNELS; i++)
      channels[i].avg_sum = 0;
}

//----------------------------

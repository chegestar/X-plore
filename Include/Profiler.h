//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#ifndef __PROFILER_H_
#define __PROFILER_H_

#include <Rules.h>

//----------------------------

class C_profiler{
   enum{
      NUM_CHANNELS = 32,
   };
   typedef void (t_DrawFunc)(const wchar *txt, int x, int y, void *context);

   int sample_index;
   int num_samples;
   struct S_channel{
      enum{
         NUM_SAMPLES = 32,
      };
      int samples[NUM_SAMPLES];
      int avg_sum;
      int beg_time;
      int call_count;
      mutable int max_time;
   } channels[NUM_CHANNELS];
   int avg_count;
   const char *names;
   dword in_blocks;
   dword calibrate_val;

   t_DrawFunc *draw_func;
   void *draw_func_context;

   void *lock;

   void Start();
   void Write(class C_application_ui *app, dword show_mask, dword color, bool show_max, bool show_avg) const;

   void Lock();
   void Unlock();

public:
   C_profiler(const char *n, t_DrawFunc *draw_func = NULL, void *draw_func_context = NULL);
   ~C_profiler();
   void MarkS(dword i, bool lock = false);
   void MarkE(dword i, bool lock = false);

   void End(dword show_mask = 0xffffffff, class C_application_ui *app = NULL, dword color = 0xffffff, bool show_max = true, bool show_avg = false);
   void WriteStatic(C_application_ui *app, dword show_mask);
   void Reset();
   void ResetMax();
   void ResetAverage();
};

//----------------------------
#endif

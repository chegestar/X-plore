//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <E32Std.h>
#include <TimeDate.h>

//----------------------------

void S_date_time::GetCurrent(){

   TTime tt; tt.HomeTime();
   TDateTime st = tt.DateTime();
   year = word(st.Year());
   month = word(st.Month());
   day = word(st.Day());
   hour = word(st.Hour());
   minute = word(st.Minute());
   second = word(st.Second());

   sort_value = GetSeconds();
}

//----------------------------

static int FixTimeZoneMinute(int min){
   if(min<-12*60)
      min += 24*60;
   else
   if(min>13*60)
      min -= 24*60;
   return min;
}

//----------------------------

int S_date_time::GetTimeZoneMinuteShift(){

   TTime tt;
   TInt64 th, tu;
   tt.HomeTime();
   th = tt.Int64();
   tt.UniversalTime();
   tu = tt.Int64();
#ifdef __SYMBIAN_3RD__
   int min = int((th-tu)/TInt64(60000000));
#else
   int min = ((th-tu)/TInt64(60000000)).Low();
#endif
   return FixTimeZoneMinute(min);
}

//----------------------------

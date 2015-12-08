//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <TimeDate.h>
#include <Util.h>

//----------------------------

static const byte days_in_month[] = {
   31, 28, 31, 30, 31, 30, 31,
   31, 30, 31, 30, 31, 30, 31
};

//----------------------------

void S_date_time::Clear(){
   MemSet(this, 0, sizeof(S_date_time));
}

//----------------------------

dword S_date_time::GetSeconds() const{

   int days = (year-1980)*365 + (year-1977)/4;
   days = Max(days, 0);
   assert(month<12);
   for(int i=month; i--; ){
      days += days_in_month[i];
      if(i==1){
                              //february has sometimes more
         if((year&3)==0 && ((year%100)!=0 || (year%400)==0))
            ++days;
      }
   }
   days += day;
   assert(hour<24);
   assert(minute<60);
   assert(second<60);
   return ((days*24 + hour) * 60 + minute) * 60 + second;
}

//----------------------------

void S_date_time::SetFromSeconds(dword val){

   int _days = int(val / dword(60*60*24));
   int _seconds = val - _days * (60*60*24);
                              //get H:M:S
   hour = word(_seconds / (60*60));
   _seconds -= hour * (60*60);
   minute = word(_seconds / 60);
   second = word(_seconds - minute * 60);
                              //get date from days
   for(year=1980; ; year++){
      int year_days = 365;
      if(!(year&3))
         ++year_days;
      if(_days<year_days)
         break;
      _days -= year_days;
   }
   for(month=0; ; month++){
      int month_days = days_in_month[month];
      if(month==1 && !(year&3))
         ++month_days;
      if(_days<month_days)
         break;
      _days -= month_days;
   }
   assert(month<12);
   day = word(_days);
   assert(day<32);
   assert(val==GetSeconds());
   sort_value = val;
}

//----------------------------

int S_date_time::Compare(const S_date_time &dt) const{

   if(sort_value < dt.sort_value) return -1;
   if(sort_value > dt.sort_value) return 1;
   return 0;
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

#if defined _WINDOWS || defined _WIN32_WCE
namespace win{
#include <Windows.h>
}

void S_date_time::GetCurrent(){

   win::SYSTEMTIME st;
   win::GetLocalTime(&st);
   year = st.wYear;
   month = st.wMonth-1;
   day = st.wDay-1;
   hour = st.wHour;
   minute = st.wMinute;
   second = st.wSecond;

   sort_value = GetSeconds();
}

//----------------------------

int S_date_time::GetTimeZoneMinuteShift(){

   win::SYSTEMTIME st;
   win::GetLocalTime(&st);
   long long ftl, fts;
   win::SystemTimeToFileTime(&st, (win::FILETIME*)&ftl);
   win::GetSystemTime(&st);
   win::SystemTimeToFileTime(&st, (win::FILETIME*)&fts);
   int min = int((ftl-fts)/(long long)600000000);
   return FixTimeZoneMinute(min);
}

#endif
//----------------------------
#ifdef BADA
#include <FBaseDateTime.h>
#include <FSysSystemTime.h>

void S_date_time::GetCurrent(){

   Osp::Base::DateTime st;
   Osp::System::SystemTime::GetCurrentTime(Osp::System::WALL_TIME, st);
   year = word(st.GetYear());
   month = word(st.GetMonth()-1);
   day = word(st.GetDay()-1);
   hour = word(st.GetHour());
   minute = word(st.GetMinute());
   second = word(st.GetSecond());

   sort_value = GetSeconds();
}

//----------------------------

int S_date_time::GetTimeZoneMinuteShift(){

   Osp::Base::DateTime utc, wall;
   Osp::System::SystemTime::GetCurrentTime(Osp::System::WALL_TIME, wall);
   Osp::System::SystemTime::GetCurrentTime(Osp::System::UTC_TIME, utc);
   Osp::Base::TimeSpan ts = wall.GetTime() - utc.GetTime();

   int min = ts.GetMinutes();
   return FixTimeZoneMinute(min);
}

#endif
//----------------------------
#ifdef ANDROID
#include <time.h>
#include <linux\time.h>
#include <Android\GlobalData.h>

void S_date_time::GetCurrent(){

   time_t t = time(NULL);
   struct tm *tm = localtime(&t);

   year = word(1900+tm->tm_year);
   month = word(tm->tm_mon);
   day = word(tm->tm_mday-1);
   hour = word(tm->tm_hour);
   minute = word(tm->tm_min);
   second = word(tm->tm_sec);
   sort_value = GetSeconds();
}

//----------------------------

int S_date_time::GetTimeZoneMinuteShift(){

#if 1
   JNIEnv &env = jni::GetEnv();

   jni::LClass cls_TimeZone = jni::FindClass("java/util/TimeZone");
   jobject tz = env.CallStaticObjectMethod(cls_TimeZone, env.GetStaticMethodID(cls_TimeZone, "getDefault", "()Ljava/util/TimeZone;"));
   int min = env.CallIntMethod(tz, env.GetMethodID(cls_TimeZone, "getRawOffset", "()I"));
   jni::LClass cls_Date = jni::FindClass("java/util/Date");
   jobject date = env.NewObject(cls_Date, env.GetMethodID(cls_Date, "<init>", "()V"));
   if(env.CallBooleanMethod(tz, env.GetMethodID(cls_TimeZone, "inDaylightTime", "(Ljava/util/Date;)Z"), date))
      min += env.CallIntMethod(tz, env.GetMethodID(cls_TimeZone, "getDSTSavings", "()I"));
   min /= 1000*60;
   env.DeleteLocalRef(date);
   env.DeleteLocalRef(tz);
#else
   timeval tv;
   struct timezone tz;
   if(gettimeofday(&tv, &tz))
      return 0;
   int min = -tz.tz_minuteswest;
#endif
   return FixTimeZoneMinute(min);
}

#endif
//----------------------------

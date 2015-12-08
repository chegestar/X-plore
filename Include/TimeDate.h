//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#ifndef __TIME_DATE_H
#define __TIME_DATE_H
#include <Rules.h>

//----------------------------

struct S_date_time{
   enum{              //named months for better imagination
      JAN, FEB, MAR, APR, MAY, JUN, JUL, AUG, SEP, OCT, NOV, DEC
   };
                              //month and day are zero-indexed
   word year, month, day;
   word hour, minute, second;

   dword sort_value;          //seconds since 1.1.1980

   void Clear();

//----------------------------
// Compare with other time-date value.
   int Compare(const S_date_time &dt) const;

//----------------------------
// Get seconds since 1.1.1980.
// It returns valid data only after struct was properly initialized by setting values or calling SetFromSeconds or GetCurrent.
   dword GetSeconds() const;

//----------------------------
// Set from seconds since 1.1.1980
   void SetFromSeconds(dword sec);

//----------------------------
// Determine current time/date, and store into this class.
   void GetCurrent();

//----------------------------
   inline void MakeSortValue(){ sort_value = GetSeconds(); }

//----------------------------
// Get difference of local time-zone from zero time-zone, in minutes. Returned value is -720 - +780.
   static int GetTimeZoneMinuteShift();
};

//----------------------------
#endif

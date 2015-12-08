//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------
// Math functions.

#ifndef _C_MATH_H_
#define _C_MATH_H_

#include <Rules.h>
#include <Fixed.h>

//----------------------------

class C_math{
public:
   static float FixedToFloat(C_fixed f);
   static C_fixed FloatToFixed(float f);
   static C_fixed PI();
   static C_fixed Sin(C_fixed f);
   static C_fixed Asin(C_fixed f);
   static C_fixed Cos(C_fixed f);
   static C_fixed Acos(C_fixed f);
   static C_fixed Sqrt(C_fixed f);
   static C_fixed Sqrt(int v);
};

//----------------------------

#endif

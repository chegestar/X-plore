//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#ifndef __FIXED_H
#define __FIXED_H

#include <Rules.h>

//----------------------------

class C_fixed{
public:
   int val;

   inline C_fixed(){}
   C_fixed(int i);
   inline C_fixed(dword i):
      val(i<<16)
   { }
   inline C_fixed(float f):
      val(int(f*65536.f))
   {}

//----------------------------
// Convert value to integer. Rounding is done down to nearest whole value.
   inline operator int() const{ return (val>>16); }

//----------------------------
// Round to nearest integer value.
   int RoundToNearest() const;

//----------------------------
// Various math operators.
   inline C_fixed operator -() const{
      C_fixed r;
      r.val = -val;
      return r;
   }

   C_fixed operator +(C_fixed f) const;
   C_fixed operator -(C_fixed f) const;

   C_fixed operator *(C_fixed f) const;
   C_fixed operator /(C_fixed f) const;

   C_fixed &operator +=(C_fixed f);
   C_fixed &operator -=(C_fixed f);
   C_fixed &operator *=(C_fixed f);

   //inline C_fixed &operator /=(C_fixed f);

   inline bool operator <(C_fixed f) const{ return (val < f.val); }
   inline bool operator <=(C_fixed f) const{ return (val <= f.val); }
   inline bool operator >(C_fixed f) const{ return (val > f.val); }
   inline bool operator >=(C_fixed f) const{ return (val >= f.val); }
   inline bool operator ==(C_fixed f) const{ return (val == f.val); }
   inline bool operator !=(C_fixed f) const{ return (val != f.val); }

   inline bool operator <(int f) const{ return (operator int() < f); }
   inline bool operator <=(int f) const{ return (operator int() <= f); }
   inline bool operator >(int f) const{ return (operator int() > f); }
   inline bool operator >=(int f) const{ return (operator int() >= f); }
   inline bool operator ==(int f) const{ return (operator int() == f); }
   inline bool operator !=(int f) const{ return (operator int() != f); }

//----------------------------
// Shifting value right/left.
   inline C_fixed operator >>(int c) const{ C_fixed r; r.val = val >> c; return r; }
   C_fixed operator <<(int c) const;
   inline void operator >>=(int c){ val >>= c; }
   void operator <<=(int c);

//----------------------------
// Get fractional value of this.
   C_fixed Frac() const;

//----------------------------
// Get absolute value of this.
   inline C_fixed Abs() const{
      return (val>=0) ? *this : -*this;
   }

//----------------------------
// Predefined fixed-point constants.
   static inline C_fixed Zero(){ return C_fixed(0); }
   static inline C_fixed One(){ return C_fixed(1); }
   static inline C_fixed Two(){ return C_fixed(2); }
   static inline C_fixed Three(){ return C_fixed(3); }
   static inline C_fixed Four(){ return C_fixed(4); }
   static inline C_fixed Half(){ C_fixed r; r.val = 0x8000; return r; }
   static inline C_fixed Third(){ C_fixed r; r.val = 0x5555; return r; }
   static inline C_fixed Quarter(){ C_fixed r; r.val = 0x4000; return r; }

//----------------------------
// Create fixed-point value from percentual number. Value of n=100 will create fixed-point number 1.0.
   static C_fixed Percent(int n);

//----------------------------
// Get reciprocal value.
   inline C_fixed Reciprocal() const{ return One() / *this; }

//----------------------------
// Create fixed-point value by directly set 16.16 internal value. Value of n=0x10000 will create fixed-point number 1.0.
   static inline C_fixed Create(int n){ C_fixed r; r.val = n; return r; }
};

//----------------------------

#endif

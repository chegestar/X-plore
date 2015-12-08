//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <C_math.h>
#include <Util.h>
#include <math.h>

//----------------------------

C_fixed C_math::PI(){
   return C_fixed::Create(205887);
}

//----------------------------

float C_math::FixedToFloat(C_fixed f){

   dword v = 0;
   if(f.val){
                              //sign
      if(f.val<0){
         f.val = -f.val;
         v |= 0x80000000;
      }
      int hb = FindHighestBit(f.val);
                              //clear highest bit of val (explicitly set to one in float-point)
      f.val ^= 1<<hb;
      dword exp = 127-16+hb;
      v |= exp<<23;
      if(hb<=23)
         v |= (f.val<<(23-hb))&0x7fffff;
      else
         v |= f.val>>(hb-23);
   }
   return *(float*)&v;
}

//----------------------------

C_fixed C_math::FloatToFixed(float f){

   C_fixed v;
   dword fv = *(dword*)&f;
   if(fv){
      v.val = (1<<23) | (fv&0x7fffff);
      int exp = (fv>>23) & 0xff;
      exp -= 127+7;
      if(exp>=0)
         v.val <<= exp;
      else
         v.val >>= -exp;
      if(fv&0x80000000)
         v.val = -v.val;
   }else
      v.val = 0;
   return v;
}

//----------------------------

static C_fixed CallFloatFnc(C_fixed f, double (*Fnc)(double)){
   float v = C_math::FixedToFloat(f);
   v = (float)Fnc(v);
   return C_math::FloatToFixed(v);
}

C_fixed C_math::Sin(C_fixed f){
   return CallFloatFnc(f, &sin);
}
C_fixed C_math::Asin(C_fixed f){
   return CallFloatFnc(f, &asin);
}
C_fixed C_math::Cos(C_fixed f){
   return CallFloatFnc(f, &cos);
}
C_fixed C_math::Acos(C_fixed f){
   return CallFloatFnc(f, &acos);
}
C_fixed C_math::Sqrt(C_fixed f){
   return CallFloatFnc(f, &sqrt);
}
C_fixed C_math::Sqrt(int v){
   return FloatToFixed((float)sqrt(double(v)));
}

//----------------------------
#ifdef BADA
#include <FBase.h>

static C_fixed CallFloatFnc(C_fixed f, double (*fnc)(double)){
   float v = (float)fnc(C_math::FixedToFloat(f));
   return C_math::FloatToFixed(v);
}

C_fixed C_math::Sin(C_fixed f){
   return CallFloatFnc(f, &Osp::Base::Utility::Math::Sin);
}
C_fixed C_math::Asin(C_fixed f){
   return CallFloatFnc(f, &Osp::Base::Utility::Math::Asin);
}
C_fixed C_math::Cos(C_fixed f){
   return CallFloatFnc(f, &Osp::Base::Utility::Math::Cos);
}
C_fixed C_math::Acos(C_fixed f){
   return CallFloatFnc(f, &Osp::Base::Utility::Math::Acos);
}
C_fixed C_math::Sqrt(C_fixed f){
   return CallFloatFnc(f, &Osp::Base::Utility::Math::Sqrt);
}
C_fixed C_math::Sqrt(int v){
   return FloatToFixed((float)Osp::Base::Utility::Math::Sqrt(double(v)));
}

#endif
//----------------------------

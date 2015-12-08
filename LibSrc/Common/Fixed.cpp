//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <Fixed.h>

#if defined _DEBUG && defined _MSC_VER && !defined ARM
//#ifdef __SYMBIAN32__
#define DEBUG_FP
//#endif
#endif

//----------------------------

C_fixed::C_fixed(int i)
   :val(i<<16)
{
#ifdef DEBUG_FP
   if(i>0x7fff || i<-0x8000){
      __asm int 3
   }
#endif
}

//----------------------------

int C_fixed::RoundToNearest() const{

   if(val<0)
      return -(-(*this)).RoundToNearest();
   return int(*this+C_fixed::Half());
}

//----------------------------

void C_fixed::operator <<=(int c){
#ifdef DEBUG_FP
   longlong rr = longlong(val) << c;
   if(rr>=longlong(0x80000000) || rr<-longlong(0x80000000)){
      __asm int 3
   }
#endif
   val <<= c;
}

//----------------------------

C_fixed C_fixed::Frac() const{
   C_fixed ret;
   ret.val = val & 0xffff;
   if(val<0 && ret.val)
      ret.val |= 0xffff0000;
   return ret;
}

//----------------------------

/*
C_fixed::C_fixed(float f):
   val(int(f*65536.0f))
{
#ifdef DEBUG_FP
   if(f>32767.0f || f<-32768.0f){
      __asm int 3
   }
#endif
}
*/

//----------------------------

C_fixed C_fixed::Percent(int n){
   C_fixed r;
   r.val = (n*41943+32)>>6;
   return r;
}

//----------------------------

C_fixed C_fixed::operator +(C_fixed f) const{
#ifdef DEBUG_FP
   longlong rr = longlong(val) + longlong(f.val);
   if(rr>=longlong(0x80000000) || rr<-longlong(0x80000000)){
      __asm int 3
   }
#endif
   C_fixed r; r.val = val+f.val; return r;
}

//----------------------------

C_fixed C_fixed::operator -(C_fixed f) const{
#ifdef DEBUG_FP
   longlong rr = longlong(val) - longlong(f.val);
   if(rr>=longlong(0x80000000) || rr<-longlong(0x80000000)){
      __asm int 3
   }
#endif
   C_fixed r; r.val = val-f.val; return r;
}

//----------------------------

C_fixed C_fixed::operator *(C_fixed f) const{
#ifdef DEBUG_FP
   int hi = int((longlong(val) * longlong(f.val))>>32);
   if(hi>=32768 || hi<-32768){
      __asm int 3
   }
#endif
   C_fixed r;
#ifdef __MARM__
   dword tmp1, tmp2;
   asm(
      "smull %0, %1, %2, %3 \n\t"   //multiply, result is in [%0, %1]
      "mov %0, %0, lsr #16 \n\t"
      "orr %0, %0, %1, lsl #16 \n\t"
      : "=&r"(r.val), "=&r"(tmp1) : "r"(f), "r"(val));
#else
   r.val = int((longlong(f.val)*longlong(val))>>16);
#endif
   return r;
}

//----------------------------

C_fixed C_fixed::operator /(C_fixed f) const{
   C_fixed r;
   r.val = int((longlong(val)<<16) / longlong(f.val));
   return r;
}

//----------------------------

C_fixed &C_fixed::operator +=(C_fixed f){
#ifdef DEBUG_FP
   longlong rr = longlong(val) + longlong(f.val);
   if(rr>=longlong(0x80000000) || rr<-longlong(0x80000000)){
      __asm int 3
   }
#endif
   val += f.val; return *this;
}

//----------------------------

C_fixed &C_fixed::operator -=(C_fixed f){
#ifdef DEBUG_FP
   longlong rr = longlong(val) - longlong(f.val);
   if(rr>=longlong(0x80000000) || rr<-longlong(0x80000000)){
      __asm int 3
   }
#endif
   val -= f.val; return *this;
}

//----------------------------

C_fixed &C_fixed::operator *=(C_fixed f){
#ifdef DEBUG_FP
   int hi = int((longlong(val) * longlong(f.val))>>32);
   if(hi>=32768 || hi<-32768){
      __asm int 3
   }
#endif
#ifdef __MARM__
   dword tmp1, tmp2;
   asm(
      "smull %0, %1, %2, %3 \n\t"   //multiply, result is in [%0, %1]
      "mov %0, %0, lsr #16 \n\t"
      "orr %0, %0, %1, lsl #16 \n\t"
      : "=&r"(tmp2), "=&r"(tmp1) : "r"(f), "r"(val));
   val = tmp2;
#else
   val = int((longlong(f.val)*longlong(val))>>16);
#endif
   return *this;
}

//----------------------------

C_fixed C_fixed::operator <<(int c) const{
#ifdef DEBUG_FP
   longlong rr = longlong(val) << c;
   if(rr>=longlong(0x80000000) || rr<-longlong(0x80000000)){
      __asm int 3
   }
#endif
   C_fixed r; r.val = val << c; return r;
}

//----------------------------


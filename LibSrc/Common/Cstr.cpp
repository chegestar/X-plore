//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

//----------------------------
#if !defined _T
#include <Rules.h>
#include <Util.h>
#include <Cstr.h>
#include <TextUtils.h>

//----------------------------
// String base - concrete string data. This class' size is
// variable, depending on size of string it contains.
template<class T>
class Cstr_rep: public Cstr_rep_base{
   void SetRef(dword r){
      ref_len &= (0xffffffff<<REF_BITS);
      ref_len |= r;
   }
   void SetLen(dword l){
      ref_len &= ~(((1<<LEN_BITS)-1)<<REF_BITS);
      ref_len |= l<<REF_BITS;
   }
public:
   enum{
      REF_BITS = 8,
      LEN_BITS = 23,
   };
   inline Cstr_rep(const T *cp, size_t len){
      ref_len = 0;
      SetRef(1);
      SetLen(len);
      MemCpy(data, cp, len*sizeof(T));
      ((T*)data)[len] = 0;
   }
   inline Cstr_rep(size_t len){
      ref_len = 0;
      SetRef(1);
      SetLen(len);
      ((T*)data)[len] = 0;
   }
   inline Cstr_rep(const T *cp1, const T *cp2, size_t l1, size_t l2){
      ref_len = 0;
      SetRef(1);
      size_t len = l1+l2;
      SetLen(len);
      MemCpy(data, cp1, l1*sizeof(T));
      MemCpy(((T*)data)+l1, cp2, l2*sizeof(T));
      ((T*)data)[len] = 0;
   }
   inline void AddRef(){
      dword ref = Count();
      ++ref;
      if(ref>=(1<<REF_BITS))
         Fatal("Cstr rep overflow");
      SetRef(ref);
   }
   inline dword Release(){
      dword ref = Count();
      --ref;
      SetRef(ref);
      return ref;
   }
   inline dword Count() const{
      return ref_len & ((1<<REF_BITS)-1);
   }

   inline T *GetData(){ return ((T*)data); }
   inline const T *GetData() const{ return ((T*)data); }
   inline size_t Length() const{
      return (ref_len>>REF_BITS) & ((1<<LEN_BITS)-1);
   }
};

//----------------------------
template<class T>
static void *AllocRep(size_t len){
   return new(true) byte[sizeof(Cstr_rep<T>) + len*sizeof(T)];
}
//----------------------------
template<class T>
static void ReleaseRep(Cstr_rep<T> *rep){
   if(rep && !rep->Release())
      delete[] (byte*)rep;
}

//----------------------------

class Cstr_build{
   dword flags;                //bit 31 set, so that we distinquish it from Cstr_rep
public:
   enum{
      t_char,
      t_wchar,
      t_int,
      t_dword,
      t_float,
      t_double,
      t_string_s,
      t_string_w,
      t_end,
      t_last
   };
private:
   typedef dword t_format_type;

//----------------------------

   static dword GetDataSize(t_format_type t, const void *dt){

      dword ret;
      switch(t){
      default: assert(0);
      case t_char: return sizeof(int);       //chars in 32-bit due to 32-bit alignment
      case t_wchar: return sizeof(int);
      case t_int: return sizeof(int);
      case t_dword: return sizeof(unsigned int);
      case t_float: return sizeof(float);
      case t_double: return sizeof(double);
      case t_string_s: ret = StrLen((const char*)dt) + 1; break;
      case t_string_w: ret = (StrLen((const wchar*)dt) + 1) * sizeof(wchar); break;
      case t_end: return 0;
      }
      return (ret+3) & -4;
   }

//----------------------------
public:
   enum{ TMP_STORAGE = (sizeof(int)+sizeof(int)) * 6};

   byte stor[TMP_STORAGE];    //local space used without allocating by new (up to N int-sized params)

   mutable dword alloc_size;  //currently new-allocated buffer (pointed to by curr_stor), if 0, no alloc made
   dword buf_pos;
   byte *curr_stor;

//----------------------------

   void *rep;                 //saved original string's rep
   byte *format_string;       //allocated format string in Cstr's type

   Cstr_build(void *r):
      flags(0x80000000),
      alloc_size(0),
      buf_pos(0),
      curr_stor(stor),
      rep(r),
      format_string(NULL)
   {}
   ~Cstr_build(){
      if(alloc_size)
         delete[] curr_stor;
      delete[] format_string;
   }

   void AddFormatData(t_format_type type, const void *data, dword force_len);

   const byte *GetFormatData(const byte *&cs, dword &cp) const;
};

//----------------------------

void Cstr_build::AddFormatData(t_format_type type, const void *data, dword force_len){

   dword data_size;
   if(force_len)
      data_size = (force_len+3)&-4;
   else
      data_size = GetDataSize(type, data);
   dword sz = sizeof(t_format_type) + data_size;
   if(force_len)
      sz += sizeof(dword);
   dword buf_sz = alloc_size ? alloc_size : TMP_STORAGE;
   if(buf_pos + sz > buf_sz){
      if(buf_pos+sizeof(t_format_type) <= buf_sz)
         *(t_format_type*)(curr_stor+buf_pos) = t_last;
      if(!alloc_size){
         alloc_size = TMP_STORAGE*2;
         if(alloc_size<sz)
            alloc_size = sz;
         curr_stor = new(true) byte[alloc_size];
         buf_pos = 0;
      }else{
         alloc_size = (alloc_size*2+3) & -4;
         if(alloc_size<buf_pos+sz)
            alloc_size = buf_pos+sz;
         byte *ns = new(true) byte[alloc_size];
         MemCpy(ns, curr_stor, buf_pos);
         delete[] curr_stor;
         curr_stor = ns;
      }
   }
   byte *dt = curr_stor + buf_pos;
   *(t_format_type*)dt = type;
   dt += sizeof(t_format_type);
   if(force_len){
      *(dword*)dt = force_len;
      dt += sizeof(dword);
   }
   MemCpy(dt, data, data_size);
   buf_pos += sz;
}

//----------------------------

const byte *Cstr_build::GetFormatData(const byte *&cs, dword &cp) const{

   dword buf_sz = (cs==stor) ? TMP_STORAGE : buf_pos;
   if(cp+sizeof(t_format_type)>buf_sz || *(t_format_type*)(cs+cp)==t_last){
                              //detect end of data
      if(cs!=stor){
         assert(0);
         return NULL;
      }
      cs = curr_stor;
      cp = 0;
   }
   const byte *ret = cs + cp;
   cp += sizeof(t_format_type) + GetDataSize(*(t_format_type*)ret, ret+sizeof(t_format_type));
   return ret;
}

//----------------------------

void Cstr_base::BeginBuild(){
                              //create Cstr_build if it is not yet
   if(!rep || !((*(dword*)rep)&0x80000000))
      rep = (Cstr_rep_base*)new(true) Cstr_build(rep);
}

//----------------------------

void Cstr_base::CancelBuild(){
                              //check if rep is pointer to Cstr_build
   if(rep && ((*(dword*)rep)&0x80000000)){
      Cstr_build *bld = (Cstr_build*)rep;
      rep = (Cstr_rep_base*)bld->rep;
      delete bld;
   }
}

//----------------------------

void Cstr_base::AddFormatData(dword type, const void *data, dword force_len){

   BeginBuild();
   Cstr_build *bld = (Cstr_build*)rep;
   bld->AddFormatData(type, data, force_len);
}

//----------------------------
                              //instantiate 2 string types
#define _T char
#include "Cstr.cpp"

#undef _T
#define _T wchar
#define __CSTR_TYPE_WIDE
#include "Cstr.cpp"
#include "C_vector.h"

//----------------------------
template<>
bool Cstr_c::ToUtf8(const wchar *s){

   bool has_high = false;
   C_vector<char> buf;
   buf.reserve(StrLen(s)*2);
   while(*s){
      char b4[4];
      text_utils::ConvertCharToUTF8(*s++, (byte*)b4);
      if(!b4[1])
         buf.push_back(*b4);
      else{
         buf.insert(buf.end(), b4, b4+StrLen(b4));
         has_high = true;
      }
   }
   Allocate(buf.begin(), buf.size());
   return has_high;
}

//----------------------------
template<>
bool Cstr_w::FromUtf8(const char *cp){

   C_vector<wchar> buf;
   buf.reserve(StrLen(cp));
   bool ret = true;
   while(true){
      dword c = byte(*cp++);
      if(!c)
         break;
      if((c&0xc0)==0xc0){
                           //decode utf-8 character
         dword code = 0;
         int i;
         for(i=5; i--; ){
            byte nc = byte(*cp++);
            if(!nc)
               break;
            if((nc&0xc0) != 0x80){
                              //coding error
               ret = false;
               break;
            }
            code <<= 6;
            code |= nc&0x3f;
            if(!(c&(2<<i)))
               break;
         }
         ++i;
         code |= (c & ((1<<i)-1)) << ((6-i)*6);
         c = code;
      }
      buf.push_back(wchar(c));
   }
   Allocate(buf.begin(), buf.size());
   return ret;
}

//----------------------------

template<>
Cstr_c Cstr_w::ToUtf8() const{
   Cstr_c ret;
   ret.ToUtf8(*this);
   return ret;
}

//----------------------------

template<>
Cstr_w Cstr_c::FromUtf8() const{
   Cstr_w ret;
   ret.FromUtf8(*this);
   return ret;
}

//----------------------------

#else
//----------------------------

#define REP ((Cstr_rep<_T>*)(rep))
#define _REP(x) ((Cstr_rep<_T>*)(x.rep))
#define CSTR Cstr<_T>
#define CSTR_REP Cstr_rep<_T>

//----------------------------

template<>
void CSTR::Clear(){
   CancelBuild();
   ReleaseRep(REP);
   rep = NULL;
}

//----------------------------

template<>
void CSTR::Allocate(const _T *cp, dword length){

   length = Min(length, dword(MAX_STRING_LEN));

   CancelBuild();
   if(!length){
      Clear();
      return;
   }
   CSTR_REP *new_rep;
   if(!cp)
      new_rep = new(AllocRep<_T>(length)) CSTR_REP(length);
   else
      new_rep = new(AllocRep<_T>(length)) CSTR_REP(cp, length);
   ReleaseRep(REP);
   rep = new_rep;
}

//----------------------------

template<>
CSTR::~Cstr(){
   Clear();
}

//----------------------------
                              //forward declaration of specialization
#ifdef __CSTR_TYPE_WIDE
template<> void CSTR::Copy(const char *cp);
#else
template<> void CSTR::Copy(const wchar *wp, char invalid);
#endif
template<> dword CSTR::Length() const;
template<> const _T *CSTR::GetPtr() const;

//----------------------------

template<>
void CSTR::Build() const{

   if(!rep || !((*(dword*)rep)&0x80000000))
      return;
   Cstr_build *bld = (Cstr_build*)rep;
   bld->AddFormatData(Cstr_build::t_end, NULL, 0);
   const _T *fmt = (_T*)bld->format_string;
   dword buf_size = ((fmt ? StrLen(fmt) : 0) + 100) & -4;
   _T *buf = new(true) _T[buf_size];
   dword buf_pos = 0;
                              //iterators for getting fed params
   const byte *cs = bld->stor;
   dword cp = 0;
   CSTR s_tmp;
   while(true){
      const _T *c = fmt;
      _T orig_c = 0;
      if(fmt){
         ++fmt;
         orig_c = *c;
      }
      dword add_len = 1;
      _T tmp[512];
      if(!fmt || *c=='%' || *c=='#'){
         if(fmt && *fmt==*c){
                              //detect intended # and % chars
            ++fmt;
         }else{
            const byte *dt = bld->GetFormatData(cs, cp);
            if(!dt)
               break;
            dword type = *((dword*&)dt)++;
            if(type==Cstr_build::t_end)
               break;
                              //format parameter
            char fs[64], *fp = fs;
            *fp++ = '%';
            if(fmt && *c=='#'){
                              //# is used for additional format params
               while(*fmt!='%'){
                  if(!*fmt)
                     Fatal("C_xstr: # without matching % in format string");
                  *fp++ = char(*fmt++);
                  if(fp == fs+sizeof(fs)-4)
                     Fatal("C_xstr: format params too long");
               }
               ++fmt;
            }
            if(type==Cstr_build::t_string_s){
#ifndef __CSTR_TYPE_WIDE
               c = (const _T*)dt;
               add_len = StrLen(c);
#else
               s_tmp.Copy((const char*)dt);
               c = s_tmp;
               add_len = s_tmp.Length();
#endif
            }else
            if(type==Cstr_build::t_string_w){
#ifdef __CSTR_TYPE_WIDE
               c = (const _T*)dt;
               add_len = StrLen(c);
#else
               s_tmp.Copy((const wchar*)dt);
               c = s_tmp;
               add_len = s_tmp.Length();
#endif
            }else{
               c = tmp;
               switch(type){
               case Cstr_build::t_char:
                  *tmp = *(char*)dt;
                  add_len = 1;
                  break;
               case Cstr_build::t_wchar:
                  *tmp = _T(*(wchar*)dt);
                  add_len = 1;
                  break;

               case Cstr_build::t_int:
               case Cstr_build::t_dword:
                  {
                     dword val = *(dword*)dt;
                     char lead = ' ';
                     int width = 0;
                     bool hex = false;
                     *fp = 0;
                     fp = fs+1;
                     while(*fp){
                        char f = *fp++;
                        if(f=='0' || f==' ')
                           lead = f;
                        else
                        if(f>='0' && f<='9'){
                           width *= 10;
                           width += f - '0';
                        }else
                        if(f=='x')
                           hex = true;
                     }
                     if(hex)
                        lead = '0';
                     _T numbuf[16];
                     _T *dst = numbuf+16;
                     bool neg = (type==Cstr_build::t_int && int(val)<0 && !hex);
                     _T *tdst = tmp;
                     if(neg){
                        val = dword(-int(val));
                        --width;
                        ++tdst;
                     }
                     if(hex){
                        *tdst++ = '0';
                        *tdst++ = 'x';
                     }
                     while(val){
                        --dst;
                        if(hex){
                           *dst = char(text_utils::MakeHexString(val&0xff)>>8);
                           val >>= 4;
                        }else{
                           dword n = val % 10;
                           *dst = _T('0' + n);
                           val /= 10;
                        }
                     }
                           //make sure something was output
                     if(dst == numbuf+16)
                        *(--dst) = '0';
                     add_len = numbuf+16-dst;

                     int fill = width - add_len;
                     if(fill>0){
                        for(int i=fill; i--; )
                           *tdst++ = lead;
                     }
                     MemCpy(tdst, dst, add_len*sizeof(_T));
                     if(fill>0)
                        add_len += fill;
                           //add minus sign
                     if(neg){
                        *tmp = '-';
                        ++add_len;
                     }
                     if(hex)
                        add_len += 2;
                  }
                  break;
               }
               assert(add_len < sizeof(tmp)-1);
            }
         }
      }
                              //realloc buf if necessary
      if(buf_pos+add_len > buf_size){
         buf_size = ((buf_pos + add_len) * 2 + 3) & -4;
         _T *nb = new(true) _T[buf_size];
         MemCpy(nb, buf, buf_pos*sizeof(_T));
         delete[] buf;
         buf = nb;
      }
                        //store next char, or formatted parameter
      if(add_len==1)
         buf[buf_pos] = *c;
      else
         MemCpy(buf+buf_pos, c, add_len*sizeof(_T));
                              //if fmt string set, detect its end of string
      if(fmt && !orig_c){
                              //format string expired, remove it and continue with remaining format data, as if format was not set
         fmt = NULL;
      }else
         buf_pos += add_len;
   }
                              //close builder
   rep = (Cstr_rep_base*)bld->rep;
   delete bld;
                              //append string
   if(!rep)
      const_cast<CSTR*>(this)->Allocate(buf, buf_pos);
   else{
      size_t len = REP->Length();
      CSTR_REP *new_rep = new(AllocRep<_T>(len + buf_pos)) CSTR_REP(REP->GetData(), buf, len, buf_pos);
      ReleaseRep(REP);
      rep = new_rep;
   }
   delete[] buf;
}

//----------------------------

template<>
dword CSTR::Length() const{

   Build();
   if(!rep)
      return 0;
   return REP->Length();
}

//----------------------------

template<>
const _T *CSTR::GetPtr() const{
   Build();
   if(!rep)
      return (const _T*)&rep;
   return REP->GetData();
}

//----------------------------

template<>
CSTR &CSTR::operator =(const _T *cp){

   CancelBuild();
   CSTR_REP *new_rep;
   if(!cp)
      new_rep = NULL;
   else{
      size_t len = StrLen(cp);
      new_rep = new(AllocRep<_T>(len)) CSTR_REP(cp, len);
   }
   ReleaseRep(REP);
   rep = new_rep;
   return *this;
}

//----------------------------

template<>
CSTR::Cstr(const CSTR &s){
   s.Build();
   rep = s.rep;
   if(rep)
      REP->AddRef();
}

//----------------------------

template<>
CSTR::Cstr(const _T *cp){
   rep = NULL;
   operator =(cp);
}

//----------------------------

template<>
void CSTR::MakeUniqueCopy(){

   if(rep && REP->Count()!=1){
                           //use placement new for proper allocation and initialization of string data.
      CSTR_REP *new_rep = new(AllocRep<_T>(REP->Length())) CSTR_REP(REP->GetData(), REP->Length());
      ReleaseRep(REP);
      rep = new_rep;
   }
}

//----------------------------

template<>
CSTR &CSTR::operator =(const CSTR &s){

   if(&s==this)
      Build();
   else
      CancelBuild();
   s.Build();
   if(s.rep){
      dword ref = _REP(s)->Count();
      if(ref==(1<<Cstr_rep<_T>::REF_BITS)-1){
                              //ref counter on s would overflow, make unique copy on s
         LOG_RUN("Cstr ref overflow, make copy");
         const_cast<CSTR&>(s).MakeUniqueCopy();
      }
      _REP(s)->AddRef();
   }
   ReleaseRep(REP);
   rep = s.rep;
   return *this;
}

//----------------------------

template<>
const _T &CSTR::operator [](dword pos) const{
   Build();
   //assert(pos<(Length()+1));
   if(pos>Length())
      Fatal("Cstr[] bad pos", pos);
   if(!rep)
      return *(_T*)&rep;
   return REP->GetData()[pos];
}

//----------------------------

template<>
_T &CSTR::At(dword pos){
   Build();
   if(pos>Length())
      Fatal("Cstr::At bad pos", pos);
   assert(pos<Length());
   if(!rep)
      return *(_T*)&rep;
   MakeUniqueCopy();
   return REP->GetData()[pos];
}

//----------------------------

template<>
CSTR CSTR::Left(dword len) const{
   dword _len = Length();
   if(len>_len)
      len = _len;
   CSTR ret;
   ret.Allocate(GetPtr(), len);
   return ret;
}

//----------------------------

template<>
CSTR CSTR::Right(dword len) const{
   dword _len = Length();
   if(len>_len)
      len = _len;
   CSTR ret;
   ret.Allocate(GetPtr()+_len-len, len);
   return ret;
}

//----------------------------

template<>
CSTR CSTR::Mid(dword pos) const{
   dword _len = Length();
   if(pos>_len)
      pos = _len;
   CSTR ret;
   ret.Allocate(GetPtr()+pos, _len-pos);
   return ret;
}

//----------------------------

template<>
CSTR CSTR::Mid(dword pos, dword len) const{
   dword _len = Length();
   if(pos+len>_len)
      return NULL;
   CSTR ret;
   ret.Allocate(GetPtr()+pos, len);
   return ret;
}

//----------------------------

template<>
bool CSTR::operator ==(const CSTR &s) const{

   Build();
   s.Build();
   if(rep==s.rep)
      return true;
   if(Length()!=s.Length())
      return false;
   return operator ==(s.GetPtr());
}

//----------------------------

template<>
void CSTR::ToLower(){

   Build();
   MakeUniqueCopy();
   _T *buf = REP->GetData();
   for(dword l = Length(); l--; ){
      _T &c = buf[l];
      /*
#ifdef __CSTR_TYPE_WIDE
      if(c<256)
#endif
         c = ::ToLower(c);
         */
      c = _T(text_utils::LowerCase(c));
   }
}

//----------------------------

template<>
void CSTR::ToUpper(){

   Build();
   MakeUniqueCopy();
   _T *buf = REP->GetData();
   for(dword l = Length(); l--; ){
      _T &c = buf[l];
      /*
#ifdef __CSTR_TYPE_WIDE
      if(c<256)
#endif
         c = ::ToUpper(c);
         */
      c = _T(text_utils::UpperCase(c));
   }
}

//----------------------------

template<>
int CSTR::FindFromPos(_T c, dword pos) const{

   const _T *tp = GetPtr();
   dword _len = Length();
   for(dword i=pos; i<_len; i++){
      if(tp[i]==c)
         return i;
   }
   return -1;
}

//----------------------------

template<>
int CSTR::FindReverse(_T c) const{

   const _T *tp = GetPtr();
   for(int i=Length(); i--; ){
      if(tp[i]==c)
         return i;
   }
   return -1;
}

//----------------------------
#ifdef __CSTR_TYPE_WIDE

template<>
void CSTR::Copy(const char *cp){

   if(!cp){
      Clear();
   }else{
      dword len = StrLen(cp);
      Allocate(NULL, len);
      _T *buf = REP->GetData();
      for(dword i=0; i<len; i++)
         buf[i] = byte(cp[i]);
   }
}

//----------------------------

template<>
void CSTR::Copy(const wchar *wp, char invalid){
   operator =(wp);
}

//----------------------------

#else//wide

template<>
void CSTR::Copy(const wchar *wp, char invalid){

   if(!wp){
      Clear();
   }else{
      dword len = StrLen(wp);
      Allocate(NULL, len);
      _T *buf = REP->GetData();
      for(dword i=0; i<len; i++){
         dword c = wp[i];
         buf[i] = _T(c<256 ? c : invalid);
      }
   }
}

//----------------------------

template<>
void CSTR::Copy(const char *cp){
   operator =(cp);
}

//----------------------------

#endif//!wide

//----------------------------

template<>
CSTR &CSTR::AppendFormat(const _T *fmt){

   Build();
   BeginBuild();
   Cstr_build *bld = (Cstr_build*)rep;
   if(bld->format_string)
      Fatal("Cstr: format already set!");
   if(fmt){
      dword len = StrLen(fmt);
      bld->format_string = new(true) byte[(len+1)*sizeof(_T)];
      MemCpy(bld->format_string, fmt, (len+1)*sizeof(_T));
   }
   return *this;
}

//----------------------------

template<>
CSTR &CSTR::Format(const _T *fmt){
   Clear();
   return AppendFormat(fmt);
}

//----------------------------

template<>
CSTR &CSTR::operator <<(char d){ AddFormatData(Cstr_build::t_char, &d); return *this; }

//----------------------------

template<>
CSTR &CSTR::operator <<(wchar d){ AddFormatData(Cstr_build::t_wchar, &d); return *this; }

//----------------------------

template<>
CSTR &CSTR::operator <<(int d){ AddFormatData(Cstr_build::t_int, &d); return *this; }

//----------------------------

template<>
CSTR &CSTR::operator <<(dword d){ AddFormatData(Cstr_build::t_dword, &d); return *this; }

//----------------------------

template<>
CSTR &CSTR::operator <<(unsigned int d){ operator <<(dword(d)); return *this; }

//----------------------------

template<>
CSTR &CSTR::operator <<(const char *d){
   AddFormatData(Cstr_build::t_string_s, d);
   return *this;
}

//----------------------------

template<>
CSTR &CSTR::operator <<(const wchar *d){
   AddFormatData(Cstr_build::t_string_w, d);
   return *this;
}

//----------------------------

static bool ScanNumber(const _T *cp, int &i, bool allow_neg){

   bool ok = false;
   i = 0;
   bool neg = false;
   while(*cp){
      _T c = *cp++;
      if(c>='0' && c<='9'){
         dword d = c-'0';
         i = i*10 + d;
                              //check overflow
         if(allow_neg){
            if(i&0x80000000)
               return false;
         }else{
            if(dword(i)<d)
               return false;
         }
         ok = true;
      }else
      if(c=='-' && allow_neg && !neg){
         neg = true;
      }else
         return false;
   }
   if(!ok)
      i = 0;
   if(allow_neg && neg)
      i = -i;
   return ok;
}

//----------------------------

template<>
bool CSTR::operator >>(int &i) const{
   return ScanNumber(GetPtr(), i, true);
}

//----------------------------

template<>
bool CSTR::operator >>(dword &i) const{
   return ScanNumber(GetPtr(), (int&)i, false);
}

//----------------------------
#endif//__CSTR_TYPE

//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
// C_buffer - template class for implementing fixed-sized buffer with automatic
// allocation, de-allocation, member-access.
//----------------------------

#ifndef __C_BUFFER_H
#define __C_BUFFER_H

#include <rules.h>
#include <Util.h>

//----------------------------

template<class T>
class C_buffer{
   T *buf;
   dword sz;

//----------------------------
// Realloc storage to store specified number of elements.
   virtual void Realloc(dword n){
      if(n!=sz){
         T *nb = n ? new(true) T[n] : NULL;
         for(dword i=Min(n, sz); i--; )
            nb[i] = buf[i];
         delete[] buf;
         buf = nb;
         sz = n;
      }
   }
public:
   C_buffer(): buf(NULL), sz(0){}
   inline C_buffer(const C_buffer &b): buf(NULL), sz(0){ Assign(b.Begin(), b.End()); }
   virtual ~C_buffer(){
      Realloc(0);
   }

//----------------------------
// Assign elements - defined as pointers to first and last members in array.
   void Assign(const T *first, const T *last){
      Realloc(last-first);
      for(dword i=sz; i--; )
         buf[i] = first[i];
   }

//----------------------------
// Assign elements - reserve space, fill with specified item.
   void Assign(dword n, const T &x){
      Realloc(n);
      for(dword i=sz; i--; )
         buf[i] = x;
   }

//----------------------------
// Reserve space for specified number of items.
   void Assign(dword n){
      Realloc(n);
   }

//----------------------------
   void operator =(const C_buffer &b){
      Assign(b.Begin(), b.End());
   }

//----------------------------
// Get pointers.
   inline const T *Begin() const{ return buf; }
   inline T *Begin(){ return buf; }
   inline const T *End() const{ return buf + sz; }
   inline T *End(){ return buf + sz; }

//----------------------------
// Get references.
   inline T &Front(){ //assert(sz);
      return *buf;
   }
   inline const T &Front() const{ assert(sz); return *buf; }
   inline T &Back(){ assert(sz); return buf[sz-1]; }
   inline const T &Back() const{ assert(sz); return buf[sz-1]; }

//----------------------------
// Clear contents.
   void Clear(){ Realloc(0); }

//----------------------------
// Access elements.
   inline const T &operator[](dword i) const{
      assert(i<sz);
      if(i>=sz) LOG_RUN_N("C_buffer[] invalid index", i);
      return buf[i];
   }
   inline T &operator[](dword i){
      assert(i<sz);
      if(i>=sz) LOG_RUN_N("C_buffer[] invalid index", i);
      return buf[i];
   }

//----------------------------
// Resize buffer, fill new items with provided value.
   void Resize(dword n, const T &x){
      dword i = sz;
      Realloc(n);
      for(; i<sz; i++)
         buf[i] = x;
   }

//----------------------------
// Resize buffer, and return pointer to beginning.
   inline T *Resize(dword n){
      Realloc(n);
      return Begin();
   }

//----------------------------
// Get size of the buffer.
   inline dword Size() const{ return sz; }
};

//----------------------------
#endif

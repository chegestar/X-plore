//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#ifndef __SMARTPTR_H_
#define __SMARTPTR_H_
/*
   Copyright (c) Lonely Cat Games  All rights reserved.

   Smart pointer class for use by classes using reference counting.
   Written by Michal Bacik 06.01.1999
   Requirements: AddRef and Release methods defined in class.
*/

class C_unknown{
   unsigned long ref;
protected:
   virtual ~C_unknown(){};
public:
   inline C_unknown(): ref(1) {}
   virtual unsigned long AddRef(){ return ++ref; }
   virtual unsigned long Release(){
      if(--ref) return ref;
      delete this;
      return 0;
   }
};

//----------------------------
//----------------------------

template<class T>
class C_smart_ptr{
protected:
   T *ptr;
public:
//----------------------------
// Default constructor - initialize to NULL.
   inline C_smart_ptr(): ptr(0){}

//----------------------------
// Constructor from pointer to T. If non-NULL, reference is increased.
   inline C_smart_ptr(T *tp): ptr(tp){ if(ptr) ptr->AddRef(); }

//----------------------------
// Constructor from reference to smartpointer. If ptr non-NULL, reference is increased.
   inline C_smart_ptr(const C_smart_ptr<T> &sp): ptr(sp.ptr){ if(ptr) ptr->AddRef(); }

//----------------------------
// Destructor - releasing reference if non-NULL pointer.
   inline ~C_smart_ptr(){ if(ptr) ptr->Release(); }

//----------------------------
// Assignment from pointer to T. References to non-NULL pointers adjusted.
   inline C_smart_ptr &operator =(T *tp){
      if(ptr!=tp){
         if(tp) tp->AddRef();
         if(ptr) ptr->Release();
         ptr = tp;
      }
      return *this;
   }

//----------------------------
// Assignment from other smartpointer.
// Note: passed by value, not reference, because sometimes the reference is part of object being destroyed by assignment.
   inline C_smart_ptr &operator =(C_smart_ptr sp){
      if(ptr!=sp.ptr){
         if(sp.ptr) sp.ptr->AddRef();
         if(ptr) ptr->Release();
         ptr = sp.ptr;
      }
      return *this;
   }

//----------------------------
// Three pointer-access operators. Const and non-const versions.
   inline operator T *(){ return ptr; }
   inline operator const T *() const{ return ptr; }
   inline T &operator *(){ return *ptr; }
   inline const T &operator *() const{ return *ptr; }
   inline T *operator ->(){ return ptr; }
   inline const T *operator ->() const{ return ptr; }
   inline T **operator &(){
#ifdef _DEBUG
      if(ptr){
         //assert(0);//__asm int 3;
      }
#endif
      return &ptr;
   }

//----------------------------
// Boolean comparison of pointer.
   inline bool operator !() const{ return (!ptr); }
   inline bool operator ==(const T *tp) const{ return (ptr==tp); }
   inline bool operator ==(const C_smart_ptr<T> &s) const{ return (ptr == s.ptr); }
   inline bool operator !=(const T *tp) const{ return (ptr!=tp); }
   inline bool operator !=(const C_smart_ptr<T> &s) const{ return (ptr != s.ptr); }

//----------------------------
// Comparison of pointers.
   inline bool operator <(const C_smart_ptr<T> &s) const{ return (ptr < s.ptr); }

//----------------------------
// Get pointer to this class. Since operator& is overloaded, we must use function.
   inline C_smart_ptr<T> *Ptr(){ return this; }

//----------------------------
// Replace stored pointer in case that we have const C_smart_ptr object.
   inline void SetPtr(const T *p) const{ const_cast<C_smart_ptr&>(*this) = const_cast<T*>(p); }
};

#endif

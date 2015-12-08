#ifdef __PALMOS__
// Palm OS allocation functions
#include <Util.h>

extern"C"{

void *calloc(dword items, dword size){
   dword sz = items*size;
   if(!sz)
      return NULL;
   void *vp = new(true) byte[sz];
   MemSet(vp, 0, sz);
   return vp;
}

//----------------------------

void free(void *ptr){
   delete[] (byte*)ptr;
}

}

//----------------------------
#endif//__PALMOS__

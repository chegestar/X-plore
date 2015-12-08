//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#ifndef __LOGMAN_H
#define __LOGMAN_H

//----------------------------

#include <Rules.h>

class C_logman{
   void *imp;

   void AddInt(int);
public:
   C_logman();
   ~C_logman(){
      Close();
   }

   bool Open(const wchar *fname, bool replace = true);
   void Close();

   void Add(const char *txt, bool eol = true);
   void AddBinary(const void *buf, dword len);

   C_logman &operator()(const char *txt, bool eol = true);
   C_logman &operator()(const char *txt, int num, bool eol = true);
   C_logman &operator()(const char *txt, int num, int num1, bool eol = true);
};

#endif

//----------------------------

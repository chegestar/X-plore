//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <E32Base.h>
#include <Util.h>
#include <EikEnv.h>
#include <Cstr.h>
#ifdef __SYMBIAN_3RD__
#include <libc\setjmp.h>
#endif

//----------------------------

int C_leave_func::Execute(){

#ifdef __SYMBIAN_3RD__
                              //we can't access some members using pointer, so we have statics

                              //long-jump buffer
   static jmp_buf jmp_env;

                              //current trap handler, get it so we can patch its virtual table
   TTrapHandler *th = User::TrapHandler();

                              //save its virtual table so that we can call original methods
   static void *save_vtab = *(void**)th;
                              //nest count, so that we know when Leave in nested (dispatched to original func), or our
   static int nest_count;
   nest_count = 0;

   /*
   static C_logman *l = new C_logman;
   l->Open(L"C:\\trap.txt");
   static C_logman &log = *l;
   */

   class C_trap_handler_my: public TTrapHandler{
                              //restore original vtab, so that original func is called; return patched vtab
      void *RestoreVtab(){
         void *tmp = *(void**)this;
         *(void**)this = save_vtab;
         return tmp;
      }
                              //reset vtab back to patched version
      void ResetVtab(void *tmp){
         *(void**)this = tmp;
      }

   //----------------------------
   // Trap / UnTrap just count nesting and call original func
      virtual void Trap(){
         ++nest_count;
         //log("Trap ", nest_count);
         void *tmp = RestoreVtab();
         Trap();
         ResetVtab(tmp);
      }
      virtual void UnTrap(){
         //log("UnTrap", nest_count);
         --nest_count;
         void *tmp = RestoreVtab();
         UnTrap();
         ResetVtab(tmp);
      }
      virtual void Leave(TInt err){
         //log("Leave ", nest_count);
         --nest_count;
                              //call original
         void *tmp = RestoreVtab();
         Leave(err);
         ResetVtab(tmp);
         if(!nest_count){
                              //this leave is intented for our trap, don't let it proceed, and make shortcut back to us
            longjmp(jmp_env, err);
         }
      }
   } th_my;
                              //patch trap handler
   *(dword**)th = *(dword**)(&th_my);
                              //make standard cleanup mark
   TTrapHandler *ph = User::MarkCleanupStack();
                              //setup long jump
   int v = setjmp(jmp_env);
   if(!v){
                              //call code that may leave
      v = Run();
      //log("Run done");
                              //standard cleanup unmark
      User::UnMarkCleanupStack(ph);
   }else{
                              //leave is caught here
      //log("Leave ", v);
   }
   assert(nest_count==0);
                              //restore trap handler's vtab
   *(void**)th = save_vtab;
   //log("Func done ", nest_count);
                              //return with error code
#else//__SYMBIAN_3RD__
                              //use standard TRAP macro
   TRAPD(v, Run());
#endif//!__SYMBIAN_3RD__
   return v;
}

//----------------------------

static void MakeHexCode(dword code, char buf[11]){

   *buf++ = '0';
   *buf++ = 'x';
   bool zero = true;
   for(int i=8; i--; ){
      dword c = (code>>(i*4))&0xf;
      if(!c && zero)
         continue;
      zero = false;
      c += c<10 ? '0' : 'a'-10;
      *buf++ = char(c);
   }
   *buf = 0;
}

//----------------------------

void Info(const wchar *msg, dword code, bool exit){

   Cstr_w s = msg;
   if(code){
      char buf[11];
      MakeHexCode(code, buf);
      s.AppendFormat(L" (%) %") <<buf <<int(code);
   }
   //User::InfoPrint(TPtrC((const word*)(const wchar*)s, s.Length()));
   if(exit){
      CEikonEnv::Static()->AlertWin(TPtrC((const word*)(const wchar*)s, s.Length()));
      User::Exit(0);
   }else
      CEikonEnv::Static()->InfoWinL(TPtrC((const word*)(const wchar*)s, s.Length()), _L(""));
   /*
   int len = StrLen(msg);
   TBuf16<20> desc; desc.Copy(TPtr8((byte*)msg, Min(len, 20), len));
   //User::InfoPrint(desc);
   //CEikonEnv::Static()->InfoWinL(desc, _L("abc"));
   CEikonEnv::Static()->AlertWin(desc);
   */
}

//----------------------------

void Info(const char *msg, dword code, bool exit){
   Cstr_w s; s.Copy(msg);
   Info(s, code, exit);
}

//----------------------------

void Fatal(const wchar *msg, dword code){

   Cstr_w s = msg;
   if(code){
      char buf[11];
      MakeHexCode(code, buf);
      s.AppendFormat(L" (%)") <<buf;
   }
#if defined S60 && defined __SYMBIAN_3RD__
   if(CActiveScheduler::Current())
      Info(msg, code, true);
   else
#endif
      User::Panic(TPtrC((word*)(const wchar*)s, s.Length()), code);
}

//----------------------------

void Fatal(const char *msg, dword code){
   Cstr_w s; s.Copy(msg);
   Fatal(s, code);
}

//----------------------------

//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <E32Base.h>
#include <Thread.h>
#include <AppBase.h>

//----------------------------

class C_thread_access: public C_thread{
public:
   inline void RunThread(){ C_thread::RunThread(); }
};

//----------------------------

class RThread1: public RThread{
public:
   C_thread_access *owner;
#ifndef __SYMBIAN_3RD__
   const S_global_data *gd;
#endif
};

static TInt ThreadProcThunk(TAny *aPtr){
   CTrapCleanup *tc = CTrapCleanup::New();
   //CActiveScheduler as;
   //CActiveScheduler::Install(&as);
   RThread1 *tp = (RThread1*)aPtr;
#ifndef __SYMBIAN_3RD__
   Dll::SetTls((void*)tp->gd);
#endif
   tp->owner->RunThread();
   //CActiveScheduler::Install(NULL);
   delete tc;
   tp->owner->KillThread();
   return 0;
}

//----------------------------

void C_thread::CreateThread(E_PRIORITY priority){

   quit_request = false;
   KillThread();
   status = STATUS_STARTING;
   RThread1 *tp = new(true) RThread1;
   tp->owner = (C_thread_access*)this;
#ifndef __SYMBIAN_3RD__
   tp->gd = GetGlobalData();
#endif
   imp = tp;
                              //make unique thread name
   Cstr_w tname; tname<<L"Thread-" <<GetTickTime();
   tp->Create(TPtrC((const word*)(const wchar*)tname, tname.Length()), ThreadProcThunk, 8192, NULL, tp);
   TThreadPriority pr;
   switch(priority){
   case PRIORITY_VERY_LOW: pr = EPriorityMuchLess; break;
   case PRIORITY_LOW: pr = EPriorityLess; break;
   default: assert(0);
   case PRIORITY_NORMAL: pr = EPriorityNormal; break;
   case PRIORITY_HIGH: pr = EPriorityMore; break;
   case PRIORITY_VERY_HIGH:
      //pr = EPriorityRealTime;
      pr = EPriorityAbsoluteHigh;
      break;
   }
   tp->SetPriority(pr);

   tp->Resume();
}

//----------------------------

void C_thread::KillThread(){
   if(imp){
      RThread *thrd = (RThread*)imp;
      if(!thrd->ExitReason()){
         thrd->Terminate(1);
         thrd->Close();
      }
      delete thrd;
      imp = NULL;
   }
   status = STATUS_UNINITIALIZED;
}

//----------------------------

void C_thread::SuspendThreadSelf(){
   if(imp){
      RThread *thrd = (RThread*)imp;
      if(!thrd->ExitReason())
         thrd->Suspend();
   }
}

//----------------------------

void C_thread::ResumeThread(){
   suspend_request = false;
   if(imp){
      RThread *thrd = (RThread*)imp;
      if(!thrd->ExitReason())
         thrd->Resume();
   }
}

//----------------------------

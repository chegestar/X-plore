//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <Thread.h>
#include <Util.h>
#include <Cstr.h>

//----------------------------

class C_thread_access: public C_thread{
public:
   inline void RunThread(){ C_thread::RunThread(); }
   /*
   void *GetImp(){
      OffsetOf(C_thread, imp);
      return 0;
   }
   */
};

//----------------------------

C_thread::C_thread()
   :imp(NULL)
   ,status(STATUS_UNINITIALIZED)
   ,suspend_request(false)
   ,quit_request(false)
{
}

//----------------------------

bool C_thread::AskToQuitThread(dword time_out){

   if(status==STATUS_STARTING || status==STATUS_RUNNING){
      //LOG_RUN("AskToQuitThread");
      quit_request = true;
      ResumeThread();

      const dword SLEEP_TIME = 10;
      while(status!=STATUS_FINISHED){
         system::Sleep(SLEEP_TIME);
         if(time_out){
            if(time_out<=SLEEP_TIME){
               LOG_RUN("AskToQuitThread time-out");
                              //elapsed
               quit_request = false;
               return false;
            }
            time_out -= SLEEP_TIME;
         }
      }
   }
   if(status!=STATUS_UNINITIALIZED){
      system::Sleep(10);
      KillThread();
   }
   return true;
}

//----------------------------

void C_thread::ThreadProc(){
   while(!IsThreadQuitRequest()){
      if(suspend_request)
         SuspendThreadSelf();
      if(!ThreadLoop())
         break;
   }
}

//----------------------------

bool C_thread::ThreadLoop(){
   return false;
}

//----------------------------
#if defined _WINDOWS || defined _WIN32_WCE
namespace win{
#include <Windows.h>
}

static dword WINAPI ThreadProcThunk(void *param){
   ((C_thread_access*)param)->RunThread();
   return 0;
}

//----------------------------

void C_thread::CreateThread(E_PRIORITY priority){

   quit_request = false;
   KillThread();
   dword tid;
   status = STATUS_STARTING;
   imp = win::CreateThread(NULL, 0, ThreadProcThunk, this, 0, &tid);
   {
      int pr;
      switch(priority){
      case PRIORITY_VERY_LOW: pr = THREAD_PRIORITY_LOWEST; break;
      case PRIORITY_LOW: pr = THREAD_PRIORITY_BELOW_NORMAL; break;
      default: assert(0);
      case PRIORITY_NORMAL: pr = THREAD_PRIORITY_NORMAL; break;
      case PRIORITY_HIGH: pr = THREAD_PRIORITY_ABOVE_NORMAL; break;
      case PRIORITY_VERY_HIGH: pr = THREAD_PRIORITY_HIGHEST; break;
      }
      win::SetThreadPriority((win::HANDLE)imp, pr);
   }
}

//----------------------------

void C_thread::KillThread(){
   if(imp){
      win::TerminateThread((win::HANDLE)imp, 0);
      win::CloseHandle((win::HANDLE)imp);
      imp = NULL;
   }
   status = STATUS_UNINITIALIZED;
}

//----------------------------

void C_thread::SuspendThreadSelf(){
   if(imp)
      win::SuspendThread((win::HANDLE)imp);
}

//----------------------------

void C_thread::ResumeThread(){
   suspend_request = false;
   if(imp)
      win::ResumeThread((win::HANDLE)imp);
}

#endif
//----------------------------
#ifdef BADA
#include <FBaseRtThreadThread.h>

//----------------------------

class C_bada_thread: public Osp::Base::Runtime::Thread{
   virtual Osp::Base::Object *Run(){
      ((C_thread_access*)thread)->RunThread();
      return NULL;
   }
   C_thread *thread;
public:
   C_bada_thread(C_thread *tp):
      thread(tp)
   {
   }
};

void C_thread::CreateThread(E_PRIORITY priority){

   quit_request = false;
   KillThread();
   status = STATUS_STARTING;

   C_bada_thread *tp = new(true) C_bada_thread(this);
   imp = tp;
   Osp::Base::Runtime::ThreadPriority pr;
   switch(priority){
   case PRIORITY_VERY_LOW: pr = Osp::Base::Runtime::THREAD_PRIORITY_LOW; break;
   case PRIORITY_LOW: pr = Osp::Base::Runtime::THREAD_PRIORITY_LOW; break;
   default: assert(0);
   case PRIORITY_NORMAL: pr = Osp::Base::Runtime::THREAD_PRIORITY_MID; break;
   case PRIORITY_HIGH: pr = Osp::Base::Runtime::THREAD_PRIORITY_HIGH; break;
   case PRIORITY_VERY_HIGH: pr = Osp::Base::Runtime::THREAD_PRIORITY_HIGH; break;
   }
   //tp->
}

//----------------------------

void C_thread::KillThread(){
   if(imp){
      C_bada_thread *tp = (C_bada_thread*)imp;
      tp->Stop();
      delete tp;
      imp = NULL;
   }
   status = STATUS_UNINITIALIZED;
}

//----------------------------

void C_thread::SuspendThreadSelf(){
   while(suspend_request)
      system::Sleep(20);
}

//----------------------------

void C_thread::ResumeThread(){
   suspend_request = false;
   if(imp){
      //C_bada_thread *tp = (C_bada_thread*)imp;
   }
}

#endif   //BADA

//----------------------------
#ifdef ANDROID
#pragma GCC diagnostic ignored "-Wshadow"
#include <pthread.h>
#include <Android\GlobalData.h>

class C_android_thread{
public:
   pthread_t handle;
   C_thread::E_PRIORITY priority;
   C_thread *thrd;

   C_android_thread():
      handle(NULL)
   {}
};

//----------------------------

static void JniAttachThread(){
   Cstr_c tname; tname<<"Thread-" <<GetTickTime();
   struct JavaVMAttachArgs args;
   args.version = JNI_VERSION_1_4;
   args.name = (const char*)tname;
   args.group = NULL;
   JNIEnv *env;
   android_globals.vm->AttachCurrentThread(&env, &args);
}

#include <unistd.h>
#include <errno.h>
#include <sys\resource.h>

static void *StartPThread(void *arg){
   JniAttachThread();
                              
   C_android_thread *at = (C_android_thread*)arg;
   C_thread_access *tp = (C_thread_access*)at->thrd;
                              //set priority now
   int pr;
   switch(at->priority){
   case C_thread::PRIORITY_VERY_LOW: pr = 19; break;
   case C_thread::PRIORITY_LOW: pr = 10; break;
   default: assert(0);
   case C_thread::PRIORITY_NORMAL: pr = 0; break;
   case C_thread::PRIORITY_HIGH: pr = -10; break;
   case C_thread::PRIORITY_VERY_HIGH: pr = -20; break;
   }
   if(pr){
      //LOG_RUN_N("Thread set priority", pr);
      int err = setpriority(PRIO_PROCESS, gettid(), pr);
      if(err)
         LOG_RUN_N("Thread: setpriority err =", errno);
   }
   tp->RunThread();
   android_globals.vm->DetachCurrentThread();
   return 0;
}

//----------------------------

void C_thread::CreateThread(E_PRIORITY priority){

   quit_request = false;
   KillThread();
   status = STATUS_STARTING;

   C_android_thread *tp = new(true) C_android_thread;
   imp = tp;
   tp->priority = priority;
   tp->thrd = this;

   pthread_create(&tp->handle, NULL, &StartPThread, tp);
   //LOG_RUN_N("CreateThread", tp->handle);
   //LOG_RUN_N("Thrd self", pthread_self());
   //LOG_RUN_N("Base Thrd tid", gettid());
}

//----------------------------

void C_thread::KillThread(){
   if(imp){
      C_android_thread *tp = (C_android_thread*)imp;
      pthread_detach(tp->handle);
      delete tp;
      imp = NULL;
   }
   status = STATUS_UNINITIALIZED;
}

//----------------------------

void C_thread::SuspendThreadSelf(){
   while(suspend_request)
      system::Sleep(20);
}

//----------------------------

void C_thread::ResumeThread(){
   suspend_request = false;
}

#endif   //ANDROID

//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------
#include <Rules.h>

#ifndef __THREAD_H_
#define __THREAD_H_

//----------------------------

class C_thread{
   void *imp;
   enum{
      STATUS_UNINITIALIZED,   //thread is not initialized
      STATUS_STARTING,        //thread is starting, didn't get to ThreadProc yet
      STATUS_RUNNING,         //thread is running in ThreadProc
      STATUS_FINISHED         //thread has returned from ThreadProc
   } status;                  //set internally by thread before ThreadProc is called, and when ThreadProc returns
   volatile bool quit_request;//signalled by main thread to ask thread to terminate ASAP
   volatile bool suspend_request;//set in SuspendThread, cleared in ResumeThread, processed in own ThreadProc.

   void SuspendThreadSelf();
protected:
   void RunThread(){ //called internally
      status = STATUS_RUNNING;
      ThreadProc();
      status = STATUS_FINISHED;
   }

   C_thread();
   ~C_thread(){ KillThread(); }

//----------------------------
// Thread function, called when thread is created. When it exits, thread terminates.
// Default implementation calls ThreadLoop until quit request is set.
   virtual void ThreadProc();

//----------------------------
// Perform one loop in thread. Return true to continue, false to exit thread.
   virtual bool ThreadLoop();

public:

   enum E_PRIORITY{
      PRIORITY_VERY_LOW,
      PRIORITY_LOW,
      PRIORITY_NORMAL,
      PRIORITY_HIGH,
      PRIORITY_VERY_HIGH,
   };

//----------------------------
// Create the thread. New thread will be run in function ThreadProc.
   void CreateThread(E_PRIORITY priority = PRIORITY_NORMAL);

//----------------------------
// Suspend thread.
   inline void SuspendThread(){ suspend_request = true; }

//----------------------------
// Resume thread.
   void ResumeThread();

//----------------------------
// Terminate (kill) thread. status is set to STATUS_UNINITIALIZED.
   void KillThread();

//----------------------------
// Check if thread is currently running ThreadProc.
   inline bool IsThreadRunning() const{ return (status==STATUS_RUNNING); }

//----------------------------
// Check if thread is finished running.
   inline bool IsThreadFinished() const{ return (status==STATUS_FINISHED); }

//----------------------------
// Check if quit request is set.
   inline bool IsThreadQuitRequest() const{ return quit_request; }

//----------------------------
// Ask created thread (if it is starting or running) to quit by setting quit_request to true.
// Then wait (sleep) until thread finishes running (STATUS_FINISHED), or time_out elapses.
// If thread finished running in given time, KillThread is called internally.
// time_out = time to wait for thread to quit in ms (0 = infinite)
// Return true if thread finished (or was uninitialized), and false if time-out elapsed; in such case thread continues to run and quit_request is set to false.
   bool AskToQuitThread(dword time_out = 0);
};

//----------------------------
#endif

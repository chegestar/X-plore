//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#ifndef __UTIL_H
#define __UTIL_H

#include <Rules.h>

//----------------------------
// Find index of highest bit, or -1 if no bit set.
dword FindHighestBit(dword mask);

// Find index of lowest bit, or -1 if no bit set.
dword FindLowestBit(dword mask);

//----------------------------
// Reverse bit order.
dword ReverseBits(dword mask);

//----------------------------
// Get number of set bits in the mask.
dword CountBits(dword mask);

//----------------------------
// Rotate left/right, and fill shifted bits on other side.
// Negative value for shift is permitted (rotates to other direction).
dword RotateLeft(dword mask, int shift);
dword RotateRight(dword mask, int shift);

inline word ByteSwap16(word n){ return (((n << 8) & 0xff00) | ((n >> 8) & 0x00ff)); }
inline dword ByteSwap32(dword n){ return (((n << 24) & 0xff000000) | ((n << 8) & 0x00ff0000) | ((n >> 8) & 0x0000ff00) | ((n >> 24) & 0x000000ff)); }

//----------------------------

template<class T>
inline void Swap(T &l, T &r){ T tmp = r; r = l; l = tmp; }

//----------------------------
// same as standard memset, memcpy, memcmp, strlen
void MemSet(void *dst, byte c, dword len);
void MemCpy(void *dst, const void *src, dword len);
int MemCmp(const void *mem1, const void *mem2, dword len);
dword StrLen(const char *cp);
dword StrLen(const wchar *cp);
dword StrCpy(char *dst, const char *src);
dword StrCpy(wchar *dst, const wchar *src);
int StrCmp(const char *cp1, const char *cp2);
int StrCmp(const wchar *cp1, const wchar *cp2);

//----------------------------

inline char ToLower(char c){
   if(c>='A' && c<='Z')
      return c + 'a'-'A';
   return c;
}

inline char ToUpper(char c){
   if(c>='a' && c<='z')
      return c + 'A'-'a';
   return c;
}

inline wchar ToLower(wchar c){
   if(c>='A' && c<='Z')
      return c + 'a'-'A';
   return c;
}

inline wchar ToUpper(wchar c){
   if(c>='a' && c<='z')
      return c + 'A'-'a';
   return c;
}

//----------------------------
// Fatal error - display message, and exit program immediately.
void Fatal(const char *msg, dword code = 0);
void Fatal(const wchar *msg, dword code = 0);
void Info(const char *msg, dword code = 0, bool exit = false);
void Info(const wchar *msg, dword code = 0, bool exit = false);

//----------------------------
#ifndef assert

#ifdef NDEBUG
#define assert(exp) ((void)0)
#else

#if defined _WIN32_WCE || !defined _MSC_VER
#define assert(exp) if(!(exp)){ Fatal(#exp " - "__FILE__, __LINE__); }
#else
                              //work with iexcpt.dll library
#define assert(exp) if(!(exp)){ const char *_f = __FILE__, *_e = #exp; int _l = __LINE__; __asm push _e __asm push _l __asm push _f __asm push 0x12345678 __asm int 3 __asm add esp, 16 }
#endif

#endif
#endif//assert

//----------------------------
#if defined DEBUG_LOG
void LOG_RUN(const char *msg);
void LOG_RUN_N(const char *msg, int n);
void LOG_DEBUG(const char *msg);
void LOG_DEBUG_N(const char *msg, int n);
#else
#define LOG_RUN(msg) (void)0
#define LOG_RUN_N(msg, n) (void)0
#define LOG_DEBUG(msg) (void)0
#define LOG_DEBUG_N(msg, n) (void)0
#endif

//----------------------------
// Get current tick counter, in ms. The count beginning is not specified, important is valid delta between calls.
dword GetTickTime();

//----------------------------
class C_application_base;

namespace system{

//----------------------------
// Get device model identifier. Platform-specific, on Symbian it is device UID.
dword GetDeviceId();

//----------------------------
// Set application as system, not closed by OS automatically.
void SetAsSystemApp(bool);

//----------------------------
// Restart device (doesn't work on all devices!)
void RebootDevice();

//----------------------------
// Check if device can be rebooted.
bool CanRebootDevice();

//----------------------------
// Pause thread for specified time.
void Sleep(dword time);

#ifdef _WIN32_WCE
//----------------------------
// Run modal dialog asking user to enter device owner name.
bool WM_RunOwnerNameDialog(C_application_base &app);
#endif

//----------------------------
// Open given file by system.
// app_uid ... ID of application that is used to open the file (on Symbian)
bool OpenFileBySystem(C_application_base &app, const wchar *filename, dword app_uid = 0);

//----------------------------
// Open web browser with given url.
void OpenWebBrowser(C_application_base &app, const char *url);

//----------------------------
// Check if application is installed.
bool IsApplicationInstalled(
#ifdef __SYMBIAN32__
   dword uid
#else
   const char *name
#endif
);

#ifdef ANDROID
//----------------------------
// Search for item(s) on Android market.
// 'what' may be in forms:
//    search?q=pub:"<publisher name>"   - displays all apps of publisher
//    details?id=<app_package>  - displays details of app
bool SearchOnAndroidMarket(const char *what);
#endif

}//system

//----------------------------

#define OffsetOf(s, m) (dword(&((s*)4)->m)-4)   //use non-NULL pointer to avoid gcc warning

#ifndef __E32STD_H__ //<- this Symbian header defines these functions
                              //min & max
template<class T> const T &Max(const T &t1, const T &t2){ return t1>t2 ? t1 : t2; }
template<class T> const T &Min(const T &t1, const T &t2){ return t1<t2 ? t1 : t2; }
template<class T> T Abs(T t){ return t>=0 ? t : -t; }

void *operator new(size_t sz);
void operator delete(void*)
#ifdef __MWERKS__
   throw()
#endif
;

                              //placement new/delete
inline void *operator new(size_t sz, void *p){ return p; }
inline void operator delete(void*, void*){ }

#endif

void *operator new(size_t sz, bool);
void operator delete(void *vp, bool);

void *operator new[](size_t sz);
void operator delete[](void *vp)
#ifdef __MWERKS__
   throw()
#endif
;

inline void *operator new[](size_t sz, bool l){ return operator new(sz, l); }
inline void operator delete[](void *vp, bool l){ operator delete[](vp); }

//----------------------------
#ifdef __SYMBIAN32__

// Return this application's Symbian UID.
dword GetSymbianAppUid();

                              //interface for safely calling Symbian functions that may leave.
class C_leave_func{
//----------------------------
// Implementation function for derived class, where Symbian leaving function can be called.
   virtual int Run() = 0;
public:
//----------------------------
// Function running Run operation, and returning either result of Run, or result of Leave.
   int Execute();
};

#endif //__SYMBIAN32__
//----------------------------
#ifdef _WIN32_WCE
// Check if Windows Mobile platform is Smartphone (no touch screen) or PocketPC (has touch screen).
bool IsWMSmartphone();
#endif
//----------------------------
// Quick-sort implementation.
// base ... pointer to beginning of sorted array
// num ... number of items to sort
// size_of_item ... number of bytes of one item
// p_Comp ... compare function; i1,i2 are pointers to items; context is same value passed to QuickSort function
// context ... user's value passed to callback functions
// p_SwapElements ... function to swap two items; leave NULL to use plain copy of bytes; or specify function for more complex swap of items
void QuickSort(void *base, dword num, dword size_of_item,
   int (*p_Comp)(const void *i1, const void *i2, void *context), void *context = NULL,
   void (*p_SwapElements)(void *i1, void *i2, dword size_of_item, void *context) = NULL);

//----------------------------
                              //generate four-cc code
#define FOUR_CC(a, b, c, d) dword((byte(d)<<24) | (byte(c)<<16) | (byte(b)<<8) | byte(a))
//----------------------------

#endif

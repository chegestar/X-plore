#ifndef __RULES_H
#define __RULES_H
//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

                              //warning settings for MS compiler
#ifdef _MSC_VER
#pragma warning(disable: 4800)//forcing value to bool 'true' or 'false'
#pragma warning(disable: 4100)//unreferenced formal parameter
#pragma warning(disable: 4505)//unreferenced local function has been removed
#pragma warning(disable: 4127)//conditional expression is constant
#pragma warning(disable: 4512)//assignment operator could not be generated
#define _DEPRECATED __declspec(deprecated)
#else
#define _DEPRECATED
#endif

//----------------------------

typedef unsigned char byte;
typedef signed char schar;
typedef unsigned short word;
typedef unsigned long dword;
typedef unsigned long ulong;
typedef long long longlong;

#if defined __GCC32__ || defined __BORLANDC__ || defined _NATIVE_WCHAR_T_DEFINED || defined BADA || defined ANDROID || defined __CW32__
typedef wchar_t wchar;
#else
typedef unsigned short wchar;
#endif

#if !(defined _SIZE_T_DEFINED_ || defined _SIZE_T_DEFINED)
#define _SIZE_T_DEFINED_
#define _SIZE_T_DEFINED
#ifdef __BORLANDC__
namespace std;
typedef unsigned int std::size_t;
#else
typedef unsigned int size_t;
#endif
#endif

#ifndef NULL
 #define NULL 0
#endif

//----------------------------

#endif

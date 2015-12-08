//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#ifndef __PHONE_H
#define __PHONE_H

#include <Rules.h>
#include <Cstr.h>

//----------------------------

class C_vibration{
   void *imp;
public:
   C_vibration();
   ~C_vibration();
//----------------------------
// Check if this device supports vibration.
   bool IsSupported();

//----------------------------
// Vibrate device for specified time.
   bool Vibrate(dword ms);
};

//----------------------------

class C_phone_profile{
public:
#if (defined S60 && defined __SYMBIAN_3RD__) || defined ANDROID
//----------------------------
// Check if vibration is enabled in active profile.
   static bool IsVibrationEnabled();
#endif

#if defined S60 && defined __SYMBIAN_3RD__
//----------------------------
// Get email alert tone (filename) from active profile. On (older) phones where it is not supported it returns false;
   static bool GetEmailAlertTone(Cstr_w &fn);

//----------------------------
// Get volume of active profile, in percent. On (older) phones where it is not supported it returns 100.
   static int GetProfileVolume();
#endif

//----------------------------
// Check if device's profile is set to silent.
   static bool IsSilentProfile();
};

//----------------------------

#endif

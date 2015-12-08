#ifndef __SIMPLESOUNDPLAYER_H_
#define __SIMPLESOUNDPLAYER_H_

#include <Rules.h>
#include <SmartPtr.h>

//----------------------------

class C_simple_sound_player: public C_unknown{
public:
   virtual ~C_simple_sound_player(){}
   virtual bool IsDone() const = 0;
   virtual dword GetPlayTime() const = 0;
   virtual dword GetPlayPos() const = 0;
   virtual void SetPlayPos(dword) = 0;
   virtual void SetVolume(int vol) = 0;
   virtual void Pause() = 0;
   virtual void Resume() = 0;
   virtual bool HasPositioning() const = 0;

//----------------------------
// Create player.
// volume = 0 - 10, if set to -1 then system default volume is used.
   static C_simple_sound_player *Create(const wchar *fn, int volume = -1);
};

//----------------------------
#endif

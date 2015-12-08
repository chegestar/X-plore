#include <Rules.h>
#include <SmartPtr.h>

//----------------------------

class C_simple_video_player: public C_unknown{
public:
   virtual ~C_simple_video_player(){}
   virtual bool IsDone() const = 0;
   virtual dword GetPlayTime() const = 0;
   virtual dword GetPlayPos() const = 0;
   virtual void SetPlayPos(dword) = 0;
   virtual void SetVolume(dword vol) = 0;
   virtual void Pause() = 0;
   virtual void Resume() = 0;
   virtual bool HasPositioning() const = 0;
   virtual void SetClipRect(const S_rect &rc_clip) = 0;
   virtual const S_rect &GetRect() const = 0;
   virtual void GetFrame() const{}
   virtual bool IsAudioOnly() const{ return false; }

   static C_simple_video_player *Create(C_application_base &app, IGraph *igraph, const wchar *fn, const S_rect &rc_clip, dword vol);
};

//----------------------------

class C_client_video_preview: public C_client{
private:
   class C_mode_this;
   friend class C_mode_this;

   bool SetMode(const wchar *fn, byte &volume);

//----------------------------

   class C_mode_this: public C_mode_app<C_client>{
   public:
      S_rect rc;
      C_smart_ptr<C_simple_video_player> plr;
      const Cstr_w fn;
      Cstr_w fn_raw;
      bool paused, finished;
      byte &volume;
      S_rect rc_clip;
      mutable dword last_drawn_second;
      void EnableBacklight(bool on);
      void DrawBottomBar() const;

      C_mode_this(C_client &_app, byte &vol, const Cstr_w &_fn):
         C_mode_app<C_client>(_app),
         fn(_fn),
         paused(false),
         finished(false),
         volume(vol),
         last_drawn_second(0)
      {
         EnableBacklight(true);
      }
      ~C_mode_this();
      virtual void InitLayout();
      virtual void ProcessInput(S_user_input &ui, bool &redraw);
      virtual void Tick(dword time, bool &redraw);
      virtual void Draw() const;

      void DrawTimePos() const;
      void DrawPositionTime() const;
      virtual void FocusChange(bool foreground){
         if(foreground)
            Draw();
      }
   };
public:
   static bool CreateMode(C_client *cl, const wchar *fn, byte &volume){
      C_client_video_preview *_this = (C_client_video_preview*)cl;
      return _this->SetMode(fn, volume);
   }
};

//----------------------------

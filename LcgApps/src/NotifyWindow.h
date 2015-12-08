#include <Rules.h>
#include <SmartPtr.h>

//----------------------------
                              //class for notification in small window
                              // when it's created, it is displayed (as topmost window or on status bar, depending on platform)
class C_notify_window: public C_unknown{
protected:
   dword width;
public:
   dword display_count;

   C_notify_window():
      display_count(0)
   {}

#ifdef __SYMBIAN32__
   virtual void DrawToBuffer(const struct S_rect &rc, const void *src, dword pitch) = 0;
#endif
#ifdef _WIN32_WCE
   virtual void ShowNotify() = 0;
#endif
#ifdef ANDROID
   virtual void ShowNotify(dword icon, const wchar *name, const wchar *title, const wchar *text, const wchar *activity_name, bool led_flash) = 0;
#endif

   inline dword GetWidth() const{ return width; }

   static C_notify_window *Create(class C_application_base &app);
};

//----------------------------

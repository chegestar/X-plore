//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <IGraph.h>
#include <Cstr.h>

#include <Math.h>
#include <Util.h>
#include <String.h>
#include <AppBase.h>
#include <TimeDate.h>

namespace win{
#ifdef _WIN32_WCE
//#define USE_MFC
#ifdef USE_MFC
#include <Afxwin.h>
#else
#include <Windows.h>
#endif
#include <aygshell.h>
#pragma comment(lib, "aygshell.lib")

//----------------------------

#define USE_GX
typedef struct _RawFrameBufferInfo{
   win::WORD wFormat;
   win::WORD wBPP;
   VOID *pFramePointer;
   int cxStride, cyStride;
   int cxPixels, cyPixels;
 } RawFrameBufferInfo;

#else
#include <Windows.h>
#include <zmouse.h>
#endif
}
#undef GetCharWidth

#if !defined _WIN32_WCE && defined _DEBUG
#include <TextUtils.h>
#endif

#ifdef USE_GX
//#include <gx.h>
#define GXDLL_API __declspec(dllimport)
struct GXKeyList {
   short vkUp;             // key for up
   win::POINT ptUp;             // x,y position of key/button.  Not on screen but in screen coordinates.
   short vkDown;
   win::POINT ptDown;
   short vkLeft;
   win::POINT ptLeft;
   short vkRight;
   win::POINT ptRight;
   short vkA;
   win::POINT ptA;
   short vkB;
   win::POINT ptB;
   short vkC;
   win::POINT ptC;
   short vkStart;
   win::POINT ptStart;
};
struct GXDisplayProperties {
   win::DWORD cxWidth;
   win::DWORD cyHeight;         // notice lack of 'th' in the word height.
   long cbxPitch;          // number of bytes to move right one x pixel - can be negative.
   long cbyPitch;          // number of bytes to move down one y pixel - can be negative.
   long cBPP;              // # of bits in each pixel
   win::DWORD ffFormat;         // format flags.
};
//GXDLL_API int GXResume();
//GXDLL_API int GXSuspend();
//GXDLL_API int GXOpenInput();
//GXDLL_API GXKeyList GXGetDefaultKeys(int iOptions);
#define GX_FULLSCREEN   0x01        // for OpenDisplay()
#define GX_NORMALKEYS   0x02
#define GX_LANDSCAPEKEYS    0x03
//GXDLL_API GXDisplayProperties GXGetDisplayProperties();
//GXDLL_API int GXOpenDisplay(struct HWND__ *hWnd, win::DWORD dwFlags);
//GXDLL_API int GXSetViewport(win::DWORD dwTop, win::DWORD dwHeight, win::DWORD dwReserved1, win::DWORD dwReserved2 );
//GXDLL_API int GXCloseDisplay();
//GXDLL_API void *GXBeginDraw();
//GXDLL_API int GXEndDraw();
//#pragma comment(lib, "gx")
#endif//USE_GX

#define GETRAWFRAMEBUFFER   0x00020001

#ifndef _WIN32_WCE
#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")
#ifdef _DEBUG
extern bool debug_has_kb, debug_has_full_kb;
extern bool debug_draw_controls_rects;
#endif
#endif

//extern bool win_is_system_app;

#pragma warning(disable: 4996) //'wcscpy' was declared deprecated
#undef DrawText

                              //backlight registry keys
static const wchar reg_name_backlight[] = L"ControlPanel\\Backlight";
static const wchar reg_battery_timeout[] = L"BatteryTimeout";
static const wchar reg_ACTimeout[] = L"ACTimeout";

static const wchar reg_name_power_timouts[] = L"System\\CurrentControlSet\\Control\\Power\\Timeouts";
static const wchar reg_BattUserIdle[] = L"BattUserIdle";
static const wchar reg_ACUserIdle[] = L"ACUserIdle";
static const wchar reg_BattSystemIdle[] = L"BattSystemIdle";
static const wchar reg_ACSystemIdle[] = L"ACSystemIdle";
static const wchar reg_BattSuspend[] = L"BattSuspend";
static const wchar reg_ACSuspend[] = L"ACSuspend";


//----------------------------

void GetClassName(wchar buf[MAX_PATH]);

//----------------------------

class IGraph_imp: public IGraph{
   enum{
      WM_UPDATE_FLAGS = WM_USER+100,
   };

   C_application_base *app;
   bool in_event;             //flag set when app's event is called, to avoid re-entrancy of events

   bool allow_draw;

#ifdef _WIN32_WCE
   void *framebuffer;         //direct address of screen buffer; if NULL, then we're using GX or BitBlt
#endif
   byte *backbuffer;

                              //backlight settings
   bool backlight_on;
#ifdef _WIN32_WCE
   win::HANDLE h_backlight_event;
   mutable dword last_time_reset_sleep_timer;

   dword save_reg_battery_timeout;
   dword save_reg_ACTimeout;

   dword save_reg_BattUserIdle;
   dword save_reg_ACUserIdle;
   dword save_reg_BattSystemIdle;
   dword save_reg_ACSystemIdle;
   dword save_reg_BattSuspend;
   dword save_reg_ACSuspend;

   int save_gui_rotation;     //screen rotation angle

#ifdef _WIN32_WCE
   win::SHACTIVATEINFO sip;
#endif
   win::HWND hwnd_menubar;
#endif

   friend IGraph *IGraphCreate(C_application_base *app, dword flags);

   enum{
      FLG_HW_LANSCAPE = 2,    //physical screen is landscape aligned
      //FLG_NATIVE_ROTATON = 4, //use screen buffer in its native rotation mode
      FLG_BBUF_DIRTY = 8,     //set when automatically rescaled, to avoid WM_PAINT drawing garbage
   };
   mutable dword internal_flags;
   enum{
      KEY_BUF_SIZE = 16,
   };

   int pitch_x, pitch_y;
   S_point fullscreen_sz;
   dword key_bits;           //currently pressed combination of keys

   win::HWND hwnd;

#ifdef _WINDOWS
   bool mouse_down, mouse_down_r;
   //S_point prev_sec_pos;
#endif
   bool temp_no_resize;

                              //bitmap blitting
#ifdef USE_MFC
   mutable CDC cdc;
#endif
   win::HDC hdc_bb;
   win::HBITMAP bmp_bb;
   win::HGDIOBJ hdc_save;

#ifdef USE_GX
   mutable win::HINSTANCE hi_gx;
   mutable bool gx_inited;
   bool InitGx() const{
      if(!gx_inited){
         hi_gx = win::LoadLibrary(L"gx.dll");
         gx_inited = true;
      }
      return (hi_gx!=NULL);
   }

   void GXResume(){
      if(InitGx()){
         typedef int (*t_GXResume)();
         t_GXResume fp = (t_GXResume)win::GetProcAddress(hi_gx, L"?GXResume@@YAHXZ");
         if(fp)
            fp();
      }
   }
   void GXSuspend(){
      if(InitGx()){
         typedef int (*t_GXSuspend)();
         t_GXSuspend fp = (t_GXSuspend)win::GetProcAddress(hi_gx, L"?GXSuspend@@YAHXZ");
         if(fp)
            fp();
      }
   }
   bool GXGetDisplayProperties(GXDisplayProperties &dp){
      if(InitGx()){
         typedef GXDisplayProperties (*t_GXGetDisplayProperties)();
         t_GXGetDisplayProperties fp = (t_GXGetDisplayProperties)win::GetProcAddress(hi_gx, L"?GXGetDisplayProperties@@YA?AUGXDisplayProperties@@XZ");
         if(fp){
            dp = fp();
            return true;
         }
      }
      return false;
   }
   void GXOpenDisplay(struct HWND__ *hWnd, win::DWORD dwFlags){
      if(InitGx()){
         typedef int (*t_GXOpenDisplay)(struct HWND__ *hWnd, win::DWORD dwFlags);
         t_GXOpenDisplay fp = (t_GXOpenDisplay)win::GetProcAddress(hi_gx, L"?GXOpenDisplay@@YAHPAUHWND__@@K@Z");
         if(fp)
            fp(hWnd, dwFlags);
      }
   }
   void GXCloseDisplay(){
      if(InitGx()){
         typedef int (*t_GXCloseDisplay)();
         t_GXCloseDisplay fp = (t_GXCloseDisplay)win::GetProcAddress(hi_gx, L"?GXCloseDisplay@@YAHXZ");
         if(fp)
            fp();
      }
   }
   void GXSetViewport(win::DWORD dwTop, win::DWORD dwHeight){
      if(InitGx()){
         typedef int (*t_GXSetViewport)(win::DWORD dwTop, win::DWORD dwHeight, win::DWORD dwReserved1, win::DWORD dwReserved2);
         t_GXSetViewport fp = (t_GXSetViewport)win::GetProcAddress(hi_gx, L"?GXSetViewport@@YAHKKKK@Z");
         if(fp)
            fp(dwTop, dwHeight, 0, 0);
      }
   }
   void *GXBeginDraw() const{
      if(InitGx()){
         typedef void *(*t_GXBeginDraw)();
         t_GXBeginDraw fp = (t_GXBeginDraw)win::GetProcAddress(hi_gx, L"?GXBeginDraw@@YAPAXXZ");
         if(fp)
            return fp();
      }
      return NULL;
   }
   void GXEndDraw() const{
      if(InitGx()){
         typedef int (*t_GXEndDraw)();
         t_GXEndDraw fp = (t_GXEndDraw)win::GetProcAddress(hi_gx, L"?GXEndDraw@@YAHXZ");
         if(fp)
            fp();
      }
   }
#endif
//----------------------------

#ifdef _WIN32_WCE

   void CheckResize(){

      if(!backbuffer)
         return;
      assert(!temp_no_resize);
      temp_no_resize = true;
      win::SIPINFO si;
      MemSet(&si, 0, sizeof(si));
      si.cbSize = sizeof(si);
      if(win::SipGetInfo(&si)){
         win::RECT rc;
         win::SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
         dword sy;
         dword top = (app->ig_flags&IG_SCREEN_USE_CLIENT_RECT) ? si.rcVisibleDesktop.top : 0;
         //dword top = si.rcVisibleDesktop.top;

         if(si.fdwFlags&SIPF_ON){
            sy = si.rcVisibleDesktop.bottom - top;
         }else{
            sy = fullscreen_sz.y;
            if(app->ig_flags&IG_SCREEN_SHOW_SIP)
               sy -= si.rcVisibleDesktop.top;
            if(app->ig_flags&IG_SCREEN_USE_CLIENT_RECT)
               sy -= top;
            //sy = si.rcVisibleDesktop.bottom - si.rcVisibleDesktop.top;
         }
                              //if rotation changed, force resize
         if(save_gui_rotation!=GetGuiRotation())
            app->_scrn_sy = 0;
         if(app->ScrnSY()!=sy){
            ResizeScreen(app->ScrnSX(), sy);
            internal_flags |= FLG_BBUF_DIRTY;
         }
      }
      temp_no_resize = false;
   }
#endif
//----------------------------

   void ResizeScreen(dword sx, dword sy){

      bool tnr = temp_no_resize;
      temp_no_resize = true;

      bool resized = (int(sx)!=app->ScrnSX() || int(sy)!=app->ScrnSY());
#ifdef _WIN32_WCE
      int gui_rot = GetGuiRotation();
      if(gui_rot!=save_gui_rotation){
         CloseScreen(false);
         save_gui_rotation = gui_rot;
         InitScreen(false);
      }else
#endif
      {
         app->_scrn_sx = sx;
         app->_scrn_sy = sy;
         if(backbuffer){
            DeleteBackbuffer();
            CreateBackbuffer();
         }
         app->FillRect(S_rect(0, 0, sx, sy), 0xff000000);
                        //resize window
#ifndef _WIN32_WCE
         win::RECT rc = {0, 0, app->ScrnSX(), app->ScrnSY() };
         AdjustWindowRect(&rc, GetWindowLong(hwnd, GWL_STYLE), false);
         dword wsx = rc.right - rc.left, wsy = rc.bottom - rc.top;
#else
         dword wsx = app->ScrnSX();
         dword wsy = app->ScrnSY();
         //SipShowIM(SIPF_OFF);
         if(!(app->ig_flags&IG_SCREEN_USE_CLIENT_RECT))
            win::SHFullScreen(hwnd, SHFS_HIDETASKBAR);
#endif
         win::SetWindowPos(hwnd, NULL, 0, 0, wsx, wsy, SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);
      }
#ifdef _WIN32_WCE
                           //put below screen (do not hide)
      bool hide_menu = false;
      if(IsWMSmartphone())
         hide_menu = !(app->ig_flags&IG_SCREEN_USE_CLIENT_RECT);
      else
         hide_menu = false;//!(app->ig_flags&IG_SCREEN_SHOW_SIP);
      if(hide_menu){
         //KillTimer(hwnd, 1);
         HideMenubar();
         win::SetTimer(hwnd, 1, 1000, NULL);
         //PostMessage(hwnd, WM_USER+101, 0, 0);
      }
#endif
      //ProcessWinMessages();
      app->InitInputCaps();
      app->ScreenReinit(resized);
      temp_no_resize = tnr;
   }

//----------------------------

                              //workaround for simulation of shift keys by other keys on devices that don't have shift key
   struct S_shift_key_workaround{
      bool any_key_down;
      S_shift_key_workaround():
         any_key_down(true)
      {
      }
   } shift_key_workaround;

//----------------------------

   void KeyEvent(int vk, bool down, bool autorepeat, bool allow_key_bits = true){

      dword bits = key_bits;
      if(allow_key_bits && !autorepeat){
         dword bit = 0;
         switch(vk){
         case VK_LEFT: bit = GKEY_LEFT; break;
         case VK_RIGHT: bit = GKEY_RIGHT; break;
         case VK_DOWN: bit = GKEY_DOWN; break;
         case VK_UP: bit = GKEY_UP; break;
         case VK_RETURN: bit = GKEY_OK; break;
         //case VK_BACK: bit = GKEY_CLEAR; break;
         case VK_CAPITAL:
         case VK_SHIFT: bit = GKEY_SHIFT; break;
         case VK_CONTROL: bit = GKEY_CTRL; break;
         case VK_MENU: bit = (dword)GKEY_ALT; break;
         //case VK_F8: bit = GKEY_ASTERISK; break;
         case VK_F3: bit = GKEY_SEND; break;
         case VK_F9:
#ifdef _WIN32_WCE
            if(IsWMSmartphone()){
                                 //simulate shift key
               if(down){
                  key_bits |= GKEY_SHIFT;
                  shift_key_workaround.any_key_down = false;
                  //app->ProcessInputInternal(K_NOKEY, false, key_bits, 0, 0, 0);
               }else{
                  key_bits &= ~GKEY_SHIFT;
                  in_event = true;
                  if(!shift_key_workaround.any_key_down){
                              //post hash key now
                     app->ProcessInputInternal((IG_KEY)'#', autorepeat, key_bits, S_point(0, 0), 0);
                  }else
                     app->ProcessInputInternal(K_NOKEY, autorepeat, key_bits, S_point(0, 0), 0);
                  in_event = false;
               }
               return;
            }
#endif
            //bit = GKEY_HASH;
            break;
#ifdef _WINDOWS
                              //simulated keys
         case VK_PRIOR:
            bit = (key_bits&GKEY_CTRL) ? K_PAGE_UP : GKEY_SOFT_MENU;
            break;
         case VK_NEXT:
            bit = (key_bits&GKEY_CTRL) ? K_PAGE_DOWN : GKEY_SOFT_SECONDARY;
            break;
         case VK_ADD: bit = GKEY_SEND; break;
#else
         case VK_F1: bit = GKEY_SOFT_MENU; break;
         case VK_F2: bit = GKEY_SOFT_SECONDARY; break;
#endif
         }
         if(bit){
            if(down)
               bits |= bit;
            else
               bits &= ~bit;
         }
      }
      shift_key_workaround.any_key_down = true;
      bool shift = (key_bits&GKEY_SHIFT);
      switch(vk){
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
#ifdef _WINDOWS
         if(shift){
            switch(vk){
            case '0': vk = ')'; break;
            case '1': vk = '!'; break;
            case '2': vk = '@'; break;
            case '3': vk = '#'; break;
            case '4': vk = '$'; break;
            case '5': vk = '%'; break;
            case '6': vk = '^'; break;
            case '7': vk = '&'; break;
            case '8': vk = '*'; break;
            case '9': vk = '('; break;
            }
            break;
         }
#endif
         if(allow_key_bits){
            dword bit = (1<< (vk-'0')) * GKEY_0;
            if(down)
               bits |= bit;
            else
               bits &= ~bit;
         }
         break;
      case VK_LEFT: vk = K_CURSORLEFT; break;
      case VK_UP: vk = K_CURSORUP; break;
      case VK_RIGHT: vk = K_CURSORRIGHT; break;
      case VK_DOWN: vk = K_CURSORDOWN; break;
      case VK_DELETE: vk = K_DEL; break;
      case VK_RETURN:
         vk = K_ENTER;
         break;
      case VK_BACK:
#ifdef _WIN32_WCE
                              //SP phones have "Del" key which produces backspace code, fix it here
         vk = K_DEL;
#else
         vk = K_BACKSPACE;
#endif
         break;
      case VK_TAB: vk = K_TAB; break;
      case VK_CAPITAL:
      case VK_SHIFT: vk = K_SHIFT; break;
      case VK_CONTROL: vk = K_CTRL; break;
      case VK_MENU: vk = K_ALT; break;
      case ' ': vk = ' '; break;
#ifdef _WINDOWS
                              //simulate softkeys
      case VK_F1: vk = K_SOFT_MENU; break;
      case VK_F2: vk = K_SOFT_SECONDARY; break;
      case VK_PRIOR:
         if(key_bits&GKEY_CTRL)
            vk = K_PAGE_UP;
         else
            vk = K_SOFT_MENU;
         break;
      case VK_NEXT:
         if(key_bits&GKEY_CTRL)
            vk = K_PAGE_DOWN;
         else
            vk = K_SOFT_SECONDARY;
         break;
      case VK_MULTIPLY: vk = '*'; break;
      case VK_DIVIDE: vk = '#'; break;
      case VK_ADD: vk = K_SEND; break;
      case VK_INSERT: vk = K_MENU; break;
      case VK_F7: vk = K_VOLUME_UP; break;
      case VK_F8: vk = K_VOLUME_DOWN; break;
      //case VK_SUBTRACT: vk = K_CLOSE; break;
      case VK_ESCAPE: vk = K_ESC; break;
      case K_SOFT_MENU:
      case K_SOFT_SECONDARY:
      case K_MEDIA_BACKWARD:
      case K_MEDIA_FORWARD:
      case K_MEDIA_PLAY_PAUSE:
      case K_MEDIA_STOP:
         break;
#define MAP_K(code, ns, s) case code: vk = !shift ? ns : s; break
      MAP_K(0xba, ';', ':');
      MAP_K(0xbb, '=', '+');
      MAP_K(0xbc, ',', '<');
      MAP_K(0xbd, '-', '_');
      MAP_K(0xbe, '.', '>');
      MAP_K(0xbf, '/', '?');
      MAP_K(0xc0, '`', '~');
      MAP_K(0xdb, '[', '{');
      MAP_K(0xdc, '\\', '|');
      MAP_K(0xdd, ']', '}');
      MAP_K(0xde, '\'', '\"');
#if defined _DEBUG && !defined _WIN32_WCE
      case VK_F4:
         if(down)
            SendMessage(hwnd, WM_SYSCOMMAND, 7, 0);
         return;
      case VK_F5:
         if(down)
            SendMessage(hwnd, WM_SYSCOMMAND, 2, 0);
         return;
      case VK_F6:
         if(down)
            SendMessage(hwnd, WM_SYSCOMMAND, 8, 0);
         return;
      case VK_F9:
         if(down)
            SendMessage(hwnd, WM_SYSCOMMAND, 6, 0);
         return;
      case VK_F10:
         if(down){
                              //debug clear screen without dirty rect, for checking redraws
            app->ClearScreen();
            app->UpdateScreen();
            app->ClearScreen(0xffff0000);
            app->ResetDirtyRect();
         }
         return;
      case VK_F11:
                              //simulate screen resize
         if(down
#ifdef NDEBUG
            && (app->ig_flags&IG_SCREEN_USE_CLIENT_RECT)
#endif
            ){
            fullscreen_sz.x = (app->ScrnSY()+2) & -4;
            fullscreen_sz.y = app->ScrnSX();
            ResizeScreen(fullscreen_sz.x, fullscreen_sz.y);
            debug_setting.res = fullscreen_sz;
            debug_setting.Save();
         }
         return;
#endif
#else//_WINDOWS
                              //WM makes some keys coded as F-keys, detect it here
      case VK_F1:
      case VK_F2:
         vk = vk==(app->menu_on_right ? VK_F2 : VK_F1) ? K_SOFT_MENU : K_SOFT_SECONDARY;
                              //simulate down, as system doesn't send it if GX is not initialized
         //if(app->ig_flags&(IG_SCREEN_NO_DSA|IG_SCREEN_USE_CLIENT_RECT))
         if(!IsWMSmartphone())
            down = true;
         break;
      case VK_F3: vk = K_SEND; break;
      case VK_F4: vk = K_HANG; down = true; break; //down message is not sent for hang key?
      case VK_F8: vk = '*'; break;
      case VK_F9: vk = '#'; break;
      case VK_ESCAPE:
         /*
         if(IsWMSmartphone())
            vk = K_DEL;       //map to delete, as there's no delete key on Smartphone
         else
         */
            vk = K_BACK;
         break;
      case 0xc1: vk = K_HARD_1; break;
      case 0xc2: vk = K_HARD_2; break;
      case 0xc3: vk = K_HARD_3; break;
      case 0xc4: vk = K_HARD_4; break;

      case VK_PRIOR: vk = K_PAGE_UP; break;
      case VK_NEXT: vk = K_PAGE_DOWN; break;
#endif//!_WINDOWS
      case VK_HOME: vk = K_HOME; break;
      case VK_END: vk = K_END; break;

      default:
         if(vk>='A' && vk<='Z'){
            if(!shift)
               vk += 'a' - 'A';
         }else
            vk = 0;
      }
      if((down && vk) || key_bits!=bits){
         key_bits = bits;
         if(!down)
            vk = 0;
         if(!in_event){
            in_event = true;
            app->ProcessInputInternal((IG_KEY)vk, autorepeat, key_bits, S_point(0, 0), 0);
            in_event = false;
         }
      }
   }

//----------------------------
#ifdef _WIN32_WCE

   void HideMenubar(){
                              //put below screen (do not hide)
      win::SetWindowPos(hwnd_menubar, NULL, 0, app->ScrnSY(), 0, 0, SWP_NOSIZE);
   }

   void CreateMenubar(win::HWND hwnd){

      win::HINSTANCE hi = win::GetModuleHandle(NULL);
      win::SHMENUBARINFO mi = {
         sizeof(mi),
         hwnd,
         //SHCMBF_EMPTYBAR |
         //SHCMBF_COLORBK |
         SHCMBF_HMENU |
         0,
         1,
         hi,
         0, 0,
         NULL, 0
      };
      if(!win::SHCreateMenuBar(&mi)){
                              //SmartPhone 2002/2003 works this way
         mi.dwFlags = SHCMBF_EMPTYBAR;
         mi.nToolBarId = 0;
         if(!win::SHCreateMenuBar(&mi))
            return;
      }
      hwnd_menubar = mi.hwndMB;
      if(IsWMSmartphone()){
#define GRAB_KEY(k) win::SendMessage(hwnd_menubar, SHCMBM_OVERRIDEKEY, k, (win::LPARAM)((win::LONG)(((win::WORD)(SHMBOF_NODEFAULT|SHMBOF_NOTIFY)) | ((win::DWORD)((win::WORD)(SHMBOF_NODEFAULT|SHMBOF_NOTIFY))) << 16)))
         GRAB_KEY(VK_F1);
         GRAB_KEY(VK_F2);
         GRAB_KEY(VK_ESCAPE);
         GRAB_KEY(VK_F3);
         if(!(app->ig_flags&IG_SCREEN_USE_CLIENT_RECT))
            HideMenubar();
      }else{
         //if(!(app->ig_flags&IG_SCREEN_USE_CLIENT_RECT))
         win::CommandBar_Show(hwnd_menubar, bool(app->ig_flags&IG_SCREEN_SHOW_SIP));
         //if(!(app->ig_flags&IG_SCREEN_SHOW_SIP))
            //HideMenubar();
      }
   }

#endif
//----------------------------
   dword timed_changed_flags, timed_new_init_flags;

   win::LRESULT WndProc(win::HWND hwnd, win::UINT msg, win::WPARAM wParam, win::LPARAM lParam){

      switch(msg){

      case WM_CREATE:
         {
#ifdef _WIN32_WCE
            MemSet(&sip, 0, sizeof(sip));
            sip.cbSize = sizeof(sip);
            CreateMenubar(hwnd);
#ifndef _DEBUG
                              //time to kill debugger for fighting hackers
            void KillDebugger(); KillDebugger();
#endif
#endif//_WIN32_WCE
#if !defined _WIN32_WCE && defined _DEBUG
                              //timer for refreshing mem info
            win::SetTimer(hwnd, 3, 1000, NULL);
#endif
         }
         break;

#ifdef _WIN32_WCE
      case WM_SETTINGCHANGE:
         if(IsWMSmartphone())
            break;
         switch(wParam){
         case SPI_SETSIPINFO:
            /*
            if(app->ig_flags&IG_SCREEN_SHOW_SIP){
               bool tnr = temp_no_resize;
               temp_no_resize = true;
               SHHandleWMSettingChange(hwnd, wParam, lParam, &sip);
               temp_no_resize = tnr;
            }
               */
            if(!temp_no_resize)
               CheckResize();
            break;
            /*
         default:
            SHHandleWMSettingChange(hwnd, wParam, lParam, &sip);
         */
         }
         break;
#endif//_WIN32_WCE

      case WM_SIZE:
         if(temp_no_resize)
            break;
         switch(wParam){
         case SIZE_RESTORED:
         case SIZE_MAXIMIZED:
            {
               int sx = word(lParam&0xffff);
               sx = (sx+2) & -4;
               int sy = word(lParam>>16);
#ifndef _WIN32_WCE
               if(sx!=(lParam&0xffff)){
                              //fix window width
                  temp_no_resize = true;
                  win::RECT rc = {0, 0, sx, sy };
                  AdjustWindowRect(&rc, GetWindowLong(hwnd, GWL_STYLE), false);
                  win::SetWindowPos(hwnd, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOZORDER|SWP_NOCOPYBITS);
                  temp_no_resize = false;
               }
               fullscreen_sz.x = sx;
               fullscreen_sz.y = sy;
#endif
               if(app->ScrnSX()!=sx || app->ScrnSY()!=sy)
                  ResizeScreen(sx, sy);
#if defined _DEBUG && !defined _WIN32_WCE
               debug_setting.res = fullscreen_sz;
               debug_setting.Save();
#endif
            }
            break;
         }
         break;

      case WM_UPDATE_FLAGS:
         if(app){
            app->ig_flags = wParam;
            UpdateFlags(wParam, lParam, true);
         }
         return 0;

      case WM_TIMER:
#ifdef _WIN32_WCE
         if(wParam==1){
                           //put below screen (do not hide)
            KillTimer(hwnd, 1);
            HideMenubar();
            return 0;
         }
#endif
         if(wParam==2){
            KillTimer(hwnd, 2);
            app->ig_flags = timed_new_init_flags;
            UpdateFlags(timed_new_init_flags, timed_changed_flags, true);
            return 0;
         }
#if !defined _WIN32_WCE && defined _DEBUG
         if(wParam==3){
            void SetMemAllocLimit(dword bytes);
            SetMemAllocLimit(0);
                              //refresh memory info
            void GetMemInfo(dword &num_blocks, dword &total_size);
            dword num_blocks, total_size;
            GetMemInfo(num_blocks, total_size);

            wchar tmp_buf[256];
            Cstr_w s, fs;
            s<<GetWindowTitle(tmp_buf);
            s.AppendFormat(L"   Mem: % blks, %")
               <<num_blocks
               <<text_utils::MakeFileSizeText(total_size, false, true);
            win::SetWindowTextW(hwnd, s);
            SetMemAllocLimit(debug_setting.memory_limit);
            return 0;
         }
#endif
         {
            if(in_event)
               break;
            class C_timer_imp: public C_application_base::C_timer{
            public:
               C_application_base *app;
               enum{
                  TYPE_PERIODIC,
                  TYPE_ALARM,
               } type;
               void *context;
               union{
                  dword ms;
                  dword alarm_time;
               };
               dword last_tick_time;
            };
            C_timer_imp *t = (C_timer_imp*)wParam;
            if(t->type==t->TYPE_ALARM){
               S_date_time dt;
               dt.GetCurrent();
               if(dt.sort_value>=t->alarm_time){
                  t->Pause();
                  in_event = true;
                  app->AlarmNotify(t, t->context);
                  in_event = false;
               }
            }else{
               dword tm = GetTickTime();
               dword d = tm - t->last_tick_time;
               t->last_tick_time = tm;
               in_event = true;
               app->TimerTick(t, t->context, d);
               in_event = false;
            }
         }
         break;

      case WM_LBUTTONDOWN:
#ifdef _WINDOWS
         mouse_down = true;
#endif
         if(!in_event){
            in_event = true;
            app->ProcessInputInternal(K_NOKEY, false, key_bits, S_point(short(lParam&0xffff), short(lParam>>16)), MOUSE_BUTTON_1_DOWN);
            in_event = false;
         }
         win::SetCapture(hwnd);
         break;

      case WM_LBUTTONUP:
         win::ReleaseCapture();
         if(!in_event){
            in_event = true;
            app->ProcessInputInternal(K_NOKEY, false, key_bits, S_point(short(lParam&0xffff), short(lParam>>16)), MOUSE_BUTTON_1_UP);
            in_event = false;
         }
#ifdef _WINDOWS
         mouse_down = false;
#endif
         break;

      case WM_MOUSEMOVE:
#ifdef _WINDOWS
         if(mouse_down)
#endif
         {
            if(!in_event){
               in_event = true;
               const S_point *sec = NULL;
               dword flags = MOUSE_BUTTON_1_DRAG;
               S_point pos(short(lParam&0xffff), short(lParam>>16));
#ifdef _WINDOWS
               S_point tmp;
               if(mouse_down_r){
                              //simulate multi-touch
                  tmp = pos;// - prev_sec_pos;
                  //prev_sec_pos = pos;
                  sec = &tmp;
                  pos = app->last_mouse_pos;
                  flags |= MOUSE_BUTTON_1_DRAG_MULTITOUCH;
               }
#endif
               app->ProcessInputInternal(K_NOKEY, false, key_bits, pos, flags, sec);
               in_event = false;
            }
         }
         break;

#ifdef _WINDOWS
      case WM_RBUTTONDOWN:
         mouse_down_r = true;
         //prev_sec_pos = S_point(short(lParam&0xffff), short(lParam>>16));
         break;
      case WM_RBUTTONUP: mouse_down_r = false; break;
#endif

#ifndef _WIN32_WCE
      case WM_MOUSEWHEEL:
         KeyEvent(int(wParam)>0 ? VK_UP : VK_DOWN, true, false, false);
         break;
#endif

      case WM_ACTIVATE:
         {
            bool act = (word(wParam&0xffff) != WA_INACTIVE);
#ifdef _WINDOWS
            if(!act)
               mouse_down = mouse_down_r = false;
#endif
#ifdef _WIN32_WCE
            allow_draw = act;
            if(!IsWMSmartphone()){
               SHHandleWMActivate(hwnd, wParam, lParam, &sip, 0);
               if(act){
                  win::SHFullScreen(hwnd, (app->ig_flags&IG_SCREEN_SHOW_SIP) ? SHFS_SHOWSIPBUTTON : SHFS_HIDESIPBUTTON);
                  if(!(app->ig_flags&IG_SCREEN_USE_CLIENT_RECT))
                     win::SHFullScreen(hwnd, SHFS_HIDETASKBAR);
               }
            }
#endif
            if(app->igraph)
               app->FocusChange(act);
         }
         break;

#ifdef _WINDOWS

      case WM_SETCURSOR:
         win::SetCursor(win::LoadCursor(NULL, (const wchar*)32512));    //IDC_ARROW
         return true;
#endif

      case WM_SYSCOMMAND:
#if defined _DEBUG && !defined _WIN32_WCE
         switch(wParam){
         case 1:              //save screenshot to clipboard
            if(win::OpenClipboard(NULL)){
               win::EmptyClipboard();

               dword sz = app->ScrnSX() * app->ScrnSY() * 3 + sizeof(win::BITMAPINFOHEADER);

               win::HGLOBAL hg = win::GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, sz);
               SaveBmp(win::GlobalLock(hg));
               win::GlobalUnlock(hg);
               win::SetClipboardData(CF_DIB, hg);
               win::CloseClipboard();
            }
            return 0;
         case 6:              //save screenshot to file
            {
               dword sz = app->ScrnSX() * app->ScrnSY() * 3 + sizeof(win::BITMAPINFOHEADER);
               byte *mem = new byte[sz];
               SaveBmp(mem);

               wchar fn[MAX_PATH];
               *fn = 0;
               win::OPENFILENAME ofn;
               MemSet(&ofn, 0, sizeof(ofn));
               ofn.lStructSize = sizeof(ofn);
               ofn.hwndOwner = hwnd;
               ofn.lpstrFile = fn;
               ofn.nMaxFile = MAX_PATH;
               ofn.lpstrTitle = L"Save BMP image to:";
               ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
               ofn.lpstrDefExt = L"bmp";
               if(win::GetOpenFileName(&ofn)){
                  C_file fl;
                  if(fl.Open(fn, fl.FILE_WRITE)){
                     struct{
                        dword size;
                        word res[2];
                        dword data_offs;
                     } hdr = { sz+14, {0,0}, 0x36 };
                     fl.WriteByte('B');
                     fl.WriteByte('M');
                     fl.Write(&hdr, 12);
                     fl.Write(mem, sz);
                  }
               }
               delete[] mem;
            }
            break;
         case 2:              //touchscreen toggle
            debug_setting.simulate_touchscreen = !debug_setting.simulate_touchscreen;
            app->has_mouse = debug_has_full_kb = debug_setting.simulate_touchscreen;
            CheckMenuItem(wParam, debug_setting.simulate_touchscreen);
            debug_setting.Save();
            app->_scrn_sx = 0;
            ResizeScreen(fullscreen_sz.x, fullscreen_sz.y);
            break;
         case 3:              //rotation toggle
            debug_setting.simulate_rotation = !debug_setting.simulate_rotation;
            CheckMenuItem(wParam, debug_setting.simulate_rotation);
            debug_setting.Save();
            app->_scrn_sx = 0; ResizeScreen(fullscreen_sz.x, fullscreen_sz.y);
            break;
         case 4:
         case 5:
         case 7:
                              //bit-depth toggle
            if(app->ig_flags&IG_SCREEN_ALLOW_32BIT){
               if(wParam==7){
                  debug_setting.bitdepth = (debug_setting.bitdepth==32) ? 16 : 32;
               }else
                  debug_setting.bitdepth = (wParam==4) ? 16 : 32;
               debug_setting.Save();
               CheckMenuItem(4, (debug_setting.bitdepth==16));
               CheckMenuItem(5, (debug_setting.bitdepth==32));
               const_cast<S_pixelformat&>(app->GetPixelFormat()).Init(debug_setting.bitdepth);
               app->_scrn_sx = 0; ResizeScreen(fullscreen_sz.x, fullscreen_sz.y);
            }
            break;
         case 8:              //toggle draw controls rects
            debug_draw_controls_rects = !debug_draw_controls_rects;
            CheckMenuItem(8, debug_draw_controls_rects);
            app->RedrawScreen();
            app->UpdateScreen();
            break;
         default:
            if(wParam>=10 && wParam<50){
                                 //memory limit
               int curr_l = debug_setting.memory_limit/(1024*1024);
               CheckMenuItem(10+curr_l, false);
               debug_setting.memory_limit = (wParam-10)*(1024*1024);
               CheckMenuItem(wParam, true);
               debug_setting.Save();

               void SetMemAllocLimit(dword bytes);
               SetMemAllocLimit(debug_setting.memory_limit);
            }else if(wParam>=100 && wParam<200){
               //resolution change
               int curr_i = FindCurrentResolution();
               if(curr_i!=-1)
                  CheckMenuItem(100+curr_i, false);
               dword num_res;
#ifdef NDEBUG
               if(!(app->ig_flags&IG_SCREEN_USE_CLIENT_RECT)){
                  //out of fullscreen
                  app->ig_flags |= IG_SCREEN_USE_CLIENT_RECT;
                  UpdateFlags(app->ig_flags, IG_SCREEN_USE_CLIENT_RECT, false);
               }
#endif
               fullscreen_sz = GetResolutions(num_res)[wParam-100];
               ResizeScreen(fullscreen_sz.x, fullscreen_sz.y);
               CheckMenuItem(wParam, true);
               debug_setting.res = fullscreen_sz;
               debug_setting.Save();
            }
         }
#endif
         break;

      case WM_SETFOCUS:
#ifdef USE_GX
         if((app->ig_flags&IG_SCREEN_USE_DSA) && !(app->ig_flags&IG_SCREEN_USE_CLIENT_RECT) && backbuffer)
            GXResume();
#endif
         return 0;

      case WM_KILLFOCUS:
#ifdef USE_GX
         if((app->ig_flags&IG_SCREEN_USE_DSA) && !(app->ig_flags&IG_SCREEN_USE_CLIENT_RECT) && backbuffer)
            GXSuspend();
#endif
         return 0;

      case WM_ERASEBKGND:
         return true;

      case WM_CLOSE:
         //if(!win_is_system_app)
         win::PostQuitMessage(0);
         return 0;

#ifdef _WIN32_WCE
      case WM_HOTKEY:
         KeyEvent(word(lParam>>16), !(lParam&MOD_KEYUP), false);
         break;
#endif
      case WM_PAINT:
         if(backbuffer && !(internal_flags&FLG_BBUF_DIRTY))
            UpdateGdi(NULL, 0);
         break;

      case WM_KEYDOWN:
         //if(wParam==VK_PROCESSKEY) wParam = win::ImmGetVirtualKey(hwnd);
         //Info("VK", wParam);
         KeyEvent(wParam, true, (lParam&0x40000000));
         return 0;

      case WM_KEYUP:
         //if(wParam==VK_PROCESSKEY) wParam = win::ImmGetVirtualKey(hwnd);
         KeyEvent(wParam, false, false);
         return 0;

      case WM_DESTROY:
#if !defined _WIN32_WCE && defined _DEBUG
         win::KillTimer(hwnd, 2);
#endif
         break;

#ifdef _WINDOWS
      case WM_SYSKEYDOWN:
      case WM_SYSKEYUP:
         if(wParam==VK_MENU)
            KeyEvent(wParam, (msg!=WM_SYSKEYUP), false);
#ifdef _DEBUG
         else if(wParam==VK_F10){
            KeyEvent(wParam, (msg!=WM_SYSKEYUP), false);
            return 0;
         }
#endif
         break;
      case WM_SYSCHAR:
         {
            IG_KEY k = K_NOKEY;
            switch(wParam){
            case '7': k = K_MEDIA_BACKWARD; break;
            case '8': k = K_MEDIA_PLAY_PAUSE; break;
            case '9': k = K_MEDIA_STOP; break;
            case '0': k = K_MEDIA_FORWARD; break;
            case ' ': break;
            default: k = (IG_KEY)ToUpper(char(wParam));
            }
            if(k)
               KeyEvent(k, true, (lParam&0x40000000), false);
         }
         break;
#endif
      }
      return win::DefWindowProc(hwnd, msg, wParam, lParam);
   }

//----------------------------

   static win::LRESULT CALLBACK WndProc_Thunk(win::HWND hwnd, win::UINT msg, win::WPARAM wParam, win::LPARAM lParam){

      if(msg==WM_CREATE){
         win::LPCREATESTRUCT cs = (win::LPCREATESTRUCT)lParam;
         win::SetWindowLong(hwnd, GWL_USERDATA, (win::LPARAM)cs->lpCreateParams);
      }
      IGraph_imp *_this = (IGraph_imp*)win::GetWindowLong(hwnd, GWL_USERDATA);
      if(!_this)
         return win::DefWindowProc(hwnd, msg, wParam, lParam);
      return _this->WndProc(hwnd, msg, wParam, lParam);
   }

//----------------------------

   static void RegisterClass(win::HINSTANCE hInstance){

      wchar clsname[MAX_PATH];
      GetClassName(clsname);

      win::WNDCLASS wc;
      wc.style = CS_HREDRAW | CS_VREDRAW;
      wc.lpfnWndProc = (win::WNDPROC)WndProc_Thunk;
      wc.cbClsExtra = 0;
      wc.cbWndExtra = 0;
      wc.hInstance = hInstance;
      wc.hIcon = win::LoadIcon(hInstance, (const wchar*)1);
      wc.hCursor = win::LoadCursor(hInstance, (const wchar*)32512);
      wc.hbrBackground = (win::HBRUSH)win::GetStockObject(WHITE_BRUSH);
      wc.lpszMenuName = 0;
      wc.lpszClassName = clsname;
      win::RegisterClass(&wc);
   }

//----------------------------

   IGraph_imp(C_application_base *_app, dword flg):
      app(_app),
      temp_no_resize(false),
      in_event(false),
      backbuffer(NULL),
#ifdef _WIN32_WCE
      framebuffer(NULL),
#endif
      hwnd(NULL),
      internal_flags(0),
      key_bits(0),
      allow_draw(true),
#ifdef _WINDOWS
      mouse_down(false),
      mouse_down_r(false),
#endif
      hdc_bb(NULL),
#ifdef _WIN32_WCE
      backlight_on(false),
      hwnd_menubar(NULL),
      h_backlight_event(NULL),
      last_time_reset_sleep_timer(0),
      save_gui_rotation(-1),
#endif
#ifdef USE_GX
      hi_gx(NULL),
      gx_inited(false),
#endif
      bmp_bb(NULL),
      hdc_save(NULL)
   {
      app->ig_flags = flg;
   }

//----------------------------

   void CreateBackbuffer(){

#ifdef USE_MFC
      cdc.CreateCompatibleDC(NULL);
      hdc_bb = cdc;
#else
      hdc_bb = win::CreateCompatibleDC(NULL);
#endif
                           //use 16-bit pixel format
      struct S_bm_info: public win::BITMAPINFOHEADER{
         dword masks[3];
      } bmii;
      const S_pixelformat &pf = app->GetPixelFormat();
      bmii.masks[0] = pf.r_mask;
      bmii.masks[1] = pf.g_mask;
      bmii.masks[2] = pf.b_mask;

      win::BITMAPINFOHEADER &bmi = bmii;
      MemSet(&bmi, 0, sizeof(bmi));
      bmi.biSize = sizeof(bmii);
      bmi.biWidth = app->ScrnSX();
#ifdef _DEBUG
      bmi.biWidth += 32;
      //bmi.biWidth = 0xa00;
#endif
      screen_pitch = bmi.biWidth*pf.bytes_per_pixel;
      bmi.biHeight = -app->ScrnSY();
      bmi.biPlanes = 1;
      bmi.biBitCount = word((pf.bits_per_pixel+7) & -8);
      //bmi.bmiHeader.biCompression = BI_RGB;
      bmi.biCompression = BI_BITFIELDS;
      bmp_bb = win::CreateDIBSection(hdc_bb, (win::BITMAPINFO*)&bmii, DIB_RGB_COLORS, (void**)&backbuffer, NULL, 0 );
      hdc_save = win::SelectObject(hdc_bb, bmp_bb);
      win::SetBkMode(hdc_bb, TRANSPARENT);
   }

//----------------------------

   void DeleteBackbuffer(){

      if(hdc_save && hdc_bb){
         win::SelectObject(hdc_bb, hdc_save);
         hdc_save = NULL;
      }
      if(bmp_bb){
         win::DeleteObject(bmp_bb);
         bmp_bb = NULL;
      }
      if(hdc_bb){
         win::DeleteDC(hdc_bb);
         hdc_bb = NULL;
      }
      backbuffer = NULL;
   }

//----------------------------

   static void SetRegistryValue(win::HKEY rkey, const wchar *reg_name, dword val){
      win::RegSetValueEx(rkey, reg_name, 0, REG_DWORD, (byte*)&val, sizeof(dword));
   }

//----------------------------

   static void SaveRegistryValue(win::HKEY rkey, const wchar *reg_name, dword &val){
      dword size = sizeof(dword);
      win::RegQueryValueEx(rkey, reg_name, NULL, NULL, (byte*)&val, &size);
   }

//----------------------------

#ifdef _WIN32_WCE
   /*
#ifdef _DEBUG
   static int GuiRotation(int rot){

      int ret = -1;
      win::HINSTANCE hi = win::LoadLibrary(L"coredll.dll");
      if(hi){
         typedef LONG t_ChangeDisplaySettingsEx(LPCTSTR lpszDeviceName, LPDEVMODE lpDevMode, HWND hwnd, DWORD dwflags, LPVOID lParam);
         t_ChangeDisplaySettingsEx *fp = (t_ChangeDisplaySettingsEx*)GetProcAddress(hi, L"ChangeDisplaySettingsEx");
         if(fp){
            DEVMODE dm;
            dm.dmSize = sizeof(dm);
            dm.dmFields = DM_DISPLAYQUERYORIENTATION;
            if(fp(NULL, &dm, NULL, CDS_TEST, NULL)==DISP_CHANGE_SUCCESSFUL){
               dm.dmFields = DM_DISPLAYORIENTATION;
               if(rot!=-1)
                  dm.dmDisplayOrientation = rot==0 ? DMDO_0 : rot==90 ? DMDO_90 : rot==180 ? DMDO_180 : DMDO_270;
               if(fp(NULL, &dm, NULL, rot==-1 ? CDS_TEST : CDS_RESET, NULL)==DISP_CHANGE_SUCCESSFUL){
                  switch(dm.dmDisplayOrientation){
                  case DMDO_0: ret = 0; break;
                  case DMDO_90: ret = 90; break;
                  case DMDO_180: ret = 180; break;
                  case DMDO_270: ret = 270; break;
                  }
               }
            }
         }
         win::FreeLibrary(hi);
      }
      return ret;
   }
#endif//_DEBUG
   */
   static int GetGuiRotation(){

#ifdef _WIN32_WCE
      if(IsWMSmartphone()){
         Cstr_c id;
         bool ReadDeviceIdentifier(Cstr_c &buf);
                              //hack for blackjack, which returns incorrect rotation
         if(ReadDeviceIdentifier(id) && id=="00PBFC9BF")
            return 0;
      }
#endif
      int ret = -1;
      win::HINSTANCE hi = win::LoadLibrary(L"coredll.dll");
      if(hi){
         typedef win::LONG t_ChangeDisplaySettingsEx(win::LPCTSTR lpszDeviceName, win::LPDEVMODE lpDevMode, win::HWND hwnd, win::DWORD dwflags, win::LPVOID lParam);
         t_ChangeDisplaySettingsEx *fp = (t_ChangeDisplaySettingsEx*)GetProcAddress(hi, L"ChangeDisplaySettingsEx");
         if(fp){
            win::DEVMODE dm;
            MemSet(&dm, 0, sizeof(dm));
            dm.dmSize = sizeof(dm);
            dm.dmFields = DM_DISPLAYORIENTATION;
            if(fp(NULL, &dm, NULL, CDS_TEST, NULL)==DISP_CHANGE_SUCCESSFUL){
               switch(dm.dmDisplayOrientation){
               case DMDO_0: ret = 0; break;
               case DMDO_90: ret = 90; break;
               case DMDO_180: ret = 180; break;
               case DMDO_270: ret = 270; break;
               }
            }
         }
         win::FreeLibrary(hi);
      }
      return ret;
   }

//----------------------------
   void SaveBacklightRegistry(){
      win::HKEY rkey;
                              //HKEY_CURRENT_USER
      if(win::RegOpenKeyEx(((win::HKEY)(win::ULONG_PTR)0x80000001), reg_name_backlight, 0, 0, &rkey) == ERROR_SUCCESS){
         SaveRegistryValue(rkey, reg_battery_timeout, save_reg_battery_timeout);
         SaveRegistryValue(rkey, reg_ACTimeout, save_reg_ACTimeout);
         win::RegCloseKey(rkey);
      }
                              //HKEY_CURRENT_USER
      if(win::RegOpenKeyEx(((win::HKEY)(win::ULONG_PTR)0x80000001), reg_name_power_timouts, 0, 0, &rkey) == ERROR_SUCCESS){
         SaveRegistryValue(rkey, reg_BattUserIdle, save_reg_BattUserIdle);
         SaveRegistryValue(rkey, reg_BattUserIdle, save_reg_BattUserIdle);
         SaveRegistryValue(rkey, reg_ACUserIdle, save_reg_ACUserIdle);
         SaveRegistryValue(rkey, reg_BattSystemIdle, save_reg_BattSystemIdle);
         SaveRegistryValue(rkey, reg_ACSystemIdle, save_reg_ACSystemIdle);
         SaveRegistryValue(rkey, reg_BattSuspend, save_reg_BattSuspend);
         SaveRegistryValue(rkey, reg_ACSuspend, save_reg_ACSuspend);
         win::RegCloseKey(rkey);
      }
   }

//----------------------------

#endif//_WIN32_WCE
//----------------------------
#if defined _DEBUG && !defined _WIN32_WCE
   struct S_debug_setting{
      S_point res;
      bool simulate_rotation;
      bool simulate_touchscreen;
      byte bitdepth;
      dword memory_limit;     //in bytes
      static const dword MEM_LIMIT_MIN = 4, MEM_LIMIT_MAX = 12;

      S_debug_setting():
         res(360, 640),
         simulate_rotation(false),
         simulate_touchscreen(true),
         bitdepth(16),
         memory_limit(1024*1024 * 10)
      {
      }
      static const wchar *SettingsName(){ return L":HKCU\\Software\\Lonely Cat Games\\IGraph\\Settings"; }
      void Load(){
         C_file fl;
         if(fl.Open(SettingsName())){
            fl.Read(this, sizeof(*this));
            if(memory_limit)
               memory_limit = Max(MEM_LIMIT_MIN*1024*1024, Min(MEM_LIMIT_MAX*1024*1024, memory_limit));
         }
      }
      void Save() const{
         C_file fl;
         if(fl.Open(SettingsName(), fl.FILE_WRITE)){
            fl.Write(this, sizeof(*this));
         }
      }
   } debug_setting;

   static void AddMenuItem(win::HMENU hm, const wchar *itm, dword id){
      win::MENUITEMINFO mi;
      mi.cbSize = sizeof(mi);
      mi.fMask = MIIM_ID | MIIM_TYPE;
      mi.fType = MFT_STRING;
      mi.wID = id;
      mi.dwTypeData = (wchar*)itm;
      mi.cch = StrLen(mi.dwTypeData);
      win::InsertMenuItem(hm, SC_CLOSE, false, &mi);
   }
   void CheckMenuItem(dword id, bool check){
      win::CheckMenuItem(win::GetSystemMenu(hwnd, false), id, MF_BYCOMMAND | (check?MF_CHECKED:MF_UNCHECKED));
   }

   static const S_point *GetResolutions(dword &num){
      static const S_point resolutions[] = {
         //S_point(176, 208), S_point(208, 176),
         S_point(240, 320), //S_point(320, 240),
         //S_point(352, 416), S_point(416, 352),
         S_point(320, 480), //S_point(480, 320),
         //S_point(800, 352),
         S_point(480, 640), //S_point(640, 480),
         S_point(360, 640), //S_point(640, 360),
         S_point(480, 800),
         S_point(640, 960),
         S_point(600, 1024),
         S_point(720, 1280),
         S_point(1080, 1920),
      };
      num = sizeof(resolutions)/sizeof(*resolutions);
      return resolutions;
   }
   int FindCurrentResolution() const{
      dword num_res;
      const S_point *res = GetResolutions(num_res);
      int i;
      for(i=num_res; i--; ){
         if(res[i]==fullscreen_sz)
            break;
      }
      return i;
   }

   void ModifySystemMenu(){
      win::HMENU hm = win::GetSystemMenu(hwnd, false);
      if(hm){
         AddMenuItem(hm, L"Copy screensho&t", 1);
         AddMenuItem(hm, L"Save screenshot [F9]", 6);
         AddMenuItem(hm, L"Touc&h screen [F5]", 2);
         CheckMenuItem(2, debug_setting.simulate_touchscreen);
         AddMenuItem(hm, L"Simulate r&otation", 3);
         CheckMenuItem(3, debug_setting.simulate_rotation);
         AddMenuItem(hm, L"Draw controls rects [F6]", 8);
         {
            win::HMENU hm1 = win::CreateMenu();
            win::MENUITEMINFO mi;
            mi.cbSize = sizeof(mi);
            mi.fMask = MIIM_TYPE | MIIM_SUBMENU;
            mi.hSubMenu = hm1;
            mi.fType = MFT_STRING;
            mi.dwTypeData = L"&Resolution";
            mi.cch = StrLen(mi.dwTypeData);
            win::InsertMenuItem(hm, SC_CLOSE, false, &mi);
            dword num_res;
            const S_point *res = GetResolutions(num_res);
            for(dword i=0; i<num_res; i++){
               Cstr_w s;
               s.Format(L"%x%") <<res[i].x <<res[i].y;
               dword id = 100+i;
               AddMenuItem(hm1, s, id);
               if(fullscreen_sz==res[i])
                  CheckMenuItem(id, true);
            }
         }
         if(app->ig_flags&IG_SCREEN_ALLOW_32BIT){
            win::HMENU hm1 = win::CreateMenu();
            win::MENUITEMINFO mi;
            mi.cbSize = sizeof(mi);
            mi.fMask = MIIM_TYPE | MIIM_SUBMENU;
            mi.hSubMenu = hm1;
            mi.fType = MFT_STRING;
            mi.dwTypeData = L"&Bit depth [F4]";
            mi.cch = StrLen(mi.dwTypeData);
            win::InsertMenuItem(hm, SC_CLOSE, false, &mi);
            AddMenuItem(hm1, L"16-bit", 4);
            AddMenuItem(hm1, L"32-bit", 5);
            CheckMenuItem(debug_setting.bitdepth==16 ? 4 : 5, true);
         }
         {
            win::HMENU hm1 = win::CreateMenu();
            win::MENUITEMINFO mi;
            mi.cbSize = sizeof(mi);
            mi.fMask = MIIM_TYPE | MIIM_SUBMENU;
            mi.hSubMenu = hm1;
            mi.fType = MFT_STRING;
            mi.dwTypeData = L"M&emory limit";
            mi.cch = StrLen(mi.dwTypeData);
            win::InsertMenuItem(hm, SC_CLOSE, false, &mi);

            AddMenuItem(hm1, L"No", 10);
            if(!debug_setting.memory_limit)
               CheckMenuItem(10, true);
            for(dword i=debug_setting.MEM_LIMIT_MIN; i<=debug_setting.MEM_LIMIT_MAX; i++){
               Cstr_w s;
               s.Format(L"% MB") <<i;
               AddMenuItem(hm1, s, 10+i);
               if(debug_setting.memory_limit==i*1024*1024)
                  CheckMenuItem(10+i, true);
            }
         }
      }
   }

//----------------------------
// mem is allocated to sx*sy*3 + sizeof(BITMAPINFOHEADER)
   void SaveBmp(void *mem){

      win::BITMAPINFOHEADER &bmi = *(win::BITMAPINFOHEADER*)mem;
      MemSet(&bmi, 0, sizeof(bmi));

      bmi.biSize = sizeof(win::BITMAPINFOHEADER);
      bmi.biWidth = app->ScrnSX();
      bmi.biHeight = app->ScrnSY();
      bmi.biPlanes = 1;
      bmi.biBitCount = 24;
      bmi.biCompression = BI_RGB;

                     //convert to dest format
      const byte *src = backbuffer + app->ScrnSY() * screen_pitch;
#pragma pack(push,1)
      struct S_rgb{
         byte b, g, r;
      } *dst = (S_rgb*)(&bmi+1);
#pragma pack(pop)
      for(int y=app->ScrnSY(); y--; ){
         //src -= app->ScrnSX()*app->GetPixelFormat().bytes_per_pixel;
         src -= screen_pitch;
         int x;
         switch(app->GetPixelFormat().bits_per_pixel){
#ifdef SUPPORT_12BIT_PIXEL
         case 12:
            for(x=0; x<app->ScrnSX(); x++){
               S_rgb &p = dst[x];
               dword s = ((word*)src)[x];
               p.r = byte((s&0x0f00) >> 4);
               p.g = byte(s&0x00f0);
               p.b = byte((s&0x000f) <<4);
            }
            break;
#endif
         case 16:
            for(x=0; x<app->ScrnSX(); x++){
               S_rgb &p = dst[x];
               dword s = ((word*)src)[x];
               p.r = byte((s&0xf800) >> 8);
               p.g = byte((s&0x07e0) >> 3);
               p.b = byte((s&0x01f) <<3);
            }
            break;
         case 32:
            for(x=0; x<app->ScrnSX(); x++){
               S_rgb &p = dst[x];
               dword s = ((dword*)src)[x];
               p.r = byte((s>>16)&0xff);
               p.g = byte((s>>8)&0xff);
               p.b = byte(s&0xff);
            }
            break;
         default:
            assert(0);
         }
         dst += app->ScrnSX();
      }
   }
#endif
//----------------------------
// Initialize screen - set window position, init GX, set screen size, pitches.
   void InitScreen(bool full){

      bool tnr = temp_no_resize;
      temp_no_resize = true;

#ifdef _WIN32_WCE
      fullscreen_sz.x = win::GetSystemMetrics(SM_CXSCREEN);
      fullscreen_sz.y = win::GetSystemMetrics(SM_CYSCREEN);

      if(full)
         win::SipShowIM(SIPF_OFF);
      if((app->ig_flags&IG_SCREEN_USE_DSA) && !(app->ig_flags&IG_SCREEN_USE_CLIENT_RECT)){
         //if(save_gui_rotation > 0)
            //GuiRotation(0);
         if(framebuffer){
            win::HDC hdc = win::GetDC(NULL);
            win::RawFrameBufferInfo rfbi;
            MemSet(&rfbi, 0, sizeof(rfbi));
            win::ExtEscape(hdc, GETRAWFRAMEBUFFER, 0, NULL, sizeof(rfbi), (char*)&rfbi);
            win::ReleaseDC(NULL, hdc);
            app->_scrn_sx = rfbi.cxPixels;
            app->_scrn_sy = rfbi.cyPixels;
            pitch_x = rfbi.cxStride;
            pitch_y = rfbi.cyStride;
            framebuffer = rfbi.pFramePointer;

            switch(save_gui_rotation){
            case 90:
#ifdef _DEBUG
               if(app->ScrnSX()>(int)fullscreen_sz.x || app->ScrnSY()>(int)fullscreen_sz.y)
#endif
               Swap(app->_scrn_sx, app->_scrn_sy);
               Swap(pitch_x, pitch_y);
               pitch_x = -pitch_x;
               (byte*&)framebuffer -= (app->ScrnSX()-1)*pitch_x;
               break;
            case 270:
#ifdef _DEBUG
               if(app->ScrnSX()>(int)fullscreen_sz.x || app->ScrnSY()>(int)fullscreen_sz.y)
#endif
               Swap(app->_scrn_sx, app->_scrn_sy);
               Swap(pitch_x, pitch_y);
               pitch_y = -pitch_y;
               (byte*&)framebuffer -= (app->ScrnSY()-1)*pitch_y;
               break;
            }
         }else{
#ifdef USE_GX
                              //use GX if framebuffer method fails
            GXDisplayProperties dp;
            if(GXGetDisplayProperties(dp)){
               app->_scrn_sx = dp.cxWidth;
               app->_scrn_sy = dp.cyHeight;
               pitch_x = dp.cbxPitch;
               pitch_y = dp.cbyPitch;
               if(dp.ffFormat&8){   //kfLandscape
                  internal_flags |= FLG_HW_LANSCAPE;
                  //if(app->ig_flags&IG_SCREEN_NATIVE_ROTATION)
                  Swap(app->_scrn_sx, app->_scrn_sy);
               }
            }else
#endif
               Fatal("no gx");
         }
#ifdef _DEBUG
         if(app->ig_flags&IG_DEBUG_LANDSCAPE){
            internal_flags |= FLG_HW_LANSCAPE;
            //app->ig_flags |= IG_SCREEN_NATIVE_ROTATION;
            Swap(app->_scrn_sx, app->_scrn_sy);
            Swap(pitch_x, pitch_y);
            pitch_y = -pitch_y;
         }
#endif
      }else{
         app->_scrn_sx = fullscreen_sz.x;
         app->_scrn_sy = fullscreen_sz.y;
                              //pitch not needed
         pitch_x = pitch_y = 0;
      }
      dword wx = 0, wy = 0;

      if(app->ig_flags&(IG_SCREEN_USE_CLIENT_RECT | IG_SCREEN_SHOW_SIP)){
         win::RECT rc;
         win::SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
         if(app->ig_flags&IG_SCREEN_USE_CLIENT_RECT){
            wy = rc.top;
            app->_scrn_sy -= wy;
         }
                              //todo: properly get height of bottom bar !!!
         if(app->ig_flags&IG_SCREEN_SHOW_SIP)
            app->_scrn_sy -= rc.top;
      }
      if(!hwnd)
         CreateAppWindow(0, wy);
      dword wsx = app->ScrnSX(), wsy = app->ScrnSY();
      if(!(app->ig_flags&IG_SCREEN_USE_CLIENT_RECT))
         win::SHFullScreen(hwnd, SHFS_HIDETASKBAR);
      if(full && !IsWMSmartphone()){
         win::SHFullScreen(hwnd, (app->ig_flags&IG_SCREEN_SHOW_SIP) ? SHFS_SHOWSIPBUTTON : SHFS_HIDESIPBUTTON);
      }
#ifdef USE_GX
      if((app->ig_flags&IG_SCREEN_USE_DSA) && !(app->ig_flags&IG_SCREEN_USE_CLIENT_RECT) && !save_gui_rotation && !framebuffer){
         GXOpenDisplay((struct HWND__*)hwnd, GX_FULLSCREEN);
         GXSetViewport(0, app->ScrnSY());
      }
#endif//!USE_GX

#else//_WIN32_WCE

#ifdef NDEBUG
      if(app->ig_flags&IG_SCREEN_USE_CLIENT_RECT){
#ifdef _DEBUG
         app->_scrn_sx = debug_setting.res.x & -4;
         app->_scrn_sy = debug_setting.res.y;
#else
         app->_scrn_sx = 640;
         app->_scrn_sy = 480;
#endif
      }else{
         app->_scrn_sx = win::GetSystemMetrics(SM_CXSCREEN);
         app->_scrn_sy = win::GetSystemMetrics(SM_CYSCREEN);
      }
#else
      app->_scrn_sx = debug_setting.res.x & -4;
      app->_scrn_sy = debug_setting.res.y;
#endif
      fullscreen_sz.x = app->ScrnSX();
      fullscreen_sz.y = app->ScrnSY();

      if(app->ig_flags&IG_DEBUG_LANDSCAPE){
         internal_flags |= FLG_HW_LANSCAPE;
                              //must use also native mode, because we use BitBlt and have no rotation in debug mode
         //app->ig_flags |= IG_SCREEN_NATIVE_ROTATION;
      }

      pitch_x = app->GetPixelFormat().bytes_per_pixel;
      pitch_y = app->ScrnSX()*pitch_x;

      dword wx = 20, wy = 20;
#ifdef NDEBUG
      if(!(app->ig_flags&IG_SCREEN_USE_CLIENT_RECT)){
         wx = wy = 0;
      }
#endif
      if(!hwnd)
         CreateAppWindow(wx, wy);

      win::RECT rc = {0, 0, app->ScrnSX(), app->ScrnSY() };
      dword wstyle = GetWindowLong(hwnd, GWL_STYLE);
      wstyle &= ~(WS_BORDER | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX | WS_POPUP);
      wstyle |= GetWStyle();
      SetWindowLong(hwnd, GWL_STYLE, wstyle);
      AdjustWindowRect(&rc, wstyle, false);
      internal_flags |= FLG_BBUF_DIRTY;
      dword wsx = rc.right - rc.left, wsy = rc.bottom - rc.top;
#endif//!_WIN32_WCE
      if(app->ig_flags&IG_DEBUG_LANDSCAPE)
         Swap(wsx, wsy);
      win::SetWindowPos(hwnd, NULL, wx, wy, wsx, wsy, SWP_NOOWNERZORDER|SWP_NOZORDER);
      //*
#ifdef _WIN32_WCE
      {
         win::RECT rc;
         win::GetClientRect(hwnd, &rc);
         app->_scrn_sx = rc.right - rc.left;
         app->_scrn_sy = rc.bottom - rc.top;
      }
#endif
      /**/
      CreateBackbuffer();
      temp_no_resize = tnr;
   }

//----------------------------

   void CloseScreen(bool full){

      DeleteBackbuffer();
#ifdef _WIN32_WCE
      if(full && !(app->ig_flags&IG_SCREEN_USE_CLIENT_RECT))
         win::SHFullScreen(hwnd, SHFS_SHOWTASKBAR);
      //if(full && !IsWMSmartphone() && !(app->ig_flags&IG_SCREEN_SHOW_SIP))
         //win::SHFullScreen(hwnd, SHFS_SHOWSIPBUTTON);
#endif
#ifdef USE_GX
      if((app->ig_flags&IG_SCREEN_USE_DSA) && !(app->ig_flags&IG_SCREEN_USE_CLIENT_RECT) && !save_gui_rotation && !framebuffer){
         GXCloseDisplay();
      }
#endif
   }

//----------------------------

   static bool IsHighResAware(){

      win::HINSTANCE hi = win::GetModuleHandle(NULL);
      win::HRSRC rsrc = win::FindResource(hi, L"HI_RES_AWARE", L"CEUX");
      if(rsrc && win::SizeofResource(hi, rsrc)==2){
         win::HGLOBAL hgl = win::LoadResource(hi, rsrc);
         if(hgl){
            const word *mem = (word*)win::LockResource(hgl);
            return (*mem==1);
         }
      }
      return false;
   }

//----------------------------

   static const wchar *GetWindowTitle(wchar tmp[256]){

      win::GetModuleFileName(win::GetModuleHandle(NULL), tmp, 256);
      int i;
      for(i=wcslen(tmp); i--; ){
         if(tmp[i]=='.')
            tmp[i] = 0;
         if(tmp[i]=='\\')
            break;
      }
      return tmp + i + 1;
   }

//----------------------------

   dword GetWStyle(){
#ifdef _WIN32_WCE
      return 0;
#else
#ifdef NDEBUG
      if(app->ig_flags&IG_SCREEN_USE_CLIENT_RECT){
         return WS_BORDER | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX;
      }else{
         return WS_POPUP | WS_SYSMENU;
      }
#else
      return WS_BORDER | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX;
#endif
#endif
   }

//----------------------------

   bool CreateAppWindow(dword wx, dword wy){

      wchar tmp_buf[256];
      const wchar *title = GetWindowTitle(tmp_buf);
      wchar clsname[MAX_PATH];
      GetClassName(clsname);

      dword wstyle = GetWStyle();
      dword wsx, wsy;
#ifdef _WIN32_WCE
      wsx = app->ScrnSX();
      wsy = app->ScrnSY();
      wstyle |= WS_VISIBLE;
#else
      win::RECT rc = {0, 0, app->ScrnSX(), app->ScrnSY() };
      AdjustWindowRect(&rc, wstyle, false);
      wsx = rc.right - rc.left;
      wsy = rc.bottom - rc.top;
#endif
      hwnd = win::CreateWindowEx(0, clsname, title, wstyle, wx, wy, wsx, wsy, NULL, NULL, win::GetModuleHandle(NULL), this);
      if(!hwnd)
         return false;
#if defined _DEBUG && !defined _WIN32_WCE
      ModifySystemMenu();
#endif
      return true;
   }

//----------------------------

   bool Construct(){

      app->InitInputCaps();
      win::HINSTANCE hi = win::GetModuleHandle(NULL);
      RegisterClass(hi);

                              //determine bytes per pixel
      byte bpp = 16;
#ifdef _WIN32_WCE
      save_gui_rotation = GetGuiRotation();
      {
         win::HDC hdc = win::GetDC(NULL);
         win::RawFrameBufferInfo rfbi;
         MemSet(&rfbi, 0, sizeof(rfbi));
         if(IsHighResAware() && ExtEscape(hdc, GETRAWFRAMEBUFFER, 0, NULL, sizeof(rfbi), (char*)&rfbi)){
            if(rfbi.cxPixels && rfbi.cyPixels && (rfbi.wBPP==16 || rfbi.wBPP==32)){
               if((app->ig_flags&IG_SCREEN_ALLOW_32BIT) || rfbi.wBPP!=32){
                  framebuffer = rfbi.pFramePointer;
                  bpp = (byte)rfbi.wBPP;
               }
            }
         }
         if(!framebuffer){
#ifdef USE_GX
            GXDisplayProperties dp;
            if(GXGetDisplayProperties(dp)){
               if(dp.cBPP==12 || dp.cBPP==16 || dp.cBPP==32)
                  bpp = (byte)dp.cBPP;
            }else
#endif
               bpp = 16;
         }
         win::ReleaseDC(NULL, hdc);
      }
      SaveBacklightRegistry();
#ifdef _DEBUG
      if(app->ig_flags&IG_DEBUG_12_BIT) bpp = 12;
      if(app->ig_flags&IG_DEBUG_15_BIT) bpp = 15;
      if((app->ig_flags&IG_DEBUG_32_BIT) && bpp!=32){
         bpp = 32;
         app->ig_flags &= ~IG_SCREEN_USE_DSA;
      }
#endif
#else//_WIN32_WCE

#ifdef _DEBUG
      debug_setting.Load();
      app->has_mouse = debug_has_full_kb = debug_setting.simulate_touchscreen;

      void SetMemAllocLimit(dword bytes);
      SetMemAllocLimit(debug_setting.memory_limit);
      if(app->ig_flags&IG_SCREEN_ALLOW_32BIT)
         bpp = debug_setting.bitdepth;
#else//_DEBUG
      app->has_mouse = true;

      {
         win::HDC hdc = win::GetDC(NULL);
         bpp = win::GetDeviceCaps(hdc, BITSPIXEL);
         win::ReleaseDC(NULL, hdc);
	      if(bpp>16)
		      bpp = 32;   // 24 bits not supported
      }
#endif

#endif//!_WIN32_WCE
      const_cast<S_pixelformat&>(app->GetPixelFormat()).Init(bpp);

      SetBackLight((app->ig_flags&IG_ENABLE_BACKLIGHT));

      temp_no_resize = true;
      InitScreen(true);

      win::ShowWindow(hwnd, SW_SHOW);
      //win::UpdateWindow(hwnd);
                              //process messages im queue, so that our window is correctly initialized
      ProcessWinMessages();
      temp_no_resize = false;

      //start_timer = win::GetTickCount();
      return true;
   }

//----------------------------

   ~IGraph_imp(){

      CloseScreen(true);
      DestroyWindow(hwnd);
      hwnd = NULL;

      SetBackLight(false);
#ifdef USE_GX
      if(hi_gx)
         win::FreeLibrary(hi_gx);
#endif
      /*
#ifdef _WIN32_WCE
      if(save_gui_rotation!=-1)
         GuiRotation(save_gui_rotation);
#endif
      */
   }

//----------------------------

   void UpdateGdi(const S_rect *rects, dword num_rects) const{

      win::HDC hdc = win::GetDC(hwnd);
      if(hdc){
         if(num_rects){
                           //only dirty rectangles
            while(num_rects--){
               const S_rect &rc = *rects++;
               win::BitBlt(hdc, rc.x, rc.y, rc.sx, rc.sy, hdc_bb, rc.x, rc.y, 0x00CC0020);   //SRCCOPY
            }
         }else{
                           //entire screen
            win::BitBlt(hdc, 0, 0, app->ScrnSX(), app->ScrnSY(), hdc_bb, 0, 0, 0x00CC0020);  //SRCCOPY
         }
         win::ReleaseDC(hwnd, hdc);
      }
   }

//----------------------------

   virtual void _UpdateScreen(const S_rect *rects = NULL, dword num_rects = 0) const{

      if(!allow_draw)
         return;
      internal_flags &= ~FLG_BBUF_DIRTY;
#ifdef USE_GX
      if((app->ig_flags&IG_SCREEN_USE_DSA) && !(app->ig_flags&IG_SCREEN_USE_CLIENT_RECT)){
         byte *dst;
         dst = (byte*)framebuffer;
         if(!dst)
            dst = (byte*)GXBeginDraw();
         if(!dst){
            UpdateGdi(NULL, 0);
         }else{
            const byte *src = backbuffer;
            //dword screen_pitch = app->ScrnSX() * app->GetPixelFormat().bytes_per_pixel;

            //if((internal_flags&FLG_HW_LANSCAPE) && !(internal_flags&FLG_NATIVE_ROTATON))
            //if(internal_flags&FLG_HW_LANSCAPE)
            if(pitch_x!=app->GetPixelFormat().bytes_per_pixel){
                                 //rotate backbuffer to screen
               if(app->GetPixelFormat().bytes_per_pixel==2){
#if 0
                              //optimized version - write 2 pixels at once (slower than below!)
                  int src_pitch_x, src_pitch_y, pitch_y1;
                  switch(save_gui_rotation){
                  case 90:
                     src_pitch_x = screen_pitch;
                     src_pitch_y = -2;
                     src += (app->ScrnSX()-1)*2;
                     dst += (app->ScrnSX()-1)*pitch_x;
                     pitch_y1 = -pitch_x;
                     break;
                  }
                  for(int x=0; x<app->ScrnSX(); x++){
                     dword *dst_line = (dword*)dst, *dst_end = dst_line+app->ScrnSY()/2;
                     word *src_line = (word*)src;
                     do{
                        dword p = *src_line;
                        (byte*&)src_line += src_pitch_x;
                        p |= (*src_line<<16);
                        (byte*&)src_line += src_pitch_x;
                        *dst_line++ = p;
                     } while(dst_line!=dst_end);
                     dst += pitch_y1;
                     src += src_pitch_y;
                  }
#else
                              //optimized version - read 2 pixels at once
                  for(int y=0; y<app->ScrnSY(); y++){
                     word *dst_line = (word*)dst;
                     dword *src_line = (dword*)src, *line_end = src_line+app->ScrnSX()/2;
                     do{
                        dword p = *src_line;
                        *dst_line = word(p);
                        (byte*&)dst_line += pitch_x;
                        *dst_line = word(p>>16);
                        (byte*&)dst_line += pitch_x;
                     } while(++src_line!=line_end);
                     dst += pitch_y;
                     src += screen_pitch;
                  }
#endif
               }else{
                  for(int y=0; y<app->ScrnSY(); y++){
                     dword *dst_line = (dword*)dst;
                     dword *src_line = (dword*)src, *line_end = src_line+app->ScrnSX();
                     do{
                        *dst_line = *src_line;
                        (byte*&)dst_line += pitch_x;
                     } while(++src_line!=line_end);
                     dst += pitch_y;
                     src += screen_pitch;
                  }
               }
            }else{
                                 //native - line by line transfer
               dword dst_pitch = pitch_y;
               if(internal_flags&FLG_HW_LANSCAPE){
#ifdef _DEBUG
                  if(!(app->ig_flags&IG_DEBUG_LANDSCAPE))
#endif
                  {
                     dst -= (app->ScrnSX()-1)*app->GetPixelFormat().bytes_per_pixel;
                     dst_pitch = pitch_x;
                  }
               }
               if(screen_pitch==dst_pitch){
                  memcpy(dst, src, screen_pitch*app->ScrnSY());
               }else{
                  for(int y=app->ScrnSY(); y--; ){
                     memcpy(dst, src, screen_pitch);
                     dst += dst_pitch;
                     src += screen_pitch;
                  }
               }
            }
            if(!framebuffer)
               GXEndDraw();
         }
      }else
#endif //USE_GX
      {
#if 1
         if(!rects)
            win::InvalidateRect(hwnd, NULL, false);
         else{
            while(num_rects--){
               S_rect rc = *rects++;
               rc.sx += rc.x;
               rc.sy += rc.y;
               win::InvalidateRect(hwnd, (win::RECT*)&rc, false);
            }
         }
         win::UpdateWindow(hwnd);
#else
         UpdateGdi(rects, num_rects);
#endif
      }
#ifdef _WIN32_WCE
                              //make backlight and power stay on
      if(backlight_on){
                              //reset sleep timer occasionally
         dword t = GetTickTime();
         if(t-last_time_reset_sleep_timer >= 4900){
            last_time_reset_sleep_timer = t;
            ResetSleepTimer();
         }
      }
#endif
   }

//----------------------------

   static const dword CHANGE_FLAGS = IG_SCREEN_SHOW_SIP | IG_SCREEN_USE_CLIENT_RECT | IG_SCREEN_USE_DSA;

//----------------------------

   void UpdateFlags(dword new_init_flags, dword changed, bool resize_notify){

      const int sx = app->ScrnSX(), sy = app->ScrnSY();
      if(changed&CHANGE_FLAGS){
         temp_no_resize = true;
         CloseScreen(false);
      }
      app->ig_flags = new_init_flags;
#if defined _WIN32_WCE
      if(!IsWMSmartphone()){
         if(changed&(IG_SCREEN_USE_CLIENT_RECT|IG_SCREEN_SHOW_SIP)){
            win::RECT rc;
            win::GetWindowRect(hwnd_menubar, &rc);
            win::SetWindowPos(hwnd_menubar, NULL, 0, fullscreen_sz.y-(rc.bottom-rc.top), 0, 0, SWP_NOSIZE);
            win::CommandBar_Show(hwnd_menubar, bool(app->ig_flags&IG_SCREEN_SHOW_SIP));
            system::Sleep(50);   //without it, SIP button doesn't disappear!
            win::SHFullScreen(hwnd, (app->ig_flags&IG_SCREEN_SHOW_SIP) ? SHFS_SHOWSIPBUTTON : SHFS_HIDESIPBUTTON);
         }
      }
#endif
      if(changed&CHANGE_FLAGS){
         InitScreen(false);
         if(resize_notify){
            app->InitInputCaps();
            app->ScreenReinit((sx!=app->ScrnSX() || sy!=app->ScrnSY()));
         }
         temp_no_resize = false;
      }
      if(changed&IG_ENABLE_BACKLIGHT)
         SetBackLight((app->ig_flags&IG_ENABLE_BACKLIGHT));
   }

//----------------------------

   virtual void _UpdateFlags(dword new_flags, dword flags_mask){

      if(temp_no_resize)
         return;
#ifdef _WIN32_WCE
      if(IsWMSmartphone())
#endif
         flags_mask &= ~IG_SCREEN_SHOW_SIP;

      flags_mask &= CHANGE_FLAGS|IG_ENABLE_BACKLIGHT;
      new_flags &= flags_mask;
      const dword curr = app->ig_flags&flags_mask;
      win::KillTimer(hwnd, 2);
      if(new_flags!=curr){
         const dword changed = curr ^ new_flags;
         dword new_init_flags = (app->ig_flags & ~flags_mask) | new_flags;
         if(changed&(IG_SCREEN_SHOW_SIP|IG_SCREEN_USE_CLIENT_RECT)){
                              //async, do this with delay
            //PostMessage(hwnd, WM_UPDATE_FLAGS, new_init_flags, changed);
            timed_changed_flags = changed;
            timed_new_init_flags = new_init_flags;
            win::SetTimer(hwnd, 2, 250, NULL);
            //app->ig_flags = new_init_flags;
         }else
            UpdateFlags(new_init_flags, changed, false);
      }
   }

//----------------------------

   void ProcessWinMessages(){

      win::MSG msg;
      if(win::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){
         do{
            //if(msg.message==WM_QUIT) exit(1);
            win::TranslateMessage(&msg);
            win::DispatchMessage(&msg);
         }while(win::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE));
      }
   }

//----------------------------

   static int SingleToWide(const char *txt, wchar out[1024]){

      int len = strlen(txt);
      len = Min(len, 1024);
      for(int i=0; i<len; i++)
         out[i] = txt[i];
      return len;
   }

//----------------------------

   virtual void *_GetBackBuffer() const{ return backbuffer; }

//----------------------------

   void SetBackLight(bool on){

      if(backlight_on!=on){
         backlight_on = on;
#ifdef _WIN32_WCE
         win::HKEY rkey;
                              //HKEY_CURRENT_USER
         if(win::RegOpenKeyEx(((win::HKEY)(win::ULONG_PTR)0x80000001), reg_name_backlight, 0, 0, &rkey) == ERROR_SUCCESS){
            if(on){
               const dword value = IsWMSmartphone() ? 7199999 : 0x7fffffff;
               SetRegistryValue(rkey, reg_battery_timeout, value);
               SetRegistryValue(rkey, reg_ACTimeout, value);
            }else{
               SetRegistryValue(rkey, reg_battery_timeout, save_reg_battery_timeout);
               SetRegistryValue(rkey, reg_ACTimeout, save_reg_ACTimeout);
            }
            RegCloseKey(rkey);
         }
         win::HANDLE h = win::CreateEvent(NULL, FALSE, FALSE, L"BackLightChangeEvent");
         if(h){
            win::SetEvent(h);
            win::CloseHandle(h);
         }
                              //HKEY_CURRENT_USER
         if(win::RegOpenKeyEx(((win::HKEY)(win::ULONG_PTR)0x80000001), reg_name_power_timouts, 0, 0, &rkey) == ERROR_SUCCESS){
            dword v0 = on ? on : save_reg_BattUserIdle,
               v1 = on ? on : save_reg_ACUserIdle,
               v2 = on ? on : save_reg_BattSystemIdle,
               v3 = on ? on : save_reg_ACSystemIdle,
               v4 = on ? on : save_reg_BattSuspend,
               v5 = on ? on : save_reg_ACSuspend;
            SetRegistryValue(rkey, reg_BattUserIdle, v0);
            SetRegistryValue(rkey, reg_ACUserIdle, v1);
            SetRegistryValue(rkey, reg_BattSystemIdle, v2);
            SetRegistryValue(rkey, reg_ACSystemIdle, v3);
            SetRegistryValue(rkey, reg_BattSuspend, v4);
            SetRegistryValue(rkey, reg_ACSuspend, v5);
            RegCloseKey(rkey);

            win::HANDLE h = win::CreateEvent(NULL, FALSE, FALSE, L"PowerManager/ReloadActivityTimeouts");
            if(h){
               win::SetEvent(h);
               win::CloseHandle(h);
            }
         }
         if(on){
            assert(!h_backlight_event);
            h_backlight_event = win::CreateEvent(NULL, FALSE, FALSE, L"TIMEOUTDISPLAYOFF");

            last_time_reset_sleep_timer = GetTickTime();
            ResetSleepTimer();
         }else{
            win::CloseHandle(h_backlight_event);
            h_backlight_event = NULL;
         }
#endif
      }
   }

//----------------------------

#ifdef _WIN32_WCE
   void ResetSleepTimer() const{
      assert(backlight_on);
      if(h_backlight_event)
         win::SetEvent(h_backlight_event);
                              //simulate key event
      win::keybd_event(VK_F24, 0, KEYEVENTF_KEYUP | KEYEVENTF_SILENT, 0);
      win::INPUT input;
      input.type = INPUT_MOUSE;
      //input.ki.dwFlags = KEYEVENTF_EXTENDEDKEY; input.ki.wVk = 0; input.ki.dwExtraInfo = 0; input.ki.wScan = VK_F1;
      input.mi.dx = 0; input.mi.dy = 0; input.mi.mouseData = 0; input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN; input.mi.time = 0; input.mi.dwExtraInfo = 0;
      win::SendInput(1, &input, sizeof(win::INPUT));
      //input.ki.dwFlags |= KEYEVENTF_KEYUP;
      input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
      win::SendInput(1, &input, sizeof(win::INPUT));
   }
#endif

//----------------------------

   virtual void *GetHWND() const{ return hwnd; }
   virtual void *GetHDC() const{ return hdc_bb; }

   virtual dword GetPhysicalRotation() const{

#ifdef _WIN32_WCE
      switch(GetGuiRotation()){
      case 0: return 0; break;   //ROTATION_NONE
      case 90: return 1; break;  //ROTATION_90CCW
      case 180: return 2; break; //ROTATION_180
      case 270: return 3; break; //ROTATION_90CW
      }
#endif
      return 0;
   }

//----------------------------

#if defined _DEBUG && !defined _WIN32_WCE
   virtual dword _GetRotation() const{
      if(app->ScrnSX()==800 && app->ScrnSY()==352)
         return 3;   //ROTATION_90CW
      return !debug_setting.simulate_rotation ? 0 : 3;
   }
#endif

//----------------------------

   virtual S_point _GetFullScreenSize() const{ return fullscreen_sz; }

//----------------------------
};

//----------------------------

C_application_base::E_LCD_SUBPIXEL_MODE C_application_base::DetermineSubpixelMode() const{

#ifdef _WIN32_WCE
   return !igraph->GetPhysicalRotation() ? LCD_SUBPIXEL_RGB : LCD_SUBPIXEL_UNKNOWN;
#else
                              //windows - simulation for LCD screen
   return LCD_SUBPIXEL_RGB;
#endif
}

//----------------------------

IGraph *IGraphCreate(C_application_base *app, dword flags){

#ifdef _WIN32_WCE
   if(IsWMSmartphone())       //no SIP in Smartphone
      flags &= ~IG_SCREEN_SHOW_SIP;
#else
#ifdef _DEBUG
   if(flags&(IG_DEBUG_NO_KB|IG_DEBUG_NO_FULL_KB)){
      debug_has_kb = !(flags&IG_DEBUG_NO_KB);
      debug_has_full_kb = debug_has_kb && !(flags&IG_DEBUG_NO_FULL_KB);
   }
#endif
#endif
   IGraph_imp *ig = new IGraph_imp(app, flags);
   if(!ig->Construct()){
      delete ig;
      ig = NULL;
   }
   return ig;
}

//----------------------------
//----------------------------

C_application_base::S_system_font::S_system_font():
   use(false),
   last_used_font(NULL),
   last_text_color(0),
   font_width(NULL),
   hdc(NULL)
{
   MemSet(font_handle, 0, sizeof(font_handle));
}

//----------------------------

void C_application_base::S_system_font::InitHdc() const{

   hdc = igraph->GetHDC();
   win::SetTextColor((win::HDC)hdc, ((last_text_color&0xff)<<16) | (last_text_color&0xff00) | ((last_text_color>>16)&0xff));
   win::SetTextAlign((win::HDC)hdc, TA_BASELINE | TA_LEFT);
}

//----------------------------

void C_application_base::S_system_font::SelectFont(void *h) const{

   bool ok = win::SelectObject((win::HDC)hdc, last_used_font = h);
   if(!ok){
      InitHdc();
      win::SelectObject((win::HDC)hdc, h);
   }
}

//----------------------------

void C_application_base::InitSystemFont(int size_delta, bool use_antialias, int small_font_size_percent){

#ifndef USE_SYSTEM_FONT
   internal_font.Close();
#endif
   int size = Max(0, Min(NUM_SYSTEM_FONTS, GetDefaultFontSize(true)+size_delta));

   system_font.Close();
   system_font.igraph = igraph;

   system_font.use = true;

   system_font.InitHdc();
   win::HDC hdc = (win::HDC)system_font.hdc;

   system_font.font_width = new(true) byte[2*2*system_font.NUM_CACHE_WIDTH_CHARS];

   font_defs = system_font.font_defs;
   system_font.size_big = 12+size*20/8;
   system_font.size_small = system_font.size_big*small_font_size_percent/100;
   system_font.use_antialias = use_antialias;

                              //create fonts
   for(int fi=0; fi<NUM_FONTS; fi++){
      //for(int i=0; i<2; i++){
         for(int b=0; b<2; b++){
            system_font.InitFont(0, b, 0, fi!=0, !fi ? system_font.size_small : system_font.size_big);
         }
      //}
      system_font.SelectFont(system_font.font_handle[0][0][0][fi]);
      win::TEXTMETRIC tm, tm_b;
      win::GetTextMetrics((win::HDC)hdc, &tm);
      system_font.SelectFont(system_font.font_handle[0][1][0][fi]);
      win::GetTextMetrics((win::HDC)hdc, &tm_b);

      const byte *fw = system_font.font_width + fi*system_font.NUM_CACHE_WIDTH_CHARS;

      S_font &fd = system_font.font_defs[fi];
      fd.cell_size_x = tm.tmAveCharWidth*12/8;
      fd.cell_size_y = tm.tmHeight-tm.tmInternalLeading;
      fd.space_width = fw[0];
      fd.letter_size_x = tm.tmAveCharWidth;
      fd.line_spacing = fd.cell_size_y*10/8;
      fd.extra_space = 0;
      fd.bold_width_add = tm_b.tmAveCharWidth - tm.tmAveCharWidth;
      fd.bold_width_subpix = fd.bold_width_add*3;
      fd.bold_width_pix = fd.bold_width_add;
   }
   fds = font_defs[UI_FONT_SMALL];
   fdb = font_defs[UI_FONT_BIG];
   SetClipRect(GetClipRect());
}

//----------------------------

void C_application_base::S_system_font::SetClipRect(const S_rect &rc){
   clip_rect = rc;
   clip_rect.sx += clip_rect.x;
   clip_rect.sy += clip_rect.y;
}

//----------------------------

dword C_application_base::S_system_font::GetCharWidth(wchar _c, bool bold, bool italic, bool big) const{

   word c = word(_c);
   if(c>=' ' && c<(' '+NUM_CACHE_WIDTH_CHARS)){
      const byte *fw = font_width + int(bold)*NUM_CACHE_WIDTH_CHARS*2 + int(big)*NUM_CACHE_WIDTH_CHARS;
      return fw[c-' '];
   }
   italic = false;
   void *h = font_handle[0][bold][italic][big];
   assert(h);
   if(last_used_font!=h)
      SelectFont(h);

   /*
   win::ABC abc;
   if(!win::GetCharABCWidths((win::HDC)hdc, c, c, &abc)){
      InitHdc();
      win::GetCharABCWidths((win::HDC)hdc, c, c, &abc);
   }
   int w = abc.abcB + abc.abcC;
   */
   int w;
   bool ok = win::GetCharWidth32((win::HDC)hdc, c, c, &w);
   if(!ok){
      InitHdc();
      win::GetCharWidth32((win::HDC)hdc, c, c, &w);
   }
   return w-1;
}

//----------------------------

void C_application_base::S_system_font::Close(){

   for(int i=0; i<4*2*2*2; i++){
      void *&h = font_handle[i>>3][(i>>2)&1][(i>>1)&1][i&1];
      if(h){
         win::DeleteObject(h);
         h = NULL;
      }
   }
   hdc = NULL;
   last_used_font = NULL;
   use = false;
   delete[] font_width; font_width = NULL;
}

//----------------------------

void C_application_base::S_system_font::InitFont(dword rotation, bool bold, bool italic, bool big, int height) const{

   rotation &= 3;
   static const dword rotation_angles[] = { 0, 900, 1800, 2700 };
   void *&h = font_handle[rotation][bold][italic][big];
   assert(!h);
   win::LOGFONT lf = {
      height,
      0,                //width
      rotation_angles[rotation],                //escapement
      rotation_angles[rotation],                //orientation
      bold ? FW_BOLD : FW_NORMAL, //weight
      //FW_NORMAL, //weight
      italic,            //italic
      false,            //underline
      false,            //strike
      ANSI_CHARSET,     //charset
      //DEFAULT_CHARSET,
      OUT_DEFAULT_PRECIS,  //out precission
      CLIP_DEFAULT_PRECIS, //clip precission
      //DEFAULT_QUALITY,  //quality
      use_antialias ? DEFAULT_QUALITY : NONANTIALIASED_QUALITY,
      DEFAULT_PITCH,    //pitch & family
      //L"Arial"
      //L"Tahoma"
#ifdef _DEBUG_
      L"Courier New"
#else
      L"NewTimes"
#endif
   };
   h = win::CreateFontIndirect(&lf);
   SelectFont(h);
   //SetTextCharacterExtra((HDC)hdc, 0);

   win::TEXTMETRIC tm;
   win::GetTextMetrics((win::HDC)hdc, &tm);
   if(!italic){
      font_baseline[bold][big] = byte(tm.tmAscent - (tm.tmInternalLeading-1));

      byte *fw = font_width + int(bold)*NUM_CACHE_WIDTH_CHARS*2 + int(big)*NUM_CACHE_WIDTH_CHARS;
      for(int i=0; i<NUM_CACHE_WIDTH_CHARS; i++){
         int w;
         win::GetCharWidth32((win::HDC)hdc, 32+i, 32+i, &w);
         /*
         win::ABC abc;
         win::GetCharABCWidths((win::HDC)hdc, 32+i, 32+i, &abc);
         int w = abc.abcB + abc.abcC;
         */
         --w;
         fw[i] = (byte)w;
      }
   }
}

//----------------------------

void C_application_base::RenderSystemChar(int x, int y, wchar c, dword curr_font, dword font_flags, dword text_color){

   assert(curr_font<2);
   bool bold = (font_flags&FF_BOLD);
   bool italic = (font_flags&FF_ITALIC);
   dword rotation = (font_flags>>FF_ROTATE_SHIFT) & 3;

   win::HDC hdc = (win::HDC)system_font.hdc;
   void *h = system_font.font_handle[rotation][bold][italic][curr_font];
   if(!h){
      system_font.InitFont(rotation, bold, italic, curr_font, !curr_font ? system_font.size_small : system_font.size_big);
      h = system_font.font_handle[rotation][bold][italic][curr_font];
   }
   assert(h);
   if(system_font.last_used_font!=h)
      system_font.SelectFont(h);

   const int baseline = system_font.font_baseline[bold][curr_font];

   switch(rotation){
   case 0:
      y += baseline;
      break;
   case 1:
      Swap(x, y);
      y = ScrnSY()-y;
      x += baseline;
      break;
   case 3:
      Swap(x, y);
      x = ScrnSX()-x;
      x -= baseline;
      break;
   }

   if(font_flags&FF_ACTIVE_HYPERLINK){
      dword w = system_font.GetCharWidth(c, bold, italic, curr_font)+1;
      if(bold)
         w += font_defs[curr_font].bold_width_add;
      FillRect(S_rect(x, y-baseline, w, font_defs[curr_font].line_spacing-1), text_color);
      text_color ^= 0xffffff;
   }
   if(font_flags&FF_UNDERLINE){
      dword w = system_font.GetCharWidth(c, bold, italic, curr_font)+1;
      if(bold)
         w += font_defs[curr_font].bold_width_add;
      DrawHorizontalLine(x, y+1, w, text_color);
   }
   if(system_font.last_text_color!=text_color){
      system_font.last_text_color = text_color;
      dword col = win::SetTextColor(hdc, ((text_color&0xff)<<16) | (text_color&0xff00) | ((text_color>>16)&0xff));
      if(col==CLR_INVALID){
         system_font.InitHdc();
         win::SetTextColor(hdc, ((text_color&0xff)<<16) | (text_color&0xff00) | ((text_color>>16)&0xff));
      }
   }
   bool ok = win::ExtTextOutW(hdc, x, y, ETO_CLIPPED, (const win::RECT*)&system_font.clip_rect, &c, 1, NULL);
   if(!ok){
      system_font.InitHdc();
      win::ExtTextOutW(hdc, x, y, ETO_CLIPPED, (const win::RECT*)&system_font.clip_rect, &c, 1, NULL);
   }
}

//----------------------------
//----------------------------

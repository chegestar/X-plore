//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#ifndef __IGRAPH_H
#define __IGRAPH_H

#include <Rules.h>
#include <SmartPtr.h>
#include <Util.h>

//----------------------------
//#define SUPPORT_12BIT_PIXEL   //define to include support for 12-bit color depth pixel


struct S_pixelformat{
   byte bytes_per_pixel;     //bytes which each pixel ocupy (may be 2 or 4)
   byte bits_per_pixel;      //pixel bitdepth (12/16/32)
   dword r_mask, g_mask, b_mask, a_mask;  //masks of components

//----------------------------
// Initialize pixelformat structure from given pixel bitdepth value.
   void Init(byte bits_per_pixel);
};

//----------------------------

struct S_rgb{
   byte b, g, r;
   byte a;
   inline byte &operator[](int i){ return (&b)[i]; }
};

struct S_rgb_x: public S_rgb{
#ifdef SUPPORT_12BIT_PIXEL
   inline void From12bit(word p){ r = byte((p>>4)&0xf0); g = byte(p&0xf0); b = byte((p<<4)&0xff); }
   word To12bit() const{ return ((r&0xf0)<<4) | (g&0xf0) | (b>>4); }
#endif
   inline void From16bit(word p){ r = byte((p>>8)&0xf8); g = byte((p>>3)&0xfc); b = byte((p<<3)&0xff); }
   word To16bit() const{ return ((r&0xf8)<<8) | ((g&0xfc)<<3) | ((b&0xf8)>>3); }

   inline void From32bit(dword p){ *(dword*)this = p; }
   dword To32bit() const{ return *(dword*)this; }

   inline void Mask(dword mask){ *(dword*)this &= mask; }

   void BlendWith(const S_rgb &p, dword alpha){
      dword ia = 256-alpha;
      r = byte((r*ia + p.r*alpha)>>8);
      g = byte((g*ia + p.g*alpha)>>8);
      b = byte((b*ia + p.b*alpha)>>8);
   }
   void Add(const S_rgb &p){
      r = byte(Max(0, Min(255, r+p.r-128)));
      g = byte(Max(0, Min(255, g+p.g-128)));
      b = byte(Max(0, Min(255, b+p.b-128)));
   }
   void Mul(const S_rgb &p){
      r = byte((r*p.r)>>8);
      g = byte((g*p.g)>>8);
      b = byte((b*p.b)>>8);
   }

   inline S_rgb_x(){}
   inline S_rgb_x(dword col){
      From32bit(col);
   }
};

//----------------------------
// Color blending.
dword MulColor(dword col, dword mul_fp);    //multiply color by 16.16 fixed-point value, preserve alpha
dword MulAlpha(dword col, dword mul_fp);    //multiply alpha by 16.16 fixed-point value, preserve color
dword BlendColor(dword col0, dword col1, dword mul_fp);    //blend 2 colors by 16.16 fixed-point value

//----------------------------
                              //virtual key values
enum IG_KEY{
                              //most of the virtual codes map to Win32 key codes
   K_NOKEY = 0,

   K_SEND, K_HANG,

                              //Symbian: K_SOFT_MENU on left, K_SOFT_SECONDARY on right
                              //WM: K_SOFT_MENU on right, K_SOFT_SECONDARY on left
   K_SOFT_MENU,               //
   K_SOFT_SECONDARY,

   //K_CLOSE,                   //close request (on some devices, closing app can be done by key or button)

   K_DEL = 7, K_BACKSPACE, K_TAB,
   K_ENTER = 13,
   K_SHIFT = 16, K_CTRL, K_ALT,
   K_CURSORLEFT = 20, K_CURSORUP, K_CURSORRIGHT, K_CURSORDOWN,
   K_ESC = 27,
   K_BACK = 31,
                              //codes 32-127 map to ASCII

                              //PocketPC keys:
   K_HARD_1 = 128,
   K_HARD_2,
   K_HARD_3,
   K_HARD_4,
   K_MENU,
                              //multimedia keys (S60 3rd):
   K_VOLUME_UP, K_VOLUME_DOWN,
   K_MEDIA_STOP,
   K_MEDIA_PLAY_PAUSE,
   K_MEDIA_FORWARD,
   K_MEDIA_BACKWARD,
   K_PAGE_UP,
   K_PAGE_DOWN,
   K_HOME,
   K_END,
   K_SEARCH,                  //search key (Android)
};

#define K_LEFT_SOFT K_SOFT_MENU
#define K_RIGHT_SOFT K_SOFT_SECONDARY
//----------------------------

enum{                         //key bits - each key has assigned 1 bit, used for detection of simultaneous key presses
   GKEY_LEFT = 1,
   GKEY_UP = 2,
   GKEY_RIGHT = 4,
   GKEY_DOWN = 8,

   GKEY_OK = 0x10,
   GKEY_SOFT_MENU = 0x20,
   GKEY_SOFT_SECONDARY = 0x40,
   //GKEY_CLEAR = 0x80,

   //GKEY_APPLICATION = 0x100,
   GKEY_SEND = 0x200,
   //GKEY_END = 0x400,
   GKEY_SHIFT = 0x800,

   GKEY_0 = 0x1000,
   GKEY_1 = 0x2000,
   GKEY_2 = 0x4000,
   GKEY_3 = 0x8000,

   GKEY_4 = 0x10000,
   GKEY_5 = 0x20000,
   GKEY_6 = 0x40000,
   GKEY_7 = 0x80000,

   GKEY_8 = 0x100000,
   GKEY_9 = 0x200000,
   //GKEY_ASTERISK = 0x400000,
   //GKEY_HASH = 0x800000,

   //GKEY_A = 0x1000000,
   GKEY_VOLUME_UP = 0x1000000,
   //GKEY_B = 0x2000000,
   GKEY_VOLUME_DOWN = 0x2000000,
   //GKEY_C = 0x4000000,
   //GKEY_START = 0x8000000,

   //GKEY_FORWARD = 0x10000000,
   //GKEY_BACK = 0x20000000,
   GKEY_CTRL = 0x40000000,
   GKEY_ALT = 0x80000000,
};

//----------------------------
                              //touch event flags
enum{
   MOUSE_BUTTON_1_DOWN = 1,
   MOUSE_BUTTON_1_DRAG = 0x100,
   MOUSE_BUTTON_1_DRAG_MULTITOUCH = 0x200,   //used together with DRAG, if secondary touch is valid
   MOUSE_BUTTON_1_UP = 0x10000,
};

//----------------------------
// graphics initialization flags
enum{
                              //bit depth simulation works only on Windows Mobile in debug builds
   IG_DEBUG_12_BIT = 1,       //12-bit 444 screen pixel-format
   IG_DEBUG_15_BIT = 2,       //15-bit 555 screen pixel-format
   IG_DEBUG_32_BIT = 4,       //32-bit screen

   IG_SCREEN_ALLOW_32BIT = 0x10, //allow initializing backbuffer to 32-bit pixel format (if screen uses it); otherwise use 2-byte pixel format (16-bit)
   IG_ENABLE_BACKLIGHT = 0x20,//enable/disable backlight. The behavior is platform-dependent, but generally setting to 'on' should turn backlight all the time on (as long as application is active), and setting to 'off' should disable backlight, either always, or when there's no keys/pen action.
   IG_USE_MEDIA_KEYS = 0x40,  //use media keys (volume, playback, etc) if available
   IG_SCREEN_USE_CLIENT_RECT = 0x80,//use native client rectangle only (no fullscreen)

   IG_SCREEN_USE_DSA = 0x100, //use direct-screen-access
   IG_DEBUG_LANDSCAPE = 0x200,//simulate landscape mode on Wins (rotated 90 deg CW)
   IG_SCREEN_SHOW_SIP = 0x400,//show soft input panel (button) on WM

   IG_DEBUG_NO_KB = 0x20000,     //simulate device without any keyboard
   IG_DEBUG_NO_FULL_KB = 0x40000,//  ''                 full keyboard
};

//----------------------------
class C_application_base;
                              //internal graphics class - don't use
class IGraph{
protected:
   dword screen_pitch;

   friend class C_application_base;
   virtual ~IGraph(){}

//----------------------------
// Update screen - copy contents of backbuffer to visible screen.
// If 'num_rects' is nonzero, only given dirty rectangles are updated.
   virtual void _UpdateScreen(const struct S_rect *rects = NULL, dword num_rects = 0) const = 0;

//----------------------------
// Get pointer to top-left corner of backbuffer.
   virtual void *_GetBackBuffer() const = 0;

   inline dword _GetScreenPitch() const{ return screen_pitch; }

//----------------------------
// Reinit screen with new flags. This call may result in screen resizing.
// Currently these flags may be specified: IG_SCREEN_USE_CLIENT_RECT, IG_SCREEN_USE_DSA
   virtual void _UpdateFlags(dword new_flags, dword flags_mask) = 0;

//----------------------------
// Get screen's rotation relative to device's default orientation.
   virtual dword _GetRotation() const{ return 0; }

//----------------------------
// Get resolution of entire screen area.
   virtual struct S_point _GetFullScreenSize() const = 0;

                              //platform-specific methods
#ifdef __SYMBIAN32__
   virtual const class CFbsBitmapDevice &GetBackBufferDevice() const = 0;
   virtual class CDirectScreenAccess *GetDsa() const = 0;
#endif
#if defined _WINDOWS || defined _WIN32_WCE
   virtual dword GetPhysicalRotation() const = 0;   //get physical screen rotation
#endif
//----------------------------
public:

#ifdef __SYMBIAN32__
   virtual class CCoeControl *GetCoeControl() = 0;
   virtual class CFbsBitGc &GetBitmapGc() const = 0;
   virtual const class CFbsBitmap &GetBackBufferBitmap() const = 0;
   virtual void ScreenChanged() = 0;
   virtual void StartDrawing() = 0;
   virtual void StopDrawing() = 0;
#endif
#ifdef BADA
   virtual void *GetCanvas() = 0;
   //virtual void *GetForm() = 0;
#endif
#ifdef ANDROID
   virtual void *GetJavaCanvas() const = 0;
   virtual void *GetJavaBitmap() const = 0;
   virtual void SetClipRect(const S_rect &rc) = 0;
#endif
#if defined _WINDOWS || defined _WIN32_WCE
   virtual void *GetHWND() const = 0;
   virtual void *GetHDC() const = 0;
#endif
};

//----------------------------
//----------------------------
#include <Fixed.h>
#include <ZipRead.h>

//----------------------------

class C_image: public C_unknown{
public:
   enum{
      IMAGE_OPEN_NO_ANIMATIONS = 1,    //don't allow animations on opened image
      IMAGE_OPEN_NO_DITHERING = 2,     //disable dithering (only in 16-bit pixel format)
      IMAGE_OPEN_NO_ASPECT_RATIO = 4,  //don't keep aspect ratio when fitting dimensions to limit size
      IMAGE_OPEN_FORCE_32BIT_PIXEL = 8,//force pixel format to be 32-bit (even if backbuffer is different)
   };

   virtual ~C_image(){}

//----------------------------
// Progress callback interface, used to report loading progress.
   class C_open_progress{
   public:
   //----------------------------
   // percent = 0 - 100 value increasing as image is being loaded
   // Return value: true to continue loading, false to cancel loading and immediately return control; return code from Open will be IMG_CANCELED.
      virtual bool ImageLoadProgress(dword percent) = 0;
   };

//----------------------------
// Helper class used to load and resize image and get output lines.
// Inherited class must implement OutputLine function to receive resized line in RGB format, and store it how it needs.
   class C_image_loader_resizer{
   protected:
      struct S_rgb{
         byte b, g, r;
         byte a;
      };
   //----------------------------
   // Output single loaded resized line.
      virtual void OutputLine(const S_rgb *line) = 0;
   public:
   //----------------------------
   // Load/decode entire image, while resizing it and outputing lines using OutputLine function as soon as they're fetched from loader.
   // loader = initialized image loader
   // dst_size, rescale_ratio = size of destination image and resize ratio as computed by C_image::ComputeSize
   // p_Progress, progress_context = same as in C_image::Open
   // special_open_draw_pos = NULL
      bool LoadAndDecodeImage(class C_image_loader *loader, const S_point &dst_size, C_fixed rescale_ratio[2], C_open_progress *open_progress = NULL, S_point *special_open_draw_pos = NULL);
   };
//----------------------------

   enum E_RESULT{
      IMG_OK,                 //image opened successfully
      IMG_NO_FILE,            //source file can't be opened
      IMG_NO_LOADER,          //there's no loader/decoder for this file
      IMG_NOT_ENOUGH_MEMORY,  //not enough memory for storing image data
      IMG_BAD_PARAMS,         //bad parameters passed to Open function
      IMG_CANCELED            //opening canceled in p_Progress callback function
   };

//----------------------------
// Open image from file.
//    filename ... name of file
//    limit_size_? ... limiting size, if set to 0, it is not used
//    max_ratio ... ratio by which image will be shrinked. Must be >0 and <=1
//    arch ... archive from which file is opened
//    open_progress ... callback for progress indication
//    file_offs ... offset in file where image begins
//    own_file ... if not NULL, it is used as source file, and filename is ignored
//    flags ... combination of IMAGE_OPEN_* flags
   virtual E_RESULT Open(const wchar *filename, int limit_size_x, int limit_size_y, C_fixed max_ratio = C_fixed::One(),
      const C_zip_package *arch = NULL, C_open_progress *open_progress = NULL, dword file_offs = 0, C_file *own_file = NULL,
      dword flags = 0) = 0;

   inline E_RESULT Open(const wchar *filename, int limit_size_x, int limit_size_y, const C_zip_package *arch){
      return Open(filename, limit_size_x, limit_size_y, C_fixed::One(), arch);
   }

//----------------------------
// Special opening of (resized) image, when exact rescale ratio is taken from img,
// and pixel filtering is computed so that this image may be drawn on top of img in draw_pos.
// draw_pos on input specifies this image's relative offset to img, on return it returns position where this image should be drawn on img for pixel match.
// In other words, this is for opening 'sprites' drawn on top of background image with matching pixel resizing.
   virtual E_RESULT OpenRelativeToImage(const wchar *filename, const C_image *img, S_point &draw_pos, const C_zip_package *arch) = 0;

//----------------------------
// Compute size of image, based on given limitations (fixed or relative).
// Computed size will be equal or smaller than those of original image, and each dimension will be bigger or equal to 1.
// sx, sy ... on input contain size of original image
// limit_size_* ... rectangle where computed size must fit (while retaining aspect ratio)
// max_ratio ... multiplier how image will be shrinked, may be in range 0 - 1
// ratio_* ... shrink ratio for computed size
   static void ComputeSize(int &sx, int &sy, int limit_size_x, int limit_size_y, C_fixed max_ratio, C_fixed &ratio_x, C_fixed &ratio_y, bool keep_aspect_ratio = true);

//----------------------------
// Create empty image.
// Returns false if memory can't be allocated.
   virtual bool Create(dword sx, dword sy, bool alpha_channel = false) = 0;

//----------------------------
// Create image with pixels copied from screen rectangle.
// Returns false if memory can't be allocated.
   virtual bool CreateFromScreen(const S_rect &rc) = 0;

//----------------------------
// Create rotation of other image.
// rotation: 1 = 90CCW, -1 = 90CW, 2 = 180
// Returns false is memory for new image can't be allocated.
   virtual bool CreateRotated(const C_image *img, int rotation) = 0;

//----------------------------
// Rotate image.
// rotation: 1 = 90CCW, -1 = 90CW, 2 = 180
// Similar to CreateRotated, but without allocating new image buffers. It is more optiomal for memory allocation, but slower.
// Internally it allocates SizeX()*SizeY()/8 temp buffer.
// Returns false is there's not enough memory.
   virtual bool Rotate(int rotation) = 0;

//----------------------------
// Set rgb pixels of image. 'pixels' must be array of [SizeX()*SizeY()].
   //virtual void SetPixels(const S_rgb_x *pixels) = 0;

//----------------------------
// Set alpha channel of image. If 'alpha' is NULL, then alpha channel is removed. 'alpha' must be array of [SizeX()*SizeY()].
   virtual void SetAlphaChannel(const byte *alpha) = 0;

//----------------------------
// Close image.
   virtual void Close() = 0;

//----------------------------
// Draw image to IGraph's backbuffer. Function performs clipping if image is drawn outside of IGraph's viewport.
//    x, y ... position of top-left corner.
   virtual void Draw(int x, int y, int rotation = 0) const = 0;

//----------------------------
// Draw image in special mode.
//    rc_src - source rectangle, if NULL then entire image is drawn
//    alpha - alpha from 0 to 1
//    mul_color - multiply color
   virtual void DrawSpecial(int x, int y, const S_rect *rc_src = NULL, C_fixed alpha = C_fixed::One(), dword mul_color = 0xffffffff) const = 0;

//----------------------------
// Compute size of drawn zoomed image fit into given rectangle while preserving aspect ratio.
// img_sx/img_sy = size of original image
// fit_sx/fit_sy = IN: size of rectangle where image must fit; OUT: computed real size for drawing, preserving aspect ratio
   static void ComputeZoomedImageSize(int img_sx, int img_sy, int &fit_sx, int &fit_sy);

//----------------------------
// Draw zoomed image. It is clipped to current clip rectangle.
// rc_fit = rectangle where image will fit; size sx/sy must be greater than 0
// keep_aspect_ratio = when set, function calls internally ComputeZoomedImageSize and draws image with preserverd pixel aspect ratio.
   virtual void DrawZoomed(const S_rect &rc_fit, bool keep_aspect_ratio = false) const = 0;

//----------------------------
// Draw image as nine-patch. Image is drawn into rc.
//    corner_src_size = size of source image, that is used as corner area (for all 4 corners) and drawn without resizing; edges and center are drawn with rescaling
   virtual void DrawNinePatch(const S_rect &rc, const S_point &corner_src_size) const = 0;

//----------------------------
// Get size of image, in pixels.
   virtual const S_point &GetSize() const = 0;
   virtual dword SizeX() const = 0;
   virtual dword SizeY() const = 0;

   virtual const S_point &GetOriginalSize() const = 0;

//----------------------------
// Get pointer to image buffer, in application's backbuffer pixel format. Size of buffer is SizeX*SizeY.
   virtual const byte *GetData() const = 0;

//----------------------------
// Get pointer to alpha buffer. This is nonzero only 2-byte pixel formats (16-bit). On 32-bit pixel format, alpha is stored directly in pixel data.
   virtual const byte *GetAlpha() const = 0;

//----------------------------
// Get size of image's line, in bytes.
   virtual dword GetPitch() const = 0;
   inline dword GetAlphaPitch() const{ return SizeX(); }

//----------------------------
// Get pixel format (bits per pixel). By default it's same as backbuffer bit depth. If IMAGE_OPEN_FORCE_32BIT_PIXEL is used to open image, it will be 32 bit.
   //virtual byte GetBitsPerPixel() const = 0;

//----------------------------
// Check if image is transparent.
   virtual bool IsTransparent() const = 0;

//----------------------------
// Check if image is animated.
   virtual bool IsAnimated() const{ return false; }

//----------------------------
// Tick animation in image. Return true if image changed (should be redrawn).
   virtual bool Tick(int time){ return false; }

//----------------------------
// Crop image to given rectange. This allocates new pixel buffer and adjusts dimensions.
// Function fails if memory for buffer can't be allocated, or if rc is outside of image's area.
   virtual bool Crop(const S_rect &rc) = 0;

//----------------------------
// Create this class.
   static C_image *Create(const C_application_base &app);
};

//----------------------------
                              //image loaders, must be defined in applications
typedef class C_image_loader *(*t_CreateLoader)(C_file *ck);
extern const t_CreateLoader image_loaders[];   //pointers to loader creation functions, terminated by NULL
//----------------------------
//----------------------------

#endif


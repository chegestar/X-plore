//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <IGraph.h>
#include <C_file.h>
#include <ImgLib.h>
#include <AppBase.h>
#include "Graph_i.h"

#if 0
#include <Profiler.h>
enum{
   PROF_ALL,
   PROF_LOAD,
   PROF_HALVE,
   PROF_SHRINK,
   PROF_BLEND,
   PROF_WRITE,
};
extern const char prof_img_texts[] =
   "- Img open\0"
   "Load\0"
   "Halve\0"
   "Shrink\0"
   "Blend\0"
   "Write\0"
;
extern C_profiler *prof_img;
#define PROF_S(n) prof_img->MarkS(n)
#define PROF_E(n) prof_img->MarkE(n)

#else

#define PROF_S(n)
#define PROF_E(n)
#endif

//----------------------------
#ifdef _DEBUG
#endif
//#define DEBUG_NO_DITHER

#define min(a, b) ((a)<(b) ? (a) : (b))   //faster than Min on ARM
//----------------------------
//----------------------------

class C_image_imp: public C_image{
   const C_application_base &app;

   byte *bitmap;
   byte *alpha;               //holds 8-bit alpha only in case of 2-byte pixel format, otherwise alpha is directly in pixels
   S_point size;
   S_point orig_size;
   byte bytes_per_pixel, bits_per_pixel;

   bool has_alpha;

   C_fixed rescale_ratio[2];
                              //for animated images, keep loader
   C_image_loader *anim_loader;

//----------------------------

   E_RESULT OpenInternal(const wchar *filename, int limit_size_x, int limit_size_y, C_fixed max_ratio, const C_zip_package *arch,
      C_open_progress *open_progress, dword file_offs, C_file *own_file, dword flags, S_point *special_open_draw_pos);

//----------------------------

   virtual E_RESULT Open(const wchar *filename, int limit_size_x, int limit_size_y, C_fixed max_ratio, const C_zip_package *arch,
      C_open_progress *open_progress, dword file_offs, C_file *own_file, dword flags){

      return OpenInternal(filename, limit_size_x, limit_size_y, max_ratio, arch, open_progress, file_offs, own_file, flags, NULL);
   }

//----------------------------

   virtual E_RESULT OpenRelativeToImage(const wchar *filename, const C_image *_img, S_point &draw_pos, const C_zip_package *arch){

      const C_image_imp *img = (C_image_imp*)_img;
      if(!img)
         return IMG_BAD_PARAMS;
      if(!img->bitmap)
         return IMG_BAD_PARAMS;

                              //adopt rescale ratio exactly from img
      MemCpy(rescale_ratio, img->rescale_ratio, sizeof(rescale_ratio));

      E_RESULT r = OpenInternal(filename, 0, 0, 1, arch, NULL, 0, NULL, 0, &draw_pos);
      return r;
   }

//----------------------------

   virtual bool Create(dword sx, dword sy, bool alpha_channel){

      Close();
      const S_pixelformat &pf = app.GetPixelFormat();
      bytes_per_pixel = pf.bytes_per_pixel;
      bits_per_pixel = pf.bits_per_pixel;
      bitmap = new byte[sx*sy*bytes_per_pixel];
      if(!bitmap)
         return false;
      orig_size.x = size.x = sx;
      orig_size.y = size.y = sy;
      if(alpha_channel){
         has_alpha = true;
         if(bytes_per_pixel==2){
            alpha = new byte[size.x * size.y];
            if(!alpha){
               delete[] bitmap;
               bitmap = NULL;
               return false;
            }
         }
      }
      return true;
   }

//----------------------------

   virtual bool CreateFromScreen(const S_rect &rc){
      if(rc.x<0 || rc.y<0 || rc.Right()>app.ScrnSX() || rc.Bottom()>app.ScrnSY()){
         LOG_RUN("CreateFromScreen: invalid rectangle");
         LOG_RUN_N("x", rc.x);
         LOG_RUN_N("y", rc.y);
         LOG_RUN_N("sx", rc.sx);
         LOG_RUN_N("sy", rc.sy);
         return false;
      }
      if(!Create(rc.sx, rc.sy, false))
         return false;
                              //copy pixels from backbuffer
      const byte *src = (byte*)app.GetBackBuffer();
      byte *dst = bitmap;
      const S_pixelformat &pf = app.GetPixelFormat();
      bytes_per_pixel = pf.bytes_per_pixel;
      bits_per_pixel = pf.bits_per_pixel;
      dword src_pitch = app.GetScreenPitch();
      src += rc.y*src_pitch + rc.x*bytes_per_pixel;
      for(int y=rc.sy; y--; ){
         MemCpy(dst, src, rc.sx*bytes_per_pixel);
         src += src_pitch;
         dst += size.x*bytes_per_pixel;
      }
      return true;
   }

//----------------------------

   virtual bool CreateRotated(const C_image *img, int rotation);

   virtual bool Rotate(int rotation);

//----------------------------
   /*
   virtual void SetPixels(const S_rgb_x *pixels){

      const S_pixelformat &pf = app.GetPixelFormat();
      bytes_per_pixel = pf.bytes_per_pixel;
      bits_per_pixel = pf.bits_per_pixel;
      byte *dst = bitmap;
      for(int i=size.x*size.y; i--; ){
         const S_rgb_x &p = *pixels++;
         switch(bits_per_pixel){
#ifdef SUPPORT_12BIT_PIXEL
         case 12: *((word*&)dst)++ = p.To12bit(); break;
#endif
         case 16: *((word*&)dst)++ = p.To16bit(); break;
         case 32: *((dword*&)dst)++ = p.To32bit(); break;
         }
      }
   }
   */

//----------------------------

   virtual void SetAlphaChannel(const byte *in_alpha){

      if(!bitmap)
         return;
      if(in_alpha){
         has_alpha = true;
         if(bytes_per_pixel==2){
            if(!alpha)
               alpha = new byte[size.x * size.y];
            MemCpy(alpha, in_alpha, size.x * size.y);
         }else{
            byte *bp = bitmap+3;
            for(const byte *end = in_alpha+size.x*size.y; in_alpha<end; bp+=4)
               *bp = *in_alpha++;
         }
      }else{
         has_alpha = false;
         delete[] alpha;
         alpha = NULL;
      }
   }

//----------------------------

   virtual void Close(){

      delete[] bitmap; bitmap = NULL;
      delete[] alpha; alpha = NULL;
      size.x = size.y = 0;
      has_alpha = false;
      delete anim_loader; anim_loader = NULL;
   }

//----------------------------

   virtual void Draw(int x, int y, int rotation = 0) const;
   virtual void DrawSpecial(int x, int y, const S_rect *rc_src, C_fixed alpha = C_fixed::One(), dword mul_color = 0xffffffff) const;
   virtual void DrawZoomed(const S_rect &rc_fit, bool keep_aspect_ratio = false) const;
   virtual void DrawNinePatch(const S_rect &rc, const S_point &corner_src_size) const;

//----------------------------

   virtual dword SizeX() const{ return size.x; }
   virtual dword SizeY() const{ return size.y; }
   virtual const S_point &GetSize() const{ return size; }

   virtual const S_point &GetOriginalSize() const{
      return orig_size;
   }

   virtual const byte *GetData() const{ return bitmap; }

   virtual const byte *GetAlpha() const{ return alpha; }

   virtual bool IsTransparent() const{ return has_alpha; }

//----------------------------

   virtual dword GetPitch() const{
      return size.x * bytes_per_pixel;
   }

//----------------------------

   //virtual byte GetBitsPerPixel() const{ return bits_per_pixel; }

//----------------------------

   virtual bool IsAnimated() const{
      return (anim_loader!=NULL);
   }

//----------------------------
// Tick animation in image. Return true if image changed (should be redrawn).
   virtual bool Tick(int time);

//----------------------------

   virtual bool Crop(const S_rect &rc);

//----------------------------

   virtual ~C_image_imp(){
      Close();
   }

//----------------------------

   bool LoadAndDecodeImage(C_image_loader *loader, C_open_progress *open_progress, bool dither, S_point *special_open_draw_pos = NULL);

public:
   C_image_imp(const C_application_base &_app):
      app(_app),
      bitmap(NULL),
      alpha(NULL),
      has_alpha(false),
      size(0, 0),
      orig_size(0, 0),
      bytes_per_pixel(0),
      bits_per_pixel(0),
      anim_loader(NULL)
   {
   }
};

//----------------------------

bool C_image_imp::CreateRotated(const C_image *src_, int rotation){

   C_image_imp *src = (C_image_imp*)src_;
   if(src!=this)
      Close();
   rotation &= 3;
   dword src_sx = src->SizeX();
   dword src_sy = src->SizeY();

   size.x = src_sx;
   size.y = src_sy;
   if(rotation!=2)
      Swap(size.x, size.y);
   bytes_per_pixel = src->bytes_per_pixel;
   bits_per_pixel = src->bits_per_pixel;

   byte *bitmap_ = new byte[size.x*size.y*bytes_per_pixel];
   if(!bitmap_)
      return false;
   byte *alpha_ = NULL;
   if(src->alpha){
      alpha_ = new byte[size.x*size.y];
      if(!alpha_){
         delete[] bitmap_;
         return false;
      }
   }
   const byte *d_src = (byte*)src->bitmap;
   const byte *d_src_a = src->alpha;
   byte *d_dst = bitmap_;
   byte *d_dst_a = alpha_;
   const dword src_pitch_y = src_sx*bytes_per_pixel;
   int dst_pitch_x = bytes_per_pixel;
   int dst_pitch_y = size.x*bytes_per_pixel;
   int dst_pitch_x_a = 1;
   int dst_pitch_y_a = size.x;

   switch(rotation){
   case 1:
      d_dst += (src_sx-1)*dst_pitch_y;
      if(d_dst_a)
         d_dst_a += (src_sx-1)*dst_pitch_y_a;
      dst_pitch_x = -dst_pitch_y;
      dst_pitch_y = bytes_per_pixel;
      dst_pitch_x_a = -dst_pitch_y_a;
      dst_pitch_y_a = 1;
      break;
   case 3:
      d_dst += (src_sy-1)*dst_pitch_x;
      if(d_dst_a)
         d_dst_a += (src_sy-1)*dst_pitch_x_a;
      dst_pitch_x = dst_pitch_y;
      dst_pitch_y = -int(bytes_per_pixel);
      dst_pitch_x_a = dst_pitch_y_a;
      dst_pitch_y_a = -1;
      break;
   case 2:
      d_dst += (src_sx*src_sy - 1)*bytes_per_pixel;
      if(d_dst_a)
         d_dst_a += (src_sx*src_sy - 1);
      dst_pitch_x = -dst_pitch_x;
      dst_pitch_y = -dst_pitch_y;
      dst_pitch_x_a = -dst_pitch_x_a;
      dst_pitch_y_a = -dst_pitch_y_a;
      break;
   default:
      assert(0);
   }
   while(src_sy--){
      const byte *src_line = d_src;
      byte *dst_line = d_dst;

      for(int i=src_sx; i--; ){
         if(bytes_per_pixel==2)
            *(word*)dst_line = *(word*)src_line;
         else
            *(dword*)dst_line = *(dword*)src_line;
         src_line += bytes_per_pixel;
         dst_line += dst_pitch_x;
      }
      d_src += src_pitch_y;
      d_dst += dst_pitch_y;
      if(d_dst_a){
         const byte *src_line1 = d_src_a;
         byte *dst_line1 = d_dst_a;
         for(int i=src_sx; i--; ){
            *dst_line1 = *src_line1++;
            dst_line1 += dst_pitch_x_a;
         }
         d_src_a += src_sx;
         d_dst_a += dst_pitch_y_a;
      }
   }
   delete[] bitmap;
   delete[] alpha;
   bitmap = bitmap_;
   alpha = alpha_;
   has_alpha = src_->IsTransparent();
   orig_size = src->orig_size;
   MemCpy(rescale_ratio, src->rescale_ratio, sizeof(rescale_ratio));
   if(rotation&1){
      Swap(orig_size.x, orig_size.y);
      Swap(rescale_ratio[0], rescale_ratio[1]);
   }
   return true;
}

//----------------------------

static void GetRotatedCoord(dword x, dword y, dword sx, dword sy, dword rot, dword &x1, dword &y1){

   switch(rot){
   case 1:
      x1 = y;
      y1 = sy-1-x;
      break;
   case 3:
      y1 = x;
      x1 = sx-1-y;
      break;
   }
}

//----------------------------
template<typename T>
static void RotateBuffer180(T *buf, dword sx, dword sy){
   for(dword y=sy/2; y--; ){
      for(dword x=0; x<sx; x++)
         Swap(buf[y*sx+x], buf[(sy-1-y)*sx+(sx-1-x)]);
   }
   if(sy&1){
      for(dword x=sx/2; x--; )
         Swap(buf[(sy/2)*sx+x], buf[(sy/2)*sx+(sx-1-x)]);
   }
}

//----------------------------

template<typename T>
static void RotateBuffer(T *buf, dword sx, dword sy, int rot, byte *tmp){

   MemSet(tmp, 0, (sx*sy+7)/8);
   dword num = sx*sy;
   for(dword y=0; num; y++){
      for(dword x=sx; x--; ){
         dword pi_start = y*sx+x;
         if(tmp[pi_start/8]&(1<<(pi_start&7)))
            continue;
         dword pi = pi_start;
         T pix = buf[pi];

         dword xx = x, yy = y;
         while(true){
            dword x1, y1;
            GetRotatedCoord(xx, yy, sy, sx, rot, x1, y1);
            assert(x1<sy && y1<sx);
            dword pi1 = y1*sy+x1;
            Swap(pix, buf[pi1]);
            assert(!(tmp[pi/8]&(1<<(pi&7))));
            tmp[pi/8] |= (1<<(pi&7));
            --num;
            if(pi_start==pi1)
               break;

            pi = pi1;
            yy = pi1/sx;
            xx = pi1-yy*sx;
         }
      }
   }
}

//----------------------------

bool C_image_imp::Rotate(int rotation){

   rotation &= 3;
   if(!rotation)
      return true;
   if(!bitmap)
      return false;

   if(rotation==2){
      if(bytes_per_pixel==2)
         RotateBuffer180((word*)GetData(), size.x, size.y);
      else
         RotateBuffer180((dword*)GetData(), size.x, size.y);
      if(alpha)
         RotateBuffer180(alpha, size.x, size.y);
   }else{
      byte *tmp = new byte[(size.x*size.y+7)/8];
      if(!tmp)
         return false;
      if(bytes_per_pixel==2)
         RotateBuffer((word*)GetData(), size.x, size.y, rotation, tmp);
      else
         RotateBuffer((dword*)GetData(), size.x, size.y, rotation, tmp);
      if(alpha)
         RotateBuffer(alpha, size.x, size.y, rotation, tmp);
      Swap(size.x, size.y);
      delete[] tmp;
   }
   return true;
}

//----------------------------

bool C_image_imp::Crop(const S_rect &rc){

   if(rc.x<0 || rc.y<0 || rc.Right()>size.x || rc.Bottom()>size.y)
      return false;
   //assert(!IsTransparent());

   byte *_bmp = new byte[rc.sx*rc.sy*bytes_per_pixel];
   if(!_bmp)
      return false;
   byte *_alp = NULL;
   if(alpha){
      _alp = new byte[rc.sx*rc.sy];
      if(!_alp){
         delete[] _bmp;
         return false;
      }
   }
   byte *dst = _bmp;
   const byte *src = bitmap + rc.y*GetPitch() + rc.x*bytes_per_pixel;
   for(int y=0; y<rc.sy; y++){
      MemCpy(dst, src, rc.sx*bytes_per_pixel);
      src += GetPitch();
      dst += rc.sx*bytes_per_pixel;
   }
   if(alpha){
      src = alpha + rc.y*GetAlphaPitch() + rc.x;
      dst = _alp;
      for(int y=0; y<rc.sy; y++){
         MemCpy(dst, src, rc.sx);
         src += GetAlphaPitch();
         dst += rc.sx;
      }
      delete[] alpha;
      alpha = _alp;
   }
   delete[] bitmap;
   bitmap = _bmp;
   size.x = rc.sx;
   size.y = rc.sy;
                              //forget animation
   delete anim_loader; anim_loader = NULL;
   return true;
}

//----------------------------

const int DITHER_SIZE = 4, DITHER_MASK = DITHER_SIZE-1;
static const byte dither_tab[DITHER_SIZE*DITHER_SIZE] = {
   0, 4, 1, 5,
   6, 2, 7, 3,
   1, 5, 0, 4,
   7, 3, 6, 2,
   /*
   0, 8, 3, 11,
   12, 4, 15, 7,
   2, 10, 1, 9,
   14, 6, 13, 5,
   /**/
};

//----------------------------

static inline void Add16BitPixelDitherVal(S_rgb &p, dword val){
   p.r = (byte)min(255, p.r + val);
   p.b = (byte)min(255, p.b + val);
   p.g = (byte)min(255, p.g + val/2);
}

//----------------------------

void C_image_imp::Draw(int x, int y, int rotation) const{

   if(!bitmap)
      return;
   rotation &= 3;
   dword d_size_x = SizeX(), d_size_y = SizeY();
   if(rotation&1)
      Swap(d_size_x, d_size_y);
                              //compute clipping region
   int l = x, t = y;
   int r = l + d_size_x, b = t + d_size_y;
   const S_rect &clip_rect = app.GetClipRect();
   const byte *src = GetData();
   int src_pitch_x = bytes_per_pixel, src_pitch_y = GetPitch();
   int alpha_pitch_x = 1, alpha_pitch_y = SizeX();

   int src_x = 0, src_y = 0;
   if(l < clip_rect.x){
      src_x = clip_rect.x - l;
      l = clip_rect.x;
   }
   if(t < clip_rect.y){
      src_y = clip_rect.y - t;
      t = clip_rect.y;
   }
   r = Min(clip_rect.Right(), r);
   b = Min(clip_rect.Bottom(), b);
   int sx = r - l, sy = b - t;
   if(sx <= 0 || sy <= 0)
      return;
   const byte *alpha_src = alpha;
   const S_pixelformat &pf = app.GetPixelFormat();
   const bool diff_pixel_depth = pf.bits_per_pixel!=bits_per_pixel;
   if(diff_pixel_depth && rotation){
      assert(0);              //not implemented
      return;
   }

   switch(rotation){
   default: assert(0);
   case 0:
      break;
   case 1:
      src += (d_size_y-1)*src_pitch_x;
      Swap(src_pitch_x, src_pitch_y);
      src_pitch_y = -src_pitch_y;
      if(alpha){
         alpha_src += (d_size_y-1)*alpha_pitch_x;
         Swap(alpha_pitch_x, alpha_pitch_y);
         alpha_pitch_y = -alpha_pitch_y;
      }
      break;
   case 2:
      src += (d_size_y-1)*src_pitch_y + (d_size_x-1)*src_pitch_x;
      src_pitch_x = -src_pitch_x;
      src_pitch_y = -src_pitch_y;
      if(alpha){
         alpha_src += (d_size_y-1)*alpha_pitch_y + (d_size_x-1)*alpha_pitch_x;
         alpha_pitch_x = -1;
         alpha_pitch_y = -alpha_pitch_y;
      }
      break;
   case 3:
      src += (d_size_x-1)*src_pitch_y;
      Swap(src_pitch_x, src_pitch_y);
      src_pitch_x = -src_pitch_x;
      if(alpha){
         alpha_src += (d_size_x-1)*alpha_pitch_y;
         Swap(alpha_pitch_x, alpha_pitch_y);
         alpha_pitch_x = -alpha_pitch_x;
      }
      break;
   }
   src += src_y*src_pitch_y + src_x*src_pitch_x;
   if(alpha)
      alpha_src += src_y*alpha_pitch_y + src_x*alpha_pitch_x;

   dword dst_pitch = app.GetScreenPitch();
   byte *dst = (byte*)app.GetBackBuffer();
   dst += dst_pitch*t + l*pf.bytes_per_pixel;
   if(!has_alpha){
      if(diff_pixel_depth){
                              //32->16 bit pixel
         int dither_y = t & DITHER_MASK;
         for(; sy--; src += src_pitch_y, dst += dst_pitch, ++dither_y &= DITHER_MASK){
            const dword *s_line = (dword*)src;
            word *d_line = (word*)dst;
            const byte *dither_line = dither_tab+dither_y*DITHER_SIZE + (l & DITHER_MASK);
            for(int x1=sx; x1--; ){
               S_rgb_x p = (S_rgb_x&)*s_line++;
               dword dither_add = *dither_line;
               if(!(dword(++dither_line)&3))
                  dither_line -= 4;
               Add16BitPixelDitherVal(p, dither_add);
               *d_line++ = p.To16bit();
            }
         }
      }else
      for(; sy--; src += src_pitch_y, dst += dst_pitch){
         if(!rotation)
            MemCpy(dst, src, sx*bytes_per_pixel);
         else{
            const byte *sline = src;
            if(bytes_per_pixel==4){
               for(int x1=0; x1<sx; x1++, sline+=src_pitch_x)
                  ((dword*)dst)[x1] = *(dword*)sline;
            }else{
               for(int x1=0; x1<sx; x1++, sline+=src_pitch_x)
                  ((word*)dst)[x1] = *(word*)sline;
            }
         }
      }
   }else{
      if(bytes_per_pixel==4){
         if(diff_pixel_depth){
            int dither_y = t & DITHER_MASK;
            for(; sy--; src += src_pitch_y, dst += dst_pitch, ++dither_y &= DITHER_MASK){
               const dword *s_line = (dword*)src;
               word *d_line = (word*)dst;
               const byte *dither_line = dither_tab+dither_y*DITHER_SIZE + (l & DITHER_MASK);
               for(int x1=sx; x1--; ){
                  S_rgb_x p = (S_rgb_x&)*s_line++;
                  if(p.a){
                     dword dither_add = *dither_line;
                     if(p.a==0xff){
                        Add16BitPixelDitherVal(p, dither_add);
                        *d_line = p.To16bit();
                     }else{
                        S_rgb_x p0;
                        p0.From16bit(*d_line);
                        p0.BlendWith(p, p.a);
                        Add16BitPixelDitherVal(p0, dither_add);
                        *d_line = p0.To16bit();
                     }
                  }
                  if(!(dword(++dither_line)&3))
                     dither_line -= 4;
                  ++d_line;
               }
            }
         }else
         for(; sy--; src += src_pitch_y, dst += dst_pitch){
            const byte *s_line = src;
            dword *d_line = (dword*)dst;
            for(int x1=sx; x1--; ){
               dword p = *(dword*)s_line;
               s_line += src_pitch_x;
               dword a = p>>24;
               if(a){
                  if(a==0xff)
                     *d_line = p;
                  else{
                     S_rgb_x p0, p1;
                     p0.From32bit(*d_line); p1.From32bit(p); p0.BlendWith(p1, a); *d_line = p0.To32bit();
                  }
               }
               ++d_line;
            }
         }
      }else{
         for(; sy--; src += src_pitch_y, dst += dst_pitch){
            const byte *s_line = src;
            const byte *a_line = alpha_src;
            word *d_line = (word*)dst;
            for(int x1=sx; x1--; ){
               dword a = *a_line;
               if(a){
                  if(a==0xff)
                     *d_line = *(word*)s_line;
                  else{
                     S_rgb_x s, p;
                     switch(bits_per_pixel){
#ifdef SUPPORT_12BIT_PIXEL
                     case 12: s.From12bit(*d_line); p.From12bit(*(word*)s_line); s.BlendWith(p, a); *d_line = s.To12bit(); break;
#endif
                     default: assert(0);
                     case 16: s.From16bit(*d_line); p.From16bit(*(word*)s_line); s.BlendWith(p, a); *d_line = s.To16bit(); break;
                     }
                  }
               }
               ++d_line;
               s_line += src_pitch_x;
               a_line += alpha_pitch_x;
            }
            alpha_src += alpha_pitch_y;
         }
      }
   }
}

//----------------------------

void C_image_imp::DrawSpecial(int dst_x, int dst_y, const S_rect *rc_src, C_fixed draw_alpha, dword mul_color) const{

   if(!rc_src && draw_alpha==C_fixed::One() && mul_color==0xffffffff){
                              //fast way, draw img directly
      Draw(dst_x, dst_y, 0);
      return;
   }
   if(rc_src){
      assert(rc_src->x>=0);
      assert(rc_src->y>=0);
      assert(rc_src->Right()<=size.x);
      assert(rc_src->Bottom()<=size.y);
   }

   const S_rect &clip_rect = app.GetClipRect();
   dword src_pitch = GetPitch(), dst_pitch = app.GetScreenPitch(), src_pitch_a = GetAlphaPitch();
   int sx = SizeX(), sy = SizeY();
   int src_x = 0, src_y = 0;
   if(rc_src){
      sx = rc_src->sx;
      sy = rc_src->sy;
      src_x = rc_src->x;
      src_y = rc_src->y;
   }
   if(dst_x < clip_rect.x){
      int d = clip_rect.x - dst_x;
      src_x += d;
      sx -= d;
      dst_x = clip_rect.x;
   }
   if(dst_y < clip_rect.y){
      int d = clip_rect.y - dst_y;
      src_y += d;
      sy -= d;
      dst_y = clip_rect.y;
   }
   if(dst_x+sx > clip_rect.Right())
      sx = clip_rect.Right()-dst_x;
   if(dst_y+sy > clip_rect.Bottom())
      sy = clip_rect.Bottom()-dst_y;
   if(sx<=0 || sy<=0)
      return;

   const S_pixelformat &pf = app.GetPixelFormat();
   const bool diff_pixel_depth = pf.bits_per_pixel!=bits_per_pixel;

   byte *dst = (byte*)app.GetBackBuffer();
   dst += dst_y*dst_pitch + dst_x*pf.bytes_per_pixel;
   const byte *src = bitmap + src_x*bytes_per_pixel + src_y*src_pitch;
   const byte *src_a = alpha;
   if(alpha)
      src_a += src_x + src_y*src_pitch_a;

   S_rgb_x mul_col; mul_col.From32bit(mul_color);
   mul_color &= 0xffffff;
   if(mul_col.a!=0xff){
      draw_alpha.val = (draw_alpha.val*mul_col.a)>>8;
   }

   if(diff_pixel_depth){
      int dither_y = dst_y & DITHER_MASK;
      for(int y=0; y<sy; y++, src+=src_pitch, src_a+=src_pitch_a, dst+=dst_pitch, ++dither_y &= DITHER_MASK){
         const byte *dither_line = dither_tab+dither_y*DITHER_SIZE + (dst_x & DITHER_MASK);
         for(int x=0; x<sx; x++){
            S_rgb_x s, b;
            b.From16bit(((word*)dst)[x]);
            s.From32bit(((dword*)src)[x]);
            if(mul_color!=0xffffff)
               s.Mul(mul_col);
            b.BlendWith(s, (s.a*draw_alpha.val)>>16);
            Add16BitPixelDitherVal(b, *dither_line);
            if(!(dword(++dither_line)&3))
               dither_line -= 4;
            ((word*)dst)[x] = b.To16bit();
         }
      }
   }else
   for(int y=0; y<sy; y++, src+=src_pitch, src_a+=src_pitch_a, dst+=dst_pitch){
      if(draw_alpha==C_fixed::One()){
         for(int x=0; x<sx; x++){
            S_rgb_x s, b;
            switch(bits_per_pixel){
#ifdef SUPPORT_12BIT_PIXEL
            case 12: b.From12bit(((word*)dst)[x]);  s.From12bit(((word*)src)[x]);  if(mul_color!=0xffffff) s.Mul(mul_col); b.BlendWith(s, src_a[x]); ((word*)dst)[x] = b.To12bit(); break;
#endif
            case 16:
               b.From16bit(((word*)dst)[x]);
               s.From16bit(((word*)src)[x]);
               if(mul_color!=0xffffff)
                  s.Mul(mul_col);
               b.BlendWith(s, src_a[x]);
               ((word*)dst)[x] = b.To16bit();
               break;
            default:
               b.From32bit(((dword*)dst)[x]);
               s.From32bit(((dword*)src)[x]);
               if(mul_color!=0xffffff)
                  s.Mul(mul_col);
               b.BlendWith(s, s.a);
               ((dword*)dst)[x] = b.To32bit();
               break;
            }
         }
      }else{
         for(int x=0; x<sx; x++){
            S_rgb_x s, b;
            switch(bits_per_pixel){
#ifdef SUPPORT_12BIT_PIXEL
            case 12: b.From12bit(((word*)dst)[x]);  s.From12bit(((word*)src)[x]);  if(mul_color!=0xffffff) s.Mul(mul_col); b.BlendWith(s, (src_a[x]*draw_alpha.val)>>16); ((word*)dst)[x] = b.To12bit(); break;
#endif
            case 16:
               b.From16bit(((word*)dst)[x]);
               s.From16bit(((word*)src)[x]);
               if(mul_color!=0xffffff)
                  s.Mul(mul_col);
               b.BlendWith(s, (src_a[x]*draw_alpha.val)>>16);
               ((word*)dst)[x] = b.To16bit();
               break;
            default:
               b.From32bit(((dword*)dst)[x]);
               s.From32bit(((dword*)src)[x]);
               if(mul_color!=0xffffff)
                  s.Mul(mul_col);
               b.BlendWith(s, (s.a*draw_alpha.val)>>16);
               ((dword*)dst)[x] = b.To32bit();
               break;
            }
         }
      }
   }
}

//----------------------------

void C_image::ComputeZoomedImageSize(int img_sx, int img_sy, int &fit_sx, int &fit_sy){

   if(!img_sx || !img_sy){
      fit_sx = fit_sy = 0;
      return;
   }
                              //ratio of width to height (to keep the aspect ratio)
   C_fixed ratio_w = C_fixed(fit_sx)/C_fixed(img_sx);
   C_fixed ratio_h = C_fixed(fit_sy)/C_fixed(img_sy);

                              //fit the width/height of image to fit into (image_width x image_height) space
   if(ratio_h > ratio_w){
      fit_sx = C_fixed(img_sx)*ratio_w;
      fit_sy = C_fixed(img_sy)*ratio_w;
   }else{
      fit_sx = C_fixed(img_sx)*ratio_h;
      fit_sy = C_fixed(img_sy)*ratio_h;
   }
   if(!fit_sx)
      fit_sx = 1;
   if(!fit_sy)
      fit_sy = 1;
}

//----------------------------

//#define DEBUG_NO_DITHER
//----------------------------

void C_image_imp::DrawZoomed(const S_rect &_rc_fit, bool keep_aspect_ratio) const{

   if(_rc_fit.IsEmpty())
      return;

   const byte *src = GetData();
   if(!src)
      return;

   S_rect rc_fit = _rc_fit;
   //DrawOutline(rc_fit, 0x80800000, 0x80800000); //!!!

   const int img_sx = SizeX(), img_sy = SizeY();
   if(keep_aspect_ratio)
      C_image::ComputeZoomedImageSize(img_sx, img_sy, rc_fit.sx, rc_fit.sy);

   if(rc_fit.sx==img_sx && rc_fit.sy==img_sy){
                              //no resizing, draw directly
      Draw(rc_fit.x, rc_fit.y);
      return;
   }

   const int FRAC = 11;
   const int FIXED_ONE = 1<<FRAC;
                              //compute steps; if there's no scaling in some dimension, use 1.0 step
   int step_x, step_y;
   if(img_sx==rc_fit.sx)
      step_x = FIXED_ONE;
   else
      step_x = (C_fixed(img_sx-1) / C_fixed(Min(rc_fit.sx, 0x7fff))).val>>(16-FRAC);;
   if(img_sy==rc_fit.sy)
      step_y = FIXED_ONE;
   else
      step_y = (C_fixed(img_sy-1) / C_fixed(Min(rc_fit.sy, 0x7fff))).val>>(16-FRAC);;

   //FillRect(rc_fit, 0x80808080); //!!!
   //img->Draw(rc_fit.x, rc_fit.y);   //!!!

   const dword src_pitch = GetPitch();
   const S_pixelformat &pf = app.GetPixelFormat();
   int beg_pos_x = 0, pos_y = 0;
   const byte *src_end = src + src_pitch*img_sy;

   const byte *src_alpha = NULL, *alpha_end = NULL;
   dword alpha_pitch = 0;
   if(has_alpha && bits_per_pixel<32){
      src_alpha = alpha;
      alpha_pitch = GetAlphaPitch();
      alpha_end = src_alpha+alpha_pitch*img_sy;
   }

                              //clip now
   const S_rect &clip = app.GetClipRect();
   if(rc_fit.x<clip.x){
      int d = clip.x-rc_fit.x;
      if((rc_fit.sx -= d) <= 0)
         return;
      rc_fit.x = clip.x;
      beg_pos_x += d*step_x;
      int offs = beg_pos_x>>FRAC;
      src += bytes_per_pixel * offs;
      if(src_alpha)
         src_alpha += offs;
      beg_pos_x &= (1<<FRAC)-1;
   }
   if(rc_fit.y<clip.y){
      int d = clip.y-rc_fit.y;
      if((rc_fit.sy -= d) <= 0)
         return;
      rc_fit.y = clip.y;
      pos_y += d*step_y;
      int offs = pos_y>>FRAC;
      src += src_pitch * offs;
      if(src_alpha)
         src_alpha += alpha_pitch * offs;
      pos_y &= (1<<FRAC)-1;
   }
   if(rc_fit.Right()>clip.Right()){
      rc_fit.sx -= rc_fit.Right()-clip.Right();
      if(rc_fit.sx<=0)
         return;
   }
   if(rc_fit.Bottom()>clip.Bottom()){
      rc_fit.sy -= rc_fit.Bottom()-clip.Bottom();
      if(rc_fit.sy<=0)
         return;
   }
   const dword dst_pitch = app.GetScreenPitch();
   byte *dst = (byte*)app.GetBackBuffer() + rc_fit.x*pf.bytes_per_pixel + rc_fit.y*dst_pitch;
   const bool diff_pixel_depth = pf.bits_per_pixel!=bits_per_pixel;

   static const byte dither_tab_2[DITHER_SIZE*DITHER_SIZE+4] = {
      0, 8, 3, 11,
      12, 4, 15, 7,
      2, 10, 1, 9,
      14, 6, 13, 5,
      0, 8, 3, 11,            //it overflows on S60 2nd ed compiler, need to repeat this here?!
   };
   int dither_y = rc_fit.y & DITHER_MASK;

   for(int y=rc_fit.sy; y--; ){
      int pos_x = beg_pos_x;
                              //draw line (optimized to pixel format)
      if(diff_pixel_depth){
         const dword *src_top = (dword*)src, *src_bot = (dword*)(src+src_pitch);
         if((byte*)src_bot>=src_end)
            src_bot = src_top;
         word *dst_line = (word*)dst;
         const word *eol = dst_line + rc_fit.sx;
         const int inv_y = FIXED_ONE - pos_y;

         if(has_alpha){
            const byte *dither_line = dither_tab+dither_y*DITHER_SIZE + (rc_fit.x & DITHER_MASK);
            do{
                                 //make final pixel filtered from 4 neighbour pixels
               dword p00 = src_top[0], p01 = src_top[1],
                  p10 = src_bot[0], p11 = src_bot[1];
               const int inv_x = FIXED_ONE - pos_x;

               const dword at = (((p00>>24)&0xff)*inv_x + ((p01>>24)&0xff)*pos_x);
               const dword ab = (((p10>>24)&0xff)*inv_x + ((p11>>24)&0xff)*pos_x);
               dword a = (at*inv_y + ab*pos_y)>>(FRAC*2);
               if(a){
                  dword rt = (((p00>>16)&0xff)*inv_x + ((p01>>16)&0xff)*pos_x);
                  dword rb = (((p10>>16)&0xff)*inv_x + ((p11>>16)&0xff)*pos_x);
                  S_rgb_x d;
                  d.r = byte((rt*inv_y + rb*pos_y)>>(FRAC*2));
                  dword gt = (((p00>>8)&0xff)*inv_x + ((p01>>8)&0xff)*pos_x);
                  dword gb = (((p10>>8)&0xff)*inv_x + ((p11>>8)&0xff)*pos_x);
                  d.g = byte((gt*inv_y + gb*pos_y)>>(FRAC*2));

                  dword bt = ((p00&0xff)*inv_x + (p01&0xff)*pos_x);
                  dword bb = ((p10&0xff)*inv_x + (p11&0xff)*pos_x);
                  d.b = byte((bt*inv_y + bb*pos_y)>>(FRAC*2));
                  if(a!=255){
                     S_rgb_x dd;
                     dd.From16bit(*dst_line);
                     d.BlendWith(dd, 255-a);
                  }
                  Add16BitPixelDitherVal(d, *dither_line);
                  *dst_line = d.To16bit();
               }
                                 //next pixel
               if(!(dword(++dither_line)&3))
                  dither_line -= 4;
               pos_x += step_x;
               int shift = pos_x>>FRAC;
               src_top += shift;
               src_bot += shift;
               pos_x &= (1<<FRAC)-1;
            }while(++dst_line!=eol);
         }else{
            const byte *dither_line_2 = dither_tab_2+dither_y*DITHER_SIZE + (rc_fit.x & DITHER_MASK);
#ifdef USE_ARM_ASM
            Line16bit(dst_line, src_top, src_bot, pos_x, step_x, eol, dither_line, pos_y, inv_y);
#else
            do{
                              //make final pixel filtered from 4 neighbour pixels
               dword p00 = src_top[0], p01 = src_top[1],
                  p10 = src_bot[0], p11 = src_bot[1];
               const int inv_x = FIXED_ONE - pos_x;

               dword dither_val = *dither_line_2 << (FRAC*2-1);
#ifdef DEBUG_NO_DITHER
               dither_val = 0;
#endif
               if(!(dword(++dither_line_2)&3))
                  dither_line_2 -= 4;
               dword rt = (((p00>>16)&0xff)*inv_x + ((p01>>16)&0xff)*pos_x);
               dword rb = (((p10>>16)&0xff)*inv_x + ((p11>>16)&0xff)*pos_x);
               dword d = min(0xffff, ((rt*inv_y + rb*pos_y
                  + dither_val
                  )>>(FRAC*2-8))) & 0xf800;

               dword gt = (((p00>>8)&0xff)*inv_x + ((p01>>8)&0xff)*pos_x);
               dword gb = (((p10>>8)&0xff)*inv_x + ((p11>>8)&0xff)*pos_x);
               d |= min(0x7ff, ((gt*inv_y + gb*pos_y
                  + (dither_val>>1)
                  )>>(FRAC*2-3))) & 0x7e0;

               dword bt = ((p00&0xff)*inv_x + (p01&0xff)*pos_x);
               dword bb = ((p10&0xff)*inv_x + (p11&0xff)*pos_x);
               d |= min(0x1f, ((bt*inv_y + bb*pos_y
                  + dither_val
                  )>>(FRAC*2+3)));
               *dst_line++ = word(d);

                                 //next pixel
               pos_x += step_x;
               int shift = pos_x>>FRAC;
               src_top += shift;
               src_bot += shift;
               pos_x &= (1<<FRAC)-1;
            }while(dst_line!=eol);
#endif
         }
         ++dither_y &= DITHER_MASK;
      }else
      if(pf.bits_per_pixel==16){
         const word *src_top = (word*)src, *src_bot = (word*)(src+src_pitch);
         if((byte*)src_bot>=src_end)
            src_bot = src_top;
         word *dst_line = (word*)dst;
         const word *eol = dst_line + rc_fit.sx;
         const int inv_y = FIXED_ONE - pos_y;

         if(has_alpha){
            const byte *dither_line = dither_tab+dither_y*DITHER_SIZE + (rc_fit.x & DITHER_MASK);
            const byte *alpha_top = src_alpha, *alpha_bot = src_alpha+alpha_pitch;
            if(alpha_bot>=alpha_end)
               alpha_bot = alpha_top;

            do{
                                 //make final pixel filtered from 4 neighbour pixels
               const int inv_x = FIXED_ONE - pos_x;

               const dword at = (alpha_top[0]*inv_x + alpha_top[1]*pos_x);
               const dword ab = (alpha_bot[0]*inv_x + alpha_bot[1]*pos_x);
               dword a = (at*inv_y + ab*pos_y)>>(FRAC*2);

               if(a){
                  word p00 = src_top[0], p01 = src_top[1],
                     p10 = src_bot[0], p11 = src_bot[1];

                  dword rt = (((p00>>8)&0xf8)*inv_x + ((p01>>8)&0xf8)*pos_x);
                  dword rb = (((p10>>8)&0xf8)*inv_x + ((p11>>8)&0xf8)*pos_x);
                  S_rgb_x d;
                  d.r = byte((rt*inv_y + rb*pos_y)>>(FRAC*2));

                  dword gt = (((p00>>3)&0xfc)*inv_x + ((p01>>3)&0xfc)*pos_x);
                  dword gb = (((p10>>3)&0xfc)*inv_x + ((p11>>3)&0xfc)*pos_x);
                  d.g = byte((gt*inv_y + gb*pos_y)>>(FRAC*2));

                  dword bt = (((p00<<3)&0xf8)*inv_x + ((p01<<3)&0xf8)*pos_x);
                  dword bb = (((p10<<3)&0xf8)*inv_x + ((p11<<3)&0xf8)*pos_x);
                  d.b = byte((bt*inv_y + bb*pos_y)>>(FRAC*2));

                  if(a!=255){
                     S_rgb_x dd;
                     dd.From16bit(*dst_line);
                     d.BlendWith(dd, 255-a);
                  }
                  Add16BitPixelDitherVal(d, *dither_line);
                  *dst_line = d.To16bit();
               }
                                 //next pixel
               if(!(dword(++dither_line)&3))
                  dither_line -= 4;
               pos_x += step_x;
               int shift = pos_x>>FRAC;
               src_top += shift;
               src_bot += shift;
               alpha_top += shift;
               alpha_bot += shift;
               pos_x &= (1<<FRAC)-1;
            }while(++dst_line!=eol);
         }else{
            const byte *dither_line_2 = dither_tab_2+dither_y*DITHER_SIZE + (rc_fit.x & DITHER_MASK);
#ifdef USE_ARM_ASM
            Line16bit(dst_line, src_top, src_bot, pos_x, step_x, eol, dither_line, pos_y, inv_y);
#else
            do{
                                 //make final pixel filtered from 4 neighbour pixels
               word p00 = src_top[0], p01 = src_top[1],
                  p10 = src_bot[0], p11 = src_bot[1];
               const int inv_x = FIXED_ONE - pos_x;

               dword dither_val = *dither_line_2 << (FRAC*2-1);
#ifdef DEBUG_NO_DITHER
               dither_val = 0;
#endif
               if(!(dword(++dither_line_2)&3))
                  dither_line_2 -= 4;
               dword rt = (((p00>>8)&0xf8)*inv_x + ((p01>>8)&0xf8)*pos_x);
               dword rb = (((p10>>8)&0xf8)*inv_x + ((p11>>8)&0xf8)*pos_x);
               dword d = ((rt*inv_y + rb*pos_y
                  + dither_val
                  )>>(FRAC*2-8)) & 0xf800;

               dword gt = (((p00>>3)&0xfc)*inv_x + ((p01>>3)&0xfc)*pos_x);
               dword gb = (((p10>>3)&0xfc)*inv_x + ((p11>>3)&0xfc)*pos_x);
               d |= ((gt*inv_y + gb*pos_y
                  + (dither_val>>1)
                  )>>(FRAC*2-3)) & 0x7e0;

               dword bt = (((p00<<3)&0xf8)*inv_x + ((p01<<3)&0xf8)*pos_x);
               dword bb = (((p10<<3)&0xf8)*inv_x + ((p11<<3)&0xf8)*pos_x);
               d |= ((bt*inv_y + bb*pos_y
                  + dither_val
                  )>>(FRAC*2+3));
               *dst_line++ = word(d);

                                 //next pixel
               pos_x += step_x;
               int shift = pos_x>>FRAC;
               src_top += shift;
               src_bot += shift;
               pos_x &= (1<<FRAC)-1;
            }while(dst_line!=eol);
#endif
         }
         ++dither_y &= DITHER_MASK;
      }else{
                              //32-bit backbuffer
         const dword *src_top = (dword*)src, *src_bot = (dword*)(src+src_pitch);
         if((byte*)src_bot>=src_end)
            src_bot = src_top;

         dword *dst_line = (dword*)dst;
         const dword *eol = dst_line + rc_fit.sx;
         const int inv_y = FIXED_ONE - pos_y;
         if(has_alpha){
            do{
                                 //make final pixel filtered from 4 neighbour pixels
               dword p00 = src_top[0], p01 = src_top[1],
                  p10 = src_bot[0], p11 = src_bot[1];
               const int inv_x = FIXED_ONE - pos_x;

               const dword at = (((p00>>24)&0xff)*inv_x + ((p01>>24)&0xff)*pos_x);
               const dword ab = (((p10>>24)&0xff)*inv_x + ((p11>>24)&0xff)*pos_x);
               dword a = (at*inv_y + ab*pos_y)>>(FRAC*2);
               if(a){
                  dword rt = (((p00>>16)&0xff)*inv_x + ((p01>>16)&0xff)*pos_x);
                  dword rb = (((p10>>16)&0xff)*inv_x + ((p11>>16)&0xff)*pos_x);
                  dword d = ((rt*inv_y + rb*pos_y)>>(FRAC*2-16)) & 0xff0000;

                  dword gt = (((p00>>8)&0xff)*inv_x + ((p01>>8)&0xff)*pos_x);
                  dword gb = (((p10>>8)&0xff)*inv_x + ((p11>>8)&0xff)*pos_x);
                  d |= ((gt*inv_y + gb*pos_y)>>(FRAC*2-8)) & 0xff00;

                  dword bt = ((p00&0xff)*inv_x + (p01&0xff)*pos_x);
                  dword bb = ((p10&0xff)*inv_x + (p11&0xff)*pos_x);
                  d |= ((bt*inv_y + bb*pos_y)>>(FRAC*2));
                  if(a!=255){
                     S_rgb_x dd(*dst_line);
                     dd.BlendWith(S_rgb_x(d), a);
                     d = dd.To32bit();
                  }
                  *dst_line = d;
               }
                                 //next pixel
               pos_x += step_x;
               int shift = pos_x>>FRAC;
               src_top += shift;
               src_bot += shift;
               pos_x &= (1<<FRAC)-1;
            }while(++dst_line!=eol);
         }else
         do{
                              //make final pixel filtered from 4 neighbour pixels
            dword p00 = src_top[0], p01 = src_top[1],
               p10 = src_bot[0], p11 = src_bot[1];
            const int inv_x = FIXED_ONE - pos_x;

            dword rt = (((p00>>16)&0xff)*inv_x + ((p01>>16)&0xff)*pos_x);
            dword rb = (((p10>>16)&0xff)*inv_x + ((p11>>16)&0xff)*pos_x);
            dword d = ((rt*inv_y + rb*pos_y)>>(FRAC*2-16)) & 0xff0000;

            dword gt = (((p00>>8)&0xff)*inv_x + ((p01>>8)&0xff)*pos_x);
            dword gb = (((p10>>8)&0xff)*inv_x + ((p11>>8)&0xff)*pos_x);
            d |= ((gt*inv_y + gb*pos_y)>>(FRAC*2-8)) & 0xff00;

            dword bt = ((p00&0xff)*inv_x + (p01&0xff)*pos_x);
            dword bb = ((p10&0xff)*inv_x + (p11&0xff)*pos_x);
            d |= ((bt*inv_y + bb*pos_y)>>(FRAC*2));
            *dst_line++ = d;

                              //next pixel
            pos_x += step_x;
            int shift = pos_x>>FRAC;
            src_top += shift;
            src_bot += shift;
            pos_x &= (1<<FRAC)-1;
         }while(dst_line!=eol);
      }
                              //next line
      dst += dst_pitch;
      pos_y += step_y;
      int shift = (pos_y>>FRAC);
      src += src_pitch * shift;
      if(src>=src_end)
         src = src_end;
      if(src_alpha && (src_alpha += alpha_pitch*shift)>alpha_end)
         src_alpha = alpha_end;
      pos_y &= (1<<FRAC)-1;
   }
#ifdef _DEBUG
   //FillRect(S_rect(rc_fit.x+2, rc_fit.y+2, 10, 10), 0xff00ff00);
#endif
}

//----------------------------

void C_image_imp::DrawNinePatch(const S_rect &rc, const S_point &_corner) const{

   C_clip_rect_helper crh(const_cast<C_application_base&>(app));

   S_point corner(Min(_corner.x, rc.sx/2), Min(_corner.y, rc.sy/2));

   S_rect rc_in = rc;
   rc_in.x += corner.x;
   rc_in.sx -= corner.x*2;
   rc_in.y += corner.y;
   rc_in.sy -= corner.y*2;
   if(rc_in.sx<0 || rc_in.sy<0){
                              //badly defined corner
      assert(0);
      return;
   }
                              //draw borders
   //*
   {
      S_rect rc1(rc.x, rc.y, corner.x, corner.y);

      if(crh.CombineClipRect(rc1))
         Draw(rc1.x, rc1.y);

      rc1.x = rc.Right()-corner.x;
      if(crh.CombineClipRect(rc1))
         Draw(rc1.Right()-SizeX(), rc1.y);

      rc1.x = rc.x; rc1.y = rc.Bottom()-corner.y;

      if(crh.CombineClipRect(rc1))
         Draw(rc1.x, rc1.Bottom()-SizeY());

      rc1.x = rc.Right()-corner.x;
      if(crh.CombineClipRect(rc1))
         Draw(rc1.Right()-SizeX(), rc1.Bottom()-SizeY());
   }
   /**/
   int offs_x, offs_y;
   const int SRC_SIZE_SUB = 4;   //compute with 4 pixels less from original img size, so that stretched part has edge blended to center of pixel of adjacent border part
   {
                              //top/bottom
      S_rect rcd(rc.x+corner.x, rc.y, rc.sx-corner.x*2, corner.y);
      offs_x = Max(1, corner.x*2*rcd.sx/Max(1, SRC_SIZE_SUB+size.x-corner.x*2));
      //*
      S_rect rc_z(rcd.x - (offs_x+1)/2, rcd.y, rcd.sx + offs_x, size.y);

      //crh.Restore(); const_cast<C_application_base&>(app).DrawOutline(rc_z, 0x80ff0000);

      if(crh.CombineClipRect(rcd))
         DrawZoomed(rc_z);

      rcd.y = rc.Bottom()-corner.y;
      rc_z.y = rcd.Bottom()-rc_z.sy;
      if(crh.CombineClipRect(rcd))
         DrawZoomed(rc_z);
      /**/
   }
   {
                              //left/right
      S_rect rcd(rc.x, rc.y+corner.y, corner.x, rc.sy-corner.y*2);
      offs_y = Max(1, corner.y*2*rcd.sy/Max(1, SRC_SIZE_SUB+size.y-corner.y*2));
      //*
      S_rect rc_z(rcd.x, rcd.y - (offs_y+1)/2, size.x, rcd.sy + offs_y);

      //crh.Restore(); const_cast<C_application_base&>(app).DrawOutline(rc_z, 0x80ff0000);
      //crh.Restore(); const_cast<C_application_base&>(app).DrawOutline(rcd, 0x80ff0000);

      if(crh.CombineClipRect(rcd))
         DrawZoomed(rc_z);

      rcd.x = rc.Right()-corner.x;
      rc_z.x = rcd.Right()-rc_z.sx;
      if(crh.CombineClipRect(rcd))
         DrawZoomed(rc_z);
      /**/
   }
   //*
   {
                              //center
      S_rect rcd(rc.x+corner.x, rc.y+corner.y, rc.sx-corner.x*2, rc.sy-corner.y*2);

      S_rect rc_z(rcd.x - (offs_x+1)/2, rcd.y - (offs_y+1)/2, rcd.sx + offs_x, rcd.sy + offs_y);

      //crh.Restore(); const_cast<C_application_base&>(app).DrawOutline(rc_z, 0x80ff0000);
      if(crh.CombineClipRect(rcd))
         DrawZoomed(rc_z);
   }
   /**/
}

//----------------------------
// Shrink 2 lines to one, using algorithm that blends 2x2 pixels to one.
static void ShrinkLinesToHalf(const C_image_loader::S_rgb *l0, const C_image_loader::S_rgb *l1, C_image_loader::S_rgb *dst, int num_src_pixels, bool use_alpha){

   PROF_S(PROF_HALVE);
   const C_image_loader::S_rgb *eol = dst+num_src_pixels/2;
                              //blend 2x2 pixel blocks to single pixels
   do{
      const C_image_loader::S_rgb &lt = *l0++;
      const C_image_loader::S_rgb &rt = *l0++;
      const C_image_loader::S_rgb &lb = *l1++;
      const C_image_loader::S_rgb &rb = *l1++;
      dword d = ((lt.r + rt.r + lb.r + rb.r + 2)<<14) & 0xff0000;
      d |= ((lt.g + rt.g + lb.g + rb.g + 2)<<6) & 0xff00;
      d |= (lt.b + rt.b + lb.b + rb.b + 2)>>2;
      if(use_alpha)
         d |= ((lt.a + rt.a + lb.a + rb.a + 2)<<22) & 0xff000000;
      (dword&)*dst++ = d;
   }while(dst!=eol);
                              //if odd number of pixels, blend last 2 pixels to one
   if(num_src_pixels&1){
      dst->r = byte((l0->r + l1->r + 1) / 2);
      dst->g = byte((l0->g + l1->g + 1) / 2);
      dst->b = byte((l0->b + l1->b + 1) / 2);
      if(use_alpha)
         dst->a = byte((l0->a + l1->a + 1) / 2);
   }
   PROF_E(PROF_HALVE);
}

//----------------------------
// Shrink horizontal line by given ratio (ratio is between 1.0 ... 2.0).
static void ShrinkHorizontalLine(const C_image_loader::S_rgb *src, int orig_size_x, C_image_loader::S_rgb *dst, int size_x, int ratio, int curr_ratio, bool use_alpha){

   PROF_S(PROF_SHRINK);
   assert(curr_ratio >= 0);
   assert(curr_ratio<0x10000);
   const C_image_loader::S_rgb *src_last = src+orig_size_x-1;
#if 1
   const C_image_loader::S_rgb *eol = dst+size_x;
   if(!use_alpha){
      do{
                                 //blend pixels by current ratio
         const C_image_loader::S_rgb *s_n = min(src+1, src_last);
         dword r_this = 0x10000 - curr_ratio;
                                 //make blended rgba
         dword d = (src->r*r_this + s_n->r*curr_ratio)&0xff0000;
         d |= ((src->g*r_this + s_n->g*curr_ratio)>>8)&0xff00;
         d |= (src->b*r_this + s_n->b*curr_ratio)>>16;
         (dword&)*dst++ = d;
                                 //advance source
         curr_ratio += ratio;
         src += curr_ratio>>16;
         curr_ratio &= 0xffff;
      }while(dst!=eol);
   }else{
      do{
                                 //blend pixels by current ratio
         const C_image_loader::S_rgb *s_n = min(src+1, src_last);
         dword r_this = 0x10000 - curr_ratio;
                                 //make blended rgba
         dword d = (src->r*r_this + s_n->r*curr_ratio)&0xff0000;
         d |= ((src->g*r_this + s_n->g*curr_ratio)>>8)&0xff00;
         d |= (src->b*r_this + s_n->b*curr_ratio)>>16;
         d |= ((src->a*r_this + s_n->a*curr_ratio)<<8)&0xff000000;
         (dword&)*dst++ = d;
                                 //advance source
         curr_ratio += ratio;
         src += curr_ratio>>16;
         curr_ratio &= 0xffff;
      }while(dst!=eol);
   }
#else
                              //compute all destination pixels
   for(int x=0; x<size_x; x++){
      //assert(src<=src_last);
                              //blend pixels by current ratio
      C_fixed r_this;
      const C_image_loader::S_rgb *s_n;
      if(curr_ratio < C_fixed::Zero()){
         r_this = C_fixed::One() + curr_ratio;
         s_n = src - 1;
      }else{
         assert(curr_ratio<=C_fixed::One());
         r_this = C_fixed::One() - curr_ratio;
         s_n = min(src+1, src_last);
      }
      C_fixed r_neighbour = C_fixed::One() - r_this;
      int r = C_fixed(src->r) * r_this + C_fixed(s_n->r) * r_neighbour;
      int g = C_fixed(src->g) * r_this + C_fixed(s_n->g) * r_neighbour;
      int b = C_fixed(src->b) * r_this + C_fixed(s_n->b) * r_neighbour;
      assert(r<256 && g<256 && b<256);
      dst->r = byte(r);
      dst->g = byte(g);
      dst->b = byte(b);
      if(use_alpha)
         dst->a = byte(C_fixed(src->a) * r_this + C_fixed(s_n->a) * r_neighbour);
                              //advance source
      curr_ratio += ratio;
      while(curr_ratio >= C_fixed::Half()){
         ++src;
         //src = min(src, src_last);
         curr_ratio -= C_fixed::One();
      }
      ++dst;
   }
#endif
   PROF_E(PROF_SHRINK);
}

//----------------------------

static void BlendVerticalLines(const C_image_loader::S_rgb *src0, const C_image_loader::S_rgb *src1, C_image_loader::S_rgb *dst, int size_x, dword ratio, bool use_alpha){

   PROF_S(PROF_BLEND);

   assert(ratio>=0 && ratio<=0x10000);
   dword r0 = 0x10000 - ratio;
   const C_image_loader::S_rgb *eol = dst+size_x;
   if(!use_alpha){
      do{
         dword d = (src0->r*r0 + src1->r*ratio)&0xff0000;
         d |= ((src0->g*r0 + src1->g*ratio)>>8)&0xff00;
         d |= ((src0++)->b*r0 + (src1++)->b*ratio)>>16;
         (dword&)*dst = d;
      }while(++dst!=eol);
   }else{
      do{
         dword d = (src0->r*r0 + src1->r*ratio)&0xff0000;
         d |= ((src0->g*r0 + src1->g*ratio)>>8)&0xff00;
         d |= (src0->b*r0 + src1->b*ratio)>>16;
         d |= (((src0++)->a*r0 + (src1++)->a*ratio)<<8)&0xff000000;
         (dword&)*dst = d;
      }while(++dst!=eol);
   }
   PROF_E(PROF_BLEND);
}

//----------------------------

#define DITHER_PATTERN

//----------------------------

static void DitherAndWriteLine(const C_image_loader::S_rgb *src, byte *dst, dword num_pixels,
#ifdef DITHER_PATTERN
   const byte *dither_pattern,
#else
   S_rgb_x *dither_line,
#endif
   dword bytes_per_pixel, dword bits_per_pixel, byte *alpha){

   PROF_S(PROF_WRITE);

   if(bytes_per_pixel==2){
      if(alpha)
         MemSet(alpha, 0, num_pixels);
#if !defined DEBUG_NO_DITHER && !defined DITHER_PATTERN
      const dword mask = bits_per_pixel==16 ? 0xf8fcf8 : 0xf0f0f0;
                              //x distribution error, starts with 0
      S_rgb_x error_x(0);
#endif
      while(num_pixels--){
         dword d;
#ifdef DITHER_PATTERN
         if(dither_pattern){
            byte dither_add = *dither_pattern++;
            if(!(dword(dither_pattern)&3))
               dither_pattern -= 4;
            //p.r = min(255, p.r+dither_add);
            //p.b = min(255, p.b+dither_add);
            //p.g = min(255, p.g+dither_add/2);
            d = min(255, src->r + dither_add) << 16;
            d |= min(255, src->b + dither_add);
            d |= min(255, src->g + dither_add/2) << 8;
         }else
#else
         if(dither_line){
                                 //get source pixel and add distributed error
            dword s = min(255, src->r + (dither_line->r + error_x.r) / 2) << 16;
            s |= min(255, src->g + (dither_line->g + error_x.g) / 2) << 8;
            s |= min(255, src->b + (dither_line->b + error_x.b) / 2);
                                 //make clipped destination rgb
            d = s & mask;
                                 //store difference error
            (dword&)*dither_line++ = (dword&)error_x = s - d;
         }else
#endif
            d = (dword&)*src;
                                 //store destination pixel
         *(word*)dst =
#ifdef SUPPORT_12BIT_PIXEL
            (pf.bits_per_pixel==12) ? ((S_rgb_x&)d).To12bit() :
#endif
            ((S_rgb_x&)d).To16bit();
         dst += 2;
         if(alpha)
            *alpha++ = src->a;
         ++src;
      }
   }else
      MemCpy(dst, src, num_pixels*4);
   PROF_E(PROF_WRITE);
}

//----------------------------

void C_image::ComputeSize(int &size_x, int &size_y, int limit_size_x, int limit_size_y, C_fixed max_ratio, C_fixed &ratio_x, C_fixed &ratio_y, bool keep_aspect_ratio){

   int orig_sx = size_x;
   int orig_sy = size_y;

   if(max_ratio!=C_fixed::One()){
      ratio_y = ratio_x = C_fixed::One() / max_ratio;
                           //rescale by ratio
      size_x = Max(1, int(max_ratio * C_fixed(orig_sx)));
      size_y = Max(1, int(max_ratio * C_fixed(orig_sy)));
   }else
      ratio_y = ratio_x = C_fixed::One();
   if(size_x > limit_size_x || size_y > limit_size_y){
      if(!limit_size_x || !limit_size_y){
         size_x = size_y = 0;
         return;
      }
                           //rescaling will occur, compute rescale ratio and destination size
      C_fixed rx = C_fixed(orig_sx) / C_fixed(limit_size_x);
      C_fixed ry = C_fixed(orig_sy) / C_fixed(limit_size_y);
      if(keep_aspect_ratio){
         if(rx > ry){
            C_fixed r = ry/rx;
            size_y = int(C_fixed(limit_size_y)*r + C_fixed::Half());
            size_y = Max(1, size_y);
            size_x = limit_size_x;
            ratio_x = rx;
            ratio_y = C_fixed(orig_sy) / C_fixed(size_y);
         }else{
            C_fixed r = rx/ry;
            size_x = int(C_fixed(limit_size_x)*r + C_fixed::Half());
            size_x = Max(1, size_x);
            size_y = limit_size_y;
            ratio_y = ry;
            ratio_x = C_fixed(orig_sx) / C_fixed(size_x);
            //ratio_x = ratio_y;
         }
      }else{
         if(size_x>limit_size_x){
            size_x = limit_size_x;
            ratio_x = rx;
         }
         if(size_y>limit_size_y){
            size_y = limit_size_y;
            ratio_y = ry;
         }
      }
      assert(size_x<=limit_size_x && size_y<=limit_size_y);
   }
}

//----------------------------

static void SwapLines(byte *s1, byte *s2, dword size_x){
   while(size_x--)
      Swap(*s1++, *s2++);
}

//----------------------------

bool C_image::C_image_loader_resizer::LoadAndDecodeImage(C_image_loader *loader, const S_point &dst_size, C_fixed rescale_ratio[2],
   C_open_progress *open_progress, S_point *special_open_draw_pos){

   const int ldr_sx = loader->GetSX(), ldr_sy = loader->GetSY();
   bool has_alpha = loader->IsTransparent();
                           //get number of shrink levels by scale 2
   int num_shrinks = 0;
   int last_level_sx = ldr_sx;
   while(rescale_ratio[0]>=C_fixed::Two() && rescale_ratio[1]>=C_fixed::Two()){
      ++num_shrinks;
      rescale_ratio[0] >>= 1;
      rescale_ratio[1] >>= 1;
      last_level_sx = (last_level_sx+1) / 2;
   }
                           //do rescale, dithering & conversion
   C_vector<C_image_loader::t_horizontal_line*> lines;
   lines.reserve(8);
   C_image_loader::t_horizontal_line last_line[2], curr_line;
   last_line[0].Resize(dst_size.x);
   last_line[1].Resize(dst_size.x);
   curr_line.Resize(dst_size.x);

   //byte *dst = bitmap;
   //byte *dst_alpha = alpha;

                              //begin at center of pixel
                              // curr_ratio specified offset from center of save line 0
                              // may be in range from 0 to 1
   C_fixed beg_ratio_x = (rescale_ratio[0]-C_fixed::One()) >> 1;
   beg_ratio_x = Min(Max(C_fixed::Zero(), beg_ratio_x), C_fixed::Create(0xffff));
   C_fixed curr_ratio_y = (rescale_ratio[1]-C_fixed::One()) >> 1;
   curr_ratio_y = Min(Max(C_fixed::Zero(), curr_ratio_y), C_fixed::One());
   if(special_open_draw_pos){
                              //compute position now, adjust beginning ratios
      int x = special_open_draw_pos->x;
      int y = special_open_draw_pos->y;
      special_open_draw_pos->x = 0;
      special_open_draw_pos->y = 0;
      while(x>0){
         beg_ratio_x += rescale_ratio[0];
         ++special_open_draw_pos->x;
         while(beg_ratio_x >= C_fixed::One()){
            --x;
            beg_ratio_x -= C_fixed::One();
         }
      }
      while(y>0){
         curr_ratio_y += rescale_ratio[1];
         ++special_open_draw_pos->y;
         while(curr_ratio_y >= C_fixed::One()){
            --y;
            curr_ratio_y -= C_fixed::One();
         }
      }
   }

   bool rescale_y = (rescale_ratio[1]!=C_fixed::One());

                              //initially, we need 2 lines
   int need_lines_to_create = rescale_y ? 2 : 1;

   int last_progress_time = open_progress ? GetTickTime() : 0;

   int dst_sy = dst_size.y;
   int src_sy = ldr_sy;
   bool ret = true;
   while(src_sy && dst_sy){
      if(num_shrinks){
                              //perform recursive line blending and shrinking
         struct S_level_info{
            int num_lines;
            int line_sx;
         } level_info[16];    //this makes max. possibility to shrink image with 2^16 lines to single line
         int curr_level = 0;
         {
            int line_sx = ldr_sx;
            for(int i=num_shrinks; i--; ){
               level_info[i].num_lines = 0;
               level_info[i].line_sx = line_sx;
               line_sx = (line_sx+1) / 2;
            }
            curr_level = num_shrinks-1;
         }
         int level_dst_line = 0;

         while(true){
            if(curr_level<num_shrinks-1){
                              //need to get lines from upper level
               if(level_info[curr_level].num_lines)
                  ++level_dst_line;
               level_info[++curr_level].num_lines = 0;
               continue;
            }
                              //it's top level, do actual line reading
                              //read 2 lines (or 1 if 2 are not available)
            //assert(src_sy);
            int want_lines = Max(1, Min(2, src_sy));
            while(lines.size()-level_dst_line < want_lines){
               if(src_sy){
                  PROF_S(PROF_LOAD);
                  if(!loader->ReadOneOrMoreLines(lines))
                     goto fail;
                  PROF_E(PROF_LOAD);
                  --src_sy;
               }else{
                  dword nl = lines.size();
                  assert(nl);
                              //reader has no more lines, copy previous line (rare condition)
                  C_image_loader::t_horizontal_line *line = new(true) C_image_loader::t_horizontal_line;
                  line->Resize(ldr_sx);
                  lines.push_back(line);
                  if(nl)
                     MemCpy(line->Begin(), lines[nl-1]->Begin(), ldr_sx*sizeof(S_rgb));
                  else
                     MemSet(line->Begin(), 0, ldr_sx*sizeof(S_rgb));
               }
            }
            while(true){
                           //blend pair of lines (make one), and delete second line
               int li1 = level_dst_line + 1;
               if(li1 >= lines.size())
                  --li1;
               ShrinkLinesToHalf(lines[level_dst_line]->Begin(), lines[li1]->Begin(), lines[level_dst_line]->Begin(), level_info[curr_level].line_sx, has_alpha);
               if(li1!=level_dst_line){
                  delete lines[li1];
                  lines.erase(&lines[li1]);
               }
               if(!curr_level)
                  goto levels_read;
                           //we made one line for upper level, inc counter
               --curr_level;
               if(++level_info[curr_level].num_lines<2)
                  break;
               --level_dst_line;
            }
         }
levels_read:{}
      }else{
                              //no square shrinking, simply read line
         PROF_S(PROF_LOAD);
         if(!loader->ReadOneOrMoreLines(lines))
            goto fail;
         PROF_E(PROF_LOAD);
         --src_sy;
      }
                              //call progress indicator, if it is time to do so
      if(open_progress){
                              //don't call too often
         int tc = GetTickTime();
         if(tc-last_progress_time >= 50){
            last_progress_time = tc;
            dword percent = 100 - (src_sy*100 / loader->GetSY());
            if(!open_progress->ImageLoadProgress(percent)){
               ret = false;
               goto fail;
            }
         }
      }
      last_line[0] = last_line[1];
                              //now we have single shrinked line in lines[0], rescale it by floating-point blending
      if(!rescale_y){
                              //just write the line now
         OutputLine((S_rgb*)lines[0]->Begin());
      }else
         ShrinkHorizontalLine(lines[0]->Begin(), last_level_sx, last_line[1].Begin(), dst_size.x, rescale_ratio[0].val, beg_ratio_x.val, has_alpha);

      delete lines[0];
      lines.erase(lines.begin());

      if(--need_lines_to_create)
         continue;

      if(!rescale_y){
         ++need_lines_to_create;
      }else{
                              //now we have lines which we're going to blend in last_lines[]
         const C_image_loader::S_rgb *src0 = last_line[0].Begin(), *src1 = last_line[1].Begin();
         /*
         if(bytes_per_pixel==4){
                              //write directly to dest 32-bit buffer
            BlendVerticalLines(src0, src1, (C_image_loader::S_rgb*)dst, dst_size.x, curr_ratio_y.val, has_alpha);
         }else*/
         {
                              //use temp buffer for conversion
            BlendVerticalLines(src0, src1, curr_line.Begin(), dst_size.x, curr_ratio_y.val, has_alpha);
                              //convert the line to image's pixel format and store to destination
            OutputLine((S_rgb*)curr_line.Begin());
         }
                              //advance source
         curr_ratio_y += rescale_ratio[1];
         while(curr_ratio_y >= C_fixed::One()){
            ++need_lines_to_create;
            curr_ratio_y -= C_fixed::One();
         }
      }
      assert(need_lines_to_create);
      assert(dst_sy);
      --dst_sy;
   }
   if(dst_sy){
                              //finish last dest line
      assert(dst_sy==1);
      OutputLine((S_rgb*)last_line[src_sy==1 ? 0 : 1].Begin());
   }
   //if(loader->IsAnimated())
   {
                              //need to read all lines
      while(src_sy){
         PROF_S(PROF_LOAD);
         if(!loader->ReadOneOrMoreLines(lines))
            break;
         PROF_E(PROF_LOAD);
         for(int i=lines.size(); i--; ){
            delete lines[i];
            assert(src_sy);
            --src_sy;
         }
         lines.clear();
      }
   }
fail:
                              //delete remaining lines
   while(lines.size()){
      delete lines.back();
      lines.pop_back();
   }
   return ret;
}

//----------------------------

class C_image_loader_resizer_imp: public C_image::C_image_loader_resizer{
   byte *dst;
   byte *dst_alpha;
   const S_point dst_size;
   dword bits_per_pixel;
   dword bytes_per_pixel;
#ifdef DITHER_PATTERN
   const byte *dither_pattern;
#else
   S_rgb_x *dither_line;      //NULL = no dithering
#endif

   virtual void OutputLine(const S_rgb *line){
      DitherAndWriteLine((const C_image_loader::S_rgb*)line, dst, dst_size.x,
#ifdef DITHER_PATTERN
         dither_pattern,
#else
         dither_line,
#endif
         bytes_per_pixel, bits_per_pixel, dst_alpha);
                              //advance destination
      dst += dst_size.x*bytes_per_pixel;
      if(dst_alpha)
         dst_alpha += dst_size.x;
#ifdef DITHER_PATTERN
      if(dither_pattern){
         dither_pattern += DITHER_SIZE;
         if(dither_pattern==dither_tab+DITHER_SIZE*DITHER_SIZE)
            dither_pattern = dither_tab;
      }
#endif
   }
public:
   C_image_loader_resizer_imp(byte *_dst, byte *_dst_alpha, const S_point &_dst_size, dword _bits_per_pixel, bool dither):
      dst(_dst),
      dst_alpha(_dst_alpha),
      dst_size(_dst_size),
      bits_per_pixel(_bits_per_pixel),
#ifdef DITHER_PATTERN
      dither_pattern(NULL),
#else
      dither_line(NULL),
#endif
      bytes_per_pixel((_bits_per_pixel+7)/8)
   {
      if(bytes_per_pixel==2 && dither){
#ifdef DITHER_PATTERN
         dither_pattern = dither_tab;
#else
         dither_line = new(true) S_rgb_x[dst_size.x];
         MemSet(dither_line, 0, sizeof(S_rgb_x)*dst_size.x);
#endif
      }
   }
   ~C_image_loader_resizer_imp(){
#ifndef DITHER_PATTERN
      delete[] dither_line;
#endif
   }
};

//----------------------------

bool C_image_imp::LoadAndDecodeImage(C_image_loader *loader, C_open_progress *open_progress, bool dither, S_point *special_open_draw_pos){

   C_fixed dec_rescale_ratio[2] = {
      rescale_ratio[0],
      rescale_ratio[1]
   };
   S_point dst_size(size.x, size.y);
   C_image_loader_resizer_imp resizer(bitmap, alpha, dst_size, bits_per_pixel, dither);
   bool ret = resizer.LoadAndDecodeImage(loader, dst_size, dec_rescale_ratio, open_progress, special_open_draw_pos);
   if(ret && loader->IsFlippedY()){
                              //flip image vertically
      assert(!alpha);
      for(int y=size.y/2; y--; )
         SwapLines(bitmap+y*size.x*bytes_per_pixel, bitmap+(size.y-1-y)*size.x*bytes_per_pixel, size.x*bytes_per_pixel);
   }
   return ret;
}

//----------------------------

C_image_imp::E_RESULT C_image_imp::OpenInternal(const wchar *filename, int limit_size_x, int limit_size_y, C_fixed max_ratio, const C_zip_package *arch,
   C_open_progress *open_progress, dword file_offs, C_file *own_file, dword flags, S_point *special_open_draw_pos){

   Close();

   if(!limit_size_x) limit_size_x = 0x7fff;
   if(!limit_size_y) limit_size_y = 0x7fff;

   C_file *fl = own_file;
   if(!fl){
      if(!arch){
         fl = new(true) C_file;
         if(!fl->Open(filename)){
            delete fl;
            return IMG_NO_FILE;
         }
      }else{
         fl = arch->OpenFile(filename);
         if(!fl)
            return IMG_NO_FILE;
      }
   }
   fl->Seek(file_offs);
   dword header;
   if(!fl->ReadDword(header)){
      delete fl;
      return IMG_NO_FILE;
   }
   if(limit_size_x<0 || limit_size_y<0){
      delete fl;
      return IMG_BAD_PARAMS;
   }

   C_image_loader *loader = NULL;

                              //try find working loader
   for(int li=0; ; li++){
      t_CreateLoader lc = image_loaders[li];
      if(!lc)
         break;
      loader = (*lc)(fl);
      fl->Seek(file_offs);
      if(loader->GetImageInfo(header, limit_size_x, limit_size_y, max_ratio))
         break;
      delete loader;
      loader = NULL;
   }
   if(!loader){
      delete fl;
      return IMG_NO_LOADER;
   }
   if(!loader->InitDecoder()){
      delete loader;
      delete fl;
      return IMG_NOT_ENOUGH_MEMORY;
   }

   orig_size.x = loader->GetOriginalSX(), orig_size.y = loader->GetOriginalSY();
   if(special_open_draw_pos){
      size.x = Max(1, int(C_fixed(orig_size.x)/rescale_ratio[0]));
      size.y = Max(1, int(C_fixed(orig_size.y)/rescale_ratio[1]));
   }else{
      int ldr_sx = loader->GetSX(), ldr_sy = loader->GetSY();
      size.x = ldr_sx, size.y = ldr_sy;
      ComputeSize((int&)size.x, (int&)size.y, limit_size_x, limit_size_y, C_fixed::One(), rescale_ratio[0], rescale_ratio[1], !(flags&IMAGE_OPEN_NO_ASPECT_RATIO));

      if(max_ratio!=C_fixed::One()){
         dword osx = loader->GetOriginalSX(), osy = loader->GetOriginalSY();
                              //rescale by ratio
         int sx = Max(1, int(max_ratio * C_fixed(osx)));
         int sy = Max(1, int(max_ratio * C_fixed(osy)));
         if(sx<size.x || sy<size.y){
            size.x = sx;
            size.y = sy;
            rescale_ratio[0] = C_fixed(ldr_sx)/C_fixed(sx);
            rescale_ratio[1] = C_fixed(ldr_sy)/C_fixed(sy);
         }
      }
   }
   if(!size.x || !size.y){
      delete fl;
      delete loader;
      return IMG_OK;
   }
                           //alloc destination buffer
   if(flags&IMAGE_OPEN_FORCE_32BIT_PIXEL){
      bytes_per_pixel = 4;
      bits_per_pixel = 32;
   }else{
      const S_pixelformat &pf = app.GetPixelFormat();
      bytes_per_pixel = pf.bytes_per_pixel;
      bits_per_pixel = pf.bits_per_pixel;
   }
   bitmap = new byte[size.x * size.y * bytes_per_pixel];
   if(!bitmap){
      delete fl;
      delete loader;
      return IMG_NOT_ENOUGH_MEMORY;
   }

   if(loader->IsTransparent()){
      has_alpha = true;
      if(bytes_per_pixel==2){
         alpha = new byte[size.x * size.y];
         if(!alpha){
            delete[] bitmap; bitmap = NULL;
            delete fl;
            delete loader;
            return IMG_NOT_ENOUGH_MEMORY;
         }
      }
   }
   E_RESULT ret = IMG_OK;
   PROF_S(PROF_ALL);
   if(!LoadAndDecodeImage(loader, open_progress, !(flags&IMAGE_OPEN_NO_DITHERING), special_open_draw_pos))
      ret = IMG_CANCELED;
   PROF_E(PROF_ALL);

   //orig_sx = loader->GetOriginalSX(), orig_sy = loader->GetOriginalSY();
   if(loader->IsAnimated() && !(flags&IMAGE_OPEN_NO_ANIMATIONS)){
      loader->SetFileOwnership();
      anim_loader = loader;
#ifdef _DEBUG
      //Tick(100);
#endif
   }else
   {
      delete loader;
      delete fl;
   }
   return ret;
}

//----------------------------

bool C_image_imp::Tick(int time){

   if(!anim_loader)
      return false;
   if(!anim_loader->Tick(time))
      return false;

   LoadAndDecodeImage(anim_loader, NULL, true);
   return true;
}

//----------------------------
//----------------------------

C_image *C_image::Create(const C_application_base &app){
   return new C_image_imp(app);
}

//----------------------------
//----------------------------

#ifdef __SYMBIAN32__
#include <E32Std.h>
#endif
#include <ImgLib.h>
#include "png.h"

//----------------------------

class C_loader_png: public C_image_loader{

   png_struct *png_ptr;
   png_info *info_ptr;

   byte *interlaced_cache;
   dword interlaced_read_line;

   static void ReadFromCache(png_struct *png_ptr, png_bytep buf, png_size_t size){
      ((C_file*)png_get_io_ptr(png_ptr))->Read(buf, size);
   }

//----------------------------

   static void PNGAPI PNG_Error(png_struct *png_ptr, png_const_charp err_msg){
      //Cstr_w s(err_msg);
      //throw(s);
   }

//----------------------------

   virtual bool IsTransparent() const{
      //return (bpp==4);
      return (info_ptr->color_type&PNG_COLOR_MASK_ALPHA);
   }

//----------------------------

   virtual bool GetImageInfo(dword header, int limit_size_x, int limit_size_y, C_fixed limit_ratio){

      if(!png_check_sig((byte*)&header, 4))
         return false;
      png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, this, PNG_Error, PNG_Error);
      if(!png_ptr)
         return false;
      //E_LOADER_RESULT ret = IMGLOAD_READ_ERROR;
      info_ptr = png_create_info_struct(png_ptr);
      if(!info_ptr)
         return false;

      png_set_bgr(png_ptr);
      png_set_expand(png_ptr);
      png_set_palette_to_rgb(png_ptr);
      png_set_strip_16(png_ptr);

      png_set_gray_to_rgb(png_ptr);
      png_set_read_fn(png_ptr, (void*)fl, &ReadFromCache);
      if(!png_read_info(png_ptr, info_ptr))
         return false;

      bpp = (info_ptr->pixel_depth+7) / 8;
      if(info_ptr->pixel_depth>32)
         bpp /= 2;
      //if(info_ptr->num_palette && info_ptr->bit_depth==8)
        // pf->bytes_per_pixel = 1;
                              //if 8-bit grayscale, consider it as RGB
      size_original_x = size_x = info_ptr->width;
      size_original_y = size_y = info_ptr->height;

      return true;
   }

//----------------------------

   virtual bool InitDecoder(){

      if(png_ptr->interlaced){
                              //read entire interlaced img now into cache
         dword use_bytes = Max(bpp, 3);
         interlaced_cache = new byte[size_x*size_y*use_bytes];
         if(!interlaced_cache)
            return false;
         int np = png_set_interlace_handling(png_ptr);
         while(np--){
            byte *bp = interlaced_cache;
            for(int y=size_y; y--; ){
               if(!png_read_row(png_ptr, bp, png_bytep_NULL))
                  return false;
               bp += size_x*use_bytes;
            }
         }
         interlaced_read_line = 0;
      }
      return true;
   }

//----------------------------

   virtual bool ReadOneOrMoreLines(C_vector<t_horizontal_line*> &lines){

                              //alloc new line
      t_horizontal_line *line = new(true) t_horizontal_line;
      line->Resize(size_x);
      lines.push_back(line);
      S_rgb *dst = line->Begin();

      const byte *src;
      if(png_ptr->interlaced){
                              //copy from cache
         if(bpp==4){
            MemCpy(dst, interlaced_cache + interlaced_read_line++ * size_x * 4, size_x*4);
            return true;
         }
         src = interlaced_cache + ++interlaced_read_line * size_x * 3;
      }else{
                              //read single non-interlaced line
         png_ptr->num_rows = png_ptr->height;
         if(!png_read_row(png_ptr, (byte*)dst, png_bytep_NULL))
            return false;
         src = (byte*)dst + size_x*3;
      }
      if(bpp!=4){
                              //epxand to 32-bit pixels
         dst += size_x;
         for(int i=size_x; i--; ){
            --dst;
            dst->r = *--src;
            dst->g = *--src;
            dst->b = *--src;
         }
      }else{
                              //set color of all transparent pixels to black for better filtering (don't rely on creating graphics application)
         for(int i=size_x; i--; ){
            S_rgb &p = *dst++;
            if(!p.a)
               p.r = p.g = p.b = 0;
         }
      }
      return true;
   }

public:
   C_loader_png(C_file *fl):
      C_image_loader(fl),
      png_ptr(NULL),
      info_ptr(NULL),
      interlaced_cache(NULL)
   {}
   ~C_loader_png(){
      delete[] interlaced_cache;
      if(png_ptr)
         png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
   }
};

//----------------------------

C_image_loader *CreateImgLoaderPNG(C_file *fl){ return new(true) C_loader_png(fl); }

//----------------------------

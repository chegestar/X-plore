//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#ifndef __IMGLIB_H_
#define __IMGLIB_H_

#include <C_file.h>
#include <C_buffer.h>
#include <C_vector.h>
#include <Fixed.h>

//----------------------------
                              //interface of generic image loader class
class C_image_loader{
protected:
   C_file *fl;
   bool owns_file;

   int size_x, size_y;
   int size_original_x, size_original_y;  //may differ from size_? if some kind of downsampling is used
   int bpp;                   //bytes per pixel
public:
   struct S_rgb{
      byte b, g, r;
      byte a;
   };

   C_image_loader(C_file *fl1):
      fl(fl1),
      owns_file(false)
   {}
   virtual ~C_image_loader(){
      if(owns_file)
         delete fl;
   }

   typedef C_buffer<S_rgb> t_horizontal_line;

   inline int GetSX() const{ return size_x; }
   inline int GetSY() const{ return size_y; }
   inline int GetOriginalSX() const{ return size_original_x; }
   inline int GetOriginalSY() const{ return size_original_y; }
   inline int GetBPP() const{ return bpp; }
   virtual bool IsTransparent() const{ return false; }
   virtual bool IsFlippedY() const{ return false; }

//----------------------------
// Get information about image.
//    header ... first 4 bytes of image data
//    limit_size_? ... limiting size of final image, if set to 0, it is not used; specifying limit may optimize image loader's performance
// Return true if identified successfully.
   virtual bool GetImageInfo(dword header, int limit_size_x = 0, int limit_size_y = 0, C_fixed limit_ratio = C_fixed::One()) = 0;

//----------------------------
// Called after GetImageInfo, and before decoding (ReadOneOrMoreLines).
// Returns true if memory for decoding can't be allocated.
   virtual bool InitDecoder() = 0;

//----------------------------
// Read one or more line(s) from image, and add them into provided vector.
// Return false is some error occured.
   virtual bool ReadOneOrMoreLines(C_vector<t_horizontal_line*> &lines) = 0;

//----------------------------
// Check if image is animated.
   virtual bool IsAnimated() const{ return false; }

//----------------------------
// Tick animation. Returns true if data for new animated frame are available.
   virtual bool Tick(int time){ return false; }

//----------------------------
// Mark that the loader owns the file and is responsible for its deletion.
   inline void SetFileOwnership(){ owns_file = true; }
};

//----------------------------
                              //image loaders implemented by ImgLib
C_image_loader *CreateImgLoaderPNG(C_file *fl);
C_image_loader *CreateImgLoaderJPG(C_file *fl);
C_image_loader *CreateImgLoaderPCX(C_file *fl);
C_image_loader *CreateImgLoaderBMP(C_file *fl);
C_image_loader *CreateImgLoaderICO(C_file *fl);
C_image_loader *CreateImgLoaderGIF(C_file *fl);

//----------------------------
#endif

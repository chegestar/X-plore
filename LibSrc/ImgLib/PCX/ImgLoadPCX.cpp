#include <ImgLib.h>

//----------------------------

class C_loader_pcx: public C_image_loader{
   S_rgb palette[256];

//----------------------------

   virtual bool GetImageInfo(dword header, int limit_size_x, int limit_size_y, C_fixed limit_ratio){

      if(header != 0x0801050a)
         return 0;

      int fsize = fl->GetFileSize();
      if(fsize < 64)
         return false;
      dword offs = fl->Tell();

                              //get size y
      fl->Seek(offs+0xa);
      byte b;
      word w;
      if(!fl->ReadWord(w))
         return false;
      size_original_y = size_y = w+1;
                              //get bpp
      fl->Seek(offs+0x41);
      if(!fl->ReadByte(b))
         return false;
      bpp = b;
                              //get size x
      if(!fl->ReadWord(w))
         return false;
      size_original_x = size_x = w;

                              //read palette (if present)
      if(bpp==1){
         if(fsize < 769)
            return false;
         fl->Seek(fsize - 769);
                              //must be magic byte 0xc
         byte magic;
         if(!fl->ReadByte(magic) || magic != 0xc)
            return false;
         for(int i=0; i<256; i++){
            S_rgb &p = palette[i];
            fl->Read(&p, 3);
            Swap(p.r, p.b);
            p.a = 0xff;
         }
      }
                              //get to data beg
      fl->Seek(offs+0x80);
      return true;
   }

//----------------------------

   virtual bool InitDecoder(){
      return true;
   }

//----------------------------

   virtual bool ReadOneOrMoreLines(C_vector<t_horizontal_line*> &lines){

                              //alloc new line
      t_horizontal_line *line = new(true) t_horizontal_line;
      line->Resize(size_x);
      lines.push_back(line);
      S_rgb *dst = line->Begin();

      if(bpp==1){
                              //do entire row
         int row;
         for(row=0; row<size_x; ){
            byte code;
            if(!fl->ReadByte(code))
               return false;
            if(code>=0xc0){
                              //color row
               dword rept = code - 0xc0;
               byte color;
               if(!fl->ReadByte(color))
                  return false;
               row += rept;
               while(rept--)
                  *dst++ = palette[color];
            }else{
                           //single color
               ++row;
               *dst++ = palette[code];
            }
         }
         assert(row==size_x);
      }else{
         byte color = 0;
         dword last_rept = 0;
                              //do all planes
         for(int plane=3; plane--; ){
            byte *bp = (byte*)dst;
            bp += plane;
                              //do entire row
            for(int row=0; row<size_x; ){
               if(last_rept){
                  dword rept = last_rept;
                  last_rept = 0;
                  if((row += rept) > size_x)
                     rept -= (row - size_x), last_rept = (row - size_x);
                  do{
                     *bp = color;
                     bp += sizeof(S_rgb);
                  }while(--rept);
               }else{
                  byte code;
                  if(!fl->ReadByte(code))
                     return false;
                  if(code>=0xc0){
                              //color row
                     dword rept = code - 0xc0;
                     if(!fl->ReadByte(color))
                        return false;
                     if((row += rept) > size_x)
                        rept -= (row - size_x), last_rept = (row - size_x);
                     do{
                        *bp = color;
                        bp += sizeof(S_rgb);
                     }while(--rept);
                  }else{
                              //single color
                     ++row;
                     *bp = code;
                     bp += sizeof(S_rgb);
                  }
               }
            }
         }
      }
      return true;
   }

public:
   C_loader_pcx(C_file *fl):
      C_image_loader(fl)
   {}
};

//----------------------------

C_image_loader *CreateImgLoaderPCX(C_file *fl){
   return new(true) C_loader_pcx(fl);
}

//----------------------------

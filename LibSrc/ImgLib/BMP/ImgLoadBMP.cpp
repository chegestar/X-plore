#include <ImgLib.h>

//----------------------------

class C_loader_bmp: public C_image_loader{
#pragma pack(push,1)
   struct S_bmp_header{
      dword size;
      word res[2];
      dword data_offs;
      struct{
         dword size;
         long sx, sy;
         word planes;
         word bitcount;
         dword compression;
         dword img_size;
         long x_ppm;
         long y_ppm;
         dword num_clr;
         dword clr_important;
      }bi_hdr;
   } bmp_header;
#pragma pack(pop)
   S_rgb palette[256];
   dword base_offs;

//----------------------------

   void ReadPalette(int num_colors){

      S_rgb *p = palette; 
      for(int i=num_colors; i--; ++p){
         fl->ReadByte(p->b);
         fl->ReadByte(p->g);
         fl->ReadByte(p->r);
         byte x;
         fl->ReadByte(x);
         p->a = 0xff;
      }
   }

public:

   C_loader_bmp(C_file *fl):
      C_image_loader(fl),
      base_offs(0)
   {}

//----------------------------

   virtual bool IsFlippedY() const{
      return (bmp_header.bi_hdr.sy>=0);
   }

//----------------------------

   virtual bool GetImageInfo(dword header, int limit_size_x, int limit_size_y, C_fixed limit_ratio){

      if((header&0xffff) != 0x4d42)
         return false;

      if(fl->GetFileSize() < sizeof(bmp_header)+2)
         return false;

      base_offs = fl->Tell();
      fl->Seek(base_offs+2);
                              //read header
      if(!fl->Read(&bmp_header, sizeof(bmp_header)))
         return false;

                              //check if it's supported bit-count
      switch(bmp_header.bi_hdr.bitcount){
      case 8:
         {
            dword num_c = bmp_header.bi_hdr.num_clr;
            if(!num_c)
               num_c = 256;
            if(fl->GetFileSize()-fl->Tell() < 768)
               return false;
         }
         break;
      case 24:
         break;
      default:
         return false;
      }

      size_original_x = size_x = bmp_header.bi_hdr.sx;
      size_original_y = size_y = Abs(bmp_header.bi_hdr.sy);
      bpp = (byte)(bmp_header.bi_hdr.bitcount / 8);

      return true;
   }

//----------------------------

   virtual bool InitDecoder(){

      if(bmp_header.bi_hdr.bitcount==8){
         dword num_c = bmp_header.bi_hdr.num_clr;
         ReadPalette(num_c);
         MemSet(palette+num_c, 0, sizeof(S_rgb)*(256-num_c));
      }
      fl->Seek(base_offs+bmp_header.data_offs);
      return true;
   }

//----------------------------

   virtual bool ReadOneOrMoreLines(C_vector<t_horizontal_line*> &lines){

                              //alloc new line
      t_horizontal_line *line = new(true) t_horizontal_line;
      line->Resize(size_x);
      lines.push_back(line);
      S_rgb *dst = line->Begin();

                              //scanline is dword aligned,
                              //compute padding length
      dword padding;
      padding = ((size_x*bpp+3) & -4) - size_x*bpp;
      switch(bmp_header.bi_hdr.compression){
      case 0:                 //BI_RGB
         if(!fl->Read(dst, size_x*bpp))
            return false;
         if(padding)
            fl->Seek(fl->Tell()+padding);
         if(bpp==3){
            for(int i=size_x; i--; ){
               MemCpy(dst+i, ((byte*)dst)+i*3, 3);
               dst[i].a = 0xff;
            }
         }
         break;
      case 1:                 //BI_RLE8
         {
            byte *db = (byte*)dst;
            while(true){
               union{
                  byte code[2];
                  word wcode;
               } au;
               if(!fl->ReadWord(au.wcode))
                  return false;
               if(!au.code[0]){
                  switch(au.code[1]){
                  case 0:     //eol
                     goto eol;
                  case 1:     //eof
                     goto eol;
                  case 2:     //delta
                     if(!fl->ReadWord(au.wcode))
                        return false;
                     break;
                  default:    //data length
                     if(!fl->Read(db, au.code[1]))
                        return false;
                     db += au.code[1];
                              //align to word
                     if(au.code[1]&1)
                        fl->Seek(fl->Tell()+1);
                  }
               }else{         //color fill
                  MemSet(db, au.code[1], au.code[0]);
                  db += au.code[0];
               }
            }
         }
eol:{}
         break;
      default:
         lines.pop_back();
         delete line;
         return false;
      }
      if(bpp==1){
                              //convert paletized to RGB
         for(int i=size_x; i--; )
            dst[i] = palette[((byte*)dst)[i]];
      }
      return true;
   }
};

//----------------------------

C_image_loader *CreateImgLoaderBMP(C_file *fl){ return new(true) C_loader_bmp(fl); }

//----------------------------

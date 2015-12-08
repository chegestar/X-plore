#include <ImgLib.h>

//----------------------------

class C_loader_ico: public C_image_loader{
#pragma pack(push,1)

   struct S_ico_header{
      struct S_bitmap_header{
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
   } ico_header;
#pragma pack(pop)
   dword data_offset;
   dword file_offset;

   S_rgb palette[256];
   dword line_index;
   enum{ MAX_WIDTH = 1024 };
   byte tmp_buf[MAX_WIDTH/8];

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
      MemSet(palette+num_colors, 0, sizeof(S_rgb)*(256-num_colors));
   }

public:

   C_loader_ico(C_file *fl):
      C_image_loader(fl),
      line_index(0)
   {}

//----------------------------

   /*
   virtual bool IsFlippedY() const{
      return true;//(ico_header.bi_hdr.sy>=0);
   }
   */

   virtual bool IsTransparent() const{ return true; }

   bool IsSupportedBitCount(dword bc){

      switch(bc){
      case 2:
      case 4:
      case 8:
      //case 16:
      case 24:
      case 32:
         return true;
      }
      return false;
   }

//----------------------------

   virtual bool GetImageInfo(dword header, int limit_size_x, int limit_size_y, C_fixed limit_ratio){

      if(header != 0x00010000)
         return false;

      dword file_size = fl->GetFileSize();
      if(file_size < 4+2)
         return false;

      file_offset = fl->Tell();
      fl->Seek(file_offset+4);
      word num_images;
      if(!fl->ReadWord(num_images) || !num_images)
         return false;

      struct S_icon_entry{
          byte bWidth;          // Width, in pixels, of the image
          byte bHeight;         // Height, in pixels, of the image
          byte bColorCount;     // Number of colors in image (0 if >=8bpp)
          byte bReserved;       // Reserved ( must be 0)
          word wPlanes;         // Color Planes
          word wBitCount;       // Bits per pixel
          dword dwBytesInRes;    // How many bytes in this resource?
          dword data_offs;   // Where in the file is this image?
      };
                              //read header (choose more detailed image)
      S_icon_entry ie, best_ie;
      MemSet(&best_ie, 0, sizeof(best_ie));
      S_ico_header::S_bitmap_header b_hdr;
      MemSet(&ico_header, 0, sizeof(ico_header));
      data_offset = 0;

      for(int i=0; i<num_images; i++){
         fl->Seek(file_offset+6+i*sizeof(ie));
         if(file_size < fl->Tell()+sizeof(ie))
            return false;
         if(!fl->Read(&ie, sizeof(ie)))
            return false;
         if(ie.data_offs+ie.dwBytesInRes > file_size)
            return false;

         fl->Seek(file_offset+ie.data_offs);
         if(!fl->Read(&b_hdr, sizeof(b_hdr)))
            return false;
         if(IsSupportedBitCount(b_hdr.bitcount)){

            if(!i || (b_hdr.bitcount>ico_header.bi_hdr.bitcount) ||
               (b_hdr.bitcount==ico_header.bi_hdr.bitcount && ((b_hdr.sx>ico_header.bi_hdr.sx) || (b_hdr.sy>ico_header.bi_hdr.sy)))
               ){
               ico_header.bi_hdr = b_hdr;
               data_offset = ie.data_offs;
            }
         }
      }
      if(!data_offset)
         return false;

      size_original_x = size_x = ico_header.bi_hdr.sx;
      size_original_y = size_y = ico_header.bi_hdr.sy/2;
      bpp = (byte)(ico_header.bi_hdr.bitcount / 8);
      if(size_x>MAX_WIDTH || size_y>MAX_WIDTH)
         return false;


      fl->Seek(file_offset+data_offset+sizeof(b_hdr));
                              //check if it's supported bit-count
      switch(ico_header.bi_hdr.bitcount){
      case 2:
         ReadPalette(4);
         break;
      case 4:
         ReadPalette(16);
         break;
      case 8:
         ReadPalette(256);
         break;
      }
      data_offset = fl->Tell();

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

      const int line_pitch = ((size_x*ico_header.bi_hdr.bitcount+7)/8+3) & -4;
                              //seek to line data (note: icon is stored bottom-to-top)
      fl->Seek(file_offset+data_offset+(size_y-1-line_index)*line_pitch);

      switch(ico_header.bi_hdr.bitcount){
      case 2:
         {
            if(!fl->Read(dst, size_x*ico_header.bi_hdr.bitcount/8))
               return false;
            for(int i=size_x; i--; ){
               dword p = ((byte*)dst)[i/4];
               p >>= 6-(i&3)*2;
               p &= 3;
               dst[i] = palette[p];
            }
         }
         break;
      case 4:
         {
            if(!fl->Read(dst, size_x*ico_header.bi_hdr.bitcount/8))
               return false;
            for(int i=size_x; i--; ){
               dword p = ((byte*)dst)[i/2];
               p >>= 4-(i&1)*4;
               p &= 0xf;
               dst[i] = palette[p];
            }
         }
         break;
      case 8:
         {
            if(!fl->Read(dst, size_x*ico_header.bi_hdr.bitcount/8))
               return false;
                              //8-bit to RGB
            for(int i=size_x; i--; )
               dst[i] = palette[((byte*)dst)[i]];
         }
         break;
         /*
      case 16:
         {
            for(int i=0; i<size_x; i++){
               S_rgb &p = dst[i];
               dword w = fl->ReadWord();
               p.r = (w<<3)&0xf8;
               p.g = (w>>3)&0xfc;
               p.b = (w>>7)&0xf8;
               p.a = 0xff;
            }
         }
         break;
         */
      case 24:
         {
            for(int i=0; i<size_x; i++){
               S_rgb &p = dst[i];
               if(!fl->Read(&p, 3))
                  return false;
               p.a = 0xff;
            }
         }
         break;
      case 32:
         if(!fl->Read(dst, size_x*4))
            return false;
         break;
      }

      const int transp_pitch = (((size_x+7)/8) + 3) & -4;
                              //read transparency, and set alpha
      fl->Seek(file_offset+data_offset + size_y*line_pitch + (size_y-1-line_index)*transp_pitch);
      if(!fl->Read(tmp_buf, (size_x+7)/8))
         return false;
      for(int i=0; i<size_x; i++){
         byte b = tmp_buf[i/8];
         if(b&(0x80>>(i&7))){
            dst[i].a = 0;
         }
      }
      ++line_index;
      return true;
   }
};

//----------------------------

C_image_loader *CreateImgLoaderICO(C_file *fl){
   return new(true) C_loader_ico(fl);
}

//----------------------------

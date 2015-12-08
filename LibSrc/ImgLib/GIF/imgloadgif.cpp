#include <ImgLib.h>

//#define DEBUG_FIXED_ANIM_TIME 1000

//----------------------------

class C_loader_gif: public C_image_loader{
   S_rgb global_palette[256];
   S_rgb local_palette[256];

   word prefix[4096];         //index of another code
   byte suffix[4096];         //terminating character
   byte out_stack[4097];      //output buffer

   C_buffer<byte> decoded_img;//decoded image sx*sy, in paletized mode; used for interlaced mode
   int curr_line;

   bool use_local_palette;
   byte background;           //bgnd color (index); used only if global palette specified (packed_fields has FLG_GLOBAL_COLOR bit set)
   //byte packed_fields;

                              //progressive loader:
   bool interlaced;
   byte init_code_size;       //1-8

   bool transparent;
   dword transparent_index;

//----------------------------
                              //bit reader
   dword bits_cache;
   dword bits_in_cache;
   int block_size;

                              //current code size (size in bits of codes)
   byte code_size;            //1 - 12
   word clear_code;           //2^n, 1 - 256
   word prev_code;
                              //index of next free entry in table
   dword next_entry_index;    //0 - 4095

   int num_pixels_left;
           
   struct S_rect{
      int x, y, sx, sy;
      S_rect(){}
      S_rect(int _x, int _y, int _sx, int _sy):
         x(_x),
         y(_y),
         sx(_sx),
         sy(_sy)
      {}
   };
                   //current frame resolution
   S_rect decoded_rect;

   enum E_DISPOSAL{
      DISPOSAL_UNDEFINED,
      DISPOSAL_NO,
      DISPOSAL_RESTORE_BGND,
      DISPOSAL_RESTORE_PREVIOUS,
   } frame_disposal;

   dword stack_top;           //0 - 4096

//----------------------------
                              //animation
   bool is_animated;
   dword beg_anim_file_offset;   //file offset to beginning of animation
   int frame_persist_time;
   byte *pic_buffer;         //pixel data of last rendered frame
   int curr_frame;

//----------------------------

   void BeginLZWDecode(){

      bits_cache = 0;
      bits_in_cache = 0;
      block_size = 0;

      code_size = init_code_size + 1;
      clear_code = 1 << init_code_size;

      next_entry_index = clear_code + 2;
      prev_code = 0;

      num_pixels_left = decoded_rect.sx * decoded_rect.sy;

      stack_top = 0;
   }

//----------------------------

   bool ReadBits(dword num, word &val){

                              //fill-in cache
      while(block_size!=-1 && bits_in_cache<=24){
         if(!block_size){
            byte b;
            fl->ReadByte(b);
            block_size = b;
            if(!block_size){
               block_size = -1;
               break;
            }
         }
         byte b;
         fl->ReadByte(b);
         --block_size;
         bits_cache |= dword(b)<<bits_in_cache;
         bits_in_cache += 8;
      }

      assert(num <= bits_in_cache);
      if(num > bits_in_cache)
         return false;

      dword mask = (1<<num) - 1;
      val = word(bits_cache&mask);

      bits_cache >>= num;
      bits_in_cache -= num;
      return true;
   }

//----------------------------

   bool ReadStack(){

                              //reset output stack
      stack_top = 0;
                              //get next code
      word code;
      assert(code_size <= 12);
      if(!ReadBits(code_size, code))
         return false;

                              //index of first free entry in table
      const word first_entry = clear_code + 2;

                              //switch, different posibilities for code:
      if(code == clear_code+1)
         return true;         //exit LZW decompression loop
      if(code == clear_code){
         code_size = init_code_size + 1;  //reset code size
         next_entry_index = first_entry;        //reset Translation Table
         prev_code = code;                //Prevent next to be added to table.
                              //restart, to get another code
         return ReadStack();
      }
      word out_code;          //0 - 4096
      if(code < next_entry_index){
                           //code is in table
                           //set code to output
         out_code = code;
      }else{
                           //code is not in table
                           //keep first character of previous output
         ++stack_top;
                           //set prev_code to be output
         out_code = prev_code;
      }
                           //expand outcode in out_stack

                           // - Elements up to first_entry are Raw-Codes and are not expanded
                           // - Table Prefices contain indexes to other codes
                           // - Table Suffices contain the raw codes to be output
      while(out_code >= first_entry){
         if(stack_top > 4096)
            return false;
                           //add suffix to output stack
         out_stack[stack_top++] = suffix[out_code];
                           //loop with preffix
         out_code = prefix[out_code];
                              //watch for corruption
         if(out_code >= 4096)
            out_code = 0;
      }

                           //now outcode is a raw code, add it to output stack
      if(stack_top > 4096)
         return false;
      out_stack[stack_top++] = byte(out_code);

                           //add new entry to table (prev_code + outcode)
                           // (except if previous code was a clearcode)
      if(prev_code!=clear_code){
                           //prevent translation table overflow
         if(next_entry_index>=4096)
            //return false;
            next_entry_index = 4095;

         prefix[next_entry_index] = prev_code;
         suffix[next_entry_index] = byte(out_code);
         ++next_entry_index;
                           //increase codesize if next_entry_index is invalid with current codesize
         if(next_entry_index >= dword(1<<code_size)){
            if(code_size < 12)
               ++code_size;
         }
      }
      prev_code = code;
      return true;
   }

//----------------------------

   bool DecodeSingleLine(byte *dst){

      int x = decoded_rect.sx;
      while(true){
                              //fill pixels that we have
         while(stack_top){
            *dst++ = out_stack[--stack_top];
            if(!--x)
               return true;
         }
                              //decode new data
         if(!ReadStack())
            return false;
      }
      return true;
   }

//----------------------------

   bool DecodeInterlaced(){

      assert(interlaced);
      byte *dst = decoded_img.Begin();

      int d_line = 0;
      for(int y=0; y<decoded_rect.sy; y++){
         if(!DecodeSingleLine(dst + d_line*decoded_rect.sx))
            return false;

         if(interlaced){
            if(!(d_line&7)){
                        //initially every 8th line
               d_line += 8;
               if(d_line >= decoded_rect.sy){
                  d_line = 4;
                  if(d_line >= decoded_rect.sy){
                     d_line = 2;
                     if(d_line >= decoded_rect.sy)
                        d_line = 1;
                  }
               }
            }else
            if(!(d_line&3)){
                        //then fill lines beginning on line 4
               d_line += 8;
               if(d_line >= decoded_rect.sy){
                  d_line = 2;
                  if(d_line >= decoded_rect.sy)
                     d_line = 1;
               }
            }else
            if(!(d_line&1)){
                        //then fill lines beginning on line 2, every 4th lines
               d_line += 4;
               if(d_line >= decoded_rect.sy)
                  d_line = 1;
            }else
               d_line += 2;
         }else
            ++d_line;
      }
      return true;
   }

//----------------------------

   void ReadPalette(S_rgb *p, int num_colors){

      for(int i=num_colors; i--; ++p){
         fl->ReadByte(p->r);
         fl->ReadByte(p->g);
         fl->ReadByte(p->b);
         p->a = 0xff;
      }
   }

   enum{
      FLG_GLOBAL_COLOR = 0x80,
   };

//----------------------------

   virtual bool GetImageInfo(dword header, int limit_size_x, int limit_size_y, C_fixed limit_ratio){

                              //check 'gif8'
      if(header!=0x38464947)
         return false;

      dword offs = fl->Tell();
      fl->Seek(offs+6);

      word wsx, wsy;
      if(!fl->ReadWord(wsx) || !fl->ReadWord(wsy))
         return false;
      size_original_x = size_x = wsx;
      size_original_y = size_y = wsy;

                              //packed fields. bits detail:
                              // 0-2: size of global color table
                              // 3: sort flag
                              // 4-6: color resolution
                              // 7: global color table
      byte packed_fields;
      fl->ReadByte(packed_fields);
                              //background color index
      fl->ReadByte(background);
      byte pixelaspectratio;
      fl->ReadByte(pixelaspectratio);

      bpp = (packed_fields&7) + 1;

                              //read/generate global color map
      if(packed_fields&FLG_GLOBAL_COLOR){
         ReadPalette(global_palette, 1<<bpp);
      }else{
                              //gif standard says to provide an internal default palette:
         for(int i=0; i<256; i++){
            global_palette[i].r = global_palette[i].g = global_palette[i].b = byte(i);
            global_palette[i].a = 0xff;
         }
      }
      beg_anim_file_offset = fl->Tell();
      //dword color_res = (packed_fields>>4) & 7;
      //assert(!decoded_rect.x && !decoded_rect.y && decoded_rect.sx==sx && decoded_rect.sy==sy);
      //is_animated = (frame_persist_time>0);
      return true;
   }

//----------------------------

   virtual bool InitDecoder(){

                              //alloc anim buffer
      pic_buffer = new byte[size_x*size_y];
      if(!pic_buffer)
         return false;

      if(!ReadGifData())
         return false;

      return true;
   }

//----------------------------

   bool ReadGifData(){

      enum{                   //gif-extension flags
         GE_TRANSPARENT = 1,
         GE_USER_INPUT = 2,
                              //2-4: disposal method
      };
      struct{                 //graphic control extension
         byte blocksize;      //block size: 4 bytes
         byte flags;
         word delay;          //delay time (1/100 seconds)
         byte transparent;    //transparent color index
      } gif_extension;
      bool gfx_extension_found = false;

      //frame_disposal = DISPOSAL_UNDEFINED;

                              //now we have 3 possibilities:
                              // a) get and extension block (blocks with additional information)
                              // b) get an image separator (introductor to an image)
                              // c) get the trailer char (end of gif file)
      while(!fl->IsEof()){
         byte code;
         if(!fl->ReadByte(code))
            return false;
         switch(code){
         case 0x21:
            {
                              //*a* extension block - only in Gif89a or newer
               byte subcode;
               fl->ReadByte(subcode);
               switch(subcode){
               case 0xf9:
                  {
                                 //graphic control extension
                     fl->Read(&gif_extension, 5);
                     assert(gif_extension.blocksize==4);
                     gfx_extension_found = true;
                     if(transparent){
                              //restore previous transparent color to non-transparent
                        assert(transparent_index!=0x100);
                        S_rgb *pal = use_local_palette ? local_palette : global_palette;
                        pal[transparent_index].a = 0xff;
                     }
                     transparent = (gif_extension.flags&GE_TRANSPARENT);
                     transparent_index = transparent ? gif_extension.transparent : 0x100;

                              //process disposal
                     if(curr_frame==-1)
                        MemSet(pic_buffer, background, size_x*size_y);
                     else
                     if(frame_disposal==DISPOSAL_RESTORE_BGND){
                        byte *d = pic_buffer + decoded_rect.y*size_x + decoded_rect.x;
                        for(int y=0; y<decoded_rect.sy; y++){
                           MemSet(d, background, decoded_rect.sx);
                           d += size_x;
                        }
                     }

                     frame_disposal = (E_DISPOSAL)(gif_extension.flags>>2);

#ifdef DEBUG_FIXED_ANIM_TIME
                     frame_persist_time = gif_extension.delay ? DEBUG_FIXED_ANIM_TIME : 0;
#else
                     frame_persist_time = gif_extension.delay;
                     if(!frame_persist_time)
                        frame_persist_time = 10;
                     frame_persist_time *= 10;
#endif

                                 //block terminator (always 0)
                     byte term;
                     fl->ReadByte(term);
                  }
                  break;

               case 0xfe:        //comment extension
               case 0x01:        //plaintext extension
               case 0xff:        //application extension
               default:          //unknown extension
                                 //read (and ignore) data sub-blocks
                  while(true){
                     byte block_length;
                     if(!fl->ReadByte(block_length))
                        return false;
                     if(!block_length)
                        break;
#ifdef _DEBUG_
                     byte *bp = new byte[block_length];
                     fl->Read(bp, block_length);
                     delete[] bp;
#else
                     if(!fl->Seek(fl->Tell() + block_length))
                        return false;
#endif
                  }
                  break;
               }
            }
            break;

         case 0x2c:
            {
                              //*b* image (0x2c image separator)
               enum{
                              //0-2: size of local color table
                              //3-4: (reserved)
                  FLG_SORT = 0x20,
                  FLG_INTERLACE = 0x40,
                  FLG_LOCAL_COLOR = 0x80,
               };

                              //read image descriptor
               struct{
                  word xpos;
                  word ypos;
                  word width;
                  word height;
                  byte flags;
               } gif_id;
               fl->Read(&gif_id, 9);
               bool local_colormap = (gif_id.flags&FLG_LOCAL_COLOR);

#if 0
               /*
               if(gfx_extension_found){
                  nextimage->transparent = (gifgce.flags&0x01) ? gifgce.transparent : -1;
                  nextimage->transparency = (gifgce.flags&0x1c)>1 ? 1 : 0;
                  nextimage->delay = gifgce.delay*10;
               }
               */
#endif
               decoded_rect = S_rect(gif_id.xpos, gif_id.ypos, gif_id.width, gif_id.height);

               if(local_colormap){
                              //read color map (if descriptor says so)
                  int num_colors = 2 << (gif_id.flags&7);
                  ReadPalette(local_palette, num_colors);
                  use_local_palette = true;
               }else{
                              //otherwise use global
                  use_local_palette = false;
               }
               if(transparent// && frame_disposal==DISPOSAL_RESTORE_BGND
                  ){
                  S_rgb *pal = use_local_palette ? local_palette : global_palette;
                  pal[transparent_index].a = 0;
               }

               interlaced = (gif_id.flags&FLG_INTERLACE);
                              //1st byte of img block (codesize)
               fl->ReadByte(init_code_size);
               assert(init_code_size>0 && init_code_size<=8);
               //if(init_code_size>8)
                  //init_code_size = 8;

               BeginLZWDecode();

               if(interlaced){
                  decoded_img.Resize(decoded_rect.sx*decoded_rect.sy, background);
                  
                  DecodeInterlaced();
               }
               curr_line = 0;
               ++curr_frame;
               gfx_extension_found = false;
               return true;
            }
            break;

         case 0x3b:
                              //*c* trailer: end of gif info
                              //standard end
            if(!IsAnimated())
               return true;
            fl->Seek(beg_anim_file_offset);
            curr_frame = -1;
            break;
         }
      }
      return true;
   }

//----------------------------

   virtual bool ReadOneOrMoreLines(C_vector<t_horizontal_line*> &lines){

      t_horizontal_line *line = new(true) t_horizontal_line;
      line->Resize(size_x);
      lines.push_back(line);
      S_rgb *dst = line->Begin();

                              //decode into temp buffer
      if(curr_line>=decoded_rect.y && curr_line<(decoded_rect.y+decoded_rect.sy)){
         byte *dst = pic_buffer + curr_line*size_x + decoded_rect.x;
         byte *tmp = (byte*)(line->Begin() + decoded_rect.sx) - decoded_rect.sx;
         if(interlaced){
            tmp = decoded_img.Begin() + decoded_rect.sx*(curr_line-decoded_rect.y);
         }else{
                              //decode line now
            if(!DecodeSingleLine(tmp)){
               delete line;
               lines.pop_back();
               return false;
            }
         }
         if(curr_frame==0 || !transparent){
            for(int i=decoded_rect.sx; i--; )
               *dst++ = *tmp++;
         }else{
            for(int i=decoded_rect.sx; i--; ){
               byte b = *tmp++;
               if(b!=transparent_index)
                  *dst = b;
               ++dst;
            }
         }
      }
      const byte *src = pic_buffer + curr_line*size_x;

      const S_rgb *pal = use_local_palette ? local_palette : global_palette;
      for(int i=size_x; i--; ){
         byte b = *src++;
         *dst++ = pal[b];
      }

      ++curr_line;
      if(curr_line==size_y && !is_animated){
         dword offs = fl->Tell();
         if(!fl->IsEof()){
            byte code;
            do{
               fl->ReadByte(code);
            }while(!code && !fl->IsEof());
            switch(code){
            case 0x21:
               is_animated = true;
               break;
            case 0x3b:
               break;
            case 0:
               break;
            default:
               assert(0);
            }
         }
         fl->Seek(offs);
      }
      return true;
   }

//----------------------------

   virtual bool IsTransparent() const{ return transparent; }

//----------------------------

   virtual bool IsAnimated() const{
      return is_animated;
   }

//----------------------------

   virtual bool Tick(int time){

      if((frame_persist_time-=time) > 0)
         return false;
      frame_persist_time = 0;
                              //decode next frame
      return ReadGifData();
   }

//----------------------------

   virtual ~C_loader_gif(){
      delete[] pic_buffer;
   }
public:
   C_loader_gif(C_file *fl):
      C_image_loader(fl),
      transparent(false),
      frame_persist_time(0),
      is_animated(false),
      curr_line(0),
      use_local_palette(false),
      transparent_index(0x100),
      frame_disposal(DISPOSAL_UNDEFINED),
      pic_buffer(NULL),
      curr_frame(-1)
   {}
};

//----------------------------

C_image_loader *CreateImgLoaderGIF(C_file *fl){
   return new(true) C_loader_gif(fl);
}

//----------------------------

//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <ZipRead.h>
#include <Zlib.h>

//----------------------------

class C_zip_raw_write_imp: public C_file::C_file_base{
   C_file_base *imp;       //original file writer
   z_stream zs;
   static const dword BUF_SIZE = 16384;
   byte buf[BUF_SIZE];     //temp compressed buffer
   dword num_written;

//----------------------------
   C_file::E_WRITE_STATUS WriteBuf(){
      C_file::E_WRITE_STATUS st = imp->Write(buf, zs.total_out);
      zs.total_out = 0;
      zs.next_out = buf;
      zs.avail_out = BUF_SIZE;
      return st;
   }
   virtual C_file::E_FILE_MODE GetMode() const{ return C_file::FILE_WRITE; }
public:
   C_zip_raw_write_imp(C_file_base *_p, int compression):
      imp(_p),
      num_written(0)
   {
      MemSet(&zs, 0, sizeof(zs));
      deflateInit2(&zs, compression, Z_DEFLATED, -MAX_WBITS, 9, Z_DEFAULT_STRATEGY);
      zs.next_out = buf;
      zs.avail_out = BUF_SIZE;

      dword sig = FOUR_CC('Z','R','A','W');
      imp->Write(&sig, 4);
      dword ver = 1;
      imp->Write(&ver, 4);
      imp->Write(&sig, 4);
   }
//----------------------------
   virtual ~C_zip_raw_write_imp(){
      imp->SetPos(8);
      imp->Write(&num_written, 4);
      delete imp;
      deflateEnd(&zs);
   }
//----------------------------
   virtual C_file::E_WRITE_STATUS Write(const void *mem, dword len){
      num_written += len;
      zs.avail_in = len;
      zs.next_in = (byte*)mem;
      while(zs.avail_in){
         int err = deflate(&zs, Z_NO_FLUSH);
         assert(!err);
         if(err<0)
            return C_file::WRITE_FAIL;
         if(!zs.avail_out){
                           //time to write to file
            C_file::E_WRITE_STATUS st = WriteBuf();
            if(st!=C_file::WRITE_OK)
               return st;
         }
      }
      return C_file::WRITE_OK;
   }
//----------------------------
   virtual C_file::E_WRITE_STATUS WriteFlush(){
      while(true){
         int err = deflate(&zs, Z_FINISH);
         assert(err>=0);
         if(err<0)
            return C_file::WRITE_FAIL;
         C_file::E_WRITE_STATUS st = WriteBuf();
         if(st!=C_file::WRITE_OK)
            return st;
         if(err==Z_STREAM_END)
            break;
      };
      return imp->WriteFlush();
   }
   virtual bool IsEof() const{ return true; }
   virtual dword GetFileSize() const{ return num_written; }
   virtual dword GetCurrPos() const{ return num_written; }
   virtual bool SetPos(dword pos){ return false; }
};

//----------------------------

//----------------------------

bool C_file_raw_zip_write::Open(const wchar *fname, int compression){

   if(compression<COMPRESSION_FAST || compression>COMPRESSION_BEST)
      return false;
   if(!C_file::Open(fname, FILE_WRITE_CREATE_PATH|FILE_WRITE))
      return false;
   imp = new C_zip_raw_write_imp(imp, compression);
   return true;
}

//----------------------------

C_file_raw_zip_write::~C_file_raw_zip_write(){
   if(imp){
      if(WriteFlush())
         Fatal("Write error");
   }
}

//----------------------------

class C_zip_raw_read_imp: public C_file::C_file_base{
   C_file_base *imp;       //original file writer
   dword sz_uncompressed, sz_compressed;
   static const dword CACHE_SIZE = 0x2000;
   byte buf[CACHE_SIZE];
   byte *top, *curr;
   dword curr_pos;         //real file pos
   z_stream zs;
   byte d_buf[1024];

   virtual C_file::E_FILE_MODE GetMode() const{ return C_file::FILE_READ; }
public:
   C_zip_raw_read_imp(C_file_base *_p):
      imp(_p),
      curr_pos(0),
      sz_uncompressed(0)
   {
      MemSet(&zs, 0, sizeof(zs));
      curr = top = buf;
   }
   bool Init(){
      dword id, ver;
      if(!imp->ReadDword(id) || id!=FOUR_CC('Z','R','A','W'))
         return false;
      if(!imp->ReadDword(ver) || ver!=1)
         return false;
      if(!imp->ReadDword(sz_uncompressed))
         return false;
      sz_compressed = imp->GetFileSize() - imp->GetCurrPos();
      int err = inflateInit2(&zs, -MAX_WBITS);
      return (err==0);
   }
   virtual ~C_zip_raw_read_imp(){
      delete imp;
      inflateEnd(&zs);
   }
   virtual bool IsEof() const{
      return (curr==top && GetFileSize()==GetCurrPos());
   }
//----------------------------
   virtual dword GetCurrPos() const{ return curr_pos+curr-top; }
   virtual dword GetFileSize() const{ return sz_uncompressed; }
   virtual bool SetPos(dword pos){

      if(pos > GetFileSize())
         return false;
      if(pos<GetCurrPos()){
         if(pos >= curr_pos-(top-buf)){
            curr = top-(curr_pos-pos);
         }else{
                              //rewind and decompress
            imp->SetPos(imp->GetFileSize()-sz_compressed);
            inflateEnd(&zs);
            MemSet(&zs, 0, sizeof(zs));
            inflateInit2(&zs, -MAX_WBITS);
            curr = top = buf;
            curr_pos = 0;
            SetPos(pos);
         }
      }else
      if(pos > curr_pos){
                              //read from file until we've desired position in cache
         do{
            curr_pos += ReadCache();
         }while(pos > curr_pos);
         curr = buf + (pos&(CACHE_SIZE-1));
      }else
         curr = top + pos - curr_pos;
      return true;
   }
//----------------------------
   virtual bool Read(void *mem, dword len){
      dword read_len = 0;
      const dword wanted_len = len;
      dword rl = 0;
      while(curr+len>top){
         dword rl1 = top-curr;
         MemCpy(mem, curr, rl1);
         len -= rl1;
         mem = (byte*)mem + rl1;
         dword sz = ReadCache();
         curr_pos += sz;
         rl += rl1;
         if(!sz){
            read_len = rl;
            goto check_read_size;
         }
      }
      MemCpy(mem, curr, len);
      curr += len;
      read_len = rl + len;
check_read_size:
      return (read_len == wanted_len);
   }
//----------------------------

   virtual bool ReadByte(byte &v){
      if(curr==top){
         dword sz = ReadCache();
         if(sz < sizeof(byte))
            return false;
         curr_pos += sz;
      }
      v = *curr++;
      return true;
   }
//----------------------------
   dword ReadCache(){
      curr = buf;
                              //read next data from cache
      zs.next_out = buf;
      zs.avail_out = Min(int(CACHE_SIZE), int(sz_uncompressed-zs.total_out));

      dword num_read = zs.total_out;
      while(zs.avail_out){
         int flush_flag = 0;
         if(!zs.avail_in){
            dword sz = Min(int(sizeof(d_buf)), int(sz_compressed-zs.total_in));
            if(sz){
               imp->Read(d_buf, sz);
               zs.next_in = d_buf;
               zs.avail_in = sz;
            }else
               flush_flag = Z_SYNC_FLUSH;
         }
         int err = inflate(&zs, flush_flag);
         if(err == Z_STREAM_END)
            break;
         if(err != Z_OK){
            switch(err){
            case Z_MEM_ERROR: assert(0); break; //ZIP file decompression failed (memory error).
            case Z_BUF_ERROR: assert(0); break; //ZIP file decompression failed (buffer error).
            case Z_DATA_ERROR: assert(0); break;//ZIP file decompression failed (data error).
            default: assert(0);  //ZIP file decompression failed (unknown error).
            }
            return 0;
         }
      }
      num_read = zs.total_out - num_read;
      top = buf + num_read;
      return num_read;
   }
};

//----------------------------

bool C_file_raw_zip_read::Open(const wchar *fname){
   if(!C_file::Open(fname))
      return false;
   C_zip_raw_read_imp *fz = new C_zip_raw_read_imp(imp);
   imp = fz;
   return fz->Init();;
}

//----------------------------

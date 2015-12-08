//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#ifdef __SYMBIAN32__
#define SYMBIAN_OPTIM //use system file access instead of C_file (overrides cache)
#endif

#ifdef SYMBIAN_OPTIM
#include <fbs.h>
#include <s32file.h>
#include <eikenv.h>
#else
#ifdef _WINDOWS
#include <ctype.h>
#endif
#include <C_file.h>
#endif

#include <C_buffer.h>
#include <C_file.h>
#include <TextUtils.h>
#include <AppBase.h>
#include <ZipRead.h>

#include <zlib.h>

#ifdef SYMBIAN_OPTIM
RFs &InitFileSession(RFs &file_session);
void CloseFileSession(RFs &file_session);
#endif

//----------------------------
//----------------------------

class C_zip_package_imp: public C_zip_package{
   /*
   struct DataDesc{
      dword signature;       // 0x08074b50
      dword crc;             //
      dword compressedSize;     //
      dword uncompressedSize;   //
   };
   */
   virtual E_TYPE GetArchiveType() const{ return TYPE_ZIP; }
   virtual const wchar *GetFileName() const{ return filename_full; }
public:
   struct S_file_entry{
      dword local_offset;     //offset info local header
      dword size_uncompressed;
      dword size_compressed;
      dword packer_version;
      wchar name[1];           //variable-length name (1st char is length)

      inline dword GetStructSize() const{
         return (((byte*)name - (byte*)this) + (((name[0]+2)*sizeof(wchar))) + 3) & -4;
      }
      inline bool CheckName(const wchar *in_name, int len) const{
         if(len!=name[0])
            return false;
         const wchar *n = name+1;
         while(len--){
            if(text_utils::LowerCase(*n++)!=*in_name++)
               return false;
         }
         return true;
      }
   };

   void *file_info;           //pointer to array of variable-length S_file_entry structures
   dword num_files;

   Cstr_w filename_full;      //zip filename
   C_file *arch_file;
   bool take_arch_ownership;
   bool is_app_data;

private:

   //C_buffer<char> password;

   friend class C_dta_handle_zip;
//----------------------------
public:
   C_zip_package_imp(bool _is_app_data):
      file_info(NULL),
      num_files(0),
      is_app_data(_is_app_data),
      arch_file(NULL)
   {}
   C_zip_package_imp(C_file *fp, bool take_ownership):
      file_info(NULL),
      num_files(0),
      arch_file(fp),
      take_arch_ownership(take_ownership)
   {}
   ~C_zip_package_imp(){
      delete[] (byte*)file_info;
      if(arch_file && take_arch_ownership)
         delete arch_file;
   }

   bool Construct(const wchar *fname, C_file &ck);

//----------------------------

   virtual dword NumFiles() const{ return num_files; }

//----------------------------

   virtual void *GetFirstEntry(const wchar *&name, dword &length) const{
      if(!num_files)
         return NULL;
      S_file_entry *fe = (S_file_entry*)file_info;
      if(fe){
         name = fe->name+1;
         length = fe->size_uncompressed;
      }
      return fe;
   }

//----------------------------

   virtual void *GetNextEntry(void *handle, const wchar *&name, dword &length) const{
      S_file_entry *fe = (S_file_entry*)handle;
      fe = (S_file_entry*)((byte*)fe + fe->GetStructSize());
      if(fe->local_offset==0xffffffff)
         return NULL;
      name = fe->name+1;
      length = fe->size_uncompressed;
      return fe;
   }

//----------------------------

   virtual bool GetFileWriteTime(const wchar *filename, S_date_time &td) const{ return false; }

   virtual C_file *OpenFile(const wchar *fname) const{
      C_file_zip *ck = new(true) C_file_zip;
      if(ck->Open(fname, this))
         return ck;
      delete ck;
      return NULL;
   }

//----------------------------

   inline bool IsAppData() const{ return is_app_data; }
};

//----------------------------

bool C_zip_package_imp::Construct(const wchar *fname, C_file &ck){

   struct C_DirEndRecord{
      dword signature;       // 0x06054b50
      word thisDisk;        // number of this disk
      word dirStartDisk;    // number of the disk containing the start of the central directory
      word numEntriesOnDisk;   // # of entries in the central directory on this disk
      word numEntriesTotal; // total # of entries in the central directory
      dword dirSize;         // size of the central directory
      dword dirStartOffset;     // offset of the start of central directory on the disk where the central directory begins
      word commentLen;         // zip file comment length

      void Read(C_file &fl){
         fl.ReadWord(thisDisk);
         fl.ReadWord(dirStartDisk);
         fl.ReadWord(numEntriesOnDisk);
         fl.ReadWord(numEntriesTotal);
         fl.ReadDword(dirSize);
         fl.ReadDword(dirStartOffset);
         fl.ReadWord(commentLen);
      }
   } er;
   const int SIZEOF_DirEndRecord = 22;
   //const int SIZEOF_C_DirFileHeader = 46;

   er.numEntriesTotal = 0;
   er.dirStartOffset = 0;
   long file_size = ck.GetFileSize();

                           //try to load end record
   if(file_size < SIZEOF_DirEndRecord)
      return false;
   {
      int i;
      for(i=0; i<8192; i++){
         int offs = file_size - SIZEOF_DirEndRecord - i;
         if(offs<0)
            return false;
         ck.Seek(offs);
         if(!ck.ReadDword(er.signature))
            return false;
         if(er.signature==0x06054b50)
            break;
      }
      if(i==100)
         return false;
                           //found, read the end record
      er.Read(ck);

                           //fail, if multi-volume zip
      if(er.thisDisk != 0 || er.dirStartDisk != 0 || er.numEntriesOnDisk != er.numEntriesTotal)
         return false;
   }
   ck.Seek(er.dirStartOffset);
                              //compute the size of data needed for storing all file info
   dword alloc_size = er.numEntriesTotal * (OffsetOf(S_file_entry, name) + sizeof(wchar)*2 + 3);
                              //space for names (deduct from used file space)
   alloc_size += er.dirSize*2;
                              //add terminating zero dword + align
   alloc_size += 4;

                           //alloc structures
   file_info = new(true) byte[alloc_size];

   num_files = 0;
   S_file_entry *file_entry = (S_file_entry*)file_info;

   for(int i=0; i<er.numEntriesTotal; i++){
      struct C_DirFileHeader{
         dword signature;       // 0x02014b50
         word versionUsed;     //
         word versionNeeded;      //
         word flags;           //
         word compression;     // compression method
         word lastModTime;     //
         word lastModDate;     //
         dword crc;          //
         dword compressed_size;     //
         dword uncompressed_size;   //
         word filenameLen;     // length of the filename field following this structure
         word extraFieldLen;      // length of the extra field following the filename field
         word commentLen;         // length of the file comment field following the extra field
         word diskStart;       // the number of the disk on which this file begins
         word internal;        // internal file attributes
         dword external;        // external file attributes
         dword localOffset;     // relative offset of the local file header

         void Read(C_file &fl){
            fl.ReadDword(signature);
            fl.ReadWord(versionUsed);
            fl.ReadWord(versionNeeded);
            fl.ReadWord(flags);
            fl.ReadWord(compression);
            fl.ReadWord(lastModTime);
            fl.ReadWord(lastModDate);
            fl.ReadDword(crc);
            fl.ReadDword(compressed_size);
            fl.ReadDword(uncompressed_size);
            fl.ReadWord(filenameLen);
            fl.ReadWord(extraFieldLen);
            fl.ReadWord(commentLen);
            fl.ReadWord(diskStart);
            fl.ReadWord(internal);
            fl.ReadDword(external);
            fl.ReadDword(localOffset);
         }
      };
      C_DirFileHeader file_hdr;
      file_hdr.Read(ck);
                           //skip empty files (directories)
      //if(file_hdr.compressedSize)
      {
         int nl = file_hdr.filenameLen;
                           //read entry name
         assert(nl<256);
         char _name[256];
         ck.Read(_name, nl);
         _name[nl] = 0;
         Cstr_w name;
         if(file_hdr.flags&0x800){
            name.FromUtf8(Cstr_c(_name));
            assert((int)name.Length()<=nl);
            nl = name.Length();
         }else
            name.Copy(_name);

         switch(file_hdr.compression){
         case 0:           //store
         case 8:           //deflate
#ifdef _DEBUG
         case 99:          //aes
#endif
            {
               file_entry->local_offset = file_hdr.localOffset;
               file_entry->size_uncompressed = file_hdr.uncompressed_size;
               file_entry->size_compressed = file_hdr.compressed_size;
               file_entry->packer_version = file_hdr.versionUsed;
               file_entry->name[0] = (word)nl;

               for(int j=0; j<nl; j++){
                  wchar c = name[j];
                  if(c=='/')
                     c = '\\';
                  else if(c<' ')
                     c = '?';
                  file_entry->name[1+j] = c;
               }
               file_entry->name[1+nl] = 0;
               ++num_files;
                           //calculate next offset
               file_entry = (S_file_entry*)((byte*)file_entry + file_entry->GetStructSize());
            }
            break;
         }
         ck.Seek(ck.Tell() + file_hdr.extraFieldLen + file_hdr.commentLen);
      //}else{
         //ck.Seek(ck.Tell() + file_hdr.extraFieldLen + file_hdr.commentLen + nl);
      }
   }
                              //add terminator
   *((dword*&)file_entry)++ = 0xffffffff;

   if(fname)
      C_file::GetFullPath(fname, filename_full);
   {
      dword real_size = (byte*)file_entry - (byte*)file_info;
      assert(real_size <= alloc_size);

                           //alloc real size now, and copy
      byte *mem = new(true) byte[real_size];
      MemCpy(mem, file_info, real_size);
      delete[] (byte*)file_info;
      file_info = mem;
   }
   return true;
}

//----------------------------
//----------------------------

C_zip_package *C_zip_package::Create(const wchar *fname, bool is_app_data){

   C_file fl;
   if(!fl.Open(fname))
      return NULL;
   C_zip_package_imp *zr = new(true) C_zip_package_imp(is_app_data);
   if(!zr->Construct(fname, fl)){
      zr->Release();
      zr = NULL;
   }
   return zr;
}

//----------------------------

C_zip_package *C_zip_package::Create(C_file *fp, bool take_ownership){

   C_zip_package_imp *zr = new(true) C_zip_package_imp(fp, take_ownership);
   if(!zr->Construct(NULL, *fp)){
      zr->Release();
      zr = NULL;
   }
   return zr;
}

//----------------------------
                              //zip-reader file

//----------------------------

class C_file_read_zip: public C_file::C_file_base{
   enum{ CACHE_SIZE = 0x2000 };
   byte base[CACHE_SIZE];

#ifdef SYMBIAN_OPTIM
   RFs file_session;
   RFile file;
#else
   C_file ck;
#endif
   C_file *arch_file;
   byte *top, *curr;
   dword curr_pos;         //real file pos
   int data_offs;

                           //decompression stream
   z_stream d_stream;

   byte d_buf[1024];
//----------------------------

   struct S_file_info{
      dword sz_compressed;    //compressed file size
      dword sz_uncompressed;  //uncompressed file size
      //dword file_time;        //in MS-DOS format
      //bool encrypted;
      enum{
         METHOD_STORE,
         METHOD_DEFLATE,
      } method;
   } file_info;

//----------------------------

   dword ReadCache();

//----------------------------
                              //zip package we're operating on
   C_smart_ptr<C_zip_package_imp> zip;

public:
//----------------------------
   C_file_read_zip(C_zip_package_imp *z):
      arch_file(NULL),
      curr_pos(0),
      zip(z)
   {
      MemSet(&d_stream, 0, sizeof(d_stream));
   }
//----------------------------
   ~C_file_read_zip(){
      if(!arch_file){
#ifdef SYMBIAN_OPTIM
         file.Close();
         CloseFileSession(file_session);
#else
         ck.Close();
#endif
      }
      inflateEnd(&d_stream);
   }
//----------------------------
   virtual C_file::E_FILE_MODE GetMode() const{ return C_file::FILE_READ; }
//----------------------------

   bool Open(const wchar *in_name, dword pack_version_needed);

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
   virtual bool IsEof() const{
      return (curr==top && GetFileSize()==GetCurrPos());
   }
//----------------------------
   virtual dword GetCurrPos() const{ return curr_pos+curr-top; }
//----------------------------

   virtual bool SetPos(dword pos){

      if(pos > GetFileSize())
         return false;
      if(pos<GetCurrPos()){
         if(pos >= curr_pos-(top-base)){
            curr = top-(curr_pos-pos);
         }else{
                              //rewind and decompress
            if(arch_file)
               arch_file->Seek(data_offs);
            else{
#ifdef SYMBIAN_OPTIM
               file.Seek(ESeekStart, data_offs);
#else
               ck.Seek(data_offs);
#endif
            }
            inflateEnd(&d_stream);
            MemSet(&d_stream, 0, sizeof(d_stream));
            inflateInit2(&d_stream, -MAX_WBITS);
            curr = top = base;
            curr_pos = 0;
            SetPos(pos);
         }
      }else
      if(pos > curr_pos){
                              //read from file until we've desired position in cache
         do{
            curr_pos += ReadCache();
         }while(pos > curr_pos);
         curr = base + (pos&(CACHE_SIZE-1));
      }else
         curr = top + pos - curr_pos;
      return true;
   }
//----------------------------
   virtual dword GetFileSize() const{ return file_info.sz_uncompressed; }
};

//----------------------------

bool C_file_read_zip::Open(const wchar *in_name, dword pack_version_needed){

                           //convert name to lowercase, and get length
   Cstr_w name = in_name;
   int name_len = name.Length();
   for(int i=name_len; i--; )
      name.At(i) = text_utils::LowerCase(name[i]);
                           //locate the filename among zip entries
   int i;
   const C_zip_package_imp::S_file_entry *file_entry = (C_zip_package_imp::S_file_entry*)zip->file_info;
   for(i=zip->num_files; i--; ){
      if(file_entry->CheckName(name, name_len))
         break;
      file_entry = (C_zip_package_imp::S_file_entry*)(((byte*)file_entry) + file_entry->GetStructSize());
   }
   if(i==-1)
      return false;
                              //compare packer version if requested
   if(pack_version_needed && file_entry->packer_version!=pack_version_needed)
      return false;
   arch_file = zip->arch_file;
   if(!arch_file){
                           //open the zip file for reading
#ifdef SYMBIAN_OPTIM
      RFs &fs = InitFileSession(file_session);
      TInt ir = file.Open(fs, TPtrC((word*)(const wchar*)zip->filename_full, zip->filename_full.Length()), EFileRead|EFileShareReadersOnly);
      if(ir!=KErrNone)
         return false;
#else
      if(!ck.Open((const wchar*)zip->filename_full))
         return false;
#endif
   }

                           //read local header
   struct S_LocalFileHeader{
      dword signature;     //0x04034b50 ("PK..")
      word _version_needed;  //version needed to extract
      word _flags;          //
      word compression;    //compression method
      word _lastModTime;    //last mod file time
      word _lastModDate;    // last mod file date
      dword _crc;           //
      dword _sz_compressed;   //may not be initialized!
      dword _sz_uncompressed; //    ''
      word filenameLen;    //length of the filename field following this structure
      word extraFieldLen;  //length of the extra field following the filename field

#ifdef SYMBIAN_OPTIM
      void Read(RFile &file){
         { TPtr8 ptr((byte*)&signature, 4); file.Read(ptr); }
         { TPtr8 ptr((byte*)&_version_needed, 2); file.Read(ptr); }
         { TPtr8 ptr((byte*)&_flags, 2); file.Read(ptr); }
         { TPtr8 ptr((byte*)&compression, 2); file.Read(ptr); }
         { TPtr8 ptr((byte*)&_lastModTime, 2); file.Read(ptr); }
         { TPtr8 ptr((byte*)&_lastModDate, 2); file.Read(ptr); }
         { TPtr8 ptr((byte*)&_crc, 4); file.Read(ptr); }
         { TPtr8 ptr((byte*)&_sz_compressed, 4); file.Read(ptr); }
         { TPtr8 ptr((byte*)&_sz_uncompressed, 4); file.Read(ptr); }
         { TPtr8 ptr((byte*)&filenameLen, 2); file.Read(ptr); }
         { TPtr8 ptr((byte*)&extraFieldLen, 2); file.Read(ptr); }
      }
#endif
      void Read(C_file &ck){
         { ck.Read(&signature, 4); }
         { ck.Read(&_version_needed, 2); }
         { ck.Read(&_flags, 2); }
         { ck.Read(&compression, 2); }
         { ck.Read(&_lastModTime, 2); }
         { ck.Read(&_lastModDate, 2); }
         { ck.Read(&_crc, 4); }
         { ck.Read(&_sz_compressed, 4); }
         { ck.Read(&_sz_uncompressed, 4); }
         { ck.Read(&filenameLen, 2); }
         { ck.Read(&extraFieldLen, 2); }
      }
   } lhdr;

   int offs = file_entry->local_offset;
   if(arch_file){
      arch_file->Seek(offs);
      lhdr.Read(*arch_file);
   }else{
#ifdef SYMBIAN_OPTIM
      file.Seek(ESeekStart, offs);
      lhdr.Read(file);
#else
      ck.Seek(offs);
      lhdr.Read(ck);
#endif
   }
   if(lhdr.signature!=0x04034b50)
      return false;

   d_stream.avail_in = 0;
   //d_stream.zalloc = &MemAlloc;
   //d_stream.zfree = &MemFree;
   int err = inflateInit2(&d_stream, -MAX_WBITS);
   if(err != Z_OK){
      assert(0);              //ZIP file decompression (inflateInit2) failed.
      return false;
   }
   file_info.sz_compressed = file_entry->size_compressed;//lhdr.sz_compressed;
   file_info.sz_uncompressed = file_entry->size_uncompressed;//lhdr.sz_uncompressed;
   file_info.method = (lhdr.compression==0) ? file_info.METHOD_STORE : file_info.METHOD_DEFLATE;
                           //skip other fields, and leave 'data_offs' positioned at beginning of data
   data_offs = lhdr.filenameLen + lhdr.extraFieldLen;
   if(arch_file){
      data_offs += arch_file->Tell();
      arch_file->Seek(data_offs);
   }else{
#ifdef SYMBIAN_OPTIM
      file.Seek(ESeekCurrent, data_offs);
#else
      data_offs += ck.Tell();
      ck.Seek(data_offs);
#endif
   }
   curr = top = base;
   return true;
}

//----------------------------

dword C_file_read_zip::ReadCache(){

   curr = base;

   if(file_info.method==file_info.METHOD_STORE){
                           //raw read, no compression
      dword sz = Min(int(CACHE_SIZE), int(GetFileSize()-curr_pos));
      if(arch_file)
         arch_file->Read(base, sz);
      else{
#ifdef SYMBIAN_OPTIM
         TPtr8 desc(base, sz);
         file.Read(desc);
#else
         ck.Read(base, sz);
#endif
      }
      top = base + sz;
      return sz;
   }
                           //read next data from cache
   d_stream.next_out = base;
   d_stream.avail_out = Min(int(CACHE_SIZE), int(file_info.sz_uncompressed-d_stream.total_out));

   dword num_read = d_stream.total_out;
   while(d_stream.avail_out){
      int flush_flag = 0;
      if(!d_stream.avail_in){
         dword sz = Min(int(sizeof(d_buf)), int(file_info.sz_compressed-d_stream.total_in));
         if(sz){
            if(arch_file)
               arch_file->Read(d_buf, sz);
            else{
#ifdef SYMBIAN_OPTIM
               TPtr8 desc(d_buf, sz);
               file.Read(desc);
#else
               ck.Read(d_buf, sz);
#endif
            }
            d_stream.next_in = d_buf;
            d_stream.avail_in = sz;
         }else
            flush_flag = Z_SYNC_FLUSH;
      }
      int err = inflate(&d_stream, flush_flag);
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
   num_read = d_stream.total_out - num_read;
   top = base + num_read;
   return num_read;
}

//----------------------------

bool C_file_zip::Open(const wchar *fname, const C_zip_package *zip, dword pack_version_needed){

   Close();
   if(!zip)
      return C_file::Open(fname, FILE_READ);
   if(zip->GetArchiveType()!=C_zip_package::TYPE_ZIP)
      return false;

   C_file_read_zip *cr = new(true) C_file_read_zip((C_zip_package_imp*)zip);
   imp = cr;
   if(!cr->Open(fname, pack_version_needed)){
      Close();
      return false;
   }
   return true;
}

//----------------------------
//----------------------------

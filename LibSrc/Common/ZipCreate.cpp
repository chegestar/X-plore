//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <Cstr.h>
#include <C_file.h>
#include <C_buffer.h>
#include "ZipRead.h"
#include <TimeDate.h>

#include <zlib.h>

//----------------------------
struct S_zip_create_file_info{
   dword local_hdr_offs;
   dword size_compr, size_uncompr;
   dword crc;
   dword time_date;
   Cstr_c rel_filename_utf8;
};

//----------------------------

bool CreateZipArchive(const wchar *zip_name, const wchar *base_path, const wchar *const filenames[], dword num_files, bool store_path_names){

   Cstr_w base_path_full;
   if(base_path){
      C_file::GetFullPath(base_path, base_path_full);
      if(base_path_full[base_path_full.Length()-1]!='\\')
         base_path_full<<'\\';
   }

   C_buffer<S_zip_create_file_info> files;
   files.Resize(num_files);
                              //prepare files
   for(dword i=0; i<num_files; i++){
      Cstr_w fn;
      C_file::GetFullPath(filenames[i], fn);

      C_file fl_src;
      if(!fl_src.Open(fn))
         return false;
      if(!store_path_names)
         fn = file_utils::GetFileNameNoPath(fn);
      else if(base_path && fn.Length()>base_path_full.Length() && base_path_full==fn.Left(base_path_full.Length())){
         fn = fn.RightFromPos(base_path_full.Length());
      }else{
#ifdef _WIN32_WCE
         fn = fn.RightFromPos(1);
#else
         fn = fn.RightFromPos(3);
#endif
      }
      files[i].rel_filename_utf8.ToUtf8(fn);
   }

   C_file fl;
   if(!fl.Open(zip_name, C_file::FILE_WRITE))
      return false;
                              //temp buffers
   const dword TMP_SIZE = 16384;
   C_buffer<byte> tmp_src, tmp_dst;
   tmp_src.Resize(TMP_SIZE);
   tmp_dst.Resize(TMP_SIZE);

   for(dword i=0; i<num_files; i++){
      S_zip_create_file_info &fi = files[i];

      C_file fl_src;
      if(!fl_src.Open(filenames[i]))
         continue;
      dword src_size = fl_src.GetFileSize();

      fi.crc = 0;
      fi.local_hdr_offs = fl.Tell();
      fi.size_uncompr = src_size;

      fi.time_date = 0;
      S_date_time td;
      if(C_file::GetFileWriteTime(filenames[i], td)){
         fi.time_date = (td.second) |
            (td.minute<<5) |
            (td.hour<<11) |
            ((td.day+1)<<16) |
            ((td.month+1)<<21) |
            ((td.year-1980)<<25);
      }
                              //write local file header
      fl.WriteDword(0x04034b50); //signature
      fl.WriteWord(20); //version_needed
      fl.WriteWord(0); //flags
      fl.WriteWord(8); //compression (deflate)
      fl.WriteDword(fi.time_date); //lastModTimeDate (DOS format)
      dword crc_offs = fl.Tell();
      fl.WriteDword(0); //crc
      dword compr_offs = fl.Tell();
      fl.WriteDword(0); //sz_compressed
      fl.WriteDword(src_size); //sz_uncompressed
      fl.WriteWord((word)fi.rel_filename_utf8.Length());  //filenameLen
      fl.WriteWord(0);  //extraFieldLen
      fl.Write((const char*)fi.rel_filename_utf8, fi.rel_filename_utf8.Length());

      z_stream zs;
      MemSet(&zs, 0, sizeof(zs));
      int err = deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 9, Z_DEFAULT_STRATEGY);
      assert(!err);

      zs.avail_in = 0;
      while(true){
         byte *src_ptr = tmp_src.Begin();
         if(zs.avail_in){
            if(src_ptr!=zs.next_in)
               MemCpy(src_ptr, zs.next_in, zs.avail_in);
            src_ptr += zs.avail_in;
         }
         dword rl = Min(src_size, TMP_SIZE-zs.avail_in);
         src_size -= rl;
         zs.avail_in += rl;
         const dword DST_TMP_SIZE = tmp_dst.Size();
         zs.avail_out = DST_TMP_SIZE;
         zs.next_in = tmp_src.Begin();
         zs.next_out = tmp_dst.Begin();
         if(rl){
            if(!fl_src.Read(src_ptr, rl)){
               assert(0);
            }
                              //compute crc (usin zlib function)
            fi.crc = crc32(fi.crc, src_ptr, rl);
         }
         err = deflate(&zs, zs.avail_in ? Z_NO_FLUSH : Z_FINISH);
         assert(err>=0);
         dword num_wr = DST_TMP_SIZE-zs.avail_out;
         C_file::E_WRITE_STATUS ws = fl.Write(tmp_dst.Begin(), num_wr);
         if(ws){
            fl.Close();
            fl.DeleteFile(zip_name);
            return false;
         }
         if(err==Z_STREAM_END)
            break;
      }
      fi.size_compr = zs.total_out;
      err = deflateEnd(&zs);
      assert(!err);

      {
                              //write back compressed size
         dword offs = fl.Tell();

         fl.Seek(compr_offs);
         fl.WriteDword(fi.size_compr);

         fl.Seek(crc_offs);
         fl.WriteDword(fi.crc);

         fl.Seek(offs);
      }
   }
   {
      dword central_offset = fl.Tell();

                              //write central directory
      for(dword i=0; i<files.Size(); i++){
         const S_zip_create_file_info &fi = files[i];

         fl.WriteDword(0x02014b50); //signature
         fl.WriteWord(20); //versionUsed
         fl.WriteWord(20); //versionNeeded
         fl.WriteWord(0); //flags
         fl.WriteWord(8); //compression
         fl.WriteDword(fi.time_date); //lastModTimeDate (DOS format)
         fl.WriteDword(fi.crc); //crc
         fl.WriteDword(fi.size_compr); //compressed_size
         fl.WriteDword(fi.size_uncompr); //uncompressed_size
         fl.WriteWord((word)fi.rel_filename_utf8.Length()); //filenameLen
         fl.WriteWord(0);  //extraFieldLen
         fl.WriteWord(0);  //commentLen
         fl.WriteWord(0);  //diskStart
         fl.WriteWord(0);  //internal
         fl.WriteDword(0);  //external
         fl.WriteDword(fi.local_hdr_offs);  //localOffset
         fl.Write((const char*)fi.rel_filename_utf8, fi.rel_filename_utf8.Length());
      }
      dword central_dir_size = fl.Tell() - central_offset;
                              //write end dir record
      fl.WriteDword(0x06054b50); //signature
      fl.WriteWord(0);  //thisDisk
      fl.WriteWord(0);  //dirStartDisk
      fl.WriteWord(word(files.Size()));  //numEntriesOnDisk
      fl.WriteWord(word(files.Size()));  //numEntriesTotal
      fl.WriteDword(central_dir_size); //size of the central directory
      fl.WriteDword(central_offset); //dirStartOffset
      fl.WriteWord(0); //commentLen
   }
   {
      C_file::E_WRITE_STATUS ws = fl.WriteFlush();
      if(ws){
         fl.Close();
         fl.DeleteFile(zip_name);
         return false;
      }
   }
   fl.Close();
   return true;
}

//----------------------------

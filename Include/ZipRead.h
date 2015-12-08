//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#ifndef __ZIPREAD_H_
#define __ZIPREAD_H_

#include <SmartPtr.h>
#include <C_file.h>

//----------------------------

class C_zip_package: public C_unknown{
public:
   enum E_TYPE{
      TYPE_ZIP,
      TYPE_RAR,
   };

//----------------------------
// Get identification of archive type.
   virtual E_TYPE GetArchiveType() const = 0;

   virtual const wchar *GetFileName() const = 0;

//----------------------------
// Get number of files in the opened zip file.
   virtual dword NumFiles() const = 0;

//----------------------------
// Enumerate file entries in archive.
   virtual void *GetFirstEntry(const wchar *&name, dword &length) const = 0;
   virtual void *GetNextEntry(void *handle, const wchar *&name, dword &length) const = 0;

//----------------------------
   virtual bool GetFileWriteTime(const wchar *filename, S_date_time &td) const = 0;

//----------------------------
// Open file from this archive. Returns NULL if file not found.
   virtual C_file *OpenFile(const wchar *fname) const = 0;

//----------------------------
// Create archive from file name.
   static C_zip_package *Create(const wchar *fname, bool is_app_data = false);

//----------------------------
// Create archive from file object. If take_ownership is true, fp is deleted by created class.
   static C_zip_package *Create(C_file *fp, bool take_ownership);
};

//----------------------------

                              //specialized file class, reading data from zip package
class C_file_zip: public C_file{
public:
//----------------------------
// Open file from zip package.
// If 'pack_version_needed' is nonzero, then opened file must be packed by specified zip version, otherwise it is not opened.
   bool Open(const wchar *fname, const C_zip_package *zip, dword pack_version_needed = 0);
};

//----------------------------
// Create zip archive from provided files.
bool CreateZipArchive(const wchar *zip_name, const wchar *base_path, const wchar *const filenames[], dword num_files, bool store_path_names = true);

//----------------------------
                              //raw zip file: writing compressed data, reading back
                              // file format has very simple header

                              //class for writing to file:
class C_file_raw_zip_write: public C_file{
public:
   enum{
      COMPRESSION_FAST = 1,
      COMPRESSION_DEFAULT = 5,
      COMPRESSION_BEST = 9,
   };
//----------------------------
// Open file for writing.
// compression may be anything between COMPRESSION_FAST to COMPRESSION_BEST.
   bool Open(const wchar *fname, int compression = COMPRESSION_DEFAULT);
   ~C_file_raw_zip_write();
};

//----------------------------
                              //class for reading from compressed file:
class C_file_raw_zip_read: public C_file{
public:
   bool Open(const wchar *fname);
};

//----------------------------

#endif

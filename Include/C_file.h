/*******************************************************
* Copyright (c) Lonely Cat Games  All rights reserved.
* Read/write using file system

Rules for locating files using filename:
1) full path including drive letter may be specified (e.g. C:\Data\System\text.txt)
2) when path starting with '\' (without drive letter) is given, then drive where executable is located is used (e.g. \Data\System\text.txt)
3) when plain filename is used, or filename with some path (e.g. System\text.txt or text.txt), then it's searched in directory of executable file
4) when path specifies drive letter and filename (e.g. C:test.txt), then path is substituted by 3), and specified drive letter is used

Platform-specific rules:
Windows Mobile:
   - filename may specify registry location, format of filename is: ":<root key>\path\key_name"
      root_key may be one of HKLM (HKEY_LOCAL_MACHINE), HKCU (HKEY_CURRENT_USER) or HKCR (HKEY_CLASSES_ROOT)

Symbian 9.1:
   - in case 3), the file is searched in application's private directory (\Private\<SID>\), where SID is typically application's UID3;

Android:
   - filename may specify asset (read-only file) in application package; such file is stored in assets\ folder in apk file; filename format is: <\<name>
   - filename may specify app's APK file (ZIP); constant filename for this is "<"

*******************************************************/
#ifndef __C_FILE_H
#define __C_FILE_H

#include <Rules.h>
#include <Util.h>
#include <Cstr.h>

//----------------------------
#if defined BADA || defined ANDROID
#define UNIX_FILE_SYSTEM      //file system with '/' path separators and unix access rights/ownerships on files and folders
#endif

#if defined _WIN32_WCE || defined UNIX_FILE_SYSTEM
#define NO_FILE_SYSTEM_DRIVES //OS doesn't have drive names, everything is in root drive
#endif

//----------------------------

class C_file{
public:
   enum E_FILE_MODE{
      FILE_NO,
      FILE_READ,
      FILE_WRITE,
                                 //modifiers
      FILE_WRITE_READONLY = 0x100, //replace also read-only file when writing
      FILE_CREATE_HIDDEN = 0x200,  //create the file (for write) as hidden
      FILE_WRITE_NOREPLACE = 0x400,//open file for write, but do not erase contents
      FILE_WRITE_CREATE_PATH = 0x800,//create full path to file (folders) if this doesn't exist
   };

   enum E_WRITE_STATUS{
      WRITE_OK,
      WRITE_FAIL,
      WRITE_DISK_FULL,
   };

                              //virtual file base
   class C_file_base{
   protected:
      static const char *ReadErr();
      static const char *WriteErr();
   public:
      virtual ~C_file_base(){}
      virtual E_FILE_MODE GetMode() const = 0;
      virtual bool Read(void *mem, dword len){ return false; }
      virtual E_WRITE_STATUS Write(const void *mem, dword len){ return WRITE_FAIL; }
      virtual E_WRITE_STATUS WriteFlush(){ return WRITE_FAIL; }

      virtual bool ReadByte(byte&){ return false; }
      bool ReadWord(word&);
      bool ReadDword(dword&);

      virtual bool IsEof() const = 0;
      virtual dword GetFileSize() const = 0;
      virtual dword GetCurrPos() const = 0;
      virtual bool SetPos(dword pos) = 0;
      virtual void SetCacheSize(dword sz){}
   };
//----------------------------
protected:
                              //pointer holding implementation class, or NULL
   C_file_base *imp;
public:
   C_file():
      imp(NULL)
   {}
   virtual ~C_file(){
      Close();
   }

//----------------------------

   bool Open(const wchar *fname, dword file_mode = FILE_READ);

//----------------------------

   void Close(){
      delete imp;
      imp = NULL;
   }

   inline E_FILE_MODE GetMode() const{ return !imp ? FILE_NO : imp->GetMode(); }

//----------------------------
// Read data and return status if reading was successful.
   inline bool Read(void *mem, dword len){
      return imp->Read(mem, len);
   }

//----------------------------
// Read data and return status if reading was successful.
   virtual bool ReadByte(byte &v){ return imp->ReadByte(v); }
   virtual bool ReadWord(word &v){ return imp->ReadWord(v); }
   virtual bool ReadDword(dword &v){ return imp->ReadDword(v); }

//----------------------------
// Write data to file, return status.
   inline E_WRITE_STATUS Write(const void *mem, dword len){ return imp->Write(mem, len); }

//----------------------------
// Flush written data to disk.
   virtual E_WRITE_STATUS WriteFlush(){ return imp->WriteFlush(); }

//----------------------------
// Write data - versions writing basic types.
   inline E_WRITE_STATUS WriteByte(byte b){ return imp->Write(&b, sizeof(byte)); }
   inline E_WRITE_STATUS WriteWord(word w){ return imp->Write(&w, sizeof(word)); }
   inline E_WRITE_STATUS WriteDword(dword d){ return imp->Write(&d, sizeof(dword)); }

//----------------------------
// Read single line from file (characters terminated by \n or \r\n) into buffer. Read line is terminated by \0. 'len' is lenght of buffer in characters, including space for last \0.
   void GetLine(char *buf, int len);
   void GetLine(wchar *buf, int len);

//----------------------------
// Read line from file. Same as previous, but without limit of line length.
   Cstr_c GetLine();

//----------------------------
// Write string to file.
   inline E_WRITE_STATUS WriteString(const char *s){ return imp->Write(s, StrLen(s)); }

//----------------------------
// Check if end of file is reached.
   inline bool IsEof() const{ return imp->IsEof(); }

//----------------------------
// Get total size of file. Works for read and written files.
   inline dword GetFileSize() const{ return imp->GetFileSize(); }

//----------------------------
// Return current read/write position in file.
   inline dword Tell() const{ return imp->GetCurrPos(); }

//----------------------------
// Set read/write position to new location.
   inline bool Seek(dword pos){ return imp->SetPos(pos); }

//----------------------------
   inline void SetCacheSize(dword sz){ imp->SetCacheSize(sz); }

//----------------------------
                              //utility functions for fast save/load
// Load from file. size specifies max size to load into buf. Returned value is number of bytes read, or -1 if read failed.
   static int Load1(const wchar *fn, void *buf, dword size);
   static bool Save(const wchar *fn, const void *buf, dword size);

//----------------------------
                              //static functions, which have something to do with files, but don't need services of the class
//----------------------------
   static bool DeleteFile(const wchar *filename);

//----------------------------
// Rename file or directory.
   static bool RenameFile(const wchar *old_name, const wchar *new_name);

//----------------------------
// Make sure given path exists; if it does not, it is created.
// The path may be absolute or relative to application.
// Last filename is ignored; if there's no filename, there should be a '\' at end of path.
   static bool MakeSurePathExists(const wchar *path);

//----------------------------
// Get fully-specified path from filename (which may already be full-path, or may be local-path).
   static void GetFullPath(const wchar *filename, Cstr_w &full_path);

//----------------------------
// Get/set file's or folder's last write time/date.
   static bool GetFileWriteTime(const wchar *filename, struct S_date_time &td);
   static bool SetFileWriteTime(const wchar *filename, const S_date_time &td);

//----------------------------
// Get/set file's or folder's attributes.
   enum{
      ATT_HIDDEN = 1,
      ATT_READ_ONLY = 2,
      ATT_SYSTEM = 4,
      ATT_ARCHIVE = 8,
      ATT_DIRECTORY = 0x10,
#ifdef UNIX_FILE_SYSTEM
                              //permissions USR = user, GRP = group, OTH = other; R = read, W = write, X = execute
      ATT_P_OTH_X = 0x100,
      ATT_P_OTH_W = 0x200,
      ATT_P_OTH_R = 0x400,
      ATT_P_GRP_X = 0x800,
      ATT_P_GRP_W = 0x1000,
      ATT_P_GRP_R = 0x2000,
      ATT_P_USR_X = 0x4000,
      ATT_P_USR_W = 0x8000,
      ATT_P_USR_R = 0x10000,
      ATT_ACCESS_READ = 0x20000, //file may be read
      ATT_ACCESS_WRITE = 0x40000, //file may be written
#endif
   };
   static bool GetAttributes(const wchar *filename, dword &atts);
   static bool SetAttributes(const wchar *filename, dword mask, dword value);

//----------------------------
// Check if given file exists.
   static bool Exists(const wchar *filename);

//----------------------------
// Copy file.
   static E_WRITE_STATUS CopyFile(const wchar *src, const wchar *dst);
};

//----------------------------
namespace file_utils{

//----------------------------
// Get the folder of file. Returned string is including last '\'.
template<class T>
T GetFilePath(const T &s){
   return s.Left(s.FindReverse('\\') + 1);
}

//----------------------------
// Get file name without path.
template<class T>
T GetFileNameNoPath(const T &s){
   return s.Right(s.Length() - s.FindReverse('\\') - 1);
}

//----------------------------
// Get valid filename from URL - strip path and additional data.
Cstr_c GetFileNameFromUrl(const char *cp);

//----------------------------
// Make unique mail filename.
// If 's' contains string, it is tried as first possibility (with appended extension).
void MakeUniqueFileName(const wchar *root_path, Cstr_w &s, const wchar *ext);

#ifdef UNIX_FILE_SYSTEM
//----------------------------
// Convert our path separators '\' to unix separators '/'.
Cstr_w ConvertPathSeparatorsToUnix(const Cstr_w &filename);
#endif
}//file_utils
//----------------------------

#endif   //__C_FILE_H

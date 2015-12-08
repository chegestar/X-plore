//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#ifndef __DIRECTORY_H_
#define __DIRECTORY_H_

#include <Rules.h>
#include <Cstr.h>
#include <C_file.h>

//----------------------------

class C_dir{
   void *find_data;           //platform-specific scan handle
public:
   C_dir():
      find_data(NULL)
   {}
   ~C_dir(){
      ScanEnd();
   }

   struct S_size{
      dword lo, hi;
   };
#ifdef NO_FILE_SYSTEM_DRIVES
//----------------------------
// Get total and free space on given folder's drive. The folder may be in device memory or on memory card (e.g. L"\\Windpows").
// If 'total' or 'free' is NULL, it's not set.
   static bool GetDriveSpace(const wchar *folder, S_size *total, S_size *free);

//----------------------------
// Begin searching for memory cards.
// To get actual data, use ScanGet. It returns raw name of card. For full path, use "\\" + card_name + "\\".
   bool ScanMemoryCards();

#else
                              //WinCE doesn't have drive letters, all folders are in root "\"
//----------------------------
// Return bit for all available drives on system. Lowest bit represents drive letter 'A'.
   static dword GetAvailableDrives();

//----------------------------
// Get total number of drives on system.
   static dword GetNumDrives();

//----------------------------
// Get letter of drive identified by index, where index is from 0 to number of drives - 1.
// The letter is in lower-case, or 0 if index is out of range.
   static char GetDriveLetter(int index);

//----------------------------
// Get drive name.
   static bool GetDriveName(char drive_letter, Cstr_w &name);

//----------------------------
// Get flags associated with drive.
   enum E_DRIVE_TYPE{
      DRIVE_NORMAL,           //normal read-write persistent drive
      DRIVE_READ_ONLY,        //drive can't be written to
      DRIVE_ROM,              //ROM drive
      DRIVE_RAMDISK,          //RAM-disk (content lost after reboot)
      DRIVE_REMOVABLE,        //Removable media (memory card)
      DRIVE_HARD_DISK,        //Hard disk drive
      DRIVE_REMOTE,           //remote drive (e.g. network drive)
      DRIVE_ERROR
   };
   static E_DRIVE_TYPE GetDriveType(char drive_letter);

//----------------------------
// Get total and free drive space. If 'total' or 'free' is NULL, it's not set.
   static bool GetDriveSpace(char drive_letter, S_size *total, S_size *free);
#endif   //!NO_FILE_SYSTEM_DRIVES

//----------------------------
// Create directory. If it already exists and 'success_if_exists' is set, return value is also true.
   static bool MakeDirectory(const wchar *path, bool success_if_exists = false);

//----------------------------
// Remove specified directory. Local or full path may be specified.
// If recursively is set, then all files and directories inside are also deleted.
   static bool RemoveDirectory(const wchar *path, bool recursively = false);

//----------------------------
// Begin scanning directory. 'dir' specifies directory to be scanned, ending '\' is optional. It may be relative path or full path.
   bool ScanBegin(const wchar *dir);

//----------------------------
   void ScanEnd();

//----------------------------
// Get next directory entry (file or sub-directory). Result is local filename, relative to scanned 'dir' directory, or NULL if no more files are found.
// Optionally function can return file's attributes, size and last modified time.
   const wchar *ScanGet(dword *attributes = NULL, dword *size = NULL, struct S_date_time *last_modify_time = NULL);
};

//----------------------------

#endif

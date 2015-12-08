//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <Directory.h>
#include <Util.h>
#include <C_file.h>
#include <TimeDate.h>

namespace win{
#include <Windows.h>
}
#undef GetDriveType
#undef DRIVE_NO_ROOT_DIR
#undef DRIVE_REMOVABLE
#undef DRIVE_RAMDISK
#undef DRIVE_REMOTE
#undef RemoveDirectory
#undef DeleteFile

//----------------------------

struct S_scan_data: public win::WIN32_FIND_DATA{
   win::HANDLE h;
   bool first_run;
#ifdef _WIN32_WCE
   bool is_card_scan;
#endif

   S_scan_data():
      first_run(true)
   {}
};

//----------------------------
#ifndef NO_FILE_SYSTEM_DRIVES

dword C_dir::GetNumDrives(){
   return CountBits(win::GetLogicalDrives());
}

//----------------------------

char C_dir::GetDriveLetter(int index){
   dword drvs = win::GetLogicalDrives();
   for(int i=0; i<32; i++){
      if(drvs&(1<<i)){
         if(!index--)
            return char('a' + i);
      }
   }
   return 0;
}

//----------------------------

dword C_dir::GetAvailableDrives(){
   return win::GetLogicalDrives();
}

//----------------------------

bool C_dir::GetDriveName(char drive_letter, Cstr_w &name){

   int drv_i = ToLower(drive_letter) - 'a';
                              //check if valid letter specified
   if(drv_i<0 || drv_i>=26)
      return false;
                              //check if the drive exists
   dword avl_drives = win::GetLogicalDrives();
   if(!(avl_drives&(1<<drv_i)))
      return false;
   int buf_size = win::GetLogicalDriveStrings(NULL, 0);
   wchar *buf = new(true) wchar[buf_size];

   dword l;
   l = win::GetLogicalDriveStrings(buf_size, buf);
   assert(l+1 == (dword)buf_size);
   wchar *n = buf;
   for(int i=0; i<drv_i; i++){
      if(avl_drives&(1<<i))
         n += StrLen(n) + 1;
   }
   name = n;
   delete[] buf;
   return true;
}

//----------------------------

C_dir::E_DRIVE_TYPE C_dir::GetDriveType(char drive_letter){

   if(!(win::GetLogicalDrives()&(1<<(ToLower(drive_letter)-'a'))))
      return DRIVE_ERROR;

   wchar root[4] = {drive_letter, ':', '\\', 0};
   switch(win::GetDriveTypeW(root)){
   case 1: return DRIVE_ERROR;   //DRIVE_NO_ROOT_DIR
   case 2: return DRIVE_REMOVABLE;  //DRIVE_REMOVABLE
   case 3: return DRIVE_HARD_DISK;  //DRIVE_FIXED
   case 4: return DRIVE_REMOTE;  //DRIVE_REMOTE
   case 5: return DRIVE_READ_ONLY;  //DRIVE_CDROM
   case 6: return DRIVE_RAMDISK; //DRIVE_RAMDISK
   }
   return DRIVE_NORMAL;
}

//----------------------------
#endif//!NO_FILE_SYSTEM_DRIVES

//----------------------------
#ifdef NO_FILE_SYSTEM_DRIVES

#ifdef _WIN32_WCE
namespace win{
#include <projects.h>
#pragma comment(lib, "note_prj.lib")
}

//----------------------------

bool C_dir::ScanMemoryCards(){

   S_scan_data *sd = (S_scan_data*)find_data;
   if(sd)
      win::FindClose(sd->h);
   else
      find_data = sd = new(true) S_scan_data;
   sd->is_card_scan = true;
   sd->h = win::FindFirstFlashCard(sd);
   if(sd->h==win::HANDLE(-1) || !sd->cFileName[0]){
      ScanEnd();
      return false;
   }
   return true;
}
#endif   //_WIN32_WCE

#endif//NO_FILE_SYSTEM_DRIVES
//----------------------------

bool C_dir::GetDriveSpace(
#ifdef NO_FILE_SYSTEM_DRIVES
   const wchar *root,
#else
   char drive_letter,
#endif
   S_size *total, S_size *free)
{
#ifndef NO_FILE_SYSTEM_DRIVES
   wchar root[4] = {drive_letter, ':', '\\', 0};
#endif

   win::ULARGE_INTEGER f, t;
#ifndef _WIN32_WCE
   win::SetErrorMode(SEM_FAILCRITICALERRORS);  //don't fail for drives with noninserted disks
#endif
   bool ok = win::GetDiskFreeSpaceEx(root, &f, &t, NULL);
#ifndef _WIN32_WCE
   win::SetErrorMode(0);
#endif
   if(!ok)
      return false;
   if(free){
      free->lo = f.LowPart;
      free->hi = f.HighPart;
   }
   if(total){
      total->lo = t.LowPart;
      total->hi = t.HighPart;
   }
   return true;
}

//----------------------------

bool C_dir::ScanBegin(const wchar *dir){

   S_scan_data *sd = (S_scan_data*)find_data;
   if(sd)
      win::FindClose(sd->h);
   else
      find_data = sd = new(true) S_scan_data;
#ifdef _WIN32_WCE
   sd->is_card_scan = false;
#endif

   Cstr_w dirw = dir;
   //C_file::GetFullPath(dir, dirw);
   if(dirw.Length() && dirw[dirw.Length()-1]!='\\')
      dirw<<'\\';
   dirw<<L"*.*";
#ifndef _WIN32_WCE
   win::SetErrorMode(SEM_FAILCRITICALERRORS);  //don't fail for drives with noninserted disks
#endif
   sd->h = win::FindFirstFile(dirw, sd);
#ifndef _WIN32_WCE
   win::SetErrorMode(0);
#endif
   if(sd->h==win::HANDLE(-1)){
      ScanEnd();
      return false;
   }
   return true;
}

//----------------------------

void C_dir::ScanEnd(){
   if(find_data){
      S_scan_data *sd = (S_scan_data*)find_data;
      win::FindClose(sd->h);
      delete sd;
      find_data = NULL;
   }
}

//----------------------------
/*
const wchar *C_dir::ScanGet(dword &atts){

   if(!find_data)
      return NULL;
   S_scan_data *sd = (S_scan_data*)find_data;
   if(sd->first_run)
      sd->first_run = false;
   else{
#ifdef _WIN32_WCE
      if(sd->is_card_scan){
         if(!win::FindNextFlashCard(sd->h, sd))
            return NULL;
         if(!*sd->cFileName)  //it can happen to be null string, ignore
            return ScanGet(atts);
      }else
#endif
      if(!win::FindNextFile(sd->h, sd))
         return NULL;
   }
   atts = 0;
   dword wa = sd->dwFileAttributes;
   if(wa&FILE_ATTRIBUTE_DIRECTORY){
                              //don't report dirs beginning with '.'
      if(sd->cFileName[0]=='.')
         return ScanGet(atts);
      atts |= C_file::ATT_DIRECTORY;
   }
   if(wa&FILE_ATTRIBUTE_HIDDEN) atts |= C_file::ATT_HIDDEN;
   if(wa&FILE_ATTRIBUTE_READONLY) atts |= C_file::ATT_READ_ONLY;
   if(wa&FILE_ATTRIBUTE_SYSTEM) atts |= C_file::ATT_SYSTEM;
   if(wa&FILE_ATTRIBUTE_ARCHIVE) atts |= C_file::ATT_ARCHIVE;
   return sd->cFileName;
}
*/
//----------------------------

const wchar *C_dir::ScanGet(dword *atts, dword *size, S_date_time *dt){

   if(!find_data)
      return NULL;
   S_scan_data *sd = (S_scan_data*)find_data;
   if(sd->first_run)
      sd->first_run = false;
   else{
#ifdef _WIN32_WCE
      if(sd->is_card_scan){
         if(!win::FindNextFlashCard(sd->h, sd))
            return NULL;
         if(!*sd->cFileName)  //it can happen to be null string, ignore
            return ScanGet(atts, size, dt);
      }else
#endif
      if(!win::FindNextFile(sd->h, sd))
         return NULL;
   }
   dword wa = sd->dwFileAttributes;
   if(wa&FILE_ATTRIBUTE_DIRECTORY){
      if(sd->cFileName[0]=='.' && (sd->cFileName[1]==0 || (sd->cFileName[1]=='.' && sd->cFileName[2]==0))){
                              //don't report dirs . and ..
         return ScanGet(atts, size, dt);
      }
   }
   if(atts){
      *atts = 0;
      if(wa&FILE_ATTRIBUTE_DIRECTORY) *atts |= C_file::ATT_DIRECTORY;
      if(wa&FILE_ATTRIBUTE_HIDDEN) *atts |= C_file::ATT_HIDDEN;
      if(wa&FILE_ATTRIBUTE_READONLY) *atts |= C_file::ATT_READ_ONLY;
      if(wa&FILE_ATTRIBUTE_SYSTEM) *atts |= C_file::ATT_SYSTEM;
      if(wa&FILE_ATTRIBUTE_ARCHIVE) *atts |= C_file::ATT_ARCHIVE;
   }
   if(size){
      if(wa&FILE_ATTRIBUTE_DIRECTORY)
         *size = 0;
      else
         *size = sd->nFileSizeLow;
   }
   if(dt){
      win::SYSTEMTIME st;
      if(win::FileTimeToSystemTime(&sd->ftLastWriteTime, &st)==TRUE){
         dt->year = st.wYear;
         dt->month = st.wMonth-1;
         dt->day = st.wDay-1;
         dt->hour = st.wHour;
         dt->minute = st.wMinute;
         dt->second = st.wSecond;

         dt->sort_value = dt->GetSeconds() + dt->GetTimeZoneMinuteShift()*60;
         dt->SetFromSeconds(dt->sort_value);
         dt->sort_value = dt->GetSeconds();
      }else{
         assert(0);
         MemSet(dt, 0, sizeof(*dt));
      }
   }
   return sd->cFileName;
}

//----------------------------

bool C_dir::MakeDirectory(const wchar *path, bool success_if_exists){
   if(win::CreateDirectory(path, NULL))
      return true;
   if(success_if_exists && win::GetLastError()==ERROR_ALREADY_EXISTS)
      return true;
   return false;
}

//----------------------------

bool C_dir::RemoveDirectory(const wchar *path, bool recursively){

   bool ok = true;
   if(recursively){
      C_dir d;
      if(d.ScanBegin(path)){
         while(true){
            dword atts;
            const wchar *wp = d.ScanGet(&atts);
            if(!wp)
               break;
            Cstr_w fn; fn<<path <<'\\' <<wp;
            if(atts&C_file::ATT_DIRECTORY)
               ok = (RemoveDirectory(fn, true) && ok);
            else
               ok = (C_file::DeleteFile(fn) && ok);
         }
         d.ScanEnd();
      }
   }
   Cstr_w dirw, tmp;
   tmp = path;
   if(*path && tmp[tmp.Length()-1]=='\\')
      tmp = tmp.Left(tmp.Length()-1);
   C_file::GetFullPath(tmp, dirw);
   return (win::RemoveDirectoryW(dirw) && ok);
}

//----------------------------

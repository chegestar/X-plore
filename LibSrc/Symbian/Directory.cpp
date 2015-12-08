//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <F32File.h>
#include <EikEnv.h>

#include <Directory.h>
#include <Util.h>
#include <C_file.h>
#include <TimeDate.h>

RFs &InitFileSession(RFs &file_session);
void CloseFileSession(RFs &file_session);

//----------------------------

static dword GetDriveMask(){

   RFs file_session;
   RFs &fs = InitFileSession(file_session);
   TDriveList dl;
   dword mask = 0;
   if(!fs.DriveList(dl)){
      for(int i=0; i<KMaxDrives; i++){
         if(dl[i])
            mask |= 1<<i;
      }
   }
   CloseFileSession(file_session);
   return mask;
}

//----------------------------

dword C_dir::GetNumDrives(){
   return CountBits(GetDriveMask());
}

//----------------------------

char C_dir::GetDriveLetter(int index){
   dword drvs = GetDriveMask();
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
   return GetDriveMask();
}

//----------------------------

bool C_dir::GetDriveName(char drive_letter, Cstr_w &name){

   RFs file_session;
   RFs &fs = InitFileSession(file_session);
   /*
   TBuf16<128> dname;
   if(fs.GetDriveName(ToLower(drive_letter)-'a', dname))
      return false;
   name = (wchar*)dname.PtrZ();
   */
   TVolumeInfo vi;
   if(fs.Volume(vi, ToLower(drive_letter)-'a')){
      CloseFileSession(file_session);
      return false;
   }
   CloseFileSession(file_session);
   name.Allocate((const wchar*)vi.iName.Ptr(), vi.iName.Length());
   return true;
}

//----------------------------

C_dir::E_DRIVE_TYPE C_dir::GetDriveType(char drive_letter){

   RFs file_session;
   RFs &fs = InitFileSession(file_session);
   TDriveInfo di;
   int err = fs.Drive(di, ToLower(drive_letter)-'a');
   CloseFileSession(file_session);
   if(err)
      return DRIVE_ERROR;

   switch(di.iType){
   case EMediaNotPresent: return DRIVE_ERROR;
   case EMediaCdRom: return DRIVE_READ_ONLY;
   case EMediaRom: return DRIVE_ROM;
   case EMediaRam: return DRIVE_RAMDISK;
   //case EMediaHardDisk: return DRIVE_HARD_DISK;  //not working
   case EMediaRemote: return DRIVE_REMOTE;
   }
   if(di.iDriveAtt&KDriveAttRemovable)
      return DRIVE_REMOVABLE;
   return DRIVE_NORMAL;
}

//----------------------------

bool C_dir::GetDriveSpace(char drive_letter, S_size *total, S_size *free){

   RFs file_session;
   RFs &fs = InitFileSession(file_session);
   TVolumeInfo vi;
   int err = fs.Volume(vi, ToLower(drive_letter)-'a');
   CloseFileSession(file_session);
   if(err)
      return false;
#ifndef __SYMBIAN_3RD__
   if(free){
      free->lo = vi.iFree.Low();
      free->hi = vi.iFree.High();
   }
   if(total){
      total->lo = vi.iSize.Low();
      total->hi = vi.iSize.High();
   }
#else
   if(free){
      free->lo = vi.iFree;
      free->hi = vi.iFree>>32;
   }
   if(total){
      total->lo = vi.iSize;
      total->hi = vi.iSize>>32;
   }
#endif
   return true;
}

//----------------------------

bool C_dir::MakeDirectory(const wchar *path, bool success_if_exists){

   RFs file_session;
   RFs &fs = InitFileSession(file_session);
   TFileName n;
   n.Copy(TPtrC((word*)path, StrLen(path)));
   if(n[n.Length()-1]!='\\')
      n.Append('\\');
   int err = fs.MkDir(n);
   CloseFileSession(file_session);
   if(success_if_exists && err==KErrAlreadyExists)
      return true;
   return (err==KErrNone);
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
   Cstr_w dirw;
   C_file::GetFullPath(path, dirw);

   RFs file_session;
   RFs &fs = InitFileSession(file_session);
   TFileName n;
   n.Copy(TPtrC((word*)(const wchar*)dirw, dirw.Length()));
   if(n[n.Length()-1]!='\\')
      n.Append('\\');
   ok = (fs.RmDir(n)==KErrNone && ok);
   CloseFileSession(file_session);
   return ok;
}

//----------------------------

struct S_scan_data{
   CDir *list;
   dword index;
   dword count;
   Cstr_w tmp;

   S_scan_data():
      index(0),
      count(0),
      list(NULL)
   {}
   ~S_scan_data(){
      delete list;
   }
};

//----------------------------

bool C_dir::ScanBegin(const wchar *dir){

   ScanEnd();
   S_scan_data *sd = new(true) S_scan_data;
   find_data = sd;

   Cstr_w dirw = dir;
   //C_file::GetFullPath(dir, dirw);
   if(dirw.Length() && dirw[dirw.Length()-1]!='\\')
      dirw<<'\\';
   TPtrC fptr((word*)(const wchar*)dirw, dirw.Length());

   RFs file_session;
   RFs &fs = InitFileSession(file_session);
   int err = fs.GetDir(fptr, KEntryAttHidden | KEntryAttSystem | KEntryAttDir, ESortNone, sd->list);
   CloseFileSession(file_session);
   if(err || !sd->list || !sd->list->Count()){
      ScanEnd();
      return false;
   }
   sd->count = sd->list->Count();
   return true;
}

//----------------------------

void C_dir::ScanEnd(){
   if(find_data){
      S_scan_data *sd = (S_scan_data*)find_data;
      delete sd;
      find_data = NULL;
   }
}

//----------------------------

const wchar *C_dir::ScanGet(dword *atts, dword *size, S_date_time *dt){

   if(!find_data)
      return NULL;

   S_scan_data *sd = (S_scan_data*)find_data;
   if(sd->index >= sd->count)
      return NULL;

   const TEntry &e = (*sd->list)[sd->index++];
   if(atts){
      *atts = 0;
      if(e.IsArchive()) *atts |= C_file::ATT_ARCHIVE;
      if(e.IsHidden()) *atts |= C_file::ATT_HIDDEN;
      if(e.IsReadOnly()) *atts |= C_file::ATT_READ_ONLY;
      if(e.IsSystem()) *atts |= C_file::ATT_SYSTEM;
      if(e.IsDir()) *atts |= C_file::ATT_DIRECTORY;
   }
   if(size)
      *size = e.iSize;
   if(dt){
      const TDateTime st = e.iModified.DateTime();
      dt->year = word(st.Year());
      dt->month = word(st.Month());
      dt->day = word(st.Day());
      dt->hour = word(st.Hour());
      dt->minute = word(st.Minute());
      dt->second = word(st.Second());

      dt->sort_value = dt->GetSeconds() + dt->GetTimeZoneMinuteShift()*60;
      dt->SetFromSeconds(dt->sort_value);
      dt->sort_value = dt->GetSeconds();
   }
   TFileName fn = e.iName;
   sd->tmp = (const wchar*)fn.PtrZ();
   return sd->tmp;
}

//----------------------------

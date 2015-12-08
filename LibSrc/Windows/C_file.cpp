//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <Util.h>
#include <C_file.h>
#include <TimeDate.h>
#include <String.h>
namespace win{
#include <Windows.h>
}

#ifndef _WIN32_WCE
#pragma comment(lib, "advapi32")
#endif

//----------------------------

#ifdef _WIN32_WCE

extern wchar base_dir[MAX_PATH];
wchar base_dir[MAX_PATH];
#define THROW(n) RaiseException((dword)C_file::n, 0, 0, 0)

#else

#define THROW(n) throw((int)C_file::n)

#endif

//----------------------------

static const wchar *ParseRegistryFileName(const wchar *fname, win::HKEY &root, wchar path[MAX_PATH]){

   assert(*fname==':');
   ++fname;
   root = 0;
   if(!MemCmp(fname, L"HKLM\\", 10))
      //root = HKEY_LOCAL_MACHINE;
      root = (win::HKEY)(win::ULONG_PTR)((win::LONG)0x80000002);
   else
   if(!MemCmp(fname, L"HKCU\\", 10))
      //root = HKEY_CURRENT_USER;
      root = (win::HKEY)(win::ULONG_PTR)((win::LONG)0x80000001);
   else
   if(!MemCmp(fname, L"HKCR\\", 10))
      //root = HKEY_CLASSES_ROOT;
      root = (win::HKEY)(win::ULONG_PTR)((win::LONG)0x80000000);
   else
      return NULL;
   fname += 5;
   const wchar *value = wcsrchr(fname, '\\');
   if(!value)
      return NULL;
   int i = 0;
   while(fname < value)
      path[i++] = *fname++;
   path[i] = 0;
   ++value;
   return value;
}

//----------------------------

class C_file_read_base: public C_file::C_file_base{
protected:
   byte *base, *top, *curr;
   dword curr_pos;         //real file pos

   C_file_read_base():
      base(NULL),
      curr_pos(0)
   {}

//----------------------------

   virtual C_file::E_FILE_MODE GetMode() const{ return C_file::FILE_READ; }

   virtual dword ReadCache() = 0;

//----------------------------

   virtual byte ReadByte(){
      if(curr==top){
         dword sz = ReadCache();
         if(sz < sizeof(byte)){
            //THROW(FILE_ERR_EOF);
            //Fatal("File", 1);
            LOG_RUN("C_file::ReadByte eof");
            return 0;
         }
         top = base + sz;
         curr_pos += sz;
      }
      return *curr++;
   }

//----------------------------

   virtual bool ReadByte(byte &v){
      if(curr==top){
         dword sz = ReadCache();
         if(sz < sizeof(byte))
            return false;
         top = base + sz;
         curr_pos += sz;
      }
      v = *curr++;
      return true;
   }

//----------------------------

   virtual bool IsEof() const{
      return (curr==top && GetFileSize()==GetCurrPos());
   }
   virtual dword GetCurrPos() const{ return curr_pos+curr-top; }
public:
   virtual bool Open(const wchar *fname) = 0;
};

//----------------------------

                           //read file
class C_file_read: public C_file_read_base{
   static const dword CACHE_SIZE = 0x2000;
   byte cache[CACHE_SIZE];
   dword real_cache_size;

   win::HANDLE hnd;
   dword file_size;

   virtual dword ReadCache(){
      curr = base;
      dword rd;
      win::ReadFile(hnd, base, real_cache_size, &rd, NULL);
      return rd;
   }
public:
   C_file_read():
      hnd(NULL),
      file_size(0),
      real_cache_size(CACHE_SIZE)
   {
      base = cache;
   }
   ~C_file_read(){
      win::CloseHandle(hnd);
   }

//----------------------------

   virtual bool Open(const wchar *fname){

      Cstr_w full_path;
      C_file::GetFullPath(fname, full_path);

#ifndef _WIN32_WCE
      win::SetErrorMode(SEM_FAILCRITICALERRORS);  //don't fail for drives with noninserted disks
#endif
      win::HANDLE h = win::CreateFile(full_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
         FILE_ATTRIBUTE_NORMAL,
         NULL);
#ifndef _WIN32_WCE
      win::SetErrorMode(0);
#endif
      if(h==win::HANDLE(-1)){
#ifdef _DEBUG
         int err = win::GetLastError();
         err = err;
#endif
         return false;
      }
      curr = top = base;
      hnd = h;
      file_size = win::GetFileSize(hnd, NULL);
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
         top = base + sz;
         curr_pos += sz;
         rl += rl1;
         if(!sz){
            read_len = rl;
            goto check_read_size;
            //return rl;
         }
      }
      MemCpy(mem, curr, len);
      curr += len;
      read_len = rl + len;
      //return rl + len;
check_read_size:
      return (read_len == wanted_len);
   }

//----------------------------

   virtual bool SetPos(dword pos){

      if(pos > GetFileSize())
         return false;
      if(pos<GetCurrPos()){
         if(pos >= curr_pos-(top-base)){
                              //position within cache
            curr = top-(curr_pos-pos);
         }else{
            curr = top = base;
            win::SetFilePointer(hnd, pos, NULL, FILE_BEGIN);
            curr_pos = pos;
         }
      }else
      if(pos > curr_pos){
         curr = top = base;
         win::SetFilePointer(hnd, pos, NULL, FILE_BEGIN);
         curr_pos = pos;
      }else{
                              //position within cache
         curr = top + pos - curr_pos;
      }
      return true;
   }
   virtual dword GetFileSize() const{ return file_size; }
   virtual void SetCacheSize(dword sz){
      if(!sz)
         sz = CACHE_SIZE;
      sz = Min(sz, CACHE_SIZE);
      real_cache_size = sz;
      curr_pos = GetCurrPos();
      curr = top = base;
   }
};

//----------------------------

class C_file_write_base: public C_file::C_file_base{
protected:
   static const dword CACHE_SIZE = 0x2000;
   byte cache[CACHE_SIZE];
   dword curr_pos;

   C_file_write_base():
      curr_pos(0)
   {}
public:
   virtual bool Open(const wchar *fname, dword flags) = 0;
};

//----------------------------
                           //write file
class C_file_write: public C_file_write_base{
   win::HANDLE hnd;
public:
   C_file_write():
      hnd(NULL)
   {}
   ~C_file_write(){
      if(hnd){
                              //flushing must be done before without error
         if(WriteFlush()){
            //Fatal("Write error");
            LOG_RUN("C_file::WriteFlush error");
         }
         win::CloseHandle(hnd);
      }
   }

//----------------------------

   virtual C_file::E_FILE_MODE GetMode() const{ return C_file::FILE_WRITE; }

//----------------------------

   virtual bool Open(const wchar *fname, dword flags){

      Cstr_w full_path;
      C_file::GetFullPath(fname, full_path);

      dword creation = CREATE_ALWAYS;
      if(flags&C_file::FILE_WRITE_NOREPLACE)
         creation = OPEN_EXISTING;
      dword flags_atts = FILE_ATTRIBUTE_NORMAL;
      if(flags&C_file::FILE_CREATE_HIDDEN)
         flags_atts |= FILE_ATTRIBUTE_HIDDEN;
      win::HANDLE h = win::CreateFile(full_path, GENERIC_WRITE, FILE_SHARE_READ, NULL, creation, flags_atts, NULL);
                              //clear read-only flags
      if(h==win::HANDLE(-1) && (flags&C_file::FILE_WRITE_READONLY) && win::GetLastError()==ERROR_ACCESS_DENIED){
         win::SetFileAttributes(full_path, win::GetFileAttributes(full_path) & ~FILE_ATTRIBUTE_READONLY);
         h = win::CreateFile(full_path, GENERIC_WRITE, FILE_SHARE_READ, NULL, creation, flags_atts, NULL);
      }
      if(h==win::HANDLE(-1))
         return false;
      hnd = h;
      return true;
   }

//----------------------------

   virtual C_file::E_WRITE_STATUS Write(const void *mem, dword len){

                              //put into cache
      while(curr_pos+len > CACHE_SIZE){
         dword sz = CACHE_SIZE - curr_pos;
         MemCpy(cache+curr_pos, mem, sz);
         curr_pos += sz;
         (byte*&)mem += sz;
         len -= sz;
         C_file::E_WRITE_STATUS ws = WriteFlush();
         if(ws)
            return ws;
      }
      MemCpy(cache+curr_pos, mem, len);
      curr_pos += len;
      return C_file::WRITE_OK;
   }

//----------------------------

   virtual C_file::E_WRITE_STATUS WriteFlush(){

      if(curr_pos){
         dword wr;
         bool ok = win::WriteFile(hnd, cache, curr_pos, &wr, NULL);
         curr_pos = 0;
         if(!ok){
            switch(win::GetLastError()){
            case ERROR_DISK_FULL:
               return C_file::WRITE_DISK_FULL;
            default:
               return C_file::WRITE_FAIL;
            }
         }
      }
      return C_file::WRITE_OK;
   }

//----------------------------

   virtual bool IsEof() const{ return true; }

   virtual dword GetCurrPos() const{
      const_cast<C_file_write*>(this)->WriteFlush();
      return win::SetFilePointer(hnd, 0, NULL, FILE_CURRENT);
   }

   virtual bool SetPos(dword pos){
      WriteFlush();
      win::SetFilePointer(hnd, pos, NULL, FILE_BEGIN);
      return true;
   }

   virtual dword GetFileSize() const{
      const_cast<C_file_write*>(this)->WriteFlush();
      return win::GetFileSize(hnd, NULL);
   }
};

//----------------------------

class C_file_read_registry: public C_file_read_base{
   win::HKEY hk;

   virtual dword ReadCache(){
                              //no more reading, since we already have all data read
      return 0;
   }
public:
   C_file_read_registry():
      hk(NULL)
   {}
   ~C_file_read_registry(){
      if(hk)
         win::RegCloseKey(hk);
      delete[] base;
   }

//----------------------------

   bool Open(const wchar *fname){

      win::HKEY root;
      wchar path[MAX_PATH];
      const wchar *value = ParseRegistryFileName(fname, root, path);
      if(!value)
         return false;

      if(win::RegOpenKeyEx(root, path, 0, KEY_QUERY_VALUE, &hk) != ERROR_SUCCESS)
         return false;

      int r = win::RegQueryValueEx(hk, value, NULL, NULL, NULL, &curr_pos);
      if(r!=ERROR_SUCCESS)
         return false;
      if(curr_pos){
         base = new(true) byte[curr_pos];
         r = win::RegQueryValueEx(hk, value, NULL, NULL, base, &curr_pos);
         if(r!=ERROR_SUCCESS)
            return false;
      }
      curr = base;
      top = base + curr_pos;
      return true;
   }

//----------------------------

   virtual bool Read(void *mem, dword len){

      dword left = top - curr;
      if(len > left)
         //THROW(FILE_ERR_EOF);
         return false;
      MemCpy(mem, curr, len);
      curr += len;
      return true;
   }

//----------------------------

   virtual bool SetPos(dword pos){
      if(pos > GetFileSize())
         return false;
      curr = base + pos;
      return true;
   }

//----------------------------

   virtual dword GetFileSize() const{
      return top-base;
   }
};

//----------------------------

class C_file_write_registry: public C_file_write_base{
   win::HKEY hk;

   wchar value_name[MAX_PATH];
public:
   C_file_write_registry():
      hk(NULL)
   {}
   ~C_file_write_registry(){
      if(hk){
                              //finalize - write contents
         win::RegSetValueEx(hk, value_name, 0, REG_BINARY, cache, curr_pos);
         win::RegCloseKey(hk);
      }
   }
   virtual C_file::E_FILE_MODE GetMode() const{ return C_file::FILE_WRITE; }

//----------------------------

   virtual bool Open(const wchar *fname, dword flags){

      win::HKEY root;
      wchar path[MAX_PATH];
      const wchar *value = ParseRegistryFileName(fname, root, path);
      if(!value)
         return false;
      StrCpy(value_name, value);

      dword disp;
      return (RegCreateKeyEx(root, path, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, &disp) == ERROR_SUCCESS);
   }

//----------------------------

   virtual C_file::E_WRITE_STATUS Write(const void *mem, dword len){

      if(len){
         if(curr_pos + len > CACHE_SIZE){
            LOG_RUN("C_file::Write reg write overflow");
            return C_file::WRITE_FAIL;
         }
         MemCpy(cache+curr_pos, mem, len);
         curr_pos += len;
      }
      return C_file::WRITE_OK;
   }

//----------------------------

   virtual C_file::E_WRITE_STATUS WriteFlush(){
      return C_file::WRITE_OK;
   }

//----------------------------

   virtual bool IsEof() const{ return true; }
   virtual dword GetCurrPos() const{
      return curr_pos;
   }
   virtual bool SetPos(dword pos){
      if(pos > CACHE_SIZE){
         LOG_RUN("C_file::Write SetPos overflow");
         return false;
      }
      curr_pos = pos;
      return true;
   }
   virtual dword GetFileSize() const{
      return curr_pos;
   }
};

//----------------------------
//----------------------------

bool C_file::Open(const wchar *fname, dword open_flags){

   Close();

#ifdef _DEBUG
                              //make sure the filename doesn't contain 2 slashes
   for(int i=1; fname[i]; i++){
      if(fname[i]=='\\' && fname[i-1]=='\\')
         return false;
   }
#endif

   bool is_registry = (*fname == ':');

   switch(open_flags&0xff){
   case FILE_READ:
      {
         C_file_read_base *cr;
         if(!is_registry)
            cr = new C_file_read;
         else
            cr = new C_file_read_registry;
         imp = cr;
         if(!cr->Open(fname)){
            Close();
            return false;
         }
      }
      break;
   case FILE_WRITE:
      {
         if(open_flags&FILE_WRITE_CREATE_PATH)
            MakeSurePathExists(fname);
         C_file_write_base *cw;
         if(!is_registry)
            cw = new C_file_write;
         else
            cw = new C_file_write_registry;
         imp = cw;
         if(!cw->Open(fname, open_flags)){
            Close();
            return false;
         }
      }
      break;
   default:
      LOG_RUN("C_file: inv mode");
      return false;
   }
   return true;
}

//----------------------------

#undef DeleteFile

bool C_file::DeleteFile(const wchar *fname){

   if(*fname == ':'){
      win::HKEY root;
      wchar path[MAX_PATH];
      const wchar *value = ParseRegistryFileName(fname, root, path);
      if(!value)
         return false;

      win::HKEY hk;
      if(win::RegOpenKeyEx(root, path, 0, KEY_SET_VALUE, &hk) != ERROR_SUCCESS)
         return false;
      bool ok = (win::RegDeleteValue(hk, value) == ERROR_SUCCESS);
      win::RegCloseKey(hk);
      return ok;
   }else{
      Cstr_w full_path;
      GetFullPath(fname, full_path);
      return (win::DeleteFileW(full_path)==TRUE);
   }
}

//----------------------------

bool C_file::RenameFile(const wchar *old_name, const wchar *new_name){

   Cstr_w old_full, new_full;
   GetFullPath(old_name, old_full);
   GetFullPath(new_name, new_full);
#ifndef _WIN32_WCE
                              //don't allow renaming (move) accross drives (helps to catch bugs)
   if(ToLower(old_full[0])!=ToLower(new_full[0]))
      return false;
#endif
   return win::MoveFile(old_full, new_full);
}

//----------------------------

bool C_file::MakeSurePathExists(const wchar *path){

   if(!*path || *path==':')
      return false;
//#ifdef _WIN32_WCE
   Cstr_w full_path;
   GetFullPath(path, full_path);
   path = full_path;
//#endif

   win::SECURITY_ATTRIBUTES sa;
   memset(&sa, 0, sizeof(sa));
   sa.nLength = sizeof(sa);
   dword cpos, npos = 0;
   Cstr_w tmp = path;
   wchar *str = &tmp.At(0);
   for(;;){
      cpos = npos;
      while(str[++npos]) if(str[npos]=='\\') break;
      if(cpos){
         str[cpos] = 0;
         win::CreateDirectory(str, &sa);
         str[cpos] = '\\';
      }
      if(!str[npos])
         break;
   }
   return true;
}

//----------------------------

void C_file::GetFullPath(const wchar *filename, Cstr_w &full_path){

   wchar path[256];
#ifdef _WIN32_WCE
   if(filename[0]=='\\')
#else
   if(filename[0] && filename[1]==':' && filename[2]=='\\')
#endif
   {
                              //already specified full path
      full_path = filename;
#ifndef _WIN32_WCE
   }else
   if(filename[0]=='\\'){
      if(StrLen(filename)+2 >= 255)
         return;
                              //Win32 - pre-append drive name from base dir
      wchar base_dir[MAX_PATH];
      win::GetCurrentDirectory(MAX_PATH, base_dir);
      assert(base_dir[1]==':');
      assert(base_dir[2]=='\\');
      path[0] = ToUpper(base_dir[0]);
      path[1] = ':';
      StrCpy(path+2, filename);
      full_path = path;
#endif //!_WIN32_WCE
   }else{
#ifndef _WIN32_WCE
                              //check if following format is given: 'C:filename'; then use specified drive letter
      wchar force_drive_letter = 0;
      if(filename[0] && filename[1]==':'){
         force_drive_letter = filename[0];
         filename += 2;
      }

      wchar base_dir[MAX_PATH];
      win::GetCurrentDirectory(MAX_PATH, base_dir);
      int len = StrLen(base_dir);
      base_dir[0] = ToUpper(base_dir[0]);
      base_dir[len] = '\\';
      base_dir[len+1] = 0;
      if(filename[0] && filename[1]==':')
         filename += 2;
#endif
      int n = StrCpy(path, base_dir);
      if(StrLen(filename)+n >= 255)
         return;
                              //check if filename specifies '..\\' to descend in path
      while(filename[0]=='.' && filename[1]=='.' && filename[2]=='\\'){
         filename += 3;
         while(n){
            --n;
            if(path[n-1]=='\\')
               break;
         }
      }
      StrCpy(path+n, filename);
#ifndef _WIN32_WCE
      if(force_drive_letter)
         path[0] = force_drive_letter;
#endif
      full_path = path;
   }
}

//----------------------------

bool C_file::GetFileWriteTime(const wchar *filename, S_date_time &td){

   bool ok;
   win::FILETIME ft;
   if(*filename == ':'){
#ifdef _WIN32_WCE
      return false;
#else
      win::HKEY root;
      wchar path[MAX_PATH];
      const wchar *value = ParseRegistryFileName(filename, root, path);
      if(!value)
         return false;

      win::HKEY hk;
      if(win::RegOpenKeyEx(root, path, 0, KEY_QUERY_VALUE, &hk) != ERROR_SUCCESS)
         return false;
      ok = (win::RegQueryInfoKey(hk, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &ft) == ERROR_SUCCESS);
      win::RegCloseKey(hk);
#endif
   }else{
      Cstr_w full_path;
      GetFullPath(filename, full_path);

      win::HANDLE h = win::CreateFile(full_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
      if(h==win::HANDLE(-1)){
                              //maybe it's directory, try it
         win::WIN32_FIND_DATA fd;
         h = win::FindFirstFile(full_path, &fd);
         if(h==win::HANDLE(-1))
            return false;
         ft = fd.ftLastWriteTime;
         win::FindClose(h);
      }else{
         ok = (win::GetFileTime(h, NULL, NULL, &ft)==TRUE);
         win::CloseHandle(h);
      }
   }
   if(ok){
      win::SYSTEMTIME st;
      ok = (win::FileTimeToSystemTime(&ft, &st)==TRUE);
      if(ok){
         td.year = st.wYear;
         td.month = st.wMonth-1;
         td.day = st.wDay-1;
         td.hour = st.wHour;
         td.minute = st.wMinute;
         td.second = st.wSecond;

         td.sort_value = td.GetSeconds() + td.GetTimeZoneMinuteShift()*60;
         td.SetFromSeconds(td.sort_value);
         td.sort_value = td.GetSeconds();
      }
   }
   return ok;
}

//----------------------------

bool C_file::SetFileWriteTime(const wchar *filename, const S_date_time &td){

   if(*filename == ':')
      return false;
   Cstr_w full_path;
   GetFullPath(filename, full_path);

   win::HANDLE h = win::CreateFile(full_path, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
   if(h==win::HANDLE(-1))
      return false;

   S_date_time td1;
   td1.SetFromSeconds(td.GetSeconds() - td1.GetTimeZoneMinuteShift()*60);

   win::SYSTEMTIME st;
   MemSet(&st, 0, sizeof(st));
   st.wYear = td1.year;
   st.wMonth = td1.month+1;
   st.wDay = td1.day+1;
   st.wHour = td1.hour;
   st.wMinute = td1.minute;
   st.wSecond = td1.second;

   win::FILETIME ft;
   win::SystemTimeToFileTime(&st, &ft);

   bool ok = (win::SetFileTime(h, NULL, NULL, &ft)==TRUE);
   win::CloseHandle(h);
   return ok;
}

//----------------------------

bool C_file::GetAttributes(const wchar *filename, dword &atts){
   if(*filename == ':')
      return false;
   Cstr_w full_path;
   GetFullPath(filename, full_path);
   dword wa = win::GetFileAttributes(full_path);
   if(wa==0xffffffff)
      return false;
   atts = 0;
   if(wa&FILE_ATTRIBUTE_ARCHIVE) atts |= ATT_ARCHIVE;
   if(wa&FILE_ATTRIBUTE_HIDDEN) atts |= ATT_HIDDEN;
   if(wa&FILE_ATTRIBUTE_READONLY) atts |= ATT_READ_ONLY;
   if(wa&FILE_ATTRIBUTE_SYSTEM) atts |= ATT_SYSTEM;
   if(wa&FILE_ATTRIBUTE_DIRECTORY) atts |= ATT_DIRECTORY;
   return true;
}

//----------------------------

bool C_file::SetAttributes(const wchar *filename, dword mask, dword value){

   if(*filename == ':')
      return false;
   Cstr_w full_path;
   GetFullPath(filename, full_path);
   dword wa = win::GetFileAttributes(full_path);
   if(wa==0xffffffff)
      return false;
   if(mask&ATT_ARCHIVE) wa &= ~FILE_ATTRIBUTE_ARCHIVE;
   if(mask&ATT_HIDDEN) wa &= ~FILE_ATTRIBUTE_HIDDEN;
   if(mask&ATT_READ_ONLY) wa &= ~FILE_ATTRIBUTE_READONLY;
   if(mask&ATT_SYSTEM) wa &= ~FILE_ATTRIBUTE_SYSTEM;

   if(value&ATT_ARCHIVE) wa |= FILE_ATTRIBUTE_ARCHIVE;
   if(value&ATT_HIDDEN) wa |= FILE_ATTRIBUTE_HIDDEN;
   if(value&ATT_READ_ONLY) wa |= FILE_ATTRIBUTE_READONLY;
   if(value&ATT_SYSTEM) wa |= FILE_ATTRIBUTE_SYSTEM;

   return (win::SetFileAttributes(full_path, wa)==TRUE);
}

//----------------------------

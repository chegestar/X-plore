//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <E32Std.h>
#include <util.h>
#include <c_file.h>
#include <fbs.h>
#include <EikEnv.h>
#include <s32file.h>
#include <msvapi.h>           //due to mail access
#include <msvids.h>
#include <AppBase.h>
#include <EikApp.h>
#include <EikAppUi.h>
#include <TimeDate.h>
#include <C_buffer.h>

//----------------------------

#ifdef __SYMBIAN_3RD__
#ifdef __WINS__
extern TEmulatorImageHeader uid;
#else
extern const dword uid;
#endif
#endif//!__SYMBIAN_3RD__

#ifdef __WINS__
extern const wchar app_path[];
#endif

//----------------------------

static bool SymbianIsMainThread(){
   return (GetGlobalData()->main_thread_id==RThread().Id());
}

//----------------------------

RFs &InitFileSession(RFs &file_session){
   bool main_thread = SymbianIsMainThread();
   RFs &fs = main_thread ? CEikonEnv::Static()->FsSession() : file_session;
   if(!main_thread)
      User::LeaveIfError(fs.Connect());
   return fs;
}

//----------------------------

void CloseFileSession(RFs &file_session){
   if(!SymbianIsMainThread())
      file_session.Close();
}

//----------------------------

class C_file_disk: public C_file::C_file_base{
protected:
   enum{ CACHE_SIZE = 0x2000 };

   RFs file_session;          //fs, used only from non-main thread (main thread uses static RFs due to better performance)
   RFile file;
   byte *base, *top, *curr;

   C_file_disk():
      base(NULL), top(NULL), curr(NULL)
   {}
   ~C_file_disk(){
      file.Close();
      CloseFileSession(file_session);
      delete[] base;
   }
};

//----------------------------

                           //read file
class C_file_read: public C_file_disk{
   dword curr_pos;         //real file pos
   int file_size;
   dword real_cache_size;

   dword ReadCache(){
      TPtr8 desc(curr=base, real_cache_size);
      int err = file.Read(desc);
      if(err)
         //Fatal("File read error", err);
         return 0;
      return desc.Size();
   }
public:
   C_file_read():
      curr_pos(0),
      real_cache_size(CACHE_SIZE),
      file_size(-1)
   {}

   virtual C_file::E_FILE_MODE GetMode() const{ return C_file::FILE_READ; }

//----------------------------

   inline bool Open(const wchar *fname){

      RFs &fs = InitFileSession(file_session);
      TInt ir = file.Open(fs, TPtrC((word*)fname, StrLen(fname)), EFileRead | EFileShareReadersOnly);
      if(ir!=KErrNone)
         return false;

      if(file.Size(file_size)!=KErrNone)
         file_size = -1;

      curr = top = base = new(true) byte[CACHE_SIZE];
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
      return (curr==top && (dword)file_size==GetCurrPos());
   }
   virtual dword GetCurrPos() const{ return curr_pos+curr-top; }

//----------------------------

   virtual bool SetPos(dword pos){
      if(pos>dword(file_size))
         return false;
      if(pos<GetCurrPos()){
         if(pos >= curr_pos-(top-base)){
            curr = top-(curr_pos-pos);
         }else{
            curr = top = base;
            TInt ipos = pos;
            file.Seek(ESeekStart, ipos);
            curr_pos = pos;
         }
      }else
      if(pos > curr_pos){
         curr = top = base;
         TInt ipos = pos;
         file.Seek(ESeekStart, ipos);
         curr_pos = pos;
      }else
         curr = top + pos - curr_pos;
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
                           //write file
class C_file_write: public C_file_disk{

public:
   ~C_file_write(){
                              //flushing should be done before without error
      WriteFlush();
   }

   virtual C_file::E_FILE_MODE GetMode() const{ return C_file::FILE_WRITE; }

//----------------------------

   inline bool Open(const wchar *fname, dword flags){

      TPtrC desc((word*)fname, StrLen(fname));
      RFs &fs = InitFileSession(file_session);
      TInt ir;
      if(flags&C_file::FILE_WRITE_NOREPLACE)
         ir = file.Open(fs, desc, EFileWrite);
      else
         ir = file.Replace(fs, desc, EFileWrite);
      if(ir==KErrAccessDenied && (flags&C_file::FILE_WRITE_READONLY)){
         fs.SetAtt(desc, 0, KEntryAttReadOnly);
         if(flags&C_file::FILE_WRITE_NOREPLACE)
            ir = file.Open(fs, desc, EFileWrite);
         else
            ir = file.Replace(fs, desc, EFileWrite);
      }
      if(ir!=KErrNone)
         return false;
      if(flags&C_file::FILE_CREATE_HIDDEN)
         file.SetAtt(KEntryAttHidden, 0);
      if(!(flags&C_file::FILE_WRITE_NOREPLACE))
         file.SetSize(0);
      curr = top = base = new(true) byte[CACHE_SIZE];
      return true;
   }

//----------------------------

   virtual C_file::E_WRITE_STATUS Write(const void *mem, dword len){

                              //put into cache
      while((curr-base)+len > CACHE_SIZE){
         dword sz = CACHE_SIZE - (curr-base);
         MemCpy(curr, mem, sz);
         curr += sz;
         top = curr;
         mem = (byte*)mem + sz;
         len -= sz;
         C_file::E_WRITE_STATUS ws = WriteFlush();
         if(ws)
            return ws;
      }
      MemCpy(curr, mem, len);
      curr += len;
      if(top<curr)
         top = curr;
      return C_file::WRITE_OK;
   }

//----------------------------

   virtual C_file::E_WRITE_STATUS WriteFlush();

//----------------------------

   virtual bool IsEof() const{ return true; }
   virtual dword GetCurrPos() const{
      const_cast<C_file_write*>(this)->WriteFlush();
      TInt ipos = 0;
      file.Seek(ESeekCurrent, ipos);
      return ipos;
   }
   virtual bool SetPos(dword pos){
      WriteFlush();
      TInt ipos = pos;
      file.Seek(ESeekStart, ipos);
      return true;
   }
   virtual dword GetFileSize() const{
      const_cast<C_file_write*>(this)->WriteFlush();
      TInt sz;
      if(file.Size(sz)!=KErrNone)
         return 0xffffffff;
      return sz;
   }
};

//----------------------------

C_file::E_WRITE_STATUS C_file_write::WriteFlush(){

   dword sz = top-base;
   if(sz){
      int err = file.Write(TPtrC8(base, sz));
      if(err){
                        //avoid flushing in destructor, so that this error is not repeated
         curr = top = base;
         switch(err){
         case KErrDiskFull:
            return C_file::WRITE_DISK_FULL;
         default:
            return C_file::WRITE_FAIL;
         }
      }
   }
   curr = top = base;
   return C_file::WRITE_OK;
}

//----------------------------
//----------------------------

bool C_file::Open(const wchar *fn, dword open_flags){

   Cstr_w fname;
   GetFullPath(fn, fname);

   Close();
   switch(open_flags&0xff){
   case FILE_READ:
      {
         C_file_read *cr = new(true) C_file_read;
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
         C_file_write *cw = new(true) C_file_write;
         imp = cw;
         if(!cw->Open(fname, open_flags)){
            Close();
            return false;
         }
      }
      break;
   default:
      return false;
   }
   return true;
}

//----------------------------

#ifndef __SYMBIAN_3RD__
static bool DeleteMailEntry(CMsvEntry *root, const TDesC &fname, dword size){

   bool ok = false;

   CMsvEntrySelection *chlds = root->ChildrenL();
   CleanupStack::PushL(chlds);

   for(int i=chlds->Count(); i-- && !ok; ){
      TMsvId id = (*chlds)[i];
      CMsvEntry *e = root->ChildEntryL(id);
      const TMsvEntry &en = e->Entry();

      if(en.StandardFolder()){
         ok = DeleteMailEntry(e, fname, size);
      }else{
                           //check if sizes match
         if(//e->HasDirectoryL() &&
            en.iSize==(int)size){
            TFileName fn;
            fn = en.iDescription;
            fn.LowerCase();
                           //check if names match
            if(fname==fn){
                           //note: we use a hack by comparing message title with filename (without base path), and sizes
                           // because it was not possible to determine full path of message's body...
               delete e;
               e = NULL;
               root->DeleteL(id);
               ok = true;
            }
         }
      }
      delete e;
   }
   CleanupStack::PopAndDestroy();   //chlds
   return ok;
}

//----------------------------

class C_observer: public MMsvSessionObserver{
   virtual void HandleSessionEvent(TMsvSessionEvent aEvent, TAny* aArg1, TAny* aArg2, TAny* aArg3){}
   virtual void HandleSessionEventL(TMsvSessionEvent aEvent, TAny* aArg1, TAny* aArg2, TAny* aArg3){}
public:
};

//----------------------------
// Delete message from mail system, which is associated with given file.
// 'fname' must be in lowercase
static void DeleteMail(const TDesC &fname, dword size){

                           //test whether file is in mail system
   if(fname.Length() > 15 && fname.Mid(1, 14)==_L(":\\system\\mail\\")){
      int slash = fname.LocateReverse('\\')+1;

      C_observer obs;
      CMsvSession *ms = CMsvSession::OpenSyncL(obs);
      CleanupStack::PushL(ms);
      CMsvEntry *root = ms->GetEntryL(KMsvRootIndexEntryId);
      CleanupStack::PushL(root);
      DeleteMailEntry(root, fname.Right(fname.Length()-slash), size);
      CleanupStack::PopAndDestroy(2);
   }
}
#endif

//----------------------------

bool C_file::DeleteFile(const wchar *filename){

#ifndef __SYMBIAN_3RD__
                              //get file size
   C_file ck;
   if(!ck.Open(filename))
      return false;
   dword filesize = ck.GetFileSize();
   ck.Close();
#endif
   Cstr_w fn;
   GetFullPath(filename, fn);

   RFs file_session;
   RFs &fs = InitFileSession(file_session);
   int err = fs.Delete(TPtrC((word*)(const wchar*)fn, fn.Length()));
   CloseFileSession(file_session);
   if(err!=KErrNone)
      return false;
#ifndef __SYMBIAN_3RD__
                  //also delete from the mail system
   fn.ToLower();
   DeleteMail(TPtrC((word*)(const wchar*)fn, fn.Length()), filesize);
#endif
   return true;
}

//----------------------------

bool C_file::RenameFile(const wchar *old_name, const wchar *new_name){

   Cstr_w old_full, new_full;
   GetFullPath(old_name, old_full);
   GetFullPath(new_name, new_full);

   RFs file_session;
   RFs &fs = InitFileSession(file_session);
   int err = fs.Rename(TPtrC((word*)(const wchar*)old_full, StrLen(old_full)), TPtrC((word*)(const wchar*)new_full, StrLen(new_full)));
   CloseFileSession(file_session);
   return (err==KErrNone);
}

//----------------------------

bool C_file::MakeSurePathExists(const wchar *path){

   Cstr_w full_path;
   GetFullPath(path, full_path);
   RFs file_session;
   RFs &fs = InitFileSession(file_session);
   int err = fs.MkDirAll(TPtrC((word*)(const wchar*)full_path, full_path.Length()));
   CloseFileSession(file_session);
   return (err==KErrNone);
}

//----------------------------

void C_file::GetFullPath(const wchar *filename, Cstr_w &full_path){

   if(filename[0] && filename[1]==':' && filename[2]=='\\'){
                              //already specified full path
      full_path = filename;
   }else{
      const S_global_data *gd = GetGlobalData();
      if(!gd){
                              //accessed from other thread, or when app is shutting down
         //Fatal("GetFullPath", 1);
         return;
      }
      const wchar *base_dir = gd->full_path;
      wchar path[256];
      if(filename[0]=='\\'){
                              //pre-append drive name from base dir
         assert(base_dir[1]==':');
         assert(base_dir[2]=='\\');
         path[0] = base_dir[0];
         path[1] = ':';
         StrCpy(path+2, filename);
#if defined __SYMBIAN_3RD__ && defined __WINS__
                              //change Z: to C: on emulator
         if(ToLower(path[0])=='z')
            path[0] = 'C';
#endif
      }else{
                              //check if following format is given: 'C:filename'; then use specified drive letter
         wchar force_drive_letter = 0;
         if(filename[0] && filename[1]==':'){
            force_drive_letter = filename[0];
            filename += 2;
         }
#ifdef __SYMBIAN_3RD__
                              //get access to private directory
         int n;
#ifdef __WINS__
         //if(!force_drive_letter)
         if(1)
         {
                              //in emulator, map private directory to E:\system\apps\<appname>
            n = StrCpy(path, L"E:\\");
            StrCpy(path+n, app_path);
            n += StrLen(app_path);
         }else
#endif//__WINS__
         {
            dword _uid =
#ifdef __WINS__
               uid.iUids[2].iUid;
#else
               uid;
#endif
            path[0] = base_dir[0];
            StrCpy(path+1, L":\\private\\");
            for(int i=0; i<8; i++){
               dword c = (_uid>>(4*(7-i))) & 0xf;
               if(c<10)
                  c += '0';
               else
                  c += 'a' - 10;
               path[11+i] = c;
            }
            n = 19;
         }
         path[n++] = '\\';
#else//__SYMBIAN_3RD__
         int n;
#ifdef __WINS__
         //if(!force_drive_letter){
         if(1){
                              //map app's dir to root of drive e
            n = StrCpy(path, L"E:\\");
            n += StrCpy(path+n, app_path);
            path[n++] = '\\';
         }else
#endif
         n = StrCpy(path, base_dir);
                                 //check if filename specifies '..\\' to descend in path
         while(filename[0]=='.' && filename[1]=='.' && filename[2]=='\\'){
            filename += 3;
            while(n){
               --n;
               if(path[n-1]=='\\')
                  break;
            }
         }
#endif//!__SYMBIAN_3RD__
         StrCpy(path+n, filename);
         if(force_drive_letter)
            path[0] = force_drive_letter;
      }
      full_path = path;
   }
}

//----------------------------

bool C_file::GetFileWriteTime(const wchar *filename, S_date_time &td){

   Cstr_w full_path;
   GetFullPath(filename, full_path);

   RFs file_session;
   RFs &fs = InitFileSession(file_session);
   TTime mt;
   TInt ir = fs.Modified(TPtrC((word*)(const wchar*)full_path, StrLen(full_path)), mt);
   CloseFileSession(file_session);

   if(ir!=KErrNone)
      return false;

   const TDateTime st = mt.DateTime();
   td.year = word(st.Year());
   td.month = word(st.Month());
   td.day = word(st.Day());
   td.hour = word(st.Hour());
   td.minute = word(st.Minute());
   td.second = word(st.Second());

   td.sort_value = td.GetSeconds() + td.GetTimeZoneMinuteShift()*60;
   td.SetFromSeconds(td.sort_value);
   td.sort_value = td.GetSeconds();

   return true;
}

//----------------------------

bool C_file::SetFileWriteTime(const wchar *filename, const S_date_time &td){

   Cstr_w full_path;
   GetFullPath(filename, full_path);

   RFs file_session;
   RFs &fs = InitFileSession(file_session);
   S_date_time td1;
   td1.SetFromSeconds(td.GetSeconds() - td1.GetTimeZoneMinuteShift()*60);

   TTime mt(TDateTime(td1.year, (TMonth)td1.month, td1.day, td1.hour, td1.minute, td1.second, 0));
   TInt ir = fs.SetModified(TPtrC((word*)(const wchar*)full_path, StrLen(full_path)), mt);
   CloseFileSession(file_session);

   return (ir==KErrNone);
}

//----------------------------

bool C_file::GetAttributes(const wchar *filename, dword &atts){

   Cstr_w full_path;
   GetFullPath(filename, full_path);

   RFs file_session;
   RFs &fs = InitFileSession(file_session);
   TUint sa;
   TInt ir = fs.Att(TPtrC((word*)(const wchar*)full_path, StrLen(full_path)), sa);
   CloseFileSession(file_session);

   if(ir!=KErrNone)
      return false;
   atts = 0;
   if(sa&KEntryAttArchive) atts |= ATT_ARCHIVE;
   if(sa&KEntryAttHidden) atts |= ATT_HIDDEN;
   if(sa&KEntryAttReadOnly) atts |= ATT_READ_ONLY;
   if(sa&KEntryAttSystem) atts |= ATT_SYSTEM;
   if(sa&KEntryAttDir) atts |= ATT_DIRECTORY;
   return true;
}

//----------------------------

bool C_file::SetAttributes(const wchar *filename, dword mask, dword value){

   Cstr_w full_path;
   GetFullPath(filename, full_path);
   TPtrC fn_desc((word*)(const wchar*)full_path, StrLen(full_path));

   RFs file_session;
   RFs &fs = InitFileSession(file_session);
   TUint sa;
   TInt ir = fs.Att(fn_desc, sa);

   if(ir!=KErrNone){
      CloseFileSession(file_session);
      return false;
   }
   value &= mask;
   mask ^= value;
   dword set = 0, clr = 0;

   if(mask&ATT_ARCHIVE) clr |= KEntryAttArchive;
   if(mask&ATT_HIDDEN) clr |= KEntryAttHidden;
   if(mask&ATT_READ_ONLY) clr |= KEntryAttReadOnly;
   if(mask&ATT_SYSTEM) clr |= KEntryAttSystem;

   if(value&ATT_ARCHIVE) set |= KEntryAttArchive;
   if(value&ATT_HIDDEN) set |= KEntryAttHidden;
   if(value&ATT_READ_ONLY) set |= KEntryAttReadOnly;
   if(value&ATT_SYSTEM) set |= KEntryAttSystem;

   ir = fs.SetAtt(fn_desc, set, clr);
   CloseFileSession(file_session);
   return (ir==KErrNone);
}

//----------------------------

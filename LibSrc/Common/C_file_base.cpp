//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <Util.h>
#include <C_file.h>
#include <C_vector.h>
#include <RndGen.h>

//----------------------------
#ifdef UNIX_FILE_SYSTEM
namespace file_utils{

Cstr_w ConvertPathSeparatorsToUnix(const Cstr_w &s){
   Cstr_w ret = s;
   for(int i=ret.Length(); i--; ){
      wchar &c = ret.At(i);
      if(c=='\\')
         c = '/';
   }
   return ret;
}

}//file_utils
#endif//UNIX_FILE_SYSTEM
//----------------------------

const char *C_file::C_file_base::ReadErr(){
   return "C_file: cannot read from file";
}
const char *C_file::C_file_base::WriteErr(){
   return "C_file: cannot write to file";
}

//----------------------------

bool C_file::C_file_base::ReadWord(word &v){
   if(!ReadByte(*(byte*)&v))
      return false;
   return ReadByte(*((byte*)&v+1));
}

//----------------------------

bool C_file::C_file_base::ReadDword(dword &v){
   if(!ReadWord(*(word*)&v))
      return false;
   return ReadWord(*((word*)&v+1));
}

//----------------------------

void C_file::GetLine(char *buf, int len){

   if(!imp) return;
   int num_chars = imp->GetFileSize() - imp->GetCurrPos();
   len = Min(len-1, num_chars);
   while(len--){
      byte c, x;
      if(!imp->ReadByte(c))
         break;
      if(c=='\r'){
         if(!imp->IsEof() && imp->ReadByte(x) && x!='\n')
            imp->SetPos(imp->GetCurrPos()-1);
         break;
      }
      if(c=='\n' || c==0)
         break;
      *buf++ = c;
   }
   *buf = 0;
}

//----------------------------

void C_file::GetLine(wchar *buf, int len){

   if(!imp) return;
   int num_chars = (imp->GetFileSize() - imp->GetCurrPos()) / sizeof(wchar);
   len = Min(len-1, num_chars);
   while(len--){
      wchar c;
      if(!imp->ReadWord((word&)c))
         break;
      if(c=='\r'){
         word x;
         imp->ReadWord(x);
         break;
      }
      if(c=='\n' || c==0)
         break;
      *buf++ = c;
   }
   *buf = 0;
}

//----------------------------

Cstr_c C_file::GetLine(){

   if(!imp) return NULL;
   int len = imp->GetFileSize() - imp->GetCurrPos();
   C_vector<char> buf;
   buf.reserve(256);
   while(len--){
      byte c;
      if(!imp->ReadByte(c))
         break;
      if(c=='\r'){
         byte x;
         imp->ReadByte(x);
         break;
      }
      if(c=='\n' || c==0)
         break;
      buf.push_back(c);
   }
   Cstr_c s;
   s.Allocate(buf.begin(), buf.size());
   return s;
}

//----------------------------

int C_file::Load1(const wchar *fn, void *buf, dword max_size){

   C_file fl;
   if(!fl.Open(fn))
      return -1;
   dword len = Min(max_size, fl.GetFileSize());
   if(!fl.Read(buf, len))
      return -1;
   return len;
}

//----------------------------

bool C_file::Save(const wchar *fn, const void *buf, dword size){

   C_file fl;
   if(!fl.Open(fn, FILE_WRITE))
      return false;
   C_file::E_WRITE_STATUS st = fl.Write(buf, size);
   if(st==fl.WRITE_OK)
      st = fl.WriteFlush();
   return (st==fl.WRITE_OK);
}

//----------------------------

bool C_file::Exists(const wchar *filename){
   dword att;
   return GetAttributes(filename, att);
}

//----------------------------

C_file::E_WRITE_STATUS C_file::CopyFile(const wchar *src, const wchar *dst_name){

   C_file fl_src;
   if(!fl_src.Open(src))
      return WRITE_FAIL;
   C_file::E_WRITE_STATUS err = C_file::WRITE_OK;
   C_file fl_dst;
   if(!fl_dst.Open(dst_name, C_file::FILE_WRITE | C_file::FILE_WRITE_READONLY))
      return WRITE_FAIL;
   int sz = fl_src.GetFileSize();
   const int BUF_SIZE = 16384;
   byte *buf = new(true) byte[BUF_SIZE];
   for(int i=0; i<sz; ){
      int copy_size = Min(sz-i, BUF_SIZE);
      if(!fl_src.Read(buf, copy_size)){
         err = WRITE_FAIL;
         break;
      }
      err = fl_dst.Write(buf, copy_size);
      if(err)
         break;
      i += copy_size;
   }
   delete[] buf;
   if(err || (err=fl_dst.WriteFlush(), err)){
      fl_dst.Close();
      DeleteFile(dst_name);
   }
   return err;
}

//----------------------------
namespace file_utils{

Cstr_c GetFileNameFromUrl(const char *url){

   int b, e;
   for(e=0; url[e]; e++){
      if(url[e]=='?')
         break;
   }
   for(b=e; b--; ){
      if(url[b]=='/' || url[b]=='\\')
         break;
   }
   ++b;
   Cstr_c ret;
   ret.Allocate(url+b, e-b);
                              //validate letters
   for(int i=ret.Length(); i--; ){
      char &c = ret.At(i);
      if(c<' ' || c>=0x7f)
         c = '_';
      switch(c){
      case '*':
      case '\\':
      case '/':
      case '|':
      case ':':
      case '\"':
      case '<':
      case '>':
      case '?':
         c = '_';
         break;
      }
   }
   return ret;
}

//----------------------------

void MakeUniqueFileName(const wchar *root_path, Cstr_w &s, const wchar *ext){

   C_rnd_gen rnd_gen;
   if(!C_file::MakeSurePathExists(root_path)){
      LOG_RUN("MakeUniqueFileName: MakeSurePathExists failed");
   }
   int fail_count = 0;
   Cstr_w orig_s = s;
   int num_digits = 2;
   wchar fn_rnd[9];
   while(true){
                              //gen random name (except for 1st loop, if 's' contains suggested name
      if(!s.Length()){
         for(int i=0; i<num_digits; i++){
            fn_rnd[i] = wchar('a'+rnd_gen.Get(26));
         }
         fn_rnd[num_digits] = 0;
         s<<fn_rnd;
         if(ext && *ext)
            s<<'.' <<ext;
      }else{
#ifdef ANDROID
         Cstr_w tmp = s;
         s<<'.' <<tmp;
#endif
         if(*ext){
            s<<L"." <<ext;
         }
      }
      Cstr_w fn; fn<<root_path <<s;
      if(!C_file::Exists(fn))
      //C_file fl;
      //if(!fl.Open(fn))
      {
         return;
         /*
         if(fl.Open(fn, C_file::FILE_WRITE)){
            fl.Close();
            C_file::DeleteFile(fn);
            return;
         }
         if(fail_count == 20){
                              //extension is probably invalid, change it to something valid (to avoid infinite loop)
            ext = L"bin";
         }
         */
      }
      ++fail_count;
      if(fail_count == 20){
                           //extension is probably invalid, change it to something valid (to avoid infinite loop)
         ext = L"bin";
      }
      if(fail_count<=10 && orig_s.Length()){
         s.Clear();
         s<<orig_s <<'[' <<(fail_count-1) <<']';
      }else{
         s.Clear();
         if(!(fail_count&3) && num_digits<8)
            ++num_digits;
      }
   }
}

}  //file_utils
//----------------------------

//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include "SocketBase.h"
#include <C_file.h>
#include <LogMan.h>

//----------------------------

C_socket_base::C_socket_base(C_internet_connection_base *_con, void *ec, const wchar *log_fn, int to, bool ssl):
   event_context(ec),
   con(_con),
   use_ssl(ssl),
   time_out(to),
   dst_ip(0), dst_port(0),
   max_cache_data(0),
   auto_receive_timeout(0)
{
#ifdef _DEBUG
   static dword si;
   socket_index = si++;
#endif
   if(log_fn){
      if(*log_fn=='!')
         ++log_fn;
      else
         C_file::DeleteFile(log_fn);
      log_file_name = log_fn;
   }
   con->sockets.push_back(this);
}

//----------------------------

C_socket_base::~C_socket_base(){
   DeleteAllBlocks();
   LOG_FULL("~finished\n");
}

//----------------------------

void C_socket_base::DeleteAllBlocks(){

   for(int i=blocks.size(); i--; )
      delete blocks[i];
   blocks.clear();
}

//----------------------------

void C_socket_base::AddToLog(const void *buf, dword len) const{

   C_logman log;
   log.Open(log_file_name, false);
   log.AddBinary(buf, len);
#ifdef DEBUG_LOG_
   char c = ((char*)buf)[len];
   if(c)
      ((char*)buf)[len] = 0;
   LOG_RUN((const char*)buf);
   if(c)
      ((char*)buf)[len] = c;
#endif
}

//----------------------------

dword C_socket_base::GetNumAvailableData() const{

   dword sz = 0;
   for(int i=blocks.size(); i--; )
      sz += blocks[i]->Size();
   return sz;
}

//----------------------------

void C_socket_base::DiscardData(dword size){

   assert(size<=GetNumAvailableData());
   while(size){
      assert(blocks.size());
      t_recv_block *bl = blocks.front();
      dword bl_sz = bl->Size();
      if(bl_sz<=size){
                              //remove entire block
         delete bl;
         blocks.pop_front();
         size -= bl_sz;
      }else{
                              //shrink block
         bl_sz -= size;
         assert(bl_sz>0);
         MemCpy(bl->Begin(), bl->Begin()+size, bl_sz);
         bl->Resize(bl_sz);
         size = 0;
      }
   }
}

//----------------------------

bool C_socket_base::PeekData(t_buffer &buf, dword min_size, dword max_size) const{

   dword copy_size = GetNumAvailableData();
   if(max_size)
      copy_size = Min(copy_size, max_size);
   if(!copy_size || copy_size<min_size)
      return false;
   buf.Resize(copy_size);
   char *dst = buf.Begin();
   for(int i=0; copy_size; i++){
      const t_recv_block *bl = blocks[i];
      dword sz = Min(bl->Size(), copy_size);
      MemCpy(dst, bl->Begin(), sz);
      dst += sz;
      copy_size -= sz;
   }
   return true;
}

//----------------------------

bool C_socket_base::GetData(t_buffer &buf, dword min_size, dword max_size){

   if(!PeekData(buf, min_size, max_size))
      return false;
   DiscardData(buf.Size());
                              //make next line get in text mode to succeed
   getline_has_new_data = true;
   return true;
}

//----------------------------

bool C_socket_base::GetLine(t_buffer &line){

   if(!getline_has_new_data)
      return false;

   int bi;

   //bool is_last_cr = false;   //flag if \r is last char of previous packet
   dword sz = 0;
   for(bi=0; bi<blocks.size(); bi++){
      const t_recv_block &bl = *blocks[bi];
      const int len = bl.Size();
      int ci;
      for(ci=0; ci<len; ci++){
         if(bl[ci]=='\n'){
            //if((!ci && is_last_cr) || (ci && bl[ci-1]=='\r'))
            {
               sz += ci + 1;
               break;
            }
         }
      }
      if(ci<len){
         assert(bl[ci]=='\n');
         break;
      }
      sz += len;
      //is_last_cr = (bl[len-1]=='\r');
   }
   if(bi==blocks.size()){
                              //we don't have complete line, wait for more data
      getline_has_new_data = false;
      return false;
   }
   assert(sz);
                              //alloc line (without \r\n, but with \0 at end)
   line.Resize(sz);
                           //copy full blocks
   dword di = 0;
   int i;
   for(i=0; i<bi; i++){
      t_recv_block *bl = blocks[i];
      int len = bl->Size();
      MemCpy(&line[di], bl->Begin(), len);
      di += len;
      delete bl;
   }
   {
      t_recv_block *bl = blocks[bi];
      dword sz1 = sz-di;
                              //copy data without last \n
      MemCpy(&line[di], bl->Begin(), sz1 - 1);
      if(bl->Size() == sz1){
                              //partial block empty
         delete bl;
         ++bi;
      }else{
                           //resize partial block
         int new_sz = bl->Size() - sz1;
         MemCpy(bl->Begin(), bl->Begin()+sz1, new_sz);
         bl->Resize(new_sz);
      }
   }
   if(line[sz-2]=='\r'){
      line[sz-2] = 0;
   }else
      line[sz-1] = 0;
   //line[sz-2] = 0;
                           //shift blocks
   if(bi)
      blocks.erase(blocks.begin(), blocks.begin()+bi);
   return true;
}

//----------------------------

void C_socket::SendString(const Cstr_c &s, int override_timeout, bool disable_logging){
   SendData((const char*)s, s.Length(), override_timeout, disable_logging);
}

//----------------------------

void C_socket::SendCString(const char *str, int override_timeout, bool disable_logging){
   SendData(str, StrLen(str), override_timeout, disable_logging);
}

//----------------------------

void C_socket_base::SetTimeout(int t){

   if(!t)
      t = con->time_out;
   time_out = t;
}

//----------------------------

class C_punycode{
                              //RFC 3492
   enum{
      base = 36,
      tmin = 1,
      tmax = 26,
      skew = 38,
      damp = 700,
      initial_bias = 72,
      initial_n = 0x80,
   };


   // Bias adaptation function

   static dword adapt(dword delta, dword numpoints, int firsttime){
      delta = firsttime ? delta / damp : delta >> 1;
      /* delta >> 1 is a faster way of doing delta / 2 */
      delta += delta / numpoints;
      dword k;
      for(k = 0; delta > ((base - tmin) * tmax) / 2; k += base)
         delta /= base - tmin;
      return k + (base - tmin + 1) * delta / (delta + skew);
   }

   static char encode_digit (dword d, int flag){
     return char(d + 22 + 75 * (d < 26) - ((flag != 0) << 5));
     //  0..25 map to ASCII a..z or A..Z
     //  26..35 map to ASCII 0..9
   }

   enum PUNYCODE_STATUS{
      PUNYCODE_SUCCESS = 0,
      PUNYCODE_BAD_INPUT = 1,     /* Input is invalid.                       */
      PUNYCODE_BIG_OUTPUT = 2,    /* Output would exceed the space provided. */
      PUNYCODE_OVERFLOW = 3       /* Wider integers needed to process input. */
   };
   static PUNYCODE_STATUS Encode(const Cstr_w &input, Cstr_c &output);
public:
   static bool HasSpecials(const Cstr_c &host_name){
      for(int i=host_name.Length(); i--; ){
         if(word(host_name[i])>=0x80)
            return true;
      }
      return false;
   }

   static Cstr_c PunycodeEncodePart(const Cstr_c &s){
      Cstr_c ret;
      int di = s.Find('.');
      if(di!=-1){
                              //convert separately 2 parts
         ret<<PunycodeEncodePart(s.Left(di))
            <<'.'
            <<PunycodeEncodePart(s.RightFromPos(di+1));

      }else{
         if(!HasSpecials(s))
            return s;
         Cstr_c tmp;
         Cstr_w ws = s.FromUtf8();
         Encode(ws, tmp);
         ret<<"xn--" <<tmp;
      }
      return ret;
   }
};

//----------------------------

C_punycode::PUNYCODE_STATUS C_punycode::Encode(const Cstr_w &input, Cstr_c &output){

   dword n = initial_n;
   dword bias = initial_bias;

   bool basic = false;
   dword input_len = input.Length();
                              //handle the basic code points
   for(dword j = 0; j < input_len; ++j){
      if(dword(input[j])<0x80){
          output<< //case_flags ? encode_basic (input[j], case_flags[j]) :
             (char)input[j];
          basic = true;
      }
   }
   dword h, b;
   h = b = output.Length();
                              //cannot overflow because output_length <= input_len <= maxint
                              //h is the number of code points that have been handled, b is the number of basic code points, and output_length is the number of ASCII code points that have been output
   if(basic)
      output<<'-';

                              //main encoding loop
   dword delta = 0;
   const dword maxint = dword(-1);

   while(h < input_len){
                              //all non-basic code points < n have been handled already.  Find the next larger one
      dword j, m;
      for(m = maxint, j = 0; j < input_len; ++j){
         if(input[j] >= n && input[j] < m)
            m = input[j];
      }
                              //increase delta enough to advance the decoder's <n,i> state to <m,0>, but guard against overflow
      if(m - n > (maxint - delta) / (h + 1))
         return PUNYCODE_OVERFLOW;
      delta += (m - n) * (h + 1);
      n = m;
      for(j = 0; j < input_len; ++j){
                              //Punycode does not need to check whether input[j] is basic
         if(input[j] < n //|| basic(input[j])
            ){
            if(++delta == 0)
               return PUNYCODE_OVERFLOW;
         }
         if(input[j] == n){
                              //pepresent delta as a generalized variable-length integer
            dword q, k;
            for(q = delta, k = base;; k += base){
               dword t = k <= bias // + tmin    +tmin not needed
                  ? tmin : k >= bias + tmax ? tmax : k - bias;
               if(q < t)
                  break;
               output<< encode_digit (t + (q - t) % (base - t), 0);
               q = (q - t) / (base - t);
            }
            output<< encode_digit (q, 0);//case_flags && case_flags[j]);
            bias = adapt(delta, h + 1, h == b);
            delta = 0;
            ++h;
         }
      }
      ++delta, ++n;
   }
   return PUNYCODE_SUCCESS;
}

//----------------------------

Cstr_c C_socket_base::PunyEncode(const Cstr_c &url){
   if(C_punycode::HasSpecials(url))
      return C_punycode::PunycodeEncodePart(url);
   return url;
}

//----------------------------

// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <Base64.h>
#include <Util.h>

//----------------------------

static bool DecodeBase64Char(char in, char &out){

   if(in>='A' && in <= 'Z')
      out = in - 'A';
   else
   if(in>='a' && in <= 'z')
      out = 26 + in - 'a';
   else
   if(in>='0' && in <= '9')
      out = 52 + in - '0';
   else
   if(in=='+')
      out = 62;
   else
   if(in=='/')
      out = 63;
   else{
      //out = 0;
      //assert(0);//in=='=');
      return false;
   }
   return true;
}

//----------------------------

bool DecodeBase64(const char *cp, int len, C_vector<byte> &out){

   if(!len)
      return false;

                              //skip invalid chars at end
   while(len){
      char c = cp[len-1];
      if(!(c==' ' || c=='\t' || c=='\n' || c=='\r'))
         break;
      --len;
   }
   /*
   if(len&3){
      assert(0);
      return false;
   }
   */
                              //determine size of data
   dword out_size = ((len+3)/4) * 3;
   switch(len&3){
   case 0:                    //try normal base64 with stuffed '='
      if(cp[len-1]=='='){
         --out_size;
         if(cp[len-2]=='=')
            --out_size;
      }
      break;
   case 1:                    //invalid possibility
      assert(0);
      return false;
   case 2:
      out_size -= 2;
      break;
   case 3:
      --out_size;
      break;
   }
   out.reserve(out_size);
   while(out_size){
      char a, b, c = 0, d = 0;
      DecodeBase64Char(*cp++, a);
      DecodeBase64Char(*cp++, b);
      if(out_size>1){
         DecodeBase64Char(*cp++, c);
         if(out_size>2)
            DecodeBase64Char(*cp++, d);
      }
      union{
         dword dw;
         struct{
            byte a, b, c;
         } s;
      } u;
      u.dw = (a<<18) | (b<<12) | (c<<6) | d;
      out.push_back(u.s.c);
      if(--out_size){
         out.push_back(u.s.b);
         if(--out_size){
            out.push_back(u.s.a);
            --out_size;
         }
      }
   }
   return true;
}

//----------------------------

static char EncodeBase64Char(byte in){

   if(in <= 25)
      return in + 'A';
   if(in>=26 && in <= 51)
      return in - 26 + 'a';
   if(in>=52 && in <= 61)
      return in - 52 + '0';
   switch(in){
   case 62: return '+';
   case 63: return '/';
   }
   assert(0);
   return 0;
}

//----------------------------

void EncodeBase64(const byte *cp, int len, C_vector<char> &out, bool fill_end_marks){

   while(len){
      int n = Min(len, 3);
      len -= n;

                              //get source bytes
      union{
         dword dw;
         struct{
            byte a, b, c;
         } s;
      } u;
      u.dw = 0;
      u.s.c = *cp++;
      switch(n){
      case 3:
         u.s.b = *cp++;
         u.s.a = *cp++;
         break;
      case 2:
         u.s.b = *cp++;
         break;
      }
                              //encode
      out.push_back(EncodeBase64Char(byte(u.dw >> 18)));
      out.push_back(EncodeBase64Char(byte((u.dw >> 12) & 0x3f)));
      if(n > 1){
         out.push_back(EncodeBase64Char(byte((u.dw >> 6) & 0x3f)));
         if(n > 2)
            out.push_back(EncodeBase64Char(byte(u.dw & 0x3f)));
         else
         if(fill_end_marks)
            out.push_back('=');
      }else
      if(fill_end_marks){
         out.push_back('=');
         out.push_back('=');
      }
   }
}

//----------------------------

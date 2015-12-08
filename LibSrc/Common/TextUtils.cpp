//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <TextUtils.h>
#include <TimeDate.h>
#include <Util.h>
#include <C_vector.h>
#include <Base64.h>

namespace text_utils{
//----------------------------

const char HTTP_PREFIX[] = "http://",
   HTTPS_PREFIX[] = "https://";
const wchar HTTP_PREFIX_L[] = L"http://",
   HTTPS_PREFIX_L[] = L"https://";

//----------------------------

wchar UpperCase(wchar c){

   if(c<128)
      return ToUpper(c);
                              //unicode
   if((c >= 0x104 && c < 0x122) ||
      (c >= 0x150 && c < 0x174))
      return c & -2;

   if((c >= 0x139 && c < 0x149) ||
      (c >= 0x179 && c < 0x17f)){
      if(!(c&1))
         --c;
      return c;
   }
                              //cyrillic
   if(c >= 0x430 && c < 0x450)
      return c - 0x20;
                              //specials
   switch(c){
   case 0x451: return 0x401;
   case 0xf7:
   case 0xff:
      break;
   default:
      if(c >= 0xe0 && c < 0x100)
         return c - 0x20;
   }
   return c;
}

//----------------------------

wchar LowerCase(wchar c){

   if(c<128)
      return (byte)ToLower(c);
                              //unicode
   if((c >= 0x104 && c < 0x122) ||
      (c >= 0x150 && c < 0x174))
      return c | 1;

   if((c >= 0x139 && c < 0x149) ||
      (c >= 0x179 && c < 0x17f)){
      if(c&1)
         ++c;
      return c;
   }
                              //cyrillic
   if(c >= 0x410 && c < 0x430)
      return c + 0x20;
                              //specials
   switch(c){
   case 0x401: return 0x451;
   case 0xd7:
   case 0xdf:
      break;
   default:
      if(c >= 0xc0 && c < 0xe0)
         return c + 0x20;
   }
   return c;
}

//----------------------------

Cstr_w GetTimeString(const S_date_time &dt, bool also_seconds){

   Cstr_w str;
   str.Format(also_seconds ? L"%:#02%:#02%" : L"%:#02%") <<(int)dt.hour <<(int)dt.minute;
   if(also_seconds)
      str<<(int)dt.second;
   return str;
}

//----------------------------

Cstr_w MakeFileSizeText(dword sz, bool short_form, bool allow_bytes){

   Cstr_w s;
   if(short_form){
      if(sz<1000){
         if(allow_bytes)
            s<<sz <<L'B';
         else
            //s.Format(L"0.%KB") <<(sz/100);
            s = L"0KB";
      }else{
         if(sz < 1024)
            sz += 512;
         sz /= 1024;
         if(!sz)
            sz = 1;
         if(sz < 1000){
            s<<sz <<L"KB";
         }else
         if(sz<1024*1024){
            s.Format(L"%.%MB") <<(sz/1024) <<((sz%1024+5)/103);
         }else{
            sz = (sz+512) / 1024;
            s.Format(L"%.%GB") <<(sz/1024) <<((sz%1024+5)/103);
         }
      }
      return s;
   }
                              //if < 10 KB: show x.xKB
                              //if 10-999KB: show xxxKB
                              //if < 10MB: show x.xMB
                              //else show xxx MB
   if(sz<1000){
      if(allow_bytes)
         s<<sz <<L'B';
      else
         s.Format(L"0.%KB") <<(sz/100);
   }else
   if(sz < 100*1024){
      int hi = sz/1024;
      int lo = sz&1023;
      //lo /= 103;
      lo = lo*100/1024;
      int d = lo/10;
      if(lo==d*10)
         s.Format(L"%.%KB") <<hi <<d;
      else
         s.Format(L"%.#02%KB") <<hi <<lo;
   }else
   if(sz < 1000*1024){
      sz += 512;
      sz /= 1024;
      s<<sz <<L"KB";
   }else{
      sz = (sz+512) / 1024;
      if(sz < 100*1024){
         int hi = sz/1024;
         int lo = sz&1023;
         //lo /= 103;
         lo = lo*100/1024;
         int d = lo/10;
         if(lo==d*10)
            s.Format(L"%.%MB") <<hi <<d;
         else
            s.Format(L"%.#02%MB") <<hi <<lo;
      }else{
         sz += 512;
         sz /= 1024;
         s<<sz <<L"MB";
      }
   }
   return s;
}

//----------------------------

bool ScanHexByte(const char *&cp, dword &c){

   char a = ToLower(*cp);
   dword ret = 0;

   if(IsDigit(a)) ret |= (a-'0')<<4;
   else
   if(a>='a' && a<='f') ret |= (10+a-'a')<<4;
   else{
      //assert(0);
      return false;
   }
   ++cp;
   char b = ToLower(*cp);
   if(IsDigit(b)) ret |= b-'0';
   else
   if(b>='a' && b<='f') ret |= 10+b-'a';
   else{
      //assert(0);
      --cp;
      return false;
   }
   ++cp;
   c = ret;
   return true;
}

//----------------------------

bool ScanDecimalNumber(const char* &cp, int &num){

   const char *in_cp = cp;
   bool ok = false;
   num = 0;
   int sign = 1;
   while(true){
      char c = *cp++;
      if(c<'0' || c>'9'){
         if(ok){
            --cp;
            num *= sign;
            return true;
         }
         switch(c){
         case 0:
            cp = in_cp;
            return false;
         case '-':
            sign = -1;
            break;
         case ' ':
         case '\t':
         case '+':
            break;
         default:
            cp = in_cp;
            return false;
         }
      }else{
         ok = true;
         num = num*10 + (c-'0');
      }
   }
}

//----------------------------

bool ScanHexNumber(const char* &cp, int &num){

   bool ok = false;
   num = 0;
   while(true){
      dword c;
      char b = ToLower(*cp);
      if(IsDigit(b)) c = b-'0';
      else
      if(b>='a' && b<='f') c = 10+b-'a';
      else
         break;
      ++cp;
      ok = true;
      num <<= 4;
      num |= c;
   }
   return ok;
}

//----------------------------

word MakeHexString(byte b, bool uppercase){

   dword ret;
   dword lo = b&0xf;
   dword hi = b>>4;
   ret = ((lo<10) ? (lo+'0') : (lo+(uppercase ? 'A' : 'a')-10)) << 8;
   ret |= (hi<10) ? (hi+'0') : (hi+(uppercase ? 'A' : 'a')-10);
   return (word)ret;
}

//----------------------------

const char *MakeHexString(byte b, dword &dw_tmp, bool uppercase){
   dw_tmp = MakeHexString(b, uppercase);
   return (const char*)&dw_tmp;
}

//----------------------------

Cstr_c DecodeCryptedString(const char *cp){

   dword len = StrLen(cp)/2;
   char *buf = new(true) char[len+1];
   for(dword i=0; i<len; i++){
      byte c = cp[i*2];
      dword ii = byte(cp[i*2+1]) ^ 0xca;
      c ^= ii;
      ii &= 7;
      c = byte(((c<<ii)&0xff) | ((c>>(8-ii))&0xff));
      buf[i] = c;
   }
   buf[len] = 0;
   Cstr_c s = buf;
   delete[] buf;
   return s;
}

//----------------------------

Cstr_c StringToBase64(const Cstr_c &s){

   C_vector<char> buf;
   EncodeBase64((const byte*)(const char*)s, s.Length(), buf);
   //buf.push_back(0);
   //return buf.begin();
   Cstr_c ret;
   ret.Allocate((const char*)buf.begin(), buf.size());
   return ret;
}

//----------------------------

Cstr_c Base64ToString(const Cstr_c &s){

   C_vector<byte> buf;
   DecodeBase64(s, s.Length(), buf);
   //buf.push_back(0);
   //return (const char*)buf.begin();
   Cstr_c ret;
   ret.Allocate((const char*)buf.begin(), buf.size());
   return ret;
}

//----------------------------

bool IsDigit(wchar c){
   return (c>='0' && c<='9');
}

//----------------------------

bool IsXDigit(wchar c){
   return (IsDigit(c) || (c>='a' && c<='f') || (c>='A' && c<='F'));
}

//----------------------------

bool IsAlNum(wchar c){
   return ((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9'));
}

//----------------------------

bool IsSpace(wchar c){
   return (c==' ' || c=='\t' || c=='\n' || c=='\r');
}

//----------------------------

void SkipWS(const char *&cp){
   while(*cp && (*cp==' ' || *cp=='\t'))
      ++cp;
}

//----------------------------

void SkipWSAndEol(const char *&cp){
   while(*cp && (*cp==' ' || *cp=='\t' || *cp=='\n' || *cp=='\r'))
      ++cp;
}

//----------------------------

bool CheckStringBegin(const char *&str, const char *kw, bool ignore_case, bool advance_str){

   const char *str1 = str;
   while(*kw){
      char c1 = *str1++;
      if(ignore_case)
         c1 = ToLower(c1);
      char c2 = *kw++;
      if(c1!=c2)
         return false;
   }
   if(advance_str)
      str = str1;
   return true;
}

//----------------------------

bool CheckStringBeginWs(const char *&str, const char *kw, bool ignore_case, bool advance_str){
   if(!CheckStringBegin(str, kw, ignore_case, advance_str))
      return false;
   SkipWS(str);
   return true;
}

//----------------------------

bool ReadQuotedString(const char *&cp, Cstr_c &str, char quote){

   if(*cp!=quote)
      return false;
   ++cp;
   int i = 0;
   while(cp[i] && cp[i]!=quote)
      ++i;
   str.Allocate(cp, i);
   cp += i;
   if(*cp)
      ++cp;
   return true;
}

//----------------------------

bool ReadToken(const char *&cp, Cstr_c &str, const char *specials){

                              //CTL =  <any ASCII control (0-31, 127)
   int i;
   for(i=0; ; i++){
      byte c = cp[i];
      int j;
      for(j=0; specials[j]; j++){
         if(specials[j]==c)
            break;
      }
      if(specials[j])
         break;
      if(c<32)
         break;
   }
   if(!i)
      return false;
   str.Allocate(cp, i);
   cp += i;
   return true;
}

//----------------------------

bool ReadWord(const char *&cp, Cstr_c &str, const char *specials){

   if(ReadQuotedString(cp, str))
      return true;
   if(ReadToken(cp, str, specials))
      return true;
   return false;
}

//----------------------------

const char specials_rfc_822[] = " ()<>@,;:\\\".[]\x7f";

//----------------------------

bool GetNextAtrrValuePair(const char *&cp, Cstr_c &attr, Cstr_c &val, bool val_to_lower){

   SkipWSAndEol(cp);
   if(!ReadToken(cp, attr, " \t\n=()<>@,;\\\"[]\x7f"))
      return false;
   attr.ToLower();
   SkipWSAndEol(cp);
   if(*cp!='=')
      return true;
   SkipWSAndEol(++cp);

   if(!ReadQuotedString(cp, val)){
      if(!ReadQuotedString(cp, val, '\'')){
         if(!ReadToken(cp, val, " >"))
            return false;
      }
   }
   if(val_to_lower)
      val.ToLower();
   return true;
}

//----------------------------
// day = "Mon" | "Tue" | "Wed" | "Thu" | "Fri" | "Sat" | "Sun"
static int ReadWeekDayShort(const char *&cp){

   if(CheckStringBegin(cp, "mon")) return 0;
   if(CheckStringBegin(cp, "tue")) return 1;
   if(CheckStringBegin(cp, "wed")) return 2;
   if(CheckStringBegin(cp, "thu")) return 3;
   if(CheckStringBegin(cp, "fri")) return 4;
   if(CheckStringBegin(cp, "sat")) return 5;
   if(CheckStringBegin(cp, "sun")) return 6;
   return -1;
}

//----------------------------

static int ReadWeekDayLong(const char *&cp){

   if(CheckStringBegin(cp, "monday")) return 0;
   if(CheckStringBegin(cp, "tuesday")) return 1;
   if(CheckStringBegin(cp, "wednesday")) return 2;
   if(CheckStringBegin(cp, "thursday")) return 3;
   if(CheckStringBegin(cp, "friday")) return 4;
   if(CheckStringBegin(cp, "saturday")) return 5;
   if(CheckStringBegin(cp, "sunday")) return 6;
   return -1;
}

//----------------------------

//----------------------------
// month =  "Jan" | "Feb" | "Mar" | "Apr" | "May" | "Jun" | "Jul" | "Aug" | "Sep" | "Oct" | "Nov" | "Dec"
static int ReadMonth(const char *&cp){

   if(CheckStringBegin(cp, "jan")) return 0;
   if(CheckStringBegin(cp, "feb")) return 1;
   if(CheckStringBegin(cp, "mar")) return 2;
   if(CheckStringBegin(cp, "apr")) return 3;
   if(CheckStringBegin(cp, "may")) return 4;
   if(CheckStringBegin(cp, "jun")) return 5;
   if(CheckStringBegin(cp, "jul")) return 6;
   if(CheckStringBegin(cp, "aug")) return 7;
   if(CheckStringBegin(cp, "sep")) return 8;
   if(CheckStringBegin(cp, "oct")) return 9;
   if(CheckStringBegin(cp, "nov")) return 10;
   if(CheckStringBegin(cp, "dec")) return 11;

                              //possibly represented by number
   int d;
   if(ScanDecimalNumber(cp, d))
      return d-1;
   //assert(0);
   return -1;
}

//----------------------------

static int ReadMonthLong(const char *&cp){

   if(CheckStringBegin(cp, "january")) return 0;
   if(CheckStringBegin(cp, "february")) return 1;
   if(CheckStringBegin(cp, "march")) return 2;
   if(CheckStringBegin(cp, "april")) return 3;
   if(CheckStringBegin(cp, "may")) return 4;
   if(CheckStringBegin(cp, "june")) return 5;
   if(CheckStringBegin(cp, "july")) return 6;
   if(CheckStringBegin(cp, "august")) return 7;
   if(CheckStringBegin(cp, "september")) return 8;
   if(CheckStringBegin(cp, "october")) return 9;
   if(CheckStringBegin(cp, "november")) return 10;
   if(CheckStringBegin(cp, "december")) return 11;

                              //possibly represented by number
   int d;
   if(ScanDecimalNumber(cp, d))
      return d-1;
   return -1;
}

//----------------------------

bool ReadDateTime_rfc822_rfc850(const char *&_cp, S_date_time &date, bool use_rfc_850, bool *missing_timezone){

   const char *cp = _cp;
   date.Clear();

                              //rfc822-date = [short_weekday "," ] SP date SP time SP zone
                              //rfc850-date = [long_weekday "," date time zone

   int week_day = (!use_rfc_850 ? ReadWeekDayShort : ReadWeekDayLong)(cp);
   if(week_day!=-1){
      SkipWS(cp);
      if(*cp++!=',')
         return false;
   }
                              //rfc822 date =  1*2DIGIT month 2*4DIGIT ;day month year
                              //rfc850-date = 1*2DIGIT "-" month "-" 2*4DIGIT ;day month year
                              // (support also non-conformant format 1*2DIGIT "." 1*2DIGIT "." 2*4DIGIT)
   int d, y;
   if(!ScanDecimalNumber(cp, d))
      return false;

   if(d<1 || d>31)
      return false;
   date.day = word(d - 1);

   SkipWS(cp);
   if(*cp=='.' || *cp=='-'){
      ++cp;
      SkipWS(cp);
   }

   int m = ReadMonthLong(cp);
   if(m==-1)
      m = ReadMonth(cp);
   if(m==-1)
      return false;
   date.month = word(m);

   SkipWS(cp);
   if(*cp=='.' || *cp=='-'){
      ++cp;
   }

   if(!ScanDecimalNumber(cp, y))
      return false;
   if(y<100){
                              //just guess what 2-digit year may mean
      if(y>=80)
         y += 1900;
      else
         y += 2000;
   }
   date.year = word(y);

                              //time = hour zone
                              //hour = 2DIGIT ":" 2DIGIT [":" 2DIGIT]
                              // zone =  "UT" | "GMT" ; Universal Time, North American : UT
                              // | "EST" | "EDT"                ;  Eastern:  - 5/ - 4
                              // | "CST" | "CDT"                ;  Central:  - 6/ - 5
                              // | "MST" | "MDT"                ;  Mountain: - 7/ - 6
                              // | "PST" | "PDT"                ;  Pacific:  - 8/ - 7
                              // | 1ALPHA                       ; Military: Z = UT; A:-1; (J not used); M:-12; N:+1; Y:+12
                              // | ( ("+" / "-") 4DIGIT )       ; local differential hours+min. (HHMM)
   SkipWS(cp);
   int h, n, s;
   if(!ScanDecimalNumber(cp, h))
      return false;
   bool neg_hour = false;
   if(h<0){
      neg_hour = true;
      h += 24;
   }
   date.hour = word(h);
   if(*cp++!=':')
      return false;
   if(!ScanDecimalNumber(cp, n))
      return false;
   date.minute = word(n);
   if(*cp==':'){
      ++cp;
      ScanDecimalNumber(cp, s);
      date.second = word(s);
   }
   date.MakeSortValue();
   if(neg_hour){
                              //fix negative hour
      date.SetFromSeconds(date.sort_value-60*60*24);
   }
   bool has_tz = false;
   {
                              //read zone
      int zone_delta_sec = 0;
      SkipWS(cp);
      if(*cp!='\n'){
         has_tz = true;
         if(*cp=='+' || *cp=='-'){
            bool neg = (*cp++=='-');
                              //hour
            zone_delta_sec = (*cp++ - '0') * 10;
            zone_delta_sec += *cp++ - '0';
            zone_delta_sec *= 60;
                              //minute
            zone_delta_sec += (*cp++ - '0') * 10;
            zone_delta_sec += (*cp++ - '0');
            zone_delta_sec *= 60;
            if(neg)
               zone_delta_sec = -zone_delta_sec;
         }else{
            if(CheckStringBegin(cp, "gmt") || CheckStringBegin(cp, "ut")){
                              //no shift
            }else if(CheckStringBegin(cp, "edt")){ zone_delta_sec = (60*60) * -4; }
            else if(CheckStringBegin(cp, "pst")){ zone_delta_sec = (60*60) * -8; }
            else if(CheckStringBegin(cp, "sgt")){ zone_delta_sec = (60*60) * 8; }
            else if(CheckStringBegin(cp, "pdt")){ zone_delta_sec = (60*60) * -7; }
            else if(CheckStringBegin(cp, "bst") || CheckStringBegin(cp, "ist") || CheckStringBegin(cp, "cet")){ zone_delta_sec = (60*60) * 1; }
            else if(CheckStringBegin(cp, "cst")){ zone_delta_sec = (60*60) * -6; }
            else if(CheckStringBegin(cp, "cest")){ zone_delta_sec = (60*60) * 2; }
            else if(CheckStringBegin(cp, "hkt")){ zone_delta_sec = (60*60) * 8; }
            else
            if(CheckStringBegin(cp, "pm")){
               assert(date.hour<=12);
               date.hour += 12;
               date.hour %= 24;
            }else if(CheckStringBegin(cp, "am")){
            }else{
               //assert(0);
               has_tz = false;
            }
         }
         date.sort_value -= zone_delta_sec;
      }
   }
   if(missing_timezone)
      *missing_timezone = !has_tz;
   date.sort_value += S_date_time::GetTimeZoneMinuteShift()*60;
   date.SetFromSeconds(date.sort_value);
   _cp = cp;
   return true;
}

//----------------------------

const wchar *GetExtension(const wchar *filename){

   for(int i=StrLen(filename); i--; ){
      wchar c = filename[i];
      if(c=='.')
         return &filename[i+1];
                              //detect invalid filename characters
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
         return NULL;
      }
   }
   return NULL;
}

//----------------------------

int FindKeyword(const char *&str, const char *keywords){

   for(int i=0; *keywords; i++){
      if(CheckStringBegin(str, keywords))
         return i;
      keywords += StrLen(keywords) + 1;
   }
   return -1;
}

//----------------------------

void MaskPassword(Cstr_w &str, int num_letters){

   if(num_letters==-1)
      num_letters = str.Length();
   for(int i=num_letters; i--; )
      str.At(i) = '*';
}

//----------------------------

int FindPreviousLineBeginning(const wchar *base, int byte_offs){

   base += byte_offs/sizeof(wchar);
   while(byte_offs){
      byte_offs -= sizeof(wchar);
      --base;
      if(base[-1]=='\n'){
         break;
      }
   }
   return byte_offs;
}

//----------------------------

int CompareStringsNoCase(const char *_cp1, const char *_cp2){

   const byte *cp1 = (byte*)_cp1;
   const byte *cp2 = (byte*)_cp2;
   while(true){
      dword c1 = ToLower((wchar)*cp1++);
      dword c2 = ToLower((wchar)*cp2++);
      if(c1<c2)
         return -1;
      if(c1>c2)
         return 1;
      if(!c1)
         return 0;
   }
}

//----------------------------

int CompareStringsNoCase(const wchar *wp1, const wchar *wp2){

   while(true){
      dword c1 = LowerCase(*wp1++);
      dword c2 = LowerCase(*wp2++);
      if(c1<c2)
         return -1;
      if(c1>c2)
         return 1;
      if(!c1)
         return 0;
   }
}

//----------------------------

dword ConvertCharToUTF8(wchar c, byte buf[4]){

   byte *beg_buf = buf;
   if(c<0x80){
                              //0000 0000 - 0000 007F:   0xxxxxxx
      *buf++ = byte(c);
   }else
   if(c<0x800){
                              //00000080 - 000007FF:   110xxxxx 10xxxxxx
      *buf++ = byte(0xc0 | (c>>6));
      *buf++ = byte(0x80 | (c&0x3f));
   }else{
                              //00000800 - 0000FFFF:   1110xxxx 10xxxxxx 10xxxxxx
      *buf++ = byte(0xe0 | (c>>12));
      *buf++ = byte(0x80 | ((c>>6)&0x3f));
      *buf++ = byte(0x80 | (c&0x3f));
   }
                              //add eol
   *buf = 0;
   return buf - beg_buf;
}

//----------------------------

wchar DecodeUtf8Char(const char *&cp){

   dword c = byte(*cp++);
   if((c&0xc0)==0xc0){
      dword code = 0;
      int i;
      for(i=5; i--; ){
         byte nc = *cp++;
         if(!nc)
            break;
         if((nc&0xc0) != 0x80){
                           //coding error
            return 0;
         }
         code <<= 6;
         code |= nc&0x3f;
         if(!(c&(2<<i)))
            break;
      }
      ++i;
      code |= (c & ((1<<i)-1)) << ((6-i)*6);
      c = code;
      //assert(c >= 0x20);
      if(c<' '){
         c = ' ';
         return 0;
      }
   }
   return wchar(c);
}

//----------------------------

void MakeFullUrlName(const char *domain, Cstr_c &name){

   assert((domain[4]==':' && domain[5]=='/' && domain[6]=='/') || (domain[5]==':' && domain[6]=='/' && domain[7]=='/'));
                              //check if domain present
   const char *cp = name;
   if(CheckStringBegin(cp, HTTP_PREFIX) || CheckStringBegin(cp, HTTPS_PREFIX))
      return;
                              //check if local path
   if(name[0]!='/'){
      if(name[0]=='.' && name[1]=='.' && name[2]=='/'){
                              //some ascending in hierarchy will occur
         Cstr_c dom = domain;
         int i;
         for(i=7; dom[i]; i++){
            if(dom[i]=='/')
               break;
         }
         do{
                              //remove ../ from name
            name = (const char*)name + 3;
            for(int j=dom.Length()-1; --j>=i; ){
               if(dom[j]=='/'){
                  dom.At(j+1) = 0;
                  dom = (const char*)dom;
                  break;
               }
            }
         }while(name[0]=='.' && name[1]=='.' && name[2]=='/');
         Cstr_c n = name;
         name.Format(NULL) <<dom <<n;
      }else{
         Cstr_c n = name;
         name.Clear();
         name<<domain;
         name<<n;
      }
      name.Build();
      return;
   }
                              //path specified from root, get pure domain name
   int i;
   for(i=8; domain[i]; i++){
      if(domain[i]=='/')
         break;
   }
   Cstr_c root;
   root.Allocate(domain, i);
   Cstr_c tmp;
   tmp<<root <<name;
   name = tmp;
}

//----------------------------

static void UrlEncodeString(const void *cp, bool wide, Cstr_c &str){

   while(true){
      dword c = GetChar(cp, wide);
      if(!c)
         break;
      if(c==' ')
         str<<'+';
      else
      if(!(
         (c>='a' && c<='z') ||
         (c>='A' && c<='Z') ||
         (c>='0' && c<='9') ||
         c=='-' || c=='_' || c=='.')){

         byte utf8[4], *cp1 = utf8;
         if(wide)
            ConvertCharToUTF8(wchar(c), utf8);
         else{
            utf8[0] = byte(c);
            utf8[1] = 0;
         }
         do{
            c = *cp1++;
            str<<'%';
            c = MakeHexString(byte(c));
            str<<char(c&0xff);
            str<<char(c>>8);
         }while(*cp1);
      }else
         str<<char(c);
   }
   str.Build();
}

//----------------------------

Cstr_c UrlEncodeString(const char *cp){
   Cstr_c ret;
   UrlEncodeString(cp, false, ret);
   return ret;
}

//----------------------------

Cstr_c UrlEncodeString(const wchar *cp){
   Cstr_c ret;
   UrlEncodeString(cp, true, ret);
   return ret;
}

//----------------------------
}

//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <UI\UserInterface.h>
#include <TextUtils.h>
#include <UI\SkinnedApp.h>
#include <C_math.h>

//----------------------------

static const dword COLOR_BLACK = 0xff000000;

//----------------------------

dword encoding::ConvertCodedCharToUnicode(dword c, E_TEXT_CODING coding){

   switch(coding){
   case COD_CYRILLIC_WIN1251:
                              //map cyrillic alphabet to unicode range
      if(c>=0xc0 && c<=0xff)
         return c-0xc0+0x410;
      switch(c){
      case 0xa8: return 0x401;    //'E', PK_2DOT
      case 0xaa: return 0x404;
      case 0xaf: return 0x407;
      case 0xb2: return 0x406;
      case 0xb3: return 0x456;
      case 0xb8: return 0x451;   //'e', PK_2DOT
      case 0xba: return 0x454;
      case 0xbf: return 0x457;
      }
      break;

   case COD_CYRILLIC_ISO:
      switch(c){
      case 0xad: return c;
      case 0xf0: return 0x2116;
      case 0xfd: return 0xa7;
      }
      if(c>=0xa1 && c<=0xff)
         return c-0xa0+0x400;
      break;

   case COD_KOI8_R:
   case COD_KOI8_U:
      if(c>=0xc0 && c<=0xff){
         static const wchar koi8_r_tab[64] = {
            /*
            0x2500,0x2502,0x250c,0x2510,0x2514,0x2518,0x251c,0x2524,
            0x252c,0x2534,0x253c,0x2580,0x2584,0x2588,0x258c,0x2590,
            0x2591,0x2592,0x2593,0x2320,0x25a0,0x2219,0x221a,0x2248,
            0x2264,0x2265,0x00a0,0x2321,0x00b0,0x00b2,0x00b7,0x00f7,
            0x2550,0x2551,0x2552,0x0451,0x2553,0x2554,0x2555,0x2556,
            0x2557,0x2558,0x2559,0x255a,0x255b,0x255c,0x255d,0x255e,
            0x255f,0x2560,0x2561,0x0401,0x2562,0x2563,0x2564,0x2565,
            0x2566,0x2567,0x2568,0x2569,0x256a,0x256b,0x256c,0x00a9,
             */
                              //c0
            0x044e,0x0430,0x0431,0x0446,0x0434,0x0435,0x0444,0x0433,  0x0445,0x0438,0x0439,0x043a,0x043b,0x043c,0x043d,0x043e,
                              //d0
            0x043f,0x044f,0x0440,0x0441,0x0442,0x0443,0x0436,0x0432,  0x044c,0x044b,0x0437,0x0448,0x044d,0x0449,0x0447,0x044a,
                              //e0
            0x042e,0x0410,0x0411,0x0426,0x0414,0x0415,0x0424,0x0413,  0x0425,0x0418,0x0419,0x041a,0x041b,0x041c,0x041d,0x041e,
                              //f0
            0x041f,0x042f,0x0420,0x0421,0x0422,0x0423,0x0416,0x0412,  0x042c,0x042b,0x0417,0x0428,0x042d,0x0429,0x0427,0x042a
         };
         if(coding==COD_KOI8_U){
            switch(c){
            case 0xa4: return 0x0454;
            case 0xa6: return 0x0456;
            case 0xa7: return 0x0457;
            case 0xad: return 0x0491;
            case 0xb4: return 0x0404;
            case 0xb6: return 0x0406;
            case 0xb7: return 0x0407;
            case 0xbd: return 0x0490;
            }
         }
         return koi8_r_tab[c-0xc0];
      }
      switch(c){
      case 0x9a: return 0xa0;
      case 0x9c: return 0xb0;
      case 0x9d: return 0xb2;
      case 0x9e: return 0xb7;
      case 0x9f: return 0xf2;
      case 0xa3: return 0x0451;
      case 0xb3: return 0x0401;
      case 0xbf: return 0xa9;
      }
      break;

   case COD_8859_13:
      switch(c){
      case 0xa1: return '"';
      case 0xa5: return '"';
      case 0xa8: return 216;
      case 0xaa: return 0x156;
      case 0xaf: return 0xc6;
      case 0xb8: return 248;
      case 0xba: return 0x157;
      case 0xbf: return 0xe6;

      case 0xc0: return 0x104;
      case 0xc1: return 0x12e;
      case 0xc2: return 0x100;
      case 0xc3: return 0x106;
      case 0xc7: return 0x118;
      case 0xc8: return 0x112;
      case 0xc9: return 0x10c;
      case 0xca: return 0x179;
      case 0xcb: return 0x116;
      case 0xcc: return 0x122;
      case 0xcd: return 0x136;
      case 0xce: return 0x12a;
      case 0xcf: return 0x13b;

      case 0xd0: return 0x160;
      case 0xd1: return 0x143;
      case 0xd2: return 0x145;
      case 0xd4: return 0x14c;
      case 0xd8: return 0x172;
      case 0xd9: return 0x141;
      case 0xda: return 0x15a;
      case 0xdb: return 0x16a;
      case 0xdd: return 0x17b;
      case 0xde: return 0x17d;

      case 0xe0: return 0x105;
      case 0xe1: return 0x12f;
      case 0xe2: return 0x101;
      case 0xe3: return 0x107;
      case 0xe6: return 0x119;
      case 0xe7: return 0x113;
      case 0xe8: return 0x10d;
      case 0xea: return 0x17a;
      case 0xeb: return 0x117;
      case 0xec: return 0x121;
      case 0xed: return 0x137;
      case 0xee: return 0x12b;
      case 0xef: return 0x12e;

      case 0xf0: return 0x161;
      case 0xf1: return 0x144;
      case 0xf2: return 0x146;
      case 0xf4: return 0x14d;
      case 0xf8: return 0x173;
      case 0xf9: return 0x142;
      case 0xfa: return 0x15b;
      case 0xfb: return 0x16b;
      case 0xfd: return 0x17c;
      case 0xfe: return 0x17e;
      case 0xff: return '\'';
         break;
      }
      return encoding::ConvertCodedCharToUnicode(c, COD_WESTERN);

   case COD_CENTRAL_EUROPE_WINDOWS:
      switch(c){
      case 0x8a: return 0x160;
      case 0xa3: return 0x141;
      case 0xa5: return 0x104;
      case 0xaa: return 0x15e;
      case 0xaf: return 0x17b;

      case 0xb3: return 0x142;
      case 0xb9: return 0x105;
      case 0xba: return 0x105;
      case 0xbc: return 0x13d;
      case 0xbe: return 0x13e;
      case 0xbf: return 0x17c;
      }
      if(c<0xc0)
         break;
                              //chars 0xc0 - 0xff are identical with 8859-2
                              //flow...
   case COD_CENTRAL_EUROPE:
      switch(c){
      case 0xa1: return 0x104;
      case 0xa3: return 0x141;
      case 0xa5: return 0x13d;
      case 0xa6: return 0x15a;
      case 0xa8: return '\"';
      case 0xa9: return 0x160;
      case 0xaa: return 0x15e;
      case 0xab: return 0x164;
      case 0xac: return 0x179;
      case 0xad: return '-';
      case 0xae: return 0x17d;
      case 0xaf: return 0x17b;

      case 0xb1: return 0x105;
      case 0xb3: return 0x142;
      case 0xb5: return 0x13e;
      case 0xb6: return 0x15b;
      case 0xb9: return 0x161;
      case 0xba: return 0x15f;
      case 0xbb: return 0x165;
      case 0xbc: return 0x17a;
      case 0xbd: return '\"';
      case 0xbe: return 0x17e;
      case 0xbf: return 0x17c;

      case 0xc0: return 0x154;
      case 0xc3: return 0x102;
      case 0xc5: return 0x139;
      case 0xc6: return 0x106;
      case 0xc8: return 0x10c;
      case 0xca: return 0x118;
      case 0xcc: return 0x11a;
      case 0xcf: return 0x10e;

      case 0xd1: return 0x143;
      case 0xd2: return 0x147;
      case 0xd5: return 0x150;
      case 0xd8: return 0x158;
      case 0xd9: return 0x16e;
      case 0xdb: return 0x170;
      case 0xde: return 0x162;

      case 0xe0: return 0x155;
      case 0xe3: return 0x103;
      case 0xe5: return 0x13a;
      case 0xe6: return 0x107;
      case 0xe8: return 0x10d;
      case 0xea: return 0x119;
      case 0xec: return 0x11b;
      case 0xef: return 0x10f;

      case 0xf1: return 0x144;
      case 0xf2: return 0x148;
      case 0xf8: return 0x159;
      case 0xf9: return 0x16f;
      case 0xfb: return 0x171;
      case 0xfe: return 0x163;
      case 0xff: return '\'';
      }
      break;

   case COD_TURKISH:
      switch(c){
      case 0xd0: return 0x11e;
      case 0xdd: return 0x130;
      case 0xde: return 0x15e;
      case 0xf0: return 0x11f;
      case 0xfd: return 0x131;
      case 0xfe: return 0x15f;
      }
      break;

   case COD_GREEK:
      if(c==0xa2)
         return 'A';
      if(c>=0xb8 && c<=0xff)
         return c + 0x2d0;
      break;

   case COD_BALTIC_ISO:
      if(c>=0xa0 && c<=0xff){
         static const wchar tab[] = {
            0x00a0,0x0104,0x0138,0x0156,0x00a4,0x0128,0x013b,0x00a7,0x00a8,0x0160,0x0112,0x0122,0x0166,0x00ad,0x017d,0x00af,
            0x00b0,0x0105,0x02db,0x0157,0x00b4,0x0129,0x013c,0x02c7,0x00b8,0x0161,0x0113,0x0123,0x0167,0x014a,0x017e,0x014b,
            0x0100,0x00c1,0x00c2,0x00c3,0x00c4,0x00c5,0x00c6,0x012e,0x010c,0x00c9,0x0118,0x00cb,0x0116,0x00cd,0x00ce,0x012a,
            0x0110,0x0145,0x014c,0x0136,0x00d4,0x00d5,0x00d6,0x00d7,0x00d8,0x0172,0x00da,0x00db,0x00dc,0x0168,0x016a,0x00df,
            0x0101,0x00e1,0x00e2,0x00e3,0x00e4,0x00e5,0x00e6,0x012f,0x010d,0x00e9,0x0119,0x00eb,0x0117,0x00ed,0x00ee,0x012b,
            0x0111,0x0146,0x014d,0x0137,0x00f4,0x00f5,0x00f6,0x00f7,0x00f8,0x0173,0x00fa,0x00fb,0x00fc,0x0169,0x016b,0x02d9,
         };
         return tab[c-0xa0];
      }
      break;

   case COD_BALTIC_WIN_1257:
      if(c>=0x80 && c<=0xff){
         static const wchar tab[] = {
            0x20ac,0x0020,0x201a,0x0020,0x201e,0x2026,0x2020,0x2021,0x0020,0x2030,0x0020,0x2039,0x0020,0x00a8,0x02c7,0x00b8,
            0x0020,0x2018,0x2019,0x201c,0x201d,0x2022,0x2013,0x2014,0x0020,0x2122,0x0020,0x203a,0x0020,0x00af,0x02db,0x0020,
            0x00a0,0x0020,0x00a2,0x00a3,0x00a4,0x0020,0x00a6,0x00a7,0x00d8,0x00a9,0x0156,0x00ab,0x00ac,0x00ad,0x00ae,0x00c6,
            0x00b0,0x00b1,0x00b2,0x00b3,0x00b4,0x00b5,0x00b6,0x00b7,0x00f8,0x00b9,0x0157,0x00bb,0x00bc,0x00bd,0x00be,0x00e6,
            0x0104,0x012e,0x0100,0x0106,0x00c4,0x00c5,0x0118,0x0112,0x010c,0x00c9,0x0179,0x0116,0x0122,0x0136,0x012a,0x013b,
            0x0160,0x0143,0x0145,0x00d3,0x014c,0x00d5,0x00d6,0x00d7,0x0172,0x0141,0x015a,0x016a,0x00dc,0x017b,0x017d,0x00df,
            0x0105,0x012f,0x0101,0x0107,0x00e4,0x00e5,0x0119,0x0113,0x010d,0x00e9,0x017a,0x0117,0x0123,0x0137,0x012b,0x013c,
            0x0161,0x0144,0x0146,0x00f3,0x014d,0x00f5,0x00f6,0x00f7,0x0173,0x0142,0x015b,0x016b,0x00fc,0x017c,0x017e,0x02d9,
         };
         return tab[c-0x80];
      }
      break;

   case COD_LATIN9:
      switch(c){
      case 0xa4: return 0x20ac;  //euro
      case 0xa6: return 0x160;   //Sv
      case 0xa8: return 0x161;   //sv
      case 0xb4: return 0x17d;   //Zv
      case 0xb8: return 0x17e;   //zv
      case 0xbc: return 0x152;   //OE
      case 0xbd: return 0x153;   //oe
      case 0xbe: return 0x178;   //Y..
      }
      return encoding::ConvertCodedCharToUnicode(c, COD_WESTERN);

      /*
   case COD_BIG5:
      if(c>=0x2f00)
         return ConvertCharToUnicodeBySystem(c, coding);
      break;
      */
   }
   switch(c){
   case 0x80: return 'E';  //Euro
   case 0x82: return ',';
   case 0x84: return '\"';
   case 0x88: return '^';
   case 0x89: return 0xeb;
   case 0x8a: return 0xe8;
   case 0x8b: return '<';
   case 0x8c: return 0x15a;
   case 0x8d: return 0x164;
   case 0x8e: return 0x17d;
   case 0x8f: return 0x179;

   case 0x91: return '\'';
   case 0x92: return '\'';
   case 0x93: return '\"';
   case 0x94: return '\"';
   case 0x96: return 0xfb;
   case 0x97: return '-';
   case 0x98: return '~';
   case 0x9a: return 0x161;
   case 0x9b: return '>';
   case 0x9c: return 0x15b;
   case 0x9d: return 0x165;
   case 0x9e: return 0x17e;
   case 0x9f: return 0x17a;
   }
   /*
                              //forced unicode mapping to western
   if(c>=0x7f00 && c<=0x7fff)
      return c-0x7f00;
   */

   return c;
}

//----------------------------

void encoding::CharsetToCoding(const Cstr_c &charset, E_TEXT_CODING &cod){

   const char *cp = charset;
   if(text_utils::CheckStringBegin(cp, "iso-8859-")){
      int n;
      if(text_utils::ScanInt(cp, n)){
         switch(n){
         case 1: cod = COD_WESTERN; break;
         case 2: cod = COD_CENTRAL_EUROPE; break;
         case 4: cod = COD_BALTIC_ISO; break;
         case 5: cod = COD_CYRILLIC_ISO; break;
         case 7: cod = COD_GREEK; break;
         case 8: cod = COD_HEBREW; break;
         case 9: cod = COD_TURKISH; break;
         case 13: cod = COD_8859_13; break;
         case 15: cod = COD_LATIN9; break;
         //default:
            //assert(0);
         }
      }else
         assert(0);
   }else if(text_utils::CheckStringBegin(cp, "windows-")){
      int n;
      if(text_utils::ScanInt(cp, n)){
         switch(n){
         case 1250: cod = COD_CENTRAL_EUROPE_WINDOWS; break;
         case 1251: cod = COD_CYRILLIC_WIN1251; break;
         case 1252: cod = COD_WESTERN; break;
         case 1253: cod = COD_GREEK; break;
         case 1254: cod = COD_TURKISH; break;
         case 1255: cod = COD_HEBREW; break;
         case 1257: cod = COD_BALTIC_WIN_1257; break;
         //default:
            //assert(0);
         }
      }else
         assert(0);
   }else if(charset=="us-ascii") cod = COD_WESTERN;
   else if(charset=="utf-8") cod = COD_UTF_8;
   else if(charset=="koi8-r") cod = COD_KOI8_R;
   else if(charset=="koi8-u") cod = COD_KOI8_U;
   else if(charset=="big5") cod = COD_BIG5;
   else if(charset=="gb2312") cod = COD_GB2312;
   else if(charset=="gbk") cod = COD_GBK;
   else if(charset=="shift_jis") cod = COD_SHIFT_JIS;
   else if(charset=="iso-2022-jp") cod = COD_JIS;
   else if(charset=="euc-kr") cod = COD_EUC_KR;
   else if(charset=="ks_c_5601-1987") cod = COD_EUC_KR;
   else if(charset=="cp1252") cod = COD_WESTERN;
#ifdef _DEBUG
   else if(charset=="ascii") cod = COD_WESTERN;
   else if(charset==""){}
   else if(charset=="random"){}
   else{
      //assert(0);
   }
#endif
}

//----------------------------

void S_text_style::WriteCode(E_TEXT_CONTROL_CODE code, const void *old, const void *nw, dword sz, C_vector<char> &buf){

   buf.push_back(char(code));
   buf.insert(buf.end(), (char*)old, (char*)old+sz);
   buf.insert(buf.end(), (char*)nw, (char*)nw+sz);
   buf.push_back(char(code));
}

//----------------------------

void S_text_style::SetDefault(){

   font_flags = 0;
   font_index = -1;
   text_color = COLOR_BLACK;
   href = false;
   bgnd_color = 0;
   block_quote_count = 0;
}

//----------------------------

void S_text_style::WriteAndUpdate(const S_text_style &ts, C_vector<char> &buf){

   if(font_flags != ts.font_flags){
      WriteCode(CC_TEXT_FONT_FLAGS, &font_flags, &ts.font_flags, 1, buf);
      font_flags = ts.font_flags;
   }
   if(font_index != ts.font_index){
      WriteCode(CC_TEXT_FONT_SIZE, &font_index, &ts.font_index, 1, buf);
      font_index = ts.font_index;
   }
   if(block_quote_count != ts.block_quote_count){
      WriteCode(CC_BLOCK_QUOTE_COUNT, &block_quote_count, &ts.block_quote_count, 1, buf);
      block_quote_count = ts.block_quote_count;
   }
   if(text_color != ts.text_color){
      WriteCode(CC_TEXT_COLOR, &text_color, &ts.text_color, 3, buf);
      text_color = ts.text_color;
   }
   if(bgnd_color != ts.bgnd_color){
      WriteCode(CC_BGND_COLOR, &bgnd_color, &ts.bgnd_color, 4, buf);
      bgnd_color = ts.bgnd_color;
   }
}

//----------------------------

void S_text_style::AddWideCharToBuf(C_vector<char> &buf, wchar c){
   if(c>=256 || (c>=CC_TEXT_FONT_FLAGS && c<CC_LAST)){
      buf.push_back(CC_WIDE_CHAR);
      buf.push_back(char(c&0xff));
      buf.push_back(char(c>>8));
      buf.push_back(CC_WIDE_CHAR);
   }else
      buf.push_back(char(c));
}

//----------------------------

void S_text_style::AddWideStringToBuf(C_vector<char> &buf, const wchar *str, int ff, int col){
   if(ff!=-1 && font_flags!=ff)
      WriteCode(CC_TEXT_FONT_FLAGS, &font_flags, &ff, 1, buf);
   if(col && col!=int(text_color))
      WriteCode(CC_TEXT_COLOR, &text_color, &col, 3, buf);
   while(*str)
      AddWideCharToBuf(buf, *str++);
   if(col && col!=int(text_color))
      WriteCode(CC_TEXT_COLOR, &col, &text_color, 3, buf);
   if(ff!=-1 && font_flags!=ff)
      WriteCode(CC_TEXT_FONT_FLAGS, &ff, &font_flags, 1, buf);
}

//----------------------------

void S_text_style::AddHorizontalLine(C_vector<char> &buf, dword color, byte width_percent){
   buf.push_back(CC_HORIZONTAL_LINE);
   buf.insert(buf.end(), (char*)&color, ((char*)&color)+4);
   buf.push_back(byte(width_percent*128/100));
   buf.push_back(CC_HORIZONTAL_LINE);
}

//----------------------------

int S_text_style::ReadCode(const char *&cp, const int *active_link){

   const char *beg = cp;
   char c = *cp++;

   int old, nw;
   switch(c){
   case CC_TEXT_FONT_FLAGS:
      old = *cp++;
      nw = *cp++;
      //assert(font_flags==old);
      font_flags &= FF_ACTIVE_HYPERLINK;
      font_flags |= nw;
      break;
   case CC_TEXT_FONT_SIZE:
      old = schar(*cp++);
      nw = schar(*cp++);
      //assert(font_index==old);
      font_index = nw;
      break;
   case CC_BLOCK_QUOTE_COUNT:
      old = byte(*cp++);
      nw = byte(*cp++);
      block_quote_count = (byte)nw;
      break;
   case CC_TEXT_COLOR:
      old = COLOR_BLACK; nw = COLOR_BLACK;
      MemCpy(&old, cp, 3);
      MemCpy(&nw, cp+3, 3);
      cp += 6;
      //assert(color==old);
      text_color = nw;
      break;
   case CC_BGND_COLOR:
      //old, nw;
      MemCpy(&old, cp, 4);
      MemCpy(&nw, cp+4, 4);
      cp += 8;
      //assert(color==old);
      bgnd_color = nw;
      break;
   case CC_HYPERLINK:
      {
         int indx = byte(*cp++);
         indx |= byte(*cp++) << 8;
         if(c==CC_HYPERLINK){
            if(active_link && *active_link==indx)
               font_flags |= FF_ACTIVE_HYPERLINK;
         }
         href = true;
      }
      break;
   case CC_FORM_INPUT:
      cp += 2;
      break;
   case CC_HYPERLINK_END:
      cp += 2;
      font_flags &= ~FF_ACTIVE_HYPERLINK;
      href = false;
      break;
   default:
      assert(0);
   }
   char c1;
   c1 = *cp++;
   assert(c==c1);
   return cp - beg;
}

//----------------------------

int S_text_style::ReadCodeBack(const char *&cp, const int *active_link){

   const char *beg = cp;
   char c = *--cp;

   int old, nw;
   switch(c){
   case CC_TEXT_FONT_FLAGS:
      old = *--cp;
      nw = *--cp;
      //assert(font_flags==old);
      font_flags &= FF_ACTIVE_HYPERLINK;
      font_flags |= nw;
      break;
   case CC_TEXT_FONT_SIZE:
      old = schar(*--cp);
      nw = schar(*--cp);
      //assert(font_index==old);
      font_index = nw;
      break;
   case CC_BLOCK_QUOTE_COUNT:
      old = byte(*--cp);
      nw = byte(*--cp);
      block_quote_count = (byte)nw;
      break;
   case CC_TEXT_COLOR:
      old = COLOR_BLACK; nw = COLOR_BLACK;
      cp -= 6;
      MemCpy(&old, cp+3, 3);
      MemCpy(&nw, cp, 3);
      //assert(color==old);
      text_color = nw;
      break;
   case CC_BGND_COLOR:
      cp -= 8;
      MemCpy(&old, cp+4, 4);
      MemCpy(&nw, cp, 4);
      //assert(color==old);
      bgnd_color = nw;
      break;
   case CC_WIDE_CHAR:
   case CC_IMAGE:
   case CC_FORM_INPUT:
                           //just skip
      cp -= 2;
      break;

   case CC_HORIZONTAL_LINE:
      cp -= 5;
      break;

   case CC_CUSTOM_ITEM:
      cp -= 2;
      break;

   case CC_HYPERLINK:
      cp -= 2;
      font_flags &= ~FF_ACTIVE_HYPERLINK;
      href = false;
      break;
   case CC_HYPERLINK_END:
      {
         int indx = byte(*--cp) << 8;
         indx |= byte(*--cp);
         if(active_link && *active_link==indx)
            font_flags |= FF_ACTIVE_HYPERLINK;
         href = true;
      }
      break;
   default:
      assert(0);
   }
   char c1;
   c1 = *--cp;
   assert(c==c1);
   return beg - cp;
}

//----------------------------

void S_text_style::SkipCode(const char *&cp){

   char c = *cp++;
   switch(c){
   case CC_TEXT_FONT_FLAGS:
   case CC_TEXT_FONT_SIZE:
   case CC_IMAGE:
   case CC_WIDE_CHAR:
   case CC_HYPERLINK:
   case CC_HYPERLINK_END:
   case CC_FORM_INPUT:
   case CC_CUSTOM_ITEM:
   case CC_BLOCK_QUOTE_COUNT:
      cp += 2;
      break;
   case CC_HORIZONTAL_LINE:
      cp += 5;
      break;
   case CC_TEXT_COLOR: cp += 6; break;
   case CC_BGND_COLOR: cp += 8; break;
   default:
      assert(0);
   }
   char c1;
   c1 = *cp++;
   assert(c==c1);
}

//----------------------------

word S_text_style::ReadWideChar(const char *&cp){

   assert(cp[-1]==CC_WIDE_CHAR);
   assert(cp[2]==CC_WIDE_CHAR);
   cp += 3;
   return byte(cp[-3]) | (cp[-2]<<8);
}

//----------------------------

dword S_text_style::Read16bitIndex(const char *&cp){
   assert(cp[-1]==cp[2]);
   cp += 3;
   return byte(cp[-3]) | (byte(cp[-2])<<8);
}

//----------------------------

dword S_text_style::ReadHorizontalLine(const char *&cp, dword rc_width, dword &color){

   assert(cp[-1]==CC_HORIZONTAL_LINE);
   assert(cp[5]==CC_HORIZONTAL_LINE);
   MemCpy(&color, cp, 4);
   cp += 6;
   return (byte(cp[-2])*rc_width)>>7;
}

//----------------------------

int S_key_accelerator::GetValue(bool pressed, int time, int min_speed, int max_speed, int accel){

   if(pressed){
      int v1 = (min_speed+speed)*time+last_frac;
      int v = v1/1000;
      last_frac = v1 - v*1000;

      int a1 = accel*time + acc_frac;
      int a = a1/1000;
      acc_frac = a1 - a;
      speed += a;

      speed = Min(speed, max_speed);
      return v;
   }
   speed = 0;
   last_frac = 0;
   acc_frac = 0;
   return 0;
}

//----------------------------

bool C_scrollbar::MakeThumbRect(const C_application_base &app, S_rect &rc_th) const{

   rc_th = rc;
   bool vertical = (rc.sy > rc.sx);

                              //get total area size
   int sb_sz = !vertical ? rc_th.sx : rc_th.sy;
                              //compute thumb size
   int th_sz = 0;
   if(total_space)
      th_sz = longlong(sb_sz) * longlong(visible_space) / longlong(total_space);
                              //don't make thumb size smaller than some minimal size
   //th_sz = Max(th_sz, (!vertical ? rc_th.sy : rc_th.sx)*2);
   th_sz = Max(th_sz, app.fdb.line_spacing*2);
                              //don't make it overflow
   th_sz = Min(sb_sz, th_sz);
                              //compute thumb positon
   int slide_sz = total_space - visible_space;
   if(slide_sz){
      int scrl_space = sb_sz - th_sz;
      (!vertical ? rc_th.x : rc_th.y) += int(longlong(scrl_space) * Min(Max(longlong(pos), longlong(0)), longlong(slide_sz)) / longlong(slide_sz));
   }
                              //store thumb size
   (!vertical ? rc_th.sx : rc_th.sy) = th_sz;
   return vertical;
}

//----------------------------

bool C_button::ProcessMouse(const S_user_input &ui, bool &redraw, C_application_base *app_for_touch_feedback){

#ifdef USE_MOUSE
   if(ui.mouse_buttons&MOUSE_BUTTON_1_DOWN){
      if(ui.CheckMouseInRect(rc_click)){
         clicked = down = true;
         redraw = true;
         if(app_for_touch_feedback)
            app_for_touch_feedback->MakeTouchFeedback(C_application_base::TOUCH_FEEDBACK_BUTTON_PRESS);
      }
   }
   if((ui.mouse_buttons&MOUSE_BUTTON_1_DRAG) && clicked){
      if(down!=ui.CheckMouseInRect(rc_click)){
         down = !down;
         redraw = true;
      }
   }
   if(ui.mouse_buttons&MOUSE_BUTTON_1_UP){
      bool ret = down;
      if(down)
         redraw = true;
      clicked = down = false;
      return ret;
   }
#endif
   return false;
}

//----------------------------

void S_text_display_info::Clear(){

   justify = false;
   byte_offs = 0;
   pixel_offs = 0;
   num_lines = 0;
   top_pixel = 0;
   total_height = 0;
   bgnd_color = 0xffffffff;
   link_color = 0xff0000ff;
   alink_color = 0xff0000ff;
   bgnd_image = -1;
   body_w.Clear();
   body_c.Clear();
   images.clear();
   hyperlinks.clear();
   active_link_index = -1;
   active_object_offs = -1;
   ts = S_text_style();
   is_wide = false;
   html_title.Clear();
   bookmarks.clear();
   last_rendered_images.clear();
   mark_start = mark_end = 0;
#ifdef WEB_FORMS
   refresh_delay = 0;
   refresh_link.Clear();
   forms.clear();
   form_inputs.clear();
   form_select_options.clear();
   te_input = NULL;
#ifdef WEB
   js.Clear();
#endif//WEB
#endif//WEB_FORMS
}

//----------------------------

void S_text_display_info::BeginHyperlink(C_vector<char> &dst_buf, const S_hyperlink &hr){

   dst_buf.push_back(CC_HYPERLINK);
   int indx = hyperlinks.size();
   dst_buf.push_back(char(indx&0xff));
   dst_buf.push_back(char(indx>>8));
   dst_buf.push_back(CC_HYPERLINK);
   hyperlinks.push_back(hr);
}

//----------------------------

void S_text_display_info::EndHyperlink(C_vector<char> &dst_buf){

   dst_buf.push_back(CC_HYPERLINK_END);
   int indx = hyperlinks.size()-1;
   dst_buf.push_back(char(indx&0xff));
   dst_buf.push_back(char(indx>>8));
   dst_buf.push_back(CC_HYPERLINK_END);
}

//----------------------------

const S_hyperlink *S_text_display_info::GetActiveHyperlink() const{

   if(active_link_index==-1)
      return NULL;

   const char *cp = body_c + active_object_offs;
   assert(*cp == CC_HYPERLINK);
   int indx = byte(cp[1]) | (byte(cp[2]) << 8);
   assert(indx < hyperlinks.size());
   return &hyperlinks[indx];
}

//----------------------------

class C_menu_imp: public C_menu{
public:
   C_smart_ptr<C_image> image; //originally copied from C_application_ui, may be changed

   static const int VERTICAL_BORDER = 3,
      SEPARATOR_HEIGHT = 4,
      MAX_ITEMS = 32;

   struct S_item{
      Cstr_w text;            //if not NULL, it is used instead of txt_id
      word txt_id;
      signed char img_index;         //(-1 = no)
      Cstr_c shortcut, shortcut_full_kb;
      dword flags;

      inline bool IsSeparator() const{ return (!text.Length() && txt_id==0); }
   };
   S_item items[MAX_ITEMS];
   int num_items;
   bool want_sel_change_notify;
   int menu_drag_begin_y;
   int menu_drag_begin_pos; //-1 = not dragging

   struct S_animation{
      C_application_base::C_timer *anim_timer;
      enum{
         PHASE_INIT_APPEAR_TIMER,
         PHASE_GROW_CIRCLE,
         PHASE_VISIBLE,
      } phase;
      int counter;
      int count_to;
      S_point init_pos;
      mutable C_smart_ptr<C_image> img_circle_bgnd;  //saved background below circle, for fast circle drawing
      S_point img_circle_pos;
      mutable S_rect rc_last_draw;

      S_animation():
         anim_timer(NULL),
         phase(PHASE_VISIBLE),
         counter(0)
      {}
      ~S_animation(){
         DestroyTimer();
      }
      void DestroyTimer(){
         delete anim_timer;
         anim_timer = NULL;
      }
   } animation;
   bool touch_mode;           //in touch mode only 5 items are shown in a cricle, and entire menu is touch-friendly
   struct S_touch_item{
      C_smart_ptr<C_image> img_text[2];
      int hover_offs;
   } touch_items[5];

                           //parent for sub-menu
   C_smart_ptr<C_menu_imp> parent_menu;

                              //graphics
   S_rect rc;
   bool mouse_down;

                              //classic menu:
   C_scrollbar sb;
   int line_height;           //pixels for each line

                              //touch menu:
   int hovered_item;
   dword menu_id;

   C_menu_imp(C_menu_imp *p, dword mid, bool touch):
      num_items(0),
      want_sel_change_notify(false),
      //selected_item(0),
      touch_mode(touch),
      parent_menu(p),
      line_height(0),
      hovered_item(-1),
      menu_id(mid),
      mouse_down(false),
      menu_drag_begin_pos(-1)
   {
      selected_item = 0;
   }

//----------------------------
// Check if touch menu us currently being animated.
   virtual bool IsAnimating() const{ return (animation.phase!=animation.PHASE_VISIBLE); }

   virtual void AddItem(dword txt_id, dword flags = 0, const char *shortcut = NULL, const char *shortcut_full_kb = NULL, char img_index = -1);
   virtual void AddItem(const Cstr_w &txt, dword flags = 0, const char *right_txt = NULL);

   virtual void RequestSelectionChangeNotify(){
      want_sel_change_notify = true;
   }
   virtual void SetImage(C_image *img){
      image = img;
   }

//----------------------------
// Get selected item's X/Y, relative to top of drawn area.
   int GetSelectedItemX() const;
   int GetSelectedItemY() const;

   void MakeSureItemIsVisible();

   virtual dword GetMenuId() const{ return menu_id; }

   virtual const C_menu *GetParentMenu() const{ return parent_menu; }

   virtual const S_rect &GetRect() const{ return rc; }
};

//----------------------------

void C_menu_imp::AddItem(dword txt_id, dword flags, const char *shortcut, const char *shortcut_full_kb, char img_index){

   assert(num_items<(touch_mode ? 5 : MAX_ITEMS));
   S_item &itm = items[num_items++];
   itm.txt_id = word(txt_id);
   itm.flags = flags;
                              //separator always disabled
   if(txt_id==0)
      itm.flags |= C_menu::DISABLED;
   itm.img_index = img_index;

   itm.shortcut = shortcut;
   itm.shortcut_full_kb = shortcut_full_kb ? Cstr_c(shortcut_full_kb) : itm.shortcut;
}

//----------------------------

void C_menu_imp::AddItem(const Cstr_w &txt, dword flags, const char *shortcut){

   assert(num_items<MAX_ITEMS);
   S_item &itm = items[num_items++];
   itm.txt_id = 0;
   itm.text = txt;
   itm.flags = flags;
   itm.img_index = -1;
   itm.shortcut = shortcut;
   itm.shortcut_full_kb = itm.shortcut;
}

//----------------------------

static const signed char touch_menu_item_pos[][2] = {
   { -1, -1 },
   { 1, -1 },
   { -1, 1 },
   { 1, 1 },
   { 0, 0 },
};

int C_menu_imp::GetSelectedItemX() const{
   int x;
#ifdef USE_MOUSE
   if(touch_mode){
      x = rc.sx/2;
      int p45 = rc.sx/2 * 707 / 1000;
      x += touch_menu_item_pos[selected_item][0]*p45;
   }else
#endif
      assert(0);
   return x;
}

//----------------------------

int C_menu_imp::GetSelectedItemY() const{

   int y;
#ifdef USE_MOUSE
   if(touch_mode){
      y = rc.sx/2;
      int p45 = rc.sx/2 * 707 / 1000;
      y += touch_menu_item_pos[selected_item][1]*p45;
   }else
#endif
   {
      y = -sb.pos;
      for(int i=0; i<selected_item; i++){
         const S_item &itm = items[i];
         if(itm.IsSeparator())
            y += C_menu_imp::SEPARATOR_HEIGHT;
         else
            y += line_height;
      }
   }
   return y;
}

//----------------------------

void C_menu_imp::MakeSureItemIsVisible(){

   assert(!touch_mode);
   int y = GetSelectedItemY();
   int rc_sy = rc.sy-VERTICAL_BORDER*2;
   if(y<0){
      sb.pos += y;
   }else
   if(y+line_height > rc_sy){
      sb.pos += y+line_height - rc_sy;
   }
}

//----------------------------

C_application_ui::C_application_ui():
   shadow_inited(false),
   data_path(NULL),
   temp_path(NULL),
   texts_buf(NULL),
   ui_touch_mode(HasMouse()),
   num_text_lines(0)
{
}

//----------------------------

C_application_ui::~C_application_ui(){
   delete[] texts_buf;
}

//----------------------------

void C_application_ui::InitInputCaps(){

   C_application_base::InitInputCaps();
   shift_key_name = "Shift";
   delete_key_name = "[Del]";
#ifdef _WIN32_WCE
   if(IsWMSmartphone()){
      if(HasFullKeyboard())
         delete_key_name = "[D]";
      else
         shift_key_name = "#";
   }
#else
   if(!HasFullKeyboard())
      delete_key_name = "[c]";
#endif
}

//----------------------------

bool C_application_ui::Construct(const wchar *_data_path){
   data_path = _data_path;
   return true;
}

//----------------------------

void C_application_ui::InitMenuAfterRescale(C_menu *_menu){

   C_menu_imp *menu = (C_menu_imp*)_menu;
   if(menu->parent_menu)
      InitMenuAfterRescale(menu->parent_menu);
   PrepareMenu(menu, menu->selected_item);
}

//----------------------------

void C_application_ui::ScreenReinit(bool resized){

   if(resized)
      CloseShadow();
   C_application_base::ScreenReinit(resized);
   RedrawScreen();
   UpdateScreen();
}

//----------------------------

void C_application_ui::InitAfterScreenResize(){

   //LOG_RUN("C_application_ui::InitAfterScreenResize");
   ui_touch_mode = HasMouse();
   text_edit_state_icons = NULL;
   C_application_base::InitAfterScreenResize();
   InitSoftButtonsRectangles();
#ifdef USE_MOUSE
   InitSoftkeyBarButtons();
#endif
   C_mode *mod = mode;
   while(mod){
#ifdef USE_MOUSE
      for(const C_menu_imp *mp=(const C_menu_imp*)(const C_menu*)mod->GetMenu(); mp; mp=mp->parent_menu){
         if(mp->touch_mode){
            mod->SetMenu(NULL);
            break;
         }
      }
#endif
      mod->InitLayout();
      C_menu *menu = mod->GetMenu();
      if(menu)
         InitMenuAfterRescale(menu);
      mod = mod->GetParent();
   }
}

//----------------------------

C_smart_ptr<C_menu> C_application_ui::CreateMenu(C_mode &mod, dword menu_id, bool inherit_touch){

   if(!mod.menu && mod.GetFocus()){
                        //un-focus currently focused control if menu is being displayed
      mod.GetFocus()->SetFocus(false);
   }
   C_menu_imp *prnt = (C_menu_imp*)(C_menu*)mod.menu;
   C_smart_ptr<C_menu> m = new(true) C_menu_imp(prnt, menu_id, (inherit_touch && prnt && prnt->touch_mode));
   m->SetImage(buttons_image);
   m->Release();
   return m;
}

//----------------------------

C_smart_ptr<C_menu> C_application_ui::CreateTouchMenu(C_menu *_prnt, dword menu_id){

#ifdef USE_MOUSE
   C_menu_imp *prnt = (C_menu_imp*)_prnt;
   C_smart_ptr<C_menu> m = new(true) C_menu_imp(prnt, menu_id, true);
   m->SetImage(buttons_image);
   m->Release();
   return m;
#else
   return NULL;
#endif
}

//----------------------------

C_smart_ptr<C_menu> C_application_ui::CreateEditCCPSubmenu(const C_text_editor *te, C_menu *_prnt){

   C_menu_imp *prnt = (C_menu_imp*)_prnt;
   C_smart_ptr<C_menu> menu = new(true) C_menu_imp(prnt, 0, false);
   int num_sel = Abs(te->GetCursorSel()-te->GetCursorPos());
   bool can_copy_paste = (num_sel && !te->IsSecret());
   menu->AddItem(SPECIAL_TEXT_CUT, can_copy_paste ? 0 : C_menu::DISABLED);
   menu->AddItem(SPECIAL_TEXT_COPY, can_copy_paste ? 0 : C_menu::DISABLED);
   menu->AddItem(SPECIAL_TEXT_PASTE, te->CanPaste() ? 0 : C_menu::DISABLED);
   menu->Release();
   return menu;
}

//----------------------------

void C_application_ui::InitShadow(){

   if(shadow_inited)
      return;
   const C_smart_ptr<C_zip_package> dta = CreateDtaFile();
   if(!dta)
      return;
   C_fixed ratio = C_fixed::One();
   int msz = Min(ScrnSX(), ScrnSY());
   if(msz<320)
      ratio = C_fixed(msz)*C_fixed::Create(205);
   static const char *const filenames[] = {
      "lt",
      "lt1",
      "t",
      "rt",
      "rt1",
      "l",
      "r",
      "lb",
      "lb1",
      "b",
      "rb",
      "rb1",
   };
   for(int i=SHD_LAST; i--; ){
      C_image *img = C_image::Create(*this);
      shadow[i] = img;
      img->Release();
      Cstr_w fn;
      fn.Format(L"Shadow\\%.png") <<filenames[i];
      if(C_image::IMG_OK!=img->Open(fn, 0, 0, ratio, dta))
         break;
   }
   shadow_inited = true;
}

//----------------------------

void C_application_ui::CloseShadow(){
   for(int i=SHD_LAST; i--; )
      shadow[i] = NULL;
   shadow_inited = false;
}

//----------------------------

void C_application_ui::FillPixels(const S_rect &rc, dword pix){

   S_rect rx;
   rx.SetIntersection(rc, GetClipRect());
   if(rx.sx<=0 || rx.sy<=0)
      return;
   const dword src_pitch = GetScreenPitch();
   if(GetPixelFormat().bytes_per_pixel==2){
      word *dst = (word*)GetBackBuffer();
      dst += rx.y*src_pitch/2 + rx.x;
      for(int y=rx.sy; y--; ){
         word *dp = dst;
         for(int x=rx.sx; x--; )
            *dp++ = word(pix);
         dst += src_pitch/2;
      }
   }else{
      dword *dst = (dword*)GetBackBuffer();
      dst += rx.y*src_pitch/4 + rx.x;
      for(int y=rx.sy; y--; ){
         dword *dp = dst;
         for(int x=rx.sx; x--; )
            *dp++ = pix;
         dst += src_pitch/4;
      }
   }
}

//----------------------------

int C_application_ui::GetTextWidth(const void *str, bool wide, E_TEXT_CODING coding, dword font_index, dword font_flags, int max_chars, const S_text_display_info *tdi) const{

   S_text_style ts;//(font_index, font_flags, ;
   ts.font_index = font_index;
   ts.font_flags = font_flags;

   int width = 0;
   bool rem_last_pixel = false;
   while(true){
      dword c = text_utils::GetChar(str, wide);
      if(!c)
         break;
      if(c=='\n')
         break;
      if(S_text_style::IsStyleCode(c)){
         assert(!wide);
         if(wide) break;
         const char *cp1 = (const char*)str;
         --cp1;
         assert(tdi);
         int i = ts.ReadCode(cp1, &tdi->active_link_index);
         if(max_chars && (max_chars -= i) <= 0)
            break;
         str = cp1;
         continue;
      }
      switch(c){
      case CC_IMAGE:
         {
            if(wide) break;
            rem_last_pixel = false;
            dword ii = ts.Read16bitIndex((const char*&)str);
            assert(int(ii) < tdi->images.size());
            const S_image_record &ir = tdi->images[ii];
            int sx = ir.sx, sy = ir.sy;
            if(ir.draw_border)
               sx += 2, sy += 2;
            width += sx;
            if(max_chars && (max_chars -= 4) <= 0)
               goto end;
         }
         continue;
#ifdef WEB_FORMS
      case CC_FORM_INPUT:
         {
            if(wide) break;
            rem_last_pixel = false;
            dword ii = ts.Read16bitIndex((const char*&)str);
            const S_html_form_input &fi = tdi->form_inputs[ii];
            width += fi.sx;
            if(max_chars && (max_chars -= 4) <= 0)
               goto end;
         }
         continue;
#endif//WEB_FORMS
      case CC_WIDE_CHAR:
         c = S_text_style::ReadWideChar((const char*&)str);
         if(max_chars)
            max_chars -= 3;
         break;
      case CC_HORIZONTAL_LINE:
         if(!width)
            width = tdi->rc.sx;
         goto end;
      case CC_CUSTOM_ITEM:
         {
            rem_last_pixel = false;
            dword ii = ts.Read16bitIndex((const char*&)str);
            assert(int(ii) < tdi->custom_items.size());
            const S_text_display_info::S_custom_item &itm = tdi->custom_items[ii];
            width += itm.sx;
            if(max_chars && (max_chars -= 4) <= 0)
               goto end;
         }
         continue;
      }
      if(c=='\n' || c=='\r')
         break;
      if(c=='\t')
         c = ' ';
      if(c>=' '){
         rem_last_pixel = true;
         const S_font &fd = GetFontDef(ts.font_index);
         if(!wide)
            c = encoding::ConvertCodedCharToUnicode(c, coding);
         int letter_width = GetCharWidth(wchar(c), (font_flags&FF_BOLD), (font_flags&FF_ITALIC), font_index);
         width += letter_width + 1 + fd.extra_space;
      }else
         assert(0);
      if(max_chars && !--max_chars)
         break;
   }
end:
                              //remove pixel-space after last letter
   if(width && rem_last_pixel)
      --width;

   return width;
}

//----------------------------

static void SaveBackgroundPixels(dword bytes_per_pixel, const S_rect &rc_save, C_buffer<byte> &transp_tmp_buf, void *backbuffer, dword screen_pitch){

   dword need_sz = rc_save.sx*rc_save.sy*bytes_per_pixel;
   if(transp_tmp_buf.Size()<need_sz)
      transp_tmp_buf.Resize(need_sz);
   const byte *src = (byte*)backbuffer + rc_save.y*screen_pitch + rc_save.x*bytes_per_pixel;
   byte *dst = transp_tmp_buf.Begin();
   for(int y=rc_save.sy; y--; src += screen_pitch, dst += rc_save.sx*bytes_per_pixel)
      MemCpy(dst, src, rc_save.sx*bytes_per_pixel);
}

//----------------------------
#if defined __SYMBIAN32__ || defined _WINDOWS || defined WINDOWS_MOBILE
#define SYSTEM_FONT_HAS_NO_ALPHA
#endif

#ifdef SYSTEM_FONT_HAS_NO_ALPHA
static void FinishTransparentTextDraw(const S_pixelformat &pf, dword dst_pitch, void *backbuffer, const S_rect &rc_save_bgnd, const C_buffer<byte> &transp_tmp_buf, dword text_color, byte alpha){

   const dword bytes_per_pixel = pf.bytes_per_pixel;
   byte *dst = (byte*)backbuffer + rc_save_bgnd.y*dst_pitch + rc_save_bgnd.x*bytes_per_pixel;
   const byte *src = transp_tmp_buf.Begin();
   S_rgb_x p_color(text_color), p_orig, p_text;
   for(int y=rc_save_bgnd.sy; y--; dst += dst_pitch, src += rc_save_bgnd.sx*bytes_per_pixel){
      switch(pf.bits_per_pixel){
#ifdef SUPPORT_12BIT_PIXEL
      case 12:
#error
         break;
#endif
      case 16:
         {
            word *s = (word*)src, *d = (word*)dst;
            for(int x=rc_save_bgnd.sx; x--; ++d){
               word t_pix = *d, o_pix = *s++;
               if(t_pix!=o_pix){
                  p_text.From16bit(t_pix);
                  p_orig.From16bit(o_pix);
                  p_orig.BlendWith(p_text, alpha);
                  o_pix = p_orig.To16bit();
                  *d = o_pix;
               }
            }
         }
         break;
      case 32:
         {
            dword *s = (dword*)src, *d = (dword*)dst;
            for(int x=rc_save_bgnd.sx; x--; ++d){
               dword t_pix = *d, o_pix = *s++;
               if(t_pix!=o_pix){
                  p_text.From32bit(t_pix);
                  p_orig.From32bit(o_pix);
                  p_orig.BlendWith(p_text, alpha);
                  o_pix = p_orig.To32bit();
                  *d = o_pix;
               }
            }
         }
         break;
      }
   }
}

#endif   //SYSTEM_FONT_HAS_NO_ALPHA
//----------------------------

int C_application_ui::DrawEncodedString(const void *str, bool wide, bool bottom_align, int x, int y, E_TEXT_CODING coding, S_text_style &ts, int max_width, const S_draw_fmt_string_data *fmt, const S_text_display_info *tdi){

   assert(ts.font_index>=0 && ts.font_index<NUM_FONTS);
   dword font_index = Max(0, Min(NUM_FONTS-1, ts.font_index));
   dword text_color = ts.href ? fmt->tdi->link_color : ts.text_color;

   int max_pixels = -1;
   int max_bytes = 0x7fffffff;
   if(max_width){
      if(max_width > 0)
         max_bytes = max_width;
      else
         max_pixels = -max_width;
   }

   int text_width = -1;

   if(ts.font_flags&(FF_CENTER|FF_RIGHT)){
                              //compute total width, and adjust x appropriately
      int max_c = Max(0, max_width);
      if(wide)
         max_c /= 2;
      text_width = GetTextWidth(str, wide, coding, font_index, ts.font_flags, max_c, fmt ? fmt->tdi : tdi) + 1;
      int width = text_width;
      if(max_pixels!=-1)
         width = Min(width, max_pixels);
      if(ts.font_flags&FF_RIGHT)
         x -= width;
      else
         x -= width/2;
   }

   int beg_x = x;
   dword bgnd_pix = 0;
   if(ts.bgnd_color){
      bgnd_pix = GetPixel(ts.bgnd_color);
      if(fmt && x!=fmt->rc_bgnd->x){
                              //fill by bgnd color from beg of line
         int sx = x - fmt->rc_bgnd->x;
         FillPixels(S_rect(fmt->rc_bgnd->x, y-fmt->line_height, sx, fmt->line_height), bgnd_pix);
      }
   }
   if(ts.block_quote_count){
                              //draw block quotes
      int sx = fds.cell_size_x;
      int w = Max(1, sx/4);
      dword pix = GetPixel(ts.text_color);
      for(int i=0; i<ts.block_quote_count; i++){
         FillPixels(S_rect(x+(sx-w)/2, y-fmt->line_height, w, fmt->line_height), pix);
         x += sx;
      }
   }

#define SUPPORT_JUSTIFY
#ifdef SUPPORT_JUSTIFY
   C_fixed space_width(0);
   C_fixed space_frac = 0;
   bool justify = (fmt && fmt->justify && tdi && max_width>0);
   if(justify){
      space_width = GetCharWidth(' ', (ts.font_flags&FF_BOLD), (ts.font_flags&FF_ITALIC), font_index);
                              //count spaces
      int num_spaces = 0;
      const void *vp = str;
      int num_chars = max_width/(wide+1);
      for(int i=num_chars; i--; ){
         dword c = text_utils::GetChar(vp, wide);
         if((c&0xff7f)==' '){
            if(i)
               ++num_spaces;
            else
               --num_chars;
         }
      }
      if(num_spaces){
         int w = GetTextWidth(str, wide, coding, font_index, ts.font_flags, num_chars, tdi) + 1;
         w = tdi->rc.sx - w + num_spaces*space_width;
         C_fixed sw = C_fixed(w-1) / C_fixed(num_spaces);
         if(sw < space_width*C_fixed(3))
            space_width = Max(space_width, sw);
      }
   }
#endif

   S_rect rc_save_bgnd(0, 0, 0, 0);
#ifdef SYSTEM_FONT_HAS_NO_ALPHA
   byte alpha = byte(text_color>>24);
   //alpha = 0x40;
   if(system_font.use)
   if(alpha!=0xff){
      ts.font_flags &= ~FF_ITALIC;
      {
                              //transparent text with system font, save original background
         if(text_width==-1)
            text_width = GetTextWidth(str, wide, coding, font_index, ts.font_flags, max_width, fmt ? fmt->tdi : tdi) + 1;
         int width = text_width + 1;
         if(max_pixels!=-1)
            width = Min(width, max_pixels);
         rc_save_bgnd = S_rect(x, y-1, width, GetFontDef(font_index).line_spacing+1);
         if(fmt)
            rc_save_bgnd.y -= fmt->line_height;
         dword rotation = (ts.font_flags>>FF_ROTATE_SHIFT)&3;
         if(rotation){
            if(rotation&1){
               Swap(rc_save_bgnd.x, rc_save_bgnd.y);
               Swap(rc_save_bgnd.sx, rc_save_bgnd.sy);
            }
            switch(rotation){
            case 1:
               rc_save_bgnd.y = ScrnSY()-rc_save_bgnd.y;
               rc_save_bgnd.y -= rc_save_bgnd.sy-1;
               break;
            case 3:
               rc_save_bgnd.x = ScrnSX()-rc_save_bgnd.x;
               rc_save_bgnd.x -= rc_save_bgnd.sx;
               break;
            }
         }
         rc_save_bgnd.SetIntersection(rc_save_bgnd, GetClipRect());
         if(!rc_save_bgnd.IsEmpty()){
            SaveBackgroundPixels(GetPixelFormat().bytes_per_pixel, rc_save_bgnd, system_font.transp_tmp_buf, GetBackBuffer(), GetScreenPitch());
         }
      }
   }
#endif
   int fade_pixels = -1;
   S_rect fade_save_crc;
   if(max_pixels!=-1){
                              //check if text will overflow
      if(text_width==-1)
         text_width = GetTextWidth(str, wide, coding, font_index, ts.font_flags, max_width, fmt ? fmt->tdi : tdi) + 1;
      if(text_width>max_pixels){
         const S_font &fd = GetFontDef(font_index);
         int fp = Min(max_pixels, fd.letter_size_x*3);
         S_rect rc_fade = S_rect(x+max_pixels-fp, y-1, fp, fd.line_spacing+1);
         rc_fade.SetIntersection(rc_fade, GetClipRect());
         if(!rc_fade.IsEmpty()){
            fade_pixels = rc_fade.sx;
                              //save background, if not yet
            if(rc_save_bgnd.IsEmpty()){
               rc_save_bgnd = rc_fade;

               SaveBackgroundPixels(GetPixelFormat().bytes_per_pixel, rc_save_bgnd, system_font.transp_tmp_buf, GetBackBuffer(), GetScreenPitch());
               //FillRect(rc_save_bgnd, 0x80000000);
            }
            fade_save_crc = GetClipRect();
            S_rect rc_clip;
            rc_clip.SetIntersection(S_rect(x, y-1, max_pixels, fd.line_spacing+1), fade_save_crc);
            if(!rc_clip.IsEmpty())
               SetClipRect(rc_clip);
         }
      }
   }

   int max_x = max_pixels==-1 ? 0x7fffffff : (x + max_pixels);
   for(int i=0; i<max_bytes && x<max_x; ){
      dword c = text_utils::GetChar(str, wide);
      if(!c)
         break;
      if(S_text_style::IsStyleCode(c)){
         assert(fmt);
         if(!fmt)
            return 0;
                              //control character
         const char *&cp1 = (const char*&)str;
         --cp1;
         dword prev_text_color = ts.text_color, prev_bgnd_color = ts.bgnd_color;
         bool pref_h = ts.href;
         i += ts.ReadCode(cp1, &fmt->tdi->active_link_index);
         if(prev_text_color != ts.text_color || pref_h!=ts.href){
            text_color = ts.href ? fmt->tdi->link_color : ts.text_color;
         }
         if(prev_bgnd_color != ts.bgnd_color && ts.bgnd_color)
            bgnd_pix = GetPixel(ts.bgnd_color);
         font_index = ts.font_index;
         if(c==CC_BLOCK_QUOTE_COUNT){
            //assert(0);
            //rc_width -= ts.block_quote_count*fds.cell_size_x;
         }
         continue;
      }
      switch(c){
      case CC_IMAGE:
         {
                                 //images are only in html text, and this is always rendered with non-NULL 'tdi'
            dword ii = ts.Read16bitIndex((const char*&)str);
            i += 4;
            assert(int(ii) < fmt->tdi->images.size());
            const S_image_record &ir = fmt->tdi->images[ii];
            x += DrawHtmlImage(ir, ts, fmt, x, y, bgnd_pix);
            if(tdi && ir.img){
                              //add to list of rendered images
               int i1;
               for(i1=tdi->last_rendered_images.size(); i1--; ){
                  if(tdi->last_rendered_images[i1]==ir.img)
                     break;
               }
               if(i1==-1)
                  tdi->last_rendered_images.push_back(ir.img);
            }
         }
         continue;

      case CC_CUSTOM_ITEM:
         {
            dword ii = ts.Read16bitIndex((const char*&)str);
            i += 4;
            assert(int(ii) < tdi->custom_items.size());
            const S_text_display_info::S_custom_item &itm = tdi->custom_items[ii];
            const S_font &fd = font_defs[font_index];
            S_rect rc(x, y-fd.line_spacing, itm.sx, itm.sy);
            (this->*itm.DrawProc)(rc, itm.custom_data);
            x += rc.sx;
         }
         continue;

#if defined WEB_FORMS
      case CC_FORM_INPUT:
         {
            bool active = (((const char*&)str - fmt->tdi->body_c) == fmt->tdi->active_object_offs+1);
            int ii = ts.Read16bitIndex((const char*&)str);
            i += 4;

            assert(ii < fmt->tdi->form_inputs.size());
            x += DrawHtmlFormInput(*fmt->tdi, fmt->tdi->form_inputs[ii], active, ts, fmt, x, y, bgnd_pix, (const char*)str);
         }
         continue;
#endif//WEB_FORMS

      case CC_WIDE_CHAR:
         c = ts.ReadWideChar((const char*&)str);
         i += 3;
         break;
      case CC_HORIZONTAL_LINE:
         {
            dword color;
            dword w = ts.ReadHorizontalLine((const char*&)str, tdi->rc.sx, color);
            dword x1 = tdi->rc.CenterX()-w/2;
            DrawHorizontalLine(x1, y-1, w, color);
         }
         goto out;
      }
      if(c=='\t')
         c = ' ';
      if(c >= ' '){
         i += (wide ? 2 : 1);
         const S_font &fd = font_defs[font_index];
         if(!wide)
            c = encoding::ConvertCodedCharToUnicode(c, coding);

         int letter_width;
#ifdef SUPPORT_JUSTIFY
         if((c&0xff7f)==' ' && justify){
            C_fixed w = space_width + space_frac;
            letter_width = w;
            space_frac = w.Frac();
         }else
#endif
            letter_width = GetCharWidth(wchar(c), (ts.font_flags&FF_BOLD), (ts.font_flags&FF_ITALIC), font_index);
         if(ts.bgnd_color){
            int yy = y;
            int h = fd.line_spacing;
            if(fmt){
               yy -= fmt->line_height;
               h = fmt->line_height;
            }
            FillPixels(S_rect(x, yy, letter_width+1+fd.extra_space, h), bgnd_pix);
         }
         int yy = y;
         if(bottom_align)
            yy -= fd.line_spacing;
         DrawLetter(x, yy, wchar(c), font_index, ts.font_flags, text_color);
         x += letter_width + 1;
         x += fd.extra_space;
      }else{
         i += (wide ? 2 : 1);
      }
   }
out:
   if(ts.bgnd_color && fmt){
                              //fill by bgnd color to end of line
      int sx = fmt->rc_bgnd->Right() - x;
      FillPixels(S_rect(x, y-fmt->line_height, sx, fmt->line_height), bgnd_pix);
   }
#ifdef SYSTEM_FONT_HAS_NO_ALPHA
   if(system_font.use)
   if(alpha!=0xff && !rc_save_bgnd.IsEmpty())
      FinishTransparentTextDraw(GetPixelFormat(), GetScreenPitch(), GetBackBuffer(), rc_save_bgnd, system_font.transp_tmp_buf, ts.href ? fmt->tdi->link_color : ts.text_color, alpha);
#endif

   //FinishTextDrawFading();
   if(fade_pixels!=-1){
      const S_pixelformat &pf = GetPixelFormat();
      const dword bytes_per_pixel = pf.bytes_per_pixel;
      dword dst_pitch = GetScreenPitch();
      byte *dst = (byte*)GetBackBuffer() + rc_save_bgnd.y*dst_pitch + (rc_save_bgnd.x+(rc_save_bgnd.sx-fade_pixels))*bytes_per_pixel;
      C_buffer<byte> &transp_tmp_buf = system_font.transp_tmp_buf;
      const byte *src = transp_tmp_buf.Begin() + (rc_save_bgnd.sx-fade_pixels)*bytes_per_pixel;
      //S_rgb_x p_color(text_color), p_orig, p_text;
      S_rgb_x p_d, p_s;
      C_fixed alpha_step = C_fixed::One() / C_fixed(fade_pixels+1);
      for(int y1=rc_save_bgnd.sy; y1--; dst += dst_pitch, src += rc_save_bgnd.sx*bytes_per_pixel){
         C_fixed alpha = 0;
         switch(pf.bits_per_pixel){
         case 16:
            {
               word *s = (word*)src, *d = (word*)dst;
               for(int x1=fade_pixels; x1--; ){
                  alpha += alpha_step;
                  p_d.From16bit(*d);
                  p_s.From16bit(*s++);
                  p_d.BlendWith(p_s, alpha.val>>8);
                  *d++ = p_d.To16bit();
               }
            }
            break;
         case 32:
            {
               dword *s = (dword*)src, *d = (dword*)dst;
               for(int x1=fade_pixels; x1--; ){
                  alpha += alpha_step;
                  p_d.From32bit(*d);
                  p_s.From32bit(*s++);
                  p_d.BlendWith(p_s, alpha.val>>8);
                  *d++ = p_d.To32bit();
               }
            }
            break;
         }
      }
      SetClipRect(fade_save_crc);
   }

   x = Min(x, max_x);
   return x - beg_x - 1;
}

//----------------------------

int C_application_ui::DrawString2(const void *str, bool wide, int x, int y, dword font_index, dword font_flags, dword color, int max_width){

   S_text_style ts(font_index, font_flags, color);
   if(font_flags&FF_SHADOW_ALPHA_MASK){
      const dword shadow_alpha = (font_flags>>FF_SHADOW_ALPHA_SHIFT)&0xff;
      dword a = ((color>>24)*shadow_alpha)>>8;
      if(a){
         ts.text_color = a<<24;
         const S_font &fd = font_defs[font_index];
         const int offs = Max(1, fd.line_spacing/12);
         DrawEncodedString(str, wide, false, x+offs, y+offs, COD_DEFAULT, ts, max_width);
         ts.text_color = color;
      }
   }
   return DrawEncodedString(str, wide, false, x, y, COD_DEFAULT, ts, max_width);
}

//----------------------------

void C_application_ui::DetermineTextLinesLengths(const wchar *wp, int font_index, int rect_width, int cursor_pos, C_vector<S_text_line> &lines, int &cursor_line, int &cursor_col, int &cursor_x_pixel) const{

   S_text_style ts; ts.font_index = font_index;
   cursor_x_pixel = 0;
   cursor_line = 0;
   cursor_col = 0;
   while(true){
      int line_height;
      int w = ComputeFittingTextLength(wp, true, rect_width, NULL, ts, line_height, font_index) / sizeof(wchar);
      if(cursor_pos>=0 && cursor_pos<=w){
         cursor_line = lines.size();
         cursor_col = cursor_pos;
         if(cursor_col){
            cursor_x_pixel = GetTextWidth(wp, font_index, 0, cursor_col);
            cursor_x_pixel = Max(0, cursor_x_pixel-font_defs[font_index].letter_size_x/2);
         }else
            cursor_x_pixel = 0;
      }
      cursor_pos -= w;
      S_text_line l = {wp, w};
      lines.push_back(l);
      if(!w || (!wp[w] && wp[w-1]!='\n'))
         break;
      wp += w;
   }
}

//----------------------------

int C_application_ui::DrawHtmlImage(const S_image_record &ir, const S_text_style &ts, const S_draw_fmt_string_data *fmt, int x, int y, dword bgnd_pix){

   if(ts.bgnd_color)
      FillPixels(S_rect(x, y-fmt->line_height, ir.sx, fmt->line_height), bgnd_pix);

   int xx = x, yy = y-ir.sy;
   if(ir.draw_border)
      ++xx, --yy;

   if(!ir.img){
                        //draw image placeholder
      S_rect rc(xx+1, yy+1, ir.sx-2, ir.sy-2);
      DrawOutline(rc, 0x80202020, 0x80808080);

      S_rect rc_u;
      rc_u.SetIntersection(GetClipRect(), rc);
//#ifdef MAIL
      //sset_mail->DrawSprite(rc.x+1, rc.y+1, ir.invalid ? SPR_IMG_INVALID : SPR_IMG_PLACEHOLDER);
      if(ir.alt.Length()){
                        //draw alt text
         --rc_u.sx;
         if(!rc_u.IsEmpty()){
            //const C_spriteset::S_sprite_rect &spr_rc = sset_mail->GetSpriteRect(SPR_IMG_PLACEHOLDER);
            S_rect save_cr = GetClipRect();
            SetClipRect(rc_u);
            DrawString(ir.alt, rc.x+1, rc.y+2, UI_FONT_SMALL, 0, 0xffa0a0a0);
            SetClipRect(save_cr);
         }
      }
//#endif
   }else{
      int top = y - fmt->line_height;
      switch(ir.align_mode&S_image_record::ALIGN_V_MASK){
      case S_image_record::ALIGN_V_TOP:
         yy = top;
         break;
      case S_image_record::ALIGN_V_BOTTOM:
         {
            yy = Max(top, yy - (fds.line_spacing-fds.cell_size_y));
         }
         break;
      case S_image_record::ALIGN_V_CENTER:
         yy = (top+yy)/2;
         break;
      }
      ir.img->Draw(xx, yy);
   }
   if(ts.href){
      S_rect rc(xx, yy, ir.sx, ir.sy);
      if(!ir.draw_border)
         rc.Compact();
      bool active = (ts.font_flags&FF_ACTIVE_HYPERLINK);
      if(ir.draw_border || active){
      dword col = fmt->tdi->link_color & 0xffffff;
      if(!active){
         col |= 0x80000000;
         DrawOutline(rc, col, col);
      }else{
         DrawDashedOutline(rc, col, 0);
         rc.Compact();
         DrawDashedOutline(rc, col, 0xffffff);
      }
      }
   }else
   if(ir.draw_border){
      S_rect rc(xx, yy, ir.sx, ir.sy);
      DrawOutline(rc, COLOR_BLACK, COLOR_BLACK);
   }
   return ir.sx + (ir.draw_border ? 2 : 0);
}

//----------------------------

int C_application_ui::ComputeFittingTextLength(const void *cp, bool wide, int rc_width, const S_text_display_info *tdi, S_text_style &ts, int &line_height, int default_font_index) const{

   int size_of_char = wide ? 2 : 1;

   //line_height = 1;
   {
      int fi = ts.font_index;
      if(fi==-1)
         fi = default_font_index;
      const S_font &fd = font_defs[fi];
      line_height = fd.line_spacing;
      rc_width -= ts.block_quote_count*fds.cell_size_x;
   }
   int begin_rc_width = rc_width;
   int last_good_i = 0;
   S_text_style last_good_ts = ts;
   int last_good_height = line_height;
   int i;
   dword prev_char = 0;

   for(i=0; rc_width>=0; ){
      dword c = text_utils::GetChar(cp, wide);
      if(!c)
         return i * size_of_char;
      if(c=='\n')
         return (i + 1) * size_of_char;
      if(c=='\r'){
         ++i;
         continue;
      }
      if(S_text_style::IsStyleCode(c)){
         assert(!wide);
         if(wide || !tdi)
            break;
         const char *&cp1 = (const char*&)cp;
         --cp1;
         i += ts.ReadCode(cp1, &tdi->active_link_index);
         if(c==CC_BLOCK_QUOTE_COUNT){
            rc_width -= ts.block_quote_count*fds.cell_size_x;
            begin_rc_width = rc_width;
         }
         continue;
      }
      if(c==CC_IMAGE)
         prev_char = ' ';
      if(i){
                              //accept good separators
         switch(prev_char){
         default:
                              //any japanese/chinese
            if(!(prev_char>=0x2e00 && prev_char<0xa000))
               break;
         case ' ':
         case ';':
         case ',':
         case 0xff0c:         //,
         case '/':
         case '\\':
         case ':':
            last_good_i = i;
            last_good_ts = ts;
            last_good_height = line_height;
            break;
         case '.':
         case '!':
         case 0xff01:         //!
         case 0xff0e:         //.
         case 0xff1f:         //?
         case '?':
                              //if followed by white-space
            dword next = wide ? *(wchar*)cp : *(char*)cp;
            if(next<=' '){
               last_good_i = i;
               last_good_ts = ts;
               last_good_height = line_height;
            }
            break;
         }
      }
      switch(c){
      case CC_IMAGE:
         {
            if(wide || !tdi)
               break;
            dword ii = ts.Read16bitIndex((const char*&)cp);
            i += 4;
            assert(int(ii) < tdi->images.size());
            const S_image_record &ir = tdi->images[ii];
            int sx = ir.sx, sy = ir.sy;
            if(ir.draw_border)
               sx += 2, sy += 2;
            line_height = Max(line_height, sy);

            prev_char = ' ';     //make image to be 'good separator' from text on right (recognized on next loop)
            if(rc_width==begin_rc_width){
                              //image is first on line, but exceeds rc width, should not happen, it's failsafe here
               last_good_i = i;
               last_good_ts = ts;
               last_good_height = line_height;
            }
            rc_width -= sx;
         }
         continue;

      case CC_CUSTOM_ITEM:
         {
            dword ii = ts.Read16bitIndex((const char*&)cp);
            i += 4;
            assert(int(ii) < tdi->custom_items.size());
            const S_text_display_info::S_custom_item &itm = tdi->custom_items[ii];
            rc_width -= itm.sx;
            line_height = Max(line_height, int(itm.sy));
            prev_char = ' ';
         }
         continue;

#ifdef WEB_FORMS
      case CC_FORM_INPUT:
         {
            if(wide) break;
            dword ii = ts.Read16bitIndex((const char*&)cp);
            i += 4;
            const S_html_form_input &fi = tdi->form_inputs[ii];
            rc_width -= fi.sx;
            line_height = Max(line_height, fi.sy);

            prev_char = ' ';
         }
         continue;
#endif//WEB_FORMS

      case CC_WIDE_CHAR:
         c = ts.ReadWideChar((const char*&)cp);
         i += 3;
         break;
      case CC_HORIZONTAL_LINE:
         if(!i){
            line_height = 1;
            return 7;
         }
         return i * size_of_char;
      }
      if(!wide)
         c = encoding::ConvertCodedCharToUnicode(c, tdi->body_c.coding);

      int fi = ts.font_index;
      if(fi==-1)
         fi = default_font_index;
      const S_font &fd = font_defs[fi];
      line_height = Max(line_height, fd.line_spacing);

      int letter_width = GetCharWidth(wchar(c), (ts.font_flags&FF_BOLD), (ts.font_flags&FF_ITALIC), fi);
      rc_width -= letter_width + 1 + fd.extra_space;
      prev_char = c;
      ++i;
   }
   ts = last_good_ts;
   line_height = last_good_height;
                        //if too long word, just cut it in half
   if(!last_good_i){
      if(!i)
         return 0;
                              //roll back one character
      --i;
      if(!wide && tdi){
         const char *cp1 = (char*)cp;
         char c = cp1[-1];
         if(S_text_style::IsControlCode(c))
            i -= ts.ReadCodeBack(cp1, &tdi->active_link_index)-1;
      }
      return i * size_of_char;
   }
   return last_good_i * size_of_char;
}

//----------------------------

void C_application_ui::CountTextLinesAndHeight(S_text_display_info &tdi, dword default_font_index) const{

   bool wide = tdi.is_wide;
   const void *cp = wide ? (const char*)(const wchar*)tdi.body_w : (const char*)tdi.body_c;

   tdi.num_lines = 0;
   tdi.total_height = 0;

   S_text_style ts = tdi.ts;
   while(true){
      int h;
      int len = ComputeFittingTextLength(cp, wide, tdi.rc.sx, &tdi, ts, h, default_font_index);
      if(!len)
         break;
      tdi.total_height += h;
      ++tdi.num_lines;
      (char*&)cp += len;
   }
                              //we must have at least one line
   if(!tdi.num_lines)
      ++tdi.num_lines;
}

//----------------------------

bool C_application_ui::ScrollText(S_text_display_info &tdi, int scroll_pixels) const{

   bool wide = tdi.is_wide;

   const char *cp = wide ? (const char*)(const wchar*)tdi.body_w : (const char*)tdi.body_c;
   cp += tdi.byte_offs;

   if(scroll_pixels > 0){
                              //clip - make sure we don't scroll too far
      scroll_pixels = Min(scroll_pixels, tdi.total_height-tdi.rc.sy-tdi.top_pixel);
      if(scroll_pixels <= 0)
         return false;
      tdi.top_pixel += scroll_pixels;

      while(scroll_pixels){
                              //get current line's length
         int line_height;
         S_text_style ts = tdi.ts;
         int len = ComputeFittingTextLength(cp, wide, tdi.rc.sx, &tdi, ts, line_height, UI_FONT_BIG);
         //assert(len);

         int pixels_left = line_height - tdi.pixel_offs;
         if(scroll_pixels < pixels_left){
                           //only pixel scroll, not line
            tdi.pixel_offs += scroll_pixels;
            break;
         }
                              //go to next line
         scroll_pixels -= pixels_left;
         tdi.pixel_offs = 0;
         tdi.ts = ts;

         tdi.byte_offs += len;
         (char*&)cp += len;
      }
   }else{
                              //clip
      scroll_pixels = Max(scroll_pixels, -tdi.top_pixel);
      if(!scroll_pixels)
         return false;
      tdi.top_pixel += scroll_pixels;

      while(scroll_pixels){
         if(-scroll_pixels <= tdi.pixel_offs){
                              //only pixel scroll, not line
            tdi.pixel_offs += scroll_pixels;
            break;
         }
                              //find prev line begin
         const char *pp = cp;
         int new_offs = tdi.byte_offs;
         if(!wide){
            while(new_offs){
               --new_offs;
               --pp;
               if(S_text_style::IsControlCode(*pp)){
                  ++pp;
                  new_offs -= tdi.ts.ReadCodeBack(pp, &tdi.active_link_index) - 1;
               }
               if(pp[-1]=='\n')
                  break;
            }
         }else{
            const wchar *wp = (wchar*)pp;
            while(new_offs){
               new_offs -= sizeof(wchar);
               --wp;
               if(wp[-1]=='\n'){
                  break;
               }
            }
            pp = (char*)wp;
         }
         S_text_style ts = tdi.ts;
         cp = pp;
         int last_height;
         for(;;){
            int len = ComputeFittingTextLength(pp, wide, tdi.rc.sx, &tdi, ts, last_height, UI_FONT_BIG);
            if(new_offs+len >= int(tdi.byte_offs))
               break;
            new_offs += len;
            pp += len;
            cp = pp;
            tdi.ts = ts;
         }
         tdi.pixel_offs += last_height;
         tdi.byte_offs = new_offs;
         //scroll_pixels += last_height;
      }
   }
   return true;
}

//----------------------------

void C_application_ui::DrawFormattedText(const S_text_display_info &tdi, const S_rect *rc_bgnd){

   S_rect rc_clip(0, 0, ScrnSX(), ScrnSY());
   rc_clip.SetIntersection(rc_clip, tdi.rc);
   if(rc_bgnd)
      rc_clip.SetIntersection(rc_clip, *rc_bgnd);
   if(rc_clip.IsEmpty())
      return;

   if(!rc_bgnd)
      rc_bgnd = &tdi.rc;

   if(tdi.bgnd_image!=-1 && tdi.images[tdi.bgnd_image].img){
                              //fill tiled image
      SetClipRect(*rc_bgnd);
      const C_image *img = tdi.images[tdi.bgnd_image].img;
                              //if transparent image, also clear
      if(img->IsTransparent())
         FillRect(*rc_bgnd, tdi.bgnd_color | 0xff000000);
      for(int y=rc_bgnd->y; y<rc_bgnd->Bottom(); y += img->SizeY()){
         for(int x=rc_bgnd->x; x<rc_bgnd->Right(); x += img->SizeX()){
            img->Draw(x, y);
         }
      }
      ResetClipRect();
   }else
   if(!tdi.HasDefaultBgndColor())
      FillRect(*rc_bgnd, tdi.bgnd_color | 0xff000000);
   AddDirtyRect(*rc_bgnd);
#ifdef _DEBUG_
   DrawOutline(tdi.rc, 0x80ff0000, 0x80ff0000);  //!
#endif

   tdi.last_rendered_images.clear();
                              //determine how many lines we're going to draw
   int y = tdi.rc.y - tdi.pixel_offs;
   bool wide = tdi.is_wide;
   const char *cp = wide ? (const char*)(const wchar*)tdi.body_w : (const char*)tdi.body_c;
   cp += tdi.byte_offs;

   SetClipRect(rc_clip);
   S_text_style ts = tdi.ts;
   if(ts.font_index==-1)
      ts.font_index = UI_FONT_BIG;

   S_draw_fmt_string_data fmt_data = { &tdi, 0, rc_bgnd, false };
   int offs = tdi.byte_offs;

   while(y < tdi.rc.Bottom()){
                              //draw line
      S_text_style ts1 = ts;
      int len = ComputeFittingTextLength(cp, wide, tdi.rc.sx, &tdi, ts1, fmt_data.line_height, ts.font_index);
      if(!len)
         break;
                              //pre-read all control styles, so that we have fetched CENTER and RIGHT options
      if(!wide){
         while(S_text_style::IsStyleCode(*cp)){
            int cl = ts.ReadCode(cp, &tdi.active_link_index);
            len -= cl;
            offs += cl;
         }
      }

      fmt_data.justify = false;
      int x = tdi.rc.x;
      if(ts.font_flags&FF_CENTER)
         x += tdi.rc.sx/2;
      else
      if(ts.font_flags&FF_RIGHT)
         x += tdi.rc.sx;
      else{
         if(cp[len-1]!='\n' && cp[len]!=0){
            fmt_data.justify = tdi.justify;
         }
      }

      int yy = y + fmt_data.line_height;
      bool drawn = false;
      if(tdi.mark_end){
         if(offs<tdi.mark_end && (offs+len)>tdi.mark_start){
            const byte *cp1 = (const byte*)cp;
            int xx = x;
            int len1 = len;
            int l1 = tdi.mark_start-offs;
            int offs1 = offs;
            if(l1>0){
                              //draw text before
               xx += DrawEncodedString(cp1, wide, true, xx, yy, tdi.body_c.coding, ts, l1, &fmt_data, &tdi) + 1;
               cp1 += l1;
               offs1 += l1;
               len1 -= l1;
            }
            int l2 = (offs1+len1) - tdi.mark_end;
            if(l2>0)
               len1 -= l2;
            int w = GetTextWidth(cp1, wide, tdi.body_c.coding, ts.font_index, ts.font_flags, len1>>int(wide), &tdi) + 1;

            DrawEncodedString(cp1, wide, true, xx, yy, tdi.body_c.coding, ts, len1, &fmt_data, &tdi);
            XorRect(S_rect(xx-1, yy-fmt_data.line_height, w+1, fmt_data.line_height));
            xx += w;
            if(l2>0){
               cp1 += len1;
               DrawEncodedString(cp1, wide, true, xx, yy, tdi.body_c.coding, ts, l2, &fmt_data, &tdi);
            }
            drawn = true;
         }
      }
      if(!drawn)
         DrawEncodedString(cp, wide, true, x, yy, tdi.body_c.coding, ts, len, &fmt_data, &tdi);
      cp += len;
      offs += len;
                              //next line
      y += fmt_data.line_height;
   }
   ResetClipRect();
}


//----------------------------

void C_application_ui::ClearTitleBar(int bottom){

   if(bottom==-1)
      bottom = GetTitleBarHeight();
   ClearToBackground(S_rect(0, 0, ScrnSX(), bottom));
}

//----------------------------

void C_application_ui::DrawDialogTitle(const S_rect &rc, const wchar *txt){

   const dword col_text = GetColor(COL_TEXT_POPUP);
   int sx = rc.sx;
   DrawString(txt, rc.x+fdb.cell_size_x, rc.y+1, UI_FONT_BIG, FF_BOLD, col_text, -(sx-fdb.letter_size_x*1));
}

//----------------------------

void C_application_ui::DrawTitleBar(const wchar *txt, int clear_bar_sy, bool draw_titlebar_buttons){

   if(clear_bar_sy)
      ClearTitleBar(clear_bar_sy);

   dword sx = ScrnSX();
   const dword color = GetColor(COL_TEXT_TITLE);
   int x = sx/2, y = fdb.cell_size_y/12;
   const dword fflags = FF_CENTER | FF_BOLD;
   int max_w = sx-10;
#ifdef S60
   if(GetScreenRotation()==ROTATION_90CW && !HasMouse()){
      max_w -= fdb.letter_size_x*8;
      x -= fdb.letter_size_x*4;
   }
#endif
   int sp_sx = fdb.cell_size_x*15;
   //DrawThickSeparator(x-sp_sx/2, sp_sx, 2+fdb.cell_size_y+1);
   DrawThickSeparator(x-sp_sx/2, sp_sx, Min((int)GetTitleBarHeight()-2, y+fdb.cell_size_y));
   DrawString(txt, x, y, UI_FONT_BIG, (0x20<<FF_SHADOW_ALPHA_SHIFT) | fflags, color, -max_w);

#ifndef ANDROID
#ifdef USE_MOUSE
                              //draw titlebar buttons
   if(draw_titlebar_buttons && HasMouse() && GetScreenRotation()!=ROTATION_90CW){
      const dword col = MulAlpha(color, 0x8000);
      for(int i=0; i<TITLE_BUTTON_LAST; i++){
         dword lt = 0x80ffffff, rb = 0x40000000;
         const C_button &but = soft_buttons[2+i];
         if(but.down)
            Swap(lt, rb);
         DrawOutline(but.GetRect(), lt, rb);

         int SZ = but.GetRect().sx/2;
         int x = but.GetRect().x+SZ/2;
         int y = but.GetRect().y+SZ/2;
         if(but.down)
            ++x, ++y;

         switch(i){
         case TITLE_BUTTON_MINIMIZE:
            {
               S_rect rc(x, y, SZ, SZ);
               DrawOutline(rc, col, col);
               if(!(GetGraphicsFlags()&IG_SCREEN_USE_CLIENT_RECT)){
                  rc.sy = SZ/3;
                  FillRect(rc, col);
               }
            }
            break;
         case TITLE_BUTTON_MAXIMIZE:
            DrawArrowVertical(x, y, SZ, col, true);
            break;
         }
      }
   }
#endif
#endif
}

//----------------------------

dword C_application_ui::GetSoftButtonBarHeight() const{

   int sy = fdb.line_spacing*4/3;
   if(HasMouse()){
      S_point fsz = GetFullScreenSize();
      //S_point ssz(ScrnSX(), ScrnSY());
      //if(ScrnSY() >= ScrnSX()*7/4)
      //if(ScrnSY() >= ScrnSX())
      if(fsz.y >= fsz.x)
         sy = fdb.line_spacing*6/3;
                              //button height must be at least some height
      sy = Max(sy, (fsz.x+fsz.y)/28);
      sy = Min(sy, fsz.y/10);
      //Fatal("y", sy);
   }
   return sy;
}

//----------------------------

void C_application_ui::ClearSoftButtonsArea(int top){

   if(top==-1)
      top = ScrnSY() - GetSoftButtonBarHeight();
   ClearToBackground(S_rect(0, top, ScrnSX(), ScrnSY()-top));
}

//----------------------------

void C_application_ui::InitSoftButtonsRectangles(){

   C_button *sb = soft_buttons;
                           //init buttons rects
   S_rect rc(S_rect(1, 0, ScrnSX()/4-1, 0));
   int sy = GetSoftButtonBarHeight();
   if(!HasMouse())
      sy = sy*3/4;
   else
      sy = sy*7/8;
   rc.sy = Max(0, sy - 1);
   rc.y = ScrnSY() - (rc.sy+1);
   sb[0].SetRect(rc);

   rc.x = ScrnSX()-rc.sx-1;
   sb[1].SetRect(rc);

   switch(GetScreenRotation()){
   case ROTATION_NONE:
   case ROTATION_90CCW:
      break;
   case ROTATION_90CW:
      if(!HasMouse()){
         sb[0] = sb[1];
         rc.y = 1;
         sb[1].SetRect(rc);
#if defined S60 && defined __SYMBIAN_3RD__
                              //check Nokia E90, swap names for it
         if(system::GetDeviceId()==0x20002496 && ScrnSX()==800)
            Swap(sb[0], sb[1]);
#endif
      }
      break;
   default:
      assert(0);
   }

#if defined USE_MOUSE && !defined ANDROID
                              //init titlebar buttons
   for(int i=TITLE_BUTTON_LAST; i--; ){
      C_button &but = sb[2+i];
      but.down = false;
      but.clicked = false;

      S_rect rc = but.GetRect();
      if(i==TITLE_BUTTON_LAST-1){
         int SZ = fdb.line_spacing*2/3;
         SZ = (SZ+1) & -2;
         rc = S_rect(ScrnSX()-SZ-fdb.cell_size_x/2, (fdb.line_spacing-SZ)/2, SZ, SZ);
      }else{
         const S_rect &rc1 = sb[2+i+1].GetRect();
         rc = rc1;
         rc.x -= rc.sx + 3;
      }
      but.SetRect(rc);
   }
#endif
}

//----------------------------

void C_application_ui::DrawSoftButtons(const wchar *menu, const wchar *secondary){

   if(!GetSoftButtonBarHeight())
      return;
   const dword color = GetColor(COL_TEXT_SOFTKEY);
   E_ROTATION rot = GetScreenRotation();
#ifdef UI_MENU_ON_RIGHT
   if(menu_on_right)
      Swap(menu, secondary);
#endif
   for(int i=0; i<2; i++){
      const wchar *str = !i ? menu : secondary;
      if(!str)
         continue;
      dword flg = FF_BOLD;
      int shadow_size = ScrnSX()/128;

      const C_button &but = soft_buttons[i];
      //int y = but.rc.y+1;
      int y = but.GetRect().CenterY() - fdb.line_spacing/2;
      y = Max(y, but.GetRect().y+1);
      int x = but.GetRect().x;
      if(i || rot==ROTATION_90CW){
         x = but.GetRect().Right();
         flg |= FF_RIGHT;
      }
      int max_w = 0;
      if(DrawSoftButtonRectangle(but)){
         x = but.GetRect().CenterX();
         flg &= ~FF_RIGHT;
         flg |= FF_CENTER;
         max_w = -but.GetRect().sx;
      }else
         AddDirtyRect(but.GetRect());
      if(but.down)
         ++x, ++y, --shadow_size;
      if(shadow_size>0)
         flg |= 0x40<<FF_SHADOW_ALPHA_SHIFT;
      DrawString(str, x, y, UI_FONT_BIG, flg, color, max_w);
   }
}

//----------------------------

void C_application_ui::DrawSoftButtonsBar(const C_mode &mod, dword left_txt_id, dword right_txt_id, const C_text_editor *te){

   if(!mod.IsActive())
      return;
#ifndef _WIN32_WCE
   if(mod.GetMenu() && !HasMouse())
      return;
#endif
   if(te && !mod.GetMenu()){
      DrawTextEditorState(*te, left_txt_id, right_txt_id);
   }
   DrawSoftButtons(GetText(left_txt_id), GetText(right_txt_id));
}

//----------------------------

static dword GetDitheredPixel(int r, int g, int b, dword bits_per_pixel){

   dword pl, pr;

                        //make simple dithering for some pixel formats
   switch(bits_per_pixel){
#ifdef SUPPORT_12BIT_PIXEL
   case 12:
      pl = ((r&0xf0)<<4) | (g&0xf0) | ((b&0xf0)>>4);
      r = Min(255, r+(r&15));
      g = Min(255, g+(g&15));
      b = Min(255, b+(b&15));
      pr = ((r&0xf0)<<4) | (g&0xf0) | ((b&0xf0)>>4);
      break;
   case 15:
      pl = ((r&0xf8)<<7) | ((g&0xf8)<<2) | ((b&0xf8)>>3);
      r = Min(255, r+(r&7));
      g = Min(255, g+(g&7));
      b = Min(255, b+(b&7));
      pr = ((r&0xf8)<<7) | ((g&0xf8)<<2) | ((b&0xf8)>>3);
      break;
#endif
   case 16:
      pl = ((r&0xf8)<<8) | ((g&0xfc)<<3) | ((b&0xf8)>>3);
      r = Min(255, r+(r&7));
      g = Min(255, g+(g&3));
      b = Min(255, b+(b&7));
      pr = ((r&0xf8)<<8) | ((g&0xfc)<<3) | ((b&0xf8)>>3);
      break;
   default:
      return (r<<16) | (g<<8) | b;
   }
   return pl | pr<<16;
}

//----------------------------

void C_application_ui::FillRectGradient(const S_rect &rc, dword col_lt, dword col_rb){

   const S_pixelformat &pf = GetPixelFormat();
   int scrn_sy = ScrnSY();

   int x = rc.x, y = rc.y;
   int sx = rc.sx, sy = rc.sy;
   const S_rect &curr_clip_rect = GetClipRect();
                              //clip
   if(x<curr_clip_rect.x){ sx+=x-curr_clip_rect.x; x = curr_clip_rect.x; }
   if(y<curr_clip_rect.y){ sy+=y-curr_clip_rect.y; y = curr_clip_rect.y; }
   if(x+sx>curr_clip_rect.Right()) sx = curr_clip_rect.Right()-x;
   if(y+sy>curr_clip_rect.Bottom()) sy = curr_clip_rect.Bottom()-y;
   if(sx<=0 || sy<=0)
      return;

   const dword scrn_pitch = GetScreenPitch();
   byte *dst = (byte*)GetBackBuffer() + y*scrn_pitch + x*pf.bytes_per_pixel;

                              //create line
   byte *line = new(true) byte[sx*2*sizeof(dword)];
   {
      C_fixed r0 = (col_lt>>16)&0xff, g0 = (col_lt>>8)&0xff, b0 = col_lt&0xff;
      C_fixed r1 = (col_rb>>16)&0xff, g1 = (col_rb>>8)&0xff, b1 = col_rb&0xff;
      C_fixed r_div = C_fixed(256) / C_fixed(sx*2);
      C_fixed rs = ((r1-r0) * r_div)>>8, gs = ((g1-g0) * r_div)>>8, bs = ((b1-b0) * r_div)>>8;
      for(int i=0; i<sx*2; i++){
         dword pix = GetDitheredPixel(r0, g0, b0, pf.bits_per_pixel);
         switch(pf.bytes_per_pixel){
         case 2:
            if(i&1) pix = (pix>>16) | (pix<<16);
            ((word*)line)[i] = word(pix&0xffff);
            ((word*)line)[sx*2+i] = word(pix>>16);
            break;
         case 4:
            ((dword*)line)[i] = pix;
            break;
         }
         r0 += rs;
         g0 += gs;
         b0 += bs;
      }
   }
                              //copy the line to rect, adjusting source
   const byte *src = line;
   C_fixed offs = C_fixed::Percent(50), step = C_fixed(sx)/C_fixed(sy);
   bool flip = false;
   int dst_y = y;
   for(int y1=0; y1<sy; y1++, dst_y++){
      int offs_i = offs;
      const byte *src_p = src+offs_i*pf.bytes_per_pixel;
      if(pf.bytes_per_pixel==2 && flip) src_p += sx*2*2;
      if(dst_y>=0 && dst_y<scrn_sy)
         MemCpy(dst, src_p, sx*pf.bytes_per_pixel);
      dst += scrn_pitch;
      offs += step;
      if(!((offs_i^(int)offs)&1))
         flip = !flip;
   }
   delete[] line;
}

//----------------------------

void C_application_ui::FillRectGradient(const S_rect &rc, dword col){

   FillRectGradient(rc, MulColor(col, 0x14000), MulColor(col, 0xc000));
}

//----------------------------

void C_application_ui::XorRect(const S_rect &rc) const{

   byte *dst = (byte*)GetBackBuffer();
   const dword bpp = GetPixelFormat().bytes_per_pixel;
   const dword dst_pitch = GetScreenPitch();

   const S_rect &curr_clip_rect = GetClipRect();

   int l = Max(curr_clip_rect.x, rc.x);
   int t = Max(curr_clip_rect.y, rc.y);
   int r = Min(curr_clip_rect.x+curr_clip_rect.sx, rc.x+rc.sx);
   int b = Min(curr_clip_rect.y+curr_clip_rect.sy, rc.y+rc.sy);
   int sx = r - l;
   int sy = b - t;
   if(sx<=0 || sy<=0)
      return;
   dst += t * dst_pitch + l * bpp;
   for(; sy--; dst += dst_pitch){
      if(bpp==2){
         word *line = (word*)dst;
         for(int i=sx; i--; )
            *line++ ^= 0xffff;
      }else{
         dword *line = (dword*)dst;
         for(int i=sx; i--; )
            *line++ ^= 0xffffffff;
      }
   }
}

//----------------------------

dword C_application_ui::GetDialogTitleHeight() const{
   return fdb.line_spacing*10/8;
}

//----------------------------

dword C_application_ui::GetTitleBarHeight() const{
   //return fdb.cell_size_y*10/8;
   return fdb.line_spacing+fdb.cell_size_y/12+2;
}

//----------------------------

bool C_application_ui::IsWideScreen() const{
   return (ScrnSX() >= ScrnSY()*7/4);
}

//----------------------------

void C_application_ui::DrawShadow(const S_rect &rc, bool expand){

   InitShadow();
   if(!shadow[0]){
      DrawSimpleShadow(rc, expand);
   }else{
                              //bitmap shadow
      S_rect rc1 = rc;
      if(expand)
         rc1.Expand();
      if(rc1.IsEmpty())
         return;

      const C_image *rt = shadow[SHD_RT],
         *rt1 = shadow[SHD_RT1],
         *rb1 = shadow[SHD_RB1],
         *rb = shadow[SHD_RB],
         *l = shadow[SHD_L],
         *b = shadow[SHD_B],
         *lb = shadow[SHD_LB],
         *lb1 = shadow[SHD_LB1],
         *t = shadow[SHD_T],
         *lt = shadow[SHD_LT],
         *lt1 = shadow[SHD_LT1],
         *r = shadow[SHD_R];
      S_rect crc = GetClipRect();

      const int lt_x = rc1.x-lt1->SizeX();
      const int lb_x = rc1.x-lb1->SizeX();
      //*
      {
                              //draw top
         const int lt_y = rc1.y-lt->SizeY(), rt_right = rc1.Right()+rt1->SizeX();
         const int rt_x = rt_right-rt->SizeX(), rt_y = rc1.y-rt->SizeY();

         const int overlap_lt = lt->SizeX()-lt1->SizeX(),
            overlap_rt = rt->SizeX()-rt1->SizeX();
         if(overlap_lt+overlap_rt <= rc1.sx){
            lt->Draw(lt_x, lt_y);
            rt->Draw(rc1.Right()+rt1->SizeX()-rt->SizeX(), rt_y);
                                 //top line
            int dx = lt->SizeX()-lt1->SizeX();
            S_rect rc_t(rc1.x+dx, rc1.y-t->SizeY(), rc1.sx-(rt->SizeX()-rt1->SizeX())-dx, t->SizeY());
            S_rect rc2;
            rc2.SetIntersection(crc, rc_t);
            if(!rc2.IsEmpty()){
               SetClipRect(rc2);
               for(int x=0; x<rc_t.sx; x+=b->SizeX())
                  t->Draw(rc_t.x+x, rc_t.y);
            }
         }else{
                                 //top corners would overlap, draw them clipped
            int center = (lt_x+rt_right)/2;
            S_rect rc_l(lt_x, lt_y, center-lt_x, lt->SizeY());
            S_rect rc2;
            rc2.SetIntersection(crc, rc_l);
            if(!rc2.IsEmpty()){
               SetClipRect(rc2);
               lt->Draw(lt_x, lt_y);
            }
            S_rect rc_r(center, rt_y, rt_right-center, rt->SizeY());
            rc2.SetIntersection(crc, rc_r);
            if(!rc2.IsEmpty()){
               SetClipRect(rc2);
               rt->Draw(rt_x, rt_y);
            }
         }
         SetClipRect(crc);
      }

      {
                              //draw bottom
         const int lb_y = rc1.Bottom(), rb_right = rc1.Right()+rb1->SizeX();
         const int rb_x = rb_right-rb->SizeX(), rb_y = lb_y;
         const int overlap_lb = lb->SizeX()-lb1->SizeX(), overlap_rb = rb->SizeX()-rb1->SizeX();
         if(overlap_lb+overlap_rb <= rc1.sx){
            lb->Draw(lb_x, lb_y);
            rb->Draw(rb_x, rb_y);
                                 //bottom line
            int dx = lb->SizeX()-lb1->SizeX();
            S_rect rc_b(rc1.x+dx, lb_y, rc1.sx-(rb->SizeX()-rb1->SizeX())-dx, b->SizeY());
            S_rect rcx;
            rcx.SetIntersection(crc, rc_b);
            if(!rcx.IsEmpty()){
               SetClipRect(rcx);
               for(int x=0; x<rc_b.sx; x+=b->SizeX())
                  b->Draw(rc_b.x+x, rc_b.y);
            }
         }else{
                                 //bottom corners would overlap, draw them clipped
            int center = (lb_x+rb_right)/2;
            S_rect rc_l(lb_x, lb_y, center-lb_x, lb->SizeY());
            S_rect rcx;
            rcx.SetIntersection(crc, rc_l);
            if(!rcx.IsEmpty()){
               SetClipRect(rcx);
               lb->Draw(lb_x, lb_y);
            }
            S_rect rc_r(center, rb_y, rb_right-center, rb->SizeY());
            rcx.SetIntersection(crc, rc_r);
            if(!rcx.IsEmpty()){
               SetClipRect(rcx);
               rb->Draw(rb_x, rb_y);
            }
         }
         SetClipRect(crc);
      }

      {
                              //left
         const int lb_y = rc1.Bottom()-lb1->SizeY();
         if(int(lt1->SizeY()+lb1->SizeY()) <= rc1.sy){
            lt1->Draw(lt_x, rc1.y);
            lb1->Draw(lb_x, lb_y);
                                 //left
            S_rect rc_l(rc1.x-l->SizeX(), rc1.y+lt1->SizeY(), l->SizeX(), rc1.sy-lt1->SizeY()-lb1->SizeY());
            S_rect rcx;
            rcx.SetIntersection(crc, rc_l);
            if(!rcx.IsEmpty()){
               SetClipRect(rcx);
               //FillRect(S_rect(0,0,1000,1000), 0xffff0000);
               for(int y=0; y<rc_l.sy; y+=l->SizeY())
                  l->Draw(rc_l.x, rc_l.y+y);
            }
         }else{
            int center = rc1.y + lt1->SizeY()*rc1.sy/(lt1->SizeY()+lb1->SizeY());
            S_rect rcx;
            S_rect rc_t(lt_x, rc1.y, lt1->SizeX(), center-rc1.y);
            rcx.SetIntersection(crc, rc_t);
            if(!rcx.IsEmpty()){
               SetClipRect(rcx);
               lt1->Draw(lt_x, rc1.y);
            }
            S_rect rc_b(lb_x, center, lb1->SizeX(), lb_y+lb1->SizeY()-center);
            rcx.SetIntersection(crc, rc_b);
            if(!rcx.IsEmpty()){
               SetClipRect(rcx);
               lb1->Draw(lb_x, lb_y);
            }
         }
         SetClipRect(crc);
      }
      /**/

      {
                              //right
         const int rb_y = rc1.Bottom()-rb1->SizeY();
         if(int(rt1->SizeY()+rb1->SizeY()) <= rc1.sy){
            rt1->Draw(rc1.Right(), rc1.y);
            rb1->Draw(rc1.Right(), rb_y);
            S_rect rc_r(rc1.Right(), rc1.y+rt1->SizeY(), r->SizeX(), rc1.sy-rt1->SizeY()-rb1->SizeY());
            S_rect rcx;
            rcx.SetIntersection(crc, rc_r);
            if(!rcx.IsEmpty()){
               SetClipRect(rcx);
               for(int y=0; y<rc_r.sy; y+=r->SizeY())
                  r->Draw(rc_r.x, rc_r.y+y);
            }
         }else{
            //int center = Max(rb_y, rc1.CenterY());// rc1.y + rt1->SizeY()*rc1.sy/(rt1->SizeY()+rb1->SizeY());
            int center = rc1.y+Min(rc1.sy, (int)rt1->SizeY());
            S_rect rcx;
            S_rect rc_t(rc1.Right(), rc1.y, rc1.Right()+lt1->SizeX(), center-rc1.y);
            rcx.SetIntersection(crc, rc_t);
            if(!rcx.IsEmpty()){
               SetClipRect(rcx);
               rt1->Draw(rc1.Right(), rc1.y);
            }
            S_rect rc_b(rc1.Right(), center, rc1.Right()+rb1->SizeX(), rb_y+rb1->SizeY()-center);
            rcx.SetIntersection(crc, rc_b);
            if(!rcx.IsEmpty()){
               SetClipRect(rcx);
               rb1->Draw(rc1.Right(), rb_y);
            }
         }
      }
      SetClipRect(crc);
      AddDirtyRect(S_rect(rc1.x-l->SizeX(), rc1.y-t->SizeY(), rc1.sx+l->SizeX()+r->SizeX(), rc1.sy+t->SizeY()+b->SizeY()));
   }
}

//----------------------------

void C_application_ui::DrawDialogRect(const S_rect &rc, dword back_color, bool draw_top_bar){

   //const dword line_col_lt = MulColor(back_color, 0x18000), line_col_rb = MulColor(back_color, 0xc000);
   const dword line_col_lt = BlendColor(0xffffffff, back_color, 0x6000), line_col_rb = BlendColor(0xff000000, back_color, 0x6000);

   DrawOutline(rc, line_col_lt, line_col_rb);

   //const int top_bar_sy = fdb.line_spacing+1;
   const int top_bar_sy = GetDialogTitleHeight();
   if(draw_top_bar){
      S_rect rc1 = rc;
      rc1.sy = top_bar_sy-1;
      FillRectGradient(rc1, GetColor(COL_TITLE));

      DrawHorizontalLine(rc1.x, rc1.Bottom(), rc1.sx, 0xff606060);
   }
   {
      S_rect rc1 = rc;
      if(draw_top_bar){
         rc1.sy -= top_bar_sy;
         rc1.y += top_bar_sy;
      }
      FillRect(rc1, back_color);
   }
}

//----------------------------

bool C_application_ui::ProcessMouseInSoftButtons(S_user_input &ui, bool &redraw, bool process_title_bar){

#ifdef USE_MOUSE
   if(!HasMouse())
      return false;

   dword num_buttons = 2;
#ifndef ANDROID
   if(process_title_bar)
      num_buttons += TITLE_BUTTON_LAST;
#endif
   if(ui.mouse_buttons&(MOUSE_BUTTON_1_DOWN|MOUSE_BUTTON_1_DRAG)){
      for(dword i=0; i<num_buttons; i++){
         C_button &b = soft_buttons[i];
         bool in = ui.CheckMouseInRect(b.GetClickRect());
         if(in && (ui.mouse_buttons&MOUSE_BUTTON_1_UP)){
            b.clicked = b.down = false;
            redraw = true;
            bool is_menu = (!i);
#ifdef UI_MENU_ON_RIGHT
            if(menu_on_right)
               is_menu = (i!=0);
#endif
            ui.key = is_menu ? K_SOFT_MENU : K_SOFT_SECONDARY;
            return true;
         }
         if(!b.clicked && (ui.mouse_buttons&MOUSE_BUTTON_1_DOWN) && in){
                              //detect fast clicks
            b.clicked = true;
            MakeTouchFeedback(TOUCH_FEEDBACK_BUTTON_PRESS);
         }
         if(b.clicked && b.down != in){
            b.down = in;
            redraw = true;
         }
      }
   }else
   if(ui.mouse_buttons&MOUSE_BUTTON_1_UP){
      for(dword i=0; i<num_buttons; i++){
         C_button &b = soft_buttons[i];
         if(b.clicked){
            b.clicked = false;
            if(b.down){
               b.down = false;
               redraw = true;
               switch(i){
               case 0:
               case 1:
                  {
                     bool is_menu = (!i);
#ifdef UI_MENU_ON_RIGHT
                     if(menu_on_right)
                        is_menu = (i!=0);
#endif
                     ui.key = is_menu ? K_SOFT_MENU : K_SOFT_SECONDARY;
                  }
                  break;
#if defined USE_MOUSE && !defined ANDROID
               case 2+TITLE_BUTTON_MAXIMIZE:
                  MinMaxApplication(true);
                  break;
               case 2+TITLE_BUTTON_MINIMIZE:
                  ToggleUseClientRect();
                  break;
#endif
               }
               return true;
            }
         }
      }
   }
#endif
   return false;
}

//----------------------------

bool C_application_ui::ButtonProcessInput(C_button &but, const S_user_input &ui, bool &redraw){

#ifdef USE_MOUSE
   if(ui.mouse_buttons&(MOUSE_BUTTON_1_DOWN|MOUSE_BUTTON_1_DRAG)){
      bool in = ui.CheckMouseInRect(but.GetClickRect());
      /*
                              //detect fast clicks
      if(in && (ui.mouse_buttons&MOUSE_BUTTON_1_UP)){
         but.clicked = but.down = false;
         return i;
      }
      */
      if(!but.clicked && (ui.mouse_buttons&MOUSE_BUTTON_1_DOWN) && in){
         but.clicked = true;
         MakeTouchFeedback(TOUCH_FEEDBACK_BUTTON_PRESS);
      }
      if(but.clicked && but.down != in){
         but.down = in;
         but.down_count = GetTickTime();
         redraw = true;
      }
   }
   if(ui.mouse_buttons&MOUSE_BUTTON_1_UP){
      if(but.clicked){
         but.clicked = false;
         if(but.down){
            but.down = false;
            redraw = true;
            return true;
         }
      }
   }
#endif
   return false;
}

//----------------------------

void C_application_ui::ClearToBackground(const S_rect &rc){
   FillRect(rc, GetColor(COL_TITLE));
}

//----------------------------

void C_application_ui::ClearWorkArea(const S_rect &rc){
   FillRect(rc, GetColor(COL_AREA));
}

//----------------------------

void C_application_ui::DrawScrollbar(const C_scrollbar &sb, dword _bgnd){

   if(!sb.visible)
      return;

   dword color = GetColor(COL_SCROLLBAR);
   dword c0 = MulColor(color, 0xc000), c1 = MulColor(color, 0x14000);
   FillRect(sb.rc, color);
   DrawOutline(sb.rc, c0, c1);

   S_rect rc;
   bool vertical;
   vertical = sb.MakeThumbRect(*this, rc);
#ifdef ANDROID
   if(sb.mouse_drag_pos!=-1){
      if(vertical){
         rc.x -= rc.sx*2;
         rc.sx *= 3;
      }else{
         rc.y -= rc.sy*2;
         rc.sy *= 3;
      }
   }
#endif
   rc.Compact();
#ifdef USE_MOUSE
                              //draw currently clicked scrolled page (up or down)
   if(sb.draw_page){
      S_rect rc1 = sb.rc;
      if(sb.draw_page==sb.DRAW_PAGE_UP){
         if(vertical)
            rc1.sy = rc.y-rc1.y;
         else
            rc1.sx = rc.x-rc1.x;
      }else{
         if(vertical){
            rc1.sy += rc1.y;
            rc1.y = rc.Bottom();
            rc1.sy -= rc1.y;
         }else{
            rc1.sx += rc1.x;
            rc1.x = rc.Right();
            rc1.sx -= rc1.x;
         }
      }
      FillRect(rc1, 0x40000000);
   }
#endif
   dword col_bar = MulColor(color, 0x7000);
#ifdef USE_MOUSE
   if(sb.mouse_drag_pos!=-1)
      col_bar = MulColor(color, 0x9000);
#endif
   //c0 = 0xf0e0e0e0, c1 = MulColor(col_bar, 0x8000);
   c0 = MulColor(color, 0x18000), c1 = MulColor(col_bar, 0x8000);
   FillRect(rc, col_bar);
   DrawOutline(rc, c0, c1);
}

//----------------------------

void C_application_ui::DrawProgress(const C_progress_indicator &pi, dword use_color, bool border, dword col_bgnd){

   if(!col_bgnd)
      col_bgnd = GetColor(COL_GREY);
   FillRect(pi.rc, col_bgnd);
   if(border){
      dword c0 = MulColor(col_bgnd, 0xc000), c1 = MulColor(col_bgnd, 0x14000);
      DrawOutline(pi.rc, c0, c1);
   }
   S_rect rc = pi.rc;//(pi.rc.x+1, pi.rc.y+1, pi.rc.sx-2, pi.rc.sy-2);
   if(border)
      rc.Compact();
   int sx = 0;
   dword t = pi.total;
   if(t){
      dword p = pi.pos;
                              //don't make math overflow
      while(t>=0x800000)
         t >>= 1, p >>= 1;
      sx = Min((dword)rc.sx, (dword)rc.sx * Max(p, 0ul) / t);
   }
   rc.sx = sx;

   if(use_color){
                              //simple color
      FillRect(rc, use_color);
   }else{
                              //rainbow
      dword color = COLOR_BLACK;
      if(sx && border){
         color = 0xff204060;
         dword c0 = 0xf0e0e0e0, c1 = MulColor(color, 0x8000);
         DrawOutline(rc, c0, c1);
      }
      rc.sx = 1;
      for(int x=0; x < sx; x++){
//#ifdef MAIL
         const int sx2 = pi.rc.sx/2, sx4 = pi.rc.sx/4;
         int r, g, b;
         if(x < sx2){
            if(x<sx4){
               r = 255;
               g = Min(255, x*256/sx4);
            }else{
               r = Min(255, (sx2-x)*256/sx4);
               g = 255;
            }
            b = 0;
         }else{
            r = 0;
            int xx = x-sx2;
            if(xx<sx4){
               g = 255;
               b = Min(255, xx*256/sx4);
            }else{
               g = Min(255, (sx2-xx)*256/sx4);
               b = 255;
            }
         }
         color = 0xff000000 | (r<<16) | (g<<8) | b;
         FillRect(rc, color);
         ++rc.x;
      }
   }
}

//----------------------------
#ifdef USE_MOUSE

                              //class for drawing circle with 1-pixel outline color and alpha-filled inside
struct S_circle_draw{
   S_rgb_x pix_outline, pix_fill;
   const byte *mem;
   dword screen_pitch;
   dword sx;
   int in_rc_size;

   typedef void (S_circle_draw::*t_MakePixel)(int x, int y, const S_rgb &pix) const;
   t_MakePixel MakePixel;

#ifdef SUPPORT_12BIT_PIXEL
   void Make12bitPixel(int x, int y, const S_rgb &pix) const{
      word &p = *(((word*)(mem+screen_pitch*y) + x));
      S_rgb_x pix1; pix1.From12bit(p);
      pix1.BlendWith(pix, pix.a);
      p = pix1.To12bit();
   }
#endif
   void Make16bitPixel(int x, int y, const S_rgb &pix) const{
      word &p = *(((word*)(mem+screen_pitch*y) + x));
      S_rgb_x pix1; pix1.From16bit(p);
      pix1.BlendWith(pix, pix.a);
      p = pix1.To16bit();
   }
   void Make32bitPixel(int x, int y, const S_rgb &pix) const{
      dword &p = *(((dword*)(mem+screen_pitch*y) + x));
      S_rgb_x pix1; pix1.From32bit(p);
      pix1.BlendWith(pix, pix.a);
      p = pix1.To32bit();
   }
   inline void DrawPixel(int x, int y, const S_rgb &pix) const{
      (this->*MakePixel)(x, y, pix);
   }
   inline void DrawPixelBgFill(int x, int y, const S_rgb &pix) const{
      if(pix_fill.a)
         DrawPixel(x, y, pix_fill);
      (this->*MakePixel)(x, y, pix);
   }
   void DrawBackgroundLine(int x, int y, int x_add, int y_add, int num) const{

      if(pix_fill.a)
      while(num-->0){
         DrawPixel(x, y, pix_fill);
         x += x_add;
         y += y_add;
      }
   }

   void DrawPixelOctan(int x, int y, byte alpha) const{

      S_rgb pix_a = pix_outline, pix_a_inv = pix_outline;
      pix_a.a = byte((alpha*pix_outline.a)>>8);
      pix_a_inv.a = byte(((255-alpha)*pix_outline.a)>>8);
      int fill_sz = y-1-in_rc_size;
                  //top right
      DrawPixel(x, -y-1, pix_a);
      DrawPixelBgFill(x, -y, pix_a_inv);
      DrawBackgroundLine(x, -y+1, 0, 1, fill_sz);
                  //top left
      DrawPixel(-x-1, -y-1, pix_a);
      DrawPixelBgFill(-x-1, -y, pix_a_inv);
      DrawBackgroundLine(-x-1, -y+1, 0, 1, fill_sz);
                  //right top
      DrawPixel(y, -x-1, pix_a);
      DrawPixelBgFill(y-1, -x-1, pix_a_inv);
      DrawBackgroundLine(y-2, -x-1, -1, 0, fill_sz);
                  //left top
      DrawPixel(-y-1, -x-1, pix_a);
      DrawPixelBgFill(-y, -x-1, pix_a_inv);
      DrawBackgroundLine(-y+1, -x-1, 1, 0, fill_sz);
                  //right bottom
      DrawPixel(y, x, pix_a);
      DrawPixelBgFill(y-1, x, pix_a_inv);
      DrawBackgroundLine(y-2, x, -1, 0, fill_sz);
                  //left bottom
      DrawPixel(-y-1, x, pix_a);
      DrawPixelBgFill(-y, x, pix_a_inv);
      DrawBackgroundLine(-y+1, x, 1, 0, fill_sz);
                  //bottom right
      DrawPixel(x, y, pix_a);
      DrawPixelBgFill(x, y-1, pix_a_inv);
      DrawBackgroundLine(x, y-2, 0, -1, fill_sz);
                  //bottom left
      DrawPixel(-x-1, y, pix_a);
      DrawPixelBgFill(-x-1, y-1, pix_a_inv);
      DrawBackgroundLine(-x-1, y-2, 0, -1, fill_sz);
   }

//----------------------------

   void DrawDiagonalPixel(int x, byte alpha) const{

      S_rgb p = pix_outline;
      p.a = byte((alpha*pix_outline.a)>>8);
                     //top right
      DrawPixel(x, -x-1, p);
                     //bottom right
      DrawPixel(x, x, p);

      x = -x-1;
                     //top left
      DrawPixel(x, -x-1, p);
                     //bottom left
      DrawPixel(x, x, p);
   }

//----------------------------

   void Draw(C_application_base &app, int x, int y, dword color_outline, dword color_fill, int _radius){

      if(_radius<1){
         assert(0);
         return;
      }
      C_fixed radius = _radius;
      pix_outline.From32bit(color_outline);
      pix_fill.From32bit(color_fill);

      mem = (byte*)app.GetBackBuffer();
      screen_pitch = app.GetScreenPitch();
      const S_pixelformat &pf = app.GetPixelFormat();
      sx = app.ScrnSX();
      mem += y*screen_pitch + x*pf.bytes_per_pixel;

      switch(pf.bits_per_pixel){
#ifdef SUPPORT_12BIT_PIXEL
      case 12: MakePixel = &S_circle_draw::Make12bitPixel; break;
#endif
      case 16: MakePixel = &S_circle_draw::Make16bitPixel; break;
      case 32: MakePixel = &S_circle_draw::Make32bitPixel; break;
      }

      int radius_i = radius;
      C_fixed *y_pos = new C_fixed[radius_i];
      C_fixed r_radius = C_fixed::One()/radius;
      int i;
      for(i=0; ; i++){
         C_fixed fy;
         if(!i){
            fy = radius-C_fixed::Create(1);
         }else{
            C_fixed f = C_fixed(i)*r_radius;
            C_fixed a = C_math::Asin(f);
            fy = C_math::Cos(a) * radius;
         }
         y_pos[i] = fy;
         int y1 = fy;
         if(y1<=i){
            in_rc_size = i;
            if(i!=y1)
               --in_rc_size;
            break;
         }
      }
      i = in_rc_size+1;
      while(i--){
         C_fixed fy = y_pos[i];
         int y1 = fy;
         byte alpha = byte(fy.Frac().val>>8);
         if(y1!=i)
            DrawPixelOctan(i, y1, alpha);
         else
            DrawDiagonalPixel(i, alpha);
      }
      delete[] y_pos;

      if(pix_fill.a){
                              //fill center
         for(int y1=-in_rc_size; y1<in_rc_size; y1++){
            for(int i1=-in_rc_size; i1<in_rc_size; i1++)
               DrawPixel(i1, y1, pix_fill);
         }
      }
   }
};

//----------------------------

void C_application_ui::PrepareTouchMenuItems(C_menu *_menu){

   C_menu_imp *menu = (C_menu_imp*)_menu;
   C_menu_imp::S_animation &anim = menu->animation;
   anim.phase = anim.PHASE_VISIBLE;
   anim.DestroyTimer();
}

//----------------------------

void C_application_ui::PrepareTouchMenu(C_menu *_menu, const S_point &pos, bool make_animation){

   C_menu_imp *menu = (C_menu_imp*)_menu;
   if(!menu->touch_mode){
                              //failback, not working properly
      PrepareMenu(menu);
      return;
   }
   assert(HasMouse());

   C_menu_imp::S_animation &anim = menu->animation;

   anim.init_pos = pos;

   if(make_animation){
      anim.anim_timer = C_application_base::CreateTimer(20, (void*)1);
      anim.phase = menu->animation.PHASE_INIT_APPEAR_TIMER;
      anim.count_to = 200;
   }else{
      PrepareTouchMenuCircle(menu);
      PrepareTouchMenuItems(menu);
      RedrawScreen();
   }
}

//----------------------------

struct S_blur_matrix{
   byte *data;
   int size;
   int rc_size;

   S_blur_matrix():
      data(NULL)
   {}
   ~S_blur_matrix(){
      delete[] data;
   }
   void Init(int sz){
      sz = Max(1, sz);
      size = sz;
      rc_size = sz*2+1;
      data = new(true) byte[rc_size*rc_size];
      C_fixed r_blur_sz = C_fixed(1)/C_fixed(size);
      for(int yy=size+1; yy--; ){
         for(int xx=size+1; xx--; ){
            byte a = 0;
            C_fixed v = C_math::Sqrt(xx*xx+yy*yy);
            if(int(v)<=size){
               v *= r_blur_sz;
               v = C_fixed::One()-v;
               a = (byte)Max(0, Min(255, v.val>>8));
            }
            data[(size+yy)*rc_size + (size+xx)] = a;
            data[(size-yy)*rc_size + (size+xx)] = a;
            data[(size+yy)*rc_size + (size-xx)] = a;
            data[(size-yy)*rc_size + (size-xx)] = a;
         }
      }
   }
};

//----------------------------

void C_application_ui::PrepareTouchMenuCircle(C_menu *_menu){

   C_menu_imp *menu = (C_menu_imp*)_menu;
   C_menu_imp::S_animation &anim = menu->animation;
   int x = anim.init_pos.x;
   int y = anim.init_pos.y;
   x = Max(2, Min(ScrnSX()-2, x));
   y = Max(2, Min(ScrnSY()-2, y));

   {
      S_rect &rc = menu->rc;
      int scrn_sx = ScrnSX(), scrn_sy = ScrnSY();
      int SZ = fdb.cell_size_x*18;
      const int CIRCLE_BORDER = fdb.cell_size_x*1;
      SZ = Min(SZ, Min(scrn_sy-CIRCLE_BORDER*2, scrn_sx-CIRCLE_BORDER*2));
      SZ &= -2;
      x -= SZ/2;
      y -= SZ/2;
      x = Max(x, CIRCLE_BORDER);
      y = Max(y, CIRCLE_BORDER);
      rc = S_rect(x, y, SZ, SZ);
      if(rc.Right() > ScrnSX()-CIRCLE_BORDER)
         rc.x = ScrnSX()-CIRCLE_BORDER-SZ;
      if(rc.Bottom() > ScrnSY()-CIRCLE_BORDER)
         rc.y = ScrnSY()-CIRCLE_BORDER-SZ;
   }

   {
                              //save screen below circle
      S_rect mrc = menu->rc;
      S_rect rc1(anim.init_pos.x, anim.init_pos.y, 0, 0);
      rc1.Expand(1);
      mrc.SetUnion(mrc, rc1);
      mrc.SetIntersection(mrc, S_rect(0, 0, ScrnSX(), ScrnSY()));
      anim.img_circle_pos = S_point(mrc.x, mrc.y);
      anim.img_circle_bgnd = C_image::Create(*this);
      anim.img_circle_bgnd->Release();
      anim.img_circle_bgnd->CreateFromScreen(mrc);

      anim.rc_last_draw = S_rect(0, 0, 0, 0);
   }
                              //initialize all menu items
   S_text_style ts;
   ts.font_flags = 0;//FF_BOLD;
   ts.font_index = UI_FONT_SMALL;

   const S_font &fd = GetFontDef(ts.font_index);

   S_blur_matrix blur_matrix[2];
   blur_matrix[0].Init(fd.cell_size_x/2);
   blur_matrix[1].Init(fd.cell_size_x/3);

   const int arrow_size = (fdb.line_spacing/2) | 1;
   const S_pixelformat &pf = GetPixelFormat();

   S_rect rc_restore(0, 0, 0, 0);

   const dword MAX_ITEM_LINES = 4;
   struct S_token{
      struct S_line{
         const wchar *txt;
         dword len;
      } lines[MAX_ITEM_LINES];
      dword num_lines;
      int center_pos;
      S_rect rc;
   } tokens[5];
                              //tokenize texts
   for(int i=Min(5, menu->num_items); i--; ){
      const C_menu_imp::S_item &itm = menu->items[i];
      if(itm.IsSeparator())
         continue;

      S_token &tok = tokens[i];
      const wchar *txt = itm.text.Length() ? (const wchar*)itm.text : GetText(itm.txt_id);

      const S_blur_matrix &blur_m = blur_matrix[0];
      dword last_offs = 0;
      S_rect &rc = tok.rc;
      rc = S_rect(0, 0, 0, blur_m.size);
                              //find width of widest word
      for(int ci=0; ; ){
         wchar c = txt[ci++];
         if(!c || c==' '){
            int w = GetTextWidth(txt+last_offs, ts.font_index, ts.font_flags, ci - last_offs - 1);
            rc.sx = Max(rc.sx, w);
            last_offs = ci;
         }
         if(!c)
            break;
      }
      int sz_fit = rc.sx+1;
      rc.sx += blur_m.rc_size+1;
      tok.center_pos = rc.CenterX();

      if(itm.flags&(C_menu::HAS_SUBMENU|C_menu::MARKED))
         rc.sx += arrow_size + fd.cell_size_x;

      tok.num_lines = 0;
      for(const wchar *wp=txt; *wp && tok.num_lines<MAX_ITEM_LINES; ){
         int line_height;
         int len = ComputeFittingTextLength(wp, true, sz_fit, NULL, ts, line_height, ts.font_index) / sizeof(wchar);
         if(len && wp[len-1]==' ')
            --len;
         tok.lines[tok.num_lines].txt = wp;
         tok.lines[tok.num_lines++].len = len;
         wp += len;
         if(*wp==' ')
            ++wp;
         rc.sy += line_height;
      }
      rc.sy += blur_m.size+1;
      rc_restore.SetUnion(rc_restore, rc);
   }

   C_image *img_save = C_image::Create(*this);
   img_save->CreateFromScreen(rc_restore);

   for(int i=Min(5, menu->num_items); i--; ){
      const C_menu_imp::S_item &itm = menu->items[i];
      if(itm.IsSeparator())
         continue;

      const S_token &tok = tokens[i];
      int num_img = 2;
      if(itm.flags&C_menu::DISABLED)
         --num_img;
                              //make 2 images - normal and pressed
      for(int imi=0; imi<num_img; imi++){
         const S_blur_matrix &blur_m = blur_matrix[imi];
         S_rect rc = tok.rc;

         int center_pos = tok.center_pos;
         if(imi){
            int d = blur_matrix[0].size-blur_m.size;
            center_pos -= d;
            rc.sx -= d*2;
            rc.sy -= d*2;
         }
         dword col_text = GetColor(!imi ? COL_TEXT_MENU : COL_TEXT_HIGHLIGHTED);
         S_rgb_x ct_rgb(col_text);
         dword col_bgnd = (ct_rgb.r+ct_rgb.g+ct_rgb.b < 128*3) ? 0xffffffff : 0xff000000;
                              //fill top
         FillRect(S_rect(rc.x, 0, rc.sx, blur_m.size), col_bgnd);

         int yy = blur_m.size;
                              //now draw all words to screen below each other
         for(dword li=0; li<tok.num_lines; li++){
            const wchar *wp = tok.lines[li].txt;
            int len = tok.lines[li].len;
            FillRect(S_rect(rc.x, yy, rc.sx, fd.line_spacing), col_bgnd);
            DrawString(wp, center_pos, yy, ts.font_index, ts.font_flags | FF_CENTER, col_text, len*sizeof(wchar));
            yy += font_defs[ts.font_index].line_spacing;
         }
                              //fill bottom
         FillRect(S_rect(rc.x, yy, rc.sx, blur_m.size+1), col_bgnd);
         yy += blur_m.size+1;

         if(itm.flags&C_menu::HAS_SUBMENU)
            DrawArrowHorizontal(rc.Right()-blur_m.size-arrow_size-1, rc.CenterY()-arrow_size/2, arrow_size, col_text, true);
         else if(itm.flags&C_menu::MARKED)
            DrawCheckbox(rc.Right()-blur_m.size-arrow_size-1, rc.CenterY()-arrow_size/2, arrow_size, true, false);

                              //scan image from screen
         C_image *img = C_image::Create(*this);
         menu->touch_items[i].img_text[imi] = img;
         img->Release();

         img->CreateFromScreen(rc);
                              //prepare alpha channel
         byte *alpha = new(true) byte[rc.sx*rc.sy];
         MemSet(alpha, 0, rc.sx*rc.sy);

                              //now parse image
         const byte *img_mem = img->GetData();
         byte *alpha_mem = alpha;
         img_mem += (blur_m.size*rc.sx + blur_m.size) * pf.bytes_per_pixel;
                              //go by all pixels of drawn text
         for(int y1=rc.sy-blur_m.size*2; y1--; ){
            const byte *line = img_mem;
            byte *alpha_line = alpha_mem;
            for(int x1=rc.sx-blur_m.size*2; x1--; ){
                                 //determine if pixel is to be drawn
               S_rgb_x pix;
               switch(pf.bits_per_pixel){
               default: assert(0);
#ifdef SUPPORT_12BIT_PIXEL
               case 12: pix.From12bit(*((word*&)line)++); break;
#endif
               case 16: pix.From16bit(*((word*&)line)++); break;
               case 32: pix.From32bit(*((dword*&)line)++); break;
               }
               //int val = (pix.r+pix.g+pix.b);
               int val = (pix.r*77 + pix.g*153 + pix.b*25)>>8;
               bool opaque;
               if(col_bgnd==0xffffffff)
                  val = 255-val;
               opaque = (val>32);
               if(opaque){
                                 //add blur mask to alpha channel
                  byte *a_mem = alpha_line;
                  const byte *bm = blur_m.data;
                  for(int y2=blur_m.rc_size; y2--; ){
                     byte *a_line = a_mem;
                     for(int xx=blur_m.rc_size; xx--; ){
                        byte &a = *a_line++;
                        a = Max(a, *bm++);
                     }
                     a_mem += rc.sx;
                  }
               }
               ++alpha_line;
            }
            img_mem += rc.sx*pf.bytes_per_pixel;
            alpha_mem += rc.sx;
         }
         if(itm.flags&C_menu::DISABLED){
            for(int j=rc.sx*rc.sy; j--; )
               alpha[j] >>= 1;
         }
         img->SetAlphaChannel(alpha);
         delete[] alpha;
      }
      menu->touch_items[i].hover_offs = blur_matrix[0].size - blur_matrix[1].size;
   }
   if(img_save->SizeY()){
      img_save->Draw(0, 0);
   }else
      RedrawScreen();
   ResetClipRect();
   img_save->Release();

   if(anim.anim_timer){
                              //restart timer so time spent in this func is not counted
      anim.anim_timer->Pause();
      anim.anim_timer->Resume();
   }
   anim.phase = anim.PHASE_GROW_CIRCLE;
   anim.counter = 0;
   anim.count_to = 300;
}

//----------------------------

void C_application_ui::AnimateTouchMenu(C_mode &mod, int time){

   C_menu_imp *menu = (C_menu_imp*)mod.GetMenu();
   C_menu_imp::S_animation &anim = menu->animation;

   switch(anim.phase){
   case C_menu_imp::S_animation::PHASE_INIT_APPEAR_TIMER:
      if((anim.counter+=time) >= anim.count_to){
                              //start menu to appear
         PrepareTouchMenuCircle(menu);
      }
      break;
   case C_menu_imp::S_animation::PHASE_GROW_CIRCLE:
      anim.counter += time;
                           //just draw menu base
      if(!anim.rc_last_draw.IsEmpty()){
                           //clear previous background
         if(anim.img_circle_bgnd){
            SetClipRect(anim.rc_last_draw);
            anim.img_circle_bgnd->Draw(anim.img_circle_pos.x, anim.img_circle_pos.y);
            AddDirtyRect(anim.rc_last_draw);
            ResetClipRect();
         }else
            RedrawScreen();
      }
      C_application_ui::DrawMenuBackground(menu, false, false);
      UpdateScreen();
      if(anim.counter >= anim.count_to){
                              //switching to permanent non-animated menu
         MakeTouchFeedback(TOUCH_FEEDBACK_CREATE_TOUCH_MENU);
         mod.ResetTouchInput();
         if(mod.GetFocus()){
                              //un-focus currently focused control if menu is being displayed
            mod.GetFocus()->SetFocus(false);
         }
         PrepareTouchMenuItems(menu);
         RedrawScreen();
         UpdateScreen();
      }
      break;
   }
}

#endif//USE_MOUSE
//----------------------------

void C_application_ui::DrawParentMenu(const C_menu *_menu){

   const C_menu_imp *menu = (C_menu_imp*)_menu;
   if(menu->parent_menu)
      DrawParentMenu(menu->parent_menu);

   S_rect rc = menu->rc;
#ifdef USE_MOUSE
   if(menu->touch_mode){
      assert(!menu->IsAnimating());
      S_circle_draw draw;
      draw.Draw(*this, rc.CenterX(), rc.CenterY(), MulAlpha(GetColor(COL_TOUCH_MENU_LINE), 0x8000),
         MulAlpha(GetColor(COL_TOUCH_MENU_FILL), 0x8000), rc.sx/2);
   }else
#endif
   {
      DrawMenuBackground(menu, true, true);
      /*
      _DrawDialogRect(rc, COL_MENU);
      DrawShadow(rc, true);
      */
      //int sel = menu->selected_item;
      //const_cast<C_menu_imp*>(menu)->selected_item = -1;
      DrawMenuContents(menu, false, true);
      //const_cast<C_menu_imp*>(menu)->selected_item = sel;
                           //gray underlying menu a bit
      //rc.Expand();
      //FillRect(rc, 0x80404040);
   }
}

//----------------------------

void C_application_ui::RedrawDrawMenuComplete(const C_menu *_menu){

   const C_menu_imp *menu = (C_menu_imp*)_menu;
   //if(!menu->touch_mode)
   if(!menu->IsAnimating())
      DrawWashOut();

   if(menu->parent_menu)
      DrawParentMenu(menu->parent_menu);
                              //draw the menu
   if(menu->IsAnimating()){
                              //need to reset saved bgnd
      if(menu->animation.img_circle_bgnd)
         menu->animation.img_circle_bgnd = NULL;
   }
#if !defined WINDOWS_MOBILE
   if(!HasMouse()){
      AddDirtyRect(soft_buttons[0].GetRect());
      AddDirtyRect(soft_buttons[1].GetRect());
      DrawSoftButtons(GetText(SPECIAL_TEXT_SELECT), GetText(SPECIAL_TEXT_CANCEL));
   }
#endif
   if(menu->touch_mode)
      C_application_ui::DrawMenuBackground(menu, true, false);
   else
      DrawMenuBackground(menu, true, false);
   DrawMenuContents(menu, false);
}

//----------------------------

void C_application_ui::DrawMenuBackground(const C_menu *_menu, bool also_frame, bool dimmed){

   const C_menu_imp *menu = (C_menu_imp*)_menu;
#ifdef USE_MOUSE
   if(menu->touch_mode){
      const S_rect &rc = menu->rc;
      S_circle_draw circle_draw;
      dword col_line = GetColor(COL_TOUCH_MENU_LINE);
      dword col_bgnd = GetColor(COL_TOUCH_MENU_FILL);

      const C_menu_imp::S_animation &anim = menu->animation;
      switch(anim.phase){
      case C_menu_imp::S_animation::PHASE_GROW_CIRCLE:
         {
            int radius = rc.sx/2;
            C_fixed pos = C_fixed(anim.counter) / C_fixed(anim.count_to);
            //assert(pos<C_fixed(1));
            pos = Min(pos, C_fixed::Create(0xffff));
            radius = C_fixed(radius)*pos;
            if(radius){
               int x = anim.init_pos.x;
               int y = anim.init_pos.y;
               x += C_fixed(rc.CenterX()-x)*pos;
               y += C_fixed(rc.CenterY()-y)*pos;
               col_line = MulAlpha(col_line, pos.val);
               col_bgnd = MulAlpha(col_bgnd, pos.val);
               circle_draw.Draw(*this, x, y, col_line, col_bgnd, radius);

               S_rect rcd(x, y, 0, 0);
               rcd.Expand(radius);
               AddDirtyRect(rcd);
               anim.rc_last_draw = rcd;
            }
         }
         break;
      case C_menu_imp::S_animation::PHASE_VISIBLE:
         {
            circle_draw.Draw(*this, rc.CenterX(), rc.CenterY(), col_line, col_bgnd, rc.sx/2);
            AddDirtyRect(rc);
         }
         break;
      }
   }else
#endif
   {
      if(also_frame){
         dword col = GetColor(COL_MENU);
         const dword line_col_lt = BlendColor(0xffffffff, col, 0x6000), line_col_rb = BlendColor(COLOR_BLACK, col, 0x6000);
         DrawOutline(menu->rc, line_col_lt, line_col_rb);

         DrawShadow(menu->rc, true);
      }
      FillRect(menu->rc, GetColor(COL_MENU));
   }
}

//----------------------------

void C_application_ui::PrepareMenu(C_menu *_menu, int sel){

   bool right_align = false;
#ifdef UI_MENU_ON_RIGHT
   right_align = menu_on_right;
#endif
   C_menu_imp *menu = (C_menu_imp*)_menu;
#ifdef USE_MOUSE
   if(menu->touch_mode){
      assert(menu->parent_menu);
      S_point pos;
      bool anim = false;
      if(menu->parent_menu){
         const C_menu_imp *parent_menu = (C_menu_imp*)(C_menu*)menu->parent_menu;
         pos.x = parent_menu->rc.x + parent_menu->GetSelectedItemX();
         pos.y = parent_menu->rc.y + parent_menu->GetSelectedItemY();
      }else{
         assert(0);
         pos.x = ScrnSX()/2;
         pos.y = ScrnSY()/2;
         anim = false;
      }
      PrepareTouchMenu(menu, pos, anim);
      return;
   }
#endif
   S_rect &rc = menu->rc;
   const int BORDER_SIZE = fdb.cell_size_x;
   rc.x = BORDER_SIZE;
   rc.sx = fdb.letter_size_x * 15;
   rc.sy = C_menu_imp::VERTICAL_BORDER*2;
   bool top_align = false;
   if(soft_buttons[0].GetRect().y>=soft_buttons[1].GetRect().y){
#ifdef ANDROID
#endif
      rc.y = ScrnSY() - 3*3;
      rc.y -= (ScrnSY()-soft_buttons[0].GetRect().y) + C_menu_imp::VERTICAL_BORDER;
   }else{
      rc.y = soft_buttons[0].GetRect().Bottom() + C_menu_imp::VERTICAL_BORDER;
      top_align = true;
   }
   menu->line_height = fdb.line_spacing;
   if(HasMouse())
      menu->line_height = menu->line_height*3/2;

   if(GetScreenRotation()==ROTATION_90CW && !HasMouse())
      right_align = true;
   const int arrow_size = (fdb.line_spacing/2) | 1;
                              //compute menu's rectangle
   for(int i=0; i<menu->num_items; i++){
      const C_menu_imp::S_item &itm = menu->items[i];
      int h;
      const wchar *txt = itm.text.Length() ? (const wchar*)itm.text : GetText(itm.txt_id);
      if(!txt){
                              //separator
         h = C_menu_imp::SEPARATOR_HEIGHT;
      }else{
         int w = GetTextWidth(txt, UI_FONT_BIG, FF_BOLD);
         if(itm.flags&C_menu::MARKED)
            w += fdb.letter_size_x * 2;
         if(HasKeyboard() && (itm.shortcut.Length() || itm.shortcut_full_kb.Length())){
            const Cstr_c &s = HasFullKeyboard() ? itm.shortcut_full_kb : itm.shortcut;
            w += GetTextWidth(s, false, COD_DEFAULT, UI_FONT_BIG) + fdb.letter_size_x * 2;
         }
         if(itm.img_index!=-1 && menu->image){
            w += menu->image->SizeY();
            w += fdb.letter_size_x;// * 2;
         }
         if(itm.flags&C_menu::HAS_SUBMENU)
            w += arrow_size + 6;
         if(itm.flags&C_menu::MARKED)
            w += fdb.cell_size_y + 5 + fdb.cell_size_x/2;
         w = Min(w, ScrnSX()-rc.x*4);
         rc.sx = Max(rc.sx, w);
         h = menu->line_height;
      }
      if(!top_align)
         rc.y -= h;
      rc.sy += h;
      if(sel==i && (itm.flags&C_menu::DISABLED))
         ++sel;
   }
   if(right_align)
      rc.x = ScrnSX() - rc.sx - (BORDER_SIZE+3*2);
   if(sel==menu->num_items)
      sel = -1;
   menu->selected_item = sel;
   rc.sx += 6;

   const C_menu_imp *mp = (C_menu_imp*)(const C_menu*)menu->parent_menu;
   if(mp){
      int y = mp->rc.y + mp->GetSelectedItemY();
      if(!mp->touch_mode)
         y += mp->line_height/2 + mp->VERTICAL_BORDER;
      rc.y = y;
      if(rc.Bottom() > ScrnSY()-2){
         rc.y -= rc.Bottom()-ScrnSY()+2;
      }
      rc.y = Max(0, rc.y);
      rc.x = ScrnSX() - rc.sx - 6;
      rc.x = Min(rc.x, mp->rc.Right()-10);
   }
                              //check size, setup scrollbar
   C_scrollbar &sb = menu->sb;
   if(rc.y<2 || rc.Bottom()>(ScrnSY()-2)){
      sb.total_space = rc.sy;
      int b = rc.Bottom();
      rc.y = Max(2, rc.y);
      b = Min(ScrnSY()-2, b);
      rc.sy = b - rc.y;
      sb.visible_space = rc.sy;
      int sb_w = GetScrollbarWidth();
      rc.sx += sb_w+2;
      if(rc.Right() >= ScrnSX() || right_align)
         rc.x -= sb_w+2;
      sb.rc = S_rect(rc.Right()-sb_w-1, rc.y+1, sb_w, rc.sy-2);
      sb.visible = true;
   }else
      sb.visible = false;

   menu->MakeSureItemIsVisible();

   //if(!mp || mp->touch_mode){
      RedrawScreen();
      /*
   }else{
                              //optimized path, softkeys are drawn, just wash parent menu and draw this menu's frame
      S_rect rc = mp->rc;
      rc.Expand();
      FillRect(rc, 0x80404040);

      if(menu->touch_mode)
         C_application_ui::DrawMenuBackground(menu, true, false);
      else
         DrawMenuBackground(menu, true, false);

      //DrawShadow(menu->rc, true);
      DrawMenuContents(menu, false);
   }
   */
}

//----------------------------

static bool IsArabicChar(wchar c){
   switch(c&0xff00){
   case 0x0600:
   case 0xfb00:
   case 0xfe00:
      return true;
   }
   return false;
}

//----------------------------

void C_application_ui::DrawMenuContents(const C_menu *_menu, bool clear_bgnd, bool dimmed){

   const C_menu_imp *menu = (C_menu_imp*)_menu;
   const S_rect &rc = menu->rc;
#ifdef USE_MOUSE
   if(menu->touch_mode){
      if(!menu->IsAnimating()){
         int x = rc.CenterX(), y = rc.CenterY();
         int p45 = rc.sx/2 * 707 / 1000;
         for(int i=Min(5, menu->num_items); i--; ){
            const C_menu_imp::S_item &itm = menu->items[i];
            if(itm.IsSeparator())
               continue;
            bool hover = (menu->selected_item==i && menu->mouse_down && menu->hovered_item==i);
            const C_image *img_text0 = menu->touch_items[i].img_text[0],
               *img_text = !hover ? img_text0 : menu->touch_items[i].img_text[1];
            if(!img_text)
               continue;
            int xx = x + touch_menu_item_pos[i][0]*p45;
            int yy = y + touch_menu_item_pos[i][1]*p45;

            if(itm.img_index!=-1 && menu->image){
               bool disabled = (itm.flags&C_menu::DISABLED);
                                 //draw icon
               S_rect irc(0, 0, 0, menu->image->SizeY());
               irc.sx = irc.sy;
               irc.x = irc.sx*itm.img_index;
               if(hover)
                  ++xx, ++yy;
               S_rect rc1(xx, yy, 0, 0);
               rc1.Expand((irc.sx/2)*10/8);
               rc1.y -= rc1.sy/2;

               C_fixed alpha = C_fixed::Percent(disabled ? 50 : 100);
               FillRect(rc1, ((disabled ? 0x80 : 0xff)<<24) | 0xffffff);
               dword out_col = ((disabled ? 0x80 : 0xff)<<24) | 0x000000;
               DrawOutline(rc1, out_col, out_col);
               if(!hover && !disabled)
                  DrawShadow(rc1, true);
               menu->image->DrawSpecial(xx-irc.sy/2, rc1.CenterY()-irc.sy/2, &irc, alpha);
               yy = rc1.Bottom() + fdb.cell_size_y/4;
            }else{
               yy -= img_text0->SizeY()/2;
               if(hover)
                  ++xx, ++yy;
            }
            if(hover){
               int delta = menu->touch_items[i].hover_offs;
               xx += delta;
               yy += delta;
            }
            img_text->Draw(xx-img_text0->SizeX()/2, yy);
         }
      }
   }else
#endif
   {
                              //clear bgnd
      const dword col_text = GetColor(menu->parent_menu ? COL_TEXT_MENU_SECONDARY : COL_TEXT_MENU);
      if(clear_bgnd){
         if(menu->touch_mode)
            C_application_ui::DrawMenuBackground(menu, false, false);
         else
            DrawMenuBackground(menu, false, false);
      }

                              //draw all menu items
      int x = rc.x + 3;
      int y = rc.y + C_menu_imp::VERTICAL_BORDER;

      const int arrow_size = (fdb.line_spacing/2) | 1;

      int draw_width = rc.sx;

      if(menu->sb.visible){
         draw_width -= menu->sb.rc.sx+2;
         y -= menu->sb.pos;
         if(!dimmed)
            SetClipRect(rc);
      }
      for(int i=0; i<menu->num_items; i++){
         const C_menu_imp::S_item &itm = menu->items[i];
         const wchar *txt = itm.text.Length() ? (const wchar*)itm.text : GetText(itm.txt_id);
         if(!txt){
                              //separator
            DrawThickSeparator(rc.x+4, draw_width-8, y+1);
            y += C_menu_imp::SEPARATOR_HEIGHT;
         }else{
            int txt_y = y+Max(1, (menu->line_height-fdb.line_spacing)/2);
            dword color = col_text;
            if(menu->selected_item==i && !dimmed && (!ui_touch_mode || menu->mouse_down)){

               DrawMenuSelection(menu, S_rect(rc.x, y-1, draw_width, menu->line_height+2));
               color = GetColor(COL_TEXT_HIGHLIGHTED);
            }
            if(itm.flags&C_menu::DISABLED){
#ifdef __SYMBIAN32__
               if(system_font.use)
                  color = GetColor(COL_GREY);
               else
#endif
                  color = MulAlpha(color, 0x8000);
            }
            if(dimmed){
#ifdef __SYMBIAN32__
               if(system_font.use)
                  color = GetColor(COL_GREY);
               else
#endif
                  color = MulAlpha(color, 0x8000);
            }

            int max_w = draw_width-5;
            int txt_x = x;
            {
               if(HasKeyboard() && (itm.shortcut.Length() || itm.shortcut_full_kb.Length())){
                  const Cstr_c &s = HasFullKeyboard() ? itm.shortcut_full_kb : itm.shortcut;
                  int xx = x+draw_width-6;
                  if(itm.flags&C_menu::HAS_SUBMENU)
                     xx -= arrow_size + 2;
                  int w = DrawStringSingle(s, xx, txt_y, UI_FONT_BIG, FF_RIGHT, MulAlpha(color, 0x8000));
                  max_w -= w + 6;
               }//else
               if(itm.img_index!=-1 && menu->image){
                           //draw image
                  int xx = x;//+draw_width-fdb.letter_size_x;
                  S_rect irc(0, 0, 0, menu->image->SizeY());
                  irc.sx = irc.sy;
                  irc.x = irc.sx*itm.img_index;
                  //xx -= irc.sx;
                  txt_x += irc.sx + fdb.letter_size_x/2;
                  C_fixed alpha = C_fixed::Percent((itm.flags&C_menu::DISABLED) ? 50 : 100);
                  if(dimmed)
                     alpha *= C_fixed::Half();
                  menu->image->DrawSpecial(xx, y+(menu->line_height-irc.sy)/2, &irc, alpha);
               }
            }
            if(itm.flags&C_menu::HAS_SUBMENU){
                                 //draw sub-menu mark (arrow at right corner)
               int xx = x+draw_width - arrow_size - 6, yy = y+(menu->line_height-arrow_size)/2;

               dword col_arrow = color & 0xffffff;
               col_arrow |= ((itm.flags&C_menu::DISABLED) ? 0x80 : 0xff)<<24;
               if(dimmed)
                  col_arrow = MulAlpha(col_arrow, 0x8000);
               DrawArrowHorizontal(xx, yy, arrow_size, col_arrow, true);
               max_w -= arrow_size + 6;
            }else
            if(itm.flags&C_menu::MARKED){
               max_w -= fdb.cell_size_y + 5 + fdb.cell_size_x/2;
            }
            int tx;
            if(IsArabicChar(txt[0])){
               tx = DrawString(txt, x+max_w, txt_y, UI_FONT_BIG, FF_BOLD | FF_RIGHT, color, -max_w);
            }else
               tx = DrawString(txt, txt_x, txt_y, UI_FONT_BIG, FF_BOLD, color, -max_w);
            if(itm.flags&C_menu::MARKED)
               DrawCheckbox(txt_x+tx+5, y+menu->line_height/4, fdb.cell_size_y, true, false, dimmed ? 0x80 : 0xff);
            y += menu->line_height;
         }
      }
      if(menu->sb.visible && !dimmed){
         DrawScrollbar(menu->sb);
         ResetClipRect();
      }
      AddDirtyRect(rc);
   }
}

//----------------------------

void C_application_ui::FocusChange(bool foreground){

   C_application_base::FocusChange(foreground);

                              //remove mode's menu
   if(mode && mode->menu){
      mode->menu = NULL;
      mode->Draw();
      UpdateScreen();
   }
                              //manage mode's timer
   if(mode && mode->timer && !mode->WantBackgroundTimer()){
      if(foreground)
         mode->timer->Resume();
      else
         mode->timer->Pause();
   }
   for(C_mode *mp = mode; mp; ){
      C_mode *mp1 = mp;
      mp = mp1->saved_parent;
      mp1->AddRef();
      mp1->FocusChange(foreground);
      mp1->Release();
   }
}

//----------------------------

int C_application_ui::MenuProcessInput(C_mode &mod, S_user_input &ui, bool &redraw){

   C_menu_imp *menu = (C_menu_imp*)mod.GetMenu();
   const S_rect &rc = menu->rc;

#ifdef USE_MOUSE
   if(menu->touch_mode){
      C_menu_imp::S_animation &anim = menu->animation;
      switch(anim.phase){
      case C_menu_imp::S_animation::PHASE_INIT_APPEAR_TIMER:
      case C_menu_imp::S_animation::PHASE_GROW_CIRCLE:
         {
            S_point init_pos = anim.init_pos;
            bool close = false;
            if(ui.mouse_buttons&MOUSE_BUTTON_1_UP)
               close = true;
            else if(ui.mouse_buttons&MOUSE_BUTTON_1_DRAG){
                              //check if we're not too far from original position
               int dx = ui.mouse.x-init_pos.x;
               int dy = ui.mouse.y-init_pos.y;
               int d2 = dx*dx+dy*dy;
               const int MAX_MOVE_DIST = fdb.cell_size_x*2;
               if(d2>=MAX_MOVE_DIST*MAX_MOVE_DIST){
                  close = true;
               }
            }
            if(close){
               if(anim.phase==anim.PHASE_GROW_CIRCLE){
                  if(anim.img_circle_bgnd){
                     anim.img_circle_bgnd->Draw(anim.img_circle_pos.x, anim.img_circle_pos.y);
                     AddDirtyRect(S_rect(anim.img_circle_pos.x, anim.img_circle_pos.y, anim.img_circle_bgnd->SizeX(), anim.img_circle_bgnd->SizeY()));
                  }else
                     redraw = true;
                  //UpdateScreen();
               }
               menu->animation.DestroyTimer();  //do now, some modal dialogs may be run in ProcessInput below, if there's anim timer, it would tick
               mod.SetMenu(menu->parent_menu);
                              //send mouse drag and up to mode
               ui.mouse_rel.x = ui.mouse.x - init_pos.x;
               ui.mouse_rel.y = ui.mouse.y - init_pos.y;
               if(ui.mouse_rel.x || ui.mouse_rel.y)
                  ui.mouse_buttons |= MOUSE_BUTTON_1_DRAG;
               ProcessInput(ui);
                              //this menu may be destroyed now
               return -1;
            }
         }
         break;

      case C_menu_imp::S_animation::PHASE_VISIBLE:
         if(ui.mouse_buttons&(MOUSE_BUTTON_1_DOWN|MOUSE_BUTTON_1_DRAG|MOUSE_BUTTON_1_UP)){
                                 //detect area (circle); 0-4 for buttons, -2 for menu circle
            int x = rc.CenterX(), y = rc.CenterY();
            const int p45 = rc.sx/2 * 707 / 1000;
            const int SMALL_CIRCLE_R = rc.sx/6;
            int sel = -1;
            for(int i=menu->num_items; i--; ){
               const C_menu_imp::S_item &itm = menu->items[i];
               if(itm.IsSeparator())
                  continue;
               if(itm.flags&C_menu::DISABLED)
                  continue;
               int xx = x + touch_menu_item_pos[i][0]*p45;
               int yy = y + touch_menu_item_pos[i][1]*p45;
               xx -= ui.mouse.x;
               yy -= ui.mouse.y;
               int d = xx*xx+yy*yy;
               if(d<SMALL_CIRCLE_R*SMALL_CIRCLE_R){
                  sel = i;
                  break;
               }
            }
            if(sel==-1){
               x -= ui.mouse.x;
               y -= ui.mouse.y;
               int d = x*x+y*y;
               int r = rc.sx/2+fdb.cell_size_x;
               if(d < r*r)
                  sel = -2;
            }
            if(sel>=0){
               const C_menu_imp::S_item &itm = menu->items[sel];
               if(!itm.IsSeparator()){
               //if(!(itm.flags&C_menu::DISABLED)){
                  if(ui.mouse_buttons&MOUSE_BUTTON_1_DOWN){
                     if(!menu->mouse_down)
                        MakeTouchFeedback(TOUCH_FEEDBACK_SELECT_MENU_ITEM);
                     menu->mouse_down = true;
                     menu->selected_item = sel;
                     redraw = true;
                  }
                  if((ui.mouse_buttons&MOUSE_BUTTON_1_UP) && menu->selected_item==sel && menu->mouse_down){
                     menu->mouse_down = false;
                     menu->hovered_item = -1;
                     int ret = itm.txt_id;
                     if(ret==0){
                        assert(itm.text.Length());
                        ret = 0x10000 | sel;
                     }
                     if(!(menu->items[sel].flags&C_menu::HAS_SUBMENU)){
                        mod.SetMenu(NULL);
                        redraw = true;
                     }
                     return ret;
                  }
               }
            }else{
               if(sel==-1){
                  if(ui.mouse_buttons&MOUSE_BUTTON_1_DOWN)
                     ui.key = K_ESC;
               }else{
                  /*
                  if(menu->mouse_down){
                     menu->mouse_down = false;
                     redraw = true;
                  }
                  */
               }
               sel = -1;
            }
            if(ui.mouse_buttons&(MOUSE_BUTTON_1_DRAG|MOUSE_BUTTON_1_DOWN)){
               if(menu->hovered_item != sel){
                  menu->hovered_item = sel;
                  redraw = true;
               }
            }
            if(ui.mouse_buttons&MOUSE_BUTTON_1_UP)
               menu->mouse_down = false;
         }
         break;
      }

      switch(ui.key){
#if !(defined S60)
      case K_SOFT_MENU:
#endif
      case K_SOFT_SECONDARY:
      case K_CURSORLEFT:
      case K_ESC:
      case K_BACK:
      case K_MENU:
         mod.SetMenu(menu->parent_menu);
         redraw = true;
         break;
      }
   }else
#endif
   {
      if(HasMouse()){
         if(ui.mouse_buttons&(MOUSE_BUTTON_1_DOWN|MOUSE_BUTTON_1_DRAG|MOUSE_BUTTON_1_UP)){
#ifdef USE_MOUSE
            C_scrollbar::E_PROCESS_MOUSE pm = ProcessScrollbarMouse(menu->sb, ui);
            if(pm){
               redraw = true;
            }else
#endif
            if(ui.mouse_buttons&MOUSE_BUTTON_1_DOWN){
               if(ui.CheckMouseInRect(rc)){
                  ui_touch_mode = true;
                              //check which menu item it may be
                  int y = rc.y + C_menu_imp::VERTICAL_BORDER - menu->sb.pos;
                  for(int i=0; i<menu->num_items; i++){
                     const C_menu_imp::S_item &itm = menu->items[i];
                     const wchar *txt = itm.text.Length() ? (const wchar*)itm.text : GetText(itm.txt_id);
                     if(!txt){
                        y += C_menu_imp::SEPARATOR_HEIGHT;
                     }else{
                        int ny = y + menu->line_height;
                        if(ui.mouse.y >= y && ui.mouse.y < ny){
                           if(!(itm.flags&C_menu::DISABLED)){
                              if(ui.mouse_buttons&MOUSE_BUTTON_1_DOWN){
                                 //if(menu->selected_item!=i)
                                    MakeTouchFeedback(TOUCH_FEEDBACK_SELECT_MENU_ITEM);
                                 menu->mouse_down = true;
                              }
                              menu->selected_item = i;
                              menu->MakeSureItemIsVisible();

                              if((ui.mouse_buttons&MOUSE_BUTTON_1_UP) && menu->mouse_down){
                                 ui.key = K_ENTER;
                                 menu->mouse_down = false;
                              }else
                                 DrawMenuContents(menu, true);
                           }
                           menu->menu_drag_begin_y = ui.mouse.y;
                           menu->menu_drag_begin_pos = menu->sb.pos;
                           break;
                        }
                        y = ny;
                     }
                  }
               }else{
                              //clicked outside, discard menu
                  ui.key = K_ESC;
               }
            }
            if((ui.mouse_buttons&MOUSE_BUTTON_1_DRAG) && menu->menu_drag_begin_pos!=-1){
               int dy = menu->menu_drag_begin_y - ui.mouse.y;
               if(Abs(dy) > menu->line_height/2){
                  menu->mouse_down = false;
               }
               if(menu->sb.visible && !menu->mouse_down){
                              //scroll menu
                  int np = menu->menu_drag_begin_pos + dy;
                  const int scroll_h = menu->sb.total_space-menu->rc.sy;
                  if(np<0)
                     np /= 2;
                  else
                  if(np>scroll_h)
                     np = scroll_h + (np-scroll_h)/2;
                  menu->sb.pos = np;
                  DrawMenuContents(menu, true);
               }
            }
            if(ui.mouse_buttons&MOUSE_BUTTON_1_UP){
               if(menu->mouse_down && ui.CheckMouseInRect(rc)){
                  ui.key = K_ENTER;
               }
               menu->mouse_down = false;
               if(menu->menu_drag_begin_pos != -1){
                              //reset scroll back
                  int np = Max(0, Min(menu->sb.total_space-menu->rc.sy, menu->sb.pos));
                  if(menu->sb.pos!=np){
                     menu->sb.pos = np;
                  }
                  DrawMenuContents(menu, true);
                  menu->menu_drag_begin_pos = -1;
               }
            }
         }
      }
      int &sel = menu->selected_item;
      switch(ui.key){
      case K_CURSORUP:
         ui_touch_mode = false;
         if(sel!=-1)
         do{
            if(!sel)
               sel = menu->num_items;
            --sel;
         }while(menu->items[sel].flags&C_menu::DISABLED);
         menu->MakeSureItemIsVisible();
         DrawMenuContents(menu, true);
         break;

      case K_CURSORDOWN:
         ui_touch_mode = false;
         if(sel!=-1)
         do{
            if(++sel == menu->num_items)
               sel = 0;
         }while(menu->items[sel].flags&C_menu::DISABLED);
         menu->MakeSureItemIsVisible();
         DrawMenuContents(menu, true);
         break;

      case K_CURSORRIGHT:
                              //only sub-menus items react on right key
         if(sel!=-1 && (menu->items[sel].flags&C_menu::HAS_SUBMENU)){
            const C_menu_imp::S_item &itm = menu->items[sel];
            int ret = itm.txt_id;
            if(ret==0){
               assert(itm.text.Length());
               ret = 0x10000 | sel;
            }
            if(!ui_touch_mode)
               return ret;
            ui_touch_mode = false;
            DrawMenuContents(menu, true);
         }
         break;

      case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
         if(sel==-1)
            break;
         {
            int num = ui.key-'1';
            int ns = 0;
            while((menu->items[ns].txt_id==0 && !menu->items[ns].text.Length()) || num--){
               if(++ns==menu->num_items)
                  break;
            }
            if(ns==menu->num_items || (menu->items[ns].flags&C_menu::DISABLED))
               break;
            ui_touch_mode = false;
            menu->selected_item = ns;
            DrawMenuContents(menu, true);
         }
                                 //flow...
      case K_ENTER:
#if defined S60
      case K_SOFT_MENU:
#endif
         /*
         if(ui_touch_mode){
            ui_touch_mode = false;
            DrawMenuContents(menu, true);
            break;
         }
         */
         if(sel!=-1){
            const C_menu_imp::S_item &itm = menu->items[sel];
            int ret = itm.txt_id;
            if(ret==0){
               assert(itm.text.Length());
               ret = 0x10000 | sel;
            }
            if(!(menu->items[sel].flags&C_menu::HAS_SUBMENU)){
               mod.SetMenu(NULL);
               redraw = true;
            }
            return ret;
         }
         break;

#if !(defined S60)
      case K_SOFT_MENU:
#endif
      case K_SOFT_SECONDARY:
      case K_CURSORLEFT:
      case K_ESC:
      case K_BACK:
      case K_MENU:
         mod.SetMenu(menu->parent_menu);
         redraw = true;
         break;
      }
   }
   return -1;
}

//----------------------------

C_scrollbar::E_PROCESS_MOUSE C_application_ui::ProcessScrollbarMouse(C_scrollbar &sb, const S_user_input &ui) const{

#if defined USE_MOUSE && !defined ANDROID_
   if(!sb.visible)
      return sb.PM_NO;
   const S_rect &rc = sb.rc;

   bool horiz = (rc.sx > rc.sy);
   S_rect rc1;
   if(sb.mouse_drag_pos!=-1){
      if(ui.mouse_buttons&MOUSE_BUTTON_1_UP){
         sb.mouse_drag_pos = -1;
         sb.button_down = false;
         sb.draw_page = sb.DRAW_PAGE_NO;
      }else{
         sb.MakeThumbRect(*this, rc1);
         const int pt = horiz ? ui.mouse.x : ui.mouse.y;
         const int scroll_sz = (horiz ? rc.sx : rc.sy);

         int delta = longlong(pt - sb.mouse_drag_pos) * longlong(sb.total_space) / longlong(scroll_sz);
         int new_pos = Max(0, Min(sb.total_space-sb.visible_space, sb.drag_init_pos+delta));
         if(sb.pos!=new_pos){
            sb.pos = new_pos;
            return sb.PM_CHANGED;
         }
      }
      return sb.PM_PROCESSED;
   }
   if((ui.mouse_buttons&(MOUSE_BUTTON_1_DOWN | MOUSE_BUTTON_1_UP)) || sb.button_down){
      const int BORDER_X = !HasMouse() ? 3 :
#ifdef ANDROID
         (!horiz ? rc.sx : rc.sy)*2;
#else
         (!horiz ? rc.sx : rc.sy)/2;
#endif
      if(!horiz){
         rc1 = S_rect(rc.x-BORDER_X, rc.y+1, rc.sx+BORDER_X*2, rc.sy-2);
      }else{
         rc1 = S_rect(rc.x+1, rc.y-BORDER_X, rc.sx-2, rc.sy+BORDER_X*2);
      }
      if(ui.CheckMouseInRect(rc1)){
         bool clicked = false;
         if(ui.mouse_buttons&MOUSE_BUTTON_1_DOWN){
            sb.repeat_countdown = 350;
            sb.button_down = true;
            sb.last_button_down_time = GetTickTime();
            clicked = true;
            const_cast<C_application_ui*>(this)->MakeTouchFeedback(TOUCH_FEEDBACK_SCROLL);
         }/*else
         if(sb.button_down){
            dword t = GetTickTime();
            dword time = t - sb.last_button_down_time;
            sb.last_button_down_time = t;
            if((sb.repeat_countdown-=time) <= 0){
               sb.repeat_countdown = 75;
               clicked = true;
            }
         }
         */
         if(ui.mouse_buttons&MOUSE_BUTTON_1_UP){
            if(!sb.button_down && sb.mouse_drag_pos==-1 && sb.draw_page==sb.DRAW_PAGE_NO)
               return sb.PM_NO;
            sb.button_down = false;
            sb.mouse_drag_pos = -1;
            sb.draw_page = sb.DRAW_PAGE_NO;
         }
         if(clicked){
                              //convert mouse to scrollbar units
            sb.MakeThumbRect(*this, rc1);
            const int lo = horiz ? rc1.x : rc1.y,
               hi = horiz ? rc1.Right() : rc1.Bottom(),
               pt = horiz ? ui.mouse.x : ui.mouse.y;

            if(pt<lo){
                              //page up
               sb.pos = Max(0, sb.pos-sb.visible_space);
               if(sb.button_down)
                  sb.draw_page = sb.DRAW_PAGE_UP;
               return sb.PM_CHANGED;
            }
            if(pt >= hi){
                              //page down
               sb.pos = Min(sb.total_space-sb.visible_space, sb.pos+sb.visible_space);
               if(sb.button_down)
                  sb.draw_page = sb.DRAW_PAGE_DOWN;
               return sb.PM_CHANGED;
            }
                              //begin dragging
            sb.mouse_drag_pos = pt;
            sb.drag_init_pos = sb.pos;
            sb.draw_page = sb.DRAW_PAGE_NO;
         }
         return sb.PM_PROCESSED;
      }
   }
#endif   //USE_MOUSE
   return sb.PM_NO;
}

//----------------------------

bool C_application_ui::IsEditedTextFrameTransparent() const{
#ifdef _DEBUG
   return true;
#else
   return false;
#endif
}

//----------------------------

void C_application_ui::DrawEditedTextFrame(const S_rect &rc){

   DrawOutline(rc, COLOR_BLACK, COLOR_BLACK);
   dword col = GetColor(COL_WHITE);
#ifdef _DEBUG
   if(IsEditedTextFrameTransparent())
      col = MulAlpha(col, 0x8000);
#endif
   FillRect(rc, col);
}

//----------------------------

void C_application_ui::DrawSelection(const S_rect &rc, bool _clear_to_bgnd){

   S_rect rc1 = rc;
   rc1.Compact();
   dword bgnd = GetColor(COL_SELECTION);
   DrawOutline(rc1,
      BlendColor(0xffffffff, bgnd, 0x6000),
      BlendColor(0xff000000, bgnd, 0x6000));
   FillRectGradient(rc1, bgnd);
}

//----------------------------

void C_application_ui::DrawSeparator(int l, int sx, int y){

   FillRect(S_rect(l, y, sx, 1), MulAlpha(GetColor(COL_TEXT), 0x2000));
}

//----------------------------

void C_application_ui::DrawThickSeparator(int l, int sx, int y){

   const dword c0 = 0x40000000, c1 = 0x80ffffff;
   FillRect(S_rect(l, y, sx, 1), c0);
   FillRect(S_rect(l, y+1, 1, 1), c0);
   FillRect(S_rect(l+1, y+1, sx-1, 1), c1);
   FillRect(S_rect(l+sx-1, y, 1, 1), c1);
}

//----------------------------

void C_application_ui::DrawPreviewWindow(const S_rect &rc){

   S_rgb_x col_text(GetColor(COL_TEXT_POPUP));
   FillRect(rc, GetColor((col_text.r+col_text.g+col_text.b)<128*3 ? COL_WHITE : COL_BACKGROUND));
   DrawEtchedFrame(rc);
}

//----------------------------

void C_application_ui::DrawDialogBase(const S_rect &rc, bool draw_top_bar, const S_rect *rc_clip){

   if(rc_clip)
      SetClipRect(*rc_clip);
   DrawDialogRect(rc, COL_AREA, draw_top_bar);
   if(!rc_clip)
      DrawShadow(rc, true);
   if(rc_clip)
      ResetClipRect();
}

//----------------------------

bool C_application_ui::DrawSoftButtonRectangle(const C_button &but){

   if(HasMouse()){
      dword c0 = 0xc0ffffff, c1 = 0x80000000;
      if(but.down)
         Swap(c0, c1);
      DrawOutline(but.GetRect(), c0, c1);
      ClearToBackground(but.GetRect());
      return true;
   }else
     return false;
}

//----------------------------

dword C_application_ui::GetScrollbarWidth() const{

#ifndef ANDROID
   if(HasMouse())
      return fdb.cell_size_x*3/2;
#endif
   return fdb.cell_size_x/2;
}

//----------------------------

dword C_application_ui::GetProgressbarHeight() const{
   return fdb.line_spacing;
}

//----------------------------

void C_application_ui::DrawButton(const C_button &but, bool enabled){

   dword c0 = 0xffffffff, c1 = 0x80000000;
   if(!enabled)
      c0 = 0x40ffffff, c1 = 0x40000000;
   const S_rect &rc = but.GetRect();
   int x = rc.x, y = rc.y;
   if(but.down){
      Swap(c0, c1);
      ++x, ++y;
   }
   DrawOutline(rc, c0, c1);
   if(enabled){
      ClearToBackground(rc);
      FillRect(rc,
         //0x40000000);
         MulAlpha(GetColor(COL_TEXT), 0x4000));
   }
}

//----------------------------

void C_application_ui::DrawCheckbox(int x, int y, int size, bool enabled, bool frame, byte alpha){

   if(frame){
      S_rect rc(x+1, y+1, size, size);
      dword col = (alpha<<24) | (GetColor(COL_DARK_GREY)&0xffffff);
      DrawOutline(rc, col);
      --rc.x, --rc.y;
      //col = GetColor(COL_SCROLLBAR);
      col = GetColor(COL_LIGHT_GREY);
      DrawOutline(rc, col);
   }
   if(enabled){
      dword color = (alpha<<24) | 0xff0000;
      x += size/4;
      int line_w = (size+2)/3;
      int h = size*3/4;
      y += size*3/4;

      int hl = (h+1)/2;
      for(int i=0; i<h; i++){
         int xx = x+i;
         int yy = y-i;
         int w = line_w;
         if(i>h-w/2)
            w -= w/2-(h-i);
         DrawHorizontalLine(xx, yy, w, color);
         if(i<hl){
            xx = x-i;
            int w1 = line_w;
            if(i>hl-w1/2){
               int c = w1/2-(hl-i);
               w1 -= c;
               xx += c;
            }
            DrawHorizontalLine(xx, yy, w1, color);
         }
      }
      for(int i=1; i<h/4; i++){
         int w = line_w - i*2;
         int xx = x+i;
         int yy = y+i;
         DrawHorizontalLine(xx, yy, w, color);
      }
   }
}

//----------------------------

void C_application_ui::DrawWashOut(){

   const dword sx = ScrnSX(), sy = ScrnSY();
   const dword color = GetColor(COL_WASHOUT);

   if(HasMouse()){
      FillRect(S_rect(0, 0, sx, sy), color, false);
   }else{
      const dword col1 = MulAlpha(color, 0xaaaa), col2 = MulAlpha(color, 0x5555);
      const int bot = sy-(GetSoftButtonBarHeight()-1);
      if(GetScreenRotation()==ROTATION_90CW){
         const int top = GetTitleBarHeight()+1;
         FillRect(S_rect(0, top, sx, bot-top), color, false);
         int max_x = soft_buttons[0].GetRect().x-2;

         FillRect(S_rect(0, 0, max_x, top), color, false);
         FillRect(S_rect(0, bot, max_x, sy-bot), color, false);

         FillRect(S_rect(max_x, top-1, sx-max_x, 1), col1, false);
         FillRect(S_rect(max_x, top-2, sx-max_x, 1), col2, false);

         FillRect(S_rect(max_x, bot, sx-max_x, 1), col1, false);
         FillRect(S_rect(max_x, bot+1, sx-max_x, 1), col2, false);

         FillRect(S_rect(max_x, 0, 1, top), col1, false);
         FillRect(S_rect(max_x+1, 0, 1, top), col2, false);

         FillRect(S_rect(max_x, bot, 1, sy-bot), col1, false);
         FillRect(S_rect(max_x+1, bot, 1, sy-bot), col2, false);
      }else{
         FillRect(S_rect(0, 0, sx, bot), color, false);
         FillRect(S_rect(0, bot, sx, 1), col1, false);
         FillRect(S_rect(0, bot+1, sx, 1), col2, false);
      }
   }
}

//----------------------------

static bool IsRtlChar(wchar c){
   return ((c&0xff00)==0x600);
}
/*
static bool IsArabicChar(wchar c){
   switch(c&0xff00){
   case 0x0600:
   case 0xfb00:
   case 0xfe00:
      return true;
   }
   return false;
}
*/
//----------------------------

static bool IsArabicCharOnlyFinal(wchar c){
   switch(c){
   case 0x622: //alef
   case 0x623:
   case 0x624:
   case 0x627: //alif
   case 0x62f: //dal
   case 0x630: //zal
   case 0x631: //re
   case 0x632: //ze
   case 0x648: //vav
   case 0x698: //zhe
      return true;
   }
   return false;
}

static bool IsArabicPunctation(wchar c){
   switch(c){
   case 0x621:
   //case 0x623:
   case 0x625:
   case 0x64b:
   case 0x651:
   case 0x6c0:
      return true;
   }
   return false;
}

//----------------------------

static void ConvertRtlLine(wchar *line){

   int len = StrLen(line);
   wchar *tmp = NULL, _tmp1[64];
   wchar *buf = _tmp1;
   if(len>64){
      buf = tmp = new(true) wchar[len];
   }

   int di = len;
   int si = 0;
   while(si<len){
      wchar c = line[si];
      switch(c){
      case '<': line[si] = '>'; break;
      case '>': line[si] = '<'; break;
      }
      if(IsRtlChar(c)){
         ++si;
         buf[--di] = c;
      }else{
                              //detect non-rtl line len
         int se;
         for(se=si+1; se<len && !IsRtlChar(line[se]); se++);
         int l = se-si;
         MemCpy(buf+di-l, line+si, l*2);
         si += l;
         di -= l;
      }
   }
   assert(!di);
   di = 0;
                              //detect various forms of Arabic letters
   for(int i=0; i<len; i++){
      wchar c = buf[i];
      enum{
         ISOLATED = 0,
         INITIAL = 1,
         FINAL = 2,
         MEDIAL = 3,
      };
      byte form = ISOLATED;
      if(IsRtlChar(c)){
         if(i){
            wchar cl = buf[i-1];
            if(IsRtlChar(cl) && !IsArabicPunctation(cl)
               )
               form |= FINAL;
         }
      retest:
         if(i<len-1){
            wchar cr = buf[i+1];
            if(IsRtlChar(cr)){
               if(!IsArabicCharOnlyFinal(cr) && !IsArabicPunctation(cr))
                  form |= INITIAL;
               bool spc = false;
               switch((cr<<16)|c){
               case 0x6440622: c = 0xfef5; spc = true; break;
               case 0x6440623: c = 0xfef7; spc = true; break;
               case 0x6440625: c = 0xfef9; spc = true; break;
               case 0x6440627: c = 0xfefb; spc = true; break;
               case 0x627064b: c = 0xfe83; spc = true; break;
               //case 0x6300651: c = 0xfefb; break;
               }
               if(spc){
                  form &= FINAL;
                  ++i;     //ignore next char
                  goto retest;
               }
            }
         }
         if(form){
                              //transform to proper unicode variant
                              //http://en.wikipedia.org/wiki/Arabic_%28Unicode_block%29
                              //http://en.wikipedia.org/wiki/Perso-Arabic_script
            wchar base = 0, fin_med = 0;
            const wchar *map = NULL;
            switch(c){
            case 0x60c: break;
            case 0x61f: break;
            case 0x621: break;
            case 0x622: fin_med = 0xfe82; break;   //alef
            case 0x623: fin_med = 0xfe84; break;
            case 0x624: break;
            case 0x625: fin_med = 0xfe88; break;
            case 0x626: base = 0xfe89; break;   //?
            case 0x627: fin_med = 0xfe8e; break;
            case 0x628: base = 0xfe8f; break;   //be
            case 0x629: fin_med = 0xfe94; break;
            case 0x62a: base = 0xfe95; break;   //pe
            case 0x62b: base = 0xfe99; break;   //se
            case 0x62c: base = 0xfe9d; break;   //jim
            case 0x62d: base = 0xfea1; break;   //he
            case 0x62e: base = 0xfea5; break;   //khe
            case 0x62f: fin_med = 0xfeaa; break;   //dal
            case 0x630: fin_med = 0xfeac; break;   //zal
            case 0x631: fin_med = 0xfeae; break;   //re
            case 0x632: fin_med = 0xfeb0; break;   //ze
            case 0x633: base = 0xfeb1; break;   //sin
            case 0x634: base = 0xfeb5; break;   //shin
            case 0x635: base = 0xfeb9; break;   //sad
            case 0x636: base = 0xfebd; break;   //zad
            case 0x637: base = 0xfec1; break;   //ta
            case 0x638: base = 0xfec5; break;   //za
            case 0x639: base = 0xfec9; break;   //eyn
            case 0x63a: base = 0xfecd; break;   //gheyn
            case 0x640: break;
            case 0x641: base = 0xfed1; break;   //fe
            case 0x642: base = 0xfed5; break;   //qaf
            case 0x643: base = 0xfed9; break;
            case 0x644: base = 0xfedd; break;   //lam
            case 0x645: base = 0xfee1; break;   //mim
            case 0x646: base = 0xfee5; break;   //nun
            case 0x647: base = 0xfee9; break;   //he
            case 0x648: fin_med = 0xfeee; break;   //vav
            case 0x649: base = 0xfeef; break;  //ye
            case 0x64a: base = 0xfef1; break;  //ye
            case 0x64f: break;
            case 0x651: break;
            case 0x67e: { static const wchar m67e[3] = { 0xfb57, 0xfb59, 0xfb59 }; map = m67e; } break;  //pe
            case 0x686: base = 0xfb7a; break;   //che
            case 0x698: fin_med = 0xfb8b; break;   //zhe
            case 0x6a9: base = 0xfb8e; break;   //kaf
            case 0x6af: base = 0xfb92; break;   //gaf
            case 0x6cc: { static const wchar m6cc[3] = { 0xfbfd, 0xfef3, 0xfef4 }; map = m6cc; } break;  //ye
            case 0xfe83:
            case 0xfef5:
            case 0xfef7:
            case 0xfef9:
            case 0xfefb:
               fin_med = c+1; break;
            default: assert(0);
            }
            if(base)
               c = base+form;
            else if(map)
               c = map[form-1];
            else if(fin_med){
               if(form&1)
                  c = fin_med;
            }
         }
      }
      line[di++] = c;
   }
   line[di] = 0;

   delete[] tmp;
}

//----------------------------
#ifndef ANDROID

bool C_application_ui::CanUseInternalFont() const{

                              //check first few texts
   for(int i=1; i<6; i++){
      const wchar *txt = GetText(i);
      while(*txt){
         wchar c = *txt++;
         switch(c>>8){
         case 0:              //Latin, Latin1
         case 1:              //Latin ext A/B
         case 2:              //Latin ext B
         //case 4:              //cyrillic
            break;
         default:
            return false;
         }
      }
   }
   return true;
}

#endif
//----------------------------

dword C_application_ui::LoadTexts(const char *lang_name, const C_zip_package *dta, wchar *&_texts_buf, dword num_lines){

   Cstr_w fn, lang;
   lang.Copy(lang_name);
   fn = L"lang\\";
#if defined _DEBUG && !defined _WIN32_WCE
   if(data_path)
      fn<<data_path;
#endif
   fn<<lang <<L".txt";

   C_file_zip fl;
   bool ok = false;
#if defined S60
   bool test_location = false;
   if(data_path){
      Cstr_w fn;
      fn.Format(L"E:\\LCG Lang\\%#%.txt") <<data_path <<lang;
      ok = fl.Open(fn, NULL);
      //Fatal(fn, ok);
      test_location = true;
   }
#endif
   if(!ok){
#if defined _DEBUG && !defined _WIN32_WCE
                              //open from local disk in debug builds
      ok = fl.Open(fn, NULL);
      if(!ok){
         fn = L"lang\\";
         fn<<lang <<L".txt";
      }
      if(!ok)
#endif
      ok = fl.Open(fn, dta);
   }
                              //not found? try default
   if(!ok){
      LOG_RUN("Can't open texts, try default English");
      ok = fl.Open(L"lang\\english.txt", dta);
   }
   if(!ok){
      //Fatal("!", (dword)dta);
      Fatal("Cannot open texts", 1);
   }

   word mark;
   if(!fl.ReadWord(mark) || mark!=0xfeff){
      Fatal("bad texts", 0xfeff);
   }

   dword sz = fl.GetFileSize()-2;
   _texts_buf = new(true) wchar[num_lines + 1 + sz/sizeof(wchar)];

   fl.Read(_texts_buf+num_lines+1, sz);
   sz /= 2;
   sz += num_lines+1;
                              //scan data, make indicies
   dword id = 0;
   _texts_buf[id] = wchar(num_lines+1);
   for(dword i=num_lines+1; i<sz; i++){
      wchar &c = _texts_buf[i];
      if(c=='\r')
         c = _texts_buf[++i];
      if(c=='\n'){
         if(id==num_lines){
#if defined S60
            if(test_location)
               break;
#endif
            Fatal("bad texts", 2);  //too many texts
         }
         c = 0;
         _texts_buf[++id] = word(i+1);
      }
   }
   if(IsRtlChar((_texts_buf+_texts_buf[1])[0])){
                              //arabic, convert lines
      for(dword i=0; i<id; i++)
         ConvertRtlLine((wchar*)(_texts_buf+_texts_buf[i]));
   }
   while(id<num_lines)
      _texts_buf[id++] = 0;
   return sz;
}

//----------------------------

void C_application_ui::LoadTexts(const char *lang_name, const C_zip_package *dta, dword last_id){

   dword num_lines = last_id-1;
   delete[] texts_buf;
   dword texts_sz = LoadTexts(lang_name, dta, texts_buf, num_lines);
   if(!texts_buf[num_lines-1]){
      if(!StrCmp(lang_name, "English"))
         Fatal("bad texts", 3);  //too few texts
                              //fill missing texts from english
      dword first_id = num_lines-1;
      while(first_id && !texts_buf[first_id-1])
         --first_id;
      wchar *buf;
      dword sz = LoadTexts("English", dta, buf, num_lines);
      dword copy_len = sz - buf[first_id];
                              //merge text buffers
      wchar *new_buf = new(true) wchar[texts_sz+copy_len];
      MemCpy(new_buf, texts_buf, texts_sz*2);
      delete[] texts_buf;
      texts_buf = new_buf;
      MemCpy(texts_buf+texts_sz, buf+sz-copy_len, copy_len*2);
      texts_sz += copy_len;
      while(first_id<num_lines){
         texts_buf[first_id] = wchar(buf[first_id] - sz + texts_sz);
         ++first_id;
      }
      delete[] buf;
   }
   num_text_lines = num_lines;
}

//----------------------------

const wchar *C_application_ui::GetText(dword id) const{

   if(id>=0x8000)
   switch(id){
   case SPECIAL_TEXT_OK: return L"OK";
   case SPECIAL_TEXT_SELECT: return L"Select";
   case SPECIAL_TEXT_CANCEL: return L"Cancel";
   case SPECIAL_TEXT_SPELL: return L"Spell";
   case SPECIAL_TEXT_PREVIOUS: return L"Previous";
   case SPECIAL_TEXT_CUT: return L"Cut";
   case SPECIAL_TEXT_COPY: return L"Copy";
   case SPECIAL_TEXT_PASTE: return L"Paste";
   case SPECIAL_TEXT_BACK: return L"Back";
   case SPECIAL_TEXT_YES: return L"Yes";
   case SPECIAL_TEXT_NO: return L"No";
   case SPECIAL_TEXT_MENU: return L"Menu";
   default: assert(0);
   }
   if(!id)
      return NULL;
   --id;                      //decrement, since ID's are 1-based
   if(id>=num_text_lines){
      assert(0);
      return NULL;
   }
   return texts_buf + texts_buf[id];
}

//----------------------------

C_mode *C_application_ui::FindMode(dword id){

   for(C_mode *m=mode; m; m=m->GetParent()){
      if(m->Id()==id)
         return m;
   }
   return NULL;
}

//----------------------------

void C_application_ui::RedrawScreen(){

   if(!mode){
                              //no mode set, just clear screen
      ClearToBackground(S_rect(0, 0, ScrnSX(), ScrnSY()));
   }else{
                              //draw active mode
      C_mode &mod = *mode;
      mod.Draw();
                              //don't redraw possible network action again
      if(mod.socket_redraw_timer)
         mod.socket_redraw_timer->Pause();

                              //draw menu
      C_menu *menu = mod.GetMenu();
      if(menu)
         RedrawDrawMenuComplete(menu);
   }
}

//----------------------------
//#include <bezier.h>

void C_application_ui::TimerTick(C_timer *t, void *context, dword time){

   switch(dword(context)&3){
   case 0:                    //C_mode::Tick, context is C_mode*
      {
         C_mode *mp = (C_mode*)context;
         if(!mp)
            mp = mode;
         if(mp){
            if(t==mp->socket_redraw_timer){
               mp->DrawDirtyControls();
               mp->last_socket_draw_time = GetTickTime();
            }else
               ClientTick(*mp, time);
            UpdateScreen();
         }
      }
      break;
#ifdef USE_MOUSE
   case 1:                 //touch menu animation
      if(mode){
         C_menu_imp *menu = (C_menu_imp*)mode->GetMenu();
         if(menu && menu->animation.anim_timer==t){
            AnimateTouchMenu(*mode, time);
         }else
            assert(0);
      }
      break;
   case 2:                 //kinetic scrolling
      if(mode){
         C_list_mode_base &mod = *(C_list_mode_base*)(dword(context) & ~3);
         assert(mod.scroll_timer==t);

         if(mod.kinetic_scroll.Tick(time))
            mod.DeleteScrollTimer();
         int sb_pos = mod.kinetic_scroll.GetCurrPos().y;
         mod.sb.pos = Max(0, Min(mod.sb.total_space-mod.sb.visible_space, sb_pos));
         if(mod.sb.pos!=sb_pos)
            mod.StopKineticScroll();
         mod.ScrollChanged();
         mod.DrawContents();
         UpdateScreen();
      }
      break;
#endif
   case 3:
      {
         C_ui_control *ctrl = (C_ui_control*)(dword(context) & ~3);
         ctrl->Tick(time);
         if(ctrl->need_draw){
            ctrl->mod.DrawDirtyControls();
            //ctrl->Draw();
            UpdateScreen();
         }
      }
      break;
   }
}

//----------------------------

void C_application_ui::ClientTick(C_mode &mod, dword time){

   bool redraw = false;
   mod.AddRef();
   mod.Tick(time, redraw);
   if((redraw || mod.IsForceRedraw()) && mod.IsActive()){
      RedrawScreen();
   }
   mod.Release();
   UpdateScreen();
}

//----------------------------

void C_application_ui::ProcessInput(S_user_input &ui){

   if(!mode)
      return;
   /*
#ifdef _DEBUG
                              //debug clear screen without dirty rect, for checking redraws
   switch(ui.key){
   case 'l':
      if(ui.key_bits&GKEY_CTRL){
         ClearScreen();
         UpdateScreen();
         ClearScreen(0xffff0000);
         ResetDirtyRect();
         return;
      }
      break;
   case 'r':
      if(ui.key_bits&GKEY_CTRL){
         RedrawScreen();
         UpdateScreen();
         return;
      }
      break;
   }
#endif
   */
   C_mode &mod = *mode;
   {
      bool redraw = false;
      C_menu_imp *menu = (C_menu_imp*)mod.GetMenu();
      mod.AddRef();
      if(menu){
         menu->AddRef();
                              //process menu
         dword mid = menu->GetMenuId();
         int prev_sel = menu->selected_item;
         int itm = MenuProcessInput(mod, ui, redraw);
         if(itm==-1 && mod.GetMenu()==menu && menu->want_sel_change_notify && menu->selected_item!=prev_sel)
            itm = -2;
                              //re-focus control which was unfocused during menu popup
         if(!mod.GetMenu() && mod.GetFocus() && mod.IsActive()){
            mod.GetFocus()->SetFocus(true);
         }
         if(itm!=-1 || mod.GetMenu()!=menu){
            mod.ProcessMenu(itm, mid);
                              //don't redraw if mode was changed
            if(&mod!=(C_mode*)mode)
               redraw = false;
         }
         menu->Release();
      }else{
         if(ui.key)
            ui_touch_mode = false;
         else
         if(ui.mouse_buttons)
            ui_touch_mode = true;
         mod.ProcessInput(ui, redraw);
      }
      mod.Release();
      if(redraw && mode)
         RedrawScreen();
      UpdateScreen();
   }
}

//----------------------------
#ifdef USE_MOUSE

void C_application_ui::InitSoftkeyBarButtons(){

   if(!HasMouse()){
      buttons_image = NULL;
      return;
   }
   int NUM_SPACES = MAX_BUTTONS;
#ifdef _WIN32_WCE
   ++NUM_SPACES;
#endif
   const int but_spc = int(ScrnSX() - soft_buttons[0].GetRect().Right()*2 - 2) / NUM_SPACES;
   const int but0_x = (ScrnSX() - but_spc*NUM_SPACES+6) / 2;
   for(int i=MAX_BUTTONS; i--; ){
      S_rect rc;
      rc.sx = but_spc - 6;
      rc.y = soft_buttons[0].GetRect().y;
      rc.sy = soft_buttons[0].GetRect().sy;
      int ii = i;
#ifdef _WIN32_WCE
                              //space for text indicator or button
      if(i>=2)
         ++ii;
#endif
      rc.x = but0_x + but_spc*ii;
      softkey_bar_buttons[i].SetRect(rc);
   }
   buttons_image = C_image::Create(*this);
   buttons_image->Release();
   const S_rect &rc = softkey_bar_buttons[0].GetRect();
   int limit = rc.sx-2;
   if(rc.sy)
      limit = Min(limit, rc.sy-2);
   buttons_image->Open(L"Buttons.png", 0, limit, CreateDtaFile());

#ifdef _WIN32_WCE
   {
      S_rect rc = softkey_bar_buttons[0].GetRect();
      rc.sx = rc.sy*3/2;
      rc.x = ScrnSX()/2 - rc.sx/2;
      but_keyboard.SetRect(rc);
   }
#endif
}

//----------------------------

const int HINT_DELAY = 500, HINT_TIME = 2500;

void C_application_ui::DrawBottomButtons(const C_mode &mod, const char but_defs[MAX_BUTTONS], const dword hints[MAX_BUTTONS], dword enabled_buttons){

                              //must have mouse, and inited button image
   if(!HasMouse() || !buttons_image || !GetSoftButtonBarHeight())
      return;
                              //must be active mode
   if(!mod.IsActive())
      return;
                              //can't have menu, or menu is animated
   if(mod.menu){
      const C_menu_imp *mp = (C_menu_imp*)(const C_menu*)mod.GetMenu();
      if(!mp->IsAnimating())
         return;
   }

   int hint_but = -1;
   for(int i=MAX_BUTTONS; i--; ){
      int bi = (signed char)but_defs[i];
      if(bi==-1)
         continue;
      const C_button &but = softkey_bar_buttons[i];
      bool enabled = (enabled_buttons&(1<<i));

      if(but.down && but.hint_on)
         hint_but = i;

      DrawButton(but, enabled);

                              //draw image
      C_fixed alpha = C_fixed::Percent(enabled ? 100 : 50);
      S_rect irc(0, 0, 0, buttons_image->SizeY());
      irc.sx = irc.sy;
      irc.x = irc.sx*bi;
      int dx = but.GetRect().CenterX() - irc.sx/2, dy = but.GetRect().CenterY() - irc.sy/2;
      if(but.down)
         ++dx, ++dy;
      buttons_image->DrawSpecial(dx, dy, &irc, alpha);
   }
   if(hint_but!=-1){
      const C_button &but = softkey_bar_buttons[hint_but];
      const wchar *wp = GetText(hints[hint_but]);
      S_rect rc(Max(1, but.GetRect().x-10), but.GetRect().y-12, GetTextWidth(wp, UI_FONT_BIG)+2, fdb.cell_size_y);
      rc.x = Min(rc.x, ScrnSX()-rc.sx-3);
      FillRect(rc, 0xffffffff);
      DrawOutline(rc, 0xff000000, 0xff000000);
      DrawString(wp, rc.x+1, rc.y, UI_FONT_BIG);
      DrawSimpleShadow(rc, true);
   }
}

//----------------------------

int C_application_ui::TickBottomButtons(const S_user_input &ui, bool &redraw, dword enabled_buttons){

                              //check hint delays
   for(int i=0; i<MAX_BUTTONS; i++){
      C_button &but = softkey_bar_buttons[i];
      bool enabled = (enabled_buttons&(1<<i));
      if(!enabled && (but.down || but.clicked)){
         but.down = but.clicked = false;
         redraw = true;
      }
      if(but.down){
         dword cnt = GetTickTime() - but.down_count;
         bool on = (cnt >= dword(HINT_DELAY) && cnt < dword(HINT_DELAY+HINT_TIME));
         if(on!=but.hint_on){
            but.hint_on = on;
            redraw = true;
         }
         break;
      }
   }
   for(int i=0; i<MAX_BUTTONS; i++){
      bool enabled = (enabled_buttons&(1<<i));
      if(!enabled)
         continue;
      if(ButtonProcessInput(softkey_bar_buttons[i], ui, redraw))
         return i;
   }
   return -1;
}

//----------------------------
#endif//USE_MOUSE

//----------------------------

void C_application_ui::GetDialogRectPadding(int &l, int &t, int &r, int &b) const{

   const_cast<C_application_ui*>(this)->InitShadow();
   const C_image *sh_l = shadow[SHD_L], *sh_r = shadow[SHD_R],
      *sh_t = shadow[SHD_T], *sh_b = shadow[SHD_B];
   if(sh_l){
      l = sh_l->SizeX() + 1;
      t = sh_t->SizeY() + 1;
      r = sh_r->SizeX() + 1;
      b = sh_b->SizeY() + 1;
   }else{
      l = t = 1;
      r = b = 4;
   }
}

//----------------------------

C_internet_connection *C_application_ui::CreateConnection(){

   if(!connection){
      connection = ::CreateConnection(this,
#if defined __SYMBIAN32__ || defined BADA
         -1,
#else
         NULL,
#endif
         60*1000);
      connection->Release();
   }
   return connection;
}

//----------------------------

void C_application_ui::SocketEvent(E_SOCKET_EVENT ev, C_socket *socket, void *context){

   const int SOCKET_REDRAW_TIME = 200;
   if(context){
      C_mode &mod = *(C_mode*)context;
      bool redraw = false;
      mod.AddRef();
      mod.SocketEvent(ev, socket, redraw);
      if(mod.IsActive()){
         if(redraw)
            mod.MarkRedraw();
         if(redraw || mod.any_ctrl_redraw){
            dword tc = GetTickTime();
            if(int(tc-mod.last_socket_draw_time) >= SOCKET_REDRAW_TIME){
                              //draw now
               mod.last_socket_draw_time = tc;
               mod.DrawDirtyControls();
            }else{
                              //schedule to redraw later
               if(!mod.socket_redraw_timer)
                  mod.socket_redraw_timer = C_application_base::CreateTimer(SOCKET_REDRAW_TIME, &mod);
               else
                  mod.socket_redraw_timer->Resume();
            }
         }
      }
      mod.Release();
      UpdateScreen();
   }
}

//----------------------------

bool C_list_mode_base::EnsureVisible(){

   if(!IsPixelMode()){
      int prev_top_line = top_line;
      if(sb.visible_space==1){
                                 //if just one line, set top_line to this
         top_line = selection;
      }else{
         if(selection < top_line)
            top_line = selection;
         else
         if(selection >= sb.visible_space+top_line){
            top_line = selection-sb.visible_space+1;
         }
         if(sb.visible_space>2){
                                 //keep selection one line from top/bottom
            if(selection==top_line && top_line)
               --top_line;
            else
            if(selection==top_line+sb.visible_space-1 && top_line<sb.total_space-sb.visible_space)
               ++top_line;
         }
      }
      top_line = Max(0, Min(sb.total_space-sb.visible_space, top_line));
      return (prev_top_line!=top_line);
   }else{
      int prev_pos = sb.pos;
      /*
      if(sb.visible_space==1){
                                 //if just one line, set top_line to this
         sb.pos = 0;
      }else*/
      {
         int sel_y = selection * entry_height;
         if(sb.visible_space<=entry_height*3){
            if(sel_y < sb.pos)
               sb.pos = sel_y;
            else if(sel_y+entry_height >= sb.visible_space+sb.pos)
               sb.pos = Min(sel_y, sel_y-sb.visible_space+entry_height);
         }else{
            if(sel_y < sb.pos+entry_height)
               sb.pos = sel_y-entry_height;
            else
            if(sel_y+entry_height >= sb.visible_space+sb.pos-entry_height)
               sb.pos = sel_y-sb.visible_space+entry_height*2;
         }
      }
      sb.pos = Max(0, Min(sb.total_space-sb.visible_space, sb.pos));
      return (prev_pos!=sb.pos);
   }
}

//----------------------------

void C_list_mode_base::MakeSureSelectionIsOnScreen(){

   selection = Max(selection, top_line);
   selection = Min(selection, top_line+sb.visible_space-1);
}

//----------------------------

bool C_list_mode_base::BeginDrawNextItem(S_rect &rc_item, int &item_index) const{

   C_application_ui &app = AppForListMode();
   if(item_index==-1){
      if(!GetNumEntries())
         return false;
                              //begin drawing
                              //compute item rect
      rc_item = rc;
      rc_item.sx = GetMaxX()-rc.x;
      rc_item.sy = entry_height;

      if(IsPixelMode()){
         app.SetClipRect(rc);
                                 //pixel offset
         rc_item.y -= (sb.pos % entry_height);
         item_index = sb.pos / entry_height;
      }else{
         item_index = top_line;
      }
      return true;
   }
                              //begin drawing next item
   int new_y = rc_item.y + entry_height;
   if(IsPixelMode()){
      if(new_y>=rc.Bottom())
         return false;
   }else{
      if((new_y+rc_item.sy)>rc.Bottom())
         return false;
   }
   if(item_index+1 >= GetNumEntries())
      return false;
   rc_item.y = new_y;
   ++item_index;
   return true;
}

//----------------------------

void C_list_mode_base::EndDrawItems() const{

   if(IsPixelMode()){
       AppForListMode().ResetClipRect();
   }
}

//----------------------------

dword C_list_mode_base::GetPageSize(bool up){

   if(IsPixelMode())
      return sb.visible_space/entry_height - 1;
   return sb.visible_space-1;
}

//----------------------------

void C_kinetic_movement::Reset(){

   bool anim = IsAnimating();
   dir.x = dir.y = 0;
   anim_timer = 0;
   detect_time = 0;
   if(anim)
      AnimationStopped();
}

//----------------------------

bool C_kinetic_movement::ProcessInput(const S_user_input &ui, const S_rect &rc_active, int axis_x, int axis_y){

   bool beg_move = false;
   if(ui.key)
      Reset();
   if(ui.mouse_buttons&MOUSE_BUTTON_1_DOWN){
      if(IsAnimating() && ui.CheckMouseInRect(rc_active))
         acc_dir = dir;
      else
         acc_dir.x = acc_dir.y = 0;
      acc_beg_time = GetTickTime();
      Reset();
      if(ui.CheckMouseInRect(rc_active))
         detect_time = GetTickTime();
   }
   if((ui.mouse_buttons&MOUSE_BUTTON_1_DRAG) && detect_time){
      if(!(ui.mouse_buttons&MOUSE_BUTTON_1_DOWN)){
         dword tc = GetTickTime();
         dword tdelta = tc - detect_time;
         detect_time = tc;
         if(tdelta){
            for(int i=0; i<2; i++){
               if(!i){
                  if(!axis_x)
                     continue;
               }else{
                  if(!axis_y)
                     continue;
               }
               int spd = !i ? ui.mouse_rel.x : ui.mouse_rel.y;
               assert(Abs(spd)<0x10000);
               spd = spd*1000/int(tdelta);
               int &d = !i ? dir.x : dir.y;
               d = (d+spd)/2;
            }
         }
      }
   }
   if(ui.mouse_buttons&MOUSE_BUTTON_1_UP){
      dword curr_t = GetTickTime();
      if(dir.x || dir.y){
         if((curr_t - detect_time) > 200)
            dir.x = dir.y = 0;
      }
      const int TRESH = 50;
      const int DIR_DIV = 2;
      if(Abs(dir.x)>TRESH || Abs(dir.y)>TRESH){
         beg_move = true;
         detect_time = 0;
         anim_timer = 1;
         dir.x /= DIR_DIV;
         dir.y /= DIR_DIV;
         dir.x *= axis_x;
         dir.y *= axis_y;

         if(dir.x && dir.y){
            total_anim_time = C_math::Sqrt(dir.x*dir.x+dir.y*dir.y);
         }else
            total_anim_time = Max(Abs(dir.x), Abs(dir.y));
         total_anim_time *= 2;
         r_total_time = C_fixed::One()/C_fixed(total_anim_time);
                              //add assumulated direction from last run

                              //don't accumulate in opposite dir
         if(acc_dir.x*dir.x < 0)
            acc_dir.x = 0;
         if(acc_dir.y*dir.y < 0)
            acc_dir.y = 0;
                              //attenuate acceleration over time
         const int ACC_ATT_TIME = 200;
         int acc_delta = Max(0, ACC_ATT_TIME - int(curr_t - acc_beg_time));
         acc_dir.x = acc_dir.x*acc_delta/ACC_ATT_TIME;
         acc_dir.y = acc_dir.y*acc_delta/ACC_ATT_TIME;
         dir += acc_dir;

         curr_pos = start_pos = ui.mouse;
      }
   }
   assert(Abs(dir.x)<0x10000);
   assert(Abs(dir.y)<0x10000);
   return beg_move;
}

//----------------------------

bool C_kinetic_movement::Tick(dword tick_time, S_point *rel_movement){

   bool end = false;
   if(!IsAnimating()){
      if(rel_movement)
         rel_movement->x = rel_movement->y = 0;
   }else{
      if(rel_movement)
         *rel_movement = curr_pos;
      curr_pos = start_pos;

      if((anim_timer += tick_time) >= total_anim_time){
                              //end of this movement
         curr_pos += dir;
         Reset();
         end = true;
      }else{
         C_fixed pos = C_fixed(anim_timer)*r_total_time;
         pos = C_math::Sin(pos*C_math::PI()>>1);
         curr_pos.x += C_fixed(dir.x)*pos;
         curr_pos.y += C_fixed(dir.y)*pos;
      }
      if(rel_movement){
         rel_movement->x = curr_pos.x - rel_movement->x;
         rel_movement->y = curr_pos.y - rel_movement->y;
      }
   }
   return end;
}

//----------------------------

bool C_list_mode_base::ProcessInputInList(S_user_input &ui, bool &redraw){

   if(!GetNumEntries())
      return false;

   bool redraw_contents = false;
   int old_sel = selection;
#ifdef USE_MOUSE
   C_application_ui &app = AppForListMode();
   {
      C_scrollbar::E_PROCESS_MOUSE pm = app.ProcessScrollbarMouse(sb, ui);
      switch(pm){
      case C_scrollbar::PM_PROCESSED:
      case C_scrollbar::PM_CHANGED:
         redraw_contents = true;
         ui.mouse_buttons = 0;
         StopKineticScroll();
         ScrollChanged();
         break;
      default:
         if(ui.mouse_buttons&MOUSE_BUTTON_1_DOWN){
            app.MakeTouchFeedback(app.TOUCH_FEEDBACK_SELECT_LIST_ITEM);

            DeleteScrollTimer();
            if(ui.CheckMouseInRect(rc)){
               assert(entry_height);
               int line;
               if(!IsPixelMode()){
                  line = (ui.mouse.y - rc.y) / entry_height;
                  line += top_line;
               }else{
                  line = (ui.mouse.y - rc.y + sb.pos) / entry_height;
               }
               if(line < (int)GetNumEntries()){
                  if(selection!=line){
                     int prev = selection;
                     selection = line;
                     if(IsPixelMode()){
                                 //fix scrolling, so that selected line is completely on screen
                        int sel_y = selection * entry_height;
                        if(sel_y < sb.pos)
                           sb.pos = sel_y;
                        else
                        if(sel_y+entry_height >= sb.visible_space+sb.pos){
                           sb.pos = sel_y-sb.visible_space+entry_height;
                        }
                     }
                     SelectionChanged(prev);
                     redraw_contents = true;
                  }else
                     touch_down_selection = selection;
                  touch_move_mouse_detect = ui.mouse.y;
               }
               dragging = true;
            }
         }
         if((ui.mouse_buttons&MOUSE_BUTTON_1_DRAG) && IsPixelMode() && dragging){
                              //detect beginning of movement
            if(touch_move_mouse_detect!=-1){
               const int MOVE_TRESH = app.fdb.line_spacing;
               int d = touch_move_mouse_detect - ui.mouse.y;
               if(Abs(d) >= MOVE_TRESH){
                  assert(Abs(d)<0x10000);
                  touch_down_selection = -1;
                  touch_move_mouse_detect = -1;
                  ui.mouse_rel.y = -d;
                  assert(Abs(ui.mouse_rel.y)<0x10000);
               }
            }
            if(touch_move_mouse_detect==-1){
               int pos = sb.pos - ui.mouse_rel.y;
               pos = Max(0, Min(sb.total_space-sb.visible_space, pos));
               if(sb.pos!=pos){
                  sb.pos = pos;
                  redraw_contents = true;
                  ScrollChanged();
                  /*
                                 //if moved too much, don't allow activating the item
                  if(Abs(int(kinetic_scroll.beg)-sb.pos) >= entry_height/2){
                     touch_down_selection = -1;
                  }
                  */
               }
            }
         }
         if(ui.mouse_buttons&MOUSE_BUTTON_1_UP){
            if(touch_move_mouse_detect!=-1)
               kinetic_scroll.Reset();
            dragging = false;
         }
         if(app.IsKineticScrollingEnabled() && IsPixelMode()){
            if(kinetic_scroll.ProcessInput(ui, rc, 0, -1)){
               if(!scroll_timer)
                  scroll_timer = ((C_application_base&)app).CreateTimer(20, (void*)(dword((C_list_mode_base*)this)|2));
               kinetic_scroll.SetAnimStartPos(0, sb.pos);
            }else if(ui.mouse_buttons&MOUSE_BUTTON_1_UP){
               if(touch_down_selection==selection)
                  ui.key = K_ENTER;
            }
         }else
         if((ui.mouse_buttons&MOUSE_BUTTON_1_UP) && touch_down_selection==selection)
            ui.key = K_ENTER;
         if(ui.mouse_buttons&MOUSE_BUTTON_1_UP)
            touch_down_selection = -1;
      }
   }
#endif

   dword key = ui.key;
   if(ui.key_bits&GKEY_CTRL)
   switch(key){
   case K_CURSORUP: key = '*'; break;
   case K_CURSORDOWN: key = '#'; break;
   }

   if(key)
      StopKineticScroll();
   switch(key){
#ifdef _DEBUG_
   case 't':
      if(!scroll_timer){
         scroll_timer = ((C_application_base&)app).CreateTimer(20, (void*)(dword((C_list_mode_base*)this)|2));
      }
      kinetic_scroll.length = entry_height*9;
      if(ui.key_bits&GKEY_CTRL)
         kinetic_scroll.length = -kinetic_scroll.length;
      kinetic_scroll.time = 0;
      kinetic_scroll_beg = sb.pos;
      break;
#endif
   case K_CURSORUP:
   case K_CURSORDOWN:
      {
         int old_sel1 = selection;
         if(key==K_CURSORUP){
            if(!selection)
               selection = GetNumEntries();
            --selection;
         }else{
            if(++selection==(int)GetNumEntries())
               selection = 0;
         }
         if(selection!=old_sel1){
            EnsureVisible();
            SelectionChanged(old_sel1);
            redraw_contents = true;
         }
      }
      break;

   case K_HOME:
   case K_END:
      {
         int s = selection;
         s = ui.key==K_HOME ? 0 : GetNumEntries()-1;
         if(selection!=s){
            int old_sel1 = selection;
            selection = s;
            EnsureVisible();
            SelectionChanged(old_sel1);
            redraw_contents = true;
         }
      }
      break;

   case '*':
   case '#':
   case 'q':
   case 'a':
   case K_PAGE_UP:
   case K_PAGE_DOWN:
      {
         int s = selection;
         bool up = (key=='*' || key=='q' || key==K_PAGE_UP);
         int add = GetPageSize(up);
         if(up)
            add = -add;
         s += add;
         s = Max(0, Min((int)GetNumEntries()-1, s));
         if(selection!=s){
            int old_sel1 = selection;
            selection = s;
            EnsureVisible();
            SelectionChanged(old_sel1);
            redraw_contents = true;
         }
      }
      break;
   }
   if(redraw_contents)
      DrawContents();

   return (old_sel!=selection);
}

//----------------------------

const char C_application_ui::send_key_name[] = {
   "[Send]"
}, C_application_ui::ok_key_name[] = {
#ifdef WINDOWS_MOBILE
   "[Enter]"
#else
   "[OK]"
#endif
};

//----------------------------

Cstr_c C_application_ui::GetShiftShortcut(const char *fmt) const{

#ifdef ANDROID
                              //ANDROID has no shift, so shortcut won't work
   return NULL;
#else
   Cstr_c ret;
   ret.Format(fmt) <<shift_key_name;
   return ret;
#endif
}

//----------------------------
//----------------------------

C_mode_dialog_base::C_mode_dialog_base(C_application_ui &_app, dword lsk_tid, dword rsk_tid):
   C_mode_app<C_application_ui>(_app),
   buttons_in_dlg(false)
{
   RemoveSoftkeyBar();
#if defined ANDROID || (!defined _WIN32_WCE && defined _DEBUG)
   if(app.HasMouse())
      buttons_in_dlg = true;
#endif
   buttons[0] = buttons[1] = NULL;
   SetSoftKeys(app.GetText(lsk_tid), app.GetText(rsk_tid));
}

//----------------------------

void C_mode_dialog_base::SetSoftKeys(const wchar *lsk, const wchar *rsk){
   lsb_txt = lsk;
   rsb_txt = rsk;
   if(buttons_in_dlg){
      if(lsk){
         if(!buttons[0])
            AddControl(buttons[0] = new C_but(this, K_SOFT_MENU), true);
         buttons[0]->SetText(lsk);
      }else
      if(buttons[0]){
         RemoveControl(buttons[0]); buttons[0] = NULL;
      }
      if(rsk){
         if(!buttons[1])
            AddControl(buttons[1] = new C_but(this, K_SOFT_SECONDARY), true);
         buttons[1]->SetText(rsk);
      }else
      if(buttons[1]){
         RemoveControl(buttons[1]); buttons[1] = NULL;
      }
      InitButtons();
   }
}

//----------------------------

void C_mode_dialog_base::ClearToBackground(const S_rect &_rc){
   app.DrawDialogBase(rc, false, &_rc);
}

//----------------------------

class C_mode_dialog_base::C_ctrl_dialog_base_title_bar: public C_ctrl_title_bar{
   virtual void InitLayout(){
   }
   virtual void Draw() const{
      C_ui_control::Draw();
      C_application_ui &app = App();
      const dword col_text = app.GetColor(app.COL_TEXT_POPUP);
      int sx = rc.sx;
      app.DrawString(GetText(), rc.x+app.fdb.cell_size_x, rc.y+(rc.sy-app.fdb.cell_size_y)/2, app.UI_FONT_BIG, FF_BOLD, col_text, -(sx-app.fdb.letter_size_x*1));
   }
public:
   C_ctrl_dialog_base_title_bar(C_mode *m):
      C_ctrl_title_bar(m)
   {}
};

//----------------------------

C_ctrl_title_bar *C_mode_dialog_base::CreateTitleBar(){
   return new(true) C_ctrl_dialog_base_title_bar(this);
}

//----------------------------

void C_mode_dialog_base::InitButtons(){

                           //init buttons rects
   S_rect brc = S_rect(rc.x, 0, rc.sx/3-1, 0);
   const int padding = rc.sx/12;
   brc.x += padding;
   int sy = app.C_application_ui::GetSoftButtonBarHeight();
   sy = sy*7/8;
   brc.sy = sy - 1;
   brc.y = rc.Bottom() - (brc.sy+padding/2);
   if(buttons[0])
      buttons[0]->SetRect(brc);

   brc.x = rc.Right()-brc.sx - padding;
   if(buttons[1])
      buttons[1]->SetRect(brc);

   if(!buttons[0] || !buttons[1]){
                              //if there's possibly only one button, center it
      C_ctrl_button *but = buttons[0] ? buttons[0] : buttons[1];
      if(but){
         brc.x = rc.CenterX()-brc.sx/2;
         but->SetRect(brc);
      }
   }
}

//----------------------------

void C_mode_dialog_base::SetClientSize(const S_point &_sz){

   S_point sz = _sz;
   if(!sz.x)                  //set default width
      sz.x = Min(app.ScrnSX()*7/8, app.fdb.cell_size_x*25);
   rc.Size() = sz;
   C_ctrl_title_bar *tb = GetTitleBar();
   int tb_sy = 0;
   if(tb){
      tb_sy = app.GetDialogTitleHeight();
      rc.sy += tb_sy;
   }
   if(buttons_in_dlg){
      const int padding = rc.sx/12;
      rc.sy += app.C_application_ui::GetSoftButtonBarHeight() + padding/2;
   }

   rc.PlaceToCenter(app.ScrnSX(), app.ScrnSY());
   rc_area.x = rc.x;
   rc_area.y = rc.y;
   rc_area.Size() = sz;
   if(tb){
      rc_area.y += tb_sy;
      tb->SetRect(S_rect(rc.x, rc.y, rc.sx, tb_sy));
   }
   if(buttons_in_dlg)
      InitButtons();
}

//----------------------------

void C_mode_dialog_base::InitLayout(){

   C_mode::InitLayout();
   rc = rc_area;
   //if(GetTitleBar())
}

//----------------------------

void C_mode_dialog_base::ProcessInput(S_user_input &ui, bool &redraw){
   C_mode::ProcessInput(ui, redraw);
#ifdef USE_MOUSE
   if(app.HasMouse()){
      if(!buttons_in_dlg)
         app.ProcessMouseInSoftButtons(ui, redraw, false);
   }
#endif
}

//----------------------------

void C_mode_dialog_base::Draw() const{

   DrawParentMode(true);
   if(!buttons_in_dlg && IsActive())
      //app.DrawSoftButtonsBar(*this, lsb_txt_id, rsb_txt_id);
      app.DrawSoftButtons(lsb_txt.Length() ? (const wchar*)lsb_txt : NULL, rsb_txt.Length() ? (const wchar*)rsb_txt : NULL);
   app.DrawDialogBase(rc, (GetTitleBar()!=NULL));
   DrawAllControls();
}

//----------------------------

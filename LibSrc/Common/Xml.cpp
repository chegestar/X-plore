//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <Xml.h>
#include <C_file.h>

//----------------------------

C_xml::C_xml():
   is_utf8(true)
{}

//----------------------------

void C_xml::Reset(){
   buf.Clear();
   is_utf8 = true;
   root = C_element();
}

//----------------------------

static dword FindTagLength(const char *cp, const char *specials){

   dword i;
   for(i=0; ; i++){
      byte c = cp[i];
      int j;
      for(j=0; specials[j]; j++){
         if(specials[j]==c)
            break;
      }
      if(specials[j])
         break;
   }
   return i;
}

//----------------------------

static void SkipWSAndEol(const char *&cp){
   while(*cp && (*cp==' ' || *cp=='\t' || *cp=='\n' || *cp=='\r'))
      ++cp;
}

//----------------------------

static bool CheckStringBegin(const char *&str, const char *kw, bool ignore_case, bool advance_str){

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

static bool CheckStringBeginWs(const char *&str, const char *kw){

   if(!CheckStringBegin(str, kw, true, true))
      return false;
   SkipWSAndEol(str);
   return true;
}

//----------------------------

static bool ScanHexNumber(const char* &cp, int &num){

   bool ok = false;
   num = 0;
   while(true){
      dword c;
      char b = ToLower(*cp);
      if(b>='0' && b<='9') c = b-'0';
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

static bool ScanDecimalNumber(const char* &cp, int &num){

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

static wchar ConvertNamedHtmlChar(const char *&cp){

   dword c = *cp;
   if(!c)
      return 0;
   if(c=='#'){
      ++cp;
      int num;
      if((ToLower(*cp)=='x' && (++cp, ScanHexNumber(cp, num))) || ScanDecimalNumber(cp, num)){
         c = num;
      }else{
         //assert(0);
      }
   }else{
      const char *val_cp = cp;
      int l = FindTagLength(cp, " =()<>@,;:\\\".[]\x7f");
      if(l){
         Cstr_c val;
         val.Allocate(cp, l);
         cp += l;
#define CMAP(n, x) n"\0"x"\0"
         static const char char_map[] =
            CMAP("nbsp", " ")
            CMAP("amp", "&")
            CMAP("apos", "\'")
            CMAP("quot", "\"")
            CMAP("lt", "<")
            CMAP("gt", ">");
         const char *cm = char_map;
         while(*cm){
            int len = StrLen(cm);
            const byte *cx = (byte*)cm + len + 1;
            if(val==cm){
               c = *cx;
               if(c=='.'){
                        //wide-char string
                  ++cx;
                  c = cx[0] | (cx[1]<<8);
               }
               break;
            }
            cm = (char*)cx;
            if(*cx=='.')
               cm += 2;
            cm += 2;
         }
         if(!*cm){
                        //leave as is
            cp = val_cp;
            c = cp[-1];
         }
      }else
         c = '&';
   }
   if(*cp==';')
      ++cp;
   return wchar(c);
}

//----------------------------

static bool DecodeUtf8Text(const char *cp, C_vector<wchar> &buf){

   bool ret = true;
   while(true){
      dword c = byte(*cp++);
      if(!c)
         break;
      if((c&0xc0)==0xc0){
                           //decode utf-8 character
         dword code = 0;
         int i;
         for(i=5; i--; ){
            byte nc = byte(*cp++);
            if(!nc)
               break;
            if((nc&0xc0) != 0x80){
                              //coding error
               ret = false;
               break;
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
            ret = false;
         }
      }else
      if(c<' '){
                              //detect invalid characters
         switch(c){
         case '\n':
         case '\r':
         case '\t':
            break;
         default:
            continue;
         }
      }
      buf.push_back(wchar(c));
   }
   return ret;
}

//----------------------------

static void ConvertHtmlString(const char *cp, bool decode_utf8, Cstr_w *dst_w, Cstr_c *dst_c, Cstr_c *dst_utf8){

   C_vector<char> src;
   const char *cp_beg = cp;
                              //decode &-encoded characters
   while(*cp){
      char c = *cp++;
      if(c=='&'){
         if(!src.size())
            src.insert(src.end(), cp_beg, cp-1);
         wchar wc = ConvertNamedHtmlChar(cp);
         if(wc){
            if(wc<128)
               src.push_back((char)wc);
            else{
               Cstr_w tmp;
               tmp << wc;
               Cstr_c u8 = tmp.ToUtf8();
               for(dword i=0; i<u8.Length(); i++)
                  src.push_back(u8[i]);
            }
         }
      }else{
         if(src.size())
            src.push_back(c);
      }
   }
   int len;
   if(src.size()){
      cp = src.begin();
      len = src.size();
      src.push_back(0);
   }else{
      len = cp-cp_beg;
      cp = cp_beg;
   }

   if(dst_utf8){
                              //direct grab of utf8 string
      dst_utf8->Allocate(cp, len);
      return;
   }

   C_vector<wchar> buf;
   if(decode_utf8){
                              //decode utf-8 text
      buf.reserve(len);
      DecodeUtf8Text(cp, buf);
   }else{
      buf.reserve(len*2);
      while(*cp){
         dword c = byte(*cp++);
         //if(coding==COD_DEFAULT){
            buf.push_back(wchar(c));
            /*
         }else{
            c = C_text_utils::ConvertCodedCharToUnicode(c, coding);
            buf.push_back(wchar(c));
         }
         */
      }
   }
   if(dst_w){
      dst_w->Allocate(buf.begin(), buf.size());
   }else{
      dst_c->Allocate(NULL, buf.size());
      for(int i=0; i<buf.size(); i++){
         wchar c = buf[i];
         if(c>=256)
            c = '?';
         dst_c->At(i) = char(c);
      }
   }
}

//----------------------------

bool C_xml::C_element::Parse(const char *&cp){

   SkipWSAndEol(cp);
   if(*cp!='<'){
      assert(0);
      return false;
   }
   ++cp;
                        //get name
   tag_name = cp;
   dword tag_name_l = FindTagLength(cp, " />");
   if(!tag_name_l)
      return false;
   cp += tag_name_l;
   SkipWSAndEol(cp);

   while(*cp){
      if(*cp=='/'){
         ++cp;
         SkipWSAndEol(cp);
         if(*cp!='>')
            return false;
         ++cp;
         ((char*)tag_name)[tag_name_l] = 0;
         return true;
      }
      if(*cp=='>'){
         ++cp;
         break;
      }
                        //read attribute-value
      S_attribute attr;
      dword attr_len = FindTagLength(cp, " \t\n=");
      if(!attr_len)
         return false;
      attr.attribute = cp;
      cp += attr_len;
      *((char*&)cp)++ = 0;
      SkipWSAndEol(cp);
      char quote[2] = { *cp++, 0 };
      if(quote[0]!='\'' && quote[0]!='\"')
         return false;
      dword val_len = FindTagLength(cp, quote);
      attr.value = cp;
      cp += val_len;
      *((char*&)cp)++ = 0;
      attributes.push_back(attr);
      SkipWSAndEol(cp);
   }
   dword content_len = 0;
   while(*cp){
      if(*cp!='<'){
                     //read content
         content_len = FindTagLength(cp, "<");
         if(content_len){
            content = cp;
            cp += content_len;
         }
         if(*cp!='<')
            return false;
      }
      {
         const char *cp1 = cp+1;
         SkipWSAndEol(cp1);
         if(*cp1=='/'){
                     //should be closing of this element
            cp = cp1+1;
            dword name_len = FindTagLength(cp, " >");
            if(name_len!=tag_name_l || MemCmp(tag_name, cp, tag_name_l))
               return false;
            cp += name_len;
            SkipWSAndEol(cp);
            if(*cp!='>')
               return false;
            ++cp;
            ((char*)tag_name)[tag_name_l] = 0;
            if(content)
               ((char*)content)[content_len] = 0;
            return true;
         }
                     //parse sub-element
         C_element e;
         if(!e.Parse(cp)){
            assert(0);
            return false;
         }
         children.push_back(e);
      }
   }
   return false;
}

//----------------------------

bool C_xml::Parse(const char *cp, const char *end){

   buf.Allocate(cp, end-cp);
   return ParseInternal();
}

//----------------------------

bool C_xml::ParseInternal(){

   const char *cp = buf;
   if(CheckStringBeginWs(cp, "<?xml")){
      while(*cp!='?'){
         if(*cp=='\'' || *cp=='\"'){
            const char specs[2] = {*cp++, 0};
            cp += FindTagLength(cp, specs)+1;
         }else
            ++cp;
      }
      ++cp;
      if(*cp++!='>')
         return false;
   }
   bool ret = root.Parse(cp);
   assert(ret);
   root.SetParent(NULL);
   return ret;
}

//----------------------------

bool C_xml::Parse(class C_file &fl){

   dword sz = fl.GetFileSize();
   if(!sz)
      return false;
   buf.Allocate(NULL, sz);
   if(!fl.Read(&buf.At(0), sz))
      return false;
   return ParseInternal();
}

//----------------------------
// Check if tag name matches path, where path is terminated by / or \0.
static bool CheckSubNodeName(const char *tag_name, const char *&path){

   while(*tag_name){
      if(*tag_name!=*path)
         return false;
      ++tag_name;
      ++path;
   }
   if(*path){
      if(*path!='/')
         return false;
      ++path;
   }
   return true;
}

//----------------------------

const C_xml::C_element *C_xml::Find(const char *tag_path, const C_element *base) const{

   if(!base)
      base = &root;
   if(!CheckSubNodeName(base->tag_name, tag_path))
      return NULL;
   if(!*tag_path)
      return base;
   return base->Find(tag_path);
}

//----------------------------

const C_xml::C_element *C_xml::C_element::Find(const char *tag_path) const{

   int n = children.size();
   for(int i=0; i<n; i++){
      const C_element *ef = &children[i];
      const char *path = tag_path;
      if(CheckSubNodeName(ef->tag_name, path)){
         if(!*path)
            return ef;
         ef = ef->Find(path);
         if(ef)
            return ef;
      }
   }
   return NULL;
}

//----------------------------

const C_xml::C_element *C_xml::C_element::FindSameSibling() const{

   if(!parent)
      return NULL;
   dword i = this - parent->children.begin();
   dword n = parent->children.size();
   assert(i<n);
   for(++i; i<n; i++){
      const C_element *el = &parent->children[i];
      if(!StrCmp(el->tag_name, tag_name))
         return el;
   }
   return NULL;
}

//----------------------------

const C_xml::C_element *C_xml::C_element::GetNextSibling() const{

   if(!parent)
      return NULL;
   dword i = this - parent->children.begin();
   dword n = parent->children.size();
   assert(i<n);
   if(++i < n)
      return &parent->children[i];
   return NULL;
}

//----------------------------

const char *C_xml::C_element::FindAttributeValue(const char *attribute) const{

   for(int i=attributes.size(); i--; ){
      const S_attribute &attr = attributes[i];
      if(!StrCmp(attr.attribute, attribute))
         return attr.value;
   }
   return NULL;
}

//----------------------------

Cstr_w C_xml::DecodeString(const char *raw_string) const{

   Cstr_w ret;
   if(raw_string)
      ConvertHtmlString(raw_string, is_utf8, &ret, NULL, NULL);
   return ret;
}

//----------------------------

Cstr_c C_xml::DecodeStringSingle(const char *raw_string) const{

   Cstr_c ret;
   if(raw_string)
      ConvertHtmlString(raw_string, is_utf8, NULL, &ret, NULL);
   return ret;
}

//----------------------------

Cstr_c C_xml::DecodeStringToUtf8(const char *raw_string) const{
   Cstr_c ret;
   if(raw_string)
      ConvertHtmlString(raw_string, false, NULL, NULL, &ret);
   return ret;
}

//----------------------------

static Cstr_c EncodeXmlString(const Cstr_c &_s, bool encode_apos){

   Cstr_c s = _s;
   for(dword i=0; i<s.Length(); i++){
      char c = s[i];
      const char *rep = NULL;
      switch(c){
      case '<': rep = "lt"; break;
      case '>': rep = "gt"; break;
      case '\"': rep = "quot"; break;
      case '&': rep = "amp"; break;
      case '\'':
         if(encode_apos)
            rep = "apos";
         break;
      }
      if(rep){
         Cstr_c r = s.RightFromPos(i+1);
         s = s.Left(i);
         s<<'&' <<rep <<';' <<r;
      }
   }
   return s;
}

//----------------------------

static bool XmlWriteChar(Cstr_c *s, C_file *fl, char c){
   if(fl){
      if(fl->WriteByte(c)!=C_file::WRITE_OK)
         return false;
   }else
      (*s)<<c;
   return true;
}

//----------------------------

static bool XmlWriteString(Cstr_c *s, C_file *fl, const Cstr_c &str){
   if(fl){
      if(fl->Write((const char*)str, str.Length())!=C_file::WRITE_OK)
         return false;
   }else
      (*s)<<str;
   return true;
}

//----------------------------

static bool XmlWriteString(Cstr_c *s, C_file *fl, const char *str){
   if(fl){
      if(fl->Write(str, StrLen(str))!=C_file::WRITE_OK)
         return false;
   }else
      (*s)<<str;
   return true;
}

//----------------------------

void C_xml_build::C_element::SetContent(const Cstr_w &c){
   content_utf8 = c.ToUtf8();
}

//----------------------------

void C_xml_build::C_element::SetContent(const Cstr_c &c){
   content_utf8 = c;
   /*
   Cstr_w tmp;
   tmp.Copy(c);
   SetContent(tmp);
   */
}

//----------------------------

bool C_xml_build::WriteToString(int ident, const C_element &el, Cstr_c *s, C_file *fl, bool write_white_space) const{

   if(write_white_space){
      for(int i=0; i<ident; i++){
         if(!XmlWriteChar(s, fl, ' '))
            return false;
      }
   }
   if(!XmlWriteChar(s, fl, '<'))
      return false;
   if(!XmlWriteString(s, fl, el.tag_name))
      return false;
   for(int i=0; i<el.attributes.size(); i++){
      if(!XmlWriteChar(s, fl, ' '))
         return false;
      const C_element::S_attribute &attr = el.attributes[i];
      if(!XmlWriteString(s, fl, attr.attribute) || !XmlWriteString(s, fl, "=\""))
         return false;
      Cstr_c utf8;
      if(attr.value_utf8.Length()){
         utf8 = attr.value_utf8;
      }else{
         utf8.ToUtf8(attr.value);
      }
      if(!XmlWriteString(s, fl, EncodeXmlString(utf8, encode_apos)))
         return false;
      if(!XmlWriteChar(s, fl, '\"'))
         return false;
   }
   if(!el.content_utf8.Length() && !el.children.size()){
                              //self-closing tag
      if(!XmlWriteString(s, fl, "/>"))
         return false;
      if(write_white_space){
         if(!XmlWriteChar(s, fl, '\n'))
            return false;
      }
      return true;
   }
   if(!XmlWriteChar(s, fl, '>'))
      return false;
   if(el.children.size()){
      if(write_white_space){
         if(!XmlWriteChar(s, fl, '\n'))
            return false;
      }
      for(int i=0; i<el.children.size(); i++){
         if(!WriteToString(ident+1, el.children[i], s, fl, write_white_space))
            return false;
      }
   }
   if(el.content_utf8.Length()){
      if(!XmlWriteString(s, fl, EncodeXmlString(el.content_utf8, encode_apos)))
         return false;
   }else
   if(write_white_space){
      for(int i=0; i<ident; i++){
         if(!XmlWriteChar(s, fl, ' '))
            return false;
      }
   }
   if(!XmlWriteString(s, fl, "</"))
      return false;
   if(!XmlWriteString(s, fl, el.tag_name))
      return false;
   if(!XmlWriteChar(s, fl, '>'))
      return false;
   if(write_white_space){
      if(!XmlWriteChar(s, fl, '\n'))
         return false;
   }
   return true;
}

//----------------------------

C_xml_build::C_element *C_xml_build::C_element::Find(const char *_tag_name){

   for(int i=children.size(); i--; ){
      C_xml_build::C_element &el = children[i];
      if(!StrCmp(el.tag_name, _tag_name))
         return &el;
   }
   return NULL;
}

//----------------------------

void C_xml_build::InitRootTag(const char *root_tag_name){

   root.tag_name = root_tag_name;
}

//----------------------------

static const char xml_header[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";

//----------------------------

Cstr_c C_xml_build::BuildText(bool add_header, int write_white_space) const{

   if(write_white_space==-1){
#ifdef _DEBUG
      write_white_space = true;
#else
      write_white_space = false;
#endif
   }

   Cstr_c ret;
   if(add_header){
      ret<<xml_header;
      if(write_white_space)
         ret<<'\n';
   }
   WriteToString(0, root, &ret, NULL, write_white_space);
   ret.Build();
   return ret;
}

//----------------------------

bool C_xml_build::Write(C_file &fl, bool add_header, int write_white_space) const{


   if(write_white_space==-1){
#ifdef _DEBUG
      write_white_space = true;
#else
      write_white_space = false;
#endif
   }
   if(add_header){
      if(fl.Write(xml_header, StrLen(xml_header))!=C_file::WRITE_OK)
         return false;
      if(write_white_space){
         if(fl.WriteByte('\n'))
            return false;
      }
   }
   if(!WriteToString(0, root, NULL, &fl, write_white_space))
      return false;
   return (fl.WriteFlush()==C_file::WRITE_OK);
}

//----------------------------

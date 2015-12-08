#include "Main.h"
#ifdef EXPLORER
#include "Explorer\Main_Explorer.h"
#endif
#include <C_file.h>
#include <RndGen.h>
#include <Base64.h>

//----------------------------

bool file_utils::ReadString(C_file &ck, encoding::C_str_cod &s){

   if(!ReadString(ck, (Cstr_c&)s))
      return false;
   byte b;
   if(!ck.ReadByte(b))
      return false;
   s.coding = (E_TEXT_CODING)b;
   return true;
}

//----------------------------

bool file_utils::ReadString(C_file &ck, Cstr_w &s){

   dword sz;
   if(!ck.ReadDword(sz))
      return false;
   if(sz!=0xffffffff){
      if(!sz || (sz&0x80000000)){
         sz &= 0x7fffffff;
         s.Allocate(NULL, sz);
         if(sz){
            if(!ck.Read(&s.At(0), sz*2))
               return false;
         }
      }else{
         assert("expecting wide string!");
         return false;
      }
   }
   return true;
}

//----------------------------

bool file_utils::ReadString(C_file &ck, Cstr_c &s){

   dword sz;
   if(!ck.ReadDword(sz)) return false;
   if(sz!=0xffffffff){
      if(sz&0x80000000){
         //assert("expecting single string!");
         ck.Seek(ck.Tell()+sz*2);
         return true;
      }else{
         s.Allocate(NULL, sz);
         if(sz){
            if(!ck.Read(&s.At(0), sz))
               return false;
         }
      }
   }
   return true;
}

//----------------------------

bool file_utils::ReadStringAsUtf8(C_file &ck, Cstr_w &s){
   Cstr_c tmp;
   if(!ReadString(ck, tmp))
      return false;
   s.FromUtf8(tmp);
   return true;
}

//----------------------------

void file_utils::WriteString(C_file &ck, const Cstr_w &s){

   dword len = s.Length();
   ck.WriteDword(len | 0x80000000);
   ck.Write(s, len*sizeof(wchar));
}

//----------------------------

void file_utils::WriteString(C_file &ck, const Cstr_c &s){

   dword len = s.Length();
   ck.WriteDword(len);
   ck.Write(s, len);
}

//----------------------------

void file_utils::WriteStringAsUtf8(C_file &ck, const Cstr_w &s){
   WriteString(ck, s.ToUtf8());
}

//----------------------------

void file_utils::WriteString(C_file &ck, const encoding::C_str_cod &s){

   WriteString(ck, (Cstr_c&)s);
   ck.WriteByte(byte(s.coding));
}

//----------------------------

void file_utils::WriteEncryptedString(C_file &ck, const Cstr_c &s, const char *enc){

   dword len = s.Length();
   ck.WriteDword(len);
   const char *ep = enc;
   for(dword i=0; i<len; i++){
      ck.WriteByte(s[i] ^ *ep);
      if(!*++ep)
         ep = enc;
   }
}

//----------------------------

bool file_utils::ReadEncryptedString(C_file &ck, Cstr_c &s, const char *enc){

   dword sz;
   if(!ck.ReadDword(sz))
      return false;
   s.Allocate(NULL, sz);
   const char *ep = enc;
   for(dword i=0; i<sz; i++){
      byte b;
      if(!ck.ReadByte(b))
         return false;
      s.At(i) = b ^ *ep;
      if(!*++ep)
         ep = enc;
   }
   return true;
}

//----------------------------

void C_client::InitActiveObjectFocus(S_text_display_info &tdi, E_TEXT_CONTROL_CODE cc, int indx){

   if(cc==CC_HYPERLINK){
      tdi.active_link_index = indx;
      tdi.ts.font_flags &= ~FF_ACTIVE_HYPERLINK;
   }else
      tdi.active_link_index = -1;
#ifdef WEB_FORMS
   tdi.te_input = NULL;
#endif
}

//----------------------------

bool C_client::GoToNextActiveObject(S_text_display_info &tdi){

   if(!tdi.hyperlinks.size()){
#ifdef WEB_FORMS
      if(!tdi.form_inputs.size())
#endif
      return false;
   }
#ifdef WEB_FORMS
   if(tdi.te_input){
      S_html_form_input *p_fi = tdi.GetActiveFormInput();
      if(p_fi){
         p_fi->edit_pos = tdi.te_input->GetCursorPos();
         p_fi->edit_sel = tdi.te_input->GetCursorSel();
      }
      tdi.te_input = NULL;
   }
#endif

   const char *base = tdi.body_c, *cp = base;
   if(tdi.active_object_offs!=-1){
      cp += tdi.active_object_offs;
      S_text_style::SkipCode(cp);
   }

   while(true){
      char c = *cp;
      if(!c)
         break;
      if(S_text_style::IsControlCode(c)){
         if(c==CC_HYPERLINK || c==CC_FORM_INPUT){
            bool use = true;
            int indx = byte(cp[1]) | (byte(cp[2]) << 8);
#ifdef WEB_FORMS
            if(c==CC_FORM_INPUT){
               S_html_form_input &fi = tdi.form_inputs[indx];
               if(fi.disabled)
                  use = false;
            }
#endif
            if(use){
               tdi.active_object_offs = cp - base;
               InitActiveObjectFocus(tdi, (E_TEXT_CONTROL_CODE)c, indx);
               return true;
            }
         }
         S_text_style::SkipCode(cp);
      }else
         ++cp;
   }
   return false;
}

//----------------------------

bool C_client::GoToPrevActiveObject(S_text_display_info &tdi){

   if(!tdi.hyperlinks.size()){
#ifdef WEB_FORMS
      if(!tdi.form_inputs.size())
#endif
      return false;
   }
#ifdef WEB_FORMS
   if(tdi.te_input){
      S_html_form_input *p_fi = tdi.GetActiveFormInput();
      if(p_fi){
         p_fi->edit_pos = tdi.te_input->GetCursorPos();
         p_fi->edit_sel = tdi.te_input->GetCursorSel();
      }
      tdi.te_input = NULL;
   }
#endif
   //if(tdi.active_object_offs==-1)
      //return false;

   const char *base = tdi.body_c, *cp = base;
   if(tdi.active_object_offs!=-1)
      cp += tdi.active_object_offs;

   while(cp!=base){
      char c = *--cp;
      if(!c)
         break;
      if(S_text_style::IsControlCode(c)){
         S_text_style ts;
         ++cp;
         ts.ReadCodeBack(cp);
         if(c==CC_HYPERLINK || c==CC_FORM_INPUT){
            bool use = true;
            int indx = byte(cp[1]) | (byte(cp[2]) << 8);
#ifdef WEB_FORMS
            if(c==CC_FORM_INPUT){
               S_html_form_input &fi = tdi.form_inputs[indx];
               if(fi.disabled)
                  use = false;
            }
#endif
            if(use){
               tdi.active_object_offs = cp - base;
               InitActiveObjectFocus(tdi, (E_TEXT_CONTROL_CODE)c, indx);
               return true;
            }
         }
      }
   }
#ifndef MUSICPLAYER
   if(cp==base && tdi.active_link_index!=-1){
      tdi.active_object_offs = -1;
      tdi.active_link_index = -1;
      tdi.ts.font_flags &= ~FF_ACTIVE_HYPERLINK;
      return true;
   }
#endif
   return false;
}

//----------------------------

#ifdef WEB_FORMS
S_html_form_input *S_text_display_info::GetActiveFormInput(){

   if(active_object_offs!=-1){
      const char *cp = (const char*)body_c + active_object_offs;
      if(*cp++ == CC_FORM_INPUT){
         int ii = S_text_style::Read16bitIndex(cp);
         assert(ii<form_inputs.size());
         return &form_inputs[ii];
      }
   }
   return NULL;
}

//----------------------------

#ifdef WEB
void S_text_display_info::S_javascript::Clear(){

   dom.name.Clear();
   dom.children.clear();
   on_body_unload.Clear();
}

#endif//WEB
#endif//WEB_FORMS

//----------------------------
//----------------------------

static const struct{
   wchar ext[6];
   C_client::E_FILE_TYPE type;
} file_types[] = {
   {L"zip", C_client::FILE_TYPE_ARCHIVE},
   {L"rar", C_client::FILE_TYPE_ARCHIVE},
   {L"jar", C_client::FILE_TYPE_ARCHIVE},

   {L"txt", C_client::FILE_TYPE_TEXT},
   {L"htm", C_client::FILE_TYPE_TEXT},
   {L"html", C_client::FILE_TYPE_TEXT},

   {L"doc", C_client::FILE_TYPE_WORD},
   {L"docx", C_client::FILE_TYPE_WORD},
   {L"xls", C_client::FILE_TYPE_XLS},
   {L"pdf", C_client::FILE_TYPE_PDF},

   {L"jpg", C_client::FILE_TYPE_IMAGE},
   {L"jpe", C_client::FILE_TYPE_IMAGE},
   {L"jpeg", C_client::FILE_TYPE_IMAGE},
   {L"png", C_client::FILE_TYPE_IMAGE},
   {L"bmp", C_client::FILE_TYPE_IMAGE},
   {L"ico", C_client::FILE_TYPE_IMAGE},
   {L"pcx", C_client::FILE_TYPE_IMAGE},
   {L"gif", C_client::FILE_TYPE_IMAGE},
   {L"tga", C_client::FILE_TYPE_IMAGE},
   {L"tif", C_client::FILE_TYPE_IMAGE},
   {L"tiff", C_client::FILE_TYPE_IMAGE},

   {L"avi", C_client::FILE_TYPE_VIDEO},
   {L"rm", C_client::FILE_TYPE_VIDEO},
   {L"divx", C_client::FILE_TYPE_VIDEO},
//#ifdef __SYMBIAN32__
   {L"3gp", C_client::FILE_TYPE_VIDEO},
   {L"mp4", C_client::FILE_TYPE_VIDEO},
//#endif
   {L"m4v", C_client::FILE_TYPE_VIDEO},
   {L"mov", C_client::FILE_TYPE_VIDEO},
   {L"wmv", C_client::FILE_TYPE_VIDEO},
   {L"flv", C_client::FILE_TYPE_VIDEO},

   {L"mp3", C_client::FILE_TYPE_AUDIO},
   {L"ogg", C_client::FILE_TYPE_AUDIO},
   {L"oga", C_client::FILE_TYPE_AUDIO},
   {L"mid", C_client::FILE_TYPE_AUDIO},
   {L"wav", C_client::FILE_TYPE_AUDIO},
   {L"aac", C_client::FILE_TYPE_AUDIO},
   {L"wma", C_client::FILE_TYPE_AUDIO},
   {L"m4a", C_client::FILE_TYPE_AUDIO},
   {L"flac", C_client::FILE_TYPE_AUDIO},
#ifdef __SYMBIAN32__
   {L"mxmf", C_client::FILE_TYPE_AUDIO},
   {L"rng", C_client::FILE_TYPE_AUDIO},
#endif
   {L"amr", C_client::FILE_TYPE_AUDIO},

   {L"exe", C_client::FILE_TYPE_EXE},
   {L"vcf", C_client::FILE_TYPE_CONTACT_CARD},
   {L"vcs", C_client::FILE_TYPE_CALENDAR_ENTRY},
   {L"ics", C_client::FILE_TYPE_CALENDAR_ENTRY},

   {L"eml", C_client::FILE_TYPE_EMAIL_MESSAGE},
   {L"url", C_client::FILE_TYPE_URL},
#ifdef __SYMBIAN32__
   {L"app", C_client::FILE_TYPE_EXE},
#else
   {L"lnk", C_client::FILE_TYPE_LINK},
#endif
   {L"", C_client::FILE_TYPE_LAST}
};

C_client::E_FILE_TYPE C_client::DetermineFileType(const wchar *ext){

   if(!ext)
      return FILE_TYPE_UNKNOWN;
   for(int i=0; file_types[i].type!=FILE_TYPE_LAST; i++){
      if(!text_utils::CompareStringsNoCase(ext, file_types[i].ext)){
         return file_types[i].type;
      }
   }
   return FILE_TYPE_UNKNOWN;
}

//----------------------------

dword C_client::InitHtmlImage(const Cstr_c &img_src, const Cstr_w &img_alt, int img_sx, int img_sy,
   const char *www_domain, S_text_display_info &tdi, C_vector<char> &dst_buf, S_image_record::t_align_mode align_mode, int border) const{

   S_image_record img_rec;
   img_rec.src = img_src;
   img_rec.alt = img_alt;
   img_rec.align_mode = align_mode;
   img_rec.draw_border = (border>0);

   if(img_sx!=-1 && img_sy!=-1){
                              //size explicitly set
      img_rec.sx = img_sx;
      img_rec.sy = img_sy;
      img_rec.size_set = true;
   }else{
                              //compute proposed size
      img_rec.sx = fds.letter_size_x * 30;
#ifdef MAIL__
      const C_spriteset::S_sprite_rect &spr_rc = sset_mail->GetSpriteRect(SPR_IMG_PLACEHOLDER);
      img_rec.sy = (spr_rc.size_y + fds.cell_size_y + 5) * 2;
#else
      img_rec.sy = (fds.cell_size_y + 5) * 2;
#endif
   }
   C_fixed ratio;
   int max_sx = tdi.rc.sx;
   int max_sy = tdi.rc.sy;
   if(img_rec.draw_border){
      max_sx -= 2;
      max_sy -= 2;
   }
   C_image::ComputeSize(img_rec.sx, img_rec.sy, max_sx, max_sy,
      /*
#if defined MAIL || defined WEB
      config.img_ratio,
#else
      */
      C_fixed::One(),
//#endif
      ratio, ratio);

   if(!tdi.images.size())
      tdi.images.reserve(16);
                              //images must have domain specified, either own, or inherited
   if(www_domain && img_rec.src.Length()){
      const char *cp = img_rec.src;
      if(text_utils::CheckStringBegin(cp, "cid:")){
                              //local content, don't add domain
      }else
         text_utils::MakeFullUrlName(www_domain, img_rec.src);
   }
   tdi.images.push_back(img_rec);

                              //insert image reference to the stream
   dword ii = tdi.images.size() - 1;
   dst_buf.push_back(CC_IMAGE);
   dst_buf.push_back(char(ii&0xff));
   dst_buf.push_back(char(ii>>8));
   dst_buf.push_back(CC_IMAGE);
   return ii;
}

//----------------------------
#ifdef WEB_FORMS

bool S_js_events::ParseAttribute(const Cstr_c &attr, const Cstr_c &val){
   if(attr=="onclick")
      js_onclick = val;
   else
   if(attr=="onfocus")
      js_onfocus = val;
   else
   if(attr=="onblur")
      js_onblur = val;
   else
   if(attr=="onselect")
      js_onselect = val;
   else
   if(attr=="onchange")
      js_onchange = val;
#ifdef _DEBUG
   else
   if(attr=="ondblclick"      //pointing device button is double clicked over an element
   || attr=="onmousedown"     //pointing device button is pressed over an element
   || attr=="onmouseup"       //pointing device button is released over an element
   || attr=="onmouseover"     //pointing device is moved onto an element
   || attr=="onmousemove"     //pointing device is moved while it is over an element
   || attr=="onmouseout"      //pointing device is moved away from an element
   || attr=="onkeypress"      //a key is pressed and released over an element
   || attr=="onkeydown"       //a key is pressed down over an element
   || attr=="onkeyup"         //a key is released over an element
   ){
   }
#endif
   else
      return false;
   return true;
}
#endif//WEB_FORMS

//----------------------------

C_file *C_client::CreateFile(const wchar *filename, const C_zip_package *arch){

   C_file *fp = NULL;
   if(!arch){
      fp = new(true) C_file;
      if(!fp->Open(filename)){
         delete fp;
         fp = NULL;
      }
   }else
      fp = arch->OpenFile(filename);
   return fp;
}

//----------------------------
//----------------------------

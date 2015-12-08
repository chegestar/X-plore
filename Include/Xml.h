//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#ifndef _XML_H_
#define _XML_H_

#include <Rules.h>
#include <C_vector.h>
#include <Cstr.h>
#include <Util.h>

//----------------------------

class C_xml{
   Cstr_c buf;                //continuous string buffer, containing all strings
   bool is_utf8;
public:
   class C_element{
   public:
      struct S_attribute{
         const char *attribute, *value;

         inline bool operator==(const char *att) const{
            return (!StrCmp(attribute, att));
         }

         int IntVal() const{
            int ret = 0;
            Cstr_c(value)>>ret;
            return ret;
         }
      };
   private:
      friend class C_xml;
      const C_element *parent;

      const char *tag_name;
      C_vector<S_attribute> attributes;
      const char *content;

      C_vector<C_element> children;

      bool Parse(const char *&cp);
      void SetParent(const C_element *p){
         parent = p;
         for(int i=children.size(); i--; )
            children[i].SetParent(this);
      }
   public:
      C_element():
         parent(NULL),
         tag_name(NULL),
         content(NULL)
      {}
      inline const C_element *GetParent() const{ return parent; }
      inline const char *GetName() const{ return tag_name; }
      inline const char *GetContent() const{ return content ? content : ""; }
      inline const C_vector<C_element> &GetChildren() const{ return children; }
      inline const C_vector<S_attribute> &GetAttributes() const{ return attributes; }

      inline bool operator==(const char *name) const{
         return (!StrCmp(tag_name, name));
      }

   //----------------------------
   // Find child in tag hierarchy.
   // tag_path is string for searching in hearchy, each path level terminated by '/' (e.g. "root/child")
      const C_element *Find(const char *tag_path) const;

   //----------------------------
   // Find sibling with same name.
      const C_element *FindSameSibling() const;

   //----------------------------
   // Get next sibling.
      const C_element *GetNextSibling() const;

      const char *FindAttributeValue(const char *attribute) const;
   };
private:
   C_element root;

   bool ParseInternal();
public:
   C_xml();

//----------------------------
// Reset contents to initial state.
   void Reset();

   //inline byte GetCoding() const{ return coding; }
   inline bool IsUtf8() const{ return is_utf8; }

//----------------------------
// Parse string.
   bool Parse(const char *beg, const char *end);
   bool Parse(const char *cp){ return Parse(cp, cp+StrLen(cp)); }
   bool Parse(class C_file &fl);

//----------------------------
// Find sub-element of base (or root) element.
// 'tag_path' is path to element tree, each name terminated by '/'.
   const C_element *Find(const char *tag_path, const C_element *base = NULL) const;

//----------------------------
// Get root element.
   const C_element &GetRoot() const{ return root; }

//----------------------------
// Get first root's sub-element child.
   const C_element *GetFirstChild() const{ return root.children.size() ? root.children.begin() : NULL; }

//----------------------------
// Decode some string from element, which is coded in xml's coding.
   Cstr_w DecodeString(const char *raw_string) const;
   Cstr_c DecodeStringSingle(const char *raw_string) const;
   Cstr_c DecodeStringToUtf8(const char *raw_string) const;
};

//----------------------------

class C_xml_build{
public:
   class C_element{
      friend class C_xml_build;
      const char *tag_name;
      Cstr_c content_utf8;
      struct S_attribute{
         const char *attribute;
         Cstr_w value;
         Cstr_c value_utf8;
      };
   public:
      C_vector<S_attribute> attributes;
      C_vector<C_element> children;

      C_element(): tag_name(NULL){}
      C_element(const char *n): tag_name(n){}

      void Setname(const char *n){ tag_name = n; }

      inline void ReserveChildren(int num){
         children.reserve(num);
      }
      C_element &AddChild(const char *_tag_name){
         return children.push_back(C_element(_tag_name));
      }
      C_element &AddChildContent(const char *_tag_name, const Cstr_w &content){
         C_element &e = AddChild(_tag_name);
         e.SetContent(content);
         return e;
      }
      C_element &AddChildContent(const char *_tag_name, const Cstr_c &content){
         C_element &e = AddChild(_tag_name);
         e.SetContent(content);
         return e;
      }
      void SetContent(const Cstr_w &c);
      void SetContent(const Cstr_c &c);

      inline void ReserveAttributes(int num){
         attributes.reserve(num);
      }
      void AddAttributeValue(const char *attr, const Cstr_w &val){
         S_attribute a;
         a.attribute = attr;
         a.value = val;
         attributes.push_back(a);
      }
   //----------------------------
   // Add attr/val pair, where val is utf8-encoded.
      void AddAttributeValueUtf8(const char *attr, const Cstr_c &val){
         S_attribute a;
         a.attribute = attr;
         a.value_utf8 = val;
         attributes.push_back(a);
      }
      void AddAttributeValue(const char *attr, const Cstr_c &val){
         Cstr_w val_w; val_w.Copy(val);
         AddAttributeValue(attr, val_w);
      }
      void AddAttributeValue(const char *attr, int val){
         Cstr_w val_w; val_w<<val;
         AddAttributeValue(attr, val_w);
      }
      inline dword NumChildren() const{ return children.size(); }
      C_element *Find(const char *tag_name);
   };

private:
   bool WriteToString(int ident, const C_element &el, Cstr_c *s, C_file *fl, bool write_white_space) const;
   bool encode_apos;          //if ' is encoded as &apos;
public:
   C_element root;

   C_xml_build(const char *root_tag_name): encode_apos(true){ InitRootTag(root_tag_name); }
   C_xml_build(): encode_apos(true){}
   void InitRootTag(const char *root_tag_name);

   void SetEncodeApos(bool b){ encode_apos = b; }

//----------------------------
// Build xml string.
   Cstr_c BuildText(bool add_header, int write_white_space = -1) const;

//----------------------------
// Write xml string to file.
   bool Write(C_file &fl, bool add_header, int write_white_space = -1) const;

//----------------------------
// Clear entire contents.
   void Clear(){ root.content_utf8.Clear(); root.children.clear(); root.attributes.clear(); }
};

//----------------------------
#endif

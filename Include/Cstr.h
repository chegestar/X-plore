//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#ifndef __CSTR_H_
#define __CSTR_H_

#include <Rules.h>

//----------------------------
class Cstr_rep_base{
protected:
   dword ref_len;             //low bits is ref, high bits is length, highest bit is 'build' flag
   byte data[1];
};

//----------------------------
// base class, internal
class Cstr_base{
public:
protected:
   mutable Cstr_rep_base *rep;     //pointer to hidden implementation class

//----------------------------
   void BeginBuild();
   void CancelBuild();

//----------------------------
// Add data for formatted string into build buffer.
   void AddFormatData(dword format_type, const void *data, dword len = 0);
};

//----------------------------
                              //actual string class
template<class T>
class Cstr: private Cstr_base{
//----------------------------
// Make sure this string is the only owner of actual string data.
   void MakeUniqueCopy();

   const T *GetPtr() const;
public:
   enum{ MAX_STRING_LEN = (1<<23) - 1 };

   inline Cstr(){ rep = NULL; }

//----------------------------
// Copy constructor.
   Cstr(const Cstr &s);
   Cstr(const T *s);

//----------------------------
   ~Cstr();

//----------------------------
// Clear contents, and set length to zero.
   void Clear();

//----------------------------
// Getting pointer to C string. This is always valid NULL-terminated string, even if the string is empty.
// It is valid until any other operation is done on contents of string.
   inline operator const T *() const{ return GetPtr(); }

//----------------------------
// Assignment operator - fast copy of reference from given string.
   Cstr &operator =(const Cstr &s);

//----------------------------
// Assignment from null-terminated C string.
   Cstr &operator =(const T *cp);

//----------------------------
// Assign raw data to string. Data may be anything, no '\0' character at the end is searched.
// If 'cp' is NULL, the string allocates uninitialized buffer for holding the string,
// which may be filled further by array-access functions.
   void Allocate(const T *cp, dword length);

//----------------------------
// Get const character reference inside of string on particular position.
   const T &operator [](dword pos) const;
   inline const T &operator [](int pos) const{ return operator[]((dword)pos); }

//----------------------------
// Get non-const character reference inside of string on particular position.
// If contents of string is shared among multiple strings, it is made unique.
   T &At(dword pos);
   inline T &At(int pos){ return At((dword)pos); }

//----------------------------
// Extract parts of string. A new string is created and returned. If pos or length points beyond string length, fatal error occurs.
   Cstr Left(dword len) const;
   Cstr Right(dword len) const;
   Cstr RightFromPos(dword pos) const{ return Right(Length()-pos); }
   Cstr Mid(dword pos) const;
   Cstr Mid(dword pos, dword len) const;

//----------------------------
// Compare this string with another C string for equality. Slow, using standard StrCmp.
   inline bool operator ==(const T *cp) const{ return !cp ? (!Length()) : !StrCmp(*this, cp); }
   inline bool operator !=(const T *cp) const{ return !cp ? Length() : StrCmp(*this, cp); }

//----------------------------
// Compare this string with another string for equality. Optimizations based on sizes of strings.
   bool operator ==(const Cstr &s) const;
   inline bool operator !=(const Cstr &s) const{ return (!operator==(s)); }

//----------------------------
// Compare this string with another string.
   inline bool operator <(const Cstr &s) const{ return (StrCmp(*this, s) < 0); }
   inline bool operator <(const T *s) const{ return (StrCmp(*this, s) < 0); }
   inline bool operator <=(const Cstr &s) const{ return (StrCmp(*this, s) <= 0); }
   inline bool operator <=(const T *s) const{ return (StrCmp(*this, s) <= 0); }

//----------------------------
// Get length of string data (number of characters), without terminating null character.
   dword Length() const;

//----------------------------
// Find specified character in string in forward/reverse directions. Returns index of 1st found, or -1 if not found.
   int FindFromPos(T c, dword pos) const;
   inline int Find(T c) const{ return FindFromPos(c, 0); }
   int FindReverse(T c) const;

//----------------------------
// Convert all characters to lower/upper case.
   void ToLower();
   void ToUpper();

//----------------------------
// Copy from single/wide string. If converson from wchar to char is done, any character greater than 256 is replaced by 'invalid'.
// If input pointer is NULL, the string is cleared.
   void Copy(const char *cp);
   void Copy(const wchar *wp, char invalid = '?');

//----------------------------
                              //String building and formatting

//----------------------------
// Setup format for string. Calling 'Format' clears the string, and sets initial format string.
// Calling 'AppendFormat' causes appending formatted string to existing string.
// The format string is standard C string, containing '%' characters which are replaced by actual data, when string is rebuilt.
// Data are fed by one or more operator<< calls. The string is built using the format string and data when any function above these is called.
// 'fmt' may be also NULL (resp. no Format may be called, why calling operator<<), in which case string will be built from all passsed data
//    stored sequentally.
// Calling one of these functions puts the class to 'build' state.
   Cstr &Format(const T *fmt);
   Cstr &AppendFormat(const T *fmt);

//----------------------------
// Parameter feeding for basic types, using overloaded operators. This may be called anytime.
// Calling one of these functions puts the class to 'build' state.
   Cstr &operator <<(char d);
   Cstr &operator <<(wchar d);
   Cstr &operator <<(int d);
   Cstr &operator <<(dword d);
   Cstr &operator <<(unsigned int d);
   Cstr &operator <<(const wchar *d);
   Cstr &operator <<(const char *d);   //note: feeding char* to wchar string causes Copy method to be used internally (replacing chars >=256 by '?')
   Cstr &operator <<(const Cstr &d){ return operator<<((const T*)d); }

//----------------------------
// Converting string to basic data types. Returns true if successful.
   bool operator >>(int &i) const;
   bool operator >>(dword &i) const;

//----------------------------
// Build string - flush params and create it.
   void Build() const;

//----------------------------
// Convert wide-char string to (this) utf-8 string.
// Return true if any character was in high range (>=0x80).
   bool ToUtf8(const wchar *s);
//----------------------------
// From (this) wide-string convert to utf-8 string.
   Cstr<char> ToUtf8() const;

//----------------------------
// Convert utf-8 string to (this) wide string.
   bool FromUtf8(const char *s);
//----------------------------
// From (this) utf-8 string, convert to wide string.
   Cstr<wchar> FromUtf8() const;
};

//----------------------------

typedef Cstr<char> Cstr_c;
typedef Cstr<wchar> Cstr_w;

//----------------------------

#endif

//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#ifndef __TEXT_UTILS_H_
#define __TEXT_UTILS_H_

#include <Rules.h>
#include <Cstr.h>

struct S_date_time;

                              //macro used to crypt string; c = char, i=any value 0-255
                              // such string can be decoded by DecodeCryptedString
                              // used in places where plain string could be misused by hackers (authentication, selfchecks, etc)
#define _(c, i) char(((dword(c)>>(i&7)) | (dword(c)<<(8-(i&7)))) ^ i), char(i^0xca),


namespace text_utils{

//----------------------------
                              //http:// and https:// strings
extern const char HTTP_PREFIX[], HTTPS_PREFIX[];
extern const wchar HTTP_PREFIX_L[], HTTPS_PREFIX_L[];

//----------------------------
// Get upper/lower case. Unicode version correctly converts several unicode ranges.
wchar UpperCase(wchar c);
wchar LowerCase(wchar c);

//----------------------------
// Make user-readable time string, made of hours, minutes and optionally seconds, separated by ':'.
Cstr_w GetTimeString(const S_date_time &dt, bool also_seconds = true);

//----------------------------
// Make user-readable size text. Units are B (bytes), KB, MB or GB depending on 'sz' range.
// allow_bytes specifies if B format is allowed (if not, small values are written in KB).
// if 'short_form' is true, returned string is shorter in length.
Cstr_w MakeFileSizeText(dword sz, bool short_form, bool allow_bytes);

//----------------------------
bool ScanHexByte(const char *&cp, dword &num);
bool ScanDecimalNumber(const char* &cp, int &num);
bool ScanHexNumber(const char* &cp, int &num);

inline bool ScanInt(const char *cp, int &num){
   return ScanDecimalNumber(cp, num);
}

//----------------------------
// Make 2-character hex string representation of byte. The result uses characters 0-9, A-F.
word MakeHexString(byte b, bool uppercase = true);
const char *MakeHexString(byte b, dword &tw_tmp, bool uppercase = true);

//----------------------------
// Get char from string pointer, and advance pointer. Single-byte and wide-char strings are possible.
inline wchar GetChar(const void *&str, bool wide){
   if(!wide)
      return *((const byte*&)str)++;
   return *((const wchar*&)str)++;
}

//----------------------------
// Decode encrypted static string created by _ macro.
Cstr_c DecodeCryptedString(const char *crypted);

//----------------------------
Cstr_c StringToBase64(const Cstr_c &s);
Cstr_c Base64ToString(const Cstr_c &s);

//----------------------------
bool IsDigit(wchar c);
bool IsXDigit(wchar c);
bool IsAlNum(wchar c);

//----------------------------
// Check if character is a white-space (space, tab, new-line).
bool IsSpace(wchar c);

//----------------------------
// Read string between quotes.
bool ReadQuotedString(const char *&cp, Cstr_c &str, char quote = '\"');

//----------------------------
// token = 1*<any CHAR except specials, SPACE and CTLs>
bool ReadToken(const char *&cp, Cstr_c &str, const char *specials);

//----------------------------
// word =  token | quoted-string
bool ReadWord(const char *&cp, Cstr_c &str, const char *specials);

//----------------------------
// Get attribute-value pair, separated by '='. Attribute is always converted to lower-case, value is converted optionally.
bool GetNextAtrrValuePair(const char *&cp, Cstr_c &attr, Cstr_c &val, bool val_to_lower = true);

//----------------------------
// Read date/time in rfc822 format, or in rfc850 format.
// rfc822 format: Sun, 06 Nov 1994 08:49:37 GMT
// rfc850 format: Sunday, 06-Nov-94 08:49:37 GMT
bool ReadDateTime_rfc822_rfc850(const char *&cp, S_date_time &date, bool use_rfc_850 = false, bool *missing_timezone = NULL);

//----------------------------
// Get filename's extension - part after '.'. If no extension is found, NULL is returned.
const wchar *GetExtension(const wchar *filename);

//----------------------------
// Find index of keyword 'str' in list of keywords (separated by \0). Comparing is case-insensitive.
// If match is found, str is advanced by the keyword length, and index is returned. Otherwise -1 is returned.
int FindKeyword(const char *&str, const char *keywords);

//----------------------------
// Change characters in 'str' to password-masking chars '*'. If num_letters is used, only given number of chars are changed.
void MaskPassword(Cstr_w &str, int num_letters = -1);

//----------------------------
// Find beginning of previous line, return as byt offset into 'base' string.
int FindPreviousLineBeginning(const wchar *base, int byte_offs);

//----------------------------
// Compare 2 strings case-insensitive. Return value is same as for StrCmp.
int CompareStringsNoCase(const char *s1, const char *s2);
int CompareStringsNoCase(const wchar *s1, const wchar *s2);

//----------------------------
// Convert wide-char into UTF-8 string. Return length of buf (not including terminating \0).
dword ConvertCharToUTF8(wchar c, byte buf[4]);

//----------------------------
// Decode single utf-8 character. Read variable amount of chars from input and advance pointer.
wchar DecodeUtf8Char(const char *&cp);

//----------------------------
// Skip leading white-space (' ', '\t') from string.
void SkipWS(const char *&cp);

//----------------------------
// Skip leading white-space and eol from string.
void SkipWSAndEol(const char *&cp);

//----------------------------
// Check if 'str' begins with 'kw'.
// May be case non-sensitive, in which case 'kw' must be in lowercase.
// If match is found and 'advance_str' is true, str is advanced to point to text after found text (of 'kw's lenght).
bool CheckStringBegin(const char *&str, const char *kw, bool ignore_case = true, bool advance_str = true);

//----------------------------
// Same as CheckStringBegin, but also calls SkipWS if match was found.
bool CheckStringBeginWs(const char *&str, const char *kw, bool ignore_case = true, bool advance_str = true);

extern const char specials_rfc_822[];

//----------------------------
// Make full from 'url'. Source may be local name, and/or optionally include http:// prefix.
// On return, url doesn't have prefix.
void MakeFullUrlName(const char *domain, Cstr_c &url);

//----------------------------
// Encode string by utf-8 for transmit over url.
Cstr_c UrlEncodeString(const char *cp);
Cstr_c UrlEncodeString(const wchar *cp);

}


#endif

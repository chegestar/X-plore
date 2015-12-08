//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <Lang.h>
namespace win{
#include <Windows.h>
}

//----------------------------
#undef LANG_CZECH
#undef LANG_SLOVAK
#undef LANG_POLISH
#undef LANG_RUSSIAN
#undef LANG_BULGARIAN
#undef LANG_GERMAN
#undef LANG_ITALIAN
#undef LANG_HUNGARIAN
#undef LANG_TURKISH
#undef LANG_SWEDISH
#undef LANG_FRENCH
#undef LANG_PORTUGUESE
#undef LANG_SPANISH
#undef LANG_DUTCH
#undef LANG_DANISH
#undef LANG_INDONESIAN
#undef LANG_NORWEGIAN
#undef LANG_FINNISH
#undef LANG_HEBREW
#undef LANG_ENGLISH
#undef LANG_JAPANESE
#undef LANG_CHINESE
#undef LANG_GREEK
#undef LANG_SERBIAN

#undef LANG_ICELANDIC
#undef LANG_SLOVENIAN
#undef LANG_ALBANIAN
#undef LANG_ARABIC
#undef LANG_BELARUSSIAN
#undef LANG_CROATIAN
#undef LANG_ESTONIAN
#undef LANG_KOREAN
#undef LANG_LATVIAN
#undef LANG_LITHUANIAN
#undef LANG_MACEDONIAN
#undef LANG_ROMANIAN
#undef LANG_UKRAINIAN
#undef LANG_VIETNAMESE

//----------------------------

E_LANGUAGE GetSystemLanguage(){

   win::LANGID id = win::GetSystemDefaultLangID();
   switch((win::WORD)(id)&0x3ff){
   case 0x05: return LANG_CZECH;
   case 0x1b: return LANG_SLOVAK;
   case 0x15: return LANG_POLISH;
   case 0x19: return LANG_RUSSIAN;
   case 0x02: return LANG_BULGARIAN;
   case 0x07: return LANG_GERMAN;
   case 0x10: return LANG_ITALIAN;
   case 0x0e: return LANG_HUNGARIAN;
   case 0x1f: return LANG_TURKISH;
   case 0x1d: return LANG_SWEDISH;
   case 0x0c: return LANG_FRENCH;
   case 0x16: return LANG_PORTUGUESE;
   case 0x0a: return LANG_SPANISH;
   case 0x13: return LANG_DUTCH;
   case 0x06: return LANG_DANISH;
   case 0x21: return LANG_INDONESIAN;
   case 0x14: return LANG_NORWEGIAN;
   case 0x0b: return LANG_FINNISH;
   case 0x0d: return LANG_HEBREW;
   case 0x11: return LANG_JAPANESE;
   case 0x04: return LANG_CHINESE;
   case 0x08: return LANG_GREEK;
   case 0x1a: return LANG_SERBIAN;
   case 0x0f: return LANG_ICELANDIC;
   case 0x24: return LANG_SLOVENIAN;
   case 0x1c: return LANG_ALBANIAN;
   case 0x01: return LANG_ARABIC;
   case 0x23: return LANG_BELARUSSIAN;
   //case 0x1a: return LANG_CROATIAN;
   case 0x25: return LANG_ESTONIAN;
   case 0x12: return LANG_KOREAN;
   case 0x26: return LANG_LATVIAN;
   case 0x27: return LANG_LITHUANIAN;
   case 0x2f: return LANG_MACEDONIAN;
   case 0x18: return LANG_ROMANIAN;
   case 0x22: return LANG_UKRAINIAN;
   case 0x2a: return LANG_VIETNAMESE;
   default: return LANG_ENGLISH;
   }
}

//----------------------------

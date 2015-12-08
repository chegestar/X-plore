//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <Lang.h>
#include <e32base.h>

//----------------------------

E_LANGUAGE GetSystemLanguage(){

   switch(User::Language()){
   case ELangCzech: return LANG_CZECH;
   case ELangSlovak: return LANG_SLOVAK;
   case ELangPolish: return LANG_POLISH;
   case ELangRussian: return LANG_RUSSIAN;
   case ELangBulgarian: return LANG_BULGARIAN;
   case ELangGerman:
   case ELangSwissGerman:
   case ELangAustrian:
      return LANG_GERMAN;
   case ELangItalian:
   case ELangSwissItalian:
      return LANG_ITALIAN;
   case ELangHungarian: return LANG_HUNGARIAN;
   case ELangTurkish: return LANG_TURKISH;
   case ELangSwedish: return LANG_SWEDISH;
   case ELangFrench:
   case ELangSwissFrench:
   case ELangBelgianFrench:
   case ELangInternationalFrench:
   case ELangCanadianFrench:
      return LANG_FRENCH;
   case ELangPortuguese:
   case ELangBrazilianPortuguese:
      return LANG_PORTUGUESE;
   case ELangSpanish:
   case ELangInternationalSpanish:
   case ELangLatinAmericanSpanish:
      return LANG_SPANISH;
   case ELangDutch:
   case ELangBelgianFlemish:
      return LANG_DUTCH;
   case ELangDanish: return LANG_DANISH;
   case ELangMalay:
   case ELangMalayalam:
   case ELangIndonesian:
      return LANG_INDONESIAN;
   case ELangNorwegian:
   case ELangNorwegianNynorsk:
      return LANG_NORWEGIAN;
   case ELangFinnish: return LANG_FINNISH;
   case ELangHebrew: return LANG_HEBREW;
   case ELangJapanese: return LANG_JAPANESE;
   case ELangPrcChinese:
   case ELangTaiwanChinese:
   case ELangHongKongChinese:
      return LANG_CHINESE;
   case ELangGreek: return LANG_GREEK;
   case ELangSerbian: return LANG_SERBIAN;
   case ELangIcelandic: return LANG_ICELANDIC;
   case ELangSlovenian: return LANG_SLOVENIAN;
   case ELangAlbanian: return LANG_ALBANIAN;
   case ELangArabic: return LANG_ARABIC;
   case ELangBelarussian: return LANG_BELARUSSIAN;
   case ELangCroatian: return LANG_CROATIAN;
   case ELangEstonian: return LANG_ESTONIAN;
   case ELangKorean: return LANG_KOREAN;
   case ELangLatvian: return LANG_LATVIAN;
   case ELangLithuanian: return LANG_LITHUANIAN;
   case ELangMacedonian: return LANG_MACEDONIAN;
   case ELangMongolian: return LANG_MONGOLIAN;
   case ELangRomanian: return LANG_ROMANIAN;
   case ELangUkrainian: return LANG_UKRAINIAN;
   case ELangVietnamese: return LANG_VIETNAMESE;

   default:
   case ELangEnglish:
   case ELangCanadianEnglish:
   case ELangInternationalEnglish:
   case ELangSouthAfricanEnglish:
      return LANG_ENGLISH;
   }
}

//----------------------------

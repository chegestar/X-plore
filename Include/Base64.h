//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#ifndef _BASE64_H_
#define _BASE64_H_

#include <C_vector.h>

//----------------------------
// Encode binary data to base-64 encoding.
// If 'fill_end_marks' is true, 'out' buffer is stuffed with '=' characters at end to align its size to multiply of 4.
void EncodeBase64(const byte *cp, int len, C_vector<char> &out, bool fill_end_marks = true);

//----------------------------
// Decode base-64 encoded buffer into binary data.
bool DecodeBase64(const char *cp, int len, C_vector<byte> &out);

//----------------------------
#endif

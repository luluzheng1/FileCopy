#ifndef __SHA1_H
#define __SHA1_H
#include <string>
using namespace std; // for C++ std library

void encodeSHA1(string dirname, string filename, unsigned char obuf[]);
string SHA1toHex(unsigned char *inputString);

#endif
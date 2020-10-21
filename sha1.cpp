#include "sha1.h"
#include <sstream>
#include <fstream>
#include <openssl/sha.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>

// Take in filename, and encrypt the content of the file using SHA1
void encodeSHA1(string dirname, string filename, unsigned char obuf[])
{
    ifstream *t; // SHA1 related variables
    stringstream *buffer;
    t = new ifstream(dirname + "/" + filename);
    buffer = new stringstream;
    *buffer << t->rdbuf();
    SHA1((const unsigned char *)buffer->str().c_str(),
         (buffer->str()).length(), obuf);

    delete t;
    delete buffer;
}

string SHA1toHex(unsigned char *SHA1Hash)
{
    stringstream hexString;

    for (int i = 0; i < 20; i++)
        hexString << setfill('0') << setw(2) << hex << int(SHA1Hash[i]);

    return hexString.str();
}
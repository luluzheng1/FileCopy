#include <string>
#include <sstream>
#include <fstream>
#include <openssl/sha.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>

using namespace std; // for C++ std library

void encodeSHA1(string filename, unsigned char obuf[]);
void printSHA1(unsigned char *received);
string SHA1toHex(unsigned char *inputString);

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

void printSHA1(unsigned char *received)
{
    // cout << "Received:" << endl;

    for (int i = 0; i < 20; i++)
    {
        printf("%02x", (unsigned int)received[i]);
    }
    cout << endl;
}

string SHA1toHex(unsigned char *SHA1Hash)
{
    stringstream hexString;

    for (int i = 0; i < 20; i++)
    {
        hexString << setfill('0') << setw(2) << hex << int(SHA1Hash[i]);
        // hexString << hex << int(SHA1Hash[i]);
    }

    return hexString.str();
}

// void hashSHA1(char *sf, sha1Map &SHA1map)
// {

//     while ((sourceFile = readdir(src)) != NULL)
//     {
//         // skip the . and .. names
//         if ((strcmp(sourceFile->d_name, ".") == 0) ||
//             (strcmp(sourceFile->d_name, "..") == 0))
//             continue; // never copy . or ..

//         encodeSHA1(sourceFile->d_name, buf);

//         unsigned char *p = new unsigned char[20];
//         strcpy((char *)p, (char *)buf);

//         SHA1map.insert({sourceFile->d_name, p});
//     }
//     closedir(src);
// }

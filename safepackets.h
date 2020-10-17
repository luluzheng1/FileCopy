#ifndef __SAFEPACKETS_H_INCLUDED__
#define __SAFEPACKETS_H_INCLUDED__

#include "c150nastyfile.h" // for c150nastyfile & framework
#include <list>
#include <unordered_map>
#include <string>
#include <vector>

using namespace std;
using namespace C150NETWORK;

typedef unordered_map<string, list<string>> sha1Map;
typedef C150NastyFile NASTYFILE;

class SafePackets
{
private:
    string mostCommonPkt();
    string generateHeader();
    size_t readFile(char *buffer);
    long getFileSize(string sourceName);
    string getSafePacket(char *buffer, int hashFreq);
    int setHashFreq(string filePath);
    void freeArray();

protected:
    sha1Map pktMap;
    vector<string> pktArray;
    int numPkts;
    string fileName;
    NASTYFILE inputFile;
    int arraySize;

public:
    SafePackets();
    SafePackets(int nastiness);
    void fileToPackets(string sourcename);

    ~SafePackets();
};

#endif
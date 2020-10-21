#ifndef __SAFEPACKETS_H_INCLUDED__
#define __SAFEPACKETS_H_INCLUDED__

#include "c150nastyfile.h"
#include <list>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <unordered_map>

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

protected:
    sha1Map pktMap;
    vector<string> pktArray;
    int numPkts;
    int nastiness;
    string fileName;
    NASTYFILE inputFile;

public:
    SafePackets();
    SafePackets(int nastiness);
    int getNumPkts();
    string getPkt(int index);
    void fileToPackets(string sourcename);
    void clear();

    ~SafePackets();
};

#endif
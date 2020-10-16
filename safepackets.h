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
    void addHeader(string &pkt, string header);
    string generateHeader();
    size_t readFile(char *buffer);

protected:
    sha1Map pktMap;
    vector<string> pktArray;
    int numPkts;
    string fileName;
    long offset;
    NASTYFILE inputFile;

public:
    SafePackets();
    SafePackets(int nastiness);
    void arrayInsert(string pkt);
    void fileToPackets(string sourcename);
    string getSafePacket(char *buffer);
    ~SafePackets();
};

#endif
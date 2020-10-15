#ifndef __SAFEPACKETS_H_INCLUDED__
#define __SAFEPACKETS_H_INCLUDED__

#include "c150nastyfile.h" // for c150nastyfile & framework
#include <list>
#include <unordered_map>
#include <string>
#include <vector>

using namespace std;
using namespace C150NETWORK;

typedef unordered_map<unsigned char *, list<string>> sha1Map;
typedef C150NastyFile NASTYFILE;

class SafePackets
{
private:
    void mostCommonPkt();

protected:
    sha1Map pktHash;
    vector<string> pktArray;
    int numPkts;
    string fileName;

public:
    NASTYFILE inputFile;
    SafePackets();
    SafePackets(int nastiness);
    string getSafePacket(char *buffer);
    void addHeader(string &pkt, string header);
    void arrayInsert(string pkt);
    void fileToPackets(string sourcename);
    string generateHeader();
};

#endif
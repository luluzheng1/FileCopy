#include "safepackets.h"
#include "sha1.cpp"
#include "iomanip"
#include <openssl/sha.h>

size_t BODYSIZE = 508;
size_t PKTSIZE = 512;
size_t HEADERSIZE = 4;
int HASHFREQ = 10;

SafePackets::SafePackets() : inputFile(0)
{
    numPkts = 0;
    fileName = "";
    offset = -BODYSIZE;
}

SafePackets::SafePackets(int nastiness) : inputFile(nastiness)
{
    numPkts = 0;
    fileName = "";
    offset = -BODYSIZE;
}

size_t SafePackets::readFile(char *buffer)
{
    size_t len = inputFile.fread(buffer, 1, BODYSIZE);
    buffer[BODYSIZE] = '\0';
    return len;
}

string SafePackets::getSafePacket(char *buffer)
{
    size_t len;
    unsigned char obuf[20];

    for (int i = 0; i < HASHFREQ; i++)
    {
        len = readFile(buffer);
        if (len == 0)
            return "-1";
        inputFile.fseek(-len, SEEK_CUR);
        SHA1((const unsigned char *)buffer, len, obuf);
        string key = SHA1toHex(obuf);
        string b(buffer);
        if (pktMap.count(key) == 0)
        {
            list<string> pktList;
            pktMap[key] = pktList;
        }
        pktMap[key].push_front(b);
    }

    inputFile.fseek(len, SEEK_CUR);

    return mostCommonPkt();
}

void SafePackets::fileToPackets(string sourceName)
{
    void *fopenretval;
    char buffer[508];
    string pkt;

    fopenretval = inputFile.fopen(sourceName.c_str(), "w+ ");

    if (fopenretval == NULL)
    {
        cerr << "Error opening input file " << sourceName << " errno=" << strerror(errno) << endl;
        exit(12);
    }

    while (inputFile.feof() == 0)
    {
        memset(buffer, 0, BODYSIZE);
        pkt = getSafePacket(buffer);
        if (pkt == "-1")
            break;
        numPkts++;
        string hd = generateHeader();
        addHeader(pkt, hd);
        cout << pkt << endl;
    }

    if (inputFile.fclose() != 0)
    {
        cerr << "Error closing input file " << sourceName << " errno=" << strerror(errno) << endl;
        exit(16);
    }
}

void SafePackets::addHeader(string &pkt, string header)
{
    pkt = header + pkt;
}

string SafePackets::generateHeader()
{
    stringstream stream;
    stream << setfill('0') << setw(4) << hex << numPkts;
    return stream.str();
}

string SafePackets::mostCommonPkt()
{
    unsigned currentMax = 0;
    string pkt;
    for (auto it : pktMap)
    {
        unsigned count = it.second.size();
        if (count > currentMax)
        {
            pkt = it.second.front();
            currentMax = count;
        }
    }
    pktMap.clear();
    return pkt;
}

SafePackets::~SafePackets() {}
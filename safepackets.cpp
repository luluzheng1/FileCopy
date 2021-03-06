#include "sha1.h"
#include "iomanip"
#include "safepackets.h"
#include <unistd.h>
#include <openssl/sha.h>

size_t BODYSIZE = 252;
size_t PKTSIZE = 256;
size_t HEADERSIZE = 4;

SafePackets::SafePackets() : inputFile(0)
{
    nastiness = 0;
    numPkts = 0;
    fileName = "";
}

SafePackets::SafePackets(int n) : inputFile(n)
{
    nastiness = n;
    numPkts = 0;
    fileName = "";
}

size_t SafePackets::readFile(char *buffer)
{
    size_t len = inputFile.fread(buffer, 1, BODYSIZE);
    buffer[BODYSIZE] = '\0';
    return len;
}

// Returns a safe packet (most commonly hashed)
string SafePackets::getSafePacket(char *buffer, int hashFreq)
{
    size_t len;
    unsigned char obuf[20];
    for (int i = 0; i < hashFreq; i++)
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

// Breaks a file into packets of size 256
void SafePackets::fileToPackets(string sourceName)
{
    void *fopenretval;
    char buffer[256];
    string pkt;

    fopenretval = inputFile.fopen(sourceName.c_str(), "rb");

    if (fopenretval == NULL)
    {
        cerr << "Error opening input file " << sourceName << " errno=" << strerror(errno) << endl;
        exit(12);
    }

    int hashFreq = setHashFreq(sourceName);
    while (inputFile.feof() == 0)
    {
        memset(buffer, 0, BODYSIZE);
        pkt = getSafePacket(buffer, hashFreq);
        if (pkt == "-1")
            break;

        // Add formatted packet to pktArray
        string header = generateHeader();
        numPkts++;
        pkt = header + pkt;
        pktArray.push_back(pkt);
    }

    if (inputFile.fclose() != 0)
    {
        cerr << "Error closing input file " << sourceName << " errno=" << strerror(errno) << endl;
        exit(16);
    }
}

// Returns header containing packet id in hex
string SafePackets::generateHeader()
{
    stringstream stream;
    stream << setfill('0') << setw(4) << hex << numPkts;
    return stream.str();
}

// Compute the most commonly hashed pkt in pktMap
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

int SafePackets::getNumPkts()
{
    return numPkts;
}

string SafePackets::getPkt(int index)
{
    return pktArray.at(index);
}

long SafePackets::getFileSize(string filePath)
{
    struct stat stat_buf;
    int rc = stat(filePath.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

// Set hash frequency based on filesize and file nastiness
int SafePackets::setHashFreq(string filePath)
{
    long size = getFileSize(filePath);

    if (nastiness == 0)
        return 1;
    else if (size > 1e6)
        return 30 + nastiness;
    else if (size > 1e4)
        return 15 + nastiness;
    else if (size > 1e3)
        return 10 + nastiness;
    else
        return 5;
}

void SafePackets::clear()
{
    pktArray.clear();
    numPkts = 0;
    return;
}

SafePackets::~SafePackets() {}
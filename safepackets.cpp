#include "safepackets.h"
#include "sha1.cpp"
#include "iomanip"
#include <openssl/sha.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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
        numPkts++;
        string header = generateHeader();
        pkt = header + pkt;
        pktArray.push_back(pkt);
    }

    if (inputFile.fclose() != 0)
    {
        cerr << "Error closing input file " << sourceName << " errno=" << strerror(errno) << endl;
        exit(16);
    }
    freeArray();
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

long SafePackets::getFileSize(string filePath)
{
    struct stat stat_buf;
    int rc = stat(filePath.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

int SafePackets::setHashFreq(string filePath)
{
    // TODO: justify based on nastiness
    long size = getFileSize(filePath);

    if (nastiness == 0)
        return 0;
    else if (size > 1e6)
        return 25;
    else if (size > 1e4)
        return 10;
    else if (size > 1e3)
        return 7;
    else
        return 5;
}

void SafePackets::freeArray()
{
    pktArray.clear();
    return;
}

SafePackets::~SafePackets() {}
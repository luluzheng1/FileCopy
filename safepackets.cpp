#include "safepackets.h"
#include "iomanip"
size_t BODYSIZE = 508;
size_t PKTSIZE = 512;
size_t HEADERSIZE = 4;

SafePackets::SafePackets() : inputFile(0)
{
    numPkts = 0;
    fileName = "";
}

SafePackets::SafePackets(int nastiness) : inputFile(nastiness)
{
    numPkts = 0;
    fileName = "";
}

string SafePackets::getSafePacket(char *buffer)
{
    inputFile.fread(buffer, 1, BODYSIZE);
    numPkts++;

    string b(buffer);
    return b;
}

void SafePackets::fileToPackets(string sourceName)
{
    void *fopenretval;
    char buffer[508];
    string pkt;

    fopenretval = inputFile.fopen(sourceName.c_str(), "rb");

    if (fopenretval == NULL)
    {
        cerr << "Error opening input file " << sourceName << " errno=" << strerror(errno) << endl;
        exit(12);
    }

    while (inputFile.feof() == 0)
    {
        memset(buffer, 0, BODYSIZE);
        pkt = getSafePacket(buffer);
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
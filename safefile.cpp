#include "safefile.h"
#include "c150nastyfile.h"

#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <algorithm>
#include <sys/stat.h>
#include <unordered_map>

using namespace std;         // for C++ std library
using namespace C150NETWORK; // for all the comp150 utilities

int CONTENT_SIZE = 252;

inline bool fileExists(string dir, string &name)
{
    struct stat buffer;
    return (stat((dir + "/" + name).c_str(), &buffer) == 0);
}

SafeFile::SafeFile(int n, string d) : outputFile(nastiness)
{
    numPackets = 0;
    nastiness = n;
    dirName = d;
    filename = "";
}

void SafeFile::setFile(int n, string f)
{
    numPackets = n;
    filename = f;
    packets.reserve(n);
}

void SafeFile::clearFile()
{
    numPackets = 0;
    filename = "";
    packets.clear();
    received.clear();
    missing.clear();
}

// Add a packet to packets array
void SafeFile::storePacket(string packet)
{
    int packetID = stoi(packet.substr(0, 4), 0, 16);
    string content = packet.substr(4);

    packets[packetID] = content;
    received.insert(packetID);
    removeMissing(packetID);
}

// Update the missing array
void SafeFile::computeMissing()
{

    for (int i = 0; i < numPackets; i++)
    {
        if (!received.count(i))
            missing.insert(i);
    }
    return;
}

// Remove packet from missing pkts array
void SafeFile::removeMissing(int packetID)
{
    if (missing.count(packetID))
        missing.erase(packetID);
}

unordered_set<int> SafeFile::getMissing()
{
    return missing;
}

// Set hash frequency based on filesize and file nastiness
int SafeFile::setHashFreq()
{
    if (nastiness == 0)
        return 0;
    else
    {
        long size = numPackets * CONTENT_SIZE;
        if (size > 1e6)
            return 14 + nastiness;
        else if (size > 1e5)
            return 9 + nastiness;
        else if (size > 1e4)
            return 7 + nastiness;
        else
            return 5 + nastiness;
    }
}

// Reconstruct file from packets hashmap
bool SafeFile::writeFile()
{
    string fn = dirName + "/" + filename + ".TMP";

    if (fileExists(dirName, filename))
        return false;

    outputFile.fopen(fn.c_str(), "wb+");

    int hashFreq = setHashFreq();

    for (int i = 0; i < numPackets; i++)
        writePacket(packets[i], hashFreq);

    outputFile.fclose();
    return true;
}

// Write one safe packet to file
void SafeFile::writePacket(string content, int hashFreq)
{
    int rewriteAttemps = 0;
    int packet_size = content.length();
    char buffer[packet_size + 1];
    void *fopenretval;

    string fn = dirName + "/" + filename + ".TMP";

    // If write fails, retry up to 10 times
    while (rewriteAttemps < 10)
    {
        // Hash map to guess what it wrote
        unordered_map<string, int> contentWritten;

        size_t size = outputFile.fwrite(content.c_str(), 1, packet_size);

        // No need to hash if nastiness is 0
        if (hashFreq == 0)
            break;

        // Do repeated reads at the location
        for (int i = 0; i < hashFreq; i++)
        {
            memset(buffer, 0, packet_size + 1); // Clear buffer

            if (outputFile.fclose() != 0)
            {
                cerr << "Error closing input file " << filename << " errno=" << strerror(errno) << endl;
                exit(16);
            }

            fopenretval = outputFile.fopen(fn.c_str(), "rb+");

            if (fopenretval == NULL)
            {
                cerr << "Error opening output file " << filename << " errno=" << strerror(errno) << endl;
                exit(12);
            }

            // Go to beginning of packet
            outputFile.fseek(-size, SEEK_END);
            outputFile.fread(buffer, sizeof(char), packet_size + 1);
            outputFile.fseek(-size, SEEK_END);

            buffer[packet_size] = '\0';
            string incoming(buffer);

            // If packet doesn't exist, add first instance
            if (contentWritten.find(incoming) == contentWritten.end())
                contentWritten.insert({{incoming, 1}});
            else
                contentWritten.at(incoming) += 1;
        }

        // Find most commonly hashed packet
        auto mostCommon = max_element(
            begin(contentWritten), end(contentWritten),
            [](const decltype(contentWritten)::value_type &p1, const decltype(contentWritten)::value_type &p2) {
                return p1.second < p2.second;
            });

        string likelyContent = mostCommon->first;

        // Move on if packet wrote correctly
        if (content.compare(likelyContent) == 0)
            break;
        else
            rewriteAttemps++;
    }

    // Set file pointer to EOF
    outputFile.fseek(0, SEEK_END);
}

SafeFile::~SafeFile() {}
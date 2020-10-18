#include "safefile.h"
#include "c150nastyfile.h"

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <unistd.h>

using namespace std;         // for C++ std library
using namespace C150NETWORK; // for all the comp150 utilities

int CONTENT_SIZE = 252;

SafeFile::SafeFile(int n, string d) : outputFile(nastiness)
{
    numPackets = 0;
    nastiness = n;
    dirName = d;
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
    if (!packets.empty())
        packets.clear();

    if (!received.empty())
        received.clear();

    if (!missing.empty())
        missing.clear();
}

int SafeFile::getNumPackets()
{
    return numPackets;
}

void SafeFile::storePacket(string packet)
{
    int packetID = stoi(packet.substr(0, 4), 0, 16);
    string content = packet.substr(4);
    pair<int, string> p = make_pair(packetID, content);

    packets.insert(packets.begin() + packetID - 1, p);
    // cout << "Inserting " << packetID << endl;
    received.insert(packetID);
}

void SafeFile::computeMissing()
{
    // cout << "Has " << received.size() << " packets" << endl;
    for (int i = 1; i < numPackets + 1; i++)
    {
        if (!received.count(i))
        {
            missing.insert(i);
            // cout << "Missing" << i << endl;
        }
    }

    return;
}

void SafeFile::removeMissing(int packetID)
{
    missing.erase(packetID);
}

unordered_set<int> SafeFile::getMissing()
{
    return missing;
}

bool SafeFile::isMissing()
{
    return !missing.empty();
}

int SafeFile::setHashFreq()
{
    if (nastiness == 0)
        return 0;
    else
    {
        long size = numPackets * CONTENT_SIZE;
        if (size > 1e6)
            return 3;
        else if (size > 1e6)
            return 3;
        else if (size > 1e6)
            return 3;
        else
            return 3;
    }
}

void SafeFile::writeFile()
{
    string fn = dirName + "/" + filename + ".TMP";

    outputFile.fopen(fn.c_str(), "w+");

    cout << "Wrie path: " << fn << endl;
    int hashFreq = setHashFreq();

    for (int i = 0; i < numPackets; i++)
    {
        // cout << "Before writing packet" << endl;
        writePacket(packets.at(i), hashFreq);
        // cout << "After writing packet" << endl;
    }

    outputFile.fclose();
}

void SafeFile::writePacket(packet packet, int hashFrequ)
{
    string content = packet.second;
    int packetID = packet.first;
    int rewriteAttemps = 0;
    int packet_size = content.length();
    (void)packetID;
    char buffer[packet_size + 1];

    string fn = dirName + "/" + filename + ".TMP";

    // cout << "Curr packet " << packetID << endl;
    // cout << "Curr packet content:" << content << endl
    //      << endl;

    // If write fails, retry up to 10 times
    while (rewriteAttemps < 10)
    {
        // Hash map to guess what it wrote
        unordered_map<string, int> contentWritten;

        // cout << "Writing packet" << endl;

        size_t size = outputFile.fwrite(content.c_str(), 1, packet_size);

        // cout << "Finished writing packet" << endl;

        if (hashFrequ == 0)
        {
            break;
        }

        // Do repeated read at the location
        for (int i = 0; i < hashFrequ; i++)
        {
            memset(buffer, 0, size + 1);

            outputFile.fclose();

            outputFile.fopen(fn.c_str(), "r+");

            outputFile.fseek(-size, SEEK_END);
            outputFile.fread(buffer, sizeof(char), packet_size);
            outputFile.fseek(-size, SEEK_END);

            buffer[size] = '\0';
            string incoming(buffer);

            // cout << "server: " << i + 1 << "th read" << endl;
            // cout << "server: read" << incoming << endl;

            if (contentWritten.find(incoming) == contentWritten.end())
            {
                contentWritten.insert({{incoming, 1}});
            }
            else
            {
                contentWritten.at(incoming) += 1;
            }
        }

        auto mostCommon = max_element(
            begin(contentWritten), end(contentWritten),
            [](const decltype(contentWritten)::value_type &p1, const decltype(contentWritten)::value_type &p2) {
                return p1.second < p2.second;
            });

        string likelyContent = mostCommon->first;

        if (content.compare(likelyContent) == 0)
        {
            // cout << "server: Wrote correct file" << endl;
            break;
        }
        else
        {
            rewriteAttemps++;
            // cout << "Didn't match" << endl;
            // cout << "Most likely wrote: " << likelyContent << endl << endl;
            // cout << "Actual content: " << content << endl;
        }
    }

    // Set the filepointe to the end of file
    outputFile.fseek(0, SEEK_END);
}

string SafeFile::likelyContent()
{
    return "";
}

vector<packet> SafeFile::getPackets()
{
    return packets;
}

string SafeFile::readTest()
{
    outputFile.fopen("TARGET/data1", "rb");
    char buffer[512];
    cout << outputFile.fread(buffer, 1, 512) << endl;
    string incoming(buffer);
    cout << incoming << endl;
    outputFile.fclose();

    return incoming;
}

SafeFile::~SafeFile() {}
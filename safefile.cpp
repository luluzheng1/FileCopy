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

SafeFile::SafeFile(int nastiness):outputFile(nastiness) {
    numPackets = 0;
    this->nastiness = nastiness;
}

void SafeFile::setNumPackets(int numPackets) {
    this->numPackets = numPackets;
    packets.reserve(numPackets);
}

int SafeFile::getNumPackets() {
    return numPackets;
}

void SafeFile::storePacket(packet packet) {
    packets.insert(packets.begin() + packet.first - 1, packet);
    received.insert(packet.first);
}

void SafeFile::computeMissing() {
    for (int i = 1; i < numPackets + 1; i++) {
        if (!received.count(i)) {
            missing.insert(i);
        }
    }

    return;
}

void SafeFile::removeMissing(int packetID) {
    missing.erase(packetID);
}

unordered_set<int> SafeFile::getMissing() {
    return missing;
}

int SafeFile::setHashFreq() {
    if (nastiness == 0) {
        return 0;
    } else {
        long size = numPackets * CONTENT_SIZE;
        if (size > 1e6) {
            return 3;
        } else if (size > 1e6) {
            return 3;
        } else if (size > 1e6) {
            return 3;
        } else {
            return 3;
        }
    }
}

void SafeFile::writeFile(string filename) {
    outputFile.fopen(filename.c_str(), "w+");
    int hashFrequ = setHashFreq();

    for (int i = 0; i < numPackets; i++) {
        writePacket(packets.at(i), filename, hashFrequ);
    }

    outputFile.fclose();
}

void SafeFile::writePacket(packet packet, string filename, int hashFrequ) {
    string content = packet.second;
    int packetID = packet.first;
    int rewriteAttemps = 0;
    int packet_size = content.length();
    (void) packetID;
    char buffer[packet_size + 1];
    
    // cout << packetID << endl;
    // cout << Content << endl << endl;;

    // If write fails, retry up to 10 times
    while (rewriteAttemps < 10) {
        // Hash map to guess what it wrote
        unordered_map<string, int> contentWritten;

        size_t size = outputFile.fwrite(content.c_str(), 1, packet_size);

        if (hashFrequ == 0) {
            break;
        }

        // Do repeated read at the location
        for (int i = 0; i < hashFrequ; i++) {
            memset(buffer, 0, size + 1);

            outputFile.fclose();
            outputFile.fopen(filename.c_str(), "r+");

            outputFile.fseek(-size, SEEK_END);
            outputFile.fread(buffer, sizeof(char), packet_size);
            outputFile.fseek(-size, SEEK_END);

            buffer[size] = '\0';
            string incoming(buffer);

            // cout << i + 1 << "th read" << endl;
            // cout << incoming << endl << endl;

            if (contentWritten.find(incoming) == contentWritten.end()) {
                contentWritten.insert({{incoming, 1}});
            } else {
                contentWritten.at(incoming) += 1;
            }
        }
 
        auto mostCommon = max_element
        (
            begin(contentWritten), end(contentWritten),
            [] (const decltype(contentWritten)::value_type & p1, const decltype(contentWritten)::value_type & p2) {
                return p1.second < p2.second;
            }
        );

        string likelyContent = mostCommon->first;

        if (content.compare(likelyContent) == 0) {
            break;    
        } else {
            rewriteAttemps++;
            // cout << "Didn't match" << endl;
            // cout << "Most likely wrote: " << likelyContent << endl << endl;
            // cout << "Actual content: " << content << endl;
        }
    }
    
    // Set the filepointe to the end of file
    outputFile.fseek(0, SEEK_END);
}

string SafeFile::likelyContent() {
    return "";
}

vector<packet> SafeFile::getPackets () {
    return packets;
}

string SafeFile::readTest() {
    outputFile.fopen("TARGET/data1", "rb");
    char buffer[512];
    cout << outputFile.fread(buffer, 1, 512) << endl;
    string incoming(buffer);
    cout << incoming << endl;
    outputFile.fclose();

    return incoming;
}
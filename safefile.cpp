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

int CONTENT_SIZE = 508;
// int CONTENT_SIZE = 12;

SafeFile::SafeFile(int nastiness):outputFile(nastiness) {
    numPackets = 0;
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

unordered_set<int> SafeFile::getMissing() {
    return missing;
}

void SafeFile::writeFile(string filename) {
    outputFile.fopen(filename.c_str(), "w+");
    for (int i = 0; i < numPackets; i++) {
        writePacket(packets.at(i), filename);
    }

    outputFile.fclose();
}

void SafeFile::writePacket(packet packet, string filename) {
    int packetID = packet.first;
    (void) packetID;
    string content = packet.second;
    int packet_size = content.length();
    int rewriteAttemps = 0;
    unordered_map<string, int> contentWritten;

    // cout << packetID << endl;
    // cout << content << endl << endl;;

    while (rewriteAttemps < 10) {
        // outputFile.fseek((packetID - 1) * CONTENT_SIZE, SEEK_SET);

        size_t size = outputFile.fwrite(content.c_str(), 1, packet_size);

        char buffer[size + 1];

        for (int i = 0; i < 5; i++) {

            outputFile.fclose();
            outputFile.fopen(filename.c_str(), "r+");

            // outputFile.fseek((packetID - 1) * CONTENT_SIZE, SEEK_SET);
            outputFile.fseek(-size, SEEK_END);
            
            // size_t size = outputFile.fread(buffer, sizeof(char), packet_size);
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
            // cout << "Matched!" << endl;
            break;    
        } else {
            rewriteAttemps++;
            cout << "Didn't match" << endl;
            cout << "Most likely wrote: " << likelyContent << endl << endl;
            cout << "Actual content: " << content << endl;
        }
    }
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
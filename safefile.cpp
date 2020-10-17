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

void SafeFile::storePacket(packet packet) {
    packets.push_back(packet);
    received.insert(packet.first);
    numPackets += 1;
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
    // for (int i = 0; i < numPackets; i++) {
    //     writePacket(packets.at(i), filename);
    // }

    writePacket(packets.at(i), filename);
    outputFile.fclose();
}

void SafeFile::writePacket(packet packet, string filename) {
    int packetID = packet.first;
    (void) packetID;
    string content = packet.second;
    int packet_size = content.length();

    // char buffer[packet_size + 1];
    unordered_map<string, int> contentWritten;

    // cout << packetID << endl;
    // cout << content << endl << endl;;

    while (1) {
        // outputFile.fseek((packetID - 1) * CONTENT_SIZE, SEEK_SET);
        // outputFile.ftell();
        outputFile.fseek(0, SEEK_END);
        size_t size = outputFile.fwrite(content.c_str(), sizeof(char), packet_size);
        // sleep(1);

        for (int i = 0; i < 10; i++) {
            char buffer[size + 1];

            outputFile.fseek(outputFile.ftell() - size, SEEK_SET);
            size_t size = outputFile.fread(buffer, sizeof(char), size);

            // outputFile.fseek((packetID - 1) * CONTENT_SIZE, SEEK_SET);
            // size_t size = outputFile.fread(buffer, sizeof(char), packet_size);

            buffer[size] = '\0';
            string incoming(buffer);

            if (packetID == 3173) {
                cout << i + 1 << "th read" << endl;
                cout << printf("%s", buffer) << endl << endl;
                // cout << incoming << endl << endl;
            }

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

        // cout << "Most likely wrote: " << likelyContent << endl;
        // cout << "Actual content: " << content << endl;
        break;

        if (content.compare(likelyContent) == 0) {
            break;    
        } else {
            cout << "didn't match" << endl;
        }
    }
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
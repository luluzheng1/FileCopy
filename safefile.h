#ifndef __SAFEFILE_H
#define __SAFEFILE_H

#include "c150nastyfile.h" 
#include <utility>   
#include <unordered_set>
#include <queue>
#include <vector>

using namespace std;
using namespace C150NETWORK; // for all the comp150 utilities

typedef pair<int, string> packet;

class SafeFile {
  private:
    vector<packet> packets;
    unordered_set<int> received;
    unordered_set<int> missing;
    int numPackets;
    NASTYFILE outputFile;
    string likelyContent();
  
  public:
    SafeFile(int nastiness);
    void setNumPackets(int input);
    int getNumPackets();
    void storePacket(packet packet);
    void computeMissing();
    unordered_set<int> getMissing();
    void writeFile(string filename);
    void writePacket(packet packet, string filename);
    vector<packet> getPackets();
    string readTest();
    
};

#endif
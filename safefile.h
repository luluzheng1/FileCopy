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

class SafeFile
{
private:
  vector<packet> packets;
  unordered_set<int> received;
  unordered_set<int> missing;
  int numPackets;
  string dirName;
  string filename;
  NASTYFILE outputFile;
  string likelyContent();

public:
  SafeFile(int n, string d);
  int nastiness;
  void setFile(int numPackets, string filename);
  void clearFile();
  void resetFile();
  int getNumPackets();
  void storePacket(string packet);
  void computeMissing();
  void removeMissing(int packetID);
  unordered_set<int> getMissing();
  int setHashFreq();
  void writeFile();
  bool isMissing();
  void writePacket(packet packet, int hashFrequ);
  vector<packet> getPackets();
  string readTest();
  ~SafeFile();
};

#endif
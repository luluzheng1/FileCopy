#ifndef __SAFEFILE_H
#define __SAFEFILE_H

#include "c150nastyfile.h"
#include <utility>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <vector>

using namespace std;
using namespace C150NETWORK; // for all the comp150 utilities

class SafeFile
{
private:
  unordered_map<int, string> packets;
  unordered_set<int> received;
  unordered_set<int> missing;
  int numPackets;
  string dirName;
  string filename;
  NASTYFILE outputFile;
  int nastiness;
  string likelyContent();

public:
  SafeFile(int n, string d);
  void setFile(int numPackets, string filename);
  void clearFile();
  void resetFile();
  int getNumPackets();
  void storePacket(string packet);
  void computeMissing();
  void removeMissing(int packetID);
  unordered_set<int> getMissing();
  int setHashFreq();
  bool writeFile();
  bool isMissing();
  void writePacket(string content, int packetID, int hashFrequ);
  unordered_map<int, string> getPackets();
  string readTest();
  ~SafeFile();
};

#endif
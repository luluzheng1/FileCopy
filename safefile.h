#ifndef __SAFEFILE_H
#define __SAFEFILE_H

#include "c150nastyfile.h"
#include <utility>
#include <queue>
#include <vector>
#include <unordered_set>
#include <unordered_map>

using namespace std;
using namespace C150NETWORK;

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
  void removeMissing(int packetID);
  void writePacket(string content, int hashFrequ);

public:
  SafeFile(int n, string d);
  void setFile(int numPackets, string filename);
  void clearFile();
  bool writeFile();
  void storePacket(string packet);
  void computeMissing();
  unordered_set<int> getMissing();
  int setHashFreq();
  ~SafeFile();
};

#endif
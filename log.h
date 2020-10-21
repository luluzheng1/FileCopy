#ifndef __LOG_H
#define __LOG_H
#include <string>
#include "c150nastydgmsocket.h"
using namespace std; // for C++ std library

void toLogClient(string logType, string filename, string status, int attempts);
void toLogServer(string logType, string filename, string status);
#endif
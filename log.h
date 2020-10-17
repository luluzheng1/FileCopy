#ifndef __LOG_H
#define __LOG_H
#include <string>
#include "c150nastydgmsocket.h"
using namespace std; // for C++ std library

void toLogClient(string filename, string status, int attempts);
void toLogServer(string filename, string status);
void checkAndPrintMessage(ssize_t readlen, char *buf, ssize_t bufferlen);
void setUpDebugLogging(const char *logname, int argc, char *argv[]);
#endif
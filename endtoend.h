#ifndef __ENDTOEND_H
#define __ENDTOEND_H

#include "c150dgmsocket.h"
#include "log.h"
using namespace std;         // for C++ std library
using namespace C150NETWORK; // for all the comp150 utilities

void performEndToEnd(char *sDir, string servername, C150DgmSocket *sock);
void tryFiveTimes(C150DgmSocket *sock, string outgoingMessage, char *incomingMessage, string serverName);
void checkDirectory(char *dirname);

#endif
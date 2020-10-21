#ifndef __ENDTOEND_H
#define __ENDTOEND_H

#include "log.h"
#include "sha1.h"
#include "c150dgmsocket.h"

using namespace std;         // for C++ std library
using namespace C150NETWORK; // for all the comp150 utilities

void performEndToEnd(string sDir, C150DgmSocket *sock, string filename, string clientSHA1Hash, string *status);
void sendManyTimes(C150DgmSocket *sock, string outgoingMessage, char *incomingMessage);
void checkDirectory(char *dirname);

#endif
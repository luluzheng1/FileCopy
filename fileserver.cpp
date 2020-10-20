#include "c150nastydgmsocket.h"
#include "c150debug.h"
#include "c150grading.h"
#include "safefile.h"
#include "endtoend.h"
#include <fstream>
#include <cstdlib>
#include <string>
#include <sstream>
#include <stdio.h>
#include <chrono>
#include <thread>
#include <openssl/sha.h>

using namespace C150NETWORK; // for all the comp150 utilities

void setUpDebugLogging(const char *logname, int argc, char *argv[]);
void interpretEnd(string incomingMessage, int *numPackets, string *filename);
void interpretBegin(string incomingMessage, int *numPackets, string *filename, string *sha1);
string createMsg(string msgType, int numPkts, string fileName);

string REQ = "REQ";
string DONE = "DONE";
string ALL = "ALL/";

int main(int argc, char *argv[])
{
    GRADEME(argc, argv);

    ssize_t readlen;           // amount of data read from socket
    char incomingMessage[257]; // received message data
    string filename, header, message, SHA1Hash, targetDir;
    int numPackets, networkNastiness, fileNastiness;

    if (argc != 4)
    {
        fprintf(stderr, "Correct syntxt is: %s <networknastiness> <filenastiness> <targetdir>\n", argv[0]);
        exit(1);
    }
    if (strspn(argv[1], "0123456789") != strlen(argv[1]))
    {
        fprintf(stderr, "Nastiness %s is not numeric\n", argv[1]);
        fprintf(stderr, "Correct syntxt is: %s <nastiness_number>\n", argv[0]);
        exit(4);
    }
    networkNastiness = atoi(argv[1]); // convert command line string to integer
    fileNastiness = atoi(argv[2]);
    targetDir = argv[3];
    SafeFile safe(fileNastiness, targetDir);

    setUpDebugLogging("serverdebug.txt", argc, argv);

    c150debug->setIndent("    ");

    try
    {
        c150debug->printf(C150APPLICATION, "Creating C150NastyDgmSocket(nastiness=%d)",
                          networkNastiness);
        C150DgmSocket *sock = new C150NastyDgmSocket(networkNastiness);

        // infinite loop processing messages
        while (1)
        {
            // Read a packet
            readlen = sock->read(incomingMessage, sizeof(incomingMessage) - 1);
            if (readlen == 0)
            {
                c150debug->printf(C150APPLICATION, "Read zero length message, trying again");
                continue;
            }

            // Clean up the message in case it contained junk
            incomingMessage[readlen] = '\0'; // make sure null terminated
            string incoming(incomingMessage);

            c150debug->printf(C150APPLICATION, "Successfully read %d bytes. Message=\"%s\"", readlen, incoming.c_str());

            // Detect message type
            header = incoming.substr(0, 4);

            if (header.compare("BEG/") == 0)
            {
                interpretBegin(incoming, &numPackets, &filename, &SHA1Hash);
                cout << filename << ": received BEGIN" << endl;
                safe.setFile(numPackets, filename);
                safe.setHashFreq();
            }
            else if (header.compare("END/") == 0)
            {
                interpretEnd(incoming, &numPackets, &filename);
                cout << filename << ": received END" << endl;
                safe.computeMissing();
                unordered_set<int> missingIDs = safe.getMissing();
                cout << filename << ": missing " << missingIDs.size() << " packets!" << endl;
                if (missingIDs.size() > 0)
                {
                    for (const auto &id : missingIDs)
                    {
                        message = createMsg(REQ, id, filename);
                        sock->write(message.c_str(), message.length());
                    }

                    message = createMsg(DONE, 0, filename);
                    sock->write(message.c_str(), message.length());
                    sock->write(message.c_str(), message.length());
                    sock->write(message.c_str(), message.length());
                }

                // this_thread::sleep_for(chrono::milliseconds(5));

                // Performs end-to-end check once server receives last packet
                else
                {
                    safe.writeFile();
                    message = createMsg(ALL, 0, filename);
                    sock->write(message.c_str(), message.length());
                    sock->write(message.c_str(), message.length());
                    sock->write(message.c_str(), message.length());

                    cout << filename << ": END TO END CHECK" << endl;
                    performEndToEnd(targetDir, sock, filename, SHA1Hash);
                    safe.clearFile();
                }
            }
            else
            {
                safe.storePacket(incoming);
            }
        }
    }

    catch (C150NetworkException &e)
    {
        // Write to debug log
        c150debug->printf(C150ALWAYSLOG, "Caught C150NetworkException: %s\n",
                          e.formattedExplanation().c_str());
        // In case we're logging to a file, write to the console too
        cerr << argv[0] << ": caught C150NetworkException: " << e.formattedExplanation() << endl;
    }

    // This only executes if there was an error caught above
    return 4;
}

void interpretEnd(string incomingMessage, int *numPackets, string *filename)
{
    incomingMessage.erase(0, 4);
    int pos = incomingMessage.find("/");
    *numPackets = stoi(incomingMessage.substr(0, pos + 1));
    incomingMessage.erase(0, pos + 1);
    pos = incomingMessage.find("/");
    *filename = incomingMessage.substr(0, pos);
}

void interpretBegin(string incomingMessage, int *numPackets, string *filename, string *sha1)
{
    incomingMessage.erase(0, 4);
    int pos = incomingMessage.find("/");
    *numPackets = stoi(incomingMessage.substr(0, pos + 1));
    incomingMessage.erase(0, pos + 1);
    pos = incomingMessage.find("/");
    *filename = incomingMessage.substr(0, pos);
    incomingMessage.erase(0, pos + 1);
    *sha1 = incomingMessage;
}

string createMsg(string msgType, int numPkts, string fileName)
{
    if (msgType == REQ)
    {
        return msgType + "/" + fileName + "/" + to_string(numPkts);
    }
    else if (msgType == DONE)
    {
        return msgType + "/" + fileName;
    }
    else if (msgType == ALL)
    {
        return msgType + fileName;
    }
    return "-1";
}
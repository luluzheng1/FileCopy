#include "safefile.h"
#include "endtoend.h"
#include "c150grading.h"
#include "c150nastydgmsocket.h"
#include <fstream>
#include <cstdlib>
#include <string>
#include <sstream>
#include <stdio.h>
#include <chrono>
#include <thread>
#include <openssl/sha.h>

using namespace C150NETWORK; // for all the comp150 utilities

string createMsg(string msgType, int numPkts, string fileName);
void interpretEnd(string incomingMessage, int *numPackets, string *filename);
void interpretBegin(string incomingMessage, int *numPackets, string *filename, string *sha1);

const string REQ = "REQ";
const string DONE = "DONE";
const string ALL = "ALL/";
const string BEGIN = "BEG";
const string END = "END";

int main(int argc, char *argv[])
{
    GRADEME(argc, argv);

    ssize_t readlen;           // amount of data read from socket
    char incomingMessage[257]; // received message data
    string filename, header, message, SHA1Hash, targetDir, status;
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
    SafeFile safe(fileNastiness, targetDir); // Create instance of safeFile

    try
    {
        C150DgmSocket *sock = new C150NastyDgmSocket(networkNastiness);

        // Infinite loop processing messages
        while (1)
        {
            // Read a packet
            readlen = sock->read(incomingMessage, sizeof(incomingMessage) - 1);
            if (readlen == 0)
            {
                continue;
            }

            // Clean up the message in case it contained junk
            incomingMessage[readlen] = '\0';
            string incoming(incomingMessage);

            // Detect message type
            header = incoming.substr(0, 4);

            // Client sent BEGIN message
            if (header.compare("BEG/") == 0)
            {
                toLogServer(BEGIN, filename, "");
                interpretBegin(incoming, &numPackets, &filename, &SHA1Hash);
                cout << filename << ": received BEGIN" << endl;
                safe.setFile(numPackets, filename);
                safe.setHashFreq();
            }
            // Client sent END message
            else if (header.compare("END/") == 0)
            {
                interpretEnd(incoming, &numPackets, &filename);
                cout << filename << ": received END" << endl;
                safe.computeMissing();
                unordered_set<int> missingIDs = safe.getMissing();
                cout << filename << ": missing " << missingIDs.size() << " packets!" << endl;

                // Send resend requests for missing packets
                if (missingIDs.size() > 0)
                {
                    for (const auto &id : missingIDs)
                    {
                        message = createMsg(REQ, id, filename);
                        sock->write(message.c_str(), message.length());
                    }

                    message = createMsg(DONE, 0, filename);
                    for (int i = 0; i < 2; i++)
                    {
                        sock->write(message.c_str(), message.length());
                    }
                }
                // Perform end-to-end check once server receives last packet
                else
                {
                    // Only write to file once, any subsequent attempts will be ignored
                    if (safe.writeFile())
                    {
                        message = createMsg(ALL, 0, filename);
                        for (int i = 0; i < 4; i++)
                        {
                            sock->write(message.c_str(), message.length());
                        }
                        cout << filename << ": END TO END CHECK" << endl;
                        toLogServer(END, filename, "");
                        performEndToEnd(targetDir, sock, filename, SHA1Hash, &status);
                        safe.clearFile();
                    }
                    else
                    {
                        performEndToEnd(targetDir, sock, filename, SHA1Hash, &status);
                        continue;
                    }
                }
            }
            // Client sent a packet
            else
            {
                safe.storePacket(incoming);
            }
        }
    }
    catch (C150NetworkException &e)
    {
        cerr << argv[0] << ": caught C150NetworkException: " << e.formattedExplanation() << endl;
    }

    // This only executes if there was an error caught above
    return 4;
}

// Extract info from an END message
void interpretEnd(string incomingMessage, int *numPackets, string *filename)
{
    incomingMessage.erase(0, 4);
    int pos = incomingMessage.find("/");
    *numPackets = stoi(incomingMessage.substr(0, pos + 1));
    incomingMessage.erase(0, pos + 1);
    pos = incomingMessage.find("/");
    *filename = incomingMessage.substr(0, pos);
}

// Extract info from a BEGIN message
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

// Format packet based on message type
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
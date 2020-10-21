#include "endtoend.h"
#include "safepackets.h"
#include "c150nastydgmsocket.h"
#include <stdio.h>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <dirent.h>
#include <stdlib.h>
#include <unordered_map>

using namespace std;         // for C++ std library
using namespace C150NETWORK; // for all the comp150 utilities

const string BEGIN = "BEG";
const string END = "END";
const string CHECK = "CHECK";
unsigned char obuf[20];

// forward declarations
string createMsg(string msgType, int numPkts, string fileName, char *sourceDir);
void interpretReq(string incomingReq, int *packetID, string *filename);
void interpretEnd(string incomingReq, string *filename);

const int serverArg = 1;           // server name is 1st arg
const int networkNastinessArg = 2; // network nastiness is 2nd arg
const int fileNastinessArg = 3;    // file nastiness is 3rd arg
const int sourceDirArg = 4;        // source directory is 4th arg

int main(int argc, char *argv[])
{
    GRADEME(argc, argv);

    DIR *src;
    ssize_t readlen; // amount of data read from socket
    char *sourceDir;
    char incomingReq[300];  // buffer for server request
    char incomingStatus[4]; // buffer for pkt type
    struct dirent *sourceFile;
    string filename, serverFilename, status;
    int networkNastiness, fileNastiness, packetID, attempts = 1;

    // Make sure command line looks right
    if (argc != 5)
    {
        fprintf(stderr, "Correct syntxt is: %s <servername> <networknastiness> <filenastiness> <srcdir>\n", argv[0]);
        exit(1);
    }

    networkNastiness = atoi(argv[networkNastinessArg]);
    fileNastiness = atoi(argv[fileNastinessArg]);
    sourceDir = argv[sourceDirArg];

    checkDirectory(sourceDir);

    src = opendir(sourceDir);
    if (src == NULL)
    {
        fprintf(stderr, "Error opening source directory %s\n", sourceDir);
        exit(8);
    }

    try
    {
        // Create the socket
        C150DgmSocket *sock = new C150NastyDgmSocket(networkNastiness);
        sock->turnOnTimeouts(5000);

        // Tell the DGMSocket which server to talk to
        sock->setServerName(argv[serverArg]);
        string serverName = argv[serverArg];

        // Declare instance of safepackets
        SafePackets safe(fileNastiness);

        // Send all files one by one
        while ((sourceFile = readdir(src)) != NULL)
        {
            attempts = 1;
            // Skip nested subdirectories
            if ((strcmp(sourceFile->d_name, ".") == 0) ||
                (strcmp(sourceFile->d_name, "..") == 0))
                continue; // never copy . or ..

            while (1)
            {
                filename = sourceFile->d_name;
                string sDir(sourceDir);
                string pkt, msg;
                int numPkts, count = 0;

                // Split file into packets and store in array
                safe.fileToPackets(sDir + "/" + filename);
                numPkts = safe.getNumPkts();

                // Send BEGIN message
                msg = createMsg(BEGIN, numPkts, filename, sourceDir);
                cout << filename << ": BEGIN transmission" << endl;
                toLogClient(BEGIN, filename, "", attempts);
                for (int i = 0; i < 10; i++)
                {
                    sock->write(msg.c_str(), msg.length());
                }

                // Send all packets
                for (int i = 0; i < numPkts; i++)
                {
                    pkt = safe.getPkt(i);
                    sock->write(pkt.c_str(), pkt.length());

                    // Delay so not to overload server
                    if (i % 100 == 0)
                        this_thread::sleep_for(chrono::milliseconds(5));
                }

                // Send END message
                msg = createMsg(END, numPkts, filename, sourceDir);
                for (int i = 0; i < 10; i++)
                {
                    sock->write(msg.c_str(), msg.length());
                    this_thread::sleep_for(chrono::milliseconds(1));
                }
                cout << filename << ": END transmission" << endl;
                toLogClient(END, filename, "", attempts);

                while (1)
                {
                    readlen = sock->read(incomingReq, 300);
                    incomingReq[readlen] = '\0';
                    if (readlen != 0 and sock->timedout() == 0)
                    {
                        string incoming(incomingReq);
                        // Extract header
                        status = incoming.substr(0, 4);

                        // Server requests resend of missing packet
                        if (status.compare("REQ/") == 0)
                        {
                            cout << "REQ" << endl;
                            interpretReq(incoming, &packetID, &serverFilename);
                            pkt = safe.getPkt(packetID);
                            sock->write(pkt.c_str(), pkt.length());
                            sock->write(pkt.c_str(), pkt.length());
                        }
                        // Server finishes one round of requests
                        else if (status.compare("DONE") == 0)
                        {
                            interpretEnd(incoming, &serverFilename);
                            cout << serverFilename << "DONE" << endl;
                            msg = createMsg(END, numPkts, filename, sourceDir);
                            for (int i = 0; i < 10; i++)
                            {
                                sock->write(msg.c_str(), msg.length());
                                this_thread::sleep_for(chrono::milliseconds(1));
                            }
                        }
                        // Server received all packets
                        else if (status.compare("ALL/") == 0)
                        {
                            interpretEnd(incoming, &serverFilename);
                            // Move on only if filename matches on both ends
                            if (filename.compare(serverFilename) == 0)
                            {
                                cout << filename << "ALL" << endl;
                                break;
                            }
                        }
                    }
                    // If server freezes for a long time
                    if (count == 70)
                    {
                        // Send all packets again
                        for (int i = 0; i < numPkts; i++)
                        {
                            pkt = safe.getPkt(i);
                            sock->write(pkt.c_str(), pkt.length());
                            if (i % 100 == 0)
                                this_thread::sleep_for(chrono::milliseconds(5));
                        }

                        // Send END packet
                        msg = createMsg(END, numPkts, filename, sourceDir);
                        for (int i = 0; i < 10; i++)
                        {
                            sock->write(msg.c_str(), msg.length());
                            this_thread::sleep_for(chrono::milliseconds(1));
                        }

                        count = 0;
                    }
                    cout << count << endl;
                    count++;
                }
                // Wait for end-to-end status from server
                while (1)
                {
                    readlen = sock->read(incomingStatus, 4);
                    incomingStatus[4] = '\0';
                    if (readlen != 0 and sock->timedout() == 0)
                    {
                        cout << filename << ": ACK/" << incomingStatus << endl;
                        string ack = "ACK/";
                        printf("%s\n", incomingStatus);
                        // Successful copy
                        if (strcmp(incomingStatus, "succ") == 0)
                        {
                            cout << "succ" << endl;
                            // Send acknowledgment
                            for (int i = 0; i < 20; i++)
                            {
                                sock->write(ack.c_str(), ack.length());
                            }
                            toLogClient(CHECK, filename, "succeeded", attempts);
                            break;
                        }
                        // Unsuccessful copy
                        else if (strcmp(incomingStatus, "fail") == 0)
                        {
                            cout << "fail" << endl;
                            for (int i = 0; i < 20; i++)
                            {
                                sock->write(ack.c_str(), ack.length());
                            }
                            toLogClient(CHECK, filename, "failed", attempts);
                            break;
                        }
                    }
                }
                // Clear file information
                safe.clear();

                // Send next file if copy was successful
                if (strcmp(incomingStatus, "succ") == 0)
                {
                    break;
                }
                attempts++;
            }
        }

        closedir(src);
    }
    //  Handle networking errors -- for now, just print message and give up!
    catch (C150NetworkException &e)
    {
        // In case we're logging to a file, write to the console too
        cerr << argv[0] << ": caught C150NetworkException: " << e.formattedExplanation() << endl;
    }

    return 0;
}

// Format packet based on message type
string createMsg(string msgType, int numPkts, string fileName, char *sourceDir)
{
    if (msgType == BEGIN)
    {
        encodeSHA1(sourceDir, fileName, obuf);
        string sha1 = SHA1toHex(obuf);
        return msgType + "/" + to_string(numPkts) + "/" + fileName + "/" + sha1;
    }
    else if (msgType == END)
    {
        return msgType + "/" + to_string(numPkts) + "/" + fileName;
    }
    return "-1";
}

// Extract information from server's request packet
void interpretReq(string incomingReq, int *packetID, string *filename)
{
    incomingReq.erase(0, 4);
    int pos = incomingReq.find("/");
    *filename = incomingReq.substr(0, pos + 1);
    incomingReq.erase(0, pos + 1);
    *packetID = stoi(incomingReq);
}

// Extract information from server's end packet
void interpretEnd(string incomingReq, string *filename)
{
    incomingReq.erase(0, 4);
    *filename = incomingReq;
}
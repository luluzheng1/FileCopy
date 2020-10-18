#include "c150nastydgmsocket.h"
#include "endtoend.h"
#include "safepackets.h"
#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <dirent.h>

using namespace std;         // for C++ std library
using namespace C150NETWORK; // for all the comp150 utilities

const string BEGIN = "BEG";
const string END = "END";

unsigned char obuf[20];
// forward declarations
string createMsg(string msgType, int numPkts, string fileName, char *sourceDir);

const int serverArg = 1;           // server name is 1st arg
const int networkNastinessArg = 2; // network nastiness is 2nd arg
const int fileNastinessArg = 3;    // file nastiness is 3rd arg
const int sourceDirArg = 4;        // source directory is 4th arg

int main(int argc, char *argv[])
{
    GRADEME(argc, argv);

    //
    // Variable declarations
    //
    ssize_t readlen; // amount of data read from socket
    int networkNastiness, fileNastiness;
    char *sourceDir;
    char incomingStatus[4];
    int attempts = 1;
    DIR *src;
    struct dirent *sourceFile;
    string filename;

    //  Set up debug message logging
    setUpDebugLogging("clientdebug.txt", argc, argv);

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
        c150debug->printf(C150APPLICATION, "Creating C150DgmSocket");
        C150DgmSocket *sock = new C150NastyDgmSocket(networkNastiness);
        sock->turnOnTimeouts(5000);

        // Tell the DGMSocket which server to talk to
        sock->setServerName(argv[serverArg]);
        string serverName = argv[serverArg];

        attempts = 1;

        // Declare instance of safepackets
        SafePackets safe(fileNastiness);

        while ((sourceFile = readdir(src)) != NULL)
        {
            if ((strcmp(sourceFile->d_name, ".") == 0) ||
                (strcmp(sourceFile->d_name, "..") == 0))
                continue; // never copy . or ..

            filename = sourceFile->d_name;

            safe.fileToPackets("SRC/" + filename); //TODO: not hardcode
            int numPkts = safe.getNumPkts();

            string msg = createMsg(BEGIN, numPkts, filename, sourceDir);
            cout << "client: BEGIN transmission" << endl;
            sock->write(msg.c_str(), msg.length());
            string pkt;
            cout << "client: WRITING packets" << endl;
            for (int i = 0; i < numPkts; i++)
            {
                pkt = safe.getPkt(i);
                sock->write(pkt.c_str(), pkt.length());
                if (i % 100 == 0)
                    this_thread::sleep_for(chrono::milliseconds(3));
            }

            msg = createMsg(END, numPkts, filename, sourceDir);
            sock->write(msg.c_str(), msg.length());
            cout << "client: END transmission" << endl;

            // Check for end to end status from server
            while (1)
            {
                readlen = sock->read(incomingStatus, 4);
                if (readlen != 0 and sock->timedout() == 0)
                {
                    cout << "client: received end-to-end status: " << incomingStatus << endl;

                    string ack = "received";
                    sock->write(ack.c_str(), ack.length());
                    if (strcmp(incomingStatus, "succ") == 0)
                    {
                        cout << "client: end-to-end success" << endl;
                        toLogClient(filename, "succeeded", attempts);
                    }
                    else if (strcmp(incomingStatus, "fail") == 0)
                    {
                        cout << "client: end-to-end fail" << endl;
                        toLogClient(filename, "failed", attempts);
                    }

                    break;
                }
            }
            // Clear file information
            safe.clear();
        }

        closedir(src);
    }
    //  Handle networking errors -- for now, just print message and give up!
    catch (C150NetworkException &e)
    {
        // Write to debug log
        c150debug->printf(C150ALWAYSLOG, "Caught C150NetworkException: %s\n",
                          e.formattedExplanation().c_str());
        // In case we're logging to a file, write to the console too
        cerr << argv[0] << ": caught C150NetworkException: " << e.formattedExplanation()
             << endl;
    }

    return 0;
}

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
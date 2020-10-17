// --------------------------------------------------------------
//
//                        pingclient.cpp
//
//        Author: Noah Mendelsohn
//
//
//        This is a simple client, designed to illustrate use of:
//
//            * The C150DgmSocket class, which provides
//              a convenient wrapper for sending and receiving
//              UDP packets in a client/server model
//
//            * The c150debug interface, which provides a framework for
//              generating a timestamped log of debugging messages.
//              Note that the socket classes described above will
//              write to these same logs, providing information
//              about things like when UDP packets are sent and received.
//              See comments section below for more information on
//              these logging classes and what they can do.
//
//
//        COMMAND LINE
//
//              pingclient <servername> <msgtxt>
//
//
//        OPERATION
//
//              pingclient will send a single UDP packet
//              to the named server, and will wait (forever)
//              for a single UDP packet response. The contents
//              of the packet sent will be the msgtxt, including
//              a terminating null. The response message
//              is checked to ensure that it's null terminated.
//              For safety, this application will use a routine
//              to clean up any garbage characters the server
//              sent us, (so a malicious server can't crash us), and
//              then print the result.
//
//              Note that the C150DgmSocket class will select a UDP
//              port automatically based on the user's login, so this
//              will (typically) work only on the test machines at Tufts
//              and for COMP 150-IDS who are registered. See documention
//              for the comp150ids getUserPort routine if you are
//              curious, but you shouldn't have to worry about it.
//              The framework automatically runs on a separate port
//              for each user, as long as you are registerd in the
//              the student port mapping table (ask Noah or the TAs if
//              the program dies because you don't have a port).
//
//        LIMITATIONS
//
//              This version does not timeout or retry when packets are lost.
//
//
//       Copyright: 2012 Noah Mendelsohn
//
// --------------------------------------------------------------
#include "c150nastydgmsocket.h"
#include "endtoend.cpp"
#include "safepackets.h"
#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;         // for C++ std library
using namespace C150NETWORK; // for all the comp150 utilities

// forward declarations

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
//                    Command line arguments
//
// The following are used as subscripts to argv, the command line arguments
// If we want to change the command line syntax, doing this
// symbolically makes it a bit easier.
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const int serverArg = 1;           // server name is 1st arg
const int networkNastinessArg = 2; // network nastiness is 2nd arg
const int fileNastinessArg = 3;    // file nastiness is 3rd arg
const int sourceDirArg = 4;        // source directory is 4th arg

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
//                           main program
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int main(int argc, char *argv[])
{
    GRADEME(argc, argv);

    //
    // Variable declarations
    //
    ssize_t readlen; // amount of data read from socket
    (void)readlen;
    int networkNastiness, fileNastiness;
    char *sourceDir;

    //
    //  Set up debug message logging
    //
    setUpDebugLogging("clientdebug.txt", argc, argv);

    //
    // Make sure command line looks right
    //
    if (argc != 5)
    {
        fprintf(stderr, "Correct syntxt is: %s <servername> <networknastiness> <filenastiness> <srcdir>\n", argv[0]);
        exit(1);
    }
    networkNastiness = atoi(argv[networkNastinessArg]);
    fileNastiness = atoi(argv[fileNastinessArg]);
    (void)fileNastiness;
    sourceDir = argv[sourceDirArg];

    //
    //
    //        Send / receive / print
    //
    try
    {
        // Create the socket
        c150debug->printf(C150APPLICATION, "Creating C150DgmSocket");
        C150DgmSocket *sock = new C150NastyDgmSocket(networkNastiness);
        sock->turnOnTimeouts(5000);

        // Tell the DGMSocket which server to talk to
        sock->setServerName(argv[serverArg]);
        string serverName = argv[serverArg];

        // Perform end to end check here
        performEndToEnd(sourceDir, serverName, sock);
    }

    //
    //  Handle networking errors -- for now, just print message and give up!
    //
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
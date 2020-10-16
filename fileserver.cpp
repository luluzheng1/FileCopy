// --------------------------------------------------------------
//
//                        pingserver.cpp
//
//        Author: Noah Mendelsohn
//
//
//        This is a simple server, designed to illustrate use of:
//
//            * The C150DgmSocket class, which provides
//              a convenient wrapper for sending and receiving
//              UDP packets in a client/server model
//
//            * The C150NastyDgmSocket class, which is a variant
//              of the socket class described above. The nasty version
//              takes an integer on its constructor, selecting a degree
//              of nastiness. Any nastiness > 0 tells the system
//              to occasionally drop, delay, reorder, duplicate or
//              damage incoming packets. Higher nastiness levels tend
//              to be more aggressive about causing trouble
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
//              pingserver <nastiness_number>
//
//
//        OPERATION
//
//              pingserver will loop receiving UDP packets from
//              any client. The data in each packet should be a null-
//              terminated string. If it is then the server
//              responds with a text message of its own.
//
//              Note that the C150DgmSocket class will select a UDP
//              port automatically based on the users login, so this
//              will (typically) work only on the test machines at Tufts
//              and for COMP 150-IDS who are registered. See documention
//              for getUserPort.
//
//
//       Copyright: 2012 Noah Mendelsohn
//
// --------------------------------------------------------------

#include "c150nastydgmsocket.h"
#include "c150debug.h"
#include "c150grading.h"
#include "sha1.cpp"
#include "log.cpp"
#include <fstream>
#include <cstdlib>
#include <string>
#include <sstream>
#include <stdio.h>
#include <openssl/sha.h>

using namespace C150NETWORK; // for all the comp150 utilities

void setUpDebugLogging(const char *logname, int argc, char *argv[]);
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
    ssize_t readlen;           // amount of data read from socket
    char incomingMessage[512]; // received message data
    // char outgoingMessage[512];
    string filename;
    string messageType;
    string status;

    int networkNastiness; // how aggressively do we drop packets, etc?
    int fileNastiness;
    string targetDir;

    unsigned char obuf[20];
    //
    // Check command line and parse arguments
    //
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
    (void)fileNastiness;
    targetDir = argv[3];

    //
    //  Set up debug message logging
    //
    setUpDebugLogging("serverdebug.txt", argc, argv);

    //
    // We set a debug output indent in the server only, not the client.
    // That way, if we run both programs and merge the logs this way:
    //
    //    cat pingserverdebug.txt pingserverclient.txt | sort
    //
    // it will be easy to tell the server and client entries apart.
    //
    // Note that the above trick works because at the start of each
    // log entry is a timestamp that sort will indeed arrange in
    // timestamp order, thus merging the logs by time across
    // server and client.
    //
    c150debug->setIndent("    "); // if we merge client and server
    // logs, server stuff will be indented

    //
    // Create socket, loop receiving and responding
    //
    try
    {
        //
        // Create the socket
        //
        c150debug->printf(C150APPLICATION, "Creating C150NastyDgmSocket(nastiness=%d)",
                          networkNastiness);
        C150DgmSocket *sock = new C150NastyDgmSocket(networkNastiness);
        c150debug->printf(C150APPLICATION, "Ready to accept messages");

        //
        // infinite loop processing messages
        //
        while (1)
        {

            //
            // Read a packet
            // -1 in size below is to leave room for null
            //
            readlen = sock->read(incomingMessage, sizeof(incomingMessage) - 1);
            if (readlen == 0)
            {
                c150debug->printf(C150APPLICATION, "Read zero length message, trying again");
                continue;
            }

            //
            // Clean up the message in case it contained junk
            //
            incomingMessage[readlen] = '\0';  // make sure null terminated
            string incoming(incomingMessage); // Convert to C++ string ...it's slightly
                                              // easier to work with, and cleanString
                                              // expects it

            c150debug->printf(C150APPLICATION, "Successfully read %d bytes. Message=\"%s\"", readlen, incoming.c_str());

            // Detect message type
            size_t pos = incoming.find(":");
            filename = incoming.substr(0, pos);
            incoming.erase(0, pos + 1);
            pos = incoming.find(":");
            messageType = incoming.substr(0, pos);

            // cout << filename << endl;
            // cout << messageType << endl;

            if (messageType.compare("filename") == 0)
            {
                // Filename sent, return back SHA1hash of the specified file
                encodeSHA1(targetDir, filename, obuf);
                c150debug->printf(C150APPLICATION, "%s: Sending SHA1 for file: \"%s\"", filename);
                string response = filename + ":" + SHA1toHex(obuf) + "\0";

                // cout << "Server computed: ";
                // cout << SHA1toHex(obuf) << endl;
                // cout << "Sending back: " << response << endl;

                sock->write(response.c_str(), strlen(response.c_str()) + 1);
            }
            else
            {
                // Received status, return back acknowledgement
                string expectedStatus = "check succeeded";
                incoming.erase(0, pos + 1);
                pos = incoming.find(":");
                status = incoming.substr(0, pos);

                if (strcmp(status.c_str(), expectedStatus.c_str()) == 0)
                    toLogServer(filename, "succeeded");
                else
                    toLogServer(filename, "failed");

                string response = filename + ":" + status;

                // cout << "From Client: " << status << endl;
                // cout << "Server expects: " << expectedStatus << endl;
                // cout << "Sending back: " << response << endl
                //  << endl;
                ;

                sock->write(response.c_str(), strlen(response.c_str()) + 1);
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

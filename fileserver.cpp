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
#include <fstream>
#include <cstdlib>
#include <string>
#include <sstream>
#include <stdio.h>
#include <openssl/sha.h>

using namespace C150NETWORK; // for all the comp150 utilities

void setUpDebugLogging(const char *logname, int argc, char *argv[]);
void encodeSHA1(string filename, unsigned char obuf[]);
void toLog(string filename, string status, int attempts);
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
    ssize_t readlen;            // amount of data read from socket
    char incomingFilename[512]; // received message data
    string filename;
    int nastiness; // how aggressively do we drop packets, etc?

    unsigned char obuf[20];
    //
    // Check command line and parse arguments
    //
    if (argc != 2)
    {
        fprintf(stderr, "Correct syntxt is: %s <nastiness_number>\n", argv[0]);
        exit(1);
    }
    if (strspn(argv[1], "0123456789") != strlen(argv[1]))
    {
        fprintf(stderr, "Nastiness %s is not numeric\n", argv[1]);
        fprintf(stderr, "Correct syntxt is: %s <nastiness_number>\n", argv[0]);
        exit(4);
    }
    nastiness = atoi(argv[1]); // convert command line string to integer

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
                          nastiness);
        C150DgmSocket *sock = new C150NastyDgmSocket(nastiness);
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
            readlen = sock->read(incomingFilename, sizeof(incomingFilename) - 1);
            if (readlen == 0)
            {
                c150debug->printf(C150APPLICATION, "Read zero length message, trying again");
                continue;
            }

            //
            // Clean up the message in case it contained junk
            //
            incomingFilename[readlen] = '\0';  // make sure null terminated
            string incoming(incomingFilename); // Convert to C++ string ...it's slightly
                                               // easier to work with, and cleanString
                                               // expects it
            cleanString(incoming);             // c150ids-supplied utility: changes
                                               // non-printing characters to .
            filename = incoming;
            c150debug->printf(C150APPLICATION, "Successfully read %d bytes. Message=\"%s\"", readlen, incoming.c_str());

            encodeSHA1(filename, obuf);

            sock->write((const char *)obuf, 20);

            readlen = 0;

            while (readlen == 0)
            {
                readlen = sock->read(incomingFilename, sizeof(incomingFilename) - 1);
                if (readlen == 0)
                {
                    c150debug->printf(C150APPLICATION, "Read zero length message, trying again");
                    continue;
                }
            }

            incomingFilename[readlen] = '\0'; // make sure null terminated
            string status(incomingFilename);  // Convert to C++ string ...it's slightly
                                              // easier to work with, and cleanString
                                              // expects it
            cleanString(status);              // c150ids-supplied utility: changes
                                              // non-printing characters to .
            string expectedStatus = filename + " check succeeded";
            if (strcmp(status.c_str(), expectedStatus.c_str()))
                toLog(filename, "succeeded", 1);
            else
                toLog(filename, "failed", 1);

            sock->write(status.c_str(), strlen(status.c_str()));
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

// Take in filename, and encrypt the content of the file using SHA1
void encodeSHA1(string filename, unsigned char obuf[])
{
    ifstream *t; // SHA1 related variables
    stringstream *buffer;
    t = new ifstream("TARGET/" + filename);
    buffer = new stringstream;
    *buffer << t->rdbuf();
    SHA1((const unsigned char *)buffer->str().c_str(),
         (buffer->str()).length(), obuf);

    delete t;
    delete buffer;
}

// TODO: Why is there a 30 second delay for log to be put into to GRADELOG?
void toLog(string filename, string status, int attempt)
{
    *GRADING << "File: " + filename + " end-to-end check " + status + ", attempt " + to_string(attempt) << endl;
    c150debug->printf(C150APPLICATION, "\"%s\": copy \"%s\"", filename, status);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
//                     setUpDebugLogging
//
//        For COMP 150-IDS, a set of standards utilities
//        are provided for logging timestamped debug messages.
//        You can use them to write your own messages, but
//        more importantly, the communication libraries provided
//        to you will write into the same logs.
//
//        As shown below, you can use the enableLogging
//        method to choose which classes of messages will show up:
//        You may want to turn on a lot for some debugging, then
//        turn off some when it gets too noisy and your core code is
//        working. You can also make up and use your own flags
//        to create different classes of debug output within your
//        application code
//
//        NEEDSWORK: should be factored and shared w/pingclient
//        NEEDSWORK: document arguments
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void setUpDebugLogging(const char *logname, int argc, char *argv[])
{

    //
    //           Choose where debug output should go
    //
    // The default is that debug output goes to cerr.
    //
    // Uncomment the following three lines to direct
    // debug output to a file. Comment them to
    // default to the console
    //
    // Note: the new DebugStream and ofstream MUST live after we return
    // from setUpDebugLogging, so we have to allocate
    // them dynamically.
    //
    //
    // Explanation:
    //
    //     The first line is ordinary C++ to open a file
    //     as an output stream.
    //
    //     The second line wraps that will all the services
    //     of a comp 150-IDS debug stream, and names that filestreamp.
    //
    //     The third line replaces the global variable c150debug
    //     and sets it to point to the new debugstream. Since c150debug
    //     is what all the c150 debug routines use to find the debug stream,
    //     you've now effectively overridden the default.
    //
    ofstream *outstreamp = new ofstream(logname);
    DebugStream *filestreamp = new DebugStream(outstreamp);
    DebugStream::setDefaultLogger(filestreamp);

    //
    //  Put the program name and a timestamp on each line of the debug log.
    //
    c150debug->setPrefix(argv[0]);
    c150debug->enableTimestamp();

    //
    // Ask to receive all classes of debug message
    //
    // See c150debug.h for other classes you can enable. To get more than
    // one class, you can or (|) the flags together and pass the combined
    // mask to c150debug -> enableLogging
    //
    // By the way, the default is to disable all output except for
    // messages written with the C150ALWAYSLOG flag. Those are typically
    // used only for things like fatal errors. So, the default is
    // for the system to run quietly without producing debug output.
    //
    c150debug->enableLogging(C150APPLICATION | C150NETWORKTRAFFIC |
                             C150NETWORKDELIVERY);
}

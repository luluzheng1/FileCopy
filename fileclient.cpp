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

#include "c150dgmsocket.h"
#include "c150debug.h"
#include "c150grading.h"
// #include "nastyfiletest.cpp"
#include <dirent.h>
#include <fstream>
#include <string>
#include <cstdlib>
#include <sstream>
#include <string>
#include <stdio.h>
#include <openssl/sha.h>
#include <unordered_map>
#include <sys/types.h>
#include <sys/stat.h>
#include <filesystem>
using namespace std;         // for C++ std library
using namespace C150NETWORK; // for all the comp150 utilities

typedef unordered_map<string, unsigned char *> sha1Map;

// forward declarations
void checkAndPrintMessage(ssize_t readlen, char *buf, ssize_t bufferlen);
void setUpDebugLogging(const char *logname, int argc, char *argv[]);
void readFiveTimes(C150DgmSocket *sock, char *incomingMessage, int size);
void toLog(string filename, string status, int attempts);
void encodeSHA1(string filename, unsigned char obuf[]);
void printSHA1(unsigned char *received, unsigned char *expected);
void hashSHA1(char *sourceFile, sha1Map &SHA1map);
void checkDirectory(char *dirname);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
//                    Command line arguments
//
// The following are used as subscripts to argv, the command line arguments
// If we want to change the command line syntax, doing this
// symbolically makes it a bit easier.
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const int serverArg = 1; // server name is 1st arg
const int msgArg = 2;    // message text is 2nd arg

unsigned char obuf[20];
int OBUF_SIZE = 20;
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
    char incomingMessage[512];      // received message data
    unsigned char incomingSHA1[20]; // received SHA1 data

    sha1Map SHA1 = {};
    //
    //  Set up debug message logging
    //
    setUpDebugLogging("clientdebug.txt", argc, argv);

    //
    // Make sure command line looks right
    //
    if (argc != 3)
    {
        fprintf(stderr, "Correct syntxt is: %s <servername> <srcdir>\n", argv[0]);
        exit(1);
    }

    //
    //
    //        Send / receive / print
    //
    try
    {

        // Create the socket
        c150debug->printf(C150APPLICATION, "Creating C150DgmSocket");
        C150DgmSocket *sock = new C150DgmSocket();
        sock->turnOnTimeouts(3000);

        // Tell the DGMSocket which server to talk to
        sock->setServerName(argv[serverArg]);

        // char filename[50] = "data1";
        string filename = "independence.txt";

        // recurse through directory
        // for each file, key = filepath, value = encoded SHA1
        encodeSHA1(filename, obuf);
        // auto s = SHA1.find("independence.txt");

        hashSHA1(argv[2], SHA1);
        for (const auto &n : SHA1)
        {
            cout << "Key: " << n.first << endl;
            printSHA1(n.second, n.second);
        }

        // Send the message to the server
        c150debug->printf(C150APPLICATION, "%s: Sending file: \"%s\"",
                          argv[0], filename.c_str());
        sock->write(filename.c_str(), filename.length() + 1); // +1 includes the null

        c150debug->printf(C150APPLICATION, "%s: Returned from write, doing read()", argv[0]);

        // Read SHA1 from server
        readFiveTimes(sock, (char *)incomingSHA1, OBUF_SIZE);

        char status[100]; // NEEDSWORK: how not to use static atribitrary num?

        if (strcmp((const char *)obuf, (const char *)incomingSHA1) == 0)
        {
            strcpy(status, filename.c_str());
            strcat(status, " check succeeded");
            toLog(filename, "succeeded", 1);
        }
        else
        {
            strcpy(status, filename.c_str());
            strcat(status, " check failed");
            toLog(filename, "failed", 1);
        }

        sock->write(status, strlen(status + 1));

        // Read ack from server
        readFiveTimes(sock, (char *)incomingMessage, OBUF_SIZE);

        c150debug->printf(C150APPLICATION, "%s", incomingMessage);
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

// NEEDSWORK: COMMENT THIS BIATCH
void readFiveTimes(C150DgmSocket *sock, char *incomingMessage, int size)
{
    int numAttempts = 0;

    while (numAttempts < 6)
    {
        sock->read(incomingMessage, size);
        if (sock->timedout() == 0)
            break;

        numAttempts++;

        if (numAttempts == 5)
            throw C150NetworkException("The server is not responding");
    }
}

// NEEDS WORK
void toLog(string filename, string status, int attempt)
{
    *GRADING << "File: " + filename + " end-to-end check " + status + ", attempt " + to_string(attempt) << endl;
    c150debug->printf(C150APPLICATION, "\"%s\": copy \"%s\"", filename, status);
}

void printSHA1(unsigned char *received, unsigned char *expected)
{
    // cout << "Received:" << endl;

    for (int i = 0; i < 20; i++)
    {
        printf("%02x", (unsigned int)received[i]);
    }
    cout << endl;

    // cout << "Computed:" << endl;
    // for (int i = 0; i < 20; i++)
    // {
    //     printf("%02x", (unsigned int)expected[i]);
    // }

    // cout << endl;
}

// Take in filename, and encrypt the content of the file using SHA1
void encodeSHA1(string filename, unsigned char obuf[])
{
    ifstream *t; // SHA1 related variables
    stringstream *buffer;
    t = new ifstream("SRC/" + filename);
    buffer = new stringstream;
    *buffer << t->rdbuf();
    SHA1((const unsigned char *)buffer->str().c_str(),
         (buffer->str()).length(), obuf);

    delete t;
    delete buffer;
}

void hashSHA1(char *sf, sha1Map &SHA1map)
{
    checkDirectory(sf);

    DIR *src;
    struct dirent *sourceFile; // Directory entry for source file

    unsigned char buf[20];

    src = opendir(sf);
    if (src == NULL)
    {
        fprintf(stderr, "Error opening source directory %s\n", sf);
        exit(8);
    }

    while ((sourceFile = readdir(src)) != NULL)
    {
        // skip the . and .. names
        if ((strcmp(sourceFile->d_name, ".") == 0) ||
            (strcmp(sourceFile->d_name, "..") == 0))
            continue; // never copy . or ..

        encodeSHA1(sourceFile->d_name, buf);

        unsigned char *p = new unsigned char[20];
        strcpy((char *)p, (char *)buf);

        SHA1map.insert({sourceFile->d_name, p});
    }
    closedir(src);
}

// ------------------------------------------------------
//
//                   checkDirectory
//
//  Make sure directory exists
//
// ------------------------------------------------------

void checkDirectory(char *dirname)
{
    struct stat statbuf;
    if (lstat(dirname, &statbuf) != 0)
    {
        fprintf(stderr, "Error stating supplied source directory %s\n", dirname);
        exit(8);
    }

    if (!S_ISDIR(statbuf.st_mode))
    {
        fprintf(stderr, "File %s exists but is not a directory\n", dirname);
        exit(8);
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
//                     checkAndPrintMessage
//
//        Make sure length is OK, clean up response buffer
//        and print it to standard output.
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void checkAndPrintMessage(ssize_t readlen, char *msg, ssize_t bufferlen)
{
    //
    // Except in case of timeouts, we're not expecting
    // a zero length read
    //
    if (readlen == 0)
    {
        throw C150NetworkException("Unexpected zero length read in client");
    }

    // DEFENSIVE PROGRAMMING: we aren't even trying to read this much
    // We're just being extra careful to check this
    if (readlen > (int)(bufferlen))
    {
        throw C150NetworkException("Unexpected over length read in client");
    }

    //
    // Make sure server followed the rules and
    // sent a null-terminated string (well, we could
    // check that it's all legal characters, but
    // at least we look for the null)
    //
    if (msg[readlen - 1] != '\0')
    {
        throw C150NetworkException("Client received message that was not null terminated");
    };

    //
    // Use a routine provided in c150utility.cpp to change any control
    // or non-printing characters to "." (this is just defensive programming:
    // if the server maliciously or inadvertently sent us junk characters, then we
    // won't send them to our terminal -- some
    // control characters can do nasty things!)
    //
    // Note: cleanString wants a C++ string, not a char*, so we make a temporary one
    // here. Not super-fast, but this is just a demo program.
    string s(msg);
    cleanString(s);

    // Echo the response on the console

    c150debug->printf(C150APPLICATION, "PRINTING RESPONSE: Response received is \"%s\"\n", s.c_str());
    printf("Response received is \"%s\"\n", s.c_str());
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
//        NEEDSWORK: should be factored into shared code w/pingserver
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
    // debug output to a file. Comment them
    // to default to the console.
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
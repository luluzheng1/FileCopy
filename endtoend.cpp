#include "endtoend.h"
#include <string>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>

void performEndToEnd(string dir, C150DgmSocket *sock, string filename, string clientSHA1Hash)
{
    checkDirectory((char *)dir.c_str());
    string tempFilename, serverSHA1Hash, status;
    char incomingMessage[512]; // received message data
    unsigned char obuf[20];

    tempFilename = filename + ".TMP";

    // cout << "Computing SHA1 of: " << tempFilename << " in " << dir << endl;

    encodeSHA1(dir, tempFilename, obuf);
    serverSHA1Hash = SHA1toHex(obuf);

    // cout << "Client computed: " << clientSHA1Hash << endl;
    // cout << "Server computed: " << serverSHA1Hash << endl;

    // Check SHA1hashes sent by client and computed by server
    if (clientSHA1Hash.compare(serverSHA1Hash) == 0)
    {
        // cout << "server: end-to-end success" << endl;
        status = "succ";
        toLogServer(filename, "succeeded");
        string oldName = dir + "/" + tempFilename;
        string newName = dir + "/" + filename;

        rename(oldName.c_str(), newName.c_str());
    }
    else
    {
        // cout << "server: end-to-end fail" << endl;
        status = "fail";
        toLogServer(filename, "failed");
    }

    // Write status to client and read ack from client
    cout << "Sending end-to-end status to client" << endl;
    tryFiveTimes(sock, status, (char *)incomingMessage);
    cout << "End-to-end status sent successfully" << endl;

    c150debug->printf(C150APPLICATION, "%s", incomingMessage);
}

void tryFiveTimes(C150DgmSocket *sock, string outgoingMessage, char *incomingMessage)
{
    for (int numAttempts = 0; numAttempts < 20; numAttempts++)
    {
        sock->write(outgoingMessage.c_str(), strlen(outgoingMessage.c_str()) + 1);
        int readlen = sock->read(incomingMessage, 512);
        if (sock->timedout() == 0 or readlen != 0)
            return;
    }
    throw C150NetworkException("The server is not responding");
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
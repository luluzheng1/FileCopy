#include "endtoend.h"
#include <string>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <chrono>
#include <thread>

inline bool fileExists(string dir, string &name)
{
    struct stat buffer;
    return (stat((dir + "/" + name).c_str(), &buffer) == 0);
}

void performEndToEnd(string dir, C150DgmSocket *sock, string filename, string clientSHA1Hash, string *status)
{
    checkDirectory((char *)dir.c_str());
    string tempFilename, serverSHA1Hash;
    char incomingMessage[512]; // received message data
    unsigned char obuf[20];

    tempFilename = filename + ".TMP";

    // if file no longer exists, don't perform end to end
    if (!fileExists(dir, tempFilename))
    {
        cout << "file no longer exists" << endl;
        tryFiveTimes(sock, *status, (char *)incomingMessage);
        return;
    }

    encodeSHA1(dir, tempFilename, obuf);
    serverSHA1Hash = SHA1toHex(obuf);

    // Check SHA1hashes sent by client and computed by server
    if (clientSHA1Hash.compare(serverSHA1Hash) == 0)
    {
        cout << filename << ": end-to-end success" << endl;
        *status = "succ";
        toLogServer(filename, "succeeded");
        string oldName = dir + "/" + tempFilename;
        string newName = dir + "/" + filename;

        rename(oldName.c_str(), newName.c_str());
    }
    else
    {
        cout << filename << ": end-to-end fail" << endl;
        *status = "fail";
        toLogServer(filename, "failed");
    }

    // Write status to client and read ack from client
    tryFiveTimes(sock, *status, (char *)incomingMessage);

    c150debug->printf(C150APPLICATION, "%s", incomingMessage);
}

void tryFiveTimes(C150DgmSocket *sock, string outgoingMessage, char *incomingMessage)
{
    for (int numAttempts = 0; numAttempts < 40; numAttempts++)
    {
        cout << "Sending status to client" << endl;
        sock->write(outgoingMessage.c_str(), strlen(outgoingMessage.c_str()) + 1);

        int readlen = sock->read(incomingMessage, 512);
        if (readlen != 0)
            return;
    }
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

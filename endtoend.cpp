#include "endtoend.h"
#include <string>
#include <cstdlib>
#include <stdio.h>
#include <chrono>
#include <thread>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

const string CHECK = "CHECK";

// Returns true if file with path dir/name exists
inline bool fileExists(string dir, string &name)
{
    struct stat buffer;
    return (stat((dir + "/" + name).c_str(), &buffer) == 0);
}

// Does end-to-end check
void performEndToEnd(string dir, C150DgmSocket *sock, string filename, string clientSHA1Hash, string *status)
{
    checkDirectory((char *)dir.c_str());
    string tempFilename, serverSHA1Hash;
    char incomingMessage[512]; // received message data
    unsigned char obuf[20];

    tempFilename = filename + ".TMP";

    // If file no longer exists, don't perform end to end
    if (!fileExists(dir, tempFilename))
    {
        sendManyTimes(sock, *status, (char *)incomingMessage);
        return;
    }

    encodeSHA1(dir, tempFilename, obuf);
    serverSHA1Hash = SHA1toHex(obuf);

    // Check SHA1 hashes sent by client and computed by server are same
    if (clientSHA1Hash.compare(serverSHA1Hash) == 0)
    {
        *status = "succ";
        toLogServer(CHECK, filename, "succeeded");
        string oldName = dir + "/" + tempFilename;
        string newName = dir + "/" + filename;

        rename(oldName.c_str(), newName.c_str());
    }
    else
    {
        *status = "fail";
        toLogServer(CHECK, filename, "failed");
    }

    // Write status to client and read ack from client
    sendManyTimes(sock, *status, (char *)incomingMessage);

    c150debug->printf(C150APPLICATION, "%s", incomingMessage);
}

// Send end-to-end status to client
void sendManyTimes(C150DgmSocket *sock, string outgoingMessage, char *incomingMessage)
{
    for (int numAttempts = 0; numAttempts < 40; numAttempts++)
    {
        sock->write(outgoingMessage.c_str(), strlen(outgoingMessage.c_str()) + 1);

        int readlen = sock->read(incomingMessage, 512);
        if (readlen != 0)
            return;
    }
}

//  Make sure directory exists
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

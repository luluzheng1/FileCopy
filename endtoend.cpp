#include "endtoend.h"
#include "sha1.h"
#include <string>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

void performEndToEnd(char *sDir, string serverName, C150DgmSocket *sock)
{

    checkDirectory(sDir);
    DIR *src;
    struct dirent *sourceFile; // Directory entry for source file
    string clientFilename, serverFilename, clientSHA1Hash, serverSHA1Hash;
    int attempts;
    char incomingMessage[512]; // received message data
    unsigned char obuf[20];
    char status[100]; // NEEDSWORK: how not to use static atribitrary num?
    src = opendir(sDir);
    if (src == NULL)
    {
        fprintf(stderr, "Error opening source directory %s\n", sDir);
        exit(8);
    }

    while ((sourceFile = readdir(src)) != NULL)
    {
        // skip the . and .. names
        if ((strcmp(sourceFile->d_name, ".") == 0) ||
            (strcmp(sourceFile->d_name, "..") == 0))
            continue; // never copy . or ..

        clientFilename = sourceFile->d_name;
        encodeSHA1(sDir, clientFilename, obuf);

        // Send the message to the server
        c150debug->printf(C150APPLICATION, "%fileclient: Sending file: \"%s\"",
                          clientFilename);
        clientSHA1Hash = SHA1toHex(obuf);

        attempts = 1;

        // Keep sending filename until client receives SHA1hash computed by server
        while (1)
        {
            // Send filename to server and read SHA1 encryption from server
            tryFiveTimes(sock, clientFilename + ":filename", incomingMessage, serverName);
            // printf("\nFile: %s, attempt %d\n", (char *)clientFilename.c_str(), attempts);

            // Parse the response
            string incoming(incomingMessage); // Convert to C++ string ...it's slightly
                                              // easier to work with, and cleanString
                                              // expects it

            // cout << "Response from server: " + incoming << endl;

            // In the response, filename and SHA1hash are separated by a colon
            size_t pos = incoming.find(":");
            serverFilename = incoming.substr(0, pos);
            incoming.erase(0, pos + 1);
            serverSHA1Hash = incoming.substr(0, 40);

            // cout << "Client calculated: ";
            // cout << clientSHA1Hash << endl;

            // cout << "Server sent: ";
            // cout << serverSHA1Hash << endl;

            // Make sure returned SHA1 is for this file
            if (serverFilename.compare(clientFilename) == 0)
            {
                strcpy(status, clientFilename.c_str());
                strcat(status, ":status:");

                // Check SHA1hashes computed by client and returned by server
                if (clientSHA1Hash.compare(serverSHA1Hash) == 0)
                {
                    strcat(status, "check succeeded");
                    toLogClient(clientFilename, "succeeded", attempts);
                }
                else
                {
                    strcat(status, "check failed");
                    toLogClient(clientFilename, "failed", attempts);
                }

                // printf("Status: %s\n", status);

                break;
            }
        }

        // Write status to server and read ack from server
        tryFiveTimes(sock, status, (char *)incomingMessage, serverName);

        c150debug->printf(C150APPLICATION, "%s", incomingMessage);
    }

    closedir(src);
}

// NEEDSWORK: COMMENT THIS BIATCH
void tryFiveTimes(C150DgmSocket *sock, string outgoingMessage, char *incomingMessage, string serverName)
{
    // cout << "Sending: " + outgoingMessage << endl;
    for (int numAttempts = 0; numAttempts < 10; numAttempts++)
    {
        sock->write(outgoingMessage.c_str(), strlen(outgoingMessage.c_str()) + 1);

        c150debug->printf(C150APPLICATION, "%s: Returned from write, doing read()", serverName);
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
#include "safefile.h"
#include "c150nastyfile.h" // for c150nastyfile & framework

#include <utility> 
#include <string>
#include <stdio.h>
#include <stdlib.h>

using namespace C150NETWORK;

int main(int argc, char *argv[])
{
    SafeFile fileoutput(1);

    FILE * file;
    size_t readlen = 0;
    char buffer[509];
    int id = 1;

    file = fopen("SRC/warandpeace.txt", "rb");

    while (1) {
        buffer[0] = '\0';
        readlen = fread(buffer, 1, 508, file);
        if (readlen == 0) {
            break;
        }
        buffer[readlen] = '\0';
        string incoming(buffer);
        pair<int, string> p = make_pair(id, incoming);
        fileoutput.storePacket(p);
        id++;
    }

    fclose(file);

    // pair<int, string> p1 = make_pair(2, "Hello world\n");
    // pair<int, string> p2 = make_pair(1, "comp105 1111");

    // fileoutput.storePacket(p1);
    // fileoutput.storePacket(p2);

    // vector<packet> packets = fileoutput.getPackets();
    // for (packet p: packets) {
    //     cout << p.first << " " << p.second<< endl;
    // }

    // fileoutput.readTest();

    fileoutput.writeFile("test.txt");

    // file = fopen("test.txt", "rb");
    // while (1) {
    //     buffer[0] = '\0';
    //     readlen = fread(buffer, 1, 508, file);
    //     if (readlen == 0) {
    //         break;
    //     }
    //     buffer[readlen] = '\0';
    //     string incoming(buffer);
    //     cout << incoming << endl;
    // }
    // fclose(file);
}
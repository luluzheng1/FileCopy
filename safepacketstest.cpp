#include "c150nastydgmsocket.h"
#include "safepackets.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{

    SafePackets safe(0);
    string sourceName = "SRC/independence.txt";

    safe.fileToPackets(sourceName);
    return 0;
}
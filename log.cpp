#include "log.h"
#include "c150grading.h"
#include <string>
#include <iomanip>
using namespace C150NETWORK;

const string BEGIN = "BEG";
const string END = "END";
const string CHECK = "CHECK";

// GRADING Log function for Client
void toLogClient(string logType, string filename, string status, int attempt)
{
    if (logType.compare(BEGIN) == 0)
        *GRADING << "File: " + filename + ", beginning transmission, attempt " + to_string(attempt) << endl;
    else if (logType.compare(END) == 0)
        *GRADING << "File: " + filename + " transmission complete, waiting for end-to-end check, attempt " + to_string(attempt) << endl;
    else if (logType.compare(CHECK) == 0)
        *GRADING << "File: " + filename + " end-to-end check " + status + ", attempt " + to_string(attempt) << endl;
}

// GRADING Log function for Server
void toLogServer(string logType, string filename, string status)
{
    if (logType.compare(BEGIN) == 0)
        *GRADING << "File: " + filename + " starting to receive file" << endl;
    else if (logType.compare(END) == 0)
        *GRADING << "File: " + filename + " received, beginning end-to-end check" << endl;
    else if (logType.compare(CHECK) == 0)
        *GRADING << "File: " + filename + " end-to-end check " + status << endl;
}
/*********************************************************************
**++
======================================================================

 COPYRIGHT (c) 2025 BY Kristina Livesay

 ALL RIGHTS RESERVED

======================================================================
**
** MODULE NAME:
**   LEFMData.h
**
** PURPOSE:
**   Declaration of the LEFMData class.
**
**   The LEFMData class is responsible for parsing, validating, and
**   extracting information from LEFM buffers received over TCP/IP.
**   This includes:
**
**     - Extracting date and time fields from incoming buffers.
**     - Validating that timestamps are within expected thresholds.
**     - Handling Daylight Savings Time transitions.
**     - Selecting the best (most recent) buffer from a collection.
**     - Parsing LEFM-specific data fields into usable formats.
**
** INPUT PARAMETERS:
**   - Buffers of type std::string (from BufferData).
**
** OUTPUT PARAMETERS:
**   - Extracted integers (year, month, day, hour, minute, second).
**   - Return codes EXIT_SUCCESS / EXIT_FAILURE.
**
** RETURN VALUE:
**   Various private/public functions return status codes indicating
**   whether parsing/validation succeeded or failed.
**
** DESCRIPTION:
**   The LEFMData class works closely with BufferData to ensure that
**   the raw TCP/IP streams are converted into valid LEFM messages
**   ready for processing. LEFMData encapsulates validation logic
**   including date/time verification against system time and threshold
**   deviation values (LEFMDATETIMEDEV).
**
** NOTES:
**   - Some private members track Daylight Savings Time state changes
**     to ensure correctness across DST boundaries.
**   - This class assumes LEFM messages are delimited by SOH/EOT.
**
======================================================================
**********************************************************************/
#ifndef LEFMDATA_H
#define LEFMDATA_H


#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <ctime>
#include <vector>
#include <string>

using namespace std;

// Constants / placeholder defines
#define BUFFER_SIZE 1024
#define SUSPICIOUS_SIZE 100
#define INVALID -1
#define SOH '\x01'  // Start of header
#define EOT '\x04'  // End of transmission

extern struct {
    float fVal;
} gtInputs[]; // Placeholder for external input structure
enum { LEFMDATETIMEDEV };

union timeunion { FILETIME fileTime; ULARGE_INTEGER ul; };
extern char bufferTEMP[BUFFER_SIZE];
int read_poll_wait (int sockfd, int msecs);
int readn(int fd, char *ptr, int nbytes);



class LEFMData
{
public:
    LEFMData() : sockfd(-1) {}
    ~LEFMData() { nCloseConnectionToLEFM(); }

    // Open TCP connection to LEFM
    int nOpenConnectionToLEFM(const std::string &hostname, int port);

    // Close TCP connection to LEFM
    void nCloseConnectionToLEFM();

    // Existing read/parse/validate functions would live here too
    int nReadFromLEFM(std::string &buffer);
    int nParseData(const std::string &buffer);
    
    int nValidateBuffers(std::vector<std::string> *Buffer);
    int nValidateDatenTime(int year, int month, int day, int hour, int min, int sec);
    int nGetDateAndTime(string input_buffer, int *year, int *month, int *day, int *hour, int *minute, int *second);


private:
    int sockfd; // Active LEFM socket descriptor, -1 if not connected
    int nValidateDatenTimechangeDST = -1;
    int nValidateBufferschangeDST = -1;
};

#endif // LEFMDATA_H

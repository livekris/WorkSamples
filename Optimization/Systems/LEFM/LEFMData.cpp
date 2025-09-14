/*********************************************************************
**++
======================================================================

 COPYRIGHT (c) 2025 BY Kristina Livesay
 
 ALL RIGHTS RESERVED

======================================================================

/* =======================================================================================
 * LEFMData Connection Management
 *
 * PURPOSE:
 *   Manage TCP/IP socket connections to the LEFM device. This includes:
 *   - Opening a connection (creating socket, resolving hostname/IP, connecting to port).
 *   - Closing a connection (gracefully shutting down the socket).
 *
 * NOTES:
 *   - Sockets are created using standard BSD sockets API.
 *   - TCP guarantees reliable data transfer, but connection interruptions must still be
 *     handled by higher-level retry logic in the application.
 * ======================================================================================= */
#include "LEFMData.h"
#include "LEFMI.h"

// --------------------------------------------------------
/*
 * nOpenConnectionToLEFM
 *
 * Establishes a TCP/IP connection to the LEFM device.
 * Creates a socket, resolves the hostname/IP,
 * and connects to the specified port.
 *
 * Returns:
 *   >=0 : valid connected socket descriptor
 *   -1  : failure
 */
// --------------------------------------------------------
int LEFMData::nOpenConnectionToLEFM(const std::string &hostname, unsigned int port)
{
    int sockfd;
    struct addrinfo hints, *res, *p;
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_UNSPEC;    // Support IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;  // TCP

    if(getaddrinfo(hostname.c_str(), port_str, &hints, &res) != 0) {
        perror("getaddrinfo");
        return -1;
    }

    // Try all addresses until one works
    for(p = res; p != nullptr; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(sockfd == -1) continue;

        if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }

        // Connected successfully
        freeaddrinfo(res);
        return sockfd;
    }

    // No connection succeeded
    freeaddrinfo(res);
    return -1;
}

// --------------------------------------------------------
/*
 * nCloseConnectionToLEFM
 *
 * Gracefully closes the socket connected to the LEFM device.
 * Ensures all system resources are released.
 */
// --------------------------------------------------------
void LEFMData::nCloseConnectionToLEFM(int sockfd)
{
    if(sockfd >= 0) {
        shutdown(sockfd, SHUT_RDWR);  // polite close (send + recv disabled)
        close(sockfd);
    }
}

// --------------------------------------------------------
// Read data from LEFM
// --------------------------------------------------------
int LEFMData::nReadFromLEFM()
{
        char msg[128];
        int  ready;
        int  n;
        int  timeout = 7000;
        DWORD dwError;


        timeout = (int)gtInputs[MAXLEFMTIMEOUT].fVal * 1000;

        ready = read_poll_wait(gdescriptor, timeout);
        if (ready == 0)
            return NO_DATA_RECEIVED;
        else if (ready < 0)
        {
            dwError = WSAGetLastError();

            (void)swprintf(gszMessageW, sizeof(gszMessageW), MSG_READ_RECV_GENERAL, gcszProcessName, this->chLEFMname, dwError);
            record_msge_wc(gszMessageW, MSGTYPE_APP_ERROR);

            nCloseConnectionToLEFM();

            return NO_DATA_RECEIVED;

        }

        /* read the variable length packet */
        n = readn(gdescriptor, bufferTEMP, 1000);
        if(n == -1)
        {
            dwError = WSAGetLastError();

            (void)swprintf(gszMessageW, sizeof(gszMessageW), MSG_READ_RECV_GENERAL, gcszProcessName, this->chLEFMname, dwError);
            record_msge_wc(gszMessageW, MSGTYPE_APP_ERROR);

            nCloseConnectionToLEFM();
            return NO_DATA_RECEIVED;

        }//end     if(n == -1)
        else if(n == 0)
        {
            (void)swprintf(gszMessageW, sizeof(gszMessageW), MSG_READ_FAIL_GENERAL, gcszProcessName, this->chLEFMname);
            record_msge_wc(gszMessageW, MSGTYPE_APP_ERROR);
            return NO_DATA_RECEIVED;
        }//end else if(n == 0)

        (void)this->buffer.nSetBuffer(bufferTEMP, Incomplete_Buffer_Possibility);

        if ( (gDebugOutput == 2 || gDebugOutput == 3) && gDebugLevel == 1)
        {
            sprintf(msg, "LEFMI: Read bytes = %d ........ first_byte= %d last_byte= %d\n", n, bufferTEMP[0], bufferTEMP[n-1]);
            OutputDebugString(msg);
            char formatBuffer[BUFFER_SIZE];
            sprintf(formatBuffer, "LEFMI raw data begins:");
            OutputDebugString(formatBuffer);
            OutputDebugString(bufferTEMP);
            sprintf(formatBuffer, "LEFMI raw data ends.");
            OutputDebugString(formatBuffer);
        }//end if ( (gDebugOutput == 2 || gDebugOutput == 1) && gDebugLevel == 1)
        
        if (bufferTEMP[0] != 1 || bufferTEMP[n-1] != 3)
            return NO_DATA_RECEIVED;

        Incomplete_Buffer_Possibility = FALSE;


        return DATA_RECEIVED;

}//end int LEFMData::nReadFromLEFM()


// --------------------------------------------------------
/*
 * Read "n" bytes from a descriptor.
 * Use in place of read() when fd is a stream socket.
 */

// Return values:
//  >0 : number of bytes actually read (could be < nbytes if timeout or EOF)
//   0 : peer closed connection
//  -1 : error (check errno / WSAGetLastError)
//  -2 : timeout
// --------------------------------------------------------
int readn(int fd, char *ptr, int nbytes, int timeout_ms) {
    int nleft = nbytes;
    int nread = 0;
    char *p = ptr;

    while (nleft > 0) {
        int ready = read_poll_wait(fd, timeout_ms);
        if (ready == 0) {
            // timeout
            return (nbytes - nleft > 0) ? (nbytes - nleft) : -2;
        } else if (ready < 0) {
            // select() error
            return -1;
        }

        nread = recv(fd, p, nleft, 0);
        if (nread < 0) {
            return -1;  // recv error
        } else if (nread == 0) {
            return 0;   // connection closed by peer /* EOF */
        }

        nleft -= nread;
        p     += nread;
    }

    return nbytes; // full read successful
}


// --------------------------------------------------------
/*
 * Select on some sockets to determine if they are ready to be read.
 */
// --------------------------------------------------------
int read_poll_wait (int sockfd, int msecs)
{
    fd_set                readfds;
    int                    num_rdy;
    char msg[128];
    struct timeval      timeout;

    /*
    ** See if a read is ready on the given socket.
    */

    timeout.tv_sec = msecs / 1000;
    timeout.tv_usec = (msecs % 1000) * 1000;
    memset((char *) &readfds, 0, sizeof(fd_set));

    FD_SET(sockfd, &readfds);
    if ((num_rdy = num_rdy = select(sockfd + 1, &readfds, NULL, NULL, &timeout)) < 0)
    {
       sprintf(msg, "LEFMI:  read_poll - Select error\n");
       RTmsg(msg);
       return(-1);
    }
    return(num_rdy);
}


// --------------------------------------------------------
// Extract date and time from LEFM buffer
// --------------------------------------------------------
int LEFMData::nGetDateAndTime(string input_buffer, int *year, int *month, int *day, int *hour, int *minute, int *second)
{
    int BEGINNING;
    int ENDING;

    if(input_buffer.length() == 0) return EXIT_FAILURE;

    size_t found = input_buffer.find(SOH);
    BEGINNING = (int)found;

    if(found == string::npos || found > BUFFER_SIZE || found < 0) return EXIT_FAILURE;

    int found_end = input_buffer.find(EOT, found);
    ENDING = found_end;
    
    if(found_end == string::npos || found_end > BUFFER_SIZE || found_end < 0) return EXIT_FAILURE;

    int i = (int)found+1;
    while(i < (int)input_buffer.length() && input_buffer[i] != ',') i++;
    if(i == input_buffer.length()) return EXIT_FAILURE;

    char DATE_TEMP[11];
    char TIME_TEMP[9];
    int DATE[3];
    int TIME[3];

    for(int j = 0; j < 10; j++) {
        if(input_buffer[i+j+1] != '\0') DATE_TEMP[j] = input_buffer[i+j+1]; else return EXIT_FAILURE;
        if(j < 8) {
            if(input_buffer[i+j+12] != '\0') TIME_TEMP[j] = input_buffer[i+j+12]; else return EXIT_FAILURE;
        }
    }

    DATE_TEMP[10] = '\0';
    TIME_TEMP[8] = '\0';

    // Convert DATE to integer
    char integer[5] = "";
    int int_loc = 0;
    int slash_count = 0;
    for(int date = 0; date < 10; date++) {
        if(DATE_TEMP[date] == '/') {
            integer[2] = '\0';
            DATE[slash_count] = atoi(integer);
            int_loc = 0;
            slash_count++;
            date++;
        }
        integer[int_loc++] = DATE_TEMP[date];
    }
    integer[4] = '\0';
    DATE[slash_count] = atoi(integer);

    // Convert TIME to integer
    char integer2[4] = "";
    int_loc = 0;
    int colon_count = 0;
    for(int time = 0; time < 10; time++) {
        if(TIME_TEMP[time] == ':') {
            TIME[colon_count] = atoi(integer2);
            int_loc = 0;
            colon_count++;
            time++;
        }
        integer2[int_loc++] = TIME_TEMP[time];
    }
    TIME[colon_count] = atoi(integer2);

    *year   = DATE[2];
    *month  = DATE[0];
    *day    = DATE[1];
    *hour   = TIME[0];
    *minute = TIME[1];
    *second = TIME[2];

    return EXIT_SUCCESS;
}

// --------------------------------------------------------
// Validate parsed date and time
// --------------------------------------------------------
int LEFMData::nValidateDatenTime(int year, int month, int day, int hour, int min, int sec)
{
    time_t t = time(0);
    struct tm * now = localtime(&t);

    // Year check
    if(year > (now->tm_year + 1900)) return EXIT_FAILURE;
    if(year < (now->tm_year + 1900)) {
        if((now->tm_mon + 1) == 1 && month != 12) return EXIT_FAILURE;
        if(day != 31) return EXIT_FAILURE;
    }

    // Month and day checks
    if((now->tm_mon + 1) < month) return EXIT_FAILURE;
    if((now->tm_mon + 1) > month && (now->tm_mon + 1) - month != 1) return EXIT_FAILURE;
    if(now->tm_mday - day < 0 || now->tm_mday - day > 1) return EXIT_FAILURE;

    // DST considerations
    if(now->tm_isdst > 0 && now->tm_isdst != -1) {
        if(this->nValidateDatenTimechangeDST != 1) {
            nValidateDatenTimechangeDST = 1;
            if(((now->tm_hour + now->tm_min/60.0 + now->tm_sec/3600.0) - (hour + min/60.0 + sec/3600.0)) < 0 || hour == 0)
                now->tm_hour++;
        }
    } else if(now->tm_isdst != -1 && this->nValidateDatenTimechangeDST != 0) {
        nValidateDatenTimechangeDST = 0;
        if(((now->tm_hour + now->tm_min/60.0 + now->tm_sec/3600.0) - (hour + min/60.0 + sec/3600.0)) < 0) now->tm_hour--;
    }

    // Threshold evaluation
    double diff = (now->tm_hour + now->tm_min/60.0 + now->tm_sec/3600.0) - (hour + min/60.0 + sec/3600.0);
    if(diff < 0) diff += 24;

    if(diff > (int)gtInputs[LEFMDATETIMEDEV].fVal) return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

// --------------------------------------------------------
// Validate multiple buffers
// --------------------------------------------------------
int LEFMData::nValidateBuffers(vector<string> *Buffer)
{
    int year[SUSPICIOUS_SIZE], month[SUSPICIOUS_SIZE], day[SUSPICIOUS_SIZE];
    int hour[SUSPICIOUS_SIZE], min[SUSPICIOUS_SIZE], sec[SUSPICIOUS_SIZE];

    time_t t = time(0);
    struct tm *now = localtime(&t);

    int i = 0;
    for(auto it = Buffer->begin(); it != Buffer->end(); ++it) {
        if(it->length() == 0) continue;

        if(this->nGetDateAndTime(*it, &year[i], &month[i], &day[i], &hour[i], &min[i], &sec[i]) == EXIT_FAILURE) {
            year[i] = month[i] = day[i] = hour[i] = min[i] = sec[i] = INVALID;
            continue;
        }
        if(this->nValidateDatenTime(year[i], month[i], day[i], hour[i], min[i], sec[i]) == EXIT_FAILURE) {
            year[i] = month[i] = day[i] = hour[i] = min[i] = sec[i] = INVALID;
            continue;
        }
        i++;
    }

    if(i == 0) return INVALID;

    int BestTime_flag = 0;
    int currentY = year[0], currentM = month[0], currentD = day[0], currentH = hour[0], currentMi = min[0], currentS = sec[0];

    for(int j = 1; j < i; j++) {
        if(currentY == INVALID) {
            currentY = year[j]; currentM = month[j]; currentD = day[j];
            currentH = hour[j]; currentMi = min[j]; currentS = sec[j];
            BestTime_flag = j;
            continue;
        }

        double diff_1 = (now->tm_hour + now->tm_min/60.0 + now->tm_sec/3600.0) - (hour[j] + min[j]/60.0 + sec[j]/3600.0);
        double diff_2 = (now->tm_hour + now->tm_min/60.0 + now->tm_sec/3600.0) - (currentH + currentMi/60.0 + currentS/3600.0);

        if((diff_1 > 0 && diff_2 > 0) || (diff_1 < 0 && diff_2 < 0)) {
            if((diff_2 - diff_1) > 0) {
                currentY = year[j]; currentM = month[j]; currentD = day[j];
                currentH = hour[j]; currentMi = min[j]; currentS = sec[j];
                BestTime_flag = j;
            }
        } else if(diff_1 > 0 && diff_2 < 0) {
            currentY = year[j]; currentM = month[j]; currentD = day[j];
            currentH = hour[j]; currentMi = min[j]; currentS = sec[j];
            BestTime_flag = j;
        }
    }

    return BestTime_flag;
}

// --------------------------------------------------------
// Parse data from a single buffer
// --------------------------------------------------------
int LEFMData::nParseData(string input_buffer, vector<string> *Buffer)
{
    if(input_buffer.empty()) return EXIT_FAILURE;
    Buffer->push_back(input_buffer);

    int latestIndex = nValidateBuffers(Buffer);
    if(latestIndex == INVALID) return EXIT_FAILURE;

    return latestIndex;
}

/*********************************************************************
**++
======================================================================

 COPYRIGHT (c) 2025 BY Kristina Livesay

 ALL RIGHTS RESERVED

======================================================================
**  MODULE NAME:
**   BufferData implementation
**
**  PURPOSE:
**   Optimized BufferData class
**
** The BufferData class is a smart buffer manager designed to handle data received from the LEFM device
** over TCP/IP, where messages may arrive fragmented or out of order. It stores incoming raw data in a temporary
** buffer (Bbuffer), tracks whether the data is complete or partial using the Incomplete_Buffer_Possibility
** flag, and classifies the data into vectors: GOOD_buffer for fully valid messages, SUSPICIOUS_buffer for
** potentially overlapping or corrupted data, and OPTIMIZE_buffer for incomplete messages that can be
** appended later. Functions like nPutToSUSPICIOUSbuffer and nPutToOPTIMIZEbuffer extract substrings based
** on start and end markers (\x01 for message start, \x03 for message end), while getter functions provide clean
** access to these classified buffers. This design ensures reliable reconstruction of full messages, preserves
** partial data for later assembly, isolates suspicious data, and allows the application to process LEFM
** messages safely and efficiently despite the unpredictable chunking behavior of TCP.
**
======================================================================
**********************************************************************/
#ifndef BUFFERDATA_H
#define BUFFERDATA_H

#include "BufferData.h"
#include <algorithm>

BufferData::BufferData(void) : Incomplete_Buffer_Possibility(false)
{
    LEFM_buffer.clear();
}

BufferData::~BufferData(void) { }

void BufferData::vClearBuffers()
{
    OPTIMIZE_buffer.clear();
    SUSPICIOUS_buffer.clear();
}

int BufferData::nSetBuffer(char *input_buffer, int TRUE_OR_FALSE)
{
    Bbuffer.assign(input_buffer, strlen(input_buffer));
    Incomplete_Buffer_Possibility = TRUE_OR_FALSE;
    LEFM_buffer = { Bbuffer };
    return EXIT_SUCCESS;
}

void BufferData::vGetBuffer(std::vector<std::string> *get_buffer)
{
    get_buffer->clear();
    if (!LEFM_buffer.empty()) {
        *get_buffer = LEFM_buffer;
    }
}

int BufferData::nPutToSUSPICIOUSbuffer(int location_begin, int location_end)
{
    if (location_begin < 0 || location_end >= (int)Bbuffer.size()) {
        return EXIT_FAILURE;
    }

    BSUSPICIOUS_buffer = Bbuffer.substr(location_begin, location_end - location_begin + 1);
    SUSPICIOUS_buffer.emplace_back(BSUSPICIOUS_buffer);

    return EXIT_SUCCESS;
}

int BufferData::nPutToOPTIMIZEbuffer(int location_begin, int location_end)
{
    if (location_begin < 0 || location_begin >= (int)Bbuffer.size()) {
        return EXIT_FAILURE;
    }

    int safe_end = std::min(location_end, (int)Bbuffer.size());
    BOPTIMIZE_buffer = Bbuffer.substr(location_begin, safe_end - location_begin);
    OPTIMIZE_buffer.emplace_back(BOPTIMIZE_buffer);

    return EXIT_SUCCESS;
}

void BufferData::vGetGOODBuffer(std::vector<std::string> *input_buffer)
{
    input_buffer->clear();
    if (!GOOD_buffer.empty()) {
        *input_buffer = GOOD_buffer;
    }
}

void BufferData::vGetSUSPICIOUSBuffers(std::vector<std::string> *input_buffer)
{
    input_buffer->clear();
    if (!SUSPICIOUS_buffer.empty()) {
        *input_buffer = SUSPICIOUS_buffer;
    }
}

void BufferData::vGetCOMPLETEDBuffer(std::vector<std::string> *input_buffer)
{
    input_buffer->clear();
    std::vector<std::string> temporary;
    std::string Buftemp;

    size_t n = 0;
    size_t find_beg, find_end;
    int break_loop = 0;

    while (n < OPTIMIZE_buffer.size())
    {
        Buftemp.clear();
        find_beg = OPTIMIZE_buffer[break_loop].find(SOH);

        if (find_beg == std::string::npos || find_beg >= BUFFER_SIZE) {
            OPTIMIZE_buffer.clear();
            input_buffer->clear();
            return;
        }

        while (n < OPTIMIZE_buffer.size())
        {
            Buftemp.append(OPTIMIZE_buffer[n]);
            find_end = OPTIMIZE_buffer[n].find(EOT);

            if (find_end != std::string::npos && find_end < BUFFER_SIZE)
            {
                temporary.emplace_back(Buftemp);
                *input_buffer = temporary;
                n++;
                break_loop = n;
                break;
            }

            n++;
        }
    }
}

/* DOC-START

    NAME:
        int BufferData::nSelectData()

    SYNOPSIS:

        #include "BufferData.h"

    INPUT PARAMETERS:
        NONE

    OUTPUT PARAMETERS:
        NONE

    RETURN VALUE:
        int -- Status code for data classification

    DESCRIPTION:
        The BufferData::nSelectData() is responsible for classifying the received data from LEFM into multiple buffers depending on their content.
        See comments below.

    NOTES:

        //THE FALLBACK PLAN:
        //The following code will select data in case the server and client are out of sync and the data arriving from the server is not as normally expected
        //Also, remember that TCP ensures timely data arrival but does not ensure data arrival as one chunk in the wire
        //So, think of the code below as some code that ensures data arrival as one chunk.
        //The code below also considers a few unlikely to happen situations, but since LEFMI is supposed to run in the backround for years
        //we need to make sure we consider all these details.

        //NOTE: Knowing the behaviour of our network, some assumptions are made; there is comments to clarify some of them
        // Below are some possible ways data could be arriving: (Note: even though some of the situations below are unlikely to happen, we should consider handling them if they do happen)

        // (5 second wait) data arrived: \x01...\x03 :: A FULL SET OF DATA HAS ARRIVED; CALL IT "GOOD" (likely)
        // (5 second wait) data arrived: \x01... :: AN INCOMPLETE SET OF DATA HAS ARRIVED; LET US OPTIMIZE INSTEAD OF JUST WAITING FOR A FRESH DATA IN THE NEXT 5 seconds (this situation is likely to happen)
        // (250 ms wait) data arrived: ...\x03 :: THE OTHER END OF THE DATA HAS ARRIVED; APPEND THE PREVIOUS DATA WITH THE CURRENT DATA AND CONTINUE WITH PARSING...
        // (5 second wait) data arrived: \x01...\x03.\x01...\x03 :: A FULL SET OF DATA HAS ARRIVED BUT ALSO SOME OTHER DATA ARRIVED (this should not be a likely situation to happen, but it could happen for multiple reasons, such as signal interruptions...
        // (the reason this situation is not likely to happen is because if we have x consecutive amount of data retrieval failures,
        // (LEFMI application should close the socket and open a new one, but if we got x-1 failures and on the xth try we received the data
        // (it is likely that the data could be arriving as old plus some new, in our case for example first \x01 through \x03 could be old)
        // ...etc

* DOC-END */
  
int BufferData::nSelectData()
{
    // Up to this point in the code we have received a buffer of data
    // The rest of the code will be checking for how the data was received, such as:
    // 1. the data was received incomplete, but we decide to not throw it away
    // 2. or, we received some complete data, but also some more data after that

    int location_begin[SUSPICIOUS_SIZE];  // locations of all '\x01'
    int location_end[SUSPICIOUS_SIZE];    // locations of all '\x03'

    int begin_counter = 0;                // number of '\x01's
    int end_counter = 0;                  // number of '\x03's

    //------------------ NEW DATA ----------------------
    // Data is considered NEW if it was sent from LEFM-EU and received at the moment
    // How do we know that?: There is a UTF timer on select() function (see nReadFromLEFM) to verify whether
    // select() waited for more than 1 second to receive the data or whether the action was virtually
    // simultaneous. (If the data is old, select waits from 0-10 milliseconds)

    if (Incomplete_Buffer_Possibility == FALSE)
    {
        //We have a beginning of data (that means our application waited for 5 (or n) seconds max to receive LEFM data)
        //Whatever incomplete buffer (whether we used it or not) we have saved we do not need anymore since new data has arrived

        vClearBuffers(); // clear old buffers
        GOOD_buffer.clear();
        
        //---------- FIND LOCATIONS OF END AND BEGINNING OF LINE --------------------
        //NOTE: In case of some interruption of the application, more than one set of data could be queued on the network
        //Also, in case the data did not arrive in one whole chunk in the network, incomplete data could have arrived.
        
        size_t found;
        // Find all beginnings
        for (int n = 0; n < BUFFER_SIZE; n++)
        {
            found = (n == 0) ? Bbuffer.find(SOH) : Bbuffer.find(SOH, found + 1);
            if (found != std::string::npos && found < 1000 && found >= 0)
            {
                if (n < SUSPICIOUS_SIZE)
                {
                    location_begin[begin_counter++] = found;
                }
                else
                {
                    return CORRUPT_DATA_POSSIBILITY; // Too many SOH markers -> corrupted buffer
                }
            }
            else
            {
                break;
            }
        }

        // Find all ends
        for (int n = 0; n < BUFFER_SIZE; n++)
        {
            found = (n == 0) ? Bbuffer.find(EOT) : Bbuffer.find(EOT, found + 1);
            if (found != std::string::npos && found <= BUFFER_SIZE && found >= 0)
            {
                if (n < SUSPICIOUS_SIZE)
                {
                    location_end[end_counter++] = found;
                }
                else
                {
                    return CORRUPT_DATA_POSSIBILITY; // Too many EOT markers -> corrupted buffer
                }
            }
            else
            {
                break;
            }
        }

        // No beginning found
        if (begin_counter == 0)
        {
            return NO_DATA_RECEIVED; // Empty or irrelevant buffer
        }

        // Some beginnings, no ends
        if (end_counter == 0)
        {
            nPutToOPTIMIZEbuffer(location_begin[begin_counter - 1], Bbuffer.size());
            return SOME_DATA_RECEIVED; // Incomplete fragment, stored for later
        }

        // One full message
        if (begin_counter == 1 && end_counter == 1)
        {
            if (location_end[0] > location_begin[0])
            {
                BGOOD_buffer.clear();
                BGOOD_buffer.append(Bbuffer);
                GOOD_buffer.clear();
                GOOD_buffer.push_back(BGOOD_buffer);
                return ALL_DATA_RECEIVED; // Exactly one complete message
            }
            else
            {
                return CORRUPT_DATA_POSSIBILITY; // End marker appears before start -> invalid
            }
        }

        // Multiple messages / extra data
        for (int i = 0; i < begin_counter; i++)
        {
            int j = i;
            while (j < end_counter)
            {
                if (location_end[j] > location_begin[i])
                {
                    if (SUSPICIOUS_buffer.size() > SUSPICIOUS_SIZE)
                        return CORRUPT_DATA_POSSIBILITY; // Too many suspicious fragments

                    if (EXIT_FAILURE == nPutToSUSPICIOUSbuffer(location_begin[i], location_end[j]))
                        return CORRUPT_DATA_POSSIBILITY; // Couldn’t extract suspicious substring

                    break;
                }
                else
                {
                    j++;
                }
            }
        }

        // Last segment incomplete
        if (end_counter < begin_counter)
        {
            nPutToOPTIMIZEbuffer(location_begin[begin_counter - 1], Bbuffer.size());
            return ALL_DATA_RECEIVED_PLUS_SOME_HALF; // Full set + trailing incomplete
        }

        return ALL_DATA_RECEIVED_PLUS_SOME; // Multiple complete sets of data
    }

    //----------------- OLD / INCOMPLETE DATA ----------------------
    else
    {
        size_t found;
        // Find all beginnings
        for (int n = 0; n < BUFFER_SIZE; n++)
        {
            found = (n == 0) ? Bbuffer.find(SOH) : Bbuffer.find(SOH, found + 1);
            if (found != std::string::npos && found <= BUFFER_SIZE && found >= 0)
            {
                if (n < SUSPICIOUS_SIZE)
                {
                    location_begin[begin_counter++] = found;
                }
                else
                {
                    return CORRUPT_DATA_POSSIBILITY; // Too many SOH markers
                }
            }
            else
            {
                break;
            }
        }

        // Find all ends
        for (int n = 0; n < BUFFER_SIZE; n++)
        {
            found = (n == 0) ? Bbuffer.find(EOT) : Bbuffer.find(EOT, found + 1);
            if (found != std::string::npos && found <= BUFFER_SIZE && found >= 0)
            {
                if (n < SUSPICIOUS_SIZE)
                {
                    location_end[end_counter++] = found;
                }
                else
                {
                    return CORRUPT_DATA_POSSIBILITY; // Too many EOT markers
                }
            }
            else
            {
                break;
            }
        }

        // No beginning
        if (begin_counter == 0)
        {
            if (end_counter == 0)
            {
                nPutToOPTIMIZEbuffer(0, Bbuffer.size());
                return ALL_INCOMPLETE_DATA_RECEIVED; // Entire buffer is incomplete
            }
            else
            {
                nPutToOPTIMIZEbuffer(0, Bbuffer.size());
                return ALL_INCOMPLETE_DATA_RECEIVED_PLUS_SOME_HALF; // Incomplete + trailing end marker
            }
        }

        // Process all full messages
        int j = 0;
        for (int i = 0; i < begin_counter; i++)
        {
            while (j < end_counter)
            {
                if (location_end[j] < location_begin[i])
                {
                    nPutToOPTIMIZEbuffer(0, location_end[j] + 1);
                }
                else
                {
                    nPutToSUSPICIOUSbuffer(location_begin[i], location_end[j]);
                    j++;
                    break;
                }
                j++;
            }
        }

        // Last segment incomplete
        if (end_counter < begin_counter || (end_counter == begin_counter && location_begin[begin_counter - 1] > location_end[end_counter - 1]))
        {
            nPutToOPTIMIZEbuffer(location_begin[begin_counter - 1], Bbuffer.size());
            return ALL_INCOMPLETE_DATA_RECEIVED_PLUS_SOME_HALF; // Full sets + incomplete trailing data
        }

        return ALL_INCOMPLETE_DATA_RECEIVED_PLUS_SOME; // Multiple incomplete sets combined
    }
}

#endif // BUFFERDATA_H

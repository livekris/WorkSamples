/*********************************************************************
**++
======================================================================

 COPYRIGHT (c) 2025 BY Kristina Livesay

 ALL RIGHTS RESERVED

======================================================================
**
** MODULE NAME:
**   BufferData.h
**
** PURPOSE:
**   Declaration of the BufferData class.
**
**   The BufferData class is responsible for managing raw data arriving
**   from the LEFM device over TCP/IP. Because TCP does not guarantee
**   packet boundaries, LEFM messages may arrive fragmented, concatenated,
**   or corrupted. BufferData classifies this data into:
**
**     - GOOD_buffer:        Fully valid and complete messages (SOH...EOT).
**     - OPTIMIZE_buffer:    Incomplete messages that can be appended later.
**     - SUSPICIOUS_buffer:  Data that looks corrupted, overlapping, or
**                           otherwise abnormal.
**
**   This ensures that LEFMData can always work with reliable and
**   reassembled messages, while preserving incomplete fragments for later
**   recovery.
**
** INPUT PARAMETERS:
**   - Raw buffer (std::string Bbuffer) obtained after TCP read.
**
** OUTPUT PARAMETERS:
**   - Classified vectors of strings (GOOD, OPTIMIZE, SUSPICIOUS).
**
** RETURN VALUE:
**   - EXIT_SUCCESS / EXIT_FAILURE codes for classification functions.
**
** DESCRIPTION:
**   The BufferData::nSelectData() is responsible for parsing the raw buffer
**   and classifying message fragments based on SOH/EOT framing markers.
**   The function accounts for several possible TCP fragmentation scenarios:
**
**     (5 second wait) data arrived: \x01...\x03
**       => A FULL SET OF DATA ARRIVED; CLASSIFIED AS "GOOD"
**
**     (5 second wait) data arrived: \x01...
**       => AN INCOMPLETE SET; PUT INTO OPTIMIZE TO BE COMPLETED LATER
**
**     (250 ms wait) data arrived: ...\x03
**       => COMPLETION OF A PREVIOUS INCOMPLETE MESSAGE; APPEND AND PARSE
**
**     (5 second wait) data arrived: \x01...\x03.\x01...\x03
**       => MULTIPLE MESSAGES IN ONE BUFFER; FIRST IS "GOOD", EXTRA MAY BE
**          CLASSIFIED AS SUSPICIOUS (possible overlap or network queuing)
**
** NOTES:
**   - TCP ensures order but not boundaries, so splitting/joining messages
**     is essential for correctness.
**   - OPTIMIZE buffer prevents data loss in case of partial arrival.
**   - SUSPICIOUS buffer ensures system robustness if abnormal data appears.
**
======================================================================
**********************************************************************/
//----------------------------------------------------------
// Enum: BufferStatus
// Purpose: Classify results of parsing LEFM data.
// Old-style enum allows direct return of values like
// ALL_DATA_RECEIVED_PLUS_SOME without needing BufferStatus::
//----------------------------------------------------------
enum BufferStatus
{
    NO_DATA_RECEIVED = 0,                      // No SOH/EOT markers found
    SOME_DATA_RECEIVED,                        // SOH found but no EOT (incomplete fragment)
    ALL_DATA_RECEIVED,                         // Exactly one complete message
    ALL_DATA_RECEIVED_PLUS_SOME,               // Multiple complete messages
    ALL_DATA_RECEIVED_PLUS_SOME_HALF,          // Complete messages + trailing incomplete
    ALL_INCOMPLETE_DATA_RECEIVED,              // Only incomplete fragments
    ALL_INCOMPLETE_DATA_RECEIVED_PLUS_SOME,    // Multiple incomplete fragments
    ALL_INCOMPLETE_DATA_RECEIVED_PLUS_SOME_HALF, // Incomplete fragments + trailing partial
    CORRUPT_DATA_POSSIBILITY                   // Invalid order/too many markers
};

//----------------------------------------------------------
// Class: BufferData
// Purpose: Smart buffer management for LEFM TCP/IP messages
//----------------------------------------------------------
class BufferData
{
public:
    BufferData();
    ~BufferData(void);

    // Classify incoming buffer (called after TCP read)
    // Core classification logic
    int nSelectData();

    // Clear all working buffers (GOOD, SUSPICIOUS, OPTIMIZE, COMPLETED, etc.)
    void vClearBuffers();
    
    // Set the working buffer from a raw char* input buffer.
    // TRUE_OR_FALSE indicates whether incomplete buffer handling is needed.
    int nSetBuffer(char *input_buffer, int TRUE_OR_FALSE);
    
    // Get the current working buffer as a vector of strings.
    void vGetBuffer(std::vector<std::string> *get_buffer);
    
    // Store suspicious data (header found but integrity questionable).
    int nPutToSUSPICIOUSbuffer(int location_begin, int location_end);
    
    // Store partially arrived data for optimization purposes.
    int nPutToOPTIMIZEbuffer(int location_begin, int location_end);
    
    // Getter functions
    void vGetGOODBuffer(std::vector<std::string> *input_buffer);
    void vGetSUSPICIOUSBuffers(std::vector<std::string> *input_buffer);
    void vGetCOMPLETEDBuffer(std::vector<std::string> *input_buffer);
    
    std::vector<std::string> LEFM_buffer;       // Original received buffer

    // Buffers exposed for processing by LEFMData
    std::vector<std::string> GOOD_buffer;       // complete, ready-to-parse messages
    std::vector<std::string> OPTIMIZE_buffer;   // partial messages waiting for completion
    std::vector<std::string> SUSPICIOUS_buffer; // malformed or unexpected cases

    // Temporary working buffers
    std::string Bbuffer;                        // raw buffer after TCP read
    std::string BOPTIMIZE_buffer;               // temp buffer used when joining OPTIMIZE pieces
    std::string BSUSPICIOUS_buffer;             // Latest suspicious fragment
    
    bool Incomplete_Buffer_Possibility;         // Tracks if incomplete data is pending

private:
    // Marker characters for framing
    static constexpr char SOH = '\x01'; // Start of Header
    static constexpr char EOT = '\x03'; // End of Transmission
};

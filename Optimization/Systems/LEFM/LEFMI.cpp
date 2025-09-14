/*********************************************************************
**++
======================================================================

 COPYRIGHT (c) 2025 BY Kristina Livesay

 ALL RIGHTS RESERVED

======================================================================
**  MODULE NAME:
**   LEFMI.cpp
**
**  PURPOSE:
**   Implementation of LEFMI main application class
**
**  DESCRIPTION:
**   Implements the top-level lifecycle of LEFMI. It initializes a TCP
**   connection to the LEFM device, enters a read/parse loop, and hands
**   data classification to BufferData. LEFMI is designed to run for
**   years as a background daemon, gracefully recovering from connection
**   errors, suspicious/corrupt data, or partial transmissions.
**
======================================================================
**********************************************************************/

#include "LEFMI.h"
#include <iostream>
#include <thread>
#include <chrono>

LEFMI::LEFMI() : running(false) {}

LEFMI::~LEFMI()
{
    vShutdown();
}

int LEFMI::nInitialize(const std::string& hostname, int port)
{
    // Open socket connection to LEFM
    int status = lefmConnection.nOpenConnectionToLEFM(hostname, port);
    if (status != EXIT_SUCCESS) {
        std::cerr << "[LEFMI] Failed to open connection to LEFM." << std::endl;
        return EXIT_FAILURE;
    }

    running = true;
    std::cout << "[LEFMI] Initialization successful. Connection established." << std::endl;
    return EXIT_SUCCESS;
}

void LEFMI::vRun()
{
    if (!running) {
        std::cerr << "[LEFMI] Cannot run, LEFMI not initialized." << std::endl;
        return;
    }

    while (running)
    {
        // Try to read from LEFM
        int bytesRead = lefmConnection.nReadFromLEFM();
        if (bytesRead <= 0) {
            std::cerr << "[LEFMI] Lost connection or no data. Retrying..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }

        // Process what we just read
        if (nProcessData() != EXIT_SUCCESS) {
            std::cerr << "[LEFMI] Error processing data." << std::endl;
        }

        // Prevent busy loop
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
}

void LEFMI::vShutdown()
{
    if (running) {
        lefmConnection.nCloseConnectionToLEFM();
        running = false;
        std::cout << "[LEFMI] Shutdown complete." << std::endl;
    }
}

int LEFMI::nProcessData()
{
    // Classify data inside BufferData
    int status = bufferManager.nSelectData();

    switch (status)
    {
    case ALL_DATA_RECEIVED:
        std::cout << "[LEFMI] Received full valid data." << std::endl;
        vHandleBuffers();
        break;

    case INCOMPLETE_DATA_RECEIVED:
        std::cout << "[LEFMI] Received incomplete data, storing for optimization." << std::endl;
        break;

    case ALL_DATA_RECEIVED_PLUS_SOME:
        std::cout << "[LEFMI] Received extra data beyond full message, handling." << std::endl;
        vHandleBuffers();
        break;

    case CORRUPT_DATA_POSSIBILITY:
        std::cerr << "[LEFMI] Suspicious or corrupted data detected." << std::endl;
        break;

    default:
        std::cerr << "[LEFMI] Unknown data state." << std::endl;
        break;
    }

    return EXIT_SUCCESS;
}

void LEFMI::vHandleBuffers()
{
    // Retrieve GOOD buffer
    std::vector<std::string> goodData;
    bufferManager.vGetGOODBuffer(&goodData);

    for (const auto& msg : goodData) {
        std::cout << "[LEFMI] Processing GOOD message: " << msg << std::endl;
        // TODO: real processing logic goes here
    }
}

// Logger.h

#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <mutex>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "Sensor.h"

namespace LogUtils
{
    inline std::string getTimestamp() 
    {
        auto now = std::chrono::system_clock::now();
        auto timeT = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&timeT);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    inline std::string averageData(float s1, float s2) 
    {
        float avg = (s1 + s2) / 2.0f;
        std::ostringstream oss;
        oss << getTimestamp() << "   "
            << "Sensor " << SENSOR_1 << ": " << std::fixed << std::setprecision(2) << s1 << " °C, "
            << "Sensor " << SENSOR_2 << ": " << s2 << " °C, "
            << "Average: " << avg << " °C\n";
        return oss.str();
    }

    // Generates formatted message for a faulty sensor reading
    inline std::string faultyData(float s, size_t index) 
    {
        std::ostringstream data; 
        data << getTimestamp()
                << "   "
                << " Fault Detected! Sensor: " 
                << index << " " 
                << s << "  °C (OUT OF RANGE)\n";

        return data.str();
    }
}

// Logger class handles thread-safe logging of sensor data
class Logger 
{
public:
    // Constructor: initializes logger with file and module type
    Logger(const std::string& filename, std::ios_base::openmode mode = std::ios::app) 
        : logFile_(filename, mode) // mode currently appends, use std::ios::out to override
    {
        if (!logFile_.is_open()) 
        {
            std::cerr << "Failed to open log file: " << filename << "\n";
        }

        std::cout << filename << " logging...\n";
    }

    // Destructor: closes the log file
    ~Logger() 
    {
        logFile_.close();
    }

    // Logs a formatted message to file (thread-safe)
    void logSensors(const std::string& message)
    {
        if (message.empty() || !logFile_.is_open())
            return;

        std::lock_guard<std::mutex> lock(logMutex_);
        logFile_ << message;
        logFile_.flush();
    }

    // Prevent copying or moving the Logger (not safe due to file and mutex)
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

private:
    std::ofstream logFile_;
    std::mutex logMutex_;
};

#endif // LOGGER_H

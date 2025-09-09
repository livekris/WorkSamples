#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <random>
#include <chrono>
#include <iomanip>
#include <atomic>
#include <csignal>
#include <array>

// Project-specific headers (to define Sensor class and Logger class/enums)
#include "Sensor.h"
#include "Logger.h"

// Global synchronization variables
std::condition_variable stopCondition; // Used to notify threads to stop
std::mutex stopMutex; // Protects condition variable wait
std::atomic<bool> stopRequested{false}; // Flag to indicate shutdown

// Shared atomic array for current sensor readings
std::array<std::atomic<float>, SENSOR_COUNT> sensorReading;

// Signal handler to request stop
void signalHandler(int signum) {
    std::cout << "\nSignal (" << signum << ") received, stopping...\n";
    stopRequested.store(true);
    stopCondition.notify_all();
}

// Function declarations
void SensorSimulator(Sensor& sensor, std::size_t index); // produces sensor values
void ControlModule(); // consumes sensor values and logs sensor values along with the average in a file
void FaultDetector(); // consumes sensor values and logs faulty sensor values (outside of MIN_TEMP and MAX_TEMP bounds)

// Main function
int main(int argc, char* argv[]) 
{
    std::cout << "Starting temperature monitoring system...\n";

    // Setup signal handler for Ctrl+C (SIGINT)
    std::signal(SIGINT, signalHandler);

    // Create two sensors with different noise characteristics
    Sensor sensor1(50.0, 2.0); // Base temp 50.0, noise 2.0
    Sensor sensor2(50.0, 3.0); // Base temp 50.0, noise 3.0

    // Launch threads
    std::thread s1Thread(SensorSimulator, std::ref(sensor1), SENSOR_1);
    std::thread s2Thread(SensorSimulator, std::ref(sensor2), SENSOR_2);

    std::thread controlThread(ControlModule);
    std::thread faultThread(FaultDetector);

    // Wait here until stopRequested is true
    {
        std::unique_lock<std::mutex> lock(stopMutex);
        stopCondition.wait(lock, [] { return stopRequested.load(); });
    }

    // Join threads
    s1Thread.join();
    s2Thread.join();
    controlThread.join();
    faultThread.join();

    std::cout << "Monitoring ended. Check log files.\n";
    return 0;
}

bool should_stop() 
{
        std::unique_lock<std::mutex> lock(stopMutex); // will unlock when it returns
        return stopCondition.wait_for(lock, std::chrono::seconds(1), [] { return stopRequested.load(); });
}

// Sensor simulator thread
void SensorSimulator(Sensor& sensor, std::size_t index) 
{
    std::cout << "Sensor Simulator started for sensor " << index << "\n";

    if (index >= SENSOR_COUNT) 
    {
        std::cerr << "Sensor index out of bounds\n";
        return;
    }

    sensorReading[index].store(SENSOR_INIT);

    while (true) 
    {
        if (should_stop())
            break;

        float value = sensor.next(); // new simulated value
        sensorReading[index].store(value);
    }

    std::cout << "Sensor Simulator stopping for sensor " << index << "\n";
}

void ControlModule() 
{
    Logger logModule("temperature_log.txt");
    
    while (true) 
    {
        if (should_stop())
            break;

        float s1 = sensorReading[SENSOR_1].load();
        float s2 = sensorReading[SENSOR_2].load();

        logModule.logSensors(LogUtils::averageData(s1, s2));
    }
}

void FaultDetector()
{
    Logger logModule("fault_log.txt");
    
    while (true) 
    {
        if (should_stop())
            break;

        float s1 = sensorReading[SENSOR_1].load();
        float s2 = sensorReading[SENSOR_2].load();

        std::string logEntry;
        if (s1 < MIN_TEMP || s1 > MAX_TEMP)
            logEntry += LogUtils::faultyData(s1, SENSOR_1);
        if (s2 < MIN_TEMP || s2 > MAX_TEMP)
            logEntry += LogUtils::faultyData(s2, SENSOR_2);

        logModule.logSensors(logEntry);
    }
}


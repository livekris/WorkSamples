# Temperature Monitoring System

## Overview

This project simulates a temperature monitoring system using multithreading in C++. It uses two sensor simulators running concurrently, logging temperature readings, detecting faults when readings fall outside predefined thresholds, and safely sharing sensor data across threads using atomic variables.

## Features

- Simulates two sensors generating temperature readings with normal distributions.
- Uses `std::atomic<float>` for thread-safe sensor data sharing.
- Logs temperature readings every second.
- Detects faults when sensor readings go beyond defined temperature ranges.
- Clean shutdown mechanism using an atomic stop flag.
- Thread-safe logging and graceful shutdown using signals.
- Modular design separating sensor simulation, control, and fault detection.

## File Structure

├── CMakeLists.txt # CMake build configuration
├── main.cpp # Main program
├── Sensor.h # Sensor class
├── Logger.h # Logger class and utility functions
├── temperature_log.txt # Output log for sensor readings (generated at runtime)
└── fault_log.txt # Output log for faulty readings (generated at runtime)

## Prerequisites

- C++17 compatible compiler (e.g., g++, clang++, MSVC)
- CMake 3.15 or higher
- Visual Studio Code (optional)
- C++ extensions for VSCode (e.g., CMake Tools, C/C++)

## Compilation using CMake

1. Create a build directory:

```bash
mkdir build
cd build

cmake ..

cmake --build .

./Simulator   # Linux/macOS
Simulator.exe # Windows

## Compile with g++

```bash
g++ -std=c++11 -pthread temperature_monitor.cpp -o temperature_monitor
./Simulator

## Output Example
temperature_log.txt
2025-08-26 15:42:12   Sensor 0: 51.23 °C, Sensor 1: 49.87 °C, Average: 50.55 °C

fault_log.txt
2025-08-26 15:42:18   Fault Detected! Sensor: 1 72.31  °C (OUT OF RANGE)



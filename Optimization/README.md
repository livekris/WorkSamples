
# Optimization: EuToMV 
C++ code that defines a class CEUtoMv, which performs conversion from engineering units (EU) to millivolts (mV) using interpolation, range handling, and multithreaded dataset generation. This class maps an engineering-unit measurement (EU) to millivolts (mV) using a two-stage interpolation (coarse + fine), with threaded dataset generation for efficiency, and an optional cross-check with an external calibration function (mv2eu).

float CEUtoMv::FindPoint(...);
This is the core function that converts an EU value into mV:
Adjust voltage range
    If units are millivolts (v_range_u == 0), range is limited to ±10,000.
    If volts, range is limited to ±10.
Generate wide dataset (m_vWp):
    Calls GetDataSet over the full range.
Rough interpolation
    Interpolates a first guess (fMv).
Determine narrow range width (s)
    Chooses precision based on total range size.
Generate narrow dataset (m_vNp)
    Focuses around estimated point fMv ± s.
Final interpolation
    Produces a refined result s.
Optional validation
    Calls mv2eu to double-check conversion and write to fOut.

void CEUtoMv::AddPoint(const vec3& v, bool IsWidePoint)
Uses multithreading (std::async) to generate interpolation points.
Splits work into NUM_THREADS chunks.
For each sampled voltage (fVal), calls mv2eu to convert to EU and stores (value, fVal, index) as a vec3.
Returns after merging all results.

float CEUtoMv::GetInterpolatedPoint(float fVal, std::vector<vec3> vp);
Sorts points (vps) by EU value.
Finds bounding indices around fVal.
Calls linear() for interpolation between bounding points.

float CEUtoMv::linear(float x, const vec3& p1, const vec3& p2);
Computes slope (m) and intercept (b).
Returns interpolated millivolt value.


# Optimization: EuToMV  with Brent's Method
## Overview

This C++ module provides a fast and accurate method to convert engineering unit (EU) values to millivolts (mV) using a hybrid approach:

1. **Compile-time interpolation:** Generates an initial guess quickly using a precomputed lookup table.
2. **Brent’s method:** Refines the result to high precision while guaranteeing convergence.

The combination ensures both **speed** and **robustness**.

## Features

- Precomputed constexpr lookup table for fast interpolation.
- Brent’s root-finding method for accurate convergence.
- Fixed-size arrays; no dynamic memory allocation.
- Narrow bracket search for efficient runtime computation.
- Easy to integrate into any project.

## Requirements

- C++17 or later
- Standard C++ libraries: `<iostream>`, `<array>`, `<cmath>`, `<stdexcept>`, `<algorithm>`

## Usage

1. **Define your device response** function (mV → EU):

``cpp
constexpr double DeviceResponse(double mv) {
    return 2.0 * mv + 0.5 * std::sin(mv); // Example nonlinear mapping
}

2. convert EU to MV
double targetEU = 5.0;
double mv = EUtoMv(targetEU);
std::cout << "Target EU: " << targetEU << " -> mV: " << mv << std::endl;

3. Optional: Adjust voltage range and tolerance:
double mv = EUtoMv(targetEU, 0.0, 10.0, 1e-6, 1000);

# Optimization: EuToMV with Brent's Method (CUDA)

## Overview
GPU-accelerated module to convert an array of engineering unit (EU) values to millivolts (mV) using:
1. Optional precomputed interpolation (can be added for speed).
2. Brent’s root-finding method per value.
3. Fully parallelized across GPU threads.

## Features
- Each EU→mV conversion is independent and runs on the GPU.
- Brent’s method ensures robust convergence for each value.
- Handles large batches of EU values efficiently.
- Simple CUDA kernel with host-device memory management.

## Requirements
- NVIDIA GPU with CUDA support
- CUDA Toolkit
- C++11 or later
- Standard C++ libraries: <iostream>, <cmath>

## Usage
1. Define the device response function on the GPU:

``cpp
__device__ double DeviceResponse(double mv) {
    return 2.0 * mv + 0.5 * sin(mv); // Example mapping
}

2. Allocate input/output arrays on host and device, then launch the kernel:
int threads = 256;
int blocks = (N + threads - 1) / threads;
EUtoMvBatch<<<blocks, threads>>>(d_targetEU, d_outMV, N);

3. Copy results back to host and use:
cudaMemcpy(h_outMV, d_outMV, N * sizeof(double), cudaMemcpyDeviceToHost);

Notes
Each thread computes one EU→mV conversion.
For higher performance, consider using precomputed lookup tables in GPU constant memory.
Brent’s method may return -1 if the root is not bracketed; ensure the voltage range covers all expected EU values.
Adjust maxIter and tol for precision requirements.

# Systems and networks: LEFM

**LEFMI** (Low Edge Flow Meter Interface) is a long-running daemon that communicates with an external **LEFM device** over TCP/IP.  
It is designed to run in the background for years without restart, ensuring reliable data acquisition, recovery from network hiccups, and safe classification of potentially fragmented or suspicious messages.

The project is built around **three main components**:

- **LEFMI**  
  Top-level application class. Manages the lifecycle of the daemon:
  - Opens/closes TCP connection (`LEFMData`)
  - Reads incoming data
  - Classifies/parses messages (`BufferData`)
  - Provides recovery logic for suspicious or corrupted transmissions

- **LEFMData**  
  Handles socket communication with the LEFM device:
  - `nOpenConnectionToLEFM()` – establishes a TCP connection
  - `nReadFromLEFM()` – reads data with `select()` and buffering
  - `nCloseConnectionToLEFM()` – gracefully closes the socket

- **BufferData**  
  Manages incoming message buffers and parsing:
  - Identifies start/end markers (`\x01` = SOH, `\x03` = EOT)
  - Distinguishes **GOOD**, **INCOMPLETE**, **SUSPICIOUS**, or **CORRUPT** data
  - Provides getters to retrieve fully reconstructed messages
  
## Build Instructions

### Prerequisites
- C++17 or newer
- POSIX sockets (Linux / Unix environment)
- Standard Make / g++ toolchain

### Compile
``bash
g++ -std=c++17 -Wall -O2 -o lefmi \
    BufferData.cpp LEFMData.cpp LEFMI.cpp main.cpp
  
./lefmi
  

# Notes: low level optimization tricks

🌟 Optimize temporal locality: reuse the same data soon after you first access it.

🌟 Optimize spatial cache locality and memory access pattern. For example this: 
for (x = 0; x < maxx; x++)
    for (y = 0; y < maxy; y++)
        do_something(a[x][y]);
outperforms this:
for (y = 0; y < maxy; y++)
    for (x = 0; x < maxx; x++)
        do_something(a[x][y]);
In most languages like C, C++, and Java, A 2D array a[x][y] is actually store row-major, meaning, all elements of a row are continguous in memory. Modern processors load blocks of memory (cache lines) into the CPU cache, so if you access a[0][0], the CPU will likely load a[0][0] through a[0][7] into cache at once. All elements of a row are contiguous in memory.

🌟 Shifting and masking by powers of two is cheaper t han division and remainder

🌟 Picking a power of two for filters etc:
    - If the data size is 2^n then input = input & Hex(2^n - 1) will truncate everything above 2^n
    
🌟 Judicious cashing (trade memory for speed): careful for staleness, invalidation and size. 

🌟 ++i can be faster than i++ (although modern compilers perhaps fix this)

🌟 Template metaprogramming or constexpr to calculate things at compile time instead of run-time

🌟 Don't do loop unrolling or Duff's device but instead make you loops as small as possible, as compilers can do that for you.

🌟 Try to keep the number of variables in use at any one time down. 

🌟 Try to never go out of the L2 cache.

🌟 Get rid of branches when possible and opt for arithmetic and bitwise operations: if (x < 0) x = -x;
replace with:
    x = (x ^ (x >> 31)) - (x >> 31);  // assuming 32-bit signed int

max = (a > b) ? a : b; replace with max = a ^ ((a ^ b) & -(a < b));

🌟 Always profile before optimzing: readability is important too

🌟 Pointer style are slightly faster: (iter(arr) + next) is faster that arr[i] but only use if profiling shows a real hotspot

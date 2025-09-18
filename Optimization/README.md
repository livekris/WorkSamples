
# Optimization: EuToMV 
C++ code that defines a class CEUtoMv, which performs conversion from engineering units (EU) to millivolts (mV) using interpolation, range handling, and multithreaded dataset generation.


# Optimization: EuToMV  with Brent's Method
This C++ module provides a fast and accurate method to convert engineering unit (EU) values to millivolts (mV) using a hybrid approach:

# Optimization: EuToMV with Brent's Method (CUDA)
GPU-accelerated module to convert an array of engineering unit (EU) values to millivolts (mV) using:
1. Optional precomputed interpolation (can be added for speed).
2. Brent’s root-finding method per value.
3. Fully parallelized across GPU threads.

# Systems and networks: LEFM
**LEFMI** (Low Edge Flow Meter Interface) is a long-running daemon that communicates with an external **LEFM device** over TCP/IP.  
It is designed to run in the background for years without restart, ensuring reliable data acquisition, recovery from network hiccups, and safe classification of potentially fragmented or suspicious messages.

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

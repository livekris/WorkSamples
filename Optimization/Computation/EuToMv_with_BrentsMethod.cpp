
#include <iostream>
#include <array>
#include <cmath>
#include <stdexcept>
#include <algorithm>

// =====================================================
// Mock DeviceResponse function
// =====================================================
// Simulates the sensor/device mapping: mV -> EU.
// Replace with your actual LEFM/device model.
constexpr double DeviceResponse(double mv) {
    return 2.0 * mv + 0.5 * std::sin(mv);  // example nonlinear response
}

// =====================================================
// Compile-time lookup table for interpolation
// =====================================================
// Precompute mV -> EU samples for fast initial guess
constexpr int SAMPLES = 21;
constexpr double MV_MIN = 0.0;
constexpr double MV_MAX = 10.0;

// Compute step size
constexpr double STEP = (MV_MAX - MV_MIN) / (SAMPLES - 1);

// Precompute EU values at compile-time
constexpr std::array<double, SAMPLES> PrecomputedMV = [](){
    std::array<double, SAMPLES> arr = {};
    for (int i = 0; i < SAMPLES; ++i) {
        arr[i] = MV_MIN + i * STEP;
    }
    return arr;
}();

constexpr std::array<double, SAMPLES> PrecomputedEU = [](){
    std::array<double, SAMPLES> arr = {};
    for (int i = 0; i < SAMPLES; ++i) {
        arr[i] = DeviceResponse(MV_MIN + i * STEP);
    }
    return arr;
}();

// =====================================================
// Fast compile-time interpolation guess
// =====================================================
double InterpolatedGuess(double targetEU) {
    // Search for bounding interval in precomputed table
    for (int i = 1; i < SAMPLES; ++i) {
        if ((targetEU >= PrecomputedEU[i-1] && targetEU <= PrecomputedEU[i]) ||
            (targetEU <= PrecomputedEU[i-1] && targetEU >= PrecomputedEU[i])) {
            // Linear interpolation
            double x1 = PrecomputedEU[i-1], y1 = PrecomputedMV[i-1];
            double x2 = PrecomputedEU[i],   y2 = PrecomputedMV[i];
            return y1 + (targetEU - x1) * (y2 - y1) / (x2 - x1);
        }
    }
    // Fallback: midpoint of full range
    return (MV_MIN + MV_MAX) / 2.0;
}

// =====================================================
// Hybrid Brent's method for EU -> mV
// =====================================================
double EUtoMv(double targetEU, double lower = MV_MIN, double upper = MV_MAX,
              double tol = 1e-6, int maxIter = 1000)
{
    auto f = [&](double mv) { return DeviceResponse(mv) - targetEU; };

    // Step 1: initial guess from fast interpolation
    double guess = InterpolatedGuess(targetEU);

    // Step 2: define a narrow bracket around guess
    double a = std::max(lower, guess - 1.0);
    double b = std::min(upper, guess + 1.0);

    double fa = f(a);
    double fb = f(b);

    // If bracket not valid, fallback to full range
    if (fa * fb >= 0.0) {
        a = lower; b = upper;
        fa = f(a); fb = f(b);
        if (fa * fb >= 0.0)
            throw std::runtime_error("Root not bracketed");
    }

    // Initialize Brent's method variables
    double c = a, fc = fa;
    bool mflag = true;
    double s = b, d = 0;

    // Step 3: Brent iteration loop
    for (int iter = 0; iter < maxIter; ++iter) {

        // Use Inverse Quadratic Interpolation or Secant
        if (fa != fc && fb != fc) {
            s = (a*fb*fc)/((fa-fb)*(fa-fc)) +
                (b*fa*fc)/((fb-fa)*(fb-fc)) +
                (c*fa*fb)/((fc-fa)*(fc-fb));
        } else {
            s = b - fb*(b-a)/(fb-fa);  // Secant
        }

        // Step 4: conditions to fall back to bisection
        if ((s < (3*a+b)/4 || s > b) ||
            (mflag && std::fabs(s-b) >= std::fabs(b-c)/2) ||
            (!mflag && std::fabs(s-b) >= std::fabs(c-d)/2) ||
            (mflag && std::fabs(b-c) < tol) ||
            (!mflag && std::fabs(c-d) < tol))
        {
            s = (a + b) / 2.0;
            mflag = true;
        } else {
            mflag = false;
        }

        // Step 5: evaluate function at new point
        double fs = f(s);

        // Step 6: update variables for next iteration
        d = c; c = b; fc = fb;
        if (fa * fs < 0) {
            b = s; fb = fs;
        } else {
            a = s; fa = fs;
        }

        if (std::fabs(fa) < std::fabs(fb)) {
            std::swap(a, b);
            std::swap(fa, fb);
        }

        // Step 7: check convergence
        if (std::fabs(b - a) < tol)
            return b;
    }

    throw std::runtime_error("Brent's method did not converge");
}

// =====================================================
// Main: Test the hybrid approach
// =====================================================
int main() {
    try {
        double targetEU = 5.0;  // Example target EU
        double mv = EUtoMv(targetEU);
        std::cout << "Target EU: " << targetEU << " -> mV: " << mv << std::endl;
        std::cout << "Check DeviceResponse(mv): " << DeviceResponse(mv) << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

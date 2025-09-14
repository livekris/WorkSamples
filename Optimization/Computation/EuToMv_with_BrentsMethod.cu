#include <cuda_runtime.h>
#include <iostream>
#include <cmath>

__device__ double DeviceResponse(double mv) {
    return 2.0 * mv + 0.5 * sin(mv); // Example
}

// Brent’s method simplified for device
__device__ double EUtoMvKernel(double targetEU, double lower, double upper, double tol=1e-6, int maxIter=100) {
    auto f = [&](double mv) { return DeviceResponse(mv) - targetEU; };
    
    double a = lower, b = upper;
    double fa = f(a), fb = f(b);
    if (fa * fb >= 0.0) return -1; // indicate failure

    double c = a, fc = fa;
    bool mflag = true;
    double s = b, d = 0;

    for (int iter = 0; iter < maxIter; ++iter) {
        if (fa != fc && fb != fc)
            s = (a*fb*fc)/((fa-fb)*(fa-fc)) +
                (b*fa*fc)/((fb-fa)*(fb-fc)) +
                (c*fa*fb)/((fc-fa)*(fc-fb));
        else
            s = b - fb*(b-a)/(fb-fa);

        if ((s < (3*a+b)/4 || s > b) ||
            (mflag && fabs(s-b) >= fabs(b-c)/2) ||
            (!mflag && fabs(s-b) >= fabs(c-d)/2) ||
            (mflag && fabs(b-c) < tol) ||
            (!mflag && fabs(c-d) < tol)) {
            s = (a+b)/2;
            mflag = true;
        } else mflag = false;

        double fs = f(s);
        d = c; c = b; fc = fb;

        if (fa * fs < 0) { b = s; fb = fs; }
        else { a = s; fa = fs; }

        if (fabs(b-a) < tol) return b;
    }
    return -1; // did not converge
}

// CUDA kernel
__global__ void EUtoMvBatch(const double* targetEU, double* outMV, int N) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= N) return;

    double target = targetEU[idx];
    outMV[idx] = EUtoMvKernel(target, 0.0, 10.0);
}

int main() {
    const int N = 1000;
    double h_targetEU[N], h_outMV[N];

    // Initialize some target EU values
    for (int i = 0; i < N; ++i) h_targetEU[i] = double(i) * 0.01;

    // Allocate device memory
    double *d_targetEU, *d_outMV;
    cudaMalloc(&d_targetEU, N * sizeof(double));
    cudaMalloc(&d_outMV, N * sizeof(double));

    cudaMemcpy(d_targetEU, h_targetEU, N * sizeof(double), cudaMemcpyHostToDevice);

    // Launch kernel
    int threads = 256;
    int blocks = (N + threads - 1) / threads;
    EUtoMvBatch<<<blocks, threads>>>(d_targetEU, d_outMV, N);

    // Copy results back
    cudaMemcpy(h_outMV, d_outMV, N * sizeof(double), cudaMemcpyDeviceToHost);

    // Print a few results
    for (int i = 0; i < 10; ++i)
        std::cout << "EU: " << h_targetEU[i] << " -> mV: " << h_outMV[i] << std::endl;

    cudaFree(d_targetEU);
    cudaFree(d_outMV);
}

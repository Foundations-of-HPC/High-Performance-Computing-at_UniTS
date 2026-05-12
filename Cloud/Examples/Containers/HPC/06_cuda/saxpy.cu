#include <stdio.h>
#include <cuda_runtime.h>

#define N (1 << 27)   // 128M elements = 512 MB per array

__global__ void saxpy(int n, float a, float *x, float *y) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) y[i] = a * x[i] + y[i];
}

int main() {
    float *d_x, *d_y;
    cudaMalloc(&d_x, N * sizeof(float));
    cudaMalloc(&d_y, N * sizeof(float));
    cudaMemset(d_x, 1, N * sizeof(float));
    cudaMemset(d_y, 1, N * sizeof(float));

    // Warm-up
    saxpy<<<(N+255)/256, 256>>>(N, 2.0f, d_x, d_y);
    cudaDeviceSynchronize();

    // Timed run
    cudaEvent_t start, stop;
    cudaEventCreate(&start); cudaEventCreate(&stop);
    cudaEventRecord(start);
    for (int i = 0; i < 100; i++)
        saxpy<<<(N+255)/256, 256>>>(N, 2.0f, d_x, d_y);
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    
    float ms;
    cudaEventElapsedTime(&ms, start, stop);
    double gbps = (3.0 * N * sizeof(float) * 100) / (ms * 1e-3) / 1e9;
    printf("SAXPY: N=%dM  time=%.2fms/iter  bandwidth=%.1f GB/s\n",
           N>>20, ms/100, gbps);
    
    cudaFree(d_x); cudaFree(d_y);
    return 0;
}

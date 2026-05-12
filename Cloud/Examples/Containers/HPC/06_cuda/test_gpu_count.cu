#include <stdio.h>
#include <cuda_runtime.h>
int main() {
    int n;
    cudaGetDeviceCount(&n);
    printf("Visible GPUs: %d\n", n);
    for (int i = 0; i < n; i++) {
        cudaDeviceProp p;
        cudaGetDeviceProperties(&p, i);
        printf("  Device %d: %s\n", i, p.name);
    }
    return 0;
}

CC      = gcc
CFLAGS  = -O3 -march=native -fopenmp -Wall -Wextra
LDFLAGS = -fopenmp
TARGET  = mc_pi_omp
 
$(TARGET): mc_pi_omp.c
	$(CC) $(CFLAGS) -o $(TARGET) mc_pi_omp.c $(LDFLAGS)
 
clean:
	rm -f $(TARGET)
 
.PHONY: clean

# Serial Mandelbrot baseline

This directory contains a serial C11 Mandelbrot renderer for Exercise 4 of the HPC final-exam project.  It avoids all MPI and OpenMP details and computes every pixel by direct scalar iteration.  The code is meant to be a decent starting point for the assignment, not a real HPC implementation.

## Files

- `mandelbrot_serial.c`: serial brute-force renderer and minimal PGM writer with 1 byte per channel (RGB).
- `Makefile`: builds the executable and runs a small smoke test.

To experiment writing a `pgm` file you can use the attached `write_ppm_image.c` which produces red, green and blue gradients.

## Build

```sh
make
```

This uses:

```sh
gcc -std=c11 -O2 -Wall -Wextra -pedantic mandelbrot_serial.c -lm -lz
```

For production, use the most appropriate set of compiler’s flags and options, and list them in the final report.

## Run

Default run:

```sh
./mandelbrot_serial
```

This renders the standard view at `1024 x 1024` pixels with `kmax = 1024` and
writes `mandelbrot.pgm`.

Example with a higher resolution:

```sh
./mandelbrot_serial --ppu 512 --kmax 2048 --output mandelbrot_2048.pgm
```

Example zoom:

```sh
./mandelbrot_serial \
  --xmin -0.76 --xmax -0.74 \
  --ymin  0.10 --ymax  0.12 \
  --ppu 50000 --kmax 5000 \
  --output zoom.pgm
```

## Command-line interface

The accepted options follow the exercise statement:

```text
--xmin VALUE        real coordinate of the upper-left corner      default -2.5
--ymax VALUE        imaginary coordinate of the upper-left corner default  2.0
--xmax VALUE        real coordinate of the bottom-right corner    default  1.5
--ymin VALUE        imaginary coordinate of the bottom-right      default -2.0
--ppu VALUE         pixels per unit length on the real axis       default 256
--dx-factor VALUE   accepted for compatibility                    default 8
--dy-factor VALUE   number of serial horizontal work stripes      default 8
--kmax VALUE        maximum iteration count                       default 1024
--output FILE       PGM output file                               default mandelbrot.pgm
--help              show help
```

`--dx-factor` is parsed so that the same launcher interface can later be used for the MPI/OpenMP versions.  In this serial Scenario-A-style baseline, only horizontal stripes are used, so `--dx-factor` has no effect.

## Output and verification aid

The program writes an 8-bit RGB PNG.  It also prints deterministic diagnostics, including an FNV-1a checksum of the raw iteration counts:

```text
iteration_checksum       ...
inside_pixels            ...
inside_fraction          ...
average_iterations       ...
total_iterations         ...
```

The checksum is useful for checking that a future parallel implementation reproduces the same image for the same view and `kmax`.

## Deliberately left as assignment material

The file contains hints but does not implement the main HPC optimisations:

- no OpenMP scheduling inside rows or stripes;
- no MPI producer-consumer dispatch;
- no SIMD batch kernel; [not covered in the course]
- no lane-refill strategy;
- no conjugate-symmetry shortcut;
- no cardioid or period-2 bulb tests;
- no Mariani-Silver subdivision.

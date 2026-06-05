/*
  Serial Mandelbrot renderer for the HPC final-exam exercise.

  This file is a clean serial baseline, whose goal is to give a correct
  and readable starting point from which introduce MPI work
  dispatch, OpenMP scheduling, optimization experiments, and different tile
  granularities.

  The program implements the command-line interface requested in Exercise 4,
  computes every pixel by direct iteration, stores the image in a row-major RGB
  buffer, and writes a PNG file through a tiny PNG writer based on zlib.

  Finally it provides some statistics as a mean of verification for the same
  case run in parallel
*/

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stb_image_write.h"


#define DEFAULT_XMIN       (-2.5)
#define DEFAULT_XMAX       (1.5)
#define DEFAULT_YMIN       (-2.0)
#define DEFAULT_YMAX       (2.0)
#define DEFAULT_PPU        (256u)
#define DEFAULT_DX_FACTOR  (8u)
#define DEFAULT_DY_FACTOR  (8u)
#define DEFAULT_KMAX       (1024u)
#define DEFAULT_OUTPUT     "mandelbrot.png"

#define FNV_OFFSET_BASIS   (1469598103934665603ULL)
#define FNV_PRIME          (1099511628211ULL)

/*
  Runtime options shared by the serial baseline and by the future parallel
  implementations.  The dx_factor entry is parsed for launcher compatibility
  with the assignment interface, but the serial stripe renderer below does not
  need it.
*/
typedef struct
{
  double            xmin;
  double            xmax;
  double            ymin;
  double            ymax;
  unsigned int      ppu;
  unsigned int      dx_factor;
  unsigned int      dy_factor;
  unsigned int      kmax;
  char             *output_path;
} options_t;

/*
  Image geometry after converting the continuous complex-plane window into a
  discrete pixel grid.  Pixels are addressed as (row, column), with row zero at
  the top of the image, consistently with the --ymax upper-left convention.
*/
typedef struct
{
  unsigned int      width;
  unsigned int      height;
  double            dx;
  double            dy;
} image_geometry_t;

/*
  Lightweight diagnostics printed at the end of the run.  The checksum is not
  a mathematical proof, but it is a useful reproducibility check: if two runs
  have the same options, compiler, and arithmetic behaviour, they should report
  the same value.
*/
typedef struct
{
  uint64_t          checksum;
  uint64_t          total_iterations;
  uint64_t          inside_pixels;
} render_stats_t;

/*
  Print the command-line interface.  The defaults follow Exercise 4: the
  standard view is [-2.5, 1.5] x [-2, 2], ppu = 256, and kmax = 1024, producing
  a 1024 x 1024 image.
*/
static void
print_usage (char     *program_name   // executable name from argv[0]
	     )
{
  fprintf (stderr,
           "Usage: %s [options]\n"
           "\n"
           "Options:\n"
           "  --xmin VALUE        real coordinate of the upper-left corner     (%.17g)\n"
           "  --ymax VALUE        imaginary coordinate of the upper-left corner (%.17g)\n"
           "  --xmax VALUE        real coordinate of the bottom-right corner   (%.17g)\n"
           "  --ymin VALUE        imaginary coordinate of the bottom-right corner (%.17g)\n"
           "  --ppu VALUE         pixels per unit length on the real axis      (%u)\n"
           "  --dx-factor VALUE   accepted for interface compatibility         (%u)\n"
           "  --dy-factor VALUE   number of serial horizontal work stripes     (%u)\n"
           "  --kmax VALUE        maximum Mandelbrot iteration count           (%u)\n"
           "  --output FILE       output PNG file                              (%s)\n"
           "  --help              show this help message\n"
           "\n"
           "Example:\n"
           "  %s --ppu 512 --kmax 2048 --output mandelbrot.png\n",
           program_name,
           DEFAULT_XMIN, DEFAULT_YMAX, DEFAULT_XMAX, DEFAULT_YMIN,
           DEFAULT_PPU, DEFAULT_DX_FACTOR, DEFAULT_DY_FACTOR,
           DEFAULT_KMAX, DEFAULT_OUTPUT, program_name);
}

/*
  Fill the options structure with assignment defaults.  This keeps main()
  small and gives a single place where students can see the default problem
  instance before changing command-line parsing or adding new options.
*/
static void
set_default_options (options_t   *options   // output options structure
		     )
{
  options->xmin      = DEFAULT_XMIN;
  options->xmax      = DEFAULT_XMAX;
  options->ymin      = DEFAULT_YMIN;
  options->ymax      = DEFAULT_YMAX;
  options->ppu       = DEFAULT_PPU;
  options->dx_factor = DEFAULT_DX_FACTOR;
  options->dy_factor = DEFAULT_DY_FACTOR;
  options->kmax      = DEFAULT_KMAX;
  options->output_path = DEFAULT_OUTPUT;
}

/*
  Parse a floating-point option of the form "--name value".  The current index
  is advanced to the value token so that the caller's for-loop can continue
  normally.  A small helper avoids duplicating error checks for xmin/xmax/ymin/ymax.
*/
static int
parse_double_option ( int      argc,    // number of command-line tokens
		      char   **argv,    // command-line token vector
		      int     *i,       // index of the option currently being parsed
		      double  *value    // destination for the parsed floating-point value
		      )
{
  char   *endptr;
  double  parsed;

  if (*i + 1 >= argc)
    {
      fprintf (stderr, "Missing value after %s\n", argv[*i]);
      return -1;
    }

  errno = 0;
  endptr = NULL;
  parsed = strtod (argv[*i + 1], &endptr);

  if (errno != 0 || endptr == argv[*i + 1] || *endptr != '\0' || !isfinite (parsed))
    {
      fprintf (stderr, "Invalid floating-point value for %s: %s\n",
               argv[*i], argv[*i + 1]);
      return -1;
    }

  *value = parsed;
  *i += 1;

  return 0;
}

/*
  Parse a positive unsigned option of the form "--name value".  It is used for
  ppu, factors, and kmax.  The baseline deliberately keeps the type simple:
  huge images are not the point of this serial starting code.
*/
static int
parse_unsigned_option ( int            argc,    // number of command-line tokens
			char         **argv,    // command-line token vector
			int           *i,       // index of the option currently being parsed
			unsigned int  *value    // destination for the parsed integer value
			)
{
  char          *endptr;
  unsigned long  parsed;

  if (*i + 1 >= argc)
    {
      fprintf (stderr, "Missing value after %s\n", argv[*i]);
      return -1;
    }

  errno = 0;
  endptr = NULL;
  parsed = strtoul (argv[*i + 1], &endptr, 10);

  if (errno != 0 || endptr == argv[*i + 1] || *endptr != '\0'
      || parsed == 0 || parsed > UINT_MAX)
    {
      fprintf (stderr, "Invalid positive integer value for %s: %s\n",
               argv[*i], argv[*i + 1]);
      return -1;
    }

  *value = (unsigned int) parsed;
  *i += 1;

  return 0;
}

/*
  Parse all command-line options.  The interface mirrors the one prescribed for
  both parallel scenarios, even though this serial code ignores the parallel
  meaning of dx_factor and uses dy_factor only as a convenient stripe count.
*/
static int
parse_command_line (int          argc,      // number of command-line tokens
		    char       **argv,      // command-line token vector
		    options_t   *options    // options structure to be modified
		    )
{
  int i;

  for (i = 1; i < argc; ++i)
    {
      if (strcmp (argv[i], "--help") == 0)
        {
          print_usage (argv[0]);
          return 1;
        }
      else if (strcmp (argv[i], "--xmin") == 0)
        {
          if (parse_double_option (argc, argv, &i, &options->xmin) != 0)
            return -1;
        }
      else if (strcmp (argv[i], "--xmax") == 0)
        {
          if (parse_double_option (argc, argv, &i, &options->xmax) != 0)
            return -1;
        }
      else if (strcmp (argv[i], "--ymin") == 0)
        {
          if (parse_double_option (argc, argv, &i, &options->ymin) != 0)
            return -1;
        }
      else if (strcmp (argv[i], "--ymax") == 0)
        {
          if (parse_double_option (argc, argv, &i, &options->ymax) != 0)
            return -1;
        }
      else if (strcmp (argv[i], "--ppu") == 0)
        {
          if (parse_unsigned_option (argc, argv, &i, &options->ppu) != 0)
            return -1;
        }
      else if (strcmp (argv[i], "--dx-factor") == 0)
        {
          if (parse_unsigned_option (argc, argv, &i, &options->dx_factor) != 0)
            return -1;
        }
      else if (strcmp (argv[i], "--dy-factor") == 0)
        {
          if (parse_unsigned_option (argc, argv, &i, &options->dy_factor) != 0)
            return -1;
        }
      else if (strcmp (argv[i], "--kmax") == 0)
        {
          if (parse_unsigned_option (argc, argv, &i, &options->kmax) != 0)
            return -1;
        }
      else if (strcmp (argv[i], "--output") == 0)
        {
          if (i + 1 >= argc)
            {
              fprintf (stderr, "Missing value after --output\n");
              return -1;
            }

          options->output_path = argv[i + 1];
          i += 1;
        }
      else
        {
          fprintf (stderr, "Unknown option: %s\n", argv[i]);
          print_usage (argv[0]);
          return -1;
        }
    }

  return 0;
}

/*
  Check that the parsed options describe a meaningful image and iteration
  problem.  The function also rejects dimensions that are likely to overflow
  later memory-size computations.
*/
static int
validate_options (options_t   *options   // already parsed options
		  )
{
  double x_extent;
  double y_extent;
  double width_estimate;
  double height_estimate;

  if (!(options->xmax > options->xmin))
    {
      fprintf (stderr, "Invalid view: --xmax must be larger than --xmin\n");
      return -1;
    }

  if (!(options->ymax > options->ymin))
    {
      fprintf (stderr, "Invalid view: --ymax must be larger than --ymin\n");
      return -1;
    }

  x_extent = options->xmax - options->xmin;
  y_extent = options->ymax - options->ymin;
  width_estimate = x_extent * (double) options->ppu;
  height_estimate = y_extent * (double) options->ppu;

  if (!isfinite (width_estimate) || !isfinite (height_estimate)
      || width_estimate < 1.0 || height_estimate < 1.0
      || width_estimate > (double) UINT_MAX
      || height_estimate > (double) UINT_MAX)
    {
      fprintf (stderr, "Invalid image dimensions implied by the view and --ppu\n");
      return -1;
    }

  return 0;
}

/*
  Convert the user-visible view definition into integer image dimensions and
  pixel spacings.  With the default values this gives exactly 1024 x 1024
  pixels.  For unusual views whose extent times ppu is not an integer, the
  dimension is rounded to the nearest pixel and the final spacing is adjusted
  slightly to hit the requested window boundaries.
*/
static int
make_geometry (options_t        *options,    // validated user options
	       image_geometry_t *geometry    // output image geometry
	       )
{
  double x_extent;
  double y_extent;
  double width_double;
  double height_double;

  x_extent = options->xmax - options->xmin;
  y_extent = options->ymax - options->ymin;
  width_double = floor (x_extent * (double) options->ppu + 0.5);
  height_double = floor (y_extent * (double) options->ppu + 0.5);

  if (width_double < 1.0 || height_double < 1.0
      || width_double > (double) UINT_MAX
      || height_double > (double) UINT_MAX)
    {
      fprintf (stderr, "Image dimensions are outside the supported range\n");
      return -1;
    }

  geometry->width = (unsigned int) width_double;
  geometry->height = (unsigned int) height_double;
  geometry->dx = x_extent / (double) geometry->width;
  geometry->dy = y_extent / (double) geometry->height;

  return 0;
}

/*
  Update a 64-bit FNV-1a checksum with one unsigned integer.  The checksum is
  computed from iteration counts, not from RGB bytes, so it is independent of
  palette changes. It can be used as a quick serial-vs-parallel check.
*/
static uint64_t
checksum_update_uint (uint64_t     checksum,    // current checksum state
		      unsigned int value        // iteration count to fold into the checksum
		      )
{
  unsigned int byte_id;

  for (byte_id = 0; byte_id < 4; ++byte_id)
    {
      checksum ^= (uint64_t) ((value >> (8u * byte_id)) & 0xffu);
      checksum *= FNV_PRIME;
    }

  return checksum;
}

/*
  Compute the Mandelbrot escape iteration for one point c = cr + i ci.  This
  is the direct scalar kernel, clean, keeps the operation count easy to read.
  This can be heavility optimized.
*/
static unsigned int
mandelbrot_escape (double       cr,      // real part of the pixel centre
		   double       ci,      // imaginary part of the pixel centre
		   unsigned int kmax     // maximum number of iterations
		   )
{
           double zr;
           double zi;
           double zr_next;
           double zr2;
           double zi2;
  unsigned int    k;

  zr = 0.0;
  zi = 0.0;
  k = 0;

  while (k < kmax)
    {
      zr2 = zr * zr;
      zi2 = zi * zi;
      
      if (zr2 + zi2 > 4.0)
        break;
      
      zr_next = zr2 - zi2 + cr;
      zi = 2.0 * zr * zi + ci;
      zr = zr_next;
      k += 1;
    }

  return k;
}

/*
  Map one iteration count to an RGB colour.  Interior points are black.  The
  outside palette is deliberately simple: a later optimisation exercise should
  not be distracted by advanced colouring or smooth-potential formulas.

  USEFUL IF YOU WANT RGB COLORS
  WHEN USING PPM IMAGES, the iteration count normalized to maximum ppm value is
  the choice
  
*/
static void
colour_from_iteration (unsigned int    iteration,    // escape iteration count
		       unsigned int    kmax,         // maximum iteration count
		       unsigned char  *rgb           // output triplet: red, green, blue
		       )
{
  double t;
  double one_minus_t;

  if (iteration >= kmax)
    {
      rgb[0] = 0u;
      rgb[1] = 0u;
      rgb[2] = 0u;
      return;
    }

  t = (double) iteration / (double) kmax;
  one_minus_t = 1.0 - t;

  rgb[0] = (unsigned char) (255.0 * 9.0 * one_minus_t * t * t * t);
  rgb[1] = (unsigned char) (255.0 * 15.0 * one_minus_t * one_minus_t * t * t);
  rgb[2] = (unsigned char) (255.0 * 8.5 * one_minus_t * one_minus_t * one_minus_t * t);
}

/*
  Render a contiguous horizontal range of rows.  This is a serial version of
  the stripe unit used in Scenario A: it is intentionally simple, but the shape
  of the function makes the future producer-consumer decomposition almost there.
*/
static void
render_stripe (options_t        *options,       // view and iteration options
	       image_geometry_t *geometry,      // image dimensions and pixel spacing
	       unsigned int      row_begin,     // first row included in this stripe
	       unsigned int      row_end,       // first row after this stripe
	       unsigned char    *image,         // complete row-major RGB image buffer
	       render_stats_t   *stats          // diagnostics accumulated over the image
	       )
{
  unsigned int row;
  unsigned int col;
  unsigned int iteration;
  uint64_t     local_checksum;
  uint64_t     local_total_iterations;
  uint64_t     local_inside_pixels;
  double       cr;
  double       ci;
  size_t       pixel_offset;

  local_checksum = stats->checksum;
  local_total_iterations = stats->total_iterations;
  local_inside_pixels = stats->inside_pixels;

  for (row = row_begin; row < row_end; ++row)
    {
      // Row zero is the top of the image, hence the minus sign from ymax.
      ci = options->ymax - ((double) row + 0.5) * geometry->dy;

      for (col = 0; col < geometry->width; ++col)
        {
          cr = options->xmin + ((double) col + 0.5) * geometry->dx;
          iteration = mandelbrot_escape (cr, ci, options->kmax);

          pixel_offset = ((size_t) row * (size_t) geometry->width
                          + (size_t) col) * 3u;
          colour_from_iteration (iteration, options->kmax, &image[pixel_offset]);

          local_checksum = checksum_update_uint (local_checksum, iteration);
          local_total_iterations += (uint64_t) iteration;

          if (iteration >= options->kmax)
            local_inside_pixels += 1u;
        }
    }

  stats->checksum = local_checksum;
  stats->total_iterations = local_total_iterations;
  stats->inside_pixels = local_inside_pixels;
}

/*
  Render the full image by visiting dy_factor horizontal stripes in order.  The
  striping is not a performance optimisation here; it keeps the serial baseline
  close to the decomposition described in the exercise while remaining a plain
  single-process program.
*/
static void
render_image (options_t        *options,     // view, factors, and kmax
	      image_geometry_t *geometry,    // image dimensions and pixel spacing
	      unsigned char    *image,       // complete row-major RGB image buffer
	      render_stats_t   *stats        // output diagnostics
	      )
{
  unsigned int stripe;
  unsigned int row_begin;
  unsigned int row_end;

  stats->checksum = FNV_OFFSET_BASIS;
  stats->total_iterations = 0u;
  stats->inside_pixels = 0u;

  for (stripe = 0; stripe < options->dy_factor; ++stripe)
    {
      row_begin = (unsigned int) (((uint64_t) stripe * geometry->height)
                                  / options->dy_factor);
      row_end = (unsigned int) (((uint64_t) (stripe + 1u) * geometry->height)
                                / options->dy_factor);

      if (row_begin < row_end)
        render_stripe (options, geometry, row_begin, row_end, image, stats);
    }
}

/*
  Write one 32-bit unsigned integer in big-endian order.  PNG uses network byte
  order for chunk lengths, dimensions, and CRC values, independently of the
  machine on which the renderer runs.
*/
static int
write_be32 (FILE     *file,    // already opened binary stream
	    uint32_t  value    // value to be written in big-endian order
	    )
{
  unsigned char bytes[4];

  bytes[0] = (unsigned char) ((value >> 24) & 0xffu);
  bytes[1] = (unsigned char) ((value >> 16) & 0xffu);
  bytes[2] = (unsigned char) ((value >> 8) & 0xffu);
  bytes[3] = (unsigned char) (value & 0xffu);

  if (fwrite (bytes, 1u, 4u, file) != 4u)
    return -1;

  return 0;
}

/*
  Write a single PNG chunk: length, four-byte type, optional data, and CRC.
  This small helper is enough for a basic true-colour PNG containing IHDR, one
  IDAT chunk, and IEND.  More sophisticated PNG filtering is deliberately left
  out to keep the I/O code readable.
*/
static int
write_png_chunk (FILE           *file,      // already opened binary PNG stream
		 char           *type,      // four-character chunk type such as "IHDR"
		 unsigned char  *data,      // chunk payload, or NULL for an empty chunk
		 uint32_t        length     // number of payload bytes
		 )
{
  uint32_t crc;

  if (write_be32 (file, length) != 0)
    return -1;

  if (fwrite (type, 1u, 4u, file) != 4u)
    return -1;

  if (length > 0u && fwrite (data, 1u, length, file) != length)
    return -1;

  crc = (uint32_t) crc32 (0L, Z_NULL, 0u);
  crc = (uint32_t) crc32 (crc, (Bytef *) type, 4u);

  if (length > 0u)
    crc = (uint32_t) crc32 (crc, (Bytef *) data, length);

  if (write_be32 (file, crc) != 0)
    return -1;

  return 0;
}

/*
  Write an unfiltered 8-bit RGB PNG file.  The input image is a tightly packed
  row-major array with three bytes per pixel.  Internally, PNG scanlines carry
  one leading filter byte per row; filter type zero means "no filter".
*/
static int
write_png_rgb8 (char           *path,      // output filename
		unsigned int    width,     // image width in pixels
		unsigned int    height,    // image height in pixels
		unsigned char  *rgb        // row-major RGB pixels, three bytes per pixel
)
{
  FILE             *file;
  unsigned char     signature[8];
  unsigned char     ihdr[13];
  unsigned char    *raw;
  unsigned char    *compressed;
  size_t            row;
  size_t            row_bytes;
  size_t            raw_size;
  size_t            source_offset;
  size_t            target_offset;
  uLong             raw_size_zlib;
  uLongf            compressed_size;
  int               z_status;
  int               write_status;

  row_bytes = (size_t) width;

  if (row_bytes > (SIZE_MAX - 1u) / 3u)
    {
      fprintf (stderr, "Image row is too large for this platform\n");
      return -1;
    }

  row_bytes = row_bytes * 3u + 1u;

  if ((size_t) height > SIZE_MAX / row_bytes)
    {
      fprintf (stderr, "Image is too large for this platform\n");
      return -1;
    }

  raw_size = (size_t) height * row_bytes;

  if (raw_size > (size_t) ULONG_MAX)
    {
      fprintf (stderr, "Image is too large for zlib on this platform\n");
      return -1;
    }

  raw = (unsigned char *) malloc (raw_size);
  if (raw == NULL)
    {
      fprintf (stderr, "Unable to allocate PNG scanline buffer (%zu bytes)\n",
               raw_size);
      return -1;
    }

  for (row = 0u; row < (size_t) height; ++row)
    {
      target_offset = row * row_bytes;
      source_offset = row * (size_t) width * 3u;

      raw[target_offset] = 0u;    // PNG filter type 0: no scanline filtering.
      memcpy (&raw[target_offset + 1u], &rgb[source_offset], (size_t) width * 3u);
    }

  raw_size_zlib = (uLong) raw_size;
  compressed_size = compressBound (raw_size_zlib);
  compressed = (unsigned char *) malloc ((size_t) compressed_size);

  if (compressed == NULL)
    {
      fprintf (stderr, "Unable to allocate compressed PNG buffer (%lu bytes)\n",
               (unsigned long) compressed_size);
      free (raw);
      return -1;
    }

  // Z_BEST_SPEED is enough for benchmark output and avoids turning I/O into a
  // compression exercise.  Students can experiment with filters and levels later.
  z_status = compress2 (compressed, &compressed_size, raw, raw_size_zlib,
                        Z_BEST_SPEED);

  if (z_status != Z_OK)
    {
      fprintf (stderr, "zlib compression failed with status %d\n", z_status);
      free (compressed);
      free (raw);
      return -1;
    }

  if (compressed_size > (uLongf) UINT32_MAX)
    {
      fprintf (stderr, "Compressed image does not fit in one PNG IDAT chunk\n");
      free (compressed);
      free (raw);
      return -1;
    }

  file = fopen (path, "wb");
  if (file == NULL)
    {
      fprintf (stderr, "Unable to open %s for writing: %s\n", path, strerror (errno));
      free (compressed);
      free (raw);
      return -1;
    }

  signature[0] = 137u;
  signature[1] = 80u;
  signature[2] = 78u;
  signature[3] = 71u;
  signature[4] = 13u;
  signature[5] = 10u;
  signature[6] = 26u;
  signature[7] = 10u;

  write_status = 0;

  if (fwrite (signature, 1u, 8u, file) != 8u)
    write_status = -1;

  ihdr[0] = (unsigned char) ((width >> 24) & 0xffu);
  ihdr[1] = (unsigned char) ((width >> 16) & 0xffu);
  ihdr[2] = (unsigned char) ((width >> 8) & 0xffu);
  ihdr[3] = (unsigned char) (width & 0xffu);
  ihdr[4] = (unsigned char) ((height >> 24) & 0xffu);
  ihdr[5] = (unsigned char) ((height >> 16) & 0xffu);
  ihdr[6] = (unsigned char) ((height >> 8) & 0xffu);
  ihdr[7] = (unsigned char) (height & 0xffu);
  ihdr[8] = 8u;     // 8 bits per channel.
  ihdr[9] = 2u;     // PNG colour type 2: true-colour RGB.
  ihdr[10] = 0u;    // Deflate compression method.
  ihdr[11] = 0u;    // Standard PNG filter method.
  ihdr[12] = 0u;    // No interlacing.

  if (write_status == 0 && write_png_chunk (file, "IHDR", ihdr, 13u) != 0)
    write_status = -1;

  if (write_status == 0
      && write_png_chunk (file, "IDAT", compressed, (uint32_t) compressed_size) != 0)
    write_status = -1;

  if (write_status == 0 && write_png_chunk (file, "IEND", NULL, 0u) != 0)
    write_status = -1;

  if (fclose (file) != 0)
    write_status = -1;

  free (compressed);
  free (raw);

  if (write_status != 0)
    {
      fprintf (stderr, "Failed while writing PNG file %s\n", path);
      return -1;
    }

  return 0;
}

/*
  Program entry point.  The workflow is intentionally linear and easy to map
  onto the exercise statement: parse options, build the image geometry, render
  the brute-force image, write a PNG, and print reproducibility diagnostics.
*/
int
main (int    argc,    // number of command-line tokens
      char **argv     // command-line token vector
      )
{
  options_t         options;
  image_geometry_t  geometry;
  render_stats_t    stats;
  unsigned char    *image;
  size_t            image_bytes;
  int               parse_status;
  double            inside_fraction;
  double            average_iterations;
  uint64_t          pixel_count;

  set_default_options (&options);
  parse_status = parse_command_line (argc, argv, &options);

  if (parse_status > 0)
    return EXIT_SUCCESS;

  if (parse_status < 0 || validate_options (&options) != 0
      || make_geometry (&options, &geometry) != 0)
    return EXIT_FAILURE;

  if ((size_t) geometry.width > SIZE_MAX / (size_t) geometry.height
      || (size_t) geometry.width * (size_t) geometry.height > SIZE_MAX / 3u)
    {
      fprintf (stderr, "Image is too large for this platform\n");
      return EXIT_FAILURE;
    }

  image_bytes = (size_t) geometry.width * (size_t) geometry.height * 3u;
  image = (unsigned char *) malloc (image_bytes);

  if (image == NULL)
    {
      fprintf (stderr, "Unable to allocate image buffer (%zu bytes)\n", image_bytes);
      return EXIT_FAILURE;
    }

  fprintf (stderr,
           "Rendering %u x %u pixels, kmax = %u, view = [%.17g, %.17g] x [%.17g, %.17g]\n",
           geometry.width, geometry.height, options.kmax,
           options.xmin, options.xmax, options.ymin, options.ymax);

  if (options.dx_factor != DEFAULT_DX_FACTOR)
    {
      fprintf (stderr,
               "Note: --dx-factor is accepted for assignment-interface compatibility; "
               "this serial stripe baseline does not use it.\n");
    }

  
  render_image (&options, &geometry, image, &stats);

  if (write_png_rgb8 (options.output_path, geometry.width, geometry.height, image) != 0)
    {
      free (image);
      return EXIT_FAILURE;
    }

  pixel_count        = (uint64_t) geometry.width * (uint64_t) geometry.height;
  inside_fraction    = (double) stats.inside_pixels / (double) pixel_count;
  average_iterations = (double) stats.total_iterations / (double) pixel_count;

  printf ("output_file              %s\n", options.output_path);
  printf ("width                    %u\n", geometry.width);
  printf ("height                   %u\n", geometry.height);
  printf ("kmax                     %u\n", options.kmax);
  printf ("iteration_checksum       %016llx\n", (unsigned long long) stats.checksum);
  printf ("inside_pixels            %llu\n", (unsigned long long) stats.inside_pixels);
  printf ("inside_fraction          %.17g\n", inside_fraction);
  printf ("average_iterations       %.17g\n", average_iterations);
  printf ("total_iterations         %llu\n", (unsigned long long) stats.total_iterations);

  free (image);

  return EXIT_SUCCESS;
}




#define _XOPEN_SOURCE 700  // ensures we're using latest standard

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define CPU_TIME ({struct  timespec ts; clock_gettime( CLOCK_PROCESS_CPUTIME_ID, &ts ),	\
                                          (double)ts.tv_sec +           \
                                          (double)ts.tv_nsec * 1e-9;})


typedef struct {
  double x,y,z;
  double m;
} particle_t;



int main( int argc, char** argv )
{
  
    const size_t N = ( argc > 1 ) ? strtoull(argv[1],0,10) : (size_t)10000;
    const int REPETITIONS = 3;

    particle_t *P = aligned_alloc ( 64, N*sizeof(particle_t) );
    double *x     = aligned_alloc ( 64, N*sizeof(double) );
    double *y     = aligned_alloc ( 64, N*sizeof(double) );
    double *z     = aligned_alloc ( 64, N*sizeof(double) );
    double *m     = aligned_alloc ( 64, N*sizeof(double) );
    if( !P || !x || !y || !z || !m) {
      perror("memory allocation failed");
      return 1; }

    printf ( "generating %zu particles.. ", N ); fflush(stdout);
    
    srand48( time(NULL) );
    for ( size_t i = 0; i < N; i++ )
      {
        double v0 = drand48() * N;
	double v1 = drand48() * N;
	double v2 = drand48() * N;
	double mass = v0+v1+v2;
	
        P[i]  = (particle_t){v0, v1, v2, mass };
        x[i] = v0; y[i] = v1; z[i] = v2; m[i] = mass;
    }

    printf ( "done\n"
	     "memory for particles data is: %5.1f MB\n"
	     "processing.. ",
	     N*sizeof(particle_t) / (1024.0*1024.0)
	     ), fflush(stdout);
    
    // warm-up the cache
    //
    volatile double sink=0;
    for ( size_t i = 0; i < N; i++ ) sink += P[i].x + x[i] + y[i] + z[i];

    double best_AoS=1e9, best_SoA=1e9;
    volatile double do_not_optimise_a = 0;
    volatile double do_not_optimise_b = 0;

    //
    // generate Nindexes unique indexes of particles
    // with which the target particle interacts
    //
    size_t  *Nindexes = (size_t*)malloc( N * sizeof(size_t));
    size_t **indexes  = (size_t**)malloc( N * sizeof(size_t*));
    
    for ( size_t i = 0; i < N; i++ ) {
      // generate list for the i-th particle
      
      Nindexes[i] = N/10 + lrand48() % (N/10);
      indexes[i] = (size_t*)malloc( Nindexes[i] * sizeof(size_t));
    
      for ( size_t j = 0; j < Nindexes[i]; j++ ) {
	// generate the Nindexes[i] for the i-th particles
	size_t sample;
	int add_index = 0;
	while ( !add_index )
	  // check that the news index has not already generated	  
	  {
	    sample   = lrand48()%N;
	    size_t k = 0;
	    while ( (k < j) && (indexes[i][k] != sample) )
	      k++;
	    add_index = (k == j);
	  }      
	indexes[i][j] = sample;
      }
    }
    

    
    for( int r = 0; r < REPETITIONS; r++ )
      {
	
        double     sum    = 0;
	particle_t source = P[0];
	
	double tstart = CPU_TIME;

	for ( size_t i = 1; i < N; i++ )
	  for ( size_t j = 1; j < Nindexes[i]; j++ )
	    {
	      size_t idx = indexes[i][j];
	     #if defined(MANHATTAN_DISTANCE)
	      double manhattan_distance = ( source.x - P[idx].x ) + ( source.y - P[idx].y ) + ( source.z - P[idx].z ) ;
	      sum += manhattan_distance;
	     #else
	      sum += P[idx].x;
	     #endif
	    }
	
	double timing = CPU_TIME - tstart;
	
        if ( timing < best_AoS)
	  { best_AoS = timing; do_not_optimise_a = sum;}
      }


    for( int r = 0; r < REPETITIONS; r++ )
      {
	
        double sum  = 0;
	double srcx = x[0];
	double srcy = y[0];
	double srcz = z[0];
	
	double tstart = CPU_TIME;
        for ( size_t i = 1; i < N; i++ )
	  for ( size_t j = 1; j < Nindexes[i]; j++ )
	    {
	      size_t idx = indexes[i][j];
	     #if defined(MANHATTAN_DISTANCE)
	      double manhattan_distance = ( srcx - x[i] ) + ( srcy - y[i] ) + ( srcz - z[i] ) ;
	      sum += manhattan_distance;
	     #else
	      sum += x[i];
	     #endif
	    }
	double timing = CPU_TIME - tstart;
	
        if ( timing < best_SoA)
	  { best_SoA = timing; do_not_optimise_b = sum;}
      }

   
    double bytes_AoS = (double)N*sizeof(particle_t); // realistically more traffic due to useless fields
    double bytes_SoA = (double)N*sizeof(double)*3
      ;
    printf ( "\n"
	     "AoS (sum x): %.3f s, < %.2f GB/s\n"
	     "SoA (sum x): %.3f s, %.2f GB/s\n"
	     "speedup: %3.2fx\n",	     
	     best_AoS, bytes_AoS/best_AoS/1e9,
	     best_SoA, bytes_SoA/best_SoA/1e9,
	     best_AoS / best_SoA);

   
    //fprintf(stderr,"[ printed: %.1f %.1f\n", do_not_optimise_a, do_not_optimise_b );

    free(P); free(x); free(y); free(z); free(m);
    return 0;
}


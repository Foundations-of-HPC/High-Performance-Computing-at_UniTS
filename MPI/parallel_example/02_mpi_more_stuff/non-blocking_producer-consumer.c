
/* ────────────────────────────────────────────────────────────────────────── *
│                                                                            │
│ This file is part of the exercises for the Lectures on                     │
│   "Foundations of High Performance Computing"                              │
│ given at                                                                   │
│   Master in HPC and                                                        │
│   Master in Data Science and Scientific Computing                          │
│ @ SISSA, ICTP and University of Trieste                                    │
│                                                                            │
│ contact: luca.tornatore@inaf.it                                            │
│                                                                            │
│     This is free software; you can redistribute it and/or modify           │
│     it under the terms of the GNU General Public License as published by   │
│     the Free Software Foundation; either version 3 of the License, or      │
│     (at your option) any later version.                                    │
│     This code is distributed in the hope that it will be useful,           │
│     but WITHOUT ANY WARRANTY; without even the implied warranty of         │
│     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          │
│     GNU General Public License for more details.                           │
│                                                                            │
│     You should have received a copy of the GNU General Public License      │
│     along with this program.  If not, see <http://www.gnu.org/licenses/>   │
│                                                                            │
* ────────────────────────────────────────────────────────────────────────── */

#if defined(__STDC__)
#  if (__STDC_VERSION__ >= 199901L)
#     define _XOPEN_SOURCE 700
#  endif
#endif
#include <stdlib.h>     
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <omp.h>
#include <mpi.h>


#define MSG_MAXSIZE  0
#define MSG_SIZE     1
#define MSG_DATA     2


double heavy_work( int );

int main( int argc, char **argv )
{

  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_SINGLE, &provided);
  if ( provided < MPI_THREAD_SINGLE )
    {
      // manage
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

  int Myrank, Ntasks;
  MPI_Comm myCOMM_WORLD;

  MPI_Comm_dup ( MPI_COMM_WORLD, &myCOMM_WORLD );
  MPI_Comm_size( myCOMM_WORLD, &Ntasks );    
  MPI_Comm_rank( myCOMM_WORLD, &Myrank );     

  if ( Ntasks > 2 )
    {
      if (Myrank == 0 )
	printf("too many people here, for this game two are enough\n");
      MPI_Abort(myCOMM_WORLD, 3);
    }

  // init the pseudo-random sequence
  srand( time(NULL) );

  unsigned int the_sender  = ( argc > 1 ? atoi(*(argv+1)) : 0 );
  unsigned int the_receiver = !the_sender;
  
  // always check for input consistency
  //...
  
  if ( Myrank == the_sender )
    {

      // get the data size from command line
      unsigned int data_size   = ( argc > 2 ? atoi(*(argv+2)) : 1000 );
      // get in how many chunks the data should be splitted, at minimum
      unsigned int Nchunks_min = ( argc > 3 ? atoi(*(argv+3)) : 10 );

      // make room for the data
      double *data = (double*)malloc(data_size * sizeof(double));
      if ( data == NULL )
	{
	  printf("Proc. %d has run out of memory. Better to stop here.\n", Myrank );
	  MPI_Abort(myCOMM_WORLD, 2);
	}

      // determine the maximum size of a data chunk
      unsigned int max_chunk_size = data_size / Nchunks_min;
      // set the minimum size of a data chunk      
      unsigned int min_chunk_size = max_chunk_size / 10;
      // guess how many chunks there will be
      unsigned int Nchunks_guess  = Nchunks_min * 2;

      // allocate room for the requests
      MPI_Request  requests_maxsize;
      MPI_Request *requests_size = (MPI_Request*)malloc( Nchunks_guess * sizeof(MPI_Request) );
      MPI_Request *requests_data = (MPI_Request*)malloc( Nchunks_guess * sizeof(MPI_Request) );

      // send the maximum size of a data chunk
      MPI_Isend ( &max_chunk_size, sizeof(unsigned int), MPI_BYTE, the_receiver, MSG_MAXSIZE, myCOMM_WORLD, &requests_maxsize );

      //
      // the loop of data generation & dispatch
      //
      
      int          Nchunks  = 0;
      unsigned int head     = 0;
      
      while ( head < data_size )
	{

	  Nchunks++;
	  if ( Nchunks == Nchunks_guess )
	    {
	      // if we do not have enough request handles, allocate more space for them
	      //
	      Nchunks_guess *= 1.2;
	      requests_size = (MPI_Request*)realloc( requests_size, sizeof(MPI_Request)*Nchunks_guess);
	      requests_data = (MPI_Request*)realloc( requests_data, sizeof(MPI_Request)*Nchunks_guess);
	    }

	  // randomly determine the current chunk size
	  unsigned int this_chunk_size = lrand48() % max_chunk_size;
	  this_chunk_size = ( this_chunk_size < min_chunk_size ? min_chunk_size : this_chunk_size );
	  this_chunk_size = ( head + this_chunk_size > data_size ? data_size - head : this_chunk_size );

	  // send the current chunk size
	  MPI_Isend ( &this_chunk_size, sizeof(unsigned int), MPI_BYTE, the_receiver, MSG_SIZE,
		      myCOMM_WORLD, &requests_size[Nchunks] );
	  
	  unsigned int start = head;
	  head              += this_chunk_size;
	  
	  for ( int i = start; i < head; i++)
	    {
	      int work = 1000 + rand() % 100000000;
	      data[i] = heavy_work(work);
	    }

	  // send the prepared data
	  MPI_Isend ( &data[start], this_chunk_size, MPI_DOUBLE, 1, MSG_DATA, myCOMM_WORLD, &requests_data[Nchunks] );
	}

      // wait for all the requests
      MPI_Waitall ( Nchunks, requests_size, MPI_STATUS_IGNORE );
      MPI_Waitall ( Nchunks, requests_data, MPI_STATUS_IGNORE );

      // free the memory
      free ( requests_data );
      free ( requests_size );
      free ( data );
      
    }
  else
    {
     #define DATA_EMPTY     0
     #define DATA_RECEIVING 1
     #define DATA_READY     2

      // this will tell us whether we have donbe or not;
      // until data arrive, we are still in the game
      int done = 0;
      
      
      double      *data          [2] = {NULL, NULL};
      int          data_status   [2] = {DATA_EMPTY, DATA_EMPTY};
      int          current_chunk     = 0;
      unsigned int chunk_size    [2] = {0, 0};
      unsigned int data_max_size;

      MPI_Request requests_size[2];
      MPI_Request requests_data[2];
      
      // receive the maximum data size
      MPI_Recv ( &data_max_size, 1, MPI_INT, the_sender, MSG_MAXSIZE, myCOMM_WORLD, MPI_STATUS_IGNORE );
      // allocate two buffers for the data
      data[0] = (double*)malloc(2 * data_max_size * sizeof(double));
      if ( data[0] == NULL )
	{
	  printf("Proc. %d has run out of memory. Better to stop here.\n", Myrank );
	  MPI_Abort(myCOMM_WORLD, 2);
	}
      data[1] = data[0] + data_max_size;      


      // receive the size of the first chunk
      MPI_Recv ( &chunk_size[current_chunk], 1, MPI_INT, the_sender, MSG_SIZE, myCOMM_WORLD,
		 MPI_STATUS_IGNORE );
      
      if ( chunk_size[current_chunk] > 0 )
	{
	  // if not zero, receive the data
	  MPI_Recv ( &data[current_chunk], chunk_size[current_chunk], MPI_DOUBLE, the_sender, MSG_DATA,
		     myCOMM_WORLD, MPI_STATUS_IGNORE);
	  data_status[current_chunk] = DATA_READY;
	  
	  // post the Irecv for the size of the next chunk
	  MPI_Irecv ( &chunk_size[ !current_chunk ], 1, MPI_INT, the_sender, MSG_SIZE, myCOMM_WORLD,
		      &requests_size[!current_chunk] );
	  data_status[!current_chunk] = DATA_EMPTY;
	}
      else
	// if zero, terminate
	done = 1;

      // main loop
      // until data arrive, we continue


      while ( !done )
	{

	  // we will process the data in subchunks
	  // when a subchunk terminate, we check the
	  // non-blocking communications
	  
	  unsigned int subchunk_size  = chunk_size[current_chunk] / 10;
	  subchunk_size              += ( subchunk_size == 0 );	  
	  unsigned int head           = 0;	  
	  while ( head < chunk_size[current_chunk])
	    {

	      // process this sub-chunk
	      unsigned int stop = head += subchunk_size;
	      stop  = (stop > chunk_size[current_chunk] ? chunk_size[current_chunk] : stop );
	      for ( ; head < stop; head++ )
		heavy_work( head );

	      // if we have not yet received the size
	      // of the next chunk, let's check for it
	      // 
	      if ( data_status[ !current_chunk ] != DATA_RECEIVING )
		{
		  // test it
		  int other_size_received  = 0;		  
		  MPI_Test ( &requests_size[ !current_chunk ], &other_size_received, MPI_STATUS_IGNORE );
		  
		  if ( other_size_received )
		    {
		      // if the communiation has completed,
		      // check that there will be new data
		      done = ( chunk_size[!current_chunk] > 0 );
		      if ( !done ) {
			// if so, post a Irecv
			MPI_Irecv ( &data[!current_chunk], chunk_size[!current_chunk], MPI_DOUBLE, the_sender, MSG_DATA,
				    myCOMM_WORLD, &requests_data[!current_chunk] );
			// set the status accordingly
			data_status[ !current_chunk ] = DATA_RECEIVING; }
		    }
		}
	    }
	      
	  data_status[ current_chunk ] = DATA_EMPTY;
	  MPI_Irecv ( &chunk_size[ current_chunk ], 1, MPI_INT, the_sender, MSG_SIZE, myCOMM_WORLD,
		      &requests_size[current_chunk] );

	  
	  current_chunk = !current_chunk;
	  if ( data_status[ current_chunk ] == DATA_EMPTY )
	    {
	      MPI_Wait( &requests_size[current_chunk], MPI_STATUS_IGNORE );
	      done = ( chunk_size[current_chunk] > 0 );
	      if ( !done ) {
		MPI_Recv ( &data[current_chunk], chunk_size[current_chunk], MPI_DOUBLE, the_sender, MSG_DATA,
			   myCOMM_WORLD, MPI_STATUS_IGNORE );
		data_status[ current_chunk ] = DATA_READY; }
	    }	  
	  data_status[ current_chunk ] = DATA_READY;
	}
      
      free ( data[0] );

     #undef DATA_EMPTY
     #undef DATA_RECEIVING
     #undef DATA_READY
     #undef DATA_DONE

    }

  MPI_Finalize();
  return 0;
}



double heavy_work( int N )
{
  double guess = 3.141572 / (3 * N+1);

  for( int i = 0; i < N; i++ )
    {
      double save = guess;
      guess = exp( guess );
      guess = sin( guess );
      if ( isinf(guess) )
        printf(". %g ", save);
    }
  return guess;
}

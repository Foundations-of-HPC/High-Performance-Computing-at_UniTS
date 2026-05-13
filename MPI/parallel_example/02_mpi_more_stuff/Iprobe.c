
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

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <mpi.h>
#include <unistd.h> // For usleep()

#define TAG_DATA 1

int main(int argc, char **argv)
{
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_SINGLE, &provided);
    if (provided < MPI_THREAD_SINGLE) {
        printf("Error: MPI_THREAD_SINGLE not provided.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int Me, Ntasks;
    MPI_Comm_size(MPI_COMM_WORLD, &Ntasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &Me);

    if (Ntasks < 2) {
        if (Me == 0) printf("This example requires at least 2 tasks.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (Me == 0) {
        srand(time(NULL));
        // Ensure N is at least 2 to safely print data[0], data[1], and data[N-1]
        int N = (rand() % 769) + 2; 
        int *data = (int*)malloc(sizeof(int) * N);
        
        for (int i = 0; i < N; i++) {
            data[i] = rand();
        }

        // Simulate some computational work before sending
        printf("Task 0: Working on data before sending...\n");
        sleep(2); 

        MPI_Send(data, N, MPI_INT, 1, TAG_DATA, MPI_COMM_WORLD);
        printf("Task 0: Has sent %d data elements: %d %d, ... %d!\n",
               N, data[0], data[1], data[N-1]);

        free(data);
    }
    else if (Me == 1) {
        MPI_Status status;
        int flag = 0;
        int work_cycles = 0;

        printf("Task 1: Starting to work while waiting for data...\n");

        // The Non-Blocking Probe Loop
        while (!flag) {
            // Check if a message has arrived
            MPI_Iprobe(0, TAG_DATA, MPI_COMM_WORLD, &flag, &status);

            if (!flag) {
                // Message hasn't arrived yet. Do some useful work here!
                work_cycles++;
                printf("Task 1: Doing work cycle %d...\n", work_cycles);
                usleep(500000); // Sleep for 0.5 seconds to simulate work
            }
        }

        // If we break out of the loop, flag is true, meaning the message is ready.
        int N;
        MPI_Get_count(&status, MPI_INT, &N);

        int *local = (int*)malloc(sizeof(int) * N);
        
        // Receive the actual message
        MPI_Recv(local, N, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);

        // Verify the actual received count (Best Practice)
        int received_count;
        MPI_Get_count(&status, MPI_INT, &received_count);

        printf("Task 1: After %d cycles of work, received %d data elements: %d %d, ... %d\n",
               work_cycles, received_count, local[0], local[1], local[received_count-1]);

        free(local);
    }

    MPI_Finalize();
    return 0;
}

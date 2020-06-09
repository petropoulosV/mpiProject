#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include "mpi_ops.h"
#include "util.h"

FILE *file = NULL;

int main(int argc, char** argv){



    if(argc != 3){
		printf("Usage: mpirun -np <count> --hostfile <file with hostnames> <executavles> <NUM_SERVERS> <testfile>\n");
		return EXIT_FAILURE;
	}

	/* Open input file */
	if (( file = fopen(argv[2], "r") ) == NULL ) {
		fprintf(stderr, "\n Could not open file: %s\n", argv[2]);
		perror("Opening test file\n");
		return EXIT_FAILURE;
	}

    NUM_SERVERS = atoi(argv[1]);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if(rank == 0){
		// Coordinator
		DPRINT("[rank: %d] Coordinator started\n", rank);

        MPI_init();

    }
    else{
        // Peers
        DPRINT("[rank: %d] Peer started\n", rank);
        MPI_server();
    }



    DPRINT("[rank: %d] Peer Terminated\n", rank);

    MPI_Finalize();
}
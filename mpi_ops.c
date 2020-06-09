#include "mpi_ops.h"

#include "util.h"

extern FILE *file;

void MPI_init(void){
    int n1, n2, n3;
    char buff[BUFFER_SIZE], event[100];
    int neighbors[2], response;


    /** MPI Initialisation **/

    MPI_Status status;
    //MPI_Datatype;

    MPI_Datatype NEIGHBOR_ARRAY;
    MPI_Type_contiguous(2, MPI_INT, &NEIGHBOR_ARRAY);
    MPI_Type_commit(&NEIGHBOR_ARRAY);

    while (fgets(buff, BUFFER_SIZE, file))
    {

        n1 = n2 = n3 = 0;
        sscanf(buff, "%s %d %d %d", event, &n1, &n2, &n3);
        //DPRINT("%s ", event);

        if(!strcmp("SERVER",event)){
            neighbors[0] = n2;
            neighbors[1] = n3;

            MPI_Send(neighbors, 1, NEIGHBOR_ARRAY, n1, SERVER, MPI_COMM_WORLD);
            DPRINT("[rank: %d] send msg of type SERVER to server: %d left: %d, right %d \n", 0, n1, neighbors[0], neighbors[1]);
            MPI_Recv(&response, 1, MPI_INT, n1, ACK, MPI_COMM_WORLD, &status);
            DPRINT("[rank: %d] received msg of type SERVER with tag %d, from server: %d \n", rank, status.MPI_TAG, response);
        }
        else if(!strcmp("UPLOAD", event)){

        }
        else if(!strcmp("RETRIEVE", event))
        {
        }
        else if(!strcmp("UPDATE", event))
        {
        }
        else if(!strcmp("START_LEADER_ELECTION", event))
        {

        }
        
    }
}

void MPI_server(void)
{

    int neighbors[2], response;
    int leader_id = rank;

        /** MPI Initialisation **/

        MPI_Status status;
    //MPI_Datatype;

    MPI_Datatype NEIGHBOR_ARRAY;
    MPI_Type_contiguous(2, MPI_INT, &NEIGHBOR_ARRAY);
    MPI_Type_commit(&NEIGHBOR_ARRAY);

    MPI_Recv(neighbors, 2, NEIGHBOR_ARRAY, 0, SERVER, MPI_COMM_WORLD, &status);
    DPRINT("[rank: %d] received msg of type SERVER with tag %d, left: %d, right %d \n", rank, status.MPI_TAG, neighbors[0], neighbors[1]);

    MPI_Send(&rank, 1, MPI_INT, 0, ACK, MPI_COMM_WORLD);
    DPRINT("[rank: %d] send msg of type ACK to Coordinator \n", rank);
}
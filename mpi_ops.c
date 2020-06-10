#include "mpi_ops.h"

#include "util.h"

//extern FILE *file;

void MPI_init(FILE *file)
{
    int n1, n2, n3;
    char buff[BUFFER_SIZE], event[100];
    int neighbors[2], response;
    int *Servers, s = 0, i;

        /** MPI Initialisation **/

        MPI_Status status;
    //MPI_Datatype;

    Servers = (int*) malloc(sizeof(int) * NUM_SERVERS);

    if (Servers == NULL)
    {
        printf("Error malloc");
    }

    MPI_Datatype NEIGHBOR_ARRAY;
    MPI_Type_contiguous(2, MPI_INT, &NEIGHBOR_ARRAY);
    MPI_Type_commit(&NEIGHBOR_ARRAY);


    while (fgets(buff, BUFFER_SIZE, file))
    {

        n1 = n2 = n3 = 0;
        sscanf(buff, "%s %d %d %d", event, &n1, &n2, &n3);
        //DPRINT("%s ", event);
        if(!strcmp("SERVER",event)){
            Servers[s++] = n1;
            neighbors[0] = n2;
            neighbors[1] = n3;

            MPI_Send(&neighbors, 1, NEIGHBOR_ARRAY, n1, SERVER, MPI_COMM_WORLD);
            DPRINT("[rank: %d] send msg of type SERVER to server: %d left: %d, right %d \n", 0, n1, neighbors[0], neighbors[1]);
            MPI_Recv(&response, 1, MPI_INT, n1, ACK, MPI_COMM_WORLD, &status);
            DPRINT("[rank: %d] received msg of type SERVER with tag %d, from server: %d \n", rank, status.MPI_TAG, status.MPI_SOURCE);
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
            for (i=0; i < NUM_SERVERS; i++){
                DPRINT("[rank: %d] send msg of type START_LEADER_ELECTION to server: %d\n", 0, Servers[i]);
                response = 0;
                MPI_Send(&response, 1, MPI_INT, Servers[i], START_LEADER_ELECTION, MPI_COMM_WORLD);
            }

            MPI_Recv(&response, 1, MPI_INT, MPI_ANY_SOURCE, LEADER_ELECTION_DONE, MPI_COMM_WORLD, &status);
            DPRINT("[rank: %d] received msg of type LEADER_ELECTION_DONE <%d> from server: %d \n", rank, response, status.MPI_SOURCE);
            for (i = 0; i < NUM_SERVERS; i++)
            {
                printf("Serers: %d\n", Servers[i]);
            }


        }
    }
}

void MPI_Peer(void)
{

    int neighbors[2], receive;
    int leader_id = rank;
    int *ServersId, s = 0, i;
    int sendCandidate = 0;

    MPI_Status status;

    ServersId = (int *)malloc(sizeof(int) * NUM_SERVERS);
    if (ServersId == NULL)
    {
        printf("Error malloc");
    }

    MPI_Datatype NEIGHBOR_ARRAY;
    MPI_Type_contiguous(2, MPI_INT, &NEIGHBOR_ARRAY);
    MPI_Type_commit(&NEIGHBOR_ARRAY);

    MPI_Recv(neighbors, 1, NEIGHBOR_ARRAY, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    if (status.MPI_TAG == SERVER){
        DPRINT("[rank: %d] received msg of type SERVER, left: %d, right %d \n", rank, neighbors[0], neighbors[1]);

        MPI_Send(&rank, 1, MPI_INT, 0, ACK, MPI_COMM_WORLD);
        DPRINT("[Server: %d] send msg of type ACK to Coordinator \n", rank);

        /* LEADER ELECTION */
        for (i = 0; i <= NUM_SERVERS; i++)
        {

        
            MPI_Recv(&receive, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            if (status.MPI_TAG == START_LEADER_ELECTION){
                DPRINT("[server: %d] received msg of type START_LEADER_ELECTION from %d\n", rank, status.MPI_SOURCE);
                if (sendCandidate == 0){
                    sendCandidate++;
                    DPRINT("[server: %d] send msg of type CANDIDATE_ID <MY_ID> to: %d \n", rank, neighbors[0]);
                    MPI_Send(&rank, 1, MPI_INT, neighbors[0], CANDIDATE_ID, MPI_COMM_WORLD);
                }

            }
            else if (status.MPI_TAG == CANDIDATE_ID){

                DPRINT("[rank: %d] received msg of type CANDIDATE_ID <%d> from %d\n", rank, receive, status.MPI_SOURCE);

                if (sendCandidate == 0){
                    sendCandidate++;
                    DPRINT("[server: %d] send msg of type CANDIDATE_ID <MY_ID> to: %d \n", rank, neighbors[0]);
                    MPI_Send(&rank, 1, MPI_INT, neighbors[0], CANDIDATE_ID, MPI_COMM_WORLD);
                }
                
                leader_id = MAX(leader_id, receive);
                ServersId[s++] = receive;

                if (receive != rank)
                {
                    DPRINT("[server: %d] send msg of type CANDIDATE_ID <%d> to: %d \n", rank, receive, neighbors[0]);
                    MPI_Send(&receive, 1, MPI_INT, neighbors[0], CANDIDATE_ID, MPI_COMM_WORLD);
                }
            }



        }/* for */
        
        if (leader_id == rank){
            DPRINT("[server: %d] >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>I AM THE LEADER \n", rank);
            DPRINT("[server: %d] send msg of type LEADER_ELECTION_DONE <%d> to: %d \n", rank, leader_id, 0);
            MPI_Send(&leader_id, 1, MPI_INT, 0, LEADER_ELECTION_DONE, MPI_COMM_WORLD);
        }


    } else if (status.MPI_TAG == CLIENT){

        DPRINT("[rank: %d] received msg of type CLIENT with Master: %d \n", rank, neighbors[0]);
        MPI_Send(&rank, 1, MPI_INT, 0, ACK, MPI_COMM_WORLD);
        DPRINT("[client: %d] send msg of type ACK to Coordinator \n", rank);
    }
}
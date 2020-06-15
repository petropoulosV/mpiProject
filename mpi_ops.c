#include <time.h>

#include "mpi_ops.h"

#include "util.h"

//extern FILE *file;

void MPI_init(FILE *file)
{
    int n1, n2, n3;
    char buff[BUFFER_SIZE], event[100];
    int neighbors[2], response;
    int *Servers, s = 0, i, Master, master = -1;

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
            master = MAX(master, n1);

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
        /*   for (i = 0; i < NUM_SERVERS; i++)
            {
                DPRINT("Serers: %d\n", Servers[i]);
            } */

            MPI_Recv(&Master, 1, MPI_INT, MPI_ANY_SOURCE, LEADER_ELECTION_DONE, MPI_COMM_WORLD, &status);
            DPRINT("[rank: %d]  received msg of type LEADER_ELECTION_DONE from %d \n", rank, Master);
            if(Master != master)
                DPRINT("[rank: %d] LEADER_ELECTION_DONE Houston we have a problem\n", rank);
        }
    }
}

void MPI_Peer(void)
{

    int neighbors[2], receive;
    int leader_id = rank;
    int *ServersId, s = 0, i;
    int sendCandidate = 0;
    struct master_struct master;

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

        master.rank = &rank;
        master.neighbors = neighbors;
        master.ServersId = ServersId;
        master.leader_id = &leader_id;

        /***************** MASTER *******************8*/
        if (leader_id == rank){

            MPI_Master(&master);
            printf("alalalalalallalalalalallalalalalallalalallallalalalalallalalalalalalalalalalla");
        }else{

            MPI_Simple_Server(&master);
            printf("alallalallalalallalalallallalalallalalalalalallalallalalalallalalalalalalalalalalla");
        }


    } else if (status.MPI_TAG == CLIENT){

        DPRINT("[rank: %d] received msg of type CLIENT with Master: %d \n", rank, neighbors[0]);
        MPI_Send(&rank, 1, MPI_INT, 0, ACK, MPI_COMM_WORLD);
        DPRINT("[client: %d] send msg of type ACK to Coordinator \n", rank);
    }/*else if*/

}/*MPI_Peer*/

void MPI_Master(struct master_struct *m)
{
    int k, i, tmp, L;
    int *long_paths;
    int response;

    MPI_Status status;

    k = NUM_SERVERS -3;
    L = k/4;

    long_paths = (int*) malloc(sizeof(int) * L);

    srand(time(0));

    DPRINT("[master: %d] >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>I AM THE LEADER \n", *m->rank);
    DPRINT("[master: %d] send msg of type LEADER_ELECTION_DONE <%d> to: %d \n", *m->rank, *m->leader_id, 0);
    MPI_Send(m->leader_id, 1, MPI_INT, 0, LEADER_ELECTION_DONE, MPI_COMM_WORLD);



    for (i = 0; i < L; i++){
        tmp = (rand() % NUM_SERVERS);
        if (m->neighbors[0] == m->ServersId[tmp] || m->neighbors[1] == m->ServersId[tmp] || rank == m->ServersId[tmp])
        {
            i--;
            continue;
        }

        long_paths[i] = m->ServersId[tmp];
    }

    for (i = 0; i < L; i++)
    {

        DPRINT("[master: %d] send msg of type CONNECT to server: %d\n", rank, long_paths[i]);
        MPI_Send(m->leader_id, 1, MPI_INT, long_paths[i], CONNECT, MPI_COMM_WORLD);

        MPI_Recv(&response, 1, MPI_INT, long_paths[i], ACK, MPI_COMM_WORLD, &status);
        DPRINT("[master: %d] %d  CONNECTED TO %d \n", rank, long_paths[i], rank);
    }

    DPRINT("[master: %d] send msg of type LEADER_ELECTION_DONE to coordinator: %d\n", rank, 0);
    MPI_Send(m->leader_id, 1, MPI_INT, 0, LEADER_ELECTION_DONE, MPI_COMM_WORLD);


}

void MPI_Simple_Server(struct master_struct *m)
{
    int k, i=0, tmp, L;
    int long_path =-1;
    int response;

    MPI_Status status;

    //while(i<1){

        MPI_Recv(&response, 1, MPI_INT,  MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        DPRINT("[server: %d] received msg of type: %d from: %d \n", *m->rank, status.MPI_TAG, status.MPI_SOURCE);

        switch (status.MPI_TAG)
        {
        case CONNECT:

            long_path = response;
            MPI_Send(m->leader_id, 1, MPI_INT, *m->leader_id, ACK, MPI_COMM_WORLD);

            DPRINT("[master: %d] send msg of type %d TO %d \n", *m->rank, ACK, *m->leader_id);
            break;
        
        default:
            break;
        }

        i++;
    //}
}
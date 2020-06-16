#include <time.h>

#include "mpi_ops.h"

#include "util.h"

//extern FILE *file;

void MPI_init(FILE *file)
{
    int n1, n2, n3;
    char buff[BUFFER_SIZE], event[100];
    int neighbors[2], response;
    int *Servers, *Clients, s = 0, i, Master, master = -1;

    /** MPI Initialisation **/

    MPI_Status status;
    //MPI_Datatype;

    Servers = (int*) malloc(sizeof(int) * NUM_SERVERS);

    if (Servers == NULL) {
        printf("Error malloc");
    }

    Clients = (int *)malloc(sizeof(int) * world_size);

    if (Clients == NULL){
        printf("Error malloc");
    }

    for ( i = 0; i < world_size; i++) Clients[i] = 1;
    Clients[0] = 0;

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
            Clients[n1] = 0;

            neighbors[0] = n2;
            neighbors[1] = n3;
            master = MAX(master, n1);

            MPI_Send(&neighbors, 1, NEIGHBOR_ARRAY, n1, SERVER, MPI_COMM_WORLD);
            DPRINT("[rank: %d] send msg of type SERVER to server: %d left: %d, right %d \n", 0, n1, neighbors[0], neighbors[1]);
            MPI_Recv(&response, 1, MPI_INT, n1, ACK, MPI_COMM_WORLD, &status);
            DPRINT("[rank: %d] received msg of type SERVER with tag %d, from server: %d \n", rank, status.MPI_TAG, status.MPI_SOURCE);
        }
        else if(!strcmp("UPLOAD", event)){
            DPRINT("%s, %d %d ", event, n1, n2 );

            DPRINT("[rank: %d] send msg of type UPLOAD <%d> to client: %d\n", 0, n2, n1);
            MPI_Send(&n2, 1, MPI_INT, n1, UPLOAD, MPI_COMM_WORLD);


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

            MPI_Recv(&Master, 1, MPI_INT, MPI_ANY_SOURCE, LEADER_ELECTION_DONE, MPI_COMM_WORLD, &status);
            DPRINT("[rank: %d]  received msg of type LEADER_ELECTION_DONE from %d \n", rank, Master);
            if(Master != master)
                DPRINT("[rank: %d] LEADER_ELECTION_DONE Houston we have a problem\n", rank);


            /* ******************** INFORM CLIENT ***********************/

            neighbors[0] = Master ;
            neighbors[1] = Master ;
            for (i = 1; i < world_size; i++){
                if(Clients[i]){
                    MPI_Send(&neighbors, 1, NEIGHBOR_ARRAY, i, CLIENT, MPI_COMM_WORLD);
                    DPRINT("[rank: %d] send msg of type CLIENT to Client: %d Master is : %d \n", 0, i, neighbors[0]);
                    MPI_Recv(&response, 1, MPI_INT, i, ACK, MPI_COMM_WORLD, &status);
                    DPRINT("[rank: %d] received msg of type ACK from Client: %d \n", rank, status.MPI_SOURCE);
                }

            }
        }
    }
    printf("[rank: %d] alallalallalalallalalallallalalallalalalalalallalallalalalallalalalalalalalalalalla\n", rank);
}

void MPI_Peer(void)
{

    int neighbors[2], receive;
    int leader_id = rank;
    int *ServersId, s = 0, i;
    int sendCandidate = 0;
    int long_path = 0;
    int flag_LED = 0, flag_LES = 0;
    int Master;
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

        

        /***************** MASTER ********************/

        
        while(1){


            MPI_Recv(&receive, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            DPRINT("[server: %d] received msg of type: %d from: %d \n", rank, status.MPI_TAG, status.MPI_SOURCE);

            switch (status.MPI_TAG)
            {

            case START_LEADER_ELECTION:
                DPRINT("[server: %d] received msg of type START_LEADER_ELECTION from %d\n", rank, status.MPI_SOURCE);
                if (sendCandidate == 0)
                {
                    sendCandidate++;
                    DPRINT("[server: %d] send msg of type CANDIDATE_ID <MY_ID> to: %d \n", rank, neighbors[0]);
                    MPI_Send(&rank, 1, MPI_INT, neighbors[0], CANDIDATE_ID, MPI_COMM_WORLD);
                }
                flag_LES++;
                break;

            case CANDIDATE_ID:
                DPRINT("[rank: %d] received msg of type CANDIDATE_ID <%d> from %d\n", rank, receive, status.MPI_SOURCE);

                if (sendCandidate == 0)
                {
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

                break;

            case CONNECT:

                long_path = receive;
                MPI_Send(&rank, 1, MPI_INT, status.MPI_SOURCE, ACK, MPI_COMM_WORLD);

                DPRINT("[master: %d] send msg of type %d TO %d \n", rank, ACK, status.MPI_SOURCE);
                break;

            case LEADER_ELECTION_DONE:
                flag_LED++;
                DPRINT("[server: %d] received msg of type LEADER_ELECTION_DONE from %d\n", rank, status.MPI_SOURCE);

                break;

            default:
                break;
            }
            
            if(s == NUM_SERVERS && flag_LES != 0){
                master.rank = &rank;
                master.neighbors = neighbors;
                master.ServersId = ServersId;
                master.leader_id = &leader_id;

                if (leader_id == rank){
                    MPI_Master(&master);
                    break;
                }
                if (flag_LED != 0)
                {
                    //MPI_Simple_Server(&master);
                    break;
                                }
            }
        }

        printf("[rank: %d] ********************************************************************************\n", rank);

        while (1)
        {   break;

            MPI_Recv(&receive, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            DPRINT("[server: %d] received msg of type: %d from: %d \n", rank, status.MPI_TAG, status.MPI_SOURCE);

            switch (status.MPI_TAG)
            {

            case START_LEADER_ELECTION:
                
                break;

            default:
                break;
            }
        }/* while */
    }/* if */

    /* ********************************************************************************************************************************* */
    /* ********************************************************************************************************************************* */
    /* *************************************************          CLIENT            **************************************************** */
    /* ********************************************************************************************************************************* */
    /* ********************************************************************************************************************************* */
    else if (status.MPI_TAG == CLIENT){
        
        Master = neighbors[0];

        DPRINT("[rank: %d] received msg of type CLIENT with Master: %d \n", rank, Master);
        MPI_Send(&rank, 1, MPI_INT, 0, ACK, MPI_COMM_WORLD);
        DPRINT("[client: %d] send msg of type ACK to Coordinator \n", rank);

        


    } /*else if*/

            printf("[rank: %d] alallalallalalallalalallallalalallalalalalalallalallalalalallalalalalalalalalalalla\n", rank);
} /*MPI_Peer*/

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
    //DPRINT("[master: %d] send msg of type LEADER_ELECTION_DONE <%d> to: %d \n", *m->rank, *m->leader_id, 0);
    //MPI_Send(m->leader_id, 1, MPI_INT, 0, LEADER_ELECTION_DONE, MPI_COMM_WORLD);



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

    for (i = 0; i < NUM_SERVERS; i++)
    {
        if (m->ServersId[i] != *m->leader_id){
            DPRINT("[master: %d] send msg of type LEADER_ELECTION_DONE to server: %d\n", rank, m->ServersId[i]);
            MPI_Send(m->leader_id, 1, MPI_INT, m->ServersId[i], LEADER_ELECTION_DONE, MPI_COMM_WORLD);

            //MPI_Recv(&response, 1, MPI_INT, m->ServersId[i], ACK, MPI_COMM_WORLD, &status);
            //DPRINT("[master: %d] rcv msg ACK from %d \n", rank, status.MPI_SOURCE);
        }
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
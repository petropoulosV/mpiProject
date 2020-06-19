#include <time.h>

#include "mpi_ops.h"
#include "util.h"


#include "files.h"
#include "queue.h"

//extern FILE *file;

//struct Queue **Hash_Table;

void
MPI_init(FILE *file)
{
    int n1, n2, n3;
    char buff[BUFFER_SIZE], event[100];
    int neighbors[2], response;
    int *Servers, *Clients, Clients_Number = 0, s = 0, i, Master, master = -1;
    int Clients_Shutdown = 0;


    MPI_Status status;


    Servers = (int*) malloc(sizeof(int) * NUM_SERVERS);
    if (Servers == NULL)
        ERRX(1, "Error malloc\n");

    Clients = (int *)malloc(sizeof(int) * world_size);
    if (Clients == NULL)
        ERRX(1, "Error malloc\n");

    for ( i = 0; i < world_size; i++) Clients[i] = 1;
    Clients[0] = 0;

    MPI_Datatype MPI_ARRAY;
    MPI_Type_contiguous(2, MPI_INT, &MPI_ARRAY);
    MPI_Type_commit(&MPI_ARRAY);


    while (fgets(buff, BUFFER_SIZE, file)){

        n1 = n2 = n3 = 0;
        sscanf(buff, "%s %d %d %d", event, &n1, &n2, &n3);

        if(!strcmp("SERVER",event)){
            
            Servers[s++] = n1;
            Clients[n1] = 0;

            neighbors[0] = n2;
            neighbors[1] = n3;
            master = MAX(master, n1);

            MPI_Send(&neighbors, 1, MPI_ARRAY, n1, SERVER, MPI_COMM_WORLD);
            DPRINT("[Coord: %d] send msg of type SERVER to server: %d left: %d, right %d \n", 0, n1, neighbors[0], neighbors[1]);

            MPI_Recv(&response, 1, MPI_INT, n1, ACK, MPI_COMM_WORLD, &status);
            DPRINT("[Coord: %d] received msg of type SERVER with tag %d, from server: %d \n", rank, status.MPI_TAG, status.MPI_SOURCE);

        }
        else if(!strcmp("UPLOAD", event)){


            DPRINT("[Coord: %d] send msg of type UPLOAD <%d> to client: %d\n", 0, n2, n1);
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
            for (i = 0; i < NUM_SERVERS; i++){
                DPRINT("[Coord: %d] send msg of type START_LEADER_ELECTION to server: %d\n", 0, Servers[i]);
                response = 0;
                MPI_Send(&response, 1, MPI_INT, Servers[i], START_LEADER_ELECTION, MPI_COMM_WORLD);
            }

            MPI_Recv(&Master, 1, MPI_INT, MPI_ANY_SOURCE, LEADER_ELECTION_DONE, MPI_COMM_WORLD, &status);
            DPRINT("[Coord: %d]  received msg of type LEADER_ELECTION_DONE from %d \n", rank, Master);
            if(Master != master){
                IPRINT("[Coord: %d] LEADER_ELECTION_DONE Houston we have a problem\n", rank);
            }


            /* ******************** INFORM CLIENT ***********************/
            neighbors[0] = Master ;
            neighbors[1] = Master ;
            for (i = 1; i < world_size; i++){
                if(Clients[i]){
                    MPI_Send(&neighbors, 1, MPI_ARRAY, i, CLIENT, MPI_COMM_WORLD);
                    DPRINT("[Coord: %d] send msg of type CLIENT to Client: %d Master is : %d \n", 0, i, neighbors[0]);
                    MPI_Recv(&response, 1, MPI_INT, i, ACK, MPI_COMM_WORLD, &status);
                    DPRINT("[Coord: %d] received msg of type ACK from Client: %d \n", rank, status.MPI_SOURCE);
                    Clients_Number++;
                }

            }
        }
    } /* while */


    /* ******************** SHUTDOWN     ***********************/
    Clients_Shutdown = Clients_Number;

    for(i = 0; i < world_size; i++){

        if (Clients[i]){
            MPI_Send(&i, 1, MPI_INT, i, REQUEST_SHUTDOWN, MPI_COMM_WORLD);
            DPRINT("[Coord: %d] send msg of type REQUEST_SHUTDOWN to Client: %d \n", 0, i);
        }
    }

    while (Clients_Shutdown){
        Clients_Shutdown--;

        MPI_Recv(&response, 1, MPI_INT, MPI_ANY_SOURCE, SHUTDOWN_OK, MPI_COMM_WORLD, &status);
        DPRINT("[Coord: %d] received msg of type SHUTDOWN_OK from Client: %d \n", rank, status.MPI_SOURCE);
    }

    MPI_Send(&i, 1, MPI_INT, Master, REQUEST_SHUTDOWN, MPI_COMM_WORLD);
    DPRINT("[Coord: %d] send msg of type REQUEST_SHUTDOWN to Master: %d \n", 0, Master);

    MPI_Recv(&response, 1, MPI_INT, MPI_ANY_SOURCE, SHUTDOWN_OK, MPI_COMM_WORLD, &status);
    DPRINT("[Coord: %d] received msg of type SHUTDOWN_OK from Master: %d \n", rank, status.MPI_SOURCE);
}


void MPI_Peer(void)
{

    int neighbors[2], receive;
    int leader_id = rank;
    int *ServersId, s = 0, i, j;
    int sendCandidate = 0;
    int long_path = 0;
    int flag_LED = 0, flag_LES = 0, flag_MST = 0, flag = 0;
    int Master;
    int N, tmp;
    int *upload_servers;
    int Open_requests = 0;
    int flag_shutdown = 0;
    int receive_array[2];
    int Client_id, file_id;
    struct master_struct master;
    struct file *local_files[1000] = {NULL};
    struct Queue *queue;

        MPI_Status status;
    MPI_Datatype MPI_ARRAY;
    MPI_Type_contiguous(2, MPI_INT, &MPI_ARRAY);
    MPI_Type_commit(&MPI_ARRAY);

    srand(time(0));

    ServersId = (int *)malloc(sizeof(int) * NUM_SERVERS);
    if (ServersId == NULL)
        ERRX(1, "Error malloc\n");


    N = ( (NUM_SERVERS - 1) / 2) + 1;


    MPI_Recv(neighbors, 1, MPI_ARRAY, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    /* ********************************************************************************************************************************* */
    /* ********************************************************************************************************************************* */
    /* *************************************************          SERVRERS            **************************************************** */
    /* ********************************************************************************************************************************* */
    /* ********************************************************************************************************************************* */

    if (status.MPI_TAG == SERVER){
        DPRINT("[server: %d] received msg of type SERVER, left: %d, right %d \n", rank, neighbors[0], neighbors[1]);

        MPI_Send(&rank, 1, MPI_INT, 0, ACK, MPI_COMM_WORLD);
        DPRINT("[server: %d] send msg of type ACK to Coordinator \n", rank);


        while(1){


            MPI_Recv(&receive, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            //DPRINT("[server: %d] received msg of type: %d from: %d \n", rank, status.MPI_TAG, status.MPI_SOURCE);

            switch (status.MPI_TAG){

                case START_LEADER_ELECTION:
                    DPRINT("[server: %d] received msg of type START_LEADER_ELECTION from %d\n", rank, status.MPI_SOURCE);

                    if (sendCandidate == 0){
                        sendCandidate++;
                        DPRINT("[server: %d] send msg of type CANDIDATE_ID <MY_ID> to: %d \n", rank, neighbors[0]);
                        MPI_Send(&rank, 1, MPI_INT, neighbors[0], CANDIDATE_ID, MPI_COMM_WORLD);
                    }
                    flag_LES++;
                    break;

                case CANDIDATE_ID:
                    DPRINT("[server: %d] received msg of type CANDIDATE_ID <%d> from %d\n", rank, receive, status.MPI_SOURCE);

                    if (sendCandidate == 0){
                        sendCandidate++;
                        DPRINT("[server: %d] send msg of type CANDIDATE_ID <MY_ID> to: %d \n", rank, neighbors[0]);
                        MPI_Send(&rank, 1, MPI_INT, neighbors[0], CANDIDATE_ID, MPI_COMM_WORLD);
                    }

                    leader_id = MAX(leader_id, receive);
                    ServersId[s++] = receive;

                    if (receive != rank){
                        DPRINT("[server: %d] send msg of type CANDIDATE_ID <%d> to: %d \n", rank, receive, neighbors[0]);
                        MPI_Send(&receive, 1, MPI_INT, neighbors[0], CANDIDATE_ID, MPI_COMM_WORLD);
                    }

                    break;

                case CONNECT:

                    long_path = receive;
                    MPI_Send(&rank, 1, MPI_INT, status.MPI_SOURCE, ACK, MPI_COMM_WORLD);

                    DPRINT("[server: %d] send msg of type ACK (for CONNECT)to %d \n", rank, status.MPI_SOURCE);
                    break;

                case LEADER_ELECTION_DONE:
                    DPRINT("[server: %d] received msg of type LEADER_ELECTION_DONE from %d\n", rank, status.MPI_SOURCE);

                    flag_LED++;
                    break;
    
                default:
                    break;

            }/* switch */
            
            if(s == NUM_SERVERS && flag_LES != 0){
                master.rank = &rank;
                master.neighbors = neighbors;
                master.ServersId = ServersId;
                master.leader_id = &leader_id;

                if (leader_id == rank){
                    MPI_Master(&master);
                    flag_MST = 1;
                    break;
                }
                if (flag_LED != 0){

                    break;
                }
            }
        }

        Master = leader_id;

        /* ********************************************************************************************************************************* */
        /* ********************************************************************************************************************************* */
        /* *************************************************    AFTER LEADER ELECTION   **************************************************** */
        /* ********************************************************************************************************************************* */
        /* ********************************************************************************************************************************* */


        /* ******************************************************   MASTER    *************************************************************** */
        if (flag_MST){

            struct Queue *Hash_Table[1000] = {NULL};
            
            upload_servers = (int *)malloc(sizeof(int) * N);

            while (flag_shutdown == 0){

                DPRINT("[Master: %d] received msg of type: %d from: %d \n", rank, status.MPI_TAG, status.MPI_SOURCE);
                MPI_Recv(&receive, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                switch (status.MPI_TAG){

                    case UPLOAD:
                        DPRINT("[Master: %d] received msg of type UPLOAD <%d> from Client %d\n", rank, receive, status.MPI_SOURCE);

                        if(!Hash_Table[receive]){
                            
                            Hash_Table[receive] = MakeEmptyQueue(receive);
                            Enqueue(status.MPI_SOURCE, N, UPLOAD, 1, Hash_Table[receive]);

                            receive_array[1] = receive;

                            /* SELECT UPLOAD SERVERS */
                            for (i = 0; i < N; i++){

                                tmp = (rand() % NUM_SERVERS);
                                upload_servers[i] = -1;
                                flag = 0;

                                if (rank == ServersId[tmp]){
                                    i--;
                                    continue;
                                }

                                for (j = 0; j < i; j++){
                                    if (upload_servers[j] == ServersId[tmp]){
                                        flag++;
                                        break;
                                    }
                                }

                                if (flag != 0){
                                    i--;
                                    continue;
                                }

                                upload_servers[i] = ServersId[tmp];
                            }

                            IPRINT("[Master: %d] send msg of type UPLOAD <%d,(", rank, receive_array[1]);
                            for (i = 0; i < N; i++){
                                receive_array[0] = upload_servers[i];
                                IPRINT("%d, \n", receive_array[0]);
                                MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], UPLOAD, MPI_COMM_WORLD);
                            }
                            IPRINT(")to Left\n");
                        }
                        else{

                            DPRINT("[Master: %d] send msg of type UPLOAD_FAILED <%d> to Client %d\n", rank, receive, status.MPI_SOURCE);
                            MPI_Send(&receive, 1, MPI_INT, status.MPI_SOURCE, UPLOAD_FAILED, MPI_COMM_WORLD);
                        }

                        break;

                    case UPLOAD_FAILED:

                        DPRINT("[Master: %d] received msg of type UPLOAD_FAILED <%d> from %d\n", rank, receive, status.MPI_SOURCE);

                        break;

                    case UPLOAD_ACK:
                        DPRINT("[Server: %d] received msg of type UPLOAD_ACK <%d>\n", rank, receive);

                        file_id = receive;

                        queue = Hash_Table[receive];
                        Client_id = client_id(queue);

                        if(!registration_decrease(queue)){

                            DPRINT("[Server: %d] send msg of type UPLOAD_OK to Client %d for file %d \n", rank, Client_id, file_id);
                            MPI_Send(&file_id, 1, MPI_INT, Client_id, UPLOAD_OK, MPI_COMM_WORLD);
                        }


                        break;

                    case REQUEST_SHUTDOWN:

                        if (status.MPI_SOURCE == 0){
                            DPRINT("[Master: %d] received msg of type _REQUEST_SHUTDOWN from Coordinator\n", rank);
                            DPRINT("[Master: %d] send msg of type REQUEST_SHUTDOWN to Left \n", rank);

                            MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], REQUEST_SHUTDOWN, MPI_COMM_WORLD);

                        }else{
                            DPRINT("[Master: %d] received msg of type REQUEST_SHUTDOWN from Right\n", rank);
                            DPRINT("[Master: %d] send msg of type SHUTDOWN_OK to Coordinator \n", rank);

                            MPI_Send(&receive_array[0], 1, MPI_INT, 0, SHUTDOWN_OK, MPI_COMM_WORLD);
                            flag_shutdown++;
                        }
                        
                        break;

                    default:
                        break;
                }/* switch */

            } /* while */

            printf("[Master: %d] End of Service\n", rank);
        } /* if SERVER)*/

        /*******************************************************  JUST SERVER    *************************************************************** */
        else{


            while (flag_shutdown == 0 ){

                MPI_Recv(&receive_array, 1, MPI_ARRAY, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                //DPRINT("[Server: %d] received msg of type: %d from: %d \n", rank, status.MPI_TAG, status.MPI_SOURCE);

                switch (status.MPI_TAG){

                    case UPLOAD:

                        DPRINT("[Server: %d] received msg of type UPLOAD <%d,%d>\n", rank, receive_array[0], receive_array[1]);

                        if (receive_array[0] == rank){

                            /* If I dont have the file add it */
                            if (!local_files[receive_array[1]]){

                                local_files[receive_array[1]] = new_file(receive_array[1]);

                                /* If there is long path use it*/
                                if (neighbors[0] == Master){
                                    DPRINT("[Server: %d] send msg of type UPLOAD_ACK to master via Loong Path\n", rank);
                                    MPI_Send(&receive_array[1], 1, MPI_INT, Master, UPLOAD_ACK, MPI_COMM_WORLD);
                                }
                                else{ 
                                    DPRINT("[Server: %d] send msg of type UPLOAD_ACK to master via Left\n", rank);
                                    MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], UPLOAD_ACK, MPI_COMM_WORLD);
                                }
                            }
                            else{/* There is not this case */

                                DPRINT("[Server: %d] send msg of type _UPLOAD_FAILED_ <%d> to Master via Left\n", rank, receive_array[1]);
                                MPI_Send(&receive_array[1], 1, MPI_INT, Master, UPLOAD_FAILED, MPI_COMM_WORLD);

                            }

                        }
                        else{

                            DPRINT("[Server: %d] send msg of type UPLOAD <%d,%d> to Left \n", rank, receive_array[0], receive_array[1]);
                            //MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], UPLOAD, MPI_COMM_WORLD);

                            if (neighbors[0] == Master)
                                MPI_Send(&receive_array[1], 1, MPI_INT, neighbors[0], UPLOAD, MPI_COMM_WORLD);
                            else
                                MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], UPLOAD, MPI_COMM_WORLD);
                        }

                        break;

                    case UPLOAD_ACK:
                        DPRINT("[Server: %d] received msg of type UPLOAD_ACK <%d,%d>\n", rank, receive_array[0], receive_array[1]);

                        
                        if (long_path){
                            DPRINT("[Server: %d] send msg of type UPLOAD_ACK to master via Loong Path\n", rank);
                            MPI_Send(&receive_array[1], 1, MPI_INT, Master, UPLOAD_ACK, MPI_COMM_WORLD);
                        }
                        else if (neighbors[0] == Master){
                            DPRINT("[Server: %d] send msg of type UPLOAD_ACK to Left \n", rank);
                            MPI_Send(&receive_array[1], 1, MPI_INT, neighbors[0], UPLOAD_ACK, MPI_COMM_WORLD);
                        }
                        else{
                            DPRINT("[Server: %d] send msg of type UPLOAD_ACK to Left \n", rank);
                            MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], UPLOAD_ACK, MPI_COMM_WORLD);
                        }

                        break;

                    case REQUEST_SHUTDOWN:

                        DPRINT("[Server: %d] received msg of type REQUEST_SHUTDOWN \n", rank);
                        DPRINT("[Server: %d] send msg of type REQUEST_SHUTDOWN to Left \n", rank);

                        if(neighbors[0] == Master)
                            MPI_Send(&receive_array, 1, MPI_INT, neighbors[0], REQUEST_SHUTDOWN, MPI_COMM_WORLD);
                        else
                            MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], REQUEST_SHUTDOWN, MPI_COMM_WORLD);

                        flag_shutdown++;

                        break;

                    default:
                        break;
                } /* switch */


            } /* WHILE */

            printf("[Server: %d] End of Service\n", rank);
        }/* if (MASTER)*/

    /* ********************************************************************************************************************************* */
    /* ********************************************************************************************************************************* */
    /* *************************************************          CLIENT            **************************************************** */
    /* ********************************************************************************************************************************* */
    /* ********************************************************************************************************************************* */

    }
    else if (status.MPI_TAG == CLIENT){


        Master = neighbors[0];

        DPRINT("[Client: %d] received msg of type CLIENT with Master: %d \n", rank, Master);
        MPI_Send(&rank, 1, MPI_INT, 0, ACK, MPI_COMM_WORLD);
        DPRINT("[Client: %d] send msg of type ACK to Coordinator \n", rank);
        Open_requests = 0;

        while ((flag_shutdown == 0) || (Open_requests > 0 )){

            MPI_Recv(&receive, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            //DPRINT("[Client: %d] received msg of type: %d from: %d \n", rank, status.MPI_TAG, status.MPI_SOURCE);

            switch (status.MPI_TAG){

                case UPLOAD:
                    DPRINT("[Client: %d] received msg of type UPLOAD_ <%d>\n", rank, receive);
                    
                    /* If I dont have the file add it */
                    if(!local_files[receive]){
                        Open_requests++;
                        local_files[receive] = new_file(receive);

                        DPRINT("[Client: %d] send msg of type UPLOAD <%d> to master \n", rank, receive);
                        MPI_Send(&receive, 1, MPI_INT, Master, UPLOAD, MPI_COMM_WORLD);
                    }
                    else{
                        
                        DPRINT("[Client: %d] send msg of type UPLOAD_FAILED <%d> to coordinator \n", rank, receive);
                        MPI_Send(&receive, 1, MPI_INT, 0, UPLOAD_FAILED, MPI_COMM_WORLD);
                    }


                    break;

                case UPLOAD_FAILED:

                    DPRINT("[Client: %d] received msg of type UPLOAD_FAILED  from Master %d\n", rank, receive);

                    IPRINT("CLIENT %d FAILED TO UPLOADE FILE %d \n", rank, receive);

                    Open_requests--;
                    break;

                case REQUEST_SHUTDOWN:
                    
                    flag_shutdown = 1 ;
                    DPRINT("[Client: %d] received msg of type REQUEST_SHUTDOWN_\n", rank);
                    
                    break;
                case UPLOAD_OK:
                    DPRINT("[Client: %d] received msg of type UPLOAD_OK <%d> \n", rank, receive);

                    IPRINT("CLIENT %d UPLOADED FILE %d \n", rank, receive);

                    Open_requests--;
                    break;

                default:
                    break;

            }/* switch */

        }/* while */

        DPRINT("[Client: %d] send msg of type SHUTDOWN_OK  \n", rank);
        MPI_Send(&receive, 1, MPI_INT, 0, SHUTDOWN_OK, MPI_COMM_WORLD);
        
        DPRINT("[Client: %d] MY LOCAL FILES: ", rank);

        struct file *tmp;
        for (i = 0; i < 100; i++)
            if (local_files[i])
            {
                tmp = local_files[i];
                 DPRINT("file <%d> ", tmp->id);
            }
            
        
        DPRINT("\n");

        printf("[Client: %d] End of Service\n", rank);

    } /*else if(CLIENT)*/

} /*MPI_Peer()*/



void MPI_Master(struct master_struct *m)
{
    int k, i, j, tmp, L, flag = 0;
    int *long_paths;
    int response;

    MPI_Status status;

    k = NUM_SERVERS -3;
    L = k/4;


    srand(time(0));

    DPRINT("[master: %d] >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>I AM THE LEADER \n", *m->rank);
    //DPRINT("[master: %d] send msg of type LEADER_ELECTION_DONE <%d> to: %d \n", *m->rank, *m->leader_id, 0);
    //MPI_Send(m->leader_id, 1, MPI_INT, 0, LEADER_ELECTION_DONE, MPI_COMM_WORLD);


    long_paths = (int*) malloc(sizeof(int) * L);

    for (i = 0; i < L; i++){

        tmp = (rand() % NUM_SERVERS);
        long_paths[i] = -1;
        flag = 0;

        if (m->neighbors[0] == m->ServersId[tmp] || m->neighbors[1] == m->ServersId[tmp] || rank == m->ServersId[tmp]){
            i--;
            continue;
        }

        for ( j = 0; j <  i ; j++){
            if (long_paths[j] == m->ServersId[tmp]){
                flag++;
                break;
            }
        }
        
        if(flag != 0){
            i--;
            continue;
        }

        long_paths[i] = m->ServersId[tmp];
    }

    for (i = 0; i < L; i++){

        DPRINT("[Master: %d] send msg of type CONNECT to server: %d\n", rank, long_paths[i]);
        MPI_Send(m->leader_id, 1, MPI_INT, long_paths[i], CONNECT, MPI_COMM_WORLD);

        MPI_Recv(&response, 1, MPI_INT, long_paths[i], ACK, MPI_COMM_WORLD, &status);
        IPRINT("[Master: %d] %d  CONNECTED TO %d \n", rank, long_paths[i], rank);
    }

    for (i = 0; i < NUM_SERVERS; i++){

        if (m->ServersId[i] != *m->leader_id){
            DPRINT("[Master: %d] send msg of type LEADER_ELECTION_DONE to server: %d\n", rank, m->ServersId[i]);
            MPI_Send(m->leader_id, 1, MPI_INT, m->ServersId[i], LEADER_ELECTION_DONE, MPI_COMM_WORLD);

            //MPI_Recv(&response, 1, MPI_INT, m->ServersId[i], ACK, MPI_COMM_WORLD, &status);
            //DPRINT("[Master: %d] rcv msg ACK from %d \n", rank, status.MPI_SOURCE);
        }
    }

    DPRINT("[Master: %d] send msg of type LEADER_ELECTION_DONE to coordinator \n", rank);
    MPI_Send(m->leader_id, 1, MPI_INT, 0, LEADER_ELECTION_DONE, MPI_COMM_WORLD);


}


/* 
void MPI_Simple_Server(struct master_struct *m)
{
    int i=0;
    //int long_path =-1;
    int response;

    MPI_Status status;

    //while(i<1){

        MPI_Recv(&response, 1, MPI_INT,  MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        DPRINT("[server: %d] received msg of type: %d from: %d \n", *m->rank, status.MPI_TAG, status.MPI_SOURCE);

        switch (status.MPI_TAG)
        {
        case CONNECT:

            //long_path = response;
            MPI_Send(m->leader_id, 1, MPI_INT, *m->leader_id, ACK, MPI_COMM_WORLD);

            DPRINT("[master: %d] send msg of type %d TO %d \n", *m->rank, ACK, *m->leader_id);
            break;
        
        default:
            break;
        }

        i++;
    //}
} */
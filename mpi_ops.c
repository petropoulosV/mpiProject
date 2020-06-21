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
    int neighbors[3], response;
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
    MPI_Type_contiguous(3, MPI_INT, &MPI_ARRAY);
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

            MPI_Recv(&response, 1, MPI_INT, MPI_ANY_SOURCE, ACK, MPI_COMM_WORLD, &status);
            DPRINT("[Coord: %d] received msg of type ACK (SERVER) from server: %d \n", rank, status.MPI_SOURCE);

        }
        else if(!strcmp("UPLOAD", event)){


            DPRINT("[Coord: %d] send msg of type UPLOAD <%d> to client: %d\n", 0, n2, n1);
            MPI_Send(&n2, 1, MPI_INT, n1, UPLOAD, MPI_COMM_WORLD);

        }
        else if(!strcmp("RETRIEVE", event)){

            DPRINT("[Coord: %d] send msg of type RETRIEVE <%d> to client: %d\n", 0, n2, n1);
            MPI_Send(&n2, 1, MPI_INT, n1, RETRIEVE, MPI_COMM_WORLD);
        }
        else if(!strcmp("UPDATE", event)){

            DPRINT("[Coord: %d] send msg of type UPDATE <%d> to client: %d\n", 0, n2, n1);
            MPI_Send(&n2, 1, MPI_INT, n1, UPDATE, MPI_COMM_WORLD);
        }
        else if (!strcmp("LEAVE", event)){
            Clients_Shutdown = Clients_Number;

            for (i = 0; i < world_size; i++){

                if (Clients[i]){
                    MPI_Send(&i, 1, MPI_INT, i, BARRIER, MPI_COMM_WORLD);
                    DPRINT("[Coord: %d] send msg of type BARRIER to Client: %d \n", 0, i);
                }
            }

            while (Clients_Shutdown){
                Clients_Shutdown--;

                MPI_Recv(&response, 1, MPI_INT, MPI_ANY_SOURCE, BARRIER_ACK, MPI_COMM_WORLD, &status);
                DPRINT("[Coord: %d] received msg of type BARRIER_ACK from Client: %d \n", rank, status.MPI_SOURCE);
            }


            MPI_Send(&i, 1, MPI_INT, n1, LEAVE, MPI_COMM_WORLD);
            DPRINT("[Coord: %d] send msg of type LEAVE to Server: %d \n", 0, n1);

            MPI_Recv(&response, 1, MPI_INT, MPI_ANY_SOURCE, LEAVE_ACK, MPI_COMM_WORLD, &status);
            DPRINT("[Coord: %d] received msg of type LEAVE_ACK from Master\n", rank);
        }
        else if(!strcmp("START_LEADER_ELECTION", event)){
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

int N;
int *ServersId, *Sortest_path;
void MPI_Peer(void)
{

    int neighbors[3], receive;
    int leader_id = rank;
    int s = 0, i, j;
    //int *ServersId, *Sortest_path;
    int sendCandidate = 0;
    int long_path = 0;
    int flag_LED = 0, flag_LES = 0, flag_MST = 0, flag = 0;
    int Master;
    int tmp;
    int *upload_servers;
    int Open_requests = 0;
    int flag_shutdown = 0;
    int flag_barrier = 0;
    int receive_array[3];
    int Client_id, file_id;
    int request_type;
    int file_version;

    struct master_struct master;
    struct file *local_files[LOCAL_NUM] = {NULL};
    struct Queue *queue;
    struct registration *reg_;

    MPI_Status status;
    MPI_Datatype MPI_ARRAY;
    MPI_Type_contiguous(3, MPI_INT, &MPI_ARRAY);
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

                    Sortest_path = (int *)malloc(sizeof(int) * NUM_SERVERS);
                    if (Sortest_path == NULL)
                        ERRX(1, "Error malloc\n");
                    MPI_Master(&master, Sortest_path);
                    flag_MST = 1;
                    break;
                }
                if (flag_LED != 0){

                    break;
                }
            }
        }/* WHILE */

        Master = leader_id;

        /* ********************************************************************************************************************************* */
        /* ********************************************************************************************************************************* */
        /* *************************************************    AFTER LEADER ELECTION   **************************************************** */
        /* ********************************************************************************************************************************* */
        /* ********************************************************************************************************************************* */


        /* ******************************************************   MASTER    *************************************************************** */
        if (flag_MST){

            struct Queue *Hash_Table[LOCAL_NUM] = {NULL};

            upload_servers = (int *)malloc(sizeof(int) * N);

            while (flag_shutdown == 0){

                MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                switch (status.MPI_TAG){

                    case RETRIEVE_ACK:
                        //RPRINT("@[Master: %d] received msg of type RETRIEVE_ACK from: %d \n", rank, status.MPI_SOURCE);
                        MPI_Recv(&receive_array, 1, MPI_ARRAY, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                        break;
                    case UPDATE:
                        //RPRINT("@[Master: %d] received msg of type UPDATE from: %d \n", rank, status.MPI_SOURCE);
                        MPI_Recv(&receive_array, 1, MPI_ARRAY, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                        break;

                    case VERSION_CHECK:
                        //RPRINT("@[Master: %d] received msg of type VERSION_CHECK from: %d \n", rank, status.MPI_SOURCE);
                        MPI_Recv(&receive_array, 1, MPI_ARRAY, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                        break;

                    case UPDATE_ACK:
                        //RPRINT("@[Master: %d] received msg of type UPDATE_ACK from: %d \n", rank, status.MPI_SOURCE);
                        MPI_Recv(&receive_array, 1, MPI_ARRAY, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                        break;

                    case LEAVE_CONFIRMATION:
                        //RPRINT("@[Master: %d] received msg of type LEAVE_CONFIRMATION from: %d \n", rank, status.MPI_SOURCE);
                        MPI_Recv(&receive_array, 1, MPI_ARRAY, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                        break;

                    default:
                        //RPRINT("@[Master: %d] received msg of type default (%d) from: %d \n", rank, status.MPI_TAG, status.MPI_SOURCE);
                        MPI_Recv(&receive, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                        break;

                } /* switch */



                //DPRINT("[Master: %d] received msg of type: %d from: %d \n", rank, status.MPI_TAG, status.MPI_SOURCE);
                //MPI_Recv(&receive, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

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
                                    if (upload_servers[j] == tmp){
                                        //if (upload_servers[j] == ServersId[tmp]){
                                        flag++;
                                        break;
                                    }
                                }

                                if (flag != 0){
                                    i--;
                                    continue;
                                }

                                //upload_servers[i] = ServersId[tmp];
                                upload_servers[i] = tmp;
                            }

                            DPRINT("[Master: %d] send msg of type UPLOAD <%d,(\n", rank, receive_array[1]);
                            for (i = 0; i < N; i++){
                                //receive_array[0] = upload_servers[i];
                                receive_array[0] = ServersId[upload_servers[i]];
                                if (Sortest_path[upload_servers[i]]){
                                    MPI_Send(&receive_array, 1, MPI_ARRAY, Sortest_path[upload_servers[i]], UPLOAD, MPI_COMM_WORLD);
                                    DPRINT("%d(%d), \n", Sortest_path[upload_servers[i]], receive_array[0]);
                                }
                                else{
                                    MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], UPLOAD, MPI_COMM_WORLD);
                                    DPRINT("%d, \n", receive_array[0]);
                                }

                            }
                            DPRINT(")to Left\n");
                        }
                        else{

                            DPRINT("[Master: %d] send msg of type UPLOAD_FAILED <%d> to Client %d\n", rank, receive, status.MPI_SOURCE);
                            MPI_Send(&receive, 1, MPI_INT, status.MPI_SOURCE, UPLOAD_FAILED, MPI_COMM_WORLD);
                        }

                        break;



                    case UPLOAD_ACK:
                        DPRINT("[Master: %d] received msg of type UPLOAD_ACK <%d>\n", rank, receive);

                        file_id = receive;

                        queue = Hash_Table[receive];
                        Client_id = client_id(queue);

                        if(!registration_decrease(queue)){

                            DPRINT("[Master: %d] send msg of type UPLOAD_OK to Client %d for file %d \n", rank, Client_id, file_id);
                            MPI_Send(&file_id, 1, MPI_INT, Client_id, UPLOAD_OK, MPI_COMM_WORLD);
                        }


                        break;

                    case UPLOAD_FAILED:

                        DPRINT("[Master: %d] received msg of type UPLOAD_FAILED <%d> from %d\n", rank, receive, status.MPI_SOURCE);

                        break;

                    case RETRIEVE:
                        DPRINT("[Master: %d] received msg of type RETRIEVE <%d,%d>\n", rank, status.MPI_SOURCE, receive);

                        if (!Hash_Table[receive]){
                            DPRINT("[Master: %d] send msg of type RETRIEVE_FAILED to Client %d \n", rank, status.MPI_SOURCE);
                            receive = -1;
                            MPI_Send(&receive, 1, MPI_INT, status.MPI_SOURCE, RETRIEVE_OK, MPI_COMM_WORLD);
                        }
                        else{

                            Enqueue(status.MPI_SOURCE, (N + 1), RETRIEVE, 0, Hash_Table[receive]);
                        }

                        break;



                    case RETRIEVE_ACK:
                        DPRINT("[Master: %d] received msg of type RETRIEVE_ACK file %d version %d from %d \n", rank, receive_array[0], receive_array[1], status.MPI_SOURCE);
                        file_id = receive_array[0];

                        queue = Hash_Table[file_id];
                        file_version = registration_retrieve_update(queue, receive_array[1]);
                        Client_id = client_id(queue);

                        if (!registration_decrease(queue)){

                            DPRINT("[Master: %d] send msg of type RETRIEVE_OK to Client %d for file %d  with version %d\n", rank, Client_id, file_id, file_version);
                            MPI_Send(&file_version, 1, MPI_INT, Client_id, RETRIEVE_OK, MPI_COMM_WORLD);
                        }

                        break;

                    case UPDATE:
                        RPRINT("[Master: %d] received msg of type UPDATE <%d,%d, %d>\n", rank, status.MPI_SOURCE, receive_array[0], receive_array[1]);

                        if (!Hash_Table[receive] || receive_array[1] == 0){
                            DPRINT("[Master: %d] send msg of type UPDATE_FAILED to Client %d for file %d\n", rank, status.MPI_SOURCE, receive_array[0]);
                            MPI_Send(&receive_array[0], 1, MPI_INT, status.MPI_SOURCE, UPDATE_FAILED, MPI_COMM_WORLD);
                        }else{
                            Enqueue(status.MPI_SOURCE, (N + 1), UPDATE, receive_array[1], Hash_Table[receive_array[0]]);
                        }

                        break;
                    case VERSION_CHECK:
                        DPRINT("[Master: %d] received msg of type VERSION_CHECK for file %d with version %d and flag %d\n", rank, receive_array[0], receive_array[1], receive_array[2]);

                        file_id = receive_array[0];

                        queue = Hash_Table[file_id];
                        file_version = registration_retrieve_update(queue, 0);
                        Client_id = client_id(queue);

                        if(receive_array[2] == 0){
                            DPRINT("[Master: %d] send msg of type VERSION_OUTDATED to Client %d for file %d \n", rank, Client_id, file_id);
                            MPI_Send(&file_id, 1, MPI_INT, Client_id, VERSION_OUTDATED, MPI_COMM_WORLD);
                            while(registration_decrease(queue)); /* remove registration from queue */
                        }else{


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
                                    if (upload_servers[j] == tmp){
                                    //if (ServersId[upload_servers[j]] == ServersId[tmp]){
                                        flag++;
                                        break;
                                    }
                                }

                                if (flag != 0){
                                    i--;
                                    continue;
                                }

                                //upload_servers[i] = ServersId[tmp];
                                upload_servers[i] = tmp;

                            }

                            //DPRINT("[Master: %d] send msg of type UPDATE <%d,(", rank, receive_array[1]);
                            receive_array[2] = file_version;
                            receive_array[1] = file_id;
                            for (i = 0; i < N; i++){
                                //receive_array[0] = upload_servers[i];
                                receive_array[0] = ServersId[upload_servers[i]];
                                if (Sortest_path[upload_servers[i]])
                                    MPI_Send(&receive_array, 1, MPI_ARRAY, Sortest_path[upload_servers[i]], UPDATE, MPI_COMM_WORLD);
                                else
                                    MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], UPDATE, MPI_COMM_WORLD);
                                //DPRINT("%d, \n", receive_array[0]);
                            }
                            //DPRINT(")to Left\n");
                        }

                        break;

                    case UPDATE_ACK:
                        DPRINT("[Master: %d] received msg of type UPDATE_ACK file %d from %d \n", rank, receive_array[1], status.MPI_SOURCE);
                        file_id = receive_array[1];

                        queue = Hash_Table[file_id];
                        Client_id = client_id(queue);

                        if (!registration_decrease(queue)){

                            DPRINT("[Master: %d] send msg of type UPDATE_OK to Client %d for file %d  \n", rank, Client_id, file_id);
                            MPI_Send(&file_id, 1, MPI_INT, Client_id, UPDATE_OK, MPI_COMM_WORLD);
                        }

                        break;

                    case REQUEST_SHUTDOWN:

                        if (status.MPI_SOURCE == 0){
                            DPRINT("[Master: %d] received msg of type _REQUEST_SHUTDOWN from Coordinator\n", rank);
                            RPRINT("[Master: %d] send msg of type REQUEST_SHUTDOWN to Left (%d) \n", rank, neighbors[0]);

                            MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], REQUEST_SHUTDOWN, MPI_COMM_WORLD);

                        }else{
                            DPRINT("[Master: %d] received msg of type REQUEST_SHUTDOWN from Right\n", rank);
                            DPRINT("[Master: %d] send msg of type SHUTDOWN_OK to Coordinator \n", rank);

                            MPI_Send(&receive_array[0], 1, MPI_INT, 0, SHUTDOWN_OK, MPI_COMM_WORLD);
                            flag_shutdown++;
                        }

                        break;


                    case LEAVE_REQUEST:
                        DPRINT("[Master: %d] received msg of type LEAVE_REQUEST \n", rank);

                        NUM_SERVERS--;
                        N = ((NUM_SERVERS - 1) / 2) + 1;

                        ServersId = remove_server(ServersId, status.MPI_SOURCE, Sortest_path);

                        receive_array[0] = status.MPI_SOURCE;
                        MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], LEAVE_CONFIRMATION, MPI_COMM_WORLD);
                        RPRINT("[Master: %d] send msg of type LEAVE_CONFIRMATION (%d) to Left \n", rank, receive_array[0]);

                        break;

                    case LEAVE_CONFIRMATION:
                        DPRINT("[Server: %d] received msg of type LEAVE_CONFIRMATION <%d>\n", rank, receive_array[0]);

                        if (neighbors[1] == receive_array[0]){
                            neighbors[1] = ServersId[0];
                        }

                        MPI_Send(&receive_array[0], 1, MPI_INT, 0, LEAVE_ACK, MPI_COMM_WORLD);
                        DPRINT("[Master: %d] send msg of type LEAVE_ACK to Coordinator \n", rank);

                        IPRINT("%d HAS LEFT THE SYSTEM \n", receive_array[0]);

                        break;

                    default:
                        break;
                }/* switch */



                /* Check if there are open requestst */
                for (i = 0; i < LOCAL_NUM ; i++){

                    queue = Hash_Table[i];

                    if(queue && registration_counter(queue) == (N + 1) ){
                        Client_id = client_id(queue);
                        request_type = registration_type(queue);
                        registration_decrease(queue);

                        RPRINT("@@@@@@@@@@@@@@@ THERE is a REQUEST %d for file %d by client %d \n", request_type, queue->key, queue->front->reg->id);
                        switch (request_type){
                            case RETRIEVE:


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
                                        if (upload_servers[j] == tmp){
                                        //if (ServersId[upload_servers[j]] == ServersId[tmp]){
                                            flag++;
                                            break;
                                        }
                                    }

                                    if (flag != 0){
                                        i--;
                                        continue;
                                    }

                                    //upload_servers[i] = ServersId[tmp];
                                    upload_servers[i] = tmp;
                                }

                                DPRINT("[Master: %d] send msg of type RETRIEVE <%d,(", rank, receive_array[1]);
                                receive_array[1] = file_id_take(queue);
                                for (i = 0; i < N; i++){
                                    //receive_array[0] = upload_servers[i];
                                    receive_array[0] = ServersId[upload_servers[i]];
                                    if (Sortest_path[upload_servers[i]])
                                        MPI_Send(&receive_array, 1, MPI_ARRAY, Sortest_path[upload_servers[i]], RETRIEVE, MPI_COMM_WORLD);
                                    else
                                        MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], RETRIEVE, MPI_COMM_WORLD);

                                    DPRINT("%d, \n", receive_array[0]);
                                }
                                DPRINT(")to Left\n");

                                break;

                            case UPDATE:

                                reg_ = first_registration(queue);
                                receive_array[0] = file_id_take(queue);
                                receive_array[1] = reg_->version;
                                receive_array[2] = 1;

                                MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], VERSION_CHECK, MPI_COMM_WORLD);

                                break;

                            default:
                                break;

                        }/* switch */  /* NOT HERE ABOVE */
                    }
                }


            } /* while */

            RPRINT("[Master: %d] End of Service\n", rank);
        } /* if SERVER)*/

        /*******************************************************  JUST SERVER    *************************************************************** */
        else{


            while (flag_shutdown == 0 ){

                MPI_Recv(&receive_array, 1, MPI_ARRAY, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                //DPRINT("[Server: %d] received msg of type: %d from: %d \n", rank, status.MPI_TAG, status.MPI_SOURCE);

                switch (status.MPI_TAG){

                    case UPLOAD:

                        DPRINT("[Server: %d] received msg of type UPLOAD <%d,%d> from %d\n", rank, receive_array[0], receive_array[1], status.MPI_SOURCE);

                        if (receive_array[0] == rank){

                            /* If I dont have the file add it */
                            if (!local_files[receive_array[1]]){

                                local_files[receive_array[1]] = new_file(receive_array[1], 1);

                                /* If there is long path use it*/
                                if (long_path){
                                    DPRINT("[Server: %d] send msg of type UPLOAD_ACK to master via Loong Path\n", rank);
                                    MPI_Send(&receive_array[1], 1, MPI_INT, Master, UPLOAD_ACK, MPI_COMM_WORLD);
                                }
                                else if (neighbors[0] == Master){
                                   // \\DPRINT("[Server: %d] send msg of type UPLOAD_ACK to master via Left\n", rank);
                                    MPI_Send(&receive_array[1], 1, MPI_INT, Master, UPLOAD_ACK, MPI_COMM_WORLD);
                                }
                                else{
                                   // \\DPRINT("[Server: %d] send msg of type UPLOAD_ACK to master via Left\n", rank);
                                    MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], UPLOAD_ACK, MPI_COMM_WORLD);
                                }
                            }
                            else{/* There is not this case */

                               // \\DPRINT("[Server: %d] send msg of type _UPLOAD_FAILED_ <%d> to Master via Left\n", rank, receive_array[1]);
                                MPI_Send(&receive_array[1], 1, MPI_INT, Master, UPLOAD_FAILED, MPI_COMM_WORLD);

                            }

                        }
                        else{

                           // \\DPRINT("[Server: %d] send msg of type UPLOAD <%d,%d> to Left \n", rank, receive_array[0], receive_array[1]);
                            //MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], UPLOAD, MPI_COMM_WORLD);

                            if (neighbors[0] == Master)
                                IPRINT("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&#\n")
                                //MPI_Send(&receive_array[1], 1, MPI_INT, Master, UPLOAD, MPI_COMM_WORLD);
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
                            // \\DPRINT("[Server: %d] send msg of type UPLOAD_ACK to Left \n", rank);
                            MPI_Send(&receive_array[1], 1, MPI_INT, Master, UPLOAD_ACK, MPI_COMM_WORLD);
                        }
                        else{
                            // \\DPRINT("[Server: %d] send msg of type UPLOAD_ACK to Left \n", rank);
                            MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], UPLOAD_ACK, MPI_COMM_WORLD);
                        }

                        break;

                    case RETRIEVE:

                        DPRINT("[Server: %d] received msg of type RETRIEVE <%d,%d>\n", rank, receive_array[0], receive_array[1]);

                        if (receive_array[0] == rank){

                            file_version = 0;

                            if (local_files[receive_array[1]]){/* If I  have the file */
                                file_version = local_files[receive_array[1]]->version;
                            }

                            receive_array[0] = receive_array[1];
                            receive_array[1] = file_version;

                            /* If there is long path use it*/
                            if (long_path){
                                DPRINT("[Server: %d] send msg of type RETRIEVE_ACK to master via Loong Path\n", rank);
                                MPI_Send(&receive_array, 1, MPI_ARRAY, Master, RETRIEVE_ACK, MPI_COMM_WORLD);
                            }
                            else{
                                DPRINT("[Server: %d] send msg of type RETRIEVE_ACK to master via Left\n", rank);
                                MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], RETRIEVE_ACK, MPI_COMM_WORLD);
                            }

                        }
                        else{

                           // \\DPRINT("[Server: %d] send msg of type RETRIEVE <%d,%d> to Left \n", rank, receive_array[0], receive_array[1]);

                            if (neighbors[0] == Master)
                                IPRINT("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&2\n")
                                //MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], RETRIEVE, MPI_COMM_WORLD); /* tototototootot */
                            else
                                MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], RETRIEVE, MPI_COMM_WORLD);
                        }

                        break;



                    case RETRIEVE_ACK:
                        DPRINT("[Server: %d] received msg of type RETRIEVE_ACK <%d,%d>\n", rank, receive_array[0], receive_array[1]);

                            /* If there is long path use it*/
                        if (long_path){
                            DPRINT("[Server: %d] send msg of type RETRIEVE_ACK to master via Loong Path\n", rank);
                            MPI_Send(&receive_array, 1, MPI_ARRAY, Master, RETRIEVE_ACK, MPI_COMM_WORLD);
                        }
                        else{
                           // \\DPRINT("[Server: %d] send msg of type RETRIEVE_ACK to master via Left\n", rank);
                            MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], RETRIEVE_ACK, MPI_COMM_WORLD);
                        }

                        break;

                    case UPDATE:
                        DPRINT("[Server: %d] received msg of type UPDATE for file %d with version %d \n", rank, receive_array[1], receive_array[2]);

                        if (receive_array[0] == rank){
                            file_id = receive_array[1];
                            if(local_files[file_id]){
                                if (local_files[file_id]->version <= receive_array[2]){
                                    local_files[file_id]->version = receive_array[2] + 1;
                                }
                            }else{
                                local_files[file_id] = new_file(file_id, (receive_array[2] + 1));
                            }

                            if (long_path){
                                DPRINT("[Server: %d] send msg of type UPDATE_ACK to master via Loong Path\n", rank);
                                MPI_Send(&receive_array, 1, MPI_ARRAY, Master, UPDATE_ACK, MPI_COMM_WORLD);
                            }
                            else{
                                // \\DPRINT("[Server: %d] send msg of type UPDATE_ACK to master via Left\n", rank);
                                MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], UPDATE_ACK, MPI_COMM_WORLD);
                            }


                        }
                        else{

                           // \\DPRINT("[Server: %d] send msg of type RETRIEVE <%d,%d> to Left \n", rank, receive_array[0], receive_array[1]);

                            if (neighbors[0] == Master)
                                IPRINT("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&2\n")
                                //MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], UPDATE, MPI_COMM_WORLD); /* tototototootot */
                            else
                                MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], UPDATE, MPI_COMM_WORLD);
                        }



                        break;

                    case VERSION_CHECK:
                        DPRINT("[Server: %d] received msg of type VERSION_CHECK for file %d with version %d and flag %d\n", rank, receive_array[0], receive_array[1], receive_array[2]);

                        if(receive_array[2] == 1){
                            if (local_files[receive_array[0]] && local_files[receive_array[0]]->version > receive_array[1]){ /* If I  have the file and version is bigger*/
                                file_version = local_files[receive_array[0]]->version;
                                receive_array[2] = 0;
                                if (long_path){
                                    MPI_Send(&receive_array, 1, MPI_ARRAY, Master, VERSION_CHECK, MPI_COMM_WORLD);
                                }
                                else{
                                    MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], VERSION_CHECK, MPI_COMM_WORLD);
                                }
                            }else{
                                // \\DPRINT("[Server: %d] send msg of type VERSION_CHECK to master via Left\n", rank);
                                MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], VERSION_CHECK, MPI_COMM_WORLD);
                            }
                        }else{
                            if (long_path){
                                MPI_Send(&receive_array, 1, MPI_ARRAY, Master, VERSION_CHECK, MPI_COMM_WORLD);
                            }
                            else{
                                MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], VERSION_CHECK, MPI_COMM_WORLD);
                            }

                        }

                        break;
                    case UPDATE_ACK:
                        DPRINT("[Server: %d] received msg of type UPDATE_ACK <%d,%d>\n", rank, receive_array[0], receive_array[1]);

                        /* If there is long path use it*/
                        if (long_path){
                            DPRINT("[Server: %d] send msg of type UPDATE_ACK to master via Loong Path\n", rank);
                            MPI_Send(&receive_array, 1, MPI_ARRAY, Master, UPDATE_ACK, MPI_COMM_WORLD);
                        }
                        else{
                            // \\DPRINT("[Server: %d] send msg of type UPDATE_ACK to master via Left\n", rank);
                            MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], UPDATE_ACK, MPI_COMM_WORLD);
                        }

                        break;

                    case REQUEST_SHUTDOWN:

                        DPRINT("[Server: %d] received msg of type REQUEST_SHUTDOWN \n", rank);
                        RPRINT("[Server: %d] send msg of type REQUEST_SHUTDOWN to Left (%d) \n", rank, neighbors[0]);

                        if(neighbors[0] == Master)
                            MPI_Send(&receive_array[0], 1, MPI_INT, Master, REQUEST_SHUTDOWN, MPI_COMM_WORLD);
                        else
                            MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], REQUEST_SHUTDOWN, MPI_COMM_WORLD);

                        flag_shutdown++;

                        break;

                    case LEAVE:
                        DPRINT("[Server: %d] received msg of type LEAVE \n", rank);

                        if(long_path){
                            MPI_Send(&receive, 1, MPI_INT, Master, LEAVE_REQUEST, MPI_COMM_WORLD);
                            DPRINT("[Server: %d] send msg of type LEAVE_REQUEST to Master \n", rank);
                        }
                        else{
                            MPI_Send(&receive, 1, MPI_INT, Master, LEAVE_REQUEST, MPI_COMM_WORLD);
                            DPRINT("[Server: %d] send msg of type LEAVE_REQUEST to Master \n", rank);
                        }

                        break;

                    case LEAVE_CONFIRMATION:
                        DPRINT("[Server: %d] received msg of type LEAVE_CONFIRMATION <%d>\n", rank, receive_array[0]);

                        if(rank != receive_array[0]){
                            i = neighbors[0];
                            if(neighbors[0] == receive_array[0]){
                                neighbors[0] = ServersId[NUM_SERVERS - 3];
                                i = receive_array[0];
                                RPRINT("[Server: %d] NEW LEFT NEIGHTBOR %d \n", rank, neighbors[0]);
                            }else if(neighbors[1] == receive_array[1]){
                                neighbors[1] = ServersId[0];
                                RPRINT("[Server: %d] NEW RIGTH NEIGHTBOR %d\n", rank, neighbors[1]);
                            }


                            NUM_SERVERS--;
                            N = ((NUM_SERVERS - 1) / 2) + 1;


                            ServersId = remove_server(ServersId, receive_array[0], NULL);

                            MPI_Send(&receive_array, 1, MPI_ARRAY, i, LEAVE_CONFIRMATION, MPI_COMM_WORLD);
                            RPRINT("[Server: %d] send msg of type LEAVE_CONFIRMATION (%d) to Left \n", rank, receive_array[0]);
                        }
                        else{
                            for (i = 0; i < LOCAL_NUM; i++){
                                if(local_files[i]){
                                    receive_array[0] = local_files[i]->id;
                                    receive_array[1] = local_files[i]->version;

                                    MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], TRANSFER, MPI_COMM_WORLD);
                                    DPRINT("[Server: %d] send msg of type TRANSFER to Left \n", rank);
                                }
                            }

                            receive_array[0] = rank;
                            MPI_Send(&receive_array, 1, MPI_ARRAY, neighbors[0], LEAVE_CONFIRMATION, MPI_COMM_WORLD);
                            RPRINT("[Server: %d] send msg of type LEAVE_CONFIRMATION (%d) to Left ##\n", rank, receive_array[0]);
                            flag_shutdown = 1;
                        }

                        break;

                    case TRANSFER:
                        DPRINT("[Server: %d] received msg of type TRANSFER <%d,%d>\n", rank, receive_array[0], receive_array[1]);

                        if(local_files[receive_array[0]]){
                            if(local_files[receive_array[0]]->version <  receive_array[1]){
                                local_files[receive_array[0]]->version =  receive_array[1];
                            }
                        }else{
                            local_files[receive_array[0]] = new_file(receive_array[0], receive_array[1]);
                        }

                        break;

                    default:
                        break;

                } /* switch */


            } /* WHILE */

            RPRINT("[Server: %d] End of Service\n", rank);
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


            switch (status.MPI_TAG){

                case UPLOAD:
                    DPRINT("[Client: %d] received msg of type UPLOAD_ <%d>\n", rank, receive);

                    /* If I dont have the file add it */
                    if(!local_files[receive]){
                        Open_requests++;
                        local_files[receive] = new_file(receive, 1);

                        DPRINT("[Client: %d] send msg of type UPLOAD <%d> to master \n", rank, receive);
                        MPI_Send(&receive, 1, MPI_INT, Master, UPLOAD, MPI_COMM_WORLD);
                    }
                    else{

                        DPRINT("[Client: %d] send msg of type UPLOAD_FAILED <%d> to Coordinator \n", rank, receive);
                        //MPI_Send(&receive, 1, MPI_INT, 0, UPLOAD_FAILED, MPI_COMM_WORLD);
                    }


                    break;

                case RETRIEVE:
                    DPRINT("[Client: %d] received msg of type RETRIEVE <%d>\n", rank, receive);

                    Open_requests++;
                    file_id = receive;

                    DPRINT("[Client: %d] send msg of type RETRIEVE <%d> to master \n", rank, receive);
                    MPI_Send(&receive, 1, MPI_INT, Master, RETRIEVE, MPI_COMM_WORLD);

                    MPI_Recv(&receive, 1, MPI_INT, Master, RETRIEVE_OK, MPI_COMM_WORLD, &status);

                    if(receive > 0){
                        if (!local_files[file_id]){
                            local_files[file_id] = new_file(file_id, receive);
                        }
                        else{
                            local_files[file_id]->version = receive;
                        }
                        DPRINT("[Client: %d] received msg of type RETRIEVE_OK <%d,%d>\n", rank, file_id, receive);
                        IPRINT("CLIENT %d RETRIEVED VERSION %d OF %d\n", rank, receive, file_id);
                    }else{
                        DPRINT("[Client: %d] received msg of type RETRIEVE_FAILED <%d>\n", rank, file_id);
                        IPRINT("CLIENT %d FAILED ΤΟ RETRIEVE %d\n", rank, file_id);
                    }

                    Open_requests--;

                    break;

                case UPDATE:
                    DPRINT("[Client: %d] received msg of type UPDATE <%d>\n", rank, receive);

                    Open_requests++;
                    file_id = receive;

                    receive_array[0] = file_id;
                    receive_array[1] = 0;
                    if (local_files[file_id]){
                        receive_array[1] = local_files[file_id]->version;
                    }

                    DPRINT("[Client: %d] send msg of type UPDATE <%d> to master \n", rank, receive);
                    MPI_Send(&receive_array, 1, MPI_ARRAY, Master, UPDATE, MPI_COMM_WORLD);

                    break;

                case UPLOAD_FAILED:

                    DPRINT("[Client: %d] received msg of type UPLOAD_FAILED <%d> from Master \n", rank, receive);

                    IPRINT("CLIENT %d FAILED TO UPLOAD FILE %d \n", rank, receive);

                    Open_requests--;

                    break;

                case UPDATE_OK:

                    DPRINT("[Client: %d] received msg of type UPDATE_OK <%d> from Master \n", rank, receive);

                    IPRINT("CLIENT %d UPDATED %d \n", rank, receive);
                    local_files[receive]->version++;
                    Open_requests--;

                    break;

                case UPDATE_FAILED:

                    DPRINT("[Client: %d] received msg of type UPDATE_FAILED <%d> from Master \n", rank, receive);

                    IPRINT("CLIENT %d UPDATE FAILED %d \n", rank, receive);

                    Open_requests--;
                    break;

                case VERSION_OUTDATED:

                    DPRINT("[Client: %d] received msg of type VERSION_OUTDATED <%d> from Master \n", rank, receive);

                    IPRINT("CLIENT %d CANNOT UPDATE %d WITHOUT MOST RECENT VERSION\n", rank, receive);

                    Open_requests--;
                    break;

                case REQUEST_SHUTDOWN:

                    flag_shutdown = 1;
                    DPRINT("[Client: %d] received msg of type REQUEST_SHUTDOWN_\n", rank);

                    break;

                case UPLOAD_OK:
                    DPRINT("[Client: %d] received msg of type UPLOAD_OK <%d> \n", rank, receive);
                    IPRINT("CLIENT %d UPLOADED FILE %d \n", rank, receive);

                    Open_requests--;

                    break;

                case BARRIER:
                    DPRINT("[Client: %d] received msg of type BARRIER \n", rank);
                    flag_barrier++;

                    break;

                case RETRIEVE_OK:
                    DPRINT("[Client: %d] received msg of type _RETRIEVE_OK_ <%d> \n", rank, receive);

                    IPRINT("CLIENT %d RETRIEVE FILE %d \n", rank, receive);

                    Open_requests--;

                    break;

                default:
                    break;

            } /* switch */

            if(flag_barrier && Open_requests == 0){
                flag_barrier = 0;
                RPRINT("[Client: %d] send msg of type BARRIER_ACK  \n", rank);
                MPI_Send(&receive, 1, MPI_INT, 0, BARRIER_ACK, MPI_COMM_WORLD);
            }

        }/* while */

        DPRINT("[Client: %d] send msg of type SHUTDOWN_OK  \n", rank);
        MPI_Send(&receive, 1, MPI_INT, 0, SHUTDOWN_OK, MPI_COMM_WORLD);

        RPRINT("[Client: %d] MY LOCAL FILES: ", rank);

        struct file *tmp;
        for (i = 0; i < LOCAL_NUM; i++){
            if (local_files[i])
            {
                tmp = local_files[i];
                 RPRINT("file <%d, %d> \n", tmp->id, tmp->version);
            }
        }

        RPRINT("[Client: %d] End of Service\n", rank);

    } /*else if(CLIENT)*/

} /*MPI_Peer()*/



void MPI_Master(struct master_struct *m, int *n)
{
    int k, i, j, tmp, L, flag = 0;
    int *long_paths;
    int response;

    MPI_Status status;

    k = NUM_SERVERS -3;
    L = k/4;


    srand(time(0));

    IPRINT("[Master: %d] >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>      I AM THE LEADER   <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n", *m->rank);


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

    sortest_paths(long_paths, n, m->ServersId, L);




    for (i = 0; i < L; i++){

        DPRINT("[Master: %d] send msg of type CONNECT to server: %d\n", rank, long_paths[i]);
        MPI_Send(m->leader_id, 1, MPI_INT, long_paths[i], CONNECT, MPI_COMM_WORLD);

        MPI_Recv(&response, 1, MPI_INT, long_paths[i], ACK, MPI_COMM_WORLD, &status);
        IPRINT("%d  CONNECTED TO %d \n", long_paths[i], rank);
    }

    IPRINT("SORTEST PATH : ");
    for (i = 0; i < NUM_SERVERS; i++){
        if(n[i] > 0)
            IPRINT("(%d->%d), ", m->ServersId[i], n[i]);
    }
    IPRINT("\n");

    for (i = 0; i < NUM_SERVERS; i++){

        if (m->ServersId[i] != *m->leader_id){
            DPRINT("[Master: %d] send msg of type LEADER_ELECTION_DONE to server: %d\n", rank, m->ServersId[i]);
            MPI_Send(m->leader_id, 1, MPI_INT, m->ServersId[i], LEADER_ELECTION_DONE, MPI_COMM_WORLD);

            //MPI_Recv(&response, 1, MPI_INT, m->ServersId[i], ACK, MPI_COMM_WORLD, &status);
            //DPRINT("[Master: %d] rcv msg ACK from %d \n", rank, status.MPI_SOURCE);
        }
    }

    DPRINT("[Master: %d] send msg of type LEADER_ELECTION_DONE to Coordinator \n", rank);
    MPI_Send(m->leader_id, 1, MPI_INT, 0, LEADER_ELECTION_DONE, MPI_COMM_WORLD);
}

void sortest_paths(int *connections, int *sortest_path_, int *neighbors, int l)
{
    int i, j;
    int cur;

    for (i = 0; i < NUM_SERVERS; i++){

        cur = neighbors[i];

        for (j = 0; j < l; j++){
            if (cur == connections[j])
                sortest_path_[i] = connections[j];
            else
                sortest_path_[i] = 0;
        }
    }

    cur = 0;
    for (i = (NUM_SERVERS-1); i >= 0; i--){
        if (sortest_path_[i] > 0)
            cur = sortest_path_[i];

        sortest_path_[i] = cur;
    }
    sortest_path_[NUM_SERVERS-1] = 0;
}

int *remove_server(int *Servers_old, int remove, int *n){
    int *Servers_new;
    int i, k = 0;
    int NUM_SERVERS_OLD = NUM_SERVERS + 1 ;

    Servers_new = (int *)malloc(sizeof(int) * NUM_SERVERS);
    if (Servers_new == NULL)
        ERRX(1, "Error malloc\n");

    for (i = 0; i < NUM_SERVERS_OLD; i++){

        if(Servers_old[i] != remove){
            Servers_new[k++] = Servers_old[i];
        }

        if(n)
            n[i] = 0;

    }

    free(Servers_old);

    return Servers_new;
}

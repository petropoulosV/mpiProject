#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
//#include <mpi.h>
#include <stddef.h>



#define DEBUG

#ifdef DEBUG
#define DPRINT(...) fprintf(stderr, __VA_ARGS__);
#else /* DEBUG */
#define DPRINT(...)
#endif /* DEBUG */

#define INFO

#ifdef INFO
#define IPRINT(...) fprintf(stderr, __VA_ARGS__);
#else /* INFO */
#define IPRINT(...)
#endif /* INFO */

/* error reporting helpers */
#define ERRX(ret, str) \
    do { fprintf(stderr, str "\n"); exit(ret); } while (0)


#define BUFFER_SIZE 1024

#define MAX(x, y) (((x) > (y)) ? (x) : (y))

#define ACK 0
#define ERROR 1
#define SERVER 2
#define CLIENT 3
#define START_LEADER_ELECTION 4
#define ELECTION_DONE 5
#define CANDIDATE_ID 6
#define LEADER_ELECTION_DONE 7
#define CONNECT 8
#define UPLOAD 9
#define UPLOAD_FAILED 10
#define REQUEST_SHUTDOWN 11
#define SHUTDOWN_OK 12
#define UPLOAD_ACK 13
#define UPLOAD_OK 14

int rank, world_size, NUM_SERVERS;


#endif /* _UTIL_H_ */
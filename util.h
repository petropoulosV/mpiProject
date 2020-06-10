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

#define BUFFER_SIZE 1024



#define ACK 0
#define ERROR 1
#define SERVER 2
#define CLIENT 3
#define START_LEADER_ELECTION 4
#define ELECTION_DONE 5
#define CANDIDATE_ID 6
#define LEADER_ELECTION_DONE 7

#define MAX(x, y) (((x) > (y)) ? (x) : (y))

int rank, world_size, NUM_SERVERS;
#endif /* _UTIL_H_ */
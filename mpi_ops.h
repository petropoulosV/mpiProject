#ifndef _MPI_OPS_H_
#define _MPI_OPS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <mpi.h>
#include <stddef.h>


struct master_struct{
    int *leader_id;
    int *ServersId;
    int *neighbors;
    int *rank;
};

void MPI_init(FILE *file);
void MPI_Peer(void);

void MPI_Master(struct master_struct *m, int *n);
void MPI_Simple_Server(struct master_struct *m);

void sortest_paths(int *connections, int *sortest_pats, int *neighbors, int l);
int *remove_server(int *Servers_old, int remove, int *n);

#endif /* _MPI_OPS_H_ */
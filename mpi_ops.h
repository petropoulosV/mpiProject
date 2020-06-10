#ifndef _MPI_OPS_H_
#define _MPI_OPS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <mpi.h>
#include <stddef.h>

void MPI_init(FILE *file);
void MPI_Peer(void);

#endif /* _MPI_OPS_H_ */
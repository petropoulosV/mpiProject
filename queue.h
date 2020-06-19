#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stdio.h>
#include <stdlib.h>
#include "files.h"

struct Queue{
    int key;
	struct queue_node * front;
	struct queue_node * back;
};



struct queue_node{
    struct registration *reg;
	struct queue_node *next;
};

struct  Queue * MakeEmptyQueue();
int IsEmptyQueue(struct Queue *Q);
struct queue_node *Front(struct Queue *Q);

void Enqueue(int id, int counter, int type, int versiom, struct Queue *Q);
struct registration *Dequeue(struct Queue *Q);

struct registration *first_registration(struct Queue *Q) ;
int registration_decrease(struct Queue *Q);

int client_id(struct Queue *Q);

#endif /* _QUEUE_H_ */
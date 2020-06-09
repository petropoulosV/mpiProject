#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stdio.h>
#include <stdlib.h>

struct Queue{
	struct queue_node * front;
	struct queue_node * back;
};

struct registration{
    int id;
    int counter;
    int type;
    int version;
};

struct queue_node{
    struct registration *reg;
	struct queue_node *next;
};

struct  Queue * MakeEmptyQueue();
int IsEmptyQueue(struct Queue *Q);
struct queue_node *Front(struct Queue *Q);
struct registration *Dequeue(struct Queue *Q);

#endif /* _QUEUE_H_ */
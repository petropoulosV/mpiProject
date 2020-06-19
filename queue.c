#include "queue.h"
#include "util.h"

struct  Queue * MakeEmptyQueue(int key){
	struct Queue *Q;

	Q = (struct Queue *)malloc(sizeof(struct Queue));

	if (!Q)
		ERRX(1, "Error malloc\n");

	Q->key = key;
	Q->back = NULL;
	Q->front = NULL;

	return Q;
}

int IsEmptyQueue(struct Queue *Q){

	if (!Q)
		return 1;

	if(!Q->front)
		return 1;
	else
		return 0;
}

struct queue_node *Front(struct Queue *Q)
{
	if (!Q)
		return NULL;

	if(IsEmptyQueue(Q))
		return NULL;
	else
		return Q->front;
}

struct registration *first_registration(struct Queue *Q)
{
	struct queue_node *tmp;

	if(IsEmptyQueue(Q))
		return NULL;

	tmp = Front(Q);

	if(tmp)
		return tmp->reg;

	return NULL;
}

int client_id(struct Queue *Q){

	struct registration *reg;


	reg = first_registration(Q);

	if(reg)
		return reg->id;
	else 
		return 0;
}


int registration_decrease(struct Queue *Q)
{
	struct registration *reg;

	reg = first_registration(Q);

	if (!reg)
		return -1;


	reg->counter --;

	if (reg->counter <= 0){

		reg = Dequeue(Q);

		//free(reg);
		return 0;
	}

	return reg->counter;
}

void Enqueue(int id, int counter, int type, int versiom, struct Queue *Q)
{
	struct registration *reg;
	struct queue_node *node;

	reg = (struct registration *)malloc(sizeof(struct registration));
	if(!reg)
		ERRX(1,"Error malloc\n");

	node = (struct queue_node *)malloc(sizeof(struct queue_node));
	if (!node)
		ERRX(1, "Error malloc\n");


	reg->id = id;
	reg->counter = counter;
	reg->type = type;
	reg->version = versiom;

	node->reg = reg;
	node->next = NULL;

	if(IsEmptyQueue(Q))
		Q->front = node;
	else
		Q->back->next = node;
	Q->back = node;
}

struct registration * Dequeue(struct Queue *Q){
	struct registration *reg = NULL;
    //struct queue_node *temp;

	if(!IsEmptyQueue(Q)){
		reg = Q->front->reg;
        //temp = Q->front;
		Q->front = Q->front->next;

		if(Q->front == NULL)
			Q->back = NULL;

	}

    return reg;
}
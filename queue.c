#include "queue.h"


struct  Queue * MakeEmptyQueue(){
	struct Queue *Q;

	Q=(struct Queue *)malloc(sizeof(struct Queue));
	if(Q==NULL){
		printf("Error no memory");
        return NULL;
	} 
	Q->back=NULL;
	Q->front=NULL;
	return Q;
}

int IsEmptyQueue(struct Queue *Q){
	if(Q->front==NULL)
		return 1;
	else
		return 0;
}

struct queue_node *Front(struct Queue *Q)
{
    if(IsEmptyQueue(Q))
		return NULL;
	else
		return Q->front;
}

void Enqueue(struct registration *reg, struct Queue *Q){
	struct queue_node *p;

	p = (struct queue_node *)malloc(sizeof(struct queue_node ));
	if(p == NULL){
		printf("Error no memory");
	} 


    p->reg = reg;
	p->next=NULL;

	if(IsEmptyQueue(Q))
		Q->front = p;
	else
		Q->back->next=p;
	Q->back=p;
}

struct registration * Dequeue(struct Queue *Q){
	struct registration *reg = NULL;
    struct queue_node *temp;

	if(!IsEmptyQueue(Q)){
		reg = Q->front->reg;
        temp = Q->front;
		Q->front = Q->front->next;

		if(Q->front == NULL)
			Q->back = NULL;

        free(temp);
	}

    return reg;
}
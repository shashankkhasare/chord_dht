#include "thread_struct.h" 
#include<stdlib.h>
#include<errno.h>
#include<signal.h>

typedef struct node {
	mythread * val ; 
	struct node * next ; 
}node ; 
typedef struct queue { 
	node * front ; 
	node * end; 
}queue; 

void queue_init(queue * q){
	q -> front = NULL;
	q -> end = NULL;
}


void push(queue * q, mythread * t){
	node * n = (node * ) malloc( sizeof(node));
	if(n == NULL)
	{
		perror("Error  : " ) ; 
		exit(errno);
	}
	if ( q == NULL){
		perror("Queue is null : " ); 
		exit(errno);
	}
	n -> val = t; 
	n-> next = NULL;
	if ( !(q -> front ))
	{
		q -> front = q -> end = n; 
		return ; 
	}

	(q -> end) -> next = n; 
	q -> end = n; 
}

void  pop(queue * q){
	if ( !(q -> front)){
		perror("Pop attempted on empty queue \n");
		exit(1);
	}
	if (( q -> front ) == ( q -> end )){
		free( q -> front ) ; 
		( q -> front ) = (q -> end ) = NULL;
		return;
	}
	node * n  = q -> front ; 
	q -> front = q -> front -> next ; 
	free(n);
}

mythread * front(queue * q ){
	if ( !(q -> front)){
		perror("Front element read attempted on empty queue \n");
		exit(1);
	}
	return q -> front -> val  ; 
}


mythread * search_by_tid(queue * q, int  tid ) {
	if ( !(q -> front)){
		perror("Search on empty attempted \n");
		exit(1);
	}
	node * n  = q -> front; 
	while(n){
		if (( (n ->val) -> thread_id ) == tid ){
			return n -> val;
		}

		n = n -> next ; 
	}
	return NULL;
}

void ready_all_threads(queue * q){
	if ( !(q -> front)){
		return ; 
	}
	node * n = (q -> front ) ; 
	while(n){
		(n -> val ) -> state = READY; 
		n = n -> next; 
	}

}

void delete_from_queue(queue * q, int tid){
	if ( !(q -> front)){
		perror("Search on empty attempted \n");
		exit(1);
	}
	node * n  = q -> front; 

	// if thread to be deleted is the first element in the queue 
	if ( n -> val -> thread_id  == tid ) {
		q -> front  = q -> front -> next ;
		free(n -> val);
		free(n);
		return ; 
	}

	// search remaining threads for the specified tid and delete the thread fron the queue 
	while(n){

		if ( n -> next  == NULL) {
			printf("TID attempted to delete not found \n" );
			return;
		}
		if (( (n ->next) -> val-> thread_id ) == tid )
			break;

		n = n -> next ; 
	}

	node * d = n -> next ; 	

	// if tail is being deleted , update the queue for new tail 
	if ( n -> next -> next == NULL){
		q -> end = n; 
		n -> next = NULL;
	}else{
		n -> next = d -> next; 
	}

	free(d -> val ) ; 
	free(d);

}


#include<stdio.h>
#include <setjmp.h>
#include <signal.h>
#include "queue.h"
#include<sys/time.h>

#define SECOND 10000
#ifdef __x86_64__
// code for 64 bit Intel arch

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

address_t mangle(address_t addr)
{
	address_t ret;
	asm volatile("xor    %%fs:0x30,%0\n"
			"rol    $0x11,%0\n"
			: "=g" (ret)
			: "0" (addr));
	return ret;
}

#else
// code for 32 bit Intel arch

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5

address_t mangle(address_t addr)
{
	address_t ret  ;
	asm volatile("xor    %%gs:0x18,%0\n"
			"rol    $0x9,%0\n"
			: "=g" (ret)
			: "0" (addr));
	return ret;
}

#endif

#define MAX_THREADS 100
char stack[MAX_THREADS][4096];
jmp_buf jbuf[MAX_THREADS];

jmp_buf main_buf; 
//int running[MAX_THREADS];

char thread_pool[MAX_THREADS];
int allocate_ptr= 5; 
int isSchedulerRunning = 0; 

queue * thread_queue = NULL;

int allocate_tid(){
	if ( isSchedulerRunning) 
		ualarm( 0, 0 ) ; 
	int tid, i; 
	for(i = allocate_ptr ; i !=  (allocate_ptr + MAX_THREADS - 1) % MAX_THREADS; i = ( i + 1 ) % MAX_THREADS){
		if ( thread_pool[ i ] == 0)
		{
			allocate_ptr  = i ; 
			thread_pool[i] = 1;
			if ( isSchedulerRunning) 
				ualarm ( 5 * SECOND, 5 * SECOND) ;
			return i; 
		}
	}
	clean();
	printf("Max thread capacity exceeded \n");
	exit(0);
}


/* Decided to abandon this with  the reason that, freeing the used thread space
 * is the users responsibility and not the threading library 
void garbage_collector(){

	if ( isSchedulerRunning ) 
		ualarm ( 0 , 0); 

	

	if ( isSchedulerRunning ) 
		ualarm ( 5 * SECOND, 5 * SECOND); 

}
*/

void dispatch(int sig)
{
	ualarm(0, 0);

	/**
	 * previously running thread is at the front of the queue 
	 * If the thread was previously running, we save its context and change its state RUNNING -> READY
	 * We move this thread to the back of the queue */

	mythread * tp = front(thread_queue);
	if ((tp -> hasContext) ){
		if ( tp -> state == RUNNING ){
			tp -> state = READY;

			unsigned long long execn_time; 
			struct timeval  now, before; 
			gettimeofday(&now, NULL);

			before = tp -> execution_start_time;
			// execution time in micro seconds 
			execn_time = ((now.tv_sec - before.tv_sec)*1000) + ((now.tv_usec - before.tv_usec)/ 1000);
			//printf ( " executed for %llu", execn_time);
			(tp -> stat).total_exec_time += execn_time;

			// set the wait start time 
			gettimeofday( &(tp -> wait_start_time), NULL);
		}
		if (sigsetjmp(jbuf[tp -> thread_id],1) == 1)
			return;
	}
	pop(thread_queue);
	push(thread_queue, tp);

	// search next thread in the queue, whose state is READY  and run it 
	mythread * next_tp;
	while (1){
		next_tp = front(thread_queue);
		if ((next_tp -> state) == READY) {
			break ;
		} 
		if ( next_tp -> state == SLEEPING ) {
			// check if the sleeping time of the thread is over 
			time_t current_time; 
			time(&current_time);
			if ( next_tp -> waketime < current_time){
				// reset its wait counter as it was not waiting but sleeping, to avoid wait time miscalculations  
				gettimeofday(&(tp -> wait_start_time), NULL);
				break;
			}
			else{
				pop(thread_queue);
				push(thread_queue, next_tp);
			}
		}
		else if ( next_tp -> state == BLOCKED_JOIN) {
			mythread * j = search_by_tid(thread_queue,  next_tp -> blocked_join_on_tid);
			if ( j -> state == FINISHED ) 
			{
				next_tp -> state = READY;
				break;
			}
			else{
				pop(thread_queue);
				push(thread_queue, next_tp);
			}
		}
		else{
			pop(thread_queue);
			push(thread_queue, next_tp);
		}
	}
	next_tp -> hasContext =1;
	next_tp -> state = RUNNING ; 
	(next_tp -> stat).no_of_bursts += 1;

	// update the start time of the execution 
	gettimeofday(&(next_tp -> execution_start_time), NULL);

	// update the total waited time of the thread 
	struct timeval now , before; 
	gettimeofday( & now , NULL);
	before = (next_tp -> wait_start_time);

	unsigned long long wait_time = ((now.tv_sec - before.tv_sec)  * 1000 ) + ((now.tv_usec - before.tv_usec) / 1000); 
	//printf("Waited for %llu\n", wait_time);
	(tp -> stat).total_wait_time+= wait_time;

	ualarm(5*SECOND, 5*SECOND);
	siglongjmp(jbuf[next_tp -> thread_id],1);
}
void launch(){
	mythread * tp = front ( thread_queue); 	
	if ( tp -> hasArgs ) 
	{
		tp -> retVal = tp -> funcWitharg(tp -> funcArg);
	}
	else
	{
		tp -> f();
	}

	tp -> state = FINISHED;
	delete(getID());

	while(1);
}
int create(void (*f) ( void )){

	if ( thread_queue == NULL){
		thread_queue = ( queue * ) malloc(sizeof(queue));
		queue_init(thread_queue);
	}
	// choose tid from the thread pool 
	//int tid = thread_pool;
	//thread_pool += 1;
	int tid = allocate_tid();
	mythread * tp = ( mythread * ) malloc ( sizeof ( mythread ) ) ; 
	tp -> thread_id = tid;
	tp -> hasContext = 0;
	tp -> state = CREATED; 
	tp -> hasArgs = 0;
	tp -> f = f; 
	gettimeofday ( & ( tp -> wait_start_time), NULL);
	init_stats(tp);

	push(thread_queue, tp);

	sigsetjmp(jbuf[tid],1);
	jbuf[tid][0].__jmpbuf[JB_SP] = mangle((unsigned) stack[tid] + 4096);
	jbuf[tid][0].__jmpbuf[JB_PC] = mangle((unsigned) launch);

	return tid; 
}


int createWithArgs(void* (*f) ( void *), void * arg){

	if ( thread_queue == NULL){
		thread_queue = ( queue * ) malloc(sizeof(queue));
		queue_init(thread_queue);
	}
	// choose tid from the thread pool 
	//int tid = thread_pool;
	//thread_pool += 1;
	int tid = allocate_tid();
	mythread * tp = ( mythread * ) malloc ( sizeof ( mythread ) ) ; 
	tp -> thread_id = tid;
	tp -> hasContext = 0;
	tp -> state = CREATED; 
	tp -> hasArgs = 1;
	tp -> funcWitharg = f ;
	tp -> funcArg = arg;  
	gettimeofday ( & ( tp -> wait_start_time), NULL);
	init_stats(tp);

	push(thread_queue, tp);

	sigsetjmp(jbuf[tid],1);
	jbuf[tid][0].__jmpbuf[JB_SP] = mangle((unsigned) stack[tid] + 4096);
	jbuf[tid][0].__jmpbuf[JB_PC] = mangle((unsigned) launch);

	return tid; 
}

int getID()
{
	return front(thread_queue) -> thread_id ; 
}

void start()
{

	int i ; 
	ready_all_threads(thread_queue);
	signal(SIGALRM, dispatch);
	isSchedulerRunning =  1;
	ualarm(5*SECOND, 5*SECOND);
	i = sigsetjmp(main_buf, 1);
	if ( i == 1)
	{
		return;
	}
	while(1);
}



void sleep_t(int secs)
{
	ualarm(0, 0);

	time_t wt;
	time(&wt);
	mythread * tp = front(thread_queue); 
	tp -> state = SLEEPING;
	tp -> waketime = wt + secs; 
	(tp -> stat).total_sleep_time = secs * 1000; 
	ualarm(5*SECOND, 5*SECOND);
	kill(getpid(), SIGALRM);
}

void run(int tid ){
	mythread * p  = search_by_tid(thread_queue, tid)	;
	if ( p )
		p -> state = READY;
	else
		perror("Thread with said thread id doesn't exist \n");
}
void suspend(int tid ){
	mythread * p  = search_by_tid(thread_queue, tid)	;
	if ( p )
		p -> state = SUSPENDED;
	else
		perror("Thread with said thread id doesn't exist \n");
}
void resume(int tid ){
	run(tid);
}

void yield()
{
	kill(getpid(), SIGALRM);	
}

void delete( int tid ) {
	ualarm(0, 0);
	delete_from_queue(thread_queue, tid);
	thread_pool[tid] = 0;
	ualarm(5*SECOND, 5*SECOND);
	kill(getpid(), SIGALRM);	
}

void calculate_stats(mythread * tp ) 
{
	statistics stat = tp -> stat; 
	if ( stat.no_of_bursts == 0) 
		return; 
	stat.avg_execution_time = stat.total_exec_time / stat.no_of_bursts ;
	stat.avg_waiting_time = stat.total_wait_time / stat.no_of_bursts;
	tp -> stat = stat; 
}
void print_stat(mythread * tp ) 
{

	calculate_stats( tp ) ; 
	statistics stat = tp -> stat; 

	printf("____Thread Statistics ______\n");
	printf ( "Thread ID : %d\n", *stat.thread_id);
	switch(*stat.state){

		case RUNNING:
			printf("State : Running\n " ) ; 
			break;
		case READY:
			printf("State : Ready \n" ) ; 
			break;
		case SUSPENDED:
			printf("State : Suspended\n" ) ; 
			break;
		case FINISHED:
			printf("State : Finished\n" ) ; 
			break;
		case CREATED:
			printf("State : Created\n" ) ; 
			break;
		case SLEEPING:
			printf("State : Sleeping \n" ) ; 
			break;
		case BLOCKED_JOIN:
			printf("State : Blocked join\n" ) ; 
			break;
		default:
			printf("State : N/A");

	}
	printf ( "No. of bursts : %lld\n", stat.no_of_bursts);
	printf ( "Total execution time ( msecs) : %lld\n", stat.total_exec_time);
	printf ( "Total sleep time ( msecs) : %lld\n", stat.total_sleep_time);
	printf ( "Average execution time ( msecs) : %lld\n", stat.avg_execution_time);
	printf ( "Average waiting time ( msecs) : %lld\n", stat.avg_waiting_time);
}


void clean(){

	ualarm( 0, 0);
	while(1){
		if ( thread_queue -> front != NULL){
			mythread * tp = front( thread_queue); 
			print_stat(tp);
			pop(thread_queue);
		}
		else
			break;
	}
	free(thread_queue);
	siglongjmp(main_buf, 1);
}

void print_all_thread_stat(){

	node * n = thread_queue -> front;
	while(1){
		if ( n != NULL){
			print_stat(n -> val);
			n = n -> next; 
		}
		else
			break;
	}
}
void join( int tid){
	mythread * current_thread = front(thread_queue);

	current_thread -> state = BLOCKED_JOIN;
	current_thread ->  blocked_join_on_tid = tid;
	kill ( getpid(), SIGALRM);
}

void *GetThreadResult(int tid) {
	mythread * tp = search_by_tid( thread_queue , tid ) ; 
	while ( 1) {
		if ( tp -> state != FINISHED) 
			kill(getpid(), SIGALRM); 
		else{
			return tp -> retVal; 
		}
	}
}
	

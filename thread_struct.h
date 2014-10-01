#ifndef THREAD_STRUCT_H
#define THREAD_STRUCT_H

#include<setjmp.h>
#include<time.h>
#include<sys/time.h>

enum thread_state {RUNNING ,READY , SUSPENDED, FINISHED , CREATED, SLEEPING, BLOCKED_JOIN};


typedef struct statistics
{
	//1. thread id
	//2. state (running, ready, sleeping, suspended).
	//3. number of bursts (none if the thread never ran).
	//4. total execution time in msec(N/A if thread never ran).
	//5. total requested sleeping time in msec (N/A if thread never slept).
	//6. average execution time quantum in msec (N/A if thread never ran).
	//7. average waiting time (status = READY) (N/A if thread never ran).
	int *thread_id; 
	enum thread_state *state; 
	unsigned long long no_of_bursts;
	unsigned long long total_exec_time; 
	unsigned long long total_wait_time; 
	unsigned long long total_sleep_time; 
	unsigned long long avg_execution_time ; 
	unsigned long long avg_waiting_time ; 


}statistics; 
typedef struct mythread{
	int hasContext ;
	void (*f) ();


	void * (*funcWitharg)(void *);
	void * funcArg; 
	void * retVal; 
	int hasArgs;

	enum thread_state state;
	int thread_id;
	time_t waketime; 
	statistics stat; 
	int  blocked_join_on_tid; 

	struct timeval execution_start_time; 
	struct timeval wait_start_time; 

}mythread; 



void init_stats ( mythread * tp ) {
	
	statistics sp;

	sp.thread_id = &(tp -> thread_id);
	sp.state = &(tp -> state);
	sp.no_of_bursts = 0;
	sp.total_exec_time = 0;
	sp.total_sleep_time = 0 ; 
	sp.avg_execution_time = 0  ; 
	sp.avg_waiting_time = 0 ; 
	sp.total_wait_time =0;

	tp -> stat = sp;

}
#endif

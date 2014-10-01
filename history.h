#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdio.h>

char * getSelfLocation(){

	char link[1024];
	int n; 
	char  * exe;
	exe =  ( char * ) malloc ( 1024 * sizeof(char)); 
	if ( exe == NULL)
	{
		printf("Error allocating memory. Exiting \n");
		exit(0);
	}
	snprintf(link,sizeof link,"/proc/%d/exe",getpid());
	n = readlink(link,exe,1024);
	if ( n ==-1)
	{
		fprintf(stderr,"ERRORRRRR\n");
		exit(1);
	}
	exe[n] = 0;
	return exe; 
}


void enable_history(){

	char command[2048] = "rlwrap " ;
	strcat(command , getSelfLocation());
	strcat ( command , " " ) ; 
	strcat ( command , " --enabled" ) ; 

	system(command);
	exit(0);

}

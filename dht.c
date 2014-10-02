#include "dht.h" 
#include "history.h" 
#include "communication_structures.h" 
#include "mythread.h" 
nodeinfo_t nodeinfo ; 

int isConnectedToRing  = 0 ; 
char port[32] = "3410";

void shell(){
	char *command[10];
	char command_string[250];
	int i ;
	char * token;

	do { 
		fgets(command_string, 250, stdin);
		if (strlen(command_string) != 1)
			command_string[strlen(command_string) - 1] = 0;
		i = 0 ; 
		token = strtok ( command_string , " ");

		while ( token != NULL){
			command[i] = token; 
			i++; 
			token = strtok(NULL, " "); 
		}
		command[i] = NULL;

		if ( command[0] == NULL)
			continue;
		if ( 0 == strcmp( command[0] , "help"))
		{
			printf("help\t: Displays the help contents \n");
			printf("port\t: Displays the port to which the server is currently listening to. \n");
			printf("dump\t: Displays the finger table of the node it is run on. \n");
			printf("port <portno>\t: Sets the port no to the specified port on which the server will listen\n");
			printf("create\t: Creates a new ring and starts listening to the requests on the specified/default port\n");
			printf("join <ip> <port>\t: Joins the ring of which the node with specified ip port is a member\n");
			printf("quit\t: Quit the interactive shell\n");
			printf("put <key> <value>\t: Stores the key-value pair in the hash table\n");
			printf("get <key>\t: Get the value of the specified key if present in the hash table \n");
			printf("predecessor\t: Prints the address of the predecessor node in the ring \n");
			printf("successor\t: Prints the address of the successor node in the ring \n");
			
		}else if ( 0 == strcmp( command[0] , "port")){
			if ( command[1] == NULL){
				printf("Port set to : %s\n" , port) ; 
			}else{
				if ( isConnectedToRing) {
					printf("Cannot set after the node has joined the ring \n");
					continue;
				}
				else
				{
					strcpy(port, command[1]);
					init_node_info(&nodeinfo, port);
				}
			}


		}else if ( 0 == strcmp( command[0] , "create"))
		{

			if ( isConnectedToRing){
				printf("Already connected to a ring. Can't create another one \n");
			}else{
				pthread_t tid; 
				pthread_create(&tid, NULL, create_ring , &nodeinfo);
			}

		}else if ( 0 == strcmp( command[0] , "join")){
			printf("join command\n");
			if ( i < 3)
			{
				printf("Insufficient arguments \n");
				continue;
			}
			if ( isConnectedToRing)
			{
				printf("Already connected to a ring \n");
				continue;
			}

			join_ring(command[1], command[2]);

		}else if ( 0 == strcmp( command[0] , "quit")){
			destroy_ring_rpc();
		}else if ( 0 == strcmp( command[0] , "put")){
			if ( i < 3)
			{
				printf("Insufficient arguments \n");
				continue;
			}
			if ( !isConnectedToRing)
			{
				printf("Not connected to any ring. Either create or join one !!\n");
			}else{
				put(command[1], command[2]);
			}
		}else if ( 0 == strcmp( command[0] , "predecessor")){
			print_predecessor_info();
		}else if ( 0 == strcmp( command[0] , "successor")){
			print_successor_info();
		}else if ( 0 == strcmp( command[0] , "dump")){
			if ( i == 1)
			{
				print_finger_table(&nodeinfo);
				continue;
			}
			else if ( i == 3)
				dump(command[1], command[2]);
		}else if ( 0 == strcmp( command[0] , "dumpall")){
			if ( !isConnectedToRing)
			{
				printf("Not connected to any ring. Either create or join one !!\n");
			}else{
				dumpall();
			}
		}else if ( 0 == strcmp( command[0] , "get")){
			if ( i < 2)
			{
				printf("Insufficient arguments \n");
				continue;
			}
			if ( !isConnectedToRing)
			{
				printf("Not connected to any ring. Either create or join one !!\n");
			}else{
				get(command[1]);
			}
		}else if ( 0 == strcmp( command[0] , "finger")){
			if ( !isConnectedToRing)
			{
				printf("Not connected to any ring. Either create or join one !!\n");
			}
			else
				finger();
		}else{
			printf("Unrecognised command. How about using 'help' ? \n");
		}

		command[0] = NULL;
	}while(1);
}
void siginthandler(int signo)
{
	clean();
}

int main(int argc, char * argv[]) {
	extern node_t NULL_NODE;
	extern data_t NULL_DATA;
	extern sem_t data_sem;
	
	sem_init(&data_sem, 0, 1);

	
	NULL_NODE.key = (key_t)pow(2,M+1);
	bzero(&(NULL_NODE.address), sizeof(NULL_NODE.address));
	NULL_DATA.id = (key_t)pow(2,M+1);

	if ( argc == 1) 
	{
		enable_history();

	}else if ( argc == 2 ) {
		if ( 0 != strcmp ( argv[1] , "--enabled" )) {
			printf("Unknown argument : %s " , argv[1]);
			exit(0);
		}
	}else 
	{
		printf("Too many arguments passed \n");
		exit(0);

	}

	init_node_info(&nodeinfo, port);
	signal( SIGINT , siginthandler);
	pthread_t tid; 
	pthread_create(&tid, NULL, shell, NULL);
	pthread_join(tid, NULL);
}

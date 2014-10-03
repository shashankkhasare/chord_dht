#ifndef DHT_H
#define DHT_H
#define M 6
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<unistd.h>
#include<openssl/sha.h>
#include<stdio.h>
#include<netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
#include "communication_structures.h"
//#include <pthread.h>
#include <semaphore.h>

#define debug	0

typedef unsigned int dhtkey_t; 
typedef struct node_t {
	dhtkey_t key ; 
	struct sockaddr_in address; 

}node_t ; 

typedef struct interval_t {
	dhtkey_t start , end; 
}interval_t ; 

typedef struct finger_t{
	dhtkey_t start ;
	interval_t interval ; 
	node_t successor; 
}finger_t; 

typedef struct hash_table_t{
	char *keys[50];
	char *values[50];
}hash_table_t;

typedef struct data{
	key_t id;
	char key[1024];
	char value[1024];
}data_t;

typedef struct nodeinfo_t 
{
	node_t self;
	node_t successor;
	node_t predecessor;
	finger_t finger[M+1];
	int self_data_index;
	int other_data_index;
	data_t self_data[1024];
	data_t other_data[1024];
}nodeinfo_t ; 

data_t NULL_DATA;

void print_finger_table(nodeinfo_t * );
void startServer();
node_t find_predecessor(key_t);
node_t find_successor(key_t );
dhtkey_t getHashID(char * , int, int);
void redist();
node_t NULL_NODE;
sem_t data_sem;

/**
 * Hashes the "size"  no of characters from c and returns the prefix 
 * no of bits from the SHA1 hash of the input data
 * prefix is restricted to 32
 * */

data_t * search(char *key){
	key_t hashed_key = getHashID(key , strlen(key), M);
	data_t *Where;
	int i;
	extern nodeinfo_t nodeinfo;
	if(hashed_key == nodeinfo.self.key)
		Where = nodeinfo.self_data;
	else
		Where = nodeinfo.other_data;	
	for(i = 0; i<1024; i++)	
		if(Where[i].id == hashed_key && strcmp(Where[i].key, key) == 0)
			return &(Where[i]);
	return &NULL_DATA;		
} 

void insert(data_t * Where, data_t d){
	extern nodeinfo_t nodeinfo;
	int *index;
	data_t *k = search(d.key);
	if(k -> id != NULL_DATA.id){
		strcpy(k -> value,  d.value);
		return;
	}	
	
	if(Where == nodeinfo.self_data)
		index = &(nodeinfo.self_data_index);
	else
		index = &(nodeinfo.other_data_index);
	Where[*index] = d;
	*index += 1;
}

void delete_data(data_t *Where, int i){
	extern nodeinfo_t nodeinfo;
	int *index, j;
	if(Where == nodeinfo.other_data)
		index = &(nodeinfo.other_data_index);
	else
		index = &(nodeinfo.self_data_index);	
		
	for(j = i; j < *index-1; j++)
		Where[j] = Where[j+1];
	*index = *index - 1;
}

void print_data(data_t d){
	if(d.id != NULL_DATA.id){
		printf("Successor : %u\n",d.id);
		printf("[%s] => %s\n", d.key, d.value);
	}
	else
		printf("Given key does not exist.\n");
}

dhtkey_t getHashID(char * c, int size , int prefix){

	unsigned int result =0 , i; 
	char str[32];
	char  hash[21];

	FILE * pf ;
	strcpy(str,"./mysha ");
	strcat(str, c);
	pf = popen(str, "r");
	fgets(hash, 21, pf);
	hash[20] = 0;

	for ( i =0 ; i < 4; i++)
	{
		result += (unsigned int ) hash[i];
		result = result << 8; 
	}


	result = result >> ( 32 - prefix ) ; 

	return result ; 

}
struct sockaddr_in getSelfAddress(){
	struct ifaddrs *ifap, *ifa;
	struct sockaddr_in *sa;
	struct sockaddr_in ret;

	getifaddrs (&ifap);
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family==AF_INET) {
			sa = (struct sockaddr_in *) ifa->ifa_addr;
			ret = *sa; 
		}
	}

	freeifaddrs(ifap);
	return ret; 
}


/**
 * Called before joining/creating a ring 
 * It initialises the ip address and hashes to it to get the key. 
 */
void init_node_info(nodeinfo_t * np, char *port){
	struct sockaddr_in address = getSelfAddress();
	char *addr; 
	char add_string[32];
	int i;
	addr = inet_ntoa(address.sin_addr);
	strcpy(add_string, addr);
	strcat(add_string, port);
	//
	address.sin_port = htons(atoi(port));
	address.sin_family = AF_INET;
	// set self 
	np ->self.key = getHashID(add_string, strlen(add_string), M);
	np ->self.address = address; 
	for ( i = 1; i < 1+M; i++){
		finger_t f ; 
		f.start = ((int)(np -> self.key +  pow(2, i -1))) % ((int)pow( 2, M)); 
		f.interval.start = f.start;
		f.interval.end = ((int)(np -> self.key +  pow(2, i))) % ((int)pow( 2, M));
		np -> finger[i]  =f; 
		np ->  finger[i].successor = NULL_NODE;
	}
	np -> successor = NULL_NODE;
	np -> predecessor = NULL_NODE;	
	printf("Fetched IP address : %s:%s\n", inet_ntoa((*np).self.address.sin_addr), port);
	printf("Hashed address %d \n", (*np).self.key);
	
	np->self_data_index = 0;
	np->other_data_index = 0;
}

void print_finger_table(nodeinfo_t * np){
	int i; 
	printf("Finger table of node %s:%d\n", inet_ntoa(np -> self.address.sin_addr), ntohs(np -> self.address.sin_port));
	printf("start\tinterval\tsuccessor\n" );
	for ( i =1 ; i <= M; i ++)
	{
		finger_t f = np -> finger[i];
		if(np->finger[i].successor.key != NULL_NODE.key)
			printf("%u\t[%u, %u)\t\t%u\n", f.start,f.interval.start, f.interval.end, f.successor.key);
		else
			printf("%u\t[%u, %u)\t\tNULL\n", f.start,f.interval.start, f.interval.end);	
	}
	if(np -> successor.key != NULL_NODE.key)
		printf("successor: %u \n", np -> successor.key ) ; 
	else	
		printf("successor: NULL \n") ;
		 
	if(np -> predecessor.key != NULL_NODE.key)
		printf("predecessor: %u \n", np -> predecessor.key ) ; 
	else	
		printf("predecessor: NULL \n") ; 
		
	printf("Data at %u:\n",np->self.key); 	
	
	printf("Self Data:\n"); 	
	for(i=0; i< np->self_data_index; i++)
		print_data(np->self_data[i]);

	printf("Other Data:\n"); 	
	for(i=0;i< np->other_data_index; i++)
		print_data(np->other_data[i]);
}

void print_predecessor_info(){

	extern nodeinfo_t nodeinfo; 
	node_t p = nodeinfo.predecessor; 
	if ( p.key == NULL_NODE.key){
		printf("Predecessor not found\n");
		return;
	}

	printf("Predecessor Information \n");
	printf("Hashed key value : %u \n", p.key ) ; 
	printf("IP address : %s \n", inet_ntoa(p.address.sin_addr));
	printf("Server port %d \n", htons(p.address.sin_port));	
}

void print_successor_info(){

	extern nodeinfo_t nodeinfo; 
	node_t p = nodeinfo.successor; 
	if ( p.key == NULL_NODE.key){
		printf("Successor not found\n");
		return;

	}

	printf("Successor Information \n");
	printf("Hashed key value : %u \n", p.key ) ; 
	printf("IP address : %s \n", inet_ntoa(p.address.sin_addr));
	printf("Server port %d \n", htons(p.address.sin_port));	
}
/***************************** S O C K E T ****************************************/
int getSocket(char *ip, char * port){
	int sock_fd;
	struct sockaddr_in server;

	if ( ( sock_fd = socket(AF_INET, SOCK_STREAM, 0) ) < 0 ){
		perror("Socket : Unable to create socket.\n");
		return errno;
	}
	
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr( ip );
	server.sin_port = htons(atoi(port));

	if( connect(sock_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
		//printf("Ring Destroyed. Quitting.\n");
		//exit(0);
		printf("Unable to connect to %s:%s\n", ip, port);
		return -1;
	}
	if ( debug ) printf("Opened socket on remote %s : %s \n", ip, port);
	return sock_fd;
}

int getSocketFromNode(node_t n){
	char * ip;
	char port[10];
	ip = inet_ntoa(n.address.sin_addr);
	sprintf(port, "%d", ntohs(n.address.sin_port));

	return getSocket( ip , port);
}

/***************************** R P C *******************************************/
node_t find_successor_rpc(int sock_fd, key_t k){
	header_t h ; 
	node_t successor;
	h.type = FIND_SUCCESSOR;
	if ( debug) printf("find_successor_rpc(): Find successor of %u \n", k); 
	send(sock_fd, &h, sizeof(h), 0);
	
	recv(sock_fd, &h, sizeof(h), 0);
	if ( h.type != OK ){
		printf("find_successor_rpc(): Unknown response from server.\n"); 
		successor = NULL_NODE;
	}
	else{
		if ( debug) printf("find_successor_rpc(): Received OK signal\n"); 
		
		send(sock_fd, &k, sizeof(k), 0);
		recv(sock_fd, &successor, sizeof(node_t), 0);
		
		if ( debug) printf("find_successor_rpc(): Received successor as %u\n", successor.key); 
	}
	return successor;
}

node_t get_predecessor_rpc(int sock_fd){
	header_t h ; 
	node_t predecessor;
	h.type = GET_PREDECESSOR; 
	if ( debug) printf("get_predecessor_rpc(): Get predecessor\n"); 
	send(sock_fd, &h, sizeof(h), 0);
	recv(sock_fd, &h, sizeof(h), 0);
	if ( h.type == OK){
		if ( debug)
			printf("get_predecessor_rpc(): Received OK signal\n"); 

		recv(sock_fd, &predecessor, sizeof(node_t), 0);
		if ( debug) printf("get_predecessor_rpc(): Received predecessor as %u\n", predecessor.key); 
	}else
	{
		printf("get_predecessor_rpc(): Unknown response from server %d\n",h.type);
		predecessor = NULL_NODE;
	}
	return predecessor;
}

void notify_rpc(int sock_fd, node_t k){
	header_t h ; 
	node_t predecessor;
	h.type = NOTIFY; 
	if ( debug) printf("notify_rpc(): Notify for %u\n", k.key); 	
	send(sock_fd, &h, sizeof(h), 0);
	
	recv(sock_fd, &h, sizeof(h), 0);
	if ( h.type == OK){
		if ( debug) printf("notify_rpc(): Received OK signal\n"); 
		send(sock_fd, &k, sizeof(k), 0);	
	}else
	{
		printf("notify_rpc(): Unknown response from server %d\n", h.type);
		return;
	}
}

void destroy_ring_rpc(){
	header_t h ; 
	extern nodeinfo_t nodeinfo ; 
	extern int server_sock; 
	if ( nodeinfo.successor.key != NULL_NODE.key){

		h.type = DESTROY_RING;
		int sock_fd = getSocketFromNode(nodeinfo.successor);
		send(sock_fd, &h, sizeof(h), 0);
		close(sock_fd);
	}
	close(server_sock);
	exit(0);
}

void finger_rpc(int sock_fd, node_t n){
	header_t h;
	h.type = FINGER;
	send(sock_fd, &h, sizeof(h), 0);
	recv(sock_fd, &h, sizeof(h), 0);
	if(h.type == OK){
		if(debug) printf("finger_rpc(): Got Ok.");
		send(sock_fd, &n, sizeof(n), 0);
	}
	else
		printf("finger_rpc(): Unknown response from successor.\n");
}

void dumpall_rpc(int sock_fd, node_t n){
	header_t h;
	h.type = DUMP_ALL;
	
	send(sock_fd, &h, sizeof(h), 0);
	recv(sock_fd, &h, sizeof(h), 0);
	if(h.type == OK){
		if(debug) printf("dumpall_rpc(): Got Ok.");
		send(sock_fd, &n, sizeof(n), 0);
	}
	else
		printf("dumpall_rpc(): Unknown response from successor.\n");
}

/***************************** R A N G E ***************************************/
int belongsToRange1(key_t key, interval_t interval){
	//	[start,end)
	if ( interval.start < interval.end ) {
		if (   interval.start <= key && key < interval.end ) 
			return 1;
	}else{

		if ( (interval.start <= key && key <= (pow(2, M) -1)  ) || ( 0 <= key && key < interval.end ) ) 
			return 1;
	}
	return 0;
}

int belongsToRange2(key_t key, interval_t interval){
	//	(start,end]
	if ( interval.start < interval.end ) {
		if (   interval.start < key && key <= interval.end ) 
			return 1;
	}else{

		if ( (interval.start < key && key <= (pow(2, M) -1)  ) || ( 0 <= key && key <= interval.end ) ) 
			return 1;
	}
	return 0;
}

int belongsToRange3(key_t key, interval_t interval){
	//	 (start,end)
	if ( interval.start < interval.end ) {
		if (   interval.start < key && key < interval.end ) 
			return 1;
	}else{

		if ( (interval.start < key && key <= (pow(2, M) -1)  ) || ( 0 <= key && key < interval.end ) ) 
			return 1;
	}
	return 0;
}

/***************************** P E R I O D I C *********************************/
void stabilize(){
	extern nodeinfo_t nodeinfo;
	node_t n = nodeinfo.successor;
	if(debug) printf("stabilize(): Setting up connection with %u\n", n.key);
	int sock_fd = getSocketFromNode(n);
	
	if(sock_fd == -1)
		return;
	
	if(debug) printf("stabilize(): Requesting successor %u for it's predecessor\n", nodeinfo.successor.key);
	node_t x = get_predecessor_rpc(sock_fd);
	if(debug) printf("stabilize(): Recieved predecessor of %u as %u \n", nodeinfo.successor.key, x.key);
	close(sock_fd);
	
	interval_t interval;
	interval.start = nodeinfo.self.key;
	interval.end = nodeinfo.successor.key;
	
	if(belongsToRange3(x.key , interval)){
		nodeinfo.successor = x;
		if(debug){
			printf("stabilize(): Successor of %u is updated to %u\n", nodeinfo.self.key, nodeinfo.successor.key);
			printf("stabilize(): Successor %s:%d \n", inet_ntoa(x.address.sin_addr), ntohs(x.address.sin_port));
		}	
	}	
	
	if(debug) printf("stabilize(): Setting up connection with %u\n", nodeinfo.successor.key);	
	sock_fd = getSocketFromNode(nodeinfo.successor);
	if(sock_fd == -1){	
		if(debug) printf("stabilize(): Connection with %u failed.\n", nodeinfo.successor.key);	
		return;
	}	
	if(debug) printf("stabilize(): Notifying successor %u for %u:%s:%d\n", nodeinfo.successor.key, nodeinfo.self.key, inet_ntoa(nodeinfo.self.address.sin_addr), ntohs(nodeinfo.self.address.sin_port));
	notify_rpc(sock_fd, nodeinfo.self);
	close(sock_fd);
}

int next = 0;
void fix_fingers(){
	extern nodeinfo_t nodeinfo;
	next = next + 1;	
	if(next > M)
		next = 1;
	if(debug) printf("fix_fingers(): Fixing %dth finger\n", next);	
	key_t k = (nodeinfo.self.key + (key_t)pow(2, next-1)) % (key_t)pow(2, M);
	nodeinfo.finger[next].successor = find_successor(k);
	if(debug) printf("fix_fingers(): Fixed %dth finger to %u\n", next, nodeinfo.finger[next].successor.key);	
}

void periodic(){
	extern nodeinfo_t nodeinfo;
	while(1){
		//sleep(1);
		usleep(1000000);
		stabilize();
		fix_fingers();
	}
}
/******************************* JOIN CREATE *************************************/

void join_ring(char *ip , char * port){
	extern int isConnectedToRing;
	isConnectedToRing = 1;

	extern nodeinfo_t nodeinfo;
	int i;
	
	for ( i = 1; i < 1+M; i++){
		nodeinfo.finger[i].successor = NULL_NODE; 
	}
	
	int sock_fd = getSocket(ip, port);
	if ( sock_fd == -1) 
	{
		printf("Failed to join the ring\n");
		isConnectedToRing = 0; 
		return; 
	}
	
	nodeinfo.predecessor = NULL_NODE;
	if(debug) printf("join_ring(): Requesting successor to %s:%s\n", ip, port);
	nodeinfo.successor = find_successor_rpc(sock_fd, nodeinfo.self.key);
	close(sock_fd);

/*
	pthread_t tid, tid1;
	pthread_create(&tid, NULL, startServer, NULL);
	pthread_create(&tid1, NULL, periodic, NULL);
	*/
	int tid1, tid2; 
	tid1 = create(startServer);
	run(tid1);
	tid2 = create(periodic);
	run(tid2);

	
	//print_finger_table(&nodeinfo);
}

void create_ring(nodeinfo_t * np){
	int i ; 
	extern int isConnectedToRing;

	isConnectedToRing = 1;
	
	for ( i = 1; i < 1+M; i++){
		np -> finger[i].successor = NULL_NODE; 
	}
	np -> successor = np -> self; 
	np -> predecessor= NULL_NODE; 
	//print_finger_table(np);
/*	
	pthread_t tid1, tid2;
	pthread_create(&tid1, NULL, periodic, NULL);
*/
	int tid; 
	tid = create ( periodic);
	run(tid);
	startServer();
}

/******************************** NON RPC ****************************************/

node_t closest_preceding_finger(key_t id){

	int i; 
	interval_t interval ; 
	extern nodeinfo_t nodeinfo;


	interval.start = nodeinfo.self.key; 
	interval.end = id; 
	for  ( i = M  ; i >= 1; i -- ) 
	{	if(nodeinfo.finger[i].successor.key != NULL_NODE.key)
			if ( belongsToRange3(nodeinfo.finger[i].successor.key , interval ) )
			{
				if(debug) printf("closest_preceding_finger(): closest_preceding_finger is %d \n", nodeinfo.finger[i].successor.key);
				return nodeinfo.finger[i].successor; 
			}
	}
	if ( debug ) printf("closest_preceding_finger(): closest_preceding_finger is %d \n", nodeinfo.self.key);
	return nodeinfo.self;
}


node_t find_successor(key_t id){
	extern nodeinfo_t nodeinfo;
	interval_t interval;
	interval.start = nodeinfo.self.key; 
	interval.end = nodeinfo.successor.key;
	
	if(belongsToRange2(id , interval)){
		if ( debug ) printf("find_successor(): successor of %u is %u \n", id, nodeinfo.successor.key);
		return nodeinfo.successor;
	}
	else{
		node_t n0;
		if ( debug ) printf("find_successor(): Finding closest_preceding_finger of %u\n", id);
		n0 = closest_preceding_finger(id);
		int sock_fd;
		
		if(debug) printf("find_successor(): Setting up connection with %u\n", n0.key);
		sock_fd = getSocketFromNode(n0);
		if(sock_fd == -1){
			if(debug) printf("find_successor(): Connection with %u failed\n", n0.key);
			return NULL_NODE;
		}
		if ( debug ) printf("find_successor(): Requesting %u to find successor of %u\n", n0.key, id);
		node_t ans = find_successor_rpc(sock_fd, id);
		close(sock_fd);
		if ( debug ) printf("find_successor(): successor of %u is %u \n", id, ans.key);
		return ans;
	}
}

void notify(node_t n1){
	extern nodeinfo_t nodeinfo;
	
	if(nodeinfo.predecessor.key == NULL_NODE.key){
		nodeinfo.predecessor = n1;
		if ( debug ) printf("notify(): Self predecessor is set to %u\n", n1.key);
		if ( debug ) printf("notify(): Redistribution Taking Place\n", n1.key);
		redist();
	}	
	else{
		interval_t interval;
		interval.start = nodeinfo.predecessor.key;
		interval.end = nodeinfo.self.key;
		if(belongsToRange3(n1.key , interval) ){
			nodeinfo.predecessor = n1;
			if ( debug ) printf("notify(): Self predecessor is set to %u\n", n1.key);
			if ( debug ) printf("join_ring(): Redistribution Taking Place\n", n1.key);
			//redist();
		}	
	}	
}

void store(char * key, char * value ) {
	key_t hashed_key = getHashID(key , strlen(key), M);
	extern nodeinfo_t nodeinfo;
	data_t * Where, d;
	if(hashed_key == nodeinfo.self.key)
		Where = nodeinfo.self_data;
	else
		Where = nodeinfo.other_data;	
	
	d.id = hashed_key;
	bzero(&(d.key), sizeof(d.key));
	bzero(&(d.value), sizeof(d.value));
	strcpy(d.key, key);
	strcpy(d.value, value);
	
	//printf("store():sem_wait\n");
	//sem_wait(&data_sem);
	insert(Where, d);
	//sem_post(&data_sem);
	//printf("store():sem_post\n");
}

void get(char * key) 
{
	header_t h;
	extern nodeinfo_t nodeinfo;
	data_t d;
	key_t hashed_key = getHashID(key , strlen(key), M);

	if ( hashed_key == nodeinfo.self.key ) {
		printf("get():sem_wait self\n");
		sem_wait(&data_sem);
		d = *(search(key)) ;
		printf("get():sem_post self\n");
		print_data(d);
		return;
	}
	node_t successor = find_successor(hashed_key);
	int sock = getSocketFromNode(successor);
	
	h.type = GET;
	send(sock, &h, sizeof(h), 0);
	recv(sock, &h, sizeof(h), 0);
	if ( h.type != OK )  {printf("get(): Unknown response from server for GET.\n") ; return -1;}
	
	send(sock, key,strlen(key) , 0);
	
	recv(sock, &h, sizeof(h), 0);
	if ( h.type != OK )  {printf("get(): Unknown response from server when key was sent.\n"); return -1;}
	
	recv(sock, &d, sizeof(d), 0);
	close(sock);
	print_data(d);
}


void put(char * key , char * value ) 
{
	header_t h;
	extern nodeinfo_t nodeinfo;

	key_t hashed_key = getHashID(key , strlen(key), M);
	printf("hashed_key=%u\n", hashed_key);

	if ( hashed_key == nodeinfo.self.key ) {
		printf("put():sem_wait self\n");
		sem_wait(&data_sem);
		store(key, value); 
		sem_post(&data_sem);
		printf("put():sem_post self\n");
		return;
	}
	
	node_t successor = find_successor(hashed_key);
	printf("hashed_key=%u, Successor=%u\n", hashed_key, successor.key);
	
	int sock = getSocketFromNode(successor);
	
	h.type = PUT;
	
	send(sock, &h, sizeof(h), 0);
	recv(sock, &h, sizeof(h), 0);
	if ( h.type != OK ){printf("put(): Unknown response from server for PUT.\n"); return -1;}
	
	send(sock, key,strlen(key) , 0);
	recv(sock, &h, sizeof(h), 0);
	if (h.type != OK){printf("put(): Unknown response from server when key was sent.\n"); return -1;}
	
	send(sock, value, strlen(value) , 0);
	recv(sock, &h, sizeof(h), 0);
	if ( h.type != OK )  {printf("put(): Unknown response from server when value was sent while PUT\n"); return -1;}
	
	printf("Put mapped to %u, %s: %d\n", hashed_key, inet_ntoa(successor.address.sin_addr) , htons(successor.address.sin_port));
	close(sock);
}

void redist(){
	extern nodeinfo_t nodeinfo;
	header_t h;
	interval_t interval;
	data_t d;
	
	int sock_fd = getSocketFromNode(nodeinfo.successor);
	if ( sock_fd == -1 ) 
	{
		printf("Invalid socket \n");
		exit(0);
	}
	
	interval.start = nodeinfo.predecessor.key;
	interval.end = nodeinfo.self.key;	
	//range2
	
	h.type = REDIST;
	send(sock_fd, &h, sizeof(h), 0);
	recv(sock_fd, &h, sizeof(h), 0);
	if ( h.type != OK )  {printf("redist(): Unknown response from server %u \n", h.type ) ; return;}	
	
	send(sock_fd, &interval, sizeof(interval), 0);
	
	recv(sock_fd, &d, sizeof(d), 0);
	while(d.id != NULL_DATA.id){
		printf("redist():sem_wait\n");
		sem_wait(&data_sem);
		store(d.key, d.value);
		sem_post(&data_sem);
		printf("redist():sem_post\n");
		//bzero(&d, sizeof(d));
		print_data(d);
		
		h.type = OK;
		send(sock_fd, &h, sizeof(h), 0);
		
		recv(sock_fd, &d, sizeof(d), 0);
	}
	close(sock_fd);
}

void finger(){
	extern nodeinfo_t nodeinfo;
	int sock_fd = getSocketFromNode(nodeinfo.successor);
	finger_rpc(sock_fd, nodeinfo.self);
	close(sock_fd);
}

void dumpall(){
	extern nodeinfo_t nodeinfo;
	int sock_fd = getSocketFromNode(nodeinfo.successor);
	dumpall_rpc(sock_fd, nodeinfo.self);
	close(sock_fd);
}

void SEND(int sock_fd){
	extern nodeinfo_t nodeinfo;
	header_t h;
	send(sock_fd, &(nodeinfo.self), sizeof(nodeinfo.self), 0);
	recv(sock_fd, &h, sizeof(h), 0);
	send(sock_fd, &(nodeinfo.successor), sizeof(nodeinfo.successor), 0);
	recv(sock_fd, &h, sizeof(h), 0);
	send(sock_fd, &(nodeinfo.predecessor), sizeof(nodeinfo.predecessor), 0);
	recv(sock_fd, &h, sizeof(h), 0);
	send(sock_fd, &(nodeinfo.finger), sizeof(nodeinfo.finger), 0);
	recv(sock_fd, &h, sizeof(h), 0);
	send(sock_fd, &(nodeinfo.self_data_index), sizeof(nodeinfo.self_data_index), 0);
	recv(sock_fd, &h, sizeof(h), 0);
	send(sock_fd, &(nodeinfo.other_data_index), sizeof(nodeinfo.other_data_index), 0);
	recv(sock_fd, &h, sizeof(h), 0);
	int i;
	for(i=0; i<1024; i++){
		send(sock_fd, &(nodeinfo.self_data[i]), sizeof(nodeinfo.self_data[i]), 0);
		recv(sock_fd, &h, sizeof(h), 0);
	}	
	for(i=0; i<1024; i++){
		send(sock_fd, &(nodeinfo.other_data[i]), sizeof(nodeinfo.other_data[i]), 0);
		recv(sock_fd, &h, sizeof(h), 0);
	}
}

void RECV(int sock_fd, nodeinfo_t *nodeinfo){
	header_t h;
	h.type = OK;
	
	recv(sock_fd, &(nodeinfo->self), sizeof(nodeinfo->self), 0);
	send(sock_fd, &h, sizeof(h), 0);
	recv(sock_fd, &(nodeinfo->successor), sizeof(nodeinfo->successor), 0);
	send(sock_fd, &h, sizeof(h), 0);
	recv(sock_fd, &(nodeinfo->predecessor), sizeof(nodeinfo->predecessor), 0);
	send(sock_fd, &h, sizeof(h), 0);
	recv(sock_fd, &(nodeinfo->finger), sizeof(nodeinfo->finger), 0);
	send(sock_fd, &h, sizeof(h), 0);
	recv(sock_fd, &(nodeinfo->self_data_index), sizeof(nodeinfo->self_data_index), 0);
	send(sock_fd, &h, sizeof(h), 0);
	recv(sock_fd, &(nodeinfo->other_data_index), sizeof(nodeinfo->other_data_index), 0);
	send(sock_fd, &h, sizeof(h), 0);
	
	int i;
	for(i=0; i<1024; i++){
		recv(sock_fd, &(nodeinfo->self_data[i]), sizeof(nodeinfo->self_data[i]), 0);
		send(sock_fd, &h, sizeof(h), 0);
	}
	
	for(i=0; i<1024; i++){
		recv(sock_fd, &(nodeinfo->other_data[i]), sizeof(nodeinfo->other_data[i]), 0);
		send(sock_fd, &h, sizeof(h), 0);
	}
}

void dump(char *ip, char* port){
	int sock_fd = getSocket(ip, port);
	header_t h;
	nodeinfo_t n;
	if(sock_fd == -1){
		printf("dump(): Unable to connect to %s:%s\n", ip, port);
		return;
	}
		
	h.type = DUMP;
	send(sock_fd, &h, sizeof(h), 0);
	recv(sock_fd, &h, sizeof(h), 0);
	if(h.type == OK){
		RECV(sock_fd, &n);
	}	
	else{
		printf("dump(): Unknown response from %s:%s\n", ip, port);
		close(sock_fd);
		return;
	}
	close(sock_fd);
	printf("__________________________________________\n");
	print_finger_table(&n);
	printf("__________________________________________\n");
}
/******************************** SERVER ****************************************/
void * serveRequest(void *arg)
{
	header_t h;
	extern nodeinfo_t nodeinfo;
	int client_sock = *((int *) arg);
	free(arg);
	recv(client_sock, &h, sizeof(h), 0);

	if ( h.type == PUT){
		char key[1024];
		char value[1024];
		bzero(key, 1024);
		bzero(value, 1024);
		// send ok
		h.type = OK;
		send(client_sock, &h, sizeof(h), 0);
		// receive key  and send ok
		recv(client_sock, key, 1024, 0);
		h.type = OK;
		send(client_sock, &h, sizeof(h), 0);
		// receive value and send ok
		recv(client_sock, value, 1024, 0);
		h.type = OK;
		send(client_sock, &h, sizeof(h), 0);
		// store in the hash table 
		printf("serveRequest():sem_wait PUT\n");
		sem_wait(&data_sem);
		store(key, value);
		sem_post(&data_sem);
		printf("serveRequest():sem_post PUT\n");
		printf("Put rpc received for key=%s, value=%s\n", key, value);

	}else if ( h.type == GET){
		if(debug) printf("Get request received\n" ) ; 
		h.type = OK;
		send(client_sock, &h, sizeof(h), 0);
		
		char key[1024];
		recv(client_sock, key, 1024, 0);
		h.type = OK;
		send(client_sock, &h, sizeof(h), 0);
		
		printf("serveRequest():sem_wait GET\n");
		sem_wait(&data_sem);
		data_t d = *(search(key));
		sem_post(&data_sem);
		printf("serveRequest():sem_post GET\n");
		send(client_sock, &d, sizeof(d), 0);
		
	}else if ( h.type == FIND_SUCCESSOR ){
		h.type = OK;
		send(client_sock, &h, sizeof(h), 0);
		
		key_t id ; 
		recv(client_sock, &id, sizeof(id), 0);
		if(debug) printf("Find successor request received for : %u\n", id ) ; 
		
		node_t n = find_successor(id);
		send(client_sock, &n, sizeof(n), 0);
		if ( debug ) printf("--Found successor. And sent it back to the requester %u \n", n.key);

	}else if ( h.type == GET_PREDECESSOR){
		if(debug) printf("Get predecessor request received\n" ) ; 
		h.type = OK;
		send(client_sock, &h, sizeof(h), 0);
		send(client_sock, &(nodeinfo.predecessor), sizeof(node_t), 0);
	}else if ( h.type == NOTIFY){
		//printf("NOtify request received\n" ) ; 
		h.type = OK;
		send(client_sock, &h, sizeof(h), 0);
		node_t n1;
		int i;
		recv(client_sock, &n1, sizeof(n1), 0);
		if(debug) printf("NOtify request received for %u:%s:%d\n", n1.key, inet_ntoa(n1.address.sin_addr), ntohs(n1.address.sin_port) ) ; 
		notify(n1);
	}else if ( h.type == REDIST){
		//printf("NOtify request received\n" ) ; 
		h.type = OK;
		send(client_sock, &h, sizeof(h), 0);
		interval_t interval;
		int i;
		
		recv(client_sock, &interval, sizeof(interval), 0);
		
		if(debug) printf("serveRequest():sem_wait REDIST\n");
		sem_wait(&data_sem);		
		for(i=0;i<1024 && i<nodeinfo.other_data_index; i++){
			if(belongsToRange2(nodeinfo.other_data[i].id , interval)){
				printf("Sending data_item:\n");
				print_data(nodeinfo.other_data[i]);
				send(client_sock, &(nodeinfo.other_data[i]), sizeof(nodeinfo.other_data[i]), 0);
				recv(client_sock, &h, sizeof(h), 0);
				if(h.type != OK){printf("Redistribution is failing.\n."); break;}
				delete_data(nodeinfo.other_data, i);
				i -= 1;
			}
		}
		sem_post(&data_sem);
		if(debug) printf("serveRequest():sem_post REDIST\n");
		
		send(client_sock, &NULL_DATA, sizeof(data_t), 0);
		
	}else if ( h.type == DESTROY_RING){
		printf("Ring Destroyed \n");
		destroy_ring_rpc();
	}else if ( h.type == FINGER){
		node_t n;
		char addr1[50], addr2[50], port1[10], port2[10];
		
		if(debug) printf("serveRequest(): Finger request received.\n");
		
		h.type = OK;
		send(client_sock, &h, sizeof(h), 0);
		recv(client_sock, &n, sizeof(n), 0);
		
		sprintf(addr1, "%s", inet_ntoa(nodeinfo.self.address.sin_addr));
		sprintf(port1, "%d", ntohs(nodeinfo.self.address.sin_port));
		sprintf(addr2, "%s", inet_ntoa(n.address.sin_addr));
		sprintf(port2, "%d", ntohs(n.address.sin_port));
		
		if(strcmp(addr1, addr2) == 0 && strcmp(port1, port2) == 0 ){
			printf("Ring Member: %s:%s\n", addr1, port1);
		}
		else{
			int sock_fd = getSocketFromNode(n);
			h.type = FINGER_RESPONSE;
			send(sock_fd, &h, sizeof(h), 0);
			recv(sock_fd, &h, sizeof(h), 0);
			if(h.type != OK){
				printf("FINGER_RESPONSE: Unknown response from %s:%s\n", addr2, port2);
			}
			else{
				send(sock_fd, &(nodeinfo.self), sizeof(node_t), 0);
			}
			close(sock_fd);
			
			sock_fd = getSocketFromNode(nodeinfo.successor);
			finger_rpc(sock_fd, n);
			close(sock_fd);
		}
		
	}else if ( h.type == FINGER_RESPONSE){
		node_t n;
		char addr[50], port[10];
		
		h.type = OK;
		send(client_sock, &h, sizeof(h), 0);
		recv(client_sock, &n, sizeof(n), 0);
		
		sprintf(addr, "%s", inet_ntoa(n.address.sin_addr));
		sprintf(port, "%d", ntohs(n.address.sin_port));
		
		printf("Ring Member: %s:%s\n", addr, port);
		
	}else if ( h.type == DUMP){
		if(debug) printf("serveRequest(): DUMP received.\n");
		h.type = OK;
		send(client_sock, &h, sizeof(h), 0);
		SEND(client_sock);
	}else if ( h.type == DUMP_ALL){
		node_t n;
		char addr1[50], addr2[50], port1[10], port2[10];
		
		if(debug) printf("serveRequest(): DUMP_ALL request received.\n");
		
		h.type = OK;
		send(client_sock, &h, sizeof(h), 0);
		recv(client_sock, &n, sizeof(n), 0);
		
		sprintf(addr1, "%s", inet_ntoa(nodeinfo.self.address.sin_addr));
		sprintf(port1, "%d", ntohs(nodeinfo.self.address.sin_port));
		sprintf(addr2, "%s", inet_ntoa(n.address.sin_addr));
		sprintf(port2, "%d", ntohs(n.address.sin_port));
		
		if(strcmp(addr1, addr2) == 0 && strcmp(port1, port2) == 0 ){
			printf("__________________________________________\n");
			print_finger_table(&nodeinfo);
			printf("__________________________________________\n");
		}
		else{
			int sock_fd = getSocketFromNode(n);
			h.type = DUMP_RESPONSE;
			send(sock_fd, &h, sizeof(h), 0);
			recv(sock_fd, &h, sizeof(h), 0);
			if(h.type != OK){
				printf("DUMP_RESPONSE: Unknown response from %s:%s\n", addr2, port2);
			}
			else{
				SEND(sock_fd);
			}
			
			recv(sock_fd, &h, sizeof(h), 0);
			if(h.type != OK){
				printf("DUMP_RESPONSE: Unknown response from %s:%s\n", addr2, port2);
			}
			close(sock_fd);
			
			sock_fd = getSocketFromNode(nodeinfo.successor);
			dumpall_rpc(sock_fd, n);
			close(sock_fd);
		}
		
	}else if ( h.type == DUMP_RESPONSE){
		nodeinfo_t n;
		char addr[50], port[10];
		
		h.type = OK;
		send(client_sock, &h, sizeof(h), 0);
		RECV(client_sock, &n);
		printf("__________________________________________\n");
		print_finger_table(&n);
		printf("__________________________________________\n");
		h.type = OK;
		send(client_sock, &h, sizeof(h), 0);
		
	}else{
		printf("serveRequest(): Unknown request\n" ) ; 
	}

	close(client_sock);
}

void startServer(){
	extern nodeinfo_t nodeinfo ; 
	extern int server_sock;
	int  client_sock, i = 0, addr_size, k;
	struct sockaddr_in addr, client;

	if ( ( server_sock = socket(AF_INET, SOCK_STREAM, 0) ) < 0 ){
		perror("socket() : Failed to create socket. ");
		exit(0);
	}


	bzero(&addr,sizeof(addr));
	addr = nodeinfo.self.address;

	if ( debug )  printf("Trying to open socket on %s : %d \n", inet_ntoa(addr.sin_addr ) , ntohs(addr.sin_port));
	

	if( bind(server_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0 ){
		perror("bind() : Failed to bind. ");
		exit(0);
	}


	if( listen(server_sock, 100) < 0 ) {
		perror("Failed to listen on socket. ");
		exit(0);
	}
	while(1){
		client_sock = accept(server_sock, (struct sockaddr *)&client, &addr_size);

		int * sockp = malloc(sizeof(int));
		if ( sockp == NULL) {printf("malloc failed \n"); exit(0);}

		*sockp = client_sock;
		/*
		pthread_t tid; 
		pthread_create(&tid, NULL, serveRequest, sockp);
		*/
		int tid ; 
		tid = createWithArgs(serveRequest, sockp);
		run(tid);
	}
}
#endif

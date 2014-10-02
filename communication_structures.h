#ifndef COMMUNICATION_STRUCTURES_H 
#define COMMUNICATION_STRUCTURES_H 
#define OK 			 1
#define JOIN 			 2
#define PUT 			 3
#define GET 			 4
#define FIND_SUCCESSOR 		 5  
#define GET_PREDECESSOR 	 6 
#define NOTIFY 			 7  
#define REDIST			 8
#define DESTROY_RING 		 9
#define FINGER 		 	10
#define FINGER_RESPONSE	 	11

typedef struct header_t{
	int type;
}header_t;

#endif

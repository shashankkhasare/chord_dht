typedef struct node {
	char * skey ; 
	char * value ; 
	key_t ikey ; 
	node * next ;  // used for chaining 
}node;

typedef sturct hashtable {

	node *table_by_skey[1024];
	node *table_by_ikey[1024];
}hashtable; 


key_t myhash(char * stuff){
	int len = strlen(stuff);

	key_t hv = getHashID(stuff , len , 20);
	hv = hv & 0x3ff;

	return hv;


}


void store_in_ht(hashtable * ht, char * skey , char * value) {
// stores both tables, by key and by key_t 
		
		int hval;
		// create the node
		node * n = malloc(sizeof(node));
		n -> skey = malloc(1024);
		n -> value = malloc(1024);
		bzero(n -> skey , 1024);
		bzero(n -> value, 1024);

		strcpy( n->skey , skey ) ;
		strcpy( n->value, value ) ;
		n -> ikey = getHashID(skey, strlen(skey), M);
		n -> next = NULL;


		// store by skey 
		hval = myhash(skey);
		if ( ht -> table_by_skey[hval] == NULL){
			ht -> table_by_skey[hval] = n;
		}else{
			// need to add to the chain due to hash collision
			node * temp; 
			temp = ht -> table_by_skey[hval]; 
			while ( temp -> next )
				temp = temp -> next ; 
			temp -> next = n; 
		}

		// store by ikey 


}

node * get_from_ht_by_key ( hashtable * ht, char * k){



}

node * get_from_ht_by_key_t(hashtable * ht, key_t k){


}

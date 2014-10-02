CC = gcc
EXEC_NAME1 = dht
EXEC_NAME2 = mysha
BASIC_C1 = dht.c 
BASIC_C2 = sha_prefix.c

all		: $(EXEC_NAME1) $(EXEC_NAME2) 

$(EXEC_NAME1)	: $(BASIC_C1) dht.h communication_structures.h history.h mythread.h queue.h thread_struct.h
	$(CC) -w $(BASIC_C1) -lm -pthread -o $(EXEC_NAME1) -g

$(EXEC_NAME2)	: $(BASIC_C2)
	$(CC) $(BASIC_C2) -lcrypto -o $(EXEC_NAME2) -g
	
clean:
	rm $(EXEC_NAME1) $(EXEC_NAME2)	

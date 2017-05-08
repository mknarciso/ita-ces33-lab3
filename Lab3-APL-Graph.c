#include <windows.h>
#include <stdio.h>

#define TRUE 1
#define FALSE 0
#define N 10
#define debugtxt(FORMAT) printf("TID %d: " #FORMAT "\n",GetCurrentThreadId())
#define debug(FORMAT, ARGS...) printf("TID %d: " #FORMAT "\n",GetCurrentThreadId(),ARGS)

// Funcoes e variaveis do buffer
int start;
int end;
int buffer[N];
int count;
int last_produced_item;
int last_consumed_item;

void insert_item(int item) {
	debug("Inserting item %d",item);
	buffer[end]=item;
	end=(end+1)%N;
	debugtxt("Item inserted!");
}
int remove_item() {
	debugtxt("Removing item ...");
	int item = buffer[start];
	start=(start+1)%N;
	debug("Item %d removed!",item);
	return item;
}

// Funcoes e variaveis das threads
HANDLE handleThread[2];
DWORD threadId[2];
const int producer = 0;
const int consumer = 1;
HANDLE full;
HANDLE empty;
HANDLE mutex;

// Truque para sabermos qual o semaforo foi chamado e poder imprimi-lo
#define up(SEM) _up(SEM,#SEM)
#define down(SEM) _down(SEM,#SEM)

void _up(HANDLE sem, const char * name) {
	debug("Up %s ...",name);
	ReleaseSemaphore(sem,1,NULL);
	debug("Up %s complete!",name);
}
void _down(HANDLE sem, const char * name) {
	debug("Down %s ...",name);
	WaitForSingleObject(sem,INFINITE);
	debug("Down %s complete!",name);
}

// Produtor e consumidor ...
int produce_item() {
	debugtxt("Producing item ...");
	last_produced_item++;
	debug("Produced item %d",last_produced_item);
	return last_produced_item;
}
void consume_item(int item) {
	debugtxt("Consuming item ...");
	last_consumed_item = item;
	debug("Consumed item %d",item);
}

DWORD WINAPI producerFunc( LPVOID lpParam ) {
	debugtxt("Starting producer");
	int item;
	while(TRUE) {
		item=produce_item();
		down(empty);
		down(mutex);
		insert_item(item);
		up(mutex);
		up(full);
	}
	debugtxt("Ending producer");
	return 0;
}

DWORD WINAPI consumerFunc( LPVOID lpParam ) {
	debugtxt("Starting consumer");
	int item;
	while(TRUE) {
		down(full);
		down(mutex);
		item = remove_item();
		up(mutex);
		up(empty);
		consume_item(item);
	}
	debugtxt("Ending consumer");
	return 0;
}

int main() {
	int i;

	last_produced_item = 0;
	start = 0;
	end = 0;
	// Criando semaforos ...
	full = CreateSemaphore( 
			NULL,           // default security attributes
			0,			// initial count
			N,  			// maximum count
			NULL);
	empty = CreateSemaphore( 
			NULL,           // default security attributes
			N,			// initial count
			N,  			// maximum count
			NULL);
	mutex = CreateSemaphore( 
			NULL,           // default security attributes
			1,			// initial count
			1,  			// maximum count
			NULL);
	
	LPTHREAD_START_ROUTINE threadFunc[2] = { producerFunc, consumerFunc };
	for(i=0;i<2;i++) {
		handleThread[i] = CreateThread( 
            NULL,               // default security attributes
            0,                  // use default stack size  
            threadFunc[i],      // thread function pointer
            &i,     			// argument to thread function 
            0,                  // use default creation flags 
            &threadId[i]);   // returns the thread identifier 
	}
	
	WaitForMultipleObjects(2, handleThread, TRUE, INFINITE);
	
	for(i=0;i<2;i++) {
		CloseHandle(handleThread[i]);
	}
	CloseHandle(empty);
	CloseHandle(full);
	CloseHandle(mutex);
	
}

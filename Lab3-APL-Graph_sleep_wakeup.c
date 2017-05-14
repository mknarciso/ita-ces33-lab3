#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0
#define N 10
#define debugtxt(FORMAT) printf(" TID %lu: " #FORMAT "\n",pthread_self())
#define debug(FORMAT, ARGS...) printf("TID %lu: " #FORMAT "\n",pthread_self(),ARGS)
int line_counter;

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

// Funcoes e variaveis das threads
long unsigned int threadId[2];
pthread_t handleThread[2];

const int producer = 0;
const int consumer = 1;
pthread_cond_t  condition[2];
pthread_mutex_t mutex;

void sleepThread(int x) {
	debugtxt("Sleeping ...");
	pthread_cond_wait(&condition[x],&mutex);
	debugtxt("Waked up!");
}
void wakeup(const int x) {
	debug("Waking up %d ...",threadId[x]);
	pthread_cond_signal(&condition[x]);
	debug("Waking up signal sent to %d!",threadId[x]);
}

void producerFunc() {
	threadId[producer] = pthread_self();
	debugtxt("Starting producer");
	int item;
	while(TRUE) {
		item=produce_item();
		if(count == N) sleepThread(producer);
		insert_item(item);
		count = count + 1;
		if(count == 1) wakeup(consumer);
	}
	debugtxt("Ending producer");
}

void consumerFunc() {
	threadId[consumer] = pthread_self();
	debugtxt("Starting consumer");
	int item;
	while(TRUE) {
		if(count == 0) {
			sleep(30); //Descomentar:forca disputa
			sleepThread(consumer);
		}
		item = remove_item();
		count = count -1;
		if(count == N-1) wakeup(producer);
		consume_item(item);
	}
	debugtxt("Ending consumer");
}

int main() {
   last_produced_item = 0;
   start = 0;
   end = 0;
   void* threadFunc[2];
   threadFunc[0] = producerFunc;
   threadFunc[1] = consumerFunc;
   int i;
   for(i=0;i<2;i++) {pthread_create( &handleThread[i], NULL, threadFunc[i], NULL );}
   for(i=0;i<2;i++) {pthread_join( handleThread[i], NULL);}
	exit(EXIT_SUCCESS);
}
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

#define TRUE 1
#define FALSE 0
#define N 10
#define N_VERT 1000
#define debugtxt(FORMAT) printf("TID %ld: " #FORMAT "\n",pthread_self())
#define debug(FORMAT, ARGS...) printf("TID %ld: " #FORMAT "\n",pthread_self(),ARGS)
//#define debugtxt(FORMAT) printf(" TID xxx: " #FORMAT "\n")
//#define debug(FORMAT, ARGS...) printf("TID xxx: " #FORMAT "\n",ARGS)

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
//HANDLE handleThread[2];
//DWORD threadId[2];
//const int producer = 0;
//const int consumer = 1;
//HANDLE full;
//HANDLE empty;
//HANDLE mutex;

pthread_t threadId[2];
const int producer = 0;
const int consumer = 1;
sem_t mutex;
sem_t full;
sem_t empty;

// Truque para sabermos qual o semaforo foi chamado e poder imprimi-lo
#define up(SEM) _up(SEM,#SEM)
#define down(SEM) _down(SEM,#SEM)

void _up(sem_t *sem, const char * name) {
	debug("Up %s ...",name+1);
	sem_post(sem);
	debug("Up %s complete!",name+1);
}
void _down(sem_t *sem, const char * name) {
	debug("Down %s ...",name+1);
	sem_wait(sem);
	debug("Down %s complete!",name+1);
}

// Produtor e consumidor ...
int produce_item(int adj_matrix[N_VERT][N_VERT]) {
	debugtxt("Producing item ...");
	float z=1.2;
	float q;
	int j,k;
	q=z/N_VERT;
	debug("Produzindo grafo (n=%d , q=%g) ...",N_VERT,q);
	//printf("j = %d, k = %d\n"j,k);
	for (j=0;j<N_VERT;j++){
		for (k=0;k<=j;k++){
			//printf("j = %d, k = %d\n",j,k);
			if(k==j){
				//printf("j = %d, k = %d\n",j,k);
    			adj_matrix[j][k]=0;
    			//printf("j = %d, k = %d\n",j,k);
    			adj_matrix[k][j]=0;
   			} else {
	   			if(((float)rand())/RAND_MAX<q){
	    			adj_matrix[j][k]=1;
	    			adj_matrix[k][j]=1;
	    		} else {
	    			adj_matrix[j][k]=0;
	    			adj_matrix[k][j]=0;
	    		}	
   			}
		}
	}
	last_produced_item++;
	debug("Produced item %d",last_produced_item);
	return last_produced_item;
}
void consume_item(int item) {
	debugtxt("Consuming item ...");
	last_consumed_item = item;
	debug("Consumed item %d",item);
}

void* producerFunc() {
	debugtxt("Starting producer");
	int item;
	int **mat;
	mat = malloc(N_VERT*sizeof(int));
	int i,j;
	for (i = 0; i < N_VERT; ++i)
	{
		mat[i] = malloc(N_VERT*sizeof(int));
	}
	while(TRUE) {
		item=produce_item(mat);
		down(&empty);
		//sem_wait(&empty);
		down(&mutex);
		//sem_wait(&mutex);
		insert_item(item);
		up(&mutex);
		//sem_post(&mutex);
		up(&full);
		//sem_post(&full);
	}
	debugtxt("Ending producer");
	return 0;
}

void* consumerFunc() {
	debugtxt("Starting consumer");
	int item;
	while(TRUE) {
		down(&full);
		//sem_wait(&full);
		down(&mutex);
		//sem_wait(&mutex);
		item = remove_item();
		up(&mutex);
		//sem_post(&mutex);
		up(&empty);
		//sem_post(&empty);
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
	int err;
	void* status;
	// Criando semaforos ...
	/*
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
	*/
	sem_init(&mutex, 0, 1);
	sem_init(&full, 0, 0);
	sem_init(&empty,0, N);
	
	for(i=0;i<2;i++) {
   		/*
		handleThread[i] = CreateThread( 
            NULL,               // default security attributes
            0,                  // use default stack size  
            threadFunc[i],      // thread function pointer
            &i,     			// argument to thread function 
            0,                  // use default creation flags 
            &threadId[i]);   // returns the thread identifier 
        */
   		if(i == 0)
   			err = pthread_create(&threadId[i], NULL, &producerFunc, NULL);
   		else
			err = pthread_create(&threadId[i], NULL, &consumerFunc, NULL);   			
   }
    pthread_join(threadId[0], &status);
    pthread_join(threadId[1], &status);
    sem_destroy(&mutex);
    sem_destroy(&full);
    sem_destroy(&empty);
    pthread_exit(NULL);
	
}
#include <stdio.h> 
#include <stdlib.h> 
#include <pthread.h> 
#include <unistd.h> 
#include <semaphore.h>
#define TRUE 1
#define FALSE 0
#define N 10
#define N_VERT 1000
#define debugtxt(FORMAT) printf("TID %lu: " #FORMAT "\n",pthread_self())
#define debug(FORMAT, ARGS...) printf("TID %lu: " #FORMAT "\n",pthread_self(),ARGS)

// Funcoes e variaveis do buffer 
int start;
int end;
int buffer[N];
int ***array_buffer;
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
int produce_item(int adj_matrix[N_VERT][N_VERT]) {
	float z=1.2;
	float q;
	q=z/N_VERT;
	debug("Produzindo grafo (n=%d , q=%g) ...",N_VERT,q);
	last_produced_item++;
	for (int j=0;j<N_VERT;j++){
		for (int k=0;k<j;k++){
   			if(((float)rand())/RAND_MAX<q){
    			adj_matrix[j][k]=1;
    			adj_matrix[k][j]=1;
    		} else {
    			adj_matrix[j][k]=0;
    			adj_matrix[k][j]=0;
    		}	
		}
		adj_matrix[j][j]=0;
		adj_matrix[j][j]=0;
	}
	debug("Produzido grafo %d <<<---------------",last_produced_item); 
	return last_produced_item;
}

void consume_item(int item) { 
	debugtxt("Consuming item ..."); 
	last_consumed_item = item; 
	debug("Consumed item %d",item);
}
// Funcoes e variaveis das threads 
pthread_t handleThread[2];
long unsigned int threadId[2]; 
const int producer = 0;
const int consumer = 1; 
sem_t full;
sem_t empty;
sem_t mutex;

void up(sem_t* sem, const char * name) { 
	debug("Up %s ...",name);
	sem_post(sem);
	debug("Up %s complete!",name); 
}

void down(sem_t* sem, const char * name) { 
	debug("Down %s ...",name); 
	sem_wait(sem);
	debug("Down %s complete!",name);
}

void producerFunc() { 
	debugtxt("Starting producer"); 
	int item=0;

	int **adj_matrix = (int **)malloc(sizeof(int)*N_VERT);
	for (int h=0;h<N_VERT;h++) adj_matrix[h] = (int *)malloc(sizeof(int)*N_VERT);

	while(TRUE) {
		item=produce_item(adj_matrix); 
		down(&empty,"empty"); 
		down(&mutex,"mutex"); 
		insert_item(item); 
		up(&mutex,"mutex"); 
		up(&full,"full");
	}
	debugtxt("Ending producer"); 
	//return 0;
}

void consumerFunc() { 
	debugtxt("Starting consumer"); 
	int item=0;
	while(TRUE) {
		down(&full,"full"); 
		down(&mutex,"mutex"); 
		item = remove_item(); 
		up(&mutex,"mutex"); 
		up(&empty,"empty"); 
		consume_item(item);
	}
	debugtxt("Ending consumer"); 
	//return 0;
}

int main() { 
	int i;
	last_produced_item = 0;
	start = 0;
	end = 0;
	// Criando semaforos ...
	sem_init(&mutex, 0, 1);
	sem_init(&empty, 0, N);
	sem_init(&full, 0, 0);
	void* threadFunc[2];
	threadFunc[0] = producerFunc;
	threadFunc[1] = consumerFunc;
	//Alocando memória para o buffer
	int ***array_buffer = (int***)malloc(N*sizeof(int**));
	for (int i = 0; i < N; i++) {
	    array_buffer[i] = (int**)malloc(N_VERT*sizeof(int*));
	    for (int j = 0; j < N_VERT; j++) {
	    	array_buffer[i][j] = (int*)malloc(N_VERT*sizeof(int));
		}
	}
	for(i=0;i<2;i++) {
		pthread_create( &handleThread[i], NULL, threadFunc[i], NULL );
	} 
	for(i=0;i<2;i++) {
		pthread_join( handleThread[i], NULL);
	}
	sem_destroy(&empty); 
	sem_destroy(&full); 
	sem_destroy(&mutex); 
	exit(EXIT_SUCCESS);
}
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
int ** removed_item;
//Buffer para guardar os grafos ja construidos
int ** graph_buffer[N];
int degrees[N_VERT];
double average_degree = 0;
double clustering_coefficient=0;

void insert_item(int item) {
	debug("Inserting item %d",item);
	buffer[end]=item;
	printf("end = %d\n", end);
	end=(end+1)%N;
	debugtxt("Item inserted!");
}
int remove_item() {
	debugtxt("Removing item ...");
	int item = buffer[start];
	int i,j;
	//Copia o primeiro elemento
	for(i=0;i<N_VERT;i++){
		for (j = 0; j < N_VERT; j++){
			//printf("i=%d, j=%d\n",i,j);
			//printf("graph_buffer[start][%d][%d] = %d\n",i,j,graph_buffer[start][i][j]);
			removed_item[i][j] = graph_buffer[start][i][j];
		}
	}
	
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
sem_t mutex_AD;
sem_t mutex_CC;

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
int produce_item(int **adj_matrix) {
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
	debugtxt("Calculando o grau medio");
	int i,j;
	int sum=0;
	int auxsum;
	double temp;
	for(i=0; i < N_VERT;i++){
		auxsum=0;
		for(j=0;j<N_VERT;j++){
			if(removed_item[i][j]==1)
				auxsum++;
		}
		sum = sum + auxsum;
	}
	temp = (double) sum/N_VERT;
	//Na hora de modificar a variável global, deve-se travá-la
	down(&mutex_AD);
	average_degree = (average_degree*last_consumed_item + temp)/(last_consumed_item+1);
	printf("O grau medio eh: %g\n", average_degree);
	up(&mutex_AD);

	debugtxt("Calculando o clustering coefficient");
	auxsum=0;
	int viz[N_VERT];
	int vizLen=0;
	double CCSum = 0;
	int m,n;
	for(i=0; i < N_VERT;i++){
		auxsum=0;
		vizLen = 0;
		for(j=0; j<N_VERT; j++){
			if(removed_item[i][j] == 1){
				viz[vizLen] = j;
				//printf("j=%d\n",j);
				vizLen++;
			}			
		}
		for(m=0; m < vizLen-1; m++){
			for(n = m+1; n<vizLen;n++){
				if(removed_item[viz[m]][viz[n]]==1){
					//printf("auxsum = %d\n", auxsum);
					auxsum++;
				}
			}
		}
		if(vizLen != 0 && vizLen != 1)
			CCSum = (double) auxsum/(vizLen*(vizLen-1)) + CCSum;

		//printf("CCSum = %g\n", CCSum);
		//printf("vizLen = %d\n", vizLen);
		//printf("auxsum = %d\n", auxsum);
	}
	temp = (double) CCSum/N_VERT;
	down(&mutex_CC);
	clustering_coefficient = (clustering_coefficient*last_consumed_item + temp)/(last_consumed_item+1);
	printf("O coeficiente de agrupamento eh: %g\n", clustering_coefficient);
	up(&mutex_CC);
	last_consumed_item = item;

	debug("Consumed item %d",item);
}

void* producerFunc() {
	debugtxt("Starting producer");
	int item;
	int **mat;
	while(TRUE) {
		down(&empty);
		down(&mutex);
		//A produção do grafo já modifica o buffer
		item=produce_item(graph_buffer[end]);
		//int i,j;
		//for(i=0;i<N_VERT;i++){
		//	for(j=0;j<N_VERT;j++){
		//		printf("graph_buffer[end][%d][%d] = %d\n",i,j, graph_buffer[end][i][j]);
		//	}
		//}
		insert_item(item);
		up(&mutex);
		up(&full);
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
	last_produced_item = 0;
	start = 0;
	end = 0;
	int err;
	void* status;
	int i,j;
	for(j=0; j<N; j++){
		graph_buffer[j] = (int **) malloc(N_VERT*sizeof(int *));
		for (i = 0; i < N_VERT; i++)
		{
			graph_buffer[j][i] = (int *) malloc(N_VERT*sizeof(int));
		}
	}

	removed_item = 	(int **) malloc(N_VERT*sizeof(int *));
	for (i = 0; i < N_VERT; i++){
		removed_item[i] = (int *) malloc(N_VERT*sizeof(int));
	}
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
	sem_init(&mutex_AD, 0, 1);
	sem_init(&mutex_CC, 0, 1);
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
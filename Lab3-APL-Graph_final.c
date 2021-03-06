#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

#define TRUE 1
#define FALSE 0
#define N 5
#define N_VERT 2000
#define N_CONS 3
#define N_PROD 1
#define debugtxt(FORMAT) printf("TID %ld: " #FORMAT "\n",pthread_self())
#define debug(FORMAT, ARGS...) printf("TID %ld: " #FORMAT "\n",pthread_self(),ARGS)

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
int ** consumer_buffer[N_CONS];

//Define a posicao da thread consumidora no array
int consumer_id = 0;
int degrees[N_VERT];
double average_degree = 0;
double clustering_coefficient=0;
int cc_count, ad_count;

// Atualiza o buffer dos índices
void insert_item(int item) {
	debug("Inserindo grafo no buffer #%d",item);
	buffer[end]=item;
	printf("end = %d\n", end);
	end=(end+1)%N;
	debugtxt("Grafo inserido!");
}

int remove_item(int id) {
	debugtxt("Removendo grafo do buffer para consumo.");
	int item = buffer[start];
	int i,j;
	//Copia a matrix a ser consumida
	for(i=0;i<N_VERT;i++){
		for (j = 0; j < N_VERT; j++){
			consumer_buffer[id][i][j] = graph_buffer[start][i][j];
		}
	}
	start=(start+1)%N;
	debug("Removido grafo #%d",item);
	return item;
}

// Funcoes e variaveis das threads
pthread_t threadId[N_CONS+N_PROD];
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
	float z=1.2; // coeficiente de conectividade
	float q;
	int j,k;
	q=z/N_VERT;
	debug("Produzindo grafo (n=%d , q=%g) ...",N_VERT,q);
	for (j=0;j<N_VERT;j++){
		for (k=0;k<=j;k++){
			if(k==j){
    			adj_matrix[j][k]=0;
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
	debug("Grafo completo #%d",last_produced_item);
	return last_produced_item;
}
void consume_item(int item, int id) {
	debugtxt("Calculando o grau medio...");
	int i,j;
	int sum=0;
	int auxsum;
	double temp;
	for(i=0; i < N_VERT;i++){
		auxsum=0;
		for(j=0;j<N_VERT;j++){
			if(consumer_buffer[id][i][j]==1)
				auxsum++;
		}
		sum = sum + auxsum;
	}
	temp = (double) sum/N_VERT;
	//Na hora de modificar as variáveis AD, deve-se travá-las
	down(&mutex_AD);
	average_degree = (average_degree*ad_count + temp)/(ad_count+1);
	ad_count++;
	printf("O grau medio eh: %g - %d iterações\n", average_degree,ad_count);
	up(&mutex_AD);

	debugtxt("Calculando o clustering coefficient...");
	auxsum=0;
	int viz[N_VERT];
	int vizLen=0;
	double CCSum = 0;
	int m,n;
	for(i=0; i < N_VERT;i++){
		auxsum=0;
		vizLen = 0;
		for(j=0; j<N_VERT; j++){
			if(consumer_buffer[id][i][j] == 1){
				viz[vizLen] = j;
				vizLen++;
			}			
		}
		for(m=0; m < vizLen-1; m++){
			for(n = m+1; n<vizLen;n++){
				if(consumer_buffer[id][viz[m]][viz[n]]==1){
					auxsum++;
				}
			}
		}
		if(vizLen != 0 && vizLen != 1)
			CCSum = (double) auxsum/(vizLen*(vizLen-1)) + CCSum;

	}
	temp = (double) CCSum/N_VERT;
	//Na hora de modificar as variáveis CC, deve-se travá-las
	down(&mutex_CC);
	clustering_coefficient = (clustering_coefficient*cc_count + temp)/(cc_count+1);
	cc_count++;
	printf("O coeficiente de agrupamento eh: %g - %d iterações\n", clustering_coefficient,cc_count);
	up(&mutex_CC);
	last_consumed_item = item;
	debug("Consumido o grafo #%d",item);
}

void* producerFunc() {
	debugtxt("Iniciando produtor de grafos");
	int item;
	int **mat;
	while(TRUE) {
		down(&empty);
		down(&mutex);
		//A produção do grafo é feita diretamente no buffer
		item=produce_item(graph_buffer[end]);
		insert_item(item);
		up(&mutex);
		up(&full);
	}
	debugtxt("Finalizando produtor");
	return 0;
}

void* consumerFunc(int id) {
	debugtxt("Iniciando consumidor de grafos");
	int item;
	int cons_ID=id-1;
	while(TRUE) {
		down(&full);
		down(&mutex);
		item = remove_item(cons_ID);
		up(&mutex);
		up(&empty);
		consume_item(item, cons_ID);
	}
	debugtxt("Finalizando consumidor");
	return 0;
}

int main() {

	last_produced_item = 0;
	start = 0;
	end = 0;
	int err;
	void* status;
	int i,j;

	//Aloca memória para o buffer do produtor
	for(j=0; j<N; j++){
		graph_buffer[j] = (int **) malloc(N_VERT*sizeof(int *));
		for (i = 0; i < N_VERT; i++)
		{
			graph_buffer[j][i] = (int *) malloc(N_VERT*sizeof(int));
		}
	}

	//Aloca memória para o buffer do consumidor
	for(j = 0; j < N_CONS; j++){
		consumer_buffer[j] = 	(int **) malloc(N_VERT*sizeof(int *));
		for (i = 0; i < N_VERT; i++){
			consumer_buffer[j][i] = (int *) malloc(N_VERT*sizeof(int));
		}
	}

	//Inicializa semaforos
	sem_init(&mutex, 0, 1);
	sem_init(&mutex_AD, 0, 1);
	sem_init(&mutex_CC, 0, 1);
	sem_init(&full, 0, 0);
	sem_init(&empty,0, N);
	
	//Cria as threads 0 Produtora e 1-3 Consumidoras
	for(i=0;i<N_PROD+N_CONS;i++) {
   		if(i == 0)
   			err = pthread_create(&threadId[i], NULL, &producerFunc, NULL);
   		else
			err = pthread_create(&threadId[i], NULL, &consumerFunc, i);   			
   	}
   	//Destroi threads e semaforos
   	for(i=0;i<N_PROD+N_CONS;i++) {
	    pthread_join(threadId[i], &status);
	}
    sem_destroy(&mutex);
    sem_destroy(&mutex_AD);
    sem_destroy(&mutex_CC);
    sem_destroy(&full);
    sem_destroy(&empty);
    pthread_exit(NULL);
	
}
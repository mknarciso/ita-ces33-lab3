#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0
#define N 1
#define N_VERT 2000
#define N_CONS 1
#define N_PROD 1
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
	//down(&mutex_AD);
	average_degree = (average_degree*ad_count + temp)/(ad_count+1);
	ad_count++;
	debug("O grau medio eh: %g - %d iterações\n", average_degree,ad_count);
	//up(&mutex_AD);

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
	//down(&mutex_CC);
	clustering_coefficient = (clustering_coefficient*cc_count + temp)/(cc_count+1);
	cc_count++;
	debug("O coeficiente de agrupamento eh: %g - %d iterações\n", clustering_coefficient,cc_count);
	//up(&mutex_CC);
	last_consumed_item = item;
	debug("Consumido o grafo #%d",item);
}

// Funcoes e variaveis das threads
long unsigned int threadId[N_CONS+N_PROD];
pthread_t handleThread[N_CONS+N_PROD];
const int producer = 0;
const int consumer = 1;
pthread_cond_t  condition[N_CONS+N_PROD];
pthread_mutex_t mutex;

void sleepThread(int x) {
	debugtxt("Dormi ...");
	pthread_cond_wait(&condition[x],&mutex);
	debugtxt("Acordei!!!");
}
void wakeup(const int x) {
	debug("Acordando: %d ...",threadId[x]);
	pthread_cond_signal(&condition[x]);
	debug("Despertando a Thread: %d!",threadId[x]);
}

void producerFunc() {
	threadId[producer] = pthread_self();
	debugtxt("Iniciando produtor de grafos");
	int item;
	int **mat;
	while(TRUE) {
		if(count == N) sleepThread(producer);
		//A produção do grafo é feita diretamente no buffer
		item=produce_item(graph_buffer[end]);
		insert_item(item);
		count = count + 1;
		if(count == 1) wakeup(consumer);
	}
	debugtxt("Finalizando produtor");
}

void consumerFunc() {
	threadId[consumer] = pthread_self();
	debugtxt("Iniciando consumidor de grafos");
	int item;
	int cons_ID=0;
	while(TRUE) {
		if(count == 0) {
			sleep(30/1000); //Descomentar:forca disputa
			sleepThread(consumer);
		}
		item = remove_item(cons_ID);
		count = count -1;
		if(count == N-1) wakeup(producer);
		consume_item(item, cons_ID);
	}
	debugtxt("Finalizando consumidor");
}

int main() {
   last_produced_item = 0;
   start = 0;
   end = 0;
   void* threadFunc[2];
   threadFunc[0] = producerFunc;
   threadFunc[1] = consumerFunc;
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

   for(i=0;i<2;i++) {pthread_create( &handleThread[i], NULL, threadFunc[i], NULL );}
   for(i=0;i<2;i++) {pthread_join( handleThread[i], NULL);}
	exit(EXIT_SUCCESS);
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>


#define STOP_AFTER 20

struct Queue{
	char ** placeforvip;
	char ** head; 
	char ** q;
	char ** tail;
	int qlen;
};

int Q_SIZE;
int M_SIZE;
struct Queue QUEUE;
static pthread_mutex_t Q_LOCKED = PTHREAD_MUTEX_INITIALIZER;
sem_t Q_FULL, Q_EMPTY;
int COUNTER = 0;

int int_message_queue(int queue_size, int queue_entity_size){
	QUEUE.q = (char **)malloc(queue_size * (sizeof(char *)));
	if (QUEUE.q == NULL) return -1;
	QUEUE.head = QUEUE.q;
	QUEUE.tail = QUEUE.q;
	QUEUE.placeforvip = QUEUE.q;
	for (int i =0; i < queue_size; i++)
	{
		QUEUE.q[i] = (char *)malloc(queue_entity_size * sizeof(char));
		if(QUEUE.q[i] == NULL) {return -1;}
	}
	return 0;
}

void init_semaphores(){
	sem_init(&Q_FULL, 0, Q_SIZE);
	sem_init(&Q_EMPTY, 0, 0);	
}

char * create_mess(time_t time1,time_t time2,char type, int size){
	char *mess = (char *)malloc(size * sizeof(char));
	sprintf(mess, "%li, %li, %c", time1, time2, type);
	mess[strlen(mess)+1] = '\0';
	return mess;
}

int put_vip_message(char *message, int message_size){
	if(message_size > M_SIZE) {	return -1;}
	long int diff = QUEUE.tail - QUEUE.placeforvip;
	if (diff){
		char * temp = *QUEUE.placeforvip;
		char *buf;
		for(int i =1; i<=diff; i++){
			buf = QUEUE.placeforvip[i];
			QUEUE.placeforvip[i] = temp;
			temp = buf;
		}
	}
	*QUEUE.placeforvip = message;
	QUEUE.placeforvip++;
	QUEUE.tail++;
	QUEUE.qlen++;
	return 0;
}

int put_user_message(char *message, int message_size){
	if(message_size > M_SIZE){	return -1;}
	*QUEUE.tail = message;
	QUEUE.tail++;
	QUEUE.qlen++;
	return 0;
}

char* cut_mess(char*message, int max_size){
	if (max_size <= 1) return "";
	char *new_mess = (char *)malloc(max_size * sizeof(char));
	for (int i = 0; i < max_size; i++){
		new_mess[i] = message[i];
	}
	new_mess[max_size] = '\0';
	return new_mess;
}

char* read_message(int max_message_size){
	char * mess = *QUEUE.head;
	if (strlen(mess) + 1 > max_message_size){
		mess = cut_mess(mess, max_message_size);
	}
	for (int i = 0; i < QUEUE.qlen-1; i++){
		QUEUE.q[i] = QUEUE.q[i+1];
	}
	if (QUEUE.placeforvip > QUEUE.head){
		QUEUE.placeforvip--;
	}
	QUEUE.qlen--;
	QUEUE.tail--;
	return mess;
}

void show_queue(){
	printf("\t\t\t\t\tQUEUE:\n");
	for (int i = 0; i < QUEUE.qlen; i++){
		printf("\t\t\t\t\t%i. %s\n", i, QUEUE.q[i]); 
	}
}



void *vip(void *args){
	//usleep(5*1000000); //test1

	time_t time1, time2;	
	for(;;){
		time1 = time(0);
		sem_wait(&Q_FULL);
		pthread_mutex_lock(&Q_LOCKED);
		time2 = time(0);
		if (COUNTER >= STOP_AFTER) {
		    pthread_mutex_unlock(&Q_LOCKED);
		    sem_post(&Q_EMPTY);
		    sem_post(&Q_FULL);
		    return NULL;
		}
		char * mess = create_mess(time1, time2, 'v', M_SIZE);
		if (put_vip_message(mess, strlen(mess) +1) == 0) {
			printf("PUT: %s\n", mess);
			show_queue();
			sem_post(&Q_EMPTY);	
		}
		COUNTER ++;
		pthread_mutex_unlock(&Q_LOCKED);
		usleep((time1 % 4 + 1) * 1000000);
	}
}

void *usr(void *args){
	//usleep(5*1000000); //test1

	time_t time1, time2;
	for(;;){
		time1 = time(0);
		sem_wait(&Q_FULL);
		pthread_mutex_lock(&Q_LOCKED);
		time2 = time(0);
		if (COUNTER >= STOP_AFTER) {
		    pthread_mutex_unlock(&Q_LOCKED);
		    sem_post(&Q_EMPTY);
		    sem_post(&Q_FULL);
		    return NULL;
		}
		char * mess = create_mess(time1, time2, 'u', M_SIZE);
		if (put_user_message(mess, strlen(mess) +1) == 0) {
			printf("PUT: %s\n", mess);
			show_queue();
			sem_post(&Q_EMPTY);	
		}
		COUNTER ++;
		pthread_mutex_unlock(&Q_LOCKED);
		usleep((time1 % 4 + 1) * 1000000);
	}
}

void *reader(void * args){
	//usleep(8*1000000); //test2
	time_t time1, time2;
	for(;;){
		time1 = time(0);
		sem_wait(&Q_EMPTY);
		pthread_mutex_lock(&Q_LOCKED);
		time2 = time(0);
		if (COUNTER >= STOP_AFTER) {
		      	pthread_mutex_unlock(&Q_LOCKED);
		      	sem_post(&Q_EMPTY);
		      	sem_post(&Q_FULL);
			return NULL;
		}
		char * mess = read_message(M_SIZE);
		printf("READING TIMES: %li, %li \nMESSAGE: %s\n", time1, time2, mess);
		show_queue();
		COUNTER++;
		pthread_mutex_unlock(&Q_LOCKED);
		sem_post(&Q_FULL);
		usleep((time1 % 4 + 1) * 1000000);
	}
}

int main(int argc, char* argv[]) {
	Q_SIZE = 4;
	M_SIZE = 50;
	if (Q_SIZE < 1 || M_SIZE <= 1){ return -1;}
	int_message_queue(Q_SIZE, M_SIZE);
	init_semaphores();

	pthread_t  thread_vip, thread_usr, thread_read;
	pthread_create(&thread_read,NULL,&reader,NULL);
	pthread_create(&thread_usr,NULL,&usr,NULL);
	pthread_create(&thread_vip,NULL,&vip,NULL);
	
	pthread_join(thread_read,NULL);
	pthread_join(thread_usr,NULL);
	pthread_join(thread_vip,NULL);

	sem_destroy(&Q_FULL);
	sem_destroy(&Q_EMPTY);
	return 0;
}

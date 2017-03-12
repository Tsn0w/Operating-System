#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

#define sendError4Threads(error,RC_COLA) {\
	printf("%s %s: %d\n",(error), strerror(RC_COLA), __LINE__);\
 	exit(RC_COLA);\
}

#define sendError(error){\
	printf("%s %s: %d\n",(error), strerror(errno), __LINE__);\
	exit(errno);\
}

//macro MARCO is for lokcing the mutex.
#define MARCO(list) {\
	int RC_COLA;\
	if( (RC_COLA = pthread_mutex_lock( intlist_get_mutex(list))) !=  0)\
		sendError4Threads("Error locking the mutex.",RC_COLA)\
}

//macro POLO is for unlokcing the mutex.
#define POLO(list){\
	int RC_COLA;\
	if ( (RC_COLA = pthread_mutex_unlock( intlist_get_mutex(list))) != 0 )\
		sendError4Threads("Error, unlocking mutex.",RC_COLA)\
}


typedef struct element
{
	int value;
	struct element *prev;
	struct element *next;
}intelem;

typedef struct Linked_List
{
	int size;
	intelem *first;
	intelem *last;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
}intlist;

void* writer(void* arg);
void* reader(void* arg);
void* thread_garbage (void* arg);

void intlist_init(intlist* list)
{
	int RC_COLA;
	list->size = 0;
	list->first = NULL;
	list->last = NULL;
	pthread_mutexattr_t mutex_attr;
	
	pthread_mutexattr_init(&mutex_attr);// no need to check return 0 anyway
	 
	if ( (RC_COLA = (pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_RECURSIVE))) != 0 )
		sendError4Threads("Error creating attribute mutex.",RC_COLA)

	pthread_mutex_init( &(list->mutex), &mutex_attr);// no need to check return 0 anyway

	if ( (RC_COLA = pthread_mutexattr_destroy(&mutex_attr)) != 0 )
		sendError4Threads("Error remove attribute mutex.",RC_COLA)

	if ( (RC_COLA = pthread_cond_init( &list->cond ,NULL)) != 0 )
		sendError4Threads("Error initialized condition variable.",RC_COLA)
}


void intlist_destroy(intlist* list)
{
	int RC_COLA;
	if (list == NULL || list->first == NULL || list->last == NULL)
		return;

	intelem *curr = list->first;

	while ( curr != list->last)
	{
		curr = curr->next;
		free(curr->prev);
	}
	free (curr);

	if ( (RC_COLA = pthread_mutex_destroy( & ( list->mutex))) != 0 )
		sendError4Threads("Error remove list mutex.",RC_COLA)

	if ( (RC_COLA = pthread_cond_destroy( & ( list->cond))) != 0 )
		sendError4Threads("Error remove list cond.",RC_COLA)
}


pthread_mutex_t* intlist_get_mutex(intlist* list)
{
	if (list == NULL)
		return NULL;
	return &(list->mutex);

}

void intlist_push_head(intlist* list, int value)
{
	if ( list == NULL)
		sendError("Error, list is NULL pointer.")

	int RC_COLA;

	intelem *new_elem;
	if(	(new_elem = (intelem*)malloc(sizeof(intelem))) == NULL)
		sendError("Error, allocating memeory failure for new element.")

	new_elem->value = value; 
	new_elem->prev = NULL;

	//lock mutex.
	MARCO(list)
	
	if(list->size == 0)
		list->last = new_elem;
	else
	{
		list->first->prev = new_elem;
		new_elem->next = list->first;
	}
	list->first = new_elem;
	list->size++;

	if ( (RC_COLA = pthread_cond_signal( & (list->cond))) != 0 )
		sendError4Threads("Error, sedning signal.",RC_COLA)

	// unlock mutex.
	POLO(list)

}

int intlist_pop_tail(intlist* list)
{
	int RC_COLA;
	if ( list == NULL)
		sendError("Error, list is a NULL pointer.")

	int value_out;
	intelem temp;

	//lock mutex.
	MARCO(list)
	

	//free mutex if there are no items in list and wait for signal.
	while (list->size == 0)
		if ( (RC_COLA = pthread_cond_wait( &list->cond, &list->mutex)) != 0 )
			sendError4Threads("Error,condition variable waiting",RC_COLA)


	value_out = list->last->value;
	//remove last item in the list
	if (list->size == 1)
	{
		temp = list ->first;
		list->last = NULL;
		list->first = NULL;
	}
	else
	{
		list->last = list->last->prev;
		temp = list->last->next;
		list->last->next = NULL;
	}
	list->size--;
	
	//unlock mutex.
	POLO(list)

	free(temp);

	return value_out;
}


void intlist_remove_last_k(intlist* list, int k)
{
	int numToRemove;
	int i;
	intelem *prev;

	if ( list == NULL)
		sendError("list is null pointer ")

	if (k < 1)
		return;

	//lock mutex.
	MARCO(list)

	numToRemove = list->size <= k  ? list->size : k;
	//remove all except last.
	for ( i = 0; i < numToRemove; i++)
	{
		prev = list->last->prev;
		free(list->last);
		list->last = prev;
		list->size--;
		if (list->last != NULL) //in case that the last element in list is removed
			list->last->next = NULL;
	}

	//unlock mutex.
	POLO(list)
}

int intlist_size(intlist* list)
{
	if (list == NULL)
		return -1;	
	return list->size;
}
// *********************** Global Variables *************************************************************

intlist *list;
pthread_cond_t GC_cond; //to singal GC that size is max, global variable
int kill_writers = 0; // to sotp writers threads
int kill_readers = 0; // to stop readers threads
int kill_GC = 0; // to stop GC thread
int shouldCollect = 1; 

//********************* Writer *************************************************************************
void* writer(void* arg)
{
	int RC_COLA;
	int MAX = *((int*)arg);
	int random_int;
	while ( kill_writers == 0)
	{
		random_int = rand() % 2147483647;
		//random integermaker.
		intlist_push_head(list, random_int);//already thread-safe
		
		if ( intlist_size(list) > MAX){
			if ( (RC_COLA = pthread_cond_signal(&GC_cond)) != 0 ) // send signal to GC to remove items
				sendError4Threads("Error, sending signal.",RC_COLA)
		}

	}
	pthread_exit(NULL);
}

//******************* Reader ***************************************************************************
void* reader(void* arg)
{
	while (kill_readers == 0 )
		intlist_pop_tail(list);//already thread-safe
	pthread_exit(NULL);
}

//**************** Garbage Collector *******************************************************************
void* thread_garbage (void* arg)
{	
	int RC_COLA;
	int size;
	int elemToRemove;
	int MAX = *((int*)arg);
	while (kill_GC == 0)
	{
		//lock mutex.
		MARCO(list)

		if ( (RC_COLA = pthread_cond_wait( &GC_cond, intlist_get_mutex(list))) != 0 )
			sendError4Threads("Error,condition variable waiting",RC_COLA)
		if (shouldCollect == 1){	
			size = intlist_size(list);
			elemToRemove = size % 2 == 0 ? size / 2 : (size + 1) / 2;

			intlist_remove_last_k(list, elemToRemove);
		}
		//unlock mutex.
		POLO(list)
		printf("GC - %d items removed from the list\n", elemToRemove);
	}
	pthread_exit(NULL);
}
//******************************** simulator ***********************************************************


int main(int argc, char const *argv[])
{
	if ( argc != 5)
		sendError("Error, Wrong number of arguments.")
	int RC_COLA;
	int WNUM = strtol(argv[1], NULL, 10);
	int RNUM = strtol(argv[2], NULL, 10);
	int MAX = strtol(argv[3], NULL, 10);
	int TIME = strtol(argv[4], NULL, 10);

	//allocate memory for list.
	if( (list = (intlist*)malloc(sizeof(intlist))) == NULL )
		sendError("Error, allocating memory failure for list")

	//initialized condition variable for GC.
	if ( (RC_COLA = pthread_cond_init(&GC_cond,NULL)) != 0 )
		sendError4Threads("Error initialized condition variable.",RC_COLA)

	//initialized list
	intlist_init (list);

	//create garbage collector.
	pthread_t garbage_collector_thread;
	if( (RC_COLA = pthread_create( &garbage_collector_thread, NULL, thread_garbage, (void*)&MAX)) != 0 )
		sendError4Threads("Error, creating garbage collector thread.",RC_COLA)

	int i;

	//creating writer threads.
	pthread_t* write_threads = (pthread_t*)malloc(WNUM*(sizeof(pthread_t)));
	for (i = 0; i < WNUM; i++)
	{
		if( (RC_COLA = pthread_create(&write_threads[i],NULL,writer,(void*)&MAX)) != 0 )
			sendError4Threads("Error creating writer thread.",RC_COLA)
		printf("created writer thread no. %d\n",i+1);
	}

	//creating reader threads.
	pthread_t read_threads = (pthread_t*)malloc(RNUM*(sizeof(pthread_t)));
	for (i = 0; i < RNUM; i++)
	{
		if( (RC_COLA = pthread_create(&read_threads[i],NULL,reader,NULL)) != 0 )
			sendError4Threads("Error creating reader thread.",RC_COLA)
		printf("created reader thread no. %d\n",i+1);
	}

	sleep(TIME);
	
	//global variable so infrom that readers need to be stopped
	kill_readers = 1;

	for (i = 0; i < RNUM; ++i)
		if( (RC_COLA = pthread_join(read_threads[i],NULL)) != 0)
			sendError4Threads("Error stopping reader thread.", RC_COLA)


	//global variable so infrom that writers need to be stopped
	kill_writers = 1;

	for (i = 0; i < WNUM; i++)
		if( (RC_COLA = pthread_join(write_threads[i],NULL)) != 0)
			sendError4Threads("Error stopping writer thread.", RC_COLA) 

	//kill GC
	kill_GC = 1;
	shouldCollect = 0;
	if ( (RC_COLA = (pthread_cond_signal(&GC_cond))) != 0 ) // send signal to GC to remove items
		sendError4Threads("Error, sending signal.", RC_COLA)
	
	if( (RC_COLA = pthread_join(garbage_collector_thread,NULL)) != 0)
		sendError4Threads("Error stopping reader thread.", RC_COLA)
	
	int size = intlist_size(list);
	for ( i = 0; i < size; i++)
		printf("item no. %d got value:%d is out\n",i+1,intlist_pop_tail(list));

	printf("list size is: %d\n",size);

	intlist_destroy(list);// to not cause memeory leak
	
	size = intlist_size(list);
	//printf list size and items init.
	printf("list size is: %d\n",size);
	
	free(list);// to not cause memeory leak
	free(read_threads);
	free(write_threads);
	return 0;
}


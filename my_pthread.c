// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name:
// username of iLab:
// iLab Server:

#include "my_pthread_t.h"
#include "ucontext.h"

#define MAX_STACK 1000 //not sure a good value
#define MAX_THREAD 256 //not sure a good value

short mode=0; //0 for SYS, 1 for USR?

typedef struct
{
	int tid;
	int state; //ready,running,waiting,embryo, terminating (enum?)
	void* retVal;
	ucontext_t context;
	//stack
	int timeSlice;
	int priority;
	struct thread* nxt;
} thread;

typedef struct
{
	int locked;
	int isinit;
	thread* nxt; //whoever first tried to get the locked lock
} mutex;

/*
ucontext
	-getcontext: inits struct to currently active context (need to capture main, but it doesn't need stack or ulink)
	-makecontext: basically this is phtread_create (look at its args). Use to init thread
	-swapcontext: does what you think
		-swap thread context with scheduler context to schedule next thread context
	-https://linux.die.net/man/3/swapcontext <- very very good resource+example
	-Just set ucontext stack to malloc(...)

Maintance cycle to reorganize priority queue.
	_____
	| 0 |-> threads of priority 0
	| 1 |-> threads of priority 1
	|...|-> ...

struct itimerval (library) to keep track if timeslice
	-Sends signal when time up and catch with a sig handler.
	-Go to scheduler context when time is up

If user function does not use pthread_exit and just returns it is a problem for return values
	-Put all user function calls in wrapper function to force pthread_exit call
	void wrapper(void* func, void* args)
	{
		(*func)(*args) //probably not 100% correct
		pthread_exit();
	}

Need USR and SYS so scheduler and maintance do not get inturrupted 

Need queue for joins, maybe also for waits
*/

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	return 0;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	return 0;
};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	return 0;
};


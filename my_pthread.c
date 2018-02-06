// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name:
// username of iLab:
// iLab Server:

//Hey, I did a thing
#include "my_pthread_t.h"
#include "ucontext.h"
#include <stdlib.h>
#include <signal.h>

#define MAX_STACK 1000 //not sure a good value
#define MAX_THREAD 256 //not sure a good value
#define MAINTENANCE 10 //not sure a good value
#define PRIORITY_LEVELS 5 //not sure good value

short mode=0; //0 for SYS, 1 for USR?
short ptinit=0; //init main stuff at first call of pthread_create
short maintenanceCounter=MAINTENANCE;
my_pthread_t idCounter=0;
ucontext_t ctx_main, ctx_sched, ctx_maintenance;


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

typedef struct ucontext {
	struct ucontext *uc_link;
	sigset_t         uc_sigmask;
	stack_t          uc_stack;
	mcontext_t       uc_mcontext;
	...
} ucontext_t;
*/

void wrapper(void* (*func)(void*),void* args)
{
	return;
}

void alarm_handler(int signum)
{
	return;
}

void scheduler()
{
	return;
}

void maintenance()
{
	return;
}

/* create a new thread */
int my_pthread_create(my_pthread_t* thread, pthread_attr_t* attr, void*(*function)(void*), void * arg)
{
	if(ptinit==0)
	{
		//init stuff
		//set up main thread/context
		queue=malloc(PRIORITY_LEVELS*sizeof(tcb));
		if(getcontext(&ctx_main)==-1)
		{
			printf("ERROR: Failed to get context for main\n");
			return 1;
		}
		tcb* maint=malloc(sizeof(tcb));
		maint->state=0;
		maint->tid=idCounter++;
		maint->context=ctx_main;
		maint->retVal=NULL;
		//maint->timeslice=0;
		maint->priority=0;
		maint->nxt=NULL;
		maint->state=1;

		//set up scheduler thread/context
		if(getcontext(&ctx_sched)==-1)
		{
			printf("ERROR: Failed to get context for scheduler\n");
			return 1;
		}
		ctx_sched.uc_link=&ctx_main;
		ctx_sched.uc_stack.ss_sp=malloc(MAX_STACK);
		ctx_sched.uc_stack.ss_size=MAX_STACK;
		tcb* schedt=malloc(sizeof(tcb));
		schedt->state=0;
		schedt->tid=idCounter++;
		schedt->context=ctx_sched;
		schedt->retVal=NULL;
		//sched->timeslice=0;
		schedt->priority=0;
		schedt->nxt=NULL;
		schedt->state=1;
		makecontext(&schedt->context,scheduler,0);

		//set up maintenance thread/context
		if(getcontext(&ctx_maintenance)==-1)
		{
			printf("ERROR: Failed to get context for maintenance\n");
			return 1;
		}
		ctx_maintenance.uc_link=&ctx_main;
		ctx_maintenance.uc_stack.ss_sp=malloc(MAX_STACK);
		ctx_maintenance.uc_stack.ss_size=MAX_STACK;
		tcb* maintenancet=malloc(sizeof(tcb));
		maintenancet->state=0;
		maintenancet->tid=idCounter++;
		maintenancet->context=ctx_maintenance;
		maintenancet->retVal=NULL;
		//maintenancet->timeslice=0;
		maintenancet->priority=0;
		maintenancet->nxt=NULL;
		maintenancet->state=1;
		makecontext(&maintenancet->context,maintenance,0);

		signal(SIGALRM,alarm_handler);
		ptinit=1;
	}
	if(idCounter==MAX_THREAD)
	{
		printf("Maximum amount of threads are made, could not make new one\n");
		return 1;
	}
	ucontext_t ctx_func;
	if(getcontext(&ctx_func)==-1)
	{
		printf("ERROR: Failed to get context for new thread\n");
		return 1;
	}
	ctx_func.uc_link=&ctx_main;
	ctx_func.uc_stack.ss_sp=malloc(MAX_STACK);
	ctx_func.uc_stack.ss_size=MAX_STACK;
	tcb* t=malloc(sizeof(tcb));
	t->state=0;
	t->tid=idCounter++;
	t->context=ctx_func;
	t->retVal=NULL;
	//t->timeslice=0;
	t->priority=0;
	t->nxt=NULL;
	t->state=1;
	makecontext(&t->context,function(arg),1,&arg); //i don't know about second param
	return 0;
}

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


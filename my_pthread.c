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
#include <sys/time.h>
#include <unistd.h>

#define MAX_STACK 10000 //not sure a good value
#define MAX_THREAD 64 //TA said a power of 2 and referenced 32
#define MAX_MUTEX 64 //TA said a power of 2 and referenced 32
#define MAINTENANCE 10 //not sure a good value
#define PRIORITY_LEVELS 5 //not sure good value

short mode=0; //0 for SYS, 1 for USR?
short ptinit=0; //init main stuff at first call of pthread_create
short maintenanceCounter=MAINTENANCE;
my_pthread_t idCounter=0;
int activeThreads=0;
ucontext_t ctx_main, ctx_sched,ctx_clean;
tcb* curr;
struct itimerval timer;
struct sigaction sa;


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
	curr->retVal=(*func)(args);
	if(curr->state!=4)
	{
		my_pthread_exit(curr->retVal);
	}
	return;
}

void alarm_handler(int signum)
{
	//i really think all this needs to do is go to the scheduler
	//if you look at pthread_exit I accounted for the drop in priority
	//maybe there is just something i am missing
	//let me know if there is but i don't think so
	//since the scheduler takes care of putting threads back and taking new ones out
	if(mode==1)
	{
		return;
	}
	swapcontext(&curr->context, &ctx_sched);
	return;
}

void scheduler()
{
	mode=1;
	int i,found=0;
	tcb* ptr;
	curr->state=1;
	if(curr->priority<PRIORITY_LEVELS-1)
	{
		curr->priority++;
	}
	//put old thread back in queue
	if(queue[curr->priority]==NULL)
	{
		queue[curr->priority]=curr;
	}
	else
	{
		tcb* temp=queue[curr->priority];
		while(temp->nxt!=NULL)
		{
			temp=temp->nxt;
		}
		temp->nxt=curr;
	}
	maintenanceCounter--;
	if(maintenanceCounter<=0)
	{
		maintenanceCounter=MAINTENANCE;
		maintenance();
	}
	//try to find a thread that can be run
	for(i=0;i<PRIORITY_LEVELS;i++)
	{
		ptr=queue[i];
		while(ptr!=NULL)
		{
			if(ptr->state==1)
			{
				ptr->state=2;
				curr=ptr;
				found=1;
				break;
			}
			ptr=ptr->nxt;
		}
		if(found)
		{
			break;
		}
	}
	//if there is no thread that can run
	if(!found)
	{
		printf("Found no thread ready to run\n");
		return;
	}
	timer.it_value.tv_sec=0;
	timer.it_value.tv_usec=(curr->priority+1)*25000;
	getitimer(ITIMER_REAL,&timer); //make sure this works right
	mode=0;
	swapcontext(&ctx_sched,&curr->context);
	return;
}

void maintenance()
{
	//address priority inversion (priority inheritence or ceiling
	//give all threads priority 0 to prevent starvation. (does this fix priority inversion?)
	int i;
	tcb* top=queue[0];
	//put everything in first level (i think the book did something like this)
	for(i=1;i<PRIORITY_LEVELS;i++)
	{
		while(top->nxt!=NULL)
		{
			top=top->nxt;
			top->priority=0;
		}
		top->nxt=queue[i];
		queue[i]=NULL;
	}
	return;
}

void clean() //still need to setup context
{
	//set scheduler's uclink to this so cleanup can be done when the scheduler ends
	//i think just free stuff
	return;
}

/* create a new thread */
int my_pthread_create(my_pthread_t* thread, pthread_attr_t* attr, void*(*function)(void*), void * arg)
{
	if(ptinit==0)
	{
		//init stuff
		//set up main thread/context
		queue=malloc(PRIORITY_LEVELS*sizeof(tcb*));
		terminating=malloc(MAX_THREAD*sizeof(tcb*));
		int i=0;
		for(i;i<PRIORITY_LEVELS;i++)
		{
			queue[0]=NULL;
		}
		if(getcontext(&ctx_main)==-1)
		{
			printf("ERROR: Failed to get context for main\n");
			return 1;
		}
		tcb* maint=malloc(sizeof(tcb*));
		maint->state=0;
		maint->tid=idCounter++;
		maint->context=ctx_main;
		maint->retVal=NULL;
		maint->priority=0;
		maint->nxt=NULL;
		maint->state=1;
		curr=maint;

		/*
		//set up context for cleanup
		if(getcontext(&ctx_clean)==-1)
		{
			printf("ERROR: Failed to get context for cleanup\n");
			return;
		}
		*/

		//set up scheduler thread/context
		if(getcontext(&ctx_sched)==-1)
		{
			printf("ERROR: Failed to get context for scheduler\n");
			return 1;
		}
		ctx_sched.uc_link=&ctx_clean;
		ctx_sched.uc_stack.ss_sp=malloc(MAX_STACK);
		ctx_sched.uc_stack.ss_size=MAX_STACK;
		/*
		tcb* schedt=malloc(sizeof(tcb*));
		schedt->state=0;
		activeThreads++;
		schedt->tid=idCounter++;
		schedt->context=ctx_sched;
		schedt->retVal=NULL;
		schedt->timeslice=0;
		schedt->priority=0;
		schedt->nxt=NULL;
		schedt->state=1;
		*/
		makecontext(&ctx_sched,scheduler,0);

		//set first timer
		signal(SIGALRM,alarm_handler);
		timer.it_value.tv_sec=0;
		timer.it_value.tv_usec=25;
		getitimer(ITIMER_REAL,&timer);

		ptinit=1;
	}
	if(activeThreads==MAX_THREAD)
	{
		printf("ERROR: Maximum amount of threads are made, could not make new one\n");
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
	tcb* t=malloc(sizeof(tcb*));
	t->state=0;
	t->tid=idCounter++;
	t->context=ctx_func;
	t->retVal=NULL;
	t->priority=0;
	t->nxt=NULL;
	t->state=1;
	makecontext(&t->context,function(arg),1,&arg); //i don't know about second param***********
	if(queue[0]==NULL)
	{
		queue[0]=t;
	}
	else
	{
		tcb* ptr=queue[0];
		while(ptr->nxt!=NULL)
		{
			ptr=ptr->nxt;
		}
		ptr->nxt=t;
	}
	activeThreads++;
	return 0;
}

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield()
{
	curr->priority = PRIORITY_LEVELS-2;
	swapcontext(&curr->context,&ctx_sched);
	return 0;
}

/* terminate a thread */
void my_pthread_exit(void* value_ptr)
{
	if(value_ptr==NULL)
	{
		printf("value_ptr is NULL\n");
		return;
	}
	//mark thread as terminating
	curr->state=4;
	//remove curr from queue
	if(queue[curr->priority]->tid==curr->tid)
	{
		queue[curr->priority]=queue[curr->priority]->nxt;
	}
	else
	{
		tcb* ptr;
		ptr=queue[curr->priority];
		tcb* prev;
		while(1)
		{
			if(ptr->tid==curr->tid)
			{
				prev->nxt=ptr->nxt;
				break;
			}
			prev=ptr;
			ptr=ptr->nxt;
		}
	}
	//add curr to terminating list
	if(terminating==NULL)
	{
		terminating=curr;
		curr->nxt=NULL;
	}
	else
	{
		curr->nxt=terminating;
		terminating=curr;
		
	}
	//set value_ptr to retVal of terminating thread?
	curr->retVal=value_ptr;
	//yield thread
	my_pthread_yield();
	return;
}

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr)
{
	if(value_ptr==NULL)
	{
		printf("value_ptr is NULL\n");
		return 1;
	}
	//mark thread as waiting
	curr->state=3;
	//look for "thread" in terminating list
	tcb* ptr=terminating;
	if(terminating->tid==thread)
	{
		value_ptr=ptr->retVal;
		terminating=terminating->nxt;
		free(ptr);
		activeThreads--;
		curr->state=1;
		return 0;
	}
	else
	{
		tcb* prev=ptr;
		ptr=ptr->nxt;
		while(1)
		{
			if(ptr->tid==thread)
			{
				value_ptr=ptr->retVal; //not too sure about this (pointer stuff)
				free(ptr);
				activeThreads--;
				return 0;
			}
			ptr=ptr->nxt;
		}
	}
	my_pthread_yield();
	return 0;
}

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	return 0;
}

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


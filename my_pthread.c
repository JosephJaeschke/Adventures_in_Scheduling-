// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name:
// username of iLab:
// iLab Server:
#define _XOPEN_SOURCE_EXTENDED 1

#include "my_pthread_t.h"
#include "ucontext.h"
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

#define MAX_STACK 32000 //my TA told someone that 32k is a good number
#define MAX_THREAD 64 //TA said a power of 2 and referenced 32
#define MAX_MUTEX 64 //TA said a power of 2 and referenced 32
#define MAINTENANCE 10 //not sure a good value
#define PRIORITY_LEVELS 5 //not sure good value

short mode=0; //0 for SYS, 1 for USR
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

void wrapper(void*(*f)(void*),void* a)
{
//	printf("--start wrapping\n");
	curr->retVal=(*f)(a);
//	printf("--done wrapping\n");
//	fflush(stdout);
	if(curr->state!=4)
	{
		curr->retVal=malloc(1);
		my_pthread_exit(curr->retVal);
	}
	return;
}

void alarm_handler(int signum)
{
//	printf("===ALARM=== (mode=%d)\n",mode);
	fflush(stdout);
	if(mode==0)
	{
		return;
	}
	//i put the stuff in sched. I hope it is not too slow, but it was better for debugging
	if(swapcontext(&curr->context, &ctx_sched)==-1)
	{
		printf("ERROR: Failed to swap to sched\n");
		fflush(stdout);
	}
	return;
}

void scheduler()
{
	while(1)
	{
		mode=0;
//		printf("--sched | p=%u, op=%u\n",curr->priority,curr->oldPriority);
//		fflush(stdout);
		//remove curr from queue at old priority level
		if(curr->state!=4)
		{
			if(queue[curr->oldPriority]->tid == curr->tid)
			{
				queue[curr->oldPriority]=queue[curr->oldPriority]->nxt;
			}
			else
			{
				tcb *ptr, *prev;
				ptr = queue[curr->oldPriority];
				while(ptr->nxt != NULL)
				{
					if(ptr->tid == curr->tid)
					{
						prev->nxt = ptr->nxt;
						break;
					}
					prev = ptr;
					ptr = ptr->nxt;
				}
			}
			if(curr->priority<PRIORITY_LEVELS-1)//////////////////////////////
			{
				curr->priority++;
			}
			curr->oldPriority=curr->priority;///////////////////////////////
			//put old thread back in queue
			curr->nxt=NULL;
			if(queue[curr->priority]==NULL)//if empty
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
		}
		int i,found=0;
		tcb* ptr;
		curr->state=1;
//		maintenanceCounter--;
//		if(maintenanceCounter<=0)
//		{
//			maintenanceCounter=MAINTENANCE;
//			maintenance();
//		}
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
//				printf("--run thread w/ id=%u p=%d\n",curr->tid,curr->priority);
				break;
			}
		}
		//if there is no thread that can run
		if(!found)
		{
//			printf("--Found no thread ready to run\n");
			return;
		}
		timer.it_value.tv_sec=0;
		timer.it_value.tv_usec=(curr->priority+1)*25000;
		setitimer(ITIMER_REAL,&timer,NULL);
		mode=1;
		swapcontext(&ctx_sched,&curr->context);
	}
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
	//The assignment description says we're supposed to change priority 
	//levels based on how long a thread has been running so older threads 
	//can move up in priority. I don't really know how to do this though
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
		queue=malloc(PRIORITY_LEVELS*sizeof(tcb));
		terminating=malloc(sizeof(tcb));
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
		tcb* maint=malloc(sizeof(tcb));
		maint->state=0;
		maint->tid=idCounter++;
		maint->context=ctx_main;
		maint->retVal=NULL;
		maint->priority=0;
		maint->oldPriority=0;
		maint->nxt=NULL;
		maint->state=1;
		curr=maint;
		queue[0]=maint;
//		printf("--Got main context\n");
		//set up context for cleanup
//		if(getcontext(&ctx_clean)==-1)
//		{
//			printf("ERROR: Failed to get context for cleanup\n");
//			return 1;
//		}
//		makecontext(&ctx_clean,clean,0);

		//set up scheduler thread/context
		if(getcontext(&ctx_sched)==-1)
		{
			printf("ERROR: Failed to get context for scheduler\n");
			return 1;
		}
		ctx_sched.uc_link=0;
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
//		printf("--Got context for scheduler\n");

		//set first timer
		signal(SIGALRM,alarm_handler);
		timer.it_value.tv_sec=0;	
		timer.it_value.tv_usec=25000; 
		setitimer(ITIMER_REAL,&timer,NULL);
//		printf("--timer set\n");
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
	ctx_func.uc_link=&curr->context;  //i think ************************
	ctx_func.uc_stack.ss_sp=malloc(MAX_STACK);
	ctx_func.uc_stack.ss_size=MAX_STACK;
	tcb* t=malloc(sizeof(tcb));
	t->state=0;
	t->tid=idCounter++;
	*thread=t->tid;
	t->context=ctx_func;
	t->retVal=NULL;
	t->priority=0;
	t->oldPriority=0;
	t->nxt=NULL;
	t->state=1;
//	package* p=malloc(sizeof(package*));
//	p->functionP=function;
//	p->argsP=arg;
//	void* warg=(void*)p;
	makecontext(&t->context,wrapper,2,function,arg);
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
//	printf("--added to queue, id %u\n",t->tid);
	activeThreads++;
	return 0;
}

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield()
{
//	printf("--yield\n");
//	fflush(stdout);
	curr->priority = PRIORITY_LEVELS-1;
	swapcontext(&curr->context,&ctx_sched);
}

/* terminate a thread */
void my_pthread_exit(void* value_ptr)
{
//	printf("--exit\n");
//	fflush(stdout);
	if(value_ptr==NULL)
	{
		printf("ERROR: value_ptr is NULL\n");
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
//	printf("--join\n");
	/*
	if(value_ptr==NULL)
	{
		printf("value_ptr is NULL\n");
		return 1;
	}
	*/
	//mark thread as waiting
	curr->state=3;
	//look for "thread" in terminating list
	while(1)
	{
		if(terminating==NULL)
		{
			continue;
		}
		else if(terminating->tid==thread)//thread is first in list
		{
//			printf("--%u joining w/ %u\n",curr->tid,terminating->tid);
			value_ptr=terminating->retVal;
			terminating=terminating->nxt;
			free(terminating);
			activeThreads--;
			curr->state=1;
			return 0;
		}
		else//thread is not first in list
		{
			tcb* ptr=terminating;
			tcb* prev=ptr;
			ptr=ptr->nxt;
			while(ptr!=NULL)
			{
				if(ptr->tid==thread)
				{
//					printf("--%u joining w/ %u\n",curr->tid,ptr->tid);
					prev->nxt=ptr->nxt;
					value_ptr=ptr->retVal; //not too sure about this (pointer stuff)
					free(ptr);
					activeThreads--;
					return 0;
				}
				ptr=ptr->nxt;
			}
		}
		my_pthread_yield();
	}
	return 0;
}
/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) 
{
	//from my understanding, the user is passing in a pointer to a my_pthread_mutex_t object, which is a struct pointer?
	if (mutexList == NULL)
	{
		mutexList =  mutex;
	}else{
		my_pthread_mutex_t *ptr = mutexList;
		while (ptr->next != NULL)
		{
			ptr = ptr->next;
			
		}
		ptr->next = mutex;
	}
	mutex->locked = 0;
	mutex->maxP=PRIORITY_LEVELS;
	//mutex->has = curr;
	mutex->next = NULL;
	return 0;
}

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) 
{
	while(__atomic_test_and_set(&mutex->locked, __ATOMIC_SEQ_CST) == 1)
	{
		//mutex already locked, change state to waiting
		curr->state = 3;
		//remove curr from ready queue
		if (queue[curr->priority]->tid == curr->tid)
		{
			queue[curr->priority] = curr->nxt;
		}else{
			tcb *ptr, *prev;
			ptr = queue[curr->priority];
			while(ptr->nxt != NULL)
			{
				if (ptr->tid == curr->tid)
				{
					prev->nxt = ptr->nxt;
					break;
				}
				prev = ptr;
				ptr = ptr->nxt;
			}
		}
		
		//place curr at the end of waiting queue for this mutex
		if(mutex->waiting == NULL)
		{
			mutex->waiting = curr;
		}else{
			tcb *ptr = mutex->waiting;
			while(ptr->nxt != NULL)
			{
				ptr = ptr->nxt;
			}
			ptr->nxt = curr;
		}
		my_pthread_yield();
	}
	return 0;
}

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) 
{
	//double check that mutex is not already unlocked (just in case)
	if(mutex->locked == 0)
	{
		printf("ERROR: Attempting to unlock UNLOCKE mutex\n");
		return 1;
	}

	//update mutex_locked status and place next in waiting queue into ready queue
	__atomic_clear(&mutex->locked, __ATOMIC_SEQ_CST);
	tcb *ptr = mutex->waiting;
	mutex->waiting = ptr->nxt;
	ptr->state = 1; //change state to ready
	queue[0] = ptr;	//need to properly add
	return 0;
}

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) 
{
	//cannot destroy a locked mutex
	if (mutex->locked == 1)
	{
		printf("ERROR: Attempting to destroy LOCKED mutex\n");
		return 1;
	}

	//change status to "destroyed" and remove from mutexList
	mutex->locked = 2;
	my_pthread_mutex_t *prev, *ptr;
	ptr = mutexList;
	while (ptr->next != NULL)
	{
		if (ptr == mutex)
		{
			if (prev == NULL)
			{
				mutexList = mutex->next;
				mutex->next = NULL;
			}else{
				prev->next = mutex->next;
				mutex->next = NULL;
			}
		}else{
			prev = ptr;
			ptr = ptr->next;
		}
	}
	return 0;
}


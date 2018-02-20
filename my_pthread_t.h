/*/ File:	my_pthread_t.h
// Author:	Yujie REN
// Date:	09/23/2017

// name: Joseph Jaeschke, Crystal Calandra, Adeeb Kebir
// username of iLab: jjj93, crystaca, ask171
// iLab Server: cray1
*/
#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

#define _GNU_SOURCE

#define pthread_t my_pthread_t
#define pthread_create my_pthread_create
#define pthread_join my_pthread_join
#define pthread_exit my_pthread_exit
#define pthread_yield my_pthread_yield
#define pthread_mutex_init my_pthread_mutex_init
#define pthread_mutex_lock my_pthread_mutex_lock
#define pthread_mutex_unlock my_pthread_mutex_unlock
#define pthread_mutex_destroy my_pthread_mutex_destroy


/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>

//typedef uint my_pthread_t;

typedef struct threadControlBlock 
{
	int tid;
	int state; //embryo,ready,running,waiting,terminating,joining
	void* retVal;
	ucontext_t context;
	//stack
	int timeslice;
	int priority;
	int oldPriority;
	struct threadControlBlock* nxt;
} tcb; 

/* mutex struct definition */
typedef struct my_pthread_mutex_t 
{
	int locked; //unlocked = 0; locked = 1; destroyed = 2;
	tcb* waiting; //waiting queue of threads for this mutex, 
	tcb* has_lock; //keeps track of who has lock.  used for inheritance
	int maxSeenPriority; //maximum seen priority when locked, starts at PRIORITY_LEVELS-1
	int priorityChange; //0 if priority is fine, 1 if seen a better one and needs to move
	int maxP;
	//first in waiting queue is first to run when mutex unlocked
	struct my_pthread_mutex_t* next; //pointer to next mutex to create mutexList
} my_pthread_mutex_t;

typedef union _mask
{
	struct _halfs
	{
		int lhalf:32;
		int hhalf:32;
	}halfs;
	void* data;
}mask;
/* define your data structures here: */

tcb** queue;
tcb* terminating;
my_pthread_mutex_t* mutexList;

// Feel free to add your own auxiliary data structures
/* Function Declarations: */

void alarm_handler(int signum);
void scheduler();
void maintenance();

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield();

/* terminate a thread */
void my_pthread_exit(void *value_ptr);

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr);

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);

#endif

// File:	my_pthread_t.h
// Author:	Yujie REN
// Date:	09/23/2017

// name:
// username of iLab:
// iLab Server: 
#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

#define _GNU_SOURCE

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>

typedef uint my_pthread_t;

typedef struct threadControlBlock 
{
	int tid;
	int state; //embryo,ready,running,waiting,terminating (enum?)
	void* retVal;
	ucontext_t context;
	//stack
	int timeslice;
	int priority;
	int regPriority;
	struct threadControlBlock* nxt;
} tcb; 

/* mutex struct definition */
typedef struct my_pthread_mutex_t 
{
	int locked; //unlocked = 0; locked = 1;
	tcb* has; //whoever first tried to get the locked lock
	//has's nxt should be next in line for the lock and so on since it should
	//not be in the run queue
	struct my_pthread_mutex_t* next; //pointer to next mutex to create mutexList
} my_pthread_mutex_t;

/* define your data structures here: */

tcb** queue;
tcb* terminating;
my_pthread_mutex_t* mutexList;

// Feel free to add your own auxiliary data structures
/* Function Declarations: */

void wrapper(void* (*func)(void*),void* args);
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

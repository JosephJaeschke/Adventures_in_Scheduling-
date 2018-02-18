#include "my_pthread_t.h"

my_pthread_mutex_t lock;
//my_pthread_mutex_init(&lock, NULL);
long long count = 0;

void* fun()
{
	printf("In fun\n");
	int a=4;
	int* ret=&a;
	*ret=4;
	int i=0;
	while(i<999999999)
	{
		i=i+1;
	}
	printf("done being busy\n");
}

int main()
{
	pthread_t tid;
	printf("start thread stuff\n");
	pthread_create(&tid,NULL,fun,NULL);
	my_pthread_mutex_init(&lock, NULL);
	pthread_yield();
	printf("returned from the yieldy void\n");
	void** v=malloc(sizeof(int));
	my_pthread_mutex_lock(&lock);
	count += 5;
	pthread_yield();
	my_pthread_mutex_unlock(&lock);
	pthread_join(tid,v);
	printf("answer: ");
//	fflush(stdout);
	printf("%d\n",**(int**)v);
	my_pthread_mutex_destroy(&lock);
	my_pthread_mutex_lock(&lock);
	printf("DONE!!!\n");
	return 0;
}
/*
//// this function is run by the second thread 
void *inc_x(void *x_void_ptr)
{
//	* increment x to 100 
	int *x_ptr = (int *)x_void_ptr;
	while(++(*x_ptr) < 100);
	printf("x increment finished\n");
//	* the function must return something - NULL will do 
	return NULL;
}
int main()
{
	int x = 3, y = 0;
	// show the initial values of x and y 
	printf("x: %d, y: %d\n", x, y);
	// this variable is our reference to the second thread 
	pthread_t inc_x_thread;
	// create a second thread which executes inc_x(&x) 
	if(pthread_create(&inc_x_thread, NULL, inc_x, &x)) {
		fprintf(stderr, "Error creating thread\n");
		return 1;
	}
	// increment y to 100 in the first thread 
	while(++y < 100);
	printf("y increment finished\n");
	// wait for the second thread to finish 
	if(pthread_join(inc_x_thread, NULL)) {
		fprintf(stderr, "Error joining thread\n");
		return 2;
	}
	// show the results - x is now 100 thanks to the second thread 
	printf("x: %d, y: %d\n", x, y);
	return 0;
}
*/

#include "my_pthread.c"

void* fun()
{
	printf("In fun\n");
	int i=0;
	while(i<9999999999)
	{
		i=i+1;
	}
}

int main()
{
	my_pthread_t tid;
	printf("start thread stuff\n");
	my_pthread_create(&tid,NULL,fun,NULL);
	void** ahhh=malloc(100);
	my_pthread_join(tid,ahhh);
	printf("WE DONE!!!\n");
	return 0;
}

#include "headers.h"


/*
╔══════════════════════════════════════════════════════════════╗
║ thread work with held tasks					                       ║
╚══════════════════════════════════════════════════════════════╝
*/



void * threadSheduller(void * arg){
	int id=*(int*)arg;
	message msg;
	worklist * tmp;
	free(arg);
	printf("start Sheduller %d\n",id);
	while(config.run){
		//check tasks
		tmp=&config.sheduller.task;
		semop(config.sheduller.sem,&sem[0],1);
			for(tmp=tmp->next;tmp!=0;tmp=tmp->next){
				//some work
				//send message
				sendMsg(msg);
			}
		semop(config.sheduller.sem,&sem[1],1);
		
//		while(getMsg(msg)>0){
//			printf("get message %s\n",msg.buf);
			//some work
//		}
		printf("ok\n");
		sleep(1);
	}
	printf("close Sheduller\n");
	return 0;
}

pthread_t startSheduller(int id){
	pthread_t th=0;
	int * arg;
	
	if ((arg=malloc(sizeof(int)))==0)
		perror("malloc startSheduller");
	*arg=id;
	
	if((config.sheduller.sem=semget(IPC_PRIVATE, 1, 0755 | IPC_CREAT))==0)
		return 0;
	semop(config.sheduller.sem,&sem[2],1);
	
	if ((config.sheduller.msg=msgget(IPC_PRIVATE,IPC_CREAT|0700))==0)
		return 0;
	
	if(pthread_create(&th,0,threadSheduller,arg)!=0)
		return 0;
	return th;
}

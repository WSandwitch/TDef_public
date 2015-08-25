﻿#include "headers.h"


/*
╔══════════════════════════════════════════════════════════════╗
║ thread wark with play rooms get statistics and other data                        ║
║ created by Dennis Yarikov						                       ║
║ sep 2014									                       ║
╚══════════════════════════════════════════════════════════════╝
*/




static int connSendRecv(char * hostname,int port, int token){
	int sockfd,n;
	struct sockaddr_in servaddr;
	struct hostent *server;
	char mes_;
	char type='c'; //'c' for get new room
	if (hostname==0)
		return -1;
	printf("try %s:%d\n",hostname,port);
	server = gethostbyname(hostname);
	if (server == NULL) {
		perror("gethostbyname");
		return -1;
	}
	
	if((sockfd=socket(AF_INET,SOCK_STREAM,0))<0){
		perror("socket");
		return -1;
	}
	
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	memcpy((char *)&servaddr.sin_addr.s_addr,(char *)server->h_addr, server->h_length);
//	servaddr.sin_addr.s_addr=inet_addr("172.16.1.40");//argv[1]);
	servaddr.sin_port=htons(port);

	if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))<0){
		perror("connect");
		return -2;
	}	
	
	sendData(sockfd,&type,sizeof(type));
//	printf("send token %d \n",message->token);
	sendData(sockfd,&token,sizeof(token));
		
	n=recv(sockfd,&mes_,sizeof(mes_),0);
	printf("get answer %d\n",n);
	if (n<0)
		return 0;
	else
		if (n>0){
			close(sockfd);
			return n;
		}
	return 0;
}


static int checkRoomStatus(room * r){
	float Dt=0.1;
//	struct {char type;int token;} message;
//	memset(&message,0,sizeof(message));
//	printf("room tok= %d status= %d\n",r->token,r->status);
	if (r->status==ROOM_PREPARE){
		if (r->timer>0)
			r->timer-=Dt;
		else{
//			message.type='c'; //'c' for get new room
//			message.token=r->token;
			//ask for room $_$
			int i;
			int * $_$;
			int _$_;
			printf("try to get room\n");
			//try to find free server
			//TODO: get sort array of servers
			$_$=serversGetSort();
			for(i=1;i<=*$_$;i++)
				if ((_$_=connSendRecv(serverGetById($_$[i]),
							serversGetPortById($_$[i]),
							r->token))<=0){//we do not get answer, all ok
					//printf("_$_= %d\n",_$_);
					if (_$_<0){//cant connect
						serversSetFail($_$[i]);
					}else{//all ok
						r->status=ROOM_WAIT;
						r->server=$_$[i];
						return 0;
					}
				}
			//we get answer from all servers, cant create room
			printf("cant get room\n");
			r->status=ROOM_FAIL;
			r->timestamp=time(0);
			return 0;
		}
	}
	if (r->status==ROOM_ERROR){
		//ask for room
	}
/*	if (r->status==ROOM_FAIL){
		//room is free
		//if (r->users==0){
		room* _r;
		//remove room
		printf("r->  %d  %d %d\n",r->id,r->type,r->token);
		t_semop(t_sem.room,&sem[0],1);
		_r=roomRem(r->type,r->id);
		if (_r!=0){
			printf("room removed\n");
			free(_r);
		}
		t_semop(t_sem.room,&sem[1],1);
		//}
	}
*/	return 0;
}

static inline int proceedServerMessage(worklist* w,char msg_type){
//	char msg;
	short l_l;
	int token;
	room * r_r;
	event * e_e;
	if (msg_type==MESSAGE_UPDATE){ //packet [mes(char) ..
		//had a request for update
		int sock=dup(w->sock); //we close original later
		//push it to updater
		worklist * tmp;
		t_semop(t_sem.updater,&sem[0],1);
			//add client to serverworker
			tmp=worklistAdd(&config.updater.task,0);
			tmp->sock=sock;
		t_semop(t_sem.updater,&sem[1],1);
		//for delete it
		return 1;
	}
	if (msg_type==MESSAGE_ROOM_STATUS){ //packet [mes(char)token(int)status(short)]
		recvData(w->sock,&token,sizeof(token));
		int port;
		short status;
//		printf("room\n");
//		printf("need %d\n",sizeof(room->port));
		recvData(w->sock,&port,sizeof(port)); //int
//		printf("port -> %hd\n",r_r->port);
		recvData(w->sock,&status,sizeof(status)); //short
//		printf("status -> %hd\n",room->status);
		t_semop(t_sem.room,&sem[0],1); 
		r_r=roomGetByToken(token);
		if (r_r!=0){
			r_r->port=port;
			r_r->status=status;
			e_e=eventGet(r_r->type);
			if (e_e!=0){
				l_l=strlen(e_e->map);
				sendData(w->sock,&l_l,sizeof(l_l));
				sendData(w->sock,e_e->map,l_l);
			}else
				close(w->sock);
			r_r->timestamp=time(0);
		}else
			close(w->sock);
		t_semop(t_sem.room,&sem[1],1);
		return 0;
	}
	if (msg_type==MESSAGE_ROOM_RESULT){ //packet [mes(char)token(int)playertoken(int) ..
		printf("get room result\n");
		recvData(w->sock,&token,sizeof(token));
		t_semop(t_sem.room,&sem[0],1);
		r_r=roomGetByToken(token);
		//add another
		if (r_r!=0){
		//add some data
		
		//at the end get status
			recvData(w->sock,&r_r->status,sizeof(r_r->status)); //short
			r_r->status=ROOM_FAIL;
		}else
			close(w->sock);
		t_semop(t_sem.room,&sem[1],1);

//		printf("get n set r status %d\n",r_r->status);
		return 0;
	}
//	printf("end of mes recv\n");
	return 0;
}

static inline int recvServerData(worklist* w){
	int i;
	char msg_type;
//	player_info * pl=w->data;
	//try to read message from server 2 times
	for(i=0;i<2;i++){
//		msg_type=0;
		if (recv(w->sock,&msg_type,sizeof(msg_type),MSG_DONTWAIT)<=0){
			//have error check what is it
			if (errno==EAGAIN){
				errno=0;
				sleep(0);
				continue;
			}else{
				perror("recv threadServerWorker");
				return 1;
			}
		}
		//get message need to proceed
		if (proceedServerMessage(w,msg_type)!=0)
			return 1;
	}
	
	return 0;
}


void * threadServerWorker(void * arg){
	int id=*(int*)arg;
//	int i;
	worklist * tmp;
	int TPS=10;  //ticks per sec
	struct timeval tv={0,0};
	timePassed(&tv);
	free(arg);
	serversLoad();
	printf("ServerWorker %d started\n",id);
	
	while(config.run){
		tmp=&config.serverworker.client;
		t_semop(t_sem.serverworker,&sem[0],1);
			for(tmp=tmp->next;tmp!=0;tmp=tmp->next){
				//get data from server about players and statistics
				if (recvServerData(tmp)!=0){
					printf("del serv from list\n");
					//close socket
					close(tmp->sock);
					//delete client
					tmp=worklistDel(&config.serverworker.client,tmp->id);
					config.serverworker.client_num--;
				}
				
			}
		t_semop(t_sem.serverworker,&sem[1],1);
			
		t_semop(t_sem.room,&sem[1],1);
		roomCheckAll(checkRoomStatus);
		t_semop(t_sem.room,&sem[0],1);
		
		//some work
		syncTPS(timePassed(&tv),TPS);
	}
	printf("close ServerWorker\n");
	return 0;
}

pthread_t  startServerWorker(int id){
	pthread_t th=0;
	int * arg;
	
	if ((arg=malloc(sizeof(int)))==0)
		perror("malloc startServerWorker");
	*arg=id;
	
	if((t_sem.serverworker=t_semget(IPC_PRIVATE, 1, 0755 | IPC_CREAT))==0)
		return 0;
	t_semop(t_sem.serverworker,&sem[1],1);
	
	if((t_sem.room=t_semget(IPC_PRIVATE, 1, 0755 | IPC_CREAT))==0)
		return 0;
	t_semop(t_sem.room,&sem[1],1);
	
	if(pthread_create(&th,0,threadServerWorker,arg)!=0)
		return 0;
	return th;
}

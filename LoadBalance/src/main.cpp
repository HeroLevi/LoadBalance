#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <list>
#include <unistd.h>

#include "processpool.h"

#define MAX_EVENTS 1024
#define BUFF_SIZE 128
using namespace std;

extern void conn();

int main()
{
	cout<<"server start.."<<endl;

	conn();

	int use_count = 0;

	struct sockaddr_in saddr;
	bzero(&saddr,sizeof(saddr));

	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr("192.168.43.25");
	saddr.sin_port = htons(6510);

	int s_socket = socket(AF_INET,SOCK_STREAM,0);
	assert(s_socket != -1);

	int ret = bind(s_socket,(struct sockaddr*)&saddr,sizeof(saddr));
	if( ret == -1)
	{
		printf("bind error\n");
		exit(-1);
	}

	listen(s_socket,5);


	processpool* pool =  processpool::create(s_socket);
	if(pool)
	{
		pool->run();
		delete pool;
	}

	close(s_socket);
	exit(0);
}


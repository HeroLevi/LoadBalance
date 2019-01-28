#ifndef _STRUCT_H
#define _STRUCT_H
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
#include <time.h>


using namespace std;

typedef struct Data
{
	string ip;
	string port;
	bool tag;
}Data;

typedef struct Map
{
	int sock;
	string port;
}Map;

typedef struct User
{
	struct sockaddr_in address;
	int connfd;
	int servfd;
}User;

typedef struct Conum
{
	int sock;
	int connum;
}Conum;

typedef struct Log
{
//	char ip[16];//Ip
//	int port;//端口
	time_t time;
	int level;//错误严重程度 如果是服务器宕机，设为1严重 其他错误0注意
	int err_info;//错误信息 如果错误不是由宕机产生的 置为-1 否则置为1
	int downtime_sfd;//宕机的fd 
}Log;
#endif


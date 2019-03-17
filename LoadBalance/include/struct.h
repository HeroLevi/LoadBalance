#ifndef _STRUCT_H
#define _STRUCT_H


//Connect
typedef struct
{
	char server_ip[16];
	int server_port;
}Serv_info;

typedef struct
{
	int serv_fd;//处理服务器socketfd
	int con_num;//连接数量
}Serv_load;

typedef struct
{
	int cfd;
	int sfd;
}User;
#endif

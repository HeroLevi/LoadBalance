#include <iostream>
#include <string>
#include <list>
#include <iterator>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "struct.h"

using namespace std;

list<Data> conf_list;
list<Map>  m;
list<Conum> con;
int socketfd;

void read_conf()
{
	fstream f;
	f.open("./conf.xml");
	Data data;
	while((f>>data.ip)&&(f>>data.port))
	{
		data.tag=0;
		conf_list.push_back(data);
	}

	f.close();
}


void conn()
{
	read_conf();

	list<Data>::iterator it = conf_list.begin();
	for(;it != conf_list.end();++it)
	{		
		struct sockaddr_in caddr;
		bzero(&caddr,sizeof(caddr));
		caddr.sin_family = AF_INET;
		caddr.sin_addr.s_addr = inet_addr((it->ip).c_str());
		caddr.sin_port = htons(atoi((it->port).c_str()));

		//cout<<(it->ip).c_str()<<" "<<atoi((it->port).c_str())<<endl;

		socketfd = socket(AF_INET,SOCK_STREAM,0);
		assert( socketfd != -1 );

		int ret = connect(socketfd,(struct sockaddr*)&caddr,sizeof(caddr));
		if(ret == -1 )
		{
			cout<<"connect error++ "<<endl;
		}
		else
		{
			Map node;
			node.sock = socketfd;
			node.port = it->port;
			m.push_back(node);
			
			Conum con_info;
			con_info.sock = socketfd;
			con_info.connum = 0;
			con.push_back(con_info);
		}
	}
}

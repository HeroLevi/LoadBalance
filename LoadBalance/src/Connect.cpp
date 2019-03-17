#include "Connect.h"

void Connect::read_conf()
{
	fstream f;
	f.open("./conf.xml");
	Serv_info data;

	while((f>>data.server_ip) && (f>>data.server_port))
		serv_info_ls.push_back(data);
	f.close();
}

bool Connect::con_serv()
{
	read_conf();

	list<Serv_info>::iterator ite = serv_info_ls.begin();
	int count = 0;
	int i = 0;

	while(ite != serv_info_ls.end())
	{
		struct sockaddr_in caddr;
		bzero(&caddr,sizeof(caddr));
		caddr.sin_family = AF_INET;
		caddr.sin_addr.s_addr = inet_addr(ite->server_ip);
		caddr.sin_port = htons(ite->server_port);

		int ToServfd = socket(AF_INET,SOCK_STREAM,0);
		if(ToServfd == -1)
		{
			close(ToServfd);
			return false;
		}
		for(int w=0;w<3;w++)
		{
			mem.num[w] = 0;
		}
		mem.sfd[i++] = ToServfd;
		int ret = connect(ToServfd,(struct sockaddr*)&caddr,sizeof(caddr));
		if(ret == -1)
		{
			close(ToServfd);
			return false;
		}
		else
		{
			cout<<"连接成功"<<endl;
			//将处理服务器fd和当前连接数量存入共享内存
			mem.mMap(mem.sfd,mem.num);
		}
		++ite;
	}
	//服务器的ip和port链表不再需要 可以清空
	serv_info_ls.clear();

	return true;
}

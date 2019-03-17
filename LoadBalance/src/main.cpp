#include "Connect.h"
#include "ShareMemory.h"
#include "ProcessPool.h"



int main(void)
{
	//1.连接到处理服务器
	Connect con;
	con.con_serv();
	
	struct sockaddr_in saddr;
	bzero(&saddr,sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	saddr.sin_port = htons(8000);

	int LoadBalancefd = socket(AF_INET,SOCK_STREAM,0);
	assert(LoadBalancefd != -1);

	int ret = bind(LoadBalancefd,(struct sockaddr*)&saddr,sizeof(saddr));
	if(ret == -1)
	{
		cout<<"LoadBalancefd 绑定失败..."<<endl;
	}
	listen(LoadBalancefd,128);
	//2.创建子进程
	ProcessPool* pool = ProcessPool::create(LoadBalancefd,3);
	if(pool)
	{
		pool->run();
		delete pool;
	}
	//	ShareMemory mem;
	//	mem.mMap(10,0);
	close(LoadBalancefd);
	return 0;
}

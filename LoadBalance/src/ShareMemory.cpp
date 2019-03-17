#include "ShareMemory.h"

void ShareMemory::mMap(int* sfd,int* num)
{
	int fd = open("../ShareMemory.cpp",O_CREAT|O_RDWR|O_TRUNC,0664);

	lseek(fd,sizeof(Serv_load)*3,SEEK_SET);

	write(fd,"",1);

	Serv_load* p_map = (Serv_load*)mmap(NULL,sizeof(Serv_load),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);

	close(fd);


	for(int i=0;i<3;++i)
	{
		(p_map+i)->serv_fd = sfd[i];
		(p_map+i)->con_num = num[i];
	}

/*	for(int i=0;i<3;++i)
	{
		if((p_map+i)->serv_fd == 0)
		{
			(p_map+i)->con_num = 2147483647;
		}
		//cout<<"sfd["<<(*(p_map+i)).serv_fd<<"]  num["<<(*(p_map+i)).con_num<<"]"<<endl;
	}
*/
	munmap(p_map,sizeof(Serv_load)*3);
}

list<Serv_load> ShareMemory::GetMap()
{
	int fd = open("../ShareMemory.cpp",O_CREAT|O_RDWR,0664);
	Serv_load* p_map = (Serv_load*)mmap(NULL,sizeof(Serv_load),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	
	for(int i =0;i<3;i++)
	{
		//if((*(p_map+i)).serv_fd < 100)
		//	cout<<"sfd["<<(*(p_map+i)).serv_fd<<"]  num["<<(*(p_map+i)).con_num<<"]"<<endl;
		Serv_load sc;
		sc.serv_fd = (*(p_map+i)).serv_fd;
		sc.con_num = (*(p_map+i)).con_num;
		Map.push_back(sc);
	}
	return Map; 
}

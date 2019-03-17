#include "ProcessPool.h"

ProcessPool* ProcessPool::m_instance = NULL;
static int sig_pipefd[2];

ProcessPool::ProcessPool(int listenfd,int process_num):m_listenfd(listenfd),m_process_num(process_num),m_fork_id(-1),m_stop(true)
{
	servflag = false;
	if(process_num <=0 || process_num > MAX_PNUM)
		cout<<"进程数量越界"<<endl;

	m_sub_process = new Process[process_num];

	for(int i=0;i<process_num;++i)
	{
		int ret = socketpair(PF_UNIX,SOCK_STREAM,0,m_sub_process[i].m_pipefd);
		assert(ret != -1);

		m_sub_process[i].m_pid = fork();
		assert(m_sub_process[i].m_pid >= 0);

		if(m_sub_process[i].m_pid > 0)
		{
			//父进程关闭读端
			close(m_sub_process[i].m_pipefd[1]);
		}
		else
		{
			//子进程关闭写端
			close(m_sub_process[i].m_pipefd[0]);
			m_fork_id = i;
		//	break;
		}
	}
}

ProcessPool::~ProcessPool()
{
	free(m_sub_process);
}

void ProcessPool::setnonblocking(int fd)
{
	int old_option = fcntl(fd,F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd,F_SETFL,new_option);
}

void ProcessPool::addfd(int epollfd,int fd)
{
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;
	epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
	setnonblocking(fd);
}

void ProcessPool::removefd(int epollfd,int fd)
{
	epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,0);
	close(fd);
}


void ProcessPool::run()
{
	if(m_fork_id != -1)
	{
		run_child();
		return;
	}
	run_parent();
}


void ProcessPool::sig_handler(int sig)
{
	int save_errno = errno;
	int msg = sig;
	send(sig_pipefd[1],(char*)&msg,1,0);
	errno = save_errno;
}

void ProcessPool::addsig(int sig,void(handler)(int),bool restart)
{
	struct sigaction sa;
	memset(&sa,'\0',sizeof(sa));
	sa.sa_handler = handler;
	if(restart)
	{
		sa.sa_flags |= SA_RESTART;
	}
	sigfillset(&sa.sa_mask);
	assert(sigaction(sig,&sa,NULL) != -1);
}

void ProcessPool::setup_sig_pipe()
{
	m_epollfd = epoll_create(10000);
	assert(m_epollfd != -1);

	int ret = socketpair(PF_UNIX,SOCK_STREAM,0,sig_pipefd);
	assert(ret != -1);

	setnonblocking(sig_pipefd[1]);
	addfd(m_epollfd,sig_pipefd[0]);

	addsig(SIGCHLD,sig_handler);
	addsig(SIGTERM,sig_handler);
	//addsig(SIGINT,sig_handler);
	addsig(SIGPIPE,SIG_IGN);
}

void ProcessPool::run_child()
{
	//树2 ： 匿名管道 客户端socket 服务器socket
	setup_sig_pipe();
	epoll_event events[MAX_EVENT_NUMBER];
	int pipefd = m_sub_process[m_fork_id].m_pipefd[1];
	addfd(m_epollfd,pipefd);
	setnonblocking(m_listenfd);
	ShareMemory mem;
	list<Serv_load> m_Map = mem.GetMap();
	list<Serv_load>::iterator itemap = m_Map.begin();
	bool servflag = true;

	while(itemap != m_Map.end())
	{

		if((itemap->serv_fd) != 0)
		{
			addfd(m_epollfd,itemap->serv_fd);
			//cout<<itemap->serv_fd<<endl;
		}
		++itemap;
	}


	while(m_stop)
	{
		int num = epoll_wait(m_epollfd,events,MAX_EVENT_NUMBER,-1);
		if((num < 0) && (errno == EINTR))
		{
			cout<<"epoll 等待事件失败..."<<endl;
			break;
		}
		for(int i=0;i<num;++i)
		{	
			int retfd = events[i].data.fd;
			if((retfd == pipefd) && (events[i].events & EPOLLIN))
			{
				struct sockaddr_in caddr;
				socklen_t len = sizeof(caddr);

				int cfd = accept(m_listenfd,(struct sockaddr*)&caddr,&len);
				if(cfd < 0)
				{
					removefd(m_epollfd,cfd);
					close(cfd);
					cout<<"客户端连接失败"<<endl;
					continue;		
				}

				cout<<"客户端连接成功"<<endl;
				addfd(m_epollfd,cfd);

				User u;
				u.cfd = cfd;

				//最小连接数
				ShareMemory mem;
				list<Serv_load> m_Map = mem.GetMap();

				list<Serv_load>::iterator itemap = m_Map.begin();
				int mincon = 2147483647;
				int minsock = -1;
				while(itemap != m_Map.end())
				{
					if(mincon > itemap->con_num)
					{
						mincon = itemap->con_num;
						minsock = itemap->serv_fd;
					}
					++itemap;
				}
				u.sfd = minsock;
				user.push_back(u);

				if(minsock == 3)
					mem.mMap(minsock,++mincon,0);
				else if(minsock == 4)
					mem.mMap(minsock,++mincon,1);
				else if(minsock == 5)
					mem.mMap(minsock,++mincon,2);

				//			cout<<"mincon:"<<mincon<<endl;
				//			cout<<"minsock:"<<minsock<<endl;

			}
			else if(events[i].events & EPOLLIN)
			{
				ShareMemory mem;
				list<Serv_load> m_Map = mem.GetMap();
				list<Serv_load>::iterator itemap = m_Map.begin();
				list<Serv_load>::iterator Ismap = m_Map.begin();
				while(Ismap != m_Map.end())
				{
					//		cout<<"itemap->serv_fd:"<<itemap->serv_fd<<endl;
					//		cout<<"retfd:"<<retfd<<endl;
					if(retfd != 0 && (retfd != Ismap->serv_fd))
					{
						servflag = false;

						char* buf = NULL;
						int nlen;
						int packsize;
						int RealReadNum = recv(retfd,(char*)&packsize,sizeof(int),0);
						nlen = packsize;
						list<User>::iterator ite = user.begin();
						if(RealReadNum < 0)
							continue;
						if(RealReadNum == 0)
						{
							//list<Serv_load> m_Map = mem.GetMap();
							/*list<Serv_load>::iterator*/ itemap = m_Map.begin();

							while(ite != user.end())
							{
								if(ite->cfd == retfd)
								{
									break;
								}
								++ite;
							}

							if(ite->sfd == 3)
								mem.mMap(ite->sfd,--itemap->con_num,0);
							else if(ite->sfd == 4)
								mem.mMap(ite->sfd,--(itemap++)->con_num,1);
							else if(ite->sfd == 5)
								mem.mMap(ite->sfd,--((itemap++)++)->con_num,2);

							removefd(m_epollfd,retfd);
							close(retfd);
						}
						else
						{
							int noffset = 0;
							cout<<m_fork_id<<"client.packsize:"<<packsize<<endl;
							buf = new char[packsize];
							bzero(buf,packsize);
							while(packsize)
							{
								RealReadNum = recv(retfd,buf+noffset,packsize,0);
								noffset += RealReadNum;
								packsize -= RealReadNum;
							}
							cout<<"client.buf:"<<buf<<endl;
								ite = user.begin();
								while(ite != user.end())
								{
								if(ite->cfd == retfd)
								{
									break;
								}
								++ite;
								}
								send(ite->sfd,(char*)&nlen,sizeof(int),0);
								send(ite->sfd,buf,nlen,0);
								delete[] buf;
							 
						}
					}
					++Ismap;
				}
/*
				if(servflag)
				{
					cout<<"servflag is true"<<endl;
					//由服务器发来的buf
					char* buf = NULL;
					int nlen;
					int packsize;
					int RealReadNum = recv(retfd,(char*)&packsize,sizeof(int),0);
					nlen = packsize;
					if(RealReadNum < 0)
						continue;
					if(RealReadNum == 0)
					{
						cout<<"服务器宕机"<<endl;
					}
					else
					{
						cout<<"packsize:"<<packsize<<endl;
						int noffset = 0;
						buf = new char[packsize];
						bzero(buf,packsize);
						while(packsize)
						{
							RealReadNum = recv(retfd,buf+noffset,packsize,0);
							noffset += RealReadNum;
							packsize -= RealReadNum;

						}
						cout<<"server.buf:"<<buf<<endl;
						delete[] buf;
					}
					servflag = true;
				}
*/
			}
			else
			{
				continue;
			}

		}
	}
	cout<<m_stop<<endl;
}
void ProcessPool::run_parent()
{
	//树1 ： 监控信号通信管道和负载均衡Sokcet
	setup_sig_pipe();
	addfd(m_epollfd,m_listenfd);//LoadBalancefd

	int new_conn = 1;
	int sub_process_counter = 0;

	epoll_event events[MAX_EVENT_NUMBER];

	while(m_stop)
	{
		int num = epoll_wait(m_epollfd,events,MAX_EVENT_NUMBER,-1);
		if((num < 0) && (errno == EINTR))
		{
			cout<<"epoll监听的事件为0..."<<endl;
			break;  
		}

		for(int i=0;i<num;++i)
		{
			int retfd = events[i].data.fd;

			if(retfd == m_listenfd)
			{
				//通知子进程有新的连接请求
				int i = sub_process_counter;
				do
				{
					if(m_sub_process[i].m_pid != -1)
					{
						break;
					}
					i = (i+1)&m_process_num;
				}while(i != sub_process_counter);

				if(m_sub_process[i].m_pid == -1)
				{
					m_stop = false;
					break;
				}
				//轮寻

				//最小连接

				sub_process_counter = (i+1)&m_process_num;
				if(i >= m_process_num)
				{
					i = i % m_process_num;
					sub_process_counter = 1;
				}

				send(m_sub_process[i].m_pipefd[0],(char*)&new_conn,sizeof(new_conn),0);
				cout<<"sub_process_counter:"<<i<<endl;
			}
			else
			{
				continue;
			}
		}
	}

}


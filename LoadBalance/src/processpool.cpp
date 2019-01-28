#include "processpool.h"

list<User> user;
processpool* processpool::m_instance = NULL;

static int sig_pipefd[2];

static int setnonblocking( int fd )
{
	int old_option = fcntl( fd, F_GETFL );
	int new_option = old_option | O_NONBLOCK;
	fcntl( fd, F_SETFL, new_option );
	return old_option;
}

void log_info(int err_info,time_t time,int level = 0,int downtime_sfd = -1)
{
	//FILE f;
	fstream f;
	f.open("./log.xml");
	Log log;
//	log.ip = ip;
//	log.port = port;
	log.time = time;
	log.level = level;
	log.err_info = err_info;
	log.downtime_sfd = downtime_sfd;
//	f<<log.ip<<endl;
//	f<<log.port<<endl;
	tm *pTmp = localtime(&log.time);
	char nowtime[64];
	sprintf(nowtime,"%d-%d-%d %d:%d:%d",pTmp->tm_year +1900,pTmp->tm_mon+1,pTmp->tm_mday,pTmp->tm_hour,pTmp->tm_min,pTmp->tm_sec);
	f<<"错误产生时间:"<<nowtime<<endl;
	if(log.level == 1)
	{
		f<<"错误严重等级:严重"<<endl;
	}
	else if(log.level == 0)
	{
		f<<"错误严重等级:轻微"<<endl;
	}
	if(log.err_info == 1)
		f<<"错误信息:服务器宕机"<<endl;
	else if(log.err_info == -1)
		f<<"错误信息:缓冲区错误"<<endl;
	f<<"宕机服务器sock id:"<<log.downtime_sfd<<endl;
	f<<"\n"<<endl;
	f.close();
}

static void addfd( int epollfd, int fd )
{
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;
	epoll_ctl( epollfd, EPOLL_CTL_ADD, fd, &event );
	setnonblocking( fd );
}

static void removefd( int epollfd, int fd )
{
	epoll_ctl( epollfd, EPOLL_CTL_DEL, fd, 0 );
	close( fd );
}

static void sig_handler( int sig )
{
	Log log;
	int save_errno = errno;
	int msg = sig;
	send( sig_pipefd[1], ( char* )&msg, 1, 0 );
	errno = save_errno;
	//log.err_info = errno;
	//log(log.err_info);
}

static void addsig( int sig, void( handler )(int), bool restart = true )
{
	struct sigaction sa;
	memset( &sa, '\0', sizeof( sa ) );
	sa.sa_handler = handler;
	if( restart )
	{
		sa.sa_flags |= SA_RESTART;
	}
	sigfillset( &sa.sa_mask );
	assert( sigaction( sig, &sa, NULL ) != -1 );
}


processpool::processpool(int listenfd,int process_number ):m_listenfd(listenfd),m_process_number(process_number),m_idx(-1),m_stop(false)
{
	assert( ( process_number > 0 ) && ( process_number <= MAX_PROCESS_NUMBER ) );

	m_sub_process = new process[ process_number ];
	assert( m_sub_process );

	for( int i = 0; i < process_number; ++i )
	{
		int ret = socketpair( PF_UNIX, SOCK_STREAM, 0, m_sub_process[i].m_pipefd );
		assert( ret == 0 );

		m_sub_process[i].m_pid = fork();
		assert( m_sub_process[i].m_pid >= 0 );
		if( m_sub_process[i].m_pid > 0 )
		{
			close( m_sub_process[i].m_pipefd[1] );
			continue;
		}
		else
		{
			close( m_sub_process[i].m_pipefd[0] );
			m_idx = i;
			break;
		}
	}
}

void processpool::run()
{
	if(m_idx != -1)
	{
		run_child();
		return;
	}
	run_parent();
}

static void deal_child(int sig)
{
	for(;;)
	{
		if(waitpid(-1,NULL,WNOHANG) == 0)
			break;
	}
}

void processpool::setup_sig_pipe()
{
	m_epollfd = epoll_create( 5 );
	assert( m_epollfd != -1 );

	int ret = socketpair( PF_UNIX, SOCK_STREAM, 0, sig_pipefd );
	assert( ret != -1 );

	setnonblocking( sig_pipefd[1] );
	addfd( m_epollfd, sig_pipefd[0] );
	addfd(m_epollfd, socketfd);

	addsig( SIGCHLD, sig_handler );
	addsig( SIGTERM, sig_handler );
	addsig( SIGINT, sig_handler );

	addsig( SIGPIPE, SIG_IGN );
}

void processpool::run_child()
{
	//cout<<"run_child"<<endl;
	setup_sig_pipe();
	int pipefd = m_sub_process[m_idx].m_pipefd[1];
	addfd(m_epollfd,pipefd);

	epoll_event events[MAX_EVENT_NUMBER];
	list<Map>::iterator it = m.begin();
	list<Conum>::iterator ite_con = con.begin();

	while(!m_stop)
	{
		int num = epoll_wait(m_epollfd,events,MAX_EVENT_NUMBER,-1);
		ite_con = con.begin();
		if((num<0)&&(errno == EINTR))
		{
			cout<<"epoll_wait error"<<endl;
			break;
		}

		for(int i = 0 ; i < num ; ++i)
		{
			int retfd = events[i].data.fd;
			if((retfd == pipefd) && (events[i].events & EPOLLIN))
			{
				int client = 0;
				int ret = recv(retfd,(char*)&client,sizeof(client),0);
				if(((ret <0)&&(errno == EAGAIN))|| ret == 0)
				{
					continue;
				}else
				{
					struct sockaddr_in caddr;
					socklen_t len = sizeof(caddr);
					int c = accept(m_listenfd,(struct sockaddr*)&caddr,&len);
					if(c < 0)
					{
						cout<<"accept error errno:"<<errno<<endl;
						continue;
					}

					addfd(m_epollfd,c);

					User u = {caddr,c,0};

					int minconser = 2147483647;
					int min_sock;
					while(ite_con != con.end())
					{
						//cout<<"sock"<<ite_con->sock<<"当前连接数量"<<ite_con->connum<<endl;
						if(minconser < ite_con->connum)
						{
							//ite_con++;
							//continue;
						}
						else
						{
							minconser = ite_con->connum;
							min_sock = ite_con->sock;
						}
						ite_con++;
					}
					if(it == m.end())
						it = m.begin();
					//u.servfd = it->sock;
					u.servfd = min_sock;

					ite_con = con.begin();
					while(ite_con != con.end())
					{
						if(ite_con->sock == u.servfd)
						{
							ite_con->connum++;
							//cout<<ite_con->sock<<"de连接数量增加"<<endl;
						}
						++ite_con;
					}
					++it;
					user.push_back(u);
				}
			}
			//服务器
			else if((retfd == socketfd) && events[i].events == EPOLLIN)
			{
				char buff[BUFF_SIZE]={0};
				int ret = recv(retfd,buff,BUFF_SIZE,0);
		//		cout<<"ret:"<<ret<<endl;
		//		cout<<"server  "<<buff<<endl;
				if(ret <0)
				{
					if((errno == EAGAIN) || (errno == EWOULDBLOCK))
					{
						continue;
					}
					//close(retfd);
				}
				else if(ret == 0)
				{
					list<User>::iterator it_use =  user.begin();	
					Log log;
					log.downtime_sfd = retfd;
					log.err_info = 1;
					time_t now_time;
					now_time = time(NULL);
					log.level = 1;
					log_info(log.err_info,now_time,log.level,retfd);

					cout<<"服务器异常中断"<<endl;
					list<Conum>::iterator ite_con = con.begin();
					int min_sfd = 2147483647;
					int min = 2147483647;
					it = m.begin();
					while(it != m.end())
					{
						if(it->sock == retfd)
						{
							m.erase(it);
							break;
						}
						++it;
					}
					while(it_use != user.end())
					{
						if(retfd == it_use->servfd)
						{
							while(ite_con != con.end())
							{
								if(it_use->servfd == retfd)
								{
									++it_use;
									continue;
								}

								if(ite_con->connum < min)
								{
									min = ite_con->connum;
									min_sfd = ite_con->sock;
								}
								++ite_con;
							}
							it_use->servfd = min_sfd;
						}
						++it_use;
					}
					ite_con = con.begin();
					while(ite_con != con.end())
					{
						if(retfd == ite_con->sock)
						{
							con.erase(ite_con);
							break;
						}
						++ite_con;
					}
					close(retfd);
				}	
				else
				{
					list<User>::iterator it_use =  user.begin();
					for(;it_use != user.end();++it_use)
					{
						if(it_use->servfd == retfd)
						{	
							send(it_use->servfd,"YES\n",4,0);
							send(it_use->connfd,buff,BUFF_SIZE,0);
						}
					}
				}
			}
			//客户端
			else if((retfd != socketfd) && events[i].events == EPOLLIN)
			{
				char buff[BUFF_SIZE]={0};
				int ret = recv(retfd,buff,BUFF_SIZE,0);
				//cout<<"ret"<<ret<<endl;
				cout<<"son"<<" "<<buff<<endl;
				if(ret < 0)
				{
					if((errno == EAGAIN)||(errno ==EWOULDBLOCK))
					{
						break;
					}
					close(retfd);
					break;
				}
				else if(ret == 0)
				{
					//客户端主动断开
					ite_con = con.begin();
					while(ite_con != con.end())
					{
						list<User>::iterator user_ite = user.begin();
						while(user_ite != user.end())
						{
							if(user_ite->connfd == retfd)
							{
								if(user_ite->servfd == ite_con->sock)
								{
									ite_con->connum--;
									//	cout<<ite_con->sock<<"de连接数量减少成功"<<endl;
									user.erase(user_ite);

									goto a;
								}
							}

							user_ite++;
						}
						ite_con++;
					}
a:
					close(retfd);
				}
				else
				{
					list<User>::iterator it_use =  user.begin();
					for(;it_use != user.end();++it_use)
					{
						cout<<"it_use->servfd"<<it_use->servfd<<endl;
						if(it_use->connfd == retfd)
						{
							send(it_use->connfd,"YES\n",4,0);
							int nlen = send(it_use->servfd,buff,BUFF_SIZE,0);
							/*	if(nlen <= 0)
								{
								cout<<"send:0"<<endl;
								ite_con = con.begin();
								int min_sfd;
								while(ite_con != con.end())
								{
								int min = 21473647;
								if(ite_con->connum < min)
								{
								min = ite_con->connum;
								ite_con->sock = min_sfd;
								}
								cout<<"min_con:"<<min<<endl;
								++ite_con;
								}
								it_use->servfd = min_sfd;

								}
							 */
						}
					}
				}

			}
			else
			{
				continue;
			}
		}

	}
	close(pipefd);
	close(m_epollfd);

}

void processpool::run_parent()
{
	//cout<<"run_parent"<<endl;
	setup_sig_pipe();
	signal(SIGCHLD,deal_child);
	addfd(m_epollfd,m_listenfd);
	epoll_event events[MAX_EVENT_NUMBER];

	int sub_process_counter = 0;
	int new_conn = 1;

	while(!m_stop)
	{
		int num = epoll_wait(m_epollfd,events,MAX_EVENT_NUMBER,-1);
		if((num < 0)&&(errno == EINTR ) )
		{
			cout<<"epoll error "<<endl;
			break;
			//continue;
		}

		for(int i=0;i<num ;++i)
		{
			int retfd = events[i].data.fd;
			if(retfd == m_listenfd)
			{
				int i = sub_process_counter;
				do {
					if(m_sub_process[i].m_pid != -1)
					{
						break;
					}
					i = (i+1)&m_process_number;
				} while (i!= sub_process_counter);

				if(m_sub_process[i].m_pid == -1)
				{
					m_stop = true;
					break;
				}
				sub_process_counter = (i+1)&m_process_number;
				send(m_sub_process[i].m_pipefd[0],(char*)&new_conn,sizeof(new_conn),0);
				//cout<<"send request to child :"<<i<<endl;
			}
			else
			{
				continue;
			}

		}
	}

}

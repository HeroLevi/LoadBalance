#ifndef _PROCESSPOOL_H
#define _PROCESSPOOL_H

#include <iostream>
//#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <list>
#include <signal.h>
#include "ShareMemory.h"
#include "Memorypool.h"
#include <pthread.h>
using namespace std;

#define MAX_PNUM 16
#define MAX_EVENT_NUMBER 10000
class Process
{
	public:
		Process():m_pid(-1){}
	public:
		pid_t m_pid;
		int m_pipefd[2];
};

class ProcessPool
{
	public:
		static ProcessPool* m_instance;
		Process* m_sub_process;
		int m_fork_id;
		bool m_stop;
		int m_listenfd;
		int m_process_num;
		int m_epollfd;
		list<User> user;
		bool servflag;
		pthread_mutex_t mem_lock;

	private:
		ProcessPool(int listenfd,int process_num = 8);
	public:
		~ProcessPool();
	public:
		void setup_sig_pipe();
		void setnonblocking(int fd);
		void addfd(int epollfd,int fd);
		void removefd(int epollfd,int fd);
		void addsig(int sig,void(handler)(int),bool restart = true);
		static void sig_handler(int sig);

		void run();
		void run_child();
		void run_parent();
	public:
		static ProcessPool* create(int listenfd,int process_num = 8)
		{
			if(!m_instance)
			{
				m_instance = new ProcessPool(listenfd,process_num);
			}
			return m_instance;
		}
		
};

#endif

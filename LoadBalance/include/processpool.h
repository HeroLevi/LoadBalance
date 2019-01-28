#ifndef _PROSESSPOOL_H
#define _PROSESSPOOL_H
#include <iostream>
#include <stdio.h>
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
#include <unistd.h>
#include <fstream>

#include "struct.h"
#include "process.h"

#define MAX_EVENTS 1024
#define BUFF_SIZE 128

using namespace std;

extern list<Data> conf_list;
extern list<Map> m;
extern list<Conum> con;
extern int socketfd;

class processpool
{
	private:
		processpool(int listenfd,int process_num = 8);
	public:
		static processpool* create(int listenfd,int process_num = 8)
		{
			if(!m_instance)
			{
				m_instance = new processpool(listenfd,process_num);
			}
			return m_instance;
		}

		~processpool()
		{
			delete [] m_sub_process;
		}

		void run();
	private:
		void setup_sig_pipe();
		void run_child();
		void run_parent();

	private:
		static const int MAX_PROCESS_NUMBER = 16;
		static const int USER_PER_PROCESS = 65536;
		static const int MAX_EVENT_NUMBER = 10000;
		int m_process_number;
		int m_idx;
		int m_epollfd;
		int m_listenfd;
		int m_stop;
		process* m_sub_process;
		static processpool* m_instance;

};
#endif

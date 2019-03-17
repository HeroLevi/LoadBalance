#ifndef _CONNECT_H
#define _CONNECT_H

#include <list>
#include <iostream>
#include <fstream>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include "Struct.h"
#include "ShareMemory.h"

using namespace std;

class Connect
{
	public:
		list<Serv_info> serv_info_ls;
		ShareMemory mem;
	public:
		void read_conf();
		bool con_serv();
};
#endif

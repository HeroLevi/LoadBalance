#ifndef _SHAREMEMORY_H
#define _SHAREMEMORY_H

#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "Struct.h"
#include <string.h>
#include <iostream>
#include <list>
#include <iterator>
using namespace std;


class ShareMemory
{
	public:
		list<Serv_load> Map;
		int sfd[3];
		int num[3];
	public:
		void mMap(int*,int*);
		list<Serv_load> GetMap();
};

#endif

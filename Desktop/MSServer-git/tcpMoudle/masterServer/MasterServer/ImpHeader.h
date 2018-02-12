#ifndef IMPHEADER_H_INCLUDED
#define IMPHEADER_H_INCLUDED


#include<string>
#include<iostream>
#include<sstream>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <vector>
#include <cstring>
#include <arpa/inet.h>
#include <pthread.h>
#include <event.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/thread.h>
#include <google/protobuf/message.h>
#include <malloc.h>

#define BufSize 8192
#define MoudleSize 30

extern int errno;

typedef unsigned int CLIENTIDTYPE;

enum LogicMoudleInd
{
   LGModuleLM = 0,
   LGModuleLS = 1,
};

enum DataMoudleInd
{
   DTModuleLM = 0,
   DTModuleLS = 1,
};

#endif // IMPHEADER_H_INCLUDED

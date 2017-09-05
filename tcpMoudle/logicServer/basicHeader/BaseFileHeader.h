#ifndef BASEFILEHEADER_H_INCLUDED
#define BASEFILEHEADER_H_INCLUDED

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
#include <map>
#include <queue>
#include <cstring>
#include <arpa/inet.h>
#include <pthread.h>
#include <event.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/thread.h>
#include <google/protobuf/message.h>

#define BufSize 8192
#define ConnSize 20
#define SendDataSize 1024

extern int errno;

typedef unsigned int CLIENTIDTYPE;

typedef unsigned int MOUDLEID;

typedef ::google::protobuf::Message PROTOMESSAGE;

#endif // BASEFILEHEADER_H_INCLUDED

#ifndef MASTERSERVER_H
#define MASTERSERVER_H

#include "SlaveServer.h"

class MasterServer;

class SlaveConn
{
    public:
        SlaveConn( int ind, int fd )
        {
            index = ind;
            notifyFd = fd;
            bufCtx = new BufCtx();
            eventState = 0;
        }

    public:
        int index;
        int notifyFd;
        int eventState;
        struct event notifyEevnt;
        BufCtx * bufCtx;
        SlaveServer * slaveServer;
        MasterServer * masterServer;
};

class MasterServer
{
    public:
        MasterServer();
        virtual ~MasterServer();
        void initServer(std::string mIp, int mPort);
        static void listenEventCallBack( int fd, short eventType, void * ctx );
        static void notifyWriteEventCallBack( int fd, short eventType, void * ctx );
        virtual void onListenEvent();
        virtual void onNotifyEvent( SlaveConn * conn );
        void setNotifyEvent( SlaveConn * conn );
    private:
        int dispatchId;
        int slaveNum;
        int listenFd;
        struct event_base * eventBase;
        struct event listenEvent;
        std::vector<SlaveConn *> slaveVec;
};

#endif // MASTERSERVER_H

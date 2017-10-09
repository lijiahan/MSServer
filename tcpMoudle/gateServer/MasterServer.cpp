#include "MasterServer.h"

extern const int NotifyCtxSize;

MasterServer::MasterServer()
{
    //ctor
    dispatchId = 0;
    slaveNum = 3;
}

MasterServer::~MasterServer()
{
    //dtor
    for( int i=0; i<slaveVec.size(); i++)
    {
        SlaveConn * slaveConn = slaveVec[i];
        close(slaveConn->notifyFd);
        delete slaveConn->bufCtx;
        delete slaveConn->slaveServer;
        delete slaveConn;
    }

    event_base_free(eventBase);
}

void MasterServer::initServer(std::string mIp, int mPort)
{
    //
    std::string lserveIp = "192.168.75.128";
    int lservePort = 8888;

    //
    listenFd = socket(AF_INET, SOCK_STREAM, 0);
    evutil_make_socket_nonblocking(listenFd);

    int flags = 1;
    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, (void *)&flags, sizeof(flags));
    setsockopt(listenFd, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags));
    setsockopt(listenFd, SOL_SOCKET, SO_LINGER, (void *)&flags, sizeof(flags));
    setsockopt(listenFd, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags));

    struct sockaddr_in listen_addr;
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(mPort);

    if( inet_pton(AF_INET, mIp.c_str(), &listen_addr.sin_addr ) < 1)
    {
        perror(" inet_pton error \n");
        exit(0);
    }

    if( bind(listenFd, (struct sockaddr *) &listen_addr, sizeof(listen_addr)) < 0)
    {
        perror(" bind error \n");
        exit(0);
    }

    if(listen( listenFd, 30 ) < 0)
    {
        perror(" listen error \n");
        exit(0);
    }

    eventBase = event_base_new();

    //

    event_assign( &listenEvent, eventBase, listenFd, EV_READ|EV_PERSIST, listenEventCallBack, (void *)this );
    event_add( &listenEvent, 0 );
    //
    slaveVec.reserve(slaveNum);
    for(int i=0; i<slaveNum; i++)
    {
        printf("start slave thread %d\n",i);
        SlaveServer * slave = new SlaveServer(i);
        slave->initialServer(lserveIp, lservePort);
        int fd = slave->getNotifyFd();

        SlaveConn * slaveConn = new SlaveConn(i, fd);
        slaveConn->slaveServer = slave;
        slaveConn->masterServer = this;

        slaveVec[i] = slaveConn;
    }

    event_base_dispatch(eventBase);

    for( int i =0; i<slaveNum; i++ )
    {
        pthread_join( slaveVec[i]->slaveServer->getThreadId(), NULL);
    }
}

void MasterServer::listenEventCallBack( int fd, short eventType, void * ctx )
{
    MasterServer * mServer = (MasterServer *) ctx;
    mServer->onListenEvent();
}

void MasterServer::notifyWriteEventCallBack( int fd, short eventType, void * ctx )
{
    SlaveConn * slaveConn = (SlaveConn *) ctx;
    //
    if((eventType & EV_PERSIST) == 0)
    {
        slaveConn->eventState &= (~WriteEventState);
    }

    slaveConn->masterServer->onNotifyEvent(slaveConn);
}

void MasterServer::onListenEvent()
{
    struct sockaddr_in clientAddr;
    socklen_t addrSize = sizeof(struct sockaddr_in);

    int clientFd = accept(listenFd, (struct sockaddr *)&clientAddr, &addrSize);
    if(clientFd != -1)
    {
        evutil_make_socket_nonblocking(clientFd);
    }

    int ind = dispatchId;
    dispatchId = (dispatchId+1) % slaveNum;

    SlaveConn * slaveConn = slaveVec[ind];

    printf("notify slave id: %d, the notifyFd: %d \n", dispatchId, slaveConn->notifyFd );

    BufCtx * bufCtx = slaveConn->bufCtx;
    int restLen = BufSize - bufCtx->bufLen;

    if(restLen < NotifyCtxSize)
    {
        printf("dispatchId: %d error! buf is full!! \n",dispatchId);
        return;
    }

    char * buf = (bufCtx->buf+bufCtx->bufLen);
    //
    NotifyCtx * notify = (NotifyCtx *) buf;
    notify->notifyType = NotifyConnect;
    notify->fd = clientFd;
    bufCtx->bufLen += NotifyCtxSize;

    setNotifyEvent(slaveConn);
}

void MasterServer::onNotifyEvent( SlaveConn * conn )
{
    BufCtx * wbufCtx = conn->bufCtx;

    int nw = write(conn->notifyFd, wbufCtx->buf, wbufCtx->bufLen);

    printf("send notify fd, send wLen: %d, the buflen : %d\n", nw,wbufCtx->bufLen);

    if( nw == wbufCtx->bufLen )
    {
        wbufCtx->bufLen = 0;
    }
    else
    {
        int llen = wbufCtx->bufLen - nw;

        if(llen > 0)
        {
            memmove(wbufCtx->buf, (wbufCtx->buf+nw), llen);
            wbufCtx->bufLen = llen;
            setNotifyEvent(conn);
        }
        else
        {
            printf(" notify write  error %d ! \n", conn->index);
        }
    }
}

void MasterServer::setNotifyEvent( SlaveConn * conn )
{
    int state = conn->eventState & WriteEventState;
    if( state )
    {
        return;
    }

    event_assign(&(conn->notifyEevnt), eventBase, conn->notifyFd, EV_WRITE, notifyWriteEventCallBack, (void *) conn );
    if( event_add(&(conn->notifyEevnt), 0) == -1 )
    {
        perror(" can not add Write event  \n");
        exit(0);
    }

    conn->eventState |= WriteEventState;
}

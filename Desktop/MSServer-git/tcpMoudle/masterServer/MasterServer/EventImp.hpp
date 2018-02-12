#ifndef EVENTIMP_HPP_INCLUDED
#define EVENTIMP_HPP_INCLUDED

#include "ImpHeader.h"

class ServerEventImp;

enum ConnState
{
   ConnFreeState = 0,
   ConnUseState,
};

enum EventState
{
    EventReadState  = 0x001,
    EventWriteState = 0x002,
    EventTimerState = 0x004,
};

enum EventTimerId
{
    EventOutTimeId = 1,
};

struct CommMsg
{
    int moduleInd;
    int cmdInd;
    int connIndex;
    CLIENTIDTYPE uid;
    int msgLen;
    char data[];
};

struct BufCtx
{
    char buf[BufSize];
    int bufLen;
    BufCtx * next;
};

struct ConnectCtx
{
    CLIENTIDTYPE uid;
    int fd;
    int freeInd;
    int connState;
    int eventState;
    int outTimeNum;
    struct event readEevnt;
    struct event writeEevnt;
    struct event_base * eventBase;
    BufCtx * readBuf;
    BufCtx * writeBuf;
    //
    int entryInd;
    ServerEventImp * eventImp;
};

class BufCtxMgr
{
public:
    BufCtxMgr()
    {

    }

    ~BufCtxMgr()
    {
        for( uint32_t i=0; i<bufFreeVec.size(); i++)
        {
             BufCtx * bufCtx = bufFreeVec[i];
             free(bufCtx);
             bufFreeVec[i] = NULL;
        }

        bufFreeVec.clear();
    }

    BufCtx * getCtx()
    {
        BufCtx * bufCtx = NULL;

        if(bufFreeVec.size() > 0)
        {
            bufCtx = bufFreeVec.back();
            bufFreeVec.pop_back();
        }
        else
        {
            bufCtx = (BufCtx *) malloc(BufCtxSize);
            memset(bufCtx, 0, BufSize);
            bufCtx->next = NULL;
        }

        return bufCtx;
    }

    void freeCtx( BufCtx * bufCtx )
    {
        memset(bufCtx, 0, BufSize);
        bufCtx->next = NULL;
        bufFreeVec.push_back(bufCtx);
    }

private:
    std::vector<BufCtx *> bufFreeVec;
public:
    static const int BufCtxSize = sizeof(BufCtx);
};

class ConnectCtxMgr
{
public:
    ConnectCtxMgr()
    {
        bufCtxMgr = new BufCtxMgr();
    }

    ~ConnectCtxMgr()
    {
        for( uint32_t i=0; i<connFreeVec.size(); i++)
        {
             ConnectCtx * conn = connFreeVec[i];
             free(conn);
             connFreeVec[i] = NULL;
        }

        connFreeVec.clear();

        for( uint32_t i=0; i<connVec.size(); i++)
        {
            ConnectCtx * conn = connVec[i];
            if( conn != NULL )
            {
                if((conn->eventState) & EventReadState)
                {
                    event_del(&conn->readEevnt);
                }

                if((conn->eventState) & EventWriteState)
                {
                    event_del(&conn->writeEevnt);
                }

                BufCtx * bufCtx = conn->readBuf;

                while(bufCtx!= NULL)
                {
                    BufCtx * bufTemp = bufCtx->next;
                    free(bufCtx);
                    bufCtx = bufTemp;
                }

                bufCtx = conn->writeBuf;
                while(bufCtx!= NULL)
                {
                    BufCtx * bufTemp = bufCtx->next;
                    free(bufCtx);
                    bufCtx = bufTemp;
                }

                close(conn->fd);

                free(conn);
            }

            connVec[i] = NULL;
        }

        connVec.clear();

        delete bufCtxMgr;
    }

    ConnectCtx * getCtx()
    {
        ConnectCtx * conn = NULL;

        if(connFreeVec.size() > 0)
        {
            conn = connFreeVec.back();
            connFreeVec.pop_back();
            connVec[conn->freeInd] = conn;
        }
        else
        {
            conn = (ConnectCtx *) malloc(ConnCtxSize);
            memset(conn, 0, ConnCtxSize);
            conn->eventBase = NULL;
            conn->readBuf = NULL;
            conn->writeBuf = NULL;
            conn->eventImp = NULL;
            conn->freeInd = connVec.size();
            connVec.push_back(conn);
        }

        return conn;
    }

    ConnectCtx * getCtx( int ind )
    {
        if( connVec.size() > ind )
        {
            return connVec[ind];
        }
        else
        {
            printf("getCtx out ind !!\n");
        }

        return NULL;
    }

    void freeCtx( ConnectCtx * conn )
    {
        BufCtx * wbufCtx = conn->writeBuf;

        while(wbufCtx != NULL)
        {
            BufCtx * bufCtx = wbufCtx;
            wbufCtx = bufCtx->next;
            bufCtxMgr->freeCtx(bufCtx);
        }

        conn->writeBuf = NULL;

        BufCtx * rbufCtx = conn->readBuf;
        while(rbufCtx != NULL)
        {
            BufCtx * bufCtx = rbufCtx;
            rbufCtx = bufCtx->next;
            bufCtxMgr->freeCtx(bufCtx);
        }

        conn->readBuf = NULL;

        int ind = conn->freeInd;
        memset(conn, 0, ConnCtxSize);
        conn->eventBase = NULL;
        conn->readBuf = NULL;
        conn->writeBuf = NULL;
        conn->eventImp = NULL;
        conn->freeInd = ind;
        connVec[ind] = NULL;
        connFreeVec.push_back(conn);
    }

    BufCtx * getReadBuf(ConnectCtx * conn)
    {
        BufCtx * bufCtx = conn->readBuf;
        if( bufCtx == NULL )
        {
           bufCtx = bufCtxMgr->getCtx();
           return bufCtx;
        }

        return bufCtx;
    }

    BufCtx * getWriteBuf(ConnectCtx * conn, int wlen)
    {
        if( conn->writeBuf == NULL )
        {
            BufCtx * bufCtx = bufCtxMgr->getCtx();
            conn->writeBuf = bufCtx;
            return bufCtx;
        }
        else
        {
            BufCtx * bufCtx = conn->writeBuf;

            while(bufCtx->next != NULL)
            {
                bufCtx = bufCtx->next;
            }

            if(BufSize - bufCtx->bufLen < wlen)
            {
                bufCtx->next = bufCtxMgr->getCtx();
                bufCtx = bufCtx->next;
            }

            return bufCtx;
        }
    }

    void freeBufCtx(BufCtx * bufCtx)
    {
        bufCtxMgr->freeCtx(bufCtx);
    }

    void connError(int code, ConnectCtx * conn )
    {
        std::cout<<"connError code:"<<code<<std::endl;
        if( code < 0 )
        {
            if( errno == EWOULDBLOCK )
            {
                return;
            }
        }

        if((conn->eventState) & EventReadState)
        {
            event_del(&conn->readEevnt);
        }

        if((conn->eventState) & EventWriteState)
        {
            event_del(&conn->writeEevnt);
        }

        close(conn->fd);
        freeCtx(conn);
    }

private:
    BufCtxMgr * bufCtxMgr;
    std::vector<ConnectCtx *> connVec;
    std::vector<ConnectCtx *> connFreeVec;
public:
    static const int ConnCtxSize = sizeof(ConnectCtx);
};

class TimerEventHandler
{
public:
    TimerEventHandler();
    virtual ~TimerEventHandler();
    virtual void timerHandle();
    void handleError(ConnectCtx * conn);
public:
    int eventId;
    int rpNum;
    int sec;
    int usec;
    struct event timeEevnt;
    ServerEventImp * eventImp;
};

class ConnectOutTimeEvent : public TimerEventHandler
{
public:
    ~ConnectOutTimeEvent()
    {

    }

    virtual void timerHandle()
    {
        std::vector<ConnectCtx *>::iterator iter;
        for( iter = connVec.begin();iter != connVec.end(); )
        {
            ConnectCtx * conn = *iter;
            if(conn->connState == ConnFreeState)
            {
               iter = connVec.erase(iter);
            }
            else
            {
                conn->outTimeNum--;
                if(conn->outTimeNum <= 0)
                {
                    handleError(conn);
                    iter = connVec.erase(iter);
                }
                else
                {
                    iter++;
                }
            }
        }
    }

    void addConnInEvent(ConnectCtx * conn)
    {
        connVec.push_back(conn);
    }

public:
    std::vector<ConnectCtx *> connVec;
};

class ServerEventImp
{
public:
    ServerEventImp()
    {
        listenFd = -1;
        eventBase = event_base_new();
        connMgr = new ConnectCtxMgr();
    }

    virtual ~ServerEventImp()
    {
        if(listenFd != -1)
        {
            event_del( &listenEvent );
            close( listenFd );
            delete connectOTimeEvent;
        }

        std::vector<TimerEventHandler *>::iterator iter;
        for( iter = timerEventVec.begin();iter != timerEventVec.end(); iter++)
        {
            TimerEventHandler * handler = *iter;
            if(handler != NULL)
            {
                delete handler;
            }
        }

        event_base_free( eventBase );
        delete connMgr;
    }

    virtual void newEvent( ConnectCtx * conn )
    {

    }
    virtual void delEvent( ConnectCtx * conn )
    {

    }
    virtual void enterModule( ConnectCtx * conn, CommMsg * cmsg )
    {

    }

public:
      //
    static void listenEventCallBack( int fd, short eventType, void * ctx )
    {
        printf("connect one server!!\n");
        ServerEventImp * imp = (ServerEventImp *) ctx;
        imp->onListenEvent();
    }

    static void readEventCallBack( int fd, short eventType, void * ctx )
    {
        ConnectCtx * conn = (ConnectCtx *) ctx;
        conn->outTimeNum = 100;
        conn->eventImp->onReadEvent(conn);
    }

    static void writeEventCallBack( int fd, short eventType, void * ctx )
    {
        ConnectCtx * conn = (ConnectCtx *) ctx;
        //
        if((eventType & EV_PERSIST) == 0)
        {
            conn->eventState &= (~EventWriteState);
        }

        conn->eventImp->onWriteEvent(conn);
    }

    static void timerEventCallBack( int fd, short eventType, void * ctx )
    {
        TimerEventHandler * handler = (TimerEventHandler *) ctx;
        handler->eventImp->onTimerEvent(handler);
    }

public:
    virtual int readEventHandler( ConnectCtx * conn );
    virtual void handleError(int errCode, ConnectCtx * conn);
    virtual void onListenEvent();
    virtual void onReadEvent( ConnectCtx * conn );
    virtual void onWriteEvent(ConnectCtx * conn);
    virtual void onTimerEvent( TimerEventHandler * handler );
    virtual void addReadEvent( ConnectCtx * conn );
    virtual void addWriteEvent( ConnectCtx * conn );
    virtual int addTimerEvent(TimerEventHandler * handler);

public:
    //
    void initListen( std::string ip, int port );
    //
    void startServer();
    //
    void addConnectOutTimer( TimerEventHandler * eventHandle, int rpnum, int sec, int usec );
private:
    BufCtx * readData(ConnectCtx * conn, int & rLen);
    void readDataEnd(ConnectCtx * conn, BufCtx * bufCtx, int lLen);

public:
    static const int CommMsgLen = sizeof(CommMsg);

protected:
    int listenFd;
    struct event_base * eventBase;
    struct event listenEvent;
    ConnectCtxMgr * connMgr;
    ConnectOutTimeEvent * connectOTimeEvent;
    std::vector<TimerEventHandler *> timerEventVec;
};

TimerEventHandler::TimerEventHandler()
{
    rpNum = 0;
    sec = 0;
    usec = 0;
}

TimerEventHandler:: ~TimerEventHandler()
{
    eventImp = NULL;
    event_del(&timeEevnt);
}

void TimerEventHandler::timerHandle(){}

void TimerEventHandler::handleError(ConnectCtx * conn)
{
    //eventImp->handleError(0, conn);
}

int ServerEventImp::readEventHandler( ConnectCtx * conn )
{
    int rLen = 0;
    BufCtx * bufCtx = readData(conn, rLen);

    if(rLen <= 0)
    {
        connMgr->freeBufCtx(bufCtx);
        handleError(rLen, conn);
        return 0;
    }

    conn->outTimeNum = 100;
    int llen = bufCtx->bufLen;
    //
    if( llen > CommMsgLen )
    {
        char * msgbuf = bufCtx->buf;
        while( 1 )
        {
            CommMsg * cmsg = (CommMsg *) msgbuf;
            int tlen = llen - CommMsgLen - cmsg->msgLen;

            if( tlen < 0 )
            {
                break;
            }

            llen = tlen;

            enterModule(conn, cmsg);

            if( llen <= CommMsgLen )
            {
                break;
            }

            msgbuf = msgbuf + CommMsgLen + cmsg->msgLen;
        }
    }

    readDataEnd(conn, bufCtx, llen);

    return llen;
}

//
void ServerEventImp::handleError(int errCode, ConnectCtx * conn)
{
    delEvent(conn);
    connMgr->connError(errCode, conn);
}

//
void ServerEventImp::onListenEvent()
{
    struct sockaddr_in clientAddr;
    socklen_t addrSize = sizeof(struct sockaddr_in);

    int serverFd = accept(listenFd, (struct sockaddr *)&clientAddr, &addrSize);
    if(serverFd != -1)
    {
        evutil_make_socket_nonblocking(serverFd);
    }

    ConnectCtx * nconn = connMgr->getCtx();
    nconn->fd = serverFd;
    nconn->eventImp = (ServerEventImp *) (this);
    nconn->connState = ConnUseState;

    addReadEvent(nconn);

    nconn->outTimeNum = 10;
    connectOTimeEvent->addConnInEvent(nconn);
}

void ServerEventImp::onReadEvent( ConnectCtx * conn )
{
    readEventHandler(conn);
}

void ServerEventImp::onWriteEvent(ConnectCtx * conn)
{
    struct msghdr  msg;
    memset(&msg, 0, sizeof(struct msghdr));
    struct iovec io[10];

    int buf_num =0;
    BufCtx * wbufCtx = conn->writeBuf;

    int sendLen = 0;

    for(int i=0; i<10; i++)
    {
        if(wbufCtx == NULL)
        {
            break;
        }

        io[i].iov_base = (void *) wbufCtx->buf;
        io[i].iov_len = wbufCtx->bufLen;
        wbufCtx = wbufCtx->next;
        buf_num++;
        sendLen += io[i].iov_len;
    }

    msg.msg_iov = io;
    msg.msg_iovlen = buf_num;

    int ss = sendmsg(conn->fd,  &msg, 0);

    if( ss < 0 )
    {
        printf(" sendmsg error!! \n");
         //
        if(errno == EWOULDBLOCK)
        {
            addWriteEvent(conn);
            return;
        }

        handleError(ss, conn);
        return;
    }

    //
    for(int i=0; i<buf_num; i++)
    {
        wbufCtx = conn->writeBuf;
        conn->writeBuf = wbufCtx->next;
        connMgr->freeBufCtx(wbufCtx);
    }

    int reLen = 0;
    wbufCtx = conn->writeBuf;
    if( wbufCtx != NULL )
    {
        addWriteEvent(conn);
        reLen = wbufCtx->bufLen;
    }

    printf("succ WriteData: send buf_num: %d, send Len: %d %d, remain len:  %d\n",buf_num, sendLen,ss, reLen);
}

//
void ServerEventImp::onTimerEvent( TimerEventHandler * handler )
{
    if( handler->rpNum > 0 )
    {
        handler->rpNum--;
    }

    handler->timerHandle();

    if(handler->rpNum == 0)
    {
        //
        timerEventVec[handler->eventId] = NULL;
        delete handler;
    }
    else
    {
        addTimerEvent(handler);
    }
}

void ServerEventImp::addReadEvent( ConnectCtx * conn )
{
    int state = conn->eventState & EventReadState;
    if( state )
    {
        return;
    }
    //
    event_assign( &(conn->readEevnt), eventBase, conn->fd, EV_READ|EV_PERSIST, readEventCallBack, (void *)conn );

    if( event_add(&(conn->readEevnt), NULL) == -1)
    {
        perror(" can not add Read event  \n");
        exit(0);
    }
    //
    conn->eventState |= EventReadState;
}

void ServerEventImp::addWriteEvent( ConnectCtx * conn )
{
    int state = conn->eventState & EventWriteState;
    if( state )
    {
        return;
    }

    event_assign(&(conn->writeEevnt), eventBase, conn->fd, EV_WRITE, writeEventCallBack, (void *) conn);
    if( event_add(&(conn->writeEevnt), 0) == -1 )
    {
        perror(" can not add Write event  \n");
        exit(0);
    }

    conn->eventState |= EventWriteState;
}


int ServerEventImp::addTimerEvent(TimerEventHandler * handler)
{
    //
    struct timeval  t_timeout;
    evutil_timerclear(&t_timeout);
    t_timeout.tv_sec = handler->sec;
    t_timeout.tv_usec = handler->usec;

    evtimer_assign(&(handler->timeEevnt), eventBase, timerEventCallBack, (void *) handler);
    if( event_add(&(handler->timeEevnt), &t_timeout) == -1 )
    {
        perror(" can not add Timer event");
    }

    int eventId = timerEventVec.size() + 1;
    handler->eventId = eventId;
    timerEventVec.push_back(handler);

    return 0;
}

void ServerEventImp::initListen( std::string ip, int port )
{
    //
    connectOTimeEvent = new ConnectOutTimeEvent();
    addConnectOutTimer((TimerEventHandler *) connectOTimeEvent, -1, 1, 0);
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
    listen_addr.sin_port = htons(port);

    if( inet_pton(AF_INET, ip.c_str(), &listen_addr.sin_addr ) < 1)
    {
        printf(" inet_pton error \n");
        return;
    }

    if( bind(listenFd, (struct sockaddr *) &listen_addr, sizeof(listen_addr)) < 0)
    {
        printf(" bind error \n");
        return;
    }

    if(listen( listenFd, 30 ) < 0)
    {
        printf(" listen error \n");
        return;
    }

    event_assign( &listenEvent, eventBase, listenFd, EV_READ|EV_PERSIST, listenEventCallBack, (void *)this );
    event_add( &listenEvent, 0 );
}
//
void ServerEventImp::startServer()
{
    //
    event_base_dispatch(eventBase);
}
//
void ServerEventImp::addConnectOutTimer( TimerEventHandler * eventHandle, int rpnum, int sec, int usec )
{
    eventHandle->rpNum = rpnum;
    eventHandle->sec = sec;
    eventHandle->usec = usec;
    eventHandle->eventImp = (ServerEventImp *) (this);

    addTimerEvent(eventHandle);
}

BufCtx * ServerEventImp::readData(ConnectCtx * conn, int & rLen)
{
    BufCtx * bufCtx = connMgr->getReadBuf(conn);

    char * pbuf = bufCtx->buf+bufCtx->bufLen;
    int mSize = BufSize-bufCtx->bufLen;

    int nread = read(conn->fd, pbuf, mSize);

    if( nread <= 0 )
    {
        rLen = nread;
        return bufCtx;
    }

    bufCtx->bufLen = bufCtx->bufLen + nread;
    rLen  = bufCtx->bufLen;

    return bufCtx;
}

void ServerEventImp::readDataEnd(ConnectCtx * conn, BufCtx * bufCtx, int lLen)
{
    if( lLen > 0 )
    {
        int rl = bufCtx->bufLen - lLen;
        memmove(bufCtx->buf, bufCtx->buf+rl, lLen);
        bufCtx->bufLen = lLen;
        conn->readBuf = bufCtx;
    }
    else
    {
        connMgr->freeBufCtx(bufCtx);
    }
}

#endif // EVENTIMP_HPP_INCLUDED

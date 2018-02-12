#include "SlaveServer.h"
#include "basicHeader/H5MessageResolver.hpp"
#include "basicHeader/MessageDisposer.hpp"

extern const int NotifyCtxSize = sizeof(NotifyCtx);

SlaveServer::SlaveServer()
{
    //ctor
}

SlaveServer::SlaveServer(int id)
{
    serverId = id;
}

SlaveServer::~SlaveServer()
{
    //dtor
    close(notifyRecFd);
    close(notifySendFd);

    delete serverConnMgr;
    delete logicSvrMgr;

    event_base_free(eventBase);
}

void SlaveServer::initialServer( std::string masterLgIp, int masterLgPort )
{
    //
    serverConnMgr = new ConnCtxMgr();
    logicSvrMgr = new LogicServerMgr();

    eventBase = event_base_new();

    int fd[2];
    if(pipe(fd) < 0)
    {
        perror(" can not create notify pipe \n");
        exit(0);
    }

    notifyRecFd = fd[0];
    notifySendFd = fd[1];

    evutil_make_socket_nonblocking( notifyRecFd );
    evutil_make_socket_nonblocking( notifySendFd );

    event_assign(&notifyEvent, eventBase, notifyRecFd, EV_READ|EV_PERSIST, notifyReadEventCallBack,  (void *) this);

    if(event_add(&notifyEvent, 0) == -1)
    {
        perror(" can not add notify event \n");
        exit(0);
    }

    //
    char msgBuf[256];
    memset(msgBuf, 0, 256);
    //
    int wlen = MsgMgr::GateWayConnectMSTLen + MsgMgr::InterComMsgLen;
    //
    InterComMsg * msg = (InterComMsg *) (msgBuf);
    msg->moudleInd = MainInitMouldeType;
    msg->cmdInd = ReqGWConnectMS;
    msg->msgLen = wlen;
    //
    GateWayConnectMST * msgSt = (GateWayConnectMST *) (msg->data);
    msgSt->handleType = ReqGWConnectMS;
    msgSt->serverId = serverId;

    connectLogicServer(masterLgIp, masterLgPort, msg);
    printf(" connect logic main server successfully\n");

    pthread_attr_t attr;
    int ret = 0;
    pthread_attr_init(&attr);
    ret = pthread_create(&threadId, &attr, threadProcess, (void *) this);

    if(  ret != 0  )
    {
        perror(" create asyn thread error \n");
        exit(0);
    }
}

ConnCtx *  SlaveServer::connectLogicServer(std::string masterLgIp, int masterLgPort, InterComMsg * msg)
{
    //
    int nfd = 0;
    if( (nfd = socket(AF_INET, SOCK_STREAM, 0))  < 0)
    {
        perror(" can not create socket \n");
        exit(0);
    }

    int addrSize = sizeof(struct sockaddr_in);
    struct sockaddr_in * server_addr = (struct sockaddr_in *) malloc(addrSize);
    bzero(server_addr, addrSize);
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(masterLgPort);

    if( inet_pton(AF_INET, masterLgIp.c_str(), &(server_addr->sin_addr) ) < 1)
    {
        perror(" inet_pton error \n");
        exit(0);
    }

    if( connect(nfd,  (struct sockaddr *) server_addr, addrSize) < 0)
    {
        perror(" connect logic master  server error \n");
        exit(0);
    }

    evutil_make_socket_nonblocking( nfd );

    //
    ConnCtx * svrConn = serverConnMgr->getCtx(nfd, ConnLogicServer, (void *) this, eventBase, ConnEnableState );
    //

    ServerCnCtx * cnCtx = new ServerCnCtx( server_addr, 0, serverId);

    svrConn->cnCtx = (void *) cnCtx;
    //
    LgServerMsgDisposer * disposer = new LgServerMsgDisposer();
    svrConn->disposer = (MessageDisposer *) disposer;
    //
    prepareRead(svrConn);

    BufCtx * wbufCtx = serverConnMgr->getWriteBuf(svrConn, msg->msgLen);
    //
    MsgMgr::addICMsgToBufCtx(msg, wbufCtx);
    //
    addWriteEvent(svrConn);

    return svrConn;
}

int SlaveServer::getNotifyFd()
{
    return notifySendFd;
}

pthread_t SlaveServer::getThreadId()
{
    return threadId;
}

void SlaveServer::prepareRead(ConnCtx * conn)
{
    addReadEvent( conn );
}

char * SlaveServer::readData(ConnCtx * conn, int & rLen)
{
    BufCtx * bufCtx = conn->connCtxMgr->getReadBuf(conn);

    char * pbuf = bufCtx->buf+bufCtx->bufLen;
    int mSize = BufSize-bufCtx->bufLen;

    int nread = read(conn->fd, pbuf, mSize);

    if( nread <= 0 )
    {
        rLen = 0;
        return NULL;
    }

    bufCtx->bufLen = bufCtx->bufLen + nread;

    char * rbuf = bufCtx->buf;
    rLen  = bufCtx->bufLen;

    return rbuf;
}

void SlaveServer::readDataEnd(ConnCtx * conn, int lLen)
{
    BufCtx * bufCtx = conn->connCtxMgr->getReadBuf(conn);

    if( lLen > 0 )
    {
        int rl = bufCtx->bufLen - lLen;
        memmove(bufCtx->buf, bufCtx->buf+rl, lLen);
        bufCtx->bufLen = lLen;
    }
    else
    {
        conn->connCtxMgr->freeBufCtx(bufCtx);
        conn->readBuf = NULL;
    }
}

BufCtx * SlaveServer::writeData(ConnCtx * conn, int wLen)
{
    if(conn->state == ConnDisable)
    {
        return NULL;
    }

    BufCtx * bufCtx = conn->connCtxMgr->getWriteBuf(conn, wLen);
    addWriteEvent(conn);
    return bufCtx;
}

int SlaveServer::addTimer(ConnCtx * conn, TimerEventHandler * handler)
{
    //
    if(handler->rpNum == 0 || handler->conn == NULL)
    {
        return -1;
    }

    handler->conn = conn;

    struct timeval  t_timeout;
    memset( &t_timeout, 0, sizeof(struct timeval));
    t_timeout.tv_sec = handler->sec;
    t_timeout.tv_usec = handler->usec;

    short eventType = EV_TIMEOUT;

    if( handler->rpNum == -1 )
    {
        eventType |= EV_PERSIST;
    }

    event_assign(&(handler->timeEevnt), conn->eventBase, conn->fd, eventType, timerEventCallBack, (void *) handler);
    if( event_add(&(handler->timeEevnt), &t_timeout) == -1 )
    {
        perror(" can not add Timer event");
    }

    return 0;
}

int SlaveServer::delTimer(TimerEventHandler * handler)
{
    handler->timerEndHandle();
    if(handler->rpNum == -1)
    {
        event_del(&handler->timeEevnt);
        delete handler;
    }

    return 0;
}

int SlaveServer::resetTimer(TimerEventHandler * handler)
{
    if(handler->rpNum > 0)
    {
        event_del(&handler->timeEevnt);
    }
    //
    addTimer(handler->conn, handler);
    return 0;
}

void SlaveServer::initClientCtx(ClientCnCtx * ctx)
{
  // ctx->logicServerMap.insert(std::map<int, int >::value_type(0, masterIndex));
}

ConnCtx * SlaveServer::getServerConnCtx( int serverInd )
{
    ConnCtx * svrConn = NULL;

    if( serverInd == 0 )
    {
        svrConn = logicSvrMgr->getMasterSerevr();
    }
    else
    {
        svrConn = logicSvrMgr->getLogicServer(serverInd);
    }

    //
    return svrConn;
}

void SlaveServer::serverConnError(int errCode, ConnCtx * svrConn)
{
    printf("serverConnError!!!\n");
    int state = svrConn->eventState & (ReadEventState);
    if( state )
    {
        event_del(&svrConn->readEevnt);
    }

    state = svrConn->eventState & (WriteEventState);
    if(state)
    {
        event_del(&svrConn->writeEevnt);
    }

    svrConn->eventState = 0;
    //
    switch(svrConn->connType)
    {
        case ConnMLogicServer:
        {
            logicSvrMgr->delMasterSerevr();
            svrConn->connType = 0;
            ServerCnCtx::setIndByConn(svrConn, 0);
            printf("ConnMLogicServer!!!\n");

            break;
        }

        case ConnSLogicServer:
        {
            int ind = ServerCnCtx::getIndByConn(svrConn);
            logicSvrMgr->delLogicServer(ind);
            svrConn->connType = 0;
            ServerCnCtx::setIndByConn(svrConn, 0);
            break;
        }
    }

    close(svrConn->fd);
    svrConn->fd = 0;

    if( svrConn->timerHandler != NULL )
    {
        svrConn->state = ConnDisable;
        svrConn->timerHandler->rpNum = 10;
        addTimer(svrConn, svrConn->timerHandler);
    }
    else
    {
        svrConn->connCtxMgr->connError(0, svrConn);
    }
}

void SlaveServer::initConnByMsg(int handlerState, void * msg, ConnCtx * conn, void * resData)
{
    switch(handlerState)
    {
        case ResGWConnectMS:
        {
            InterComMsg * inMsg = (InterComMsg *) msg;
            GateWayConnectMST * msgSt = (GateWayConnectMST *) (inMsg->data);
            printf("*&*ResGWConnectMS!!! serverInd: %d, fd: %d\n", msgSt->logicSvrIndex, conn->fd);
            conn->connType = ConnMLogicServer;
            //
            ServerCnCtx::setIndByConn(conn, msgSt->logicSvrIndex);
            logicSvrMgr->addMasterServer(msgSt->logicSvrIndex, conn);

            //
            ReconnectHandler::setTimerHandlerToConn(conn, 1);
            break;
        }

        case ReqGWConnectSS:
        {
            InterComMsg * inMsg = (InterComMsg *) msg;
            inMsg->moudleInd = GateWayMsgType;
            //
            SlaveSvrConnectMST * msgSt = (SlaveSvrConnectMST *) (inMsg->data);
            printf("**ReqGWConnectSS serverInd : %d serverId: %d \n", msgSt->logicSvrIndex, msgSt->serverId);

            //
            std::string masterLgIp = msgSt->ip;
            int masterLgPort = msgSt->prot;
            //
            ConnCtx * slaveConn = connectLogicServer(masterLgIp, masterLgPort, inMsg);
            slaveConn->connType = ConnSLogicServer;
            ServerCnCtx::setIndByConn(slaveConn, msgSt->logicSvrIndex);
            logicSvrMgr->addLogicServer(msgSt->logicSvrIndex, slaveConn);
            //
            printf("****connect logic slave server successfully ip: %s  port : %d\n", msgSt->ip, msgSt->prot);

            break;
        }
        //
        case ResGWConnectSS:
        {
            InterComMsg * inMsg = (InterComMsg *) msg;
            SlaveSvrConnectMST * msgSt = (SlaveSvrConnectMST *) (inMsg->data);
            printf("ResGWConnectSS serverInd : %d fd: %d \n", msgSt->logicSvrIndex, conn->fd);
            break;
        }

    }
}

void SlaveServer::addReadEvent( ConnCtx * conn )
{
    int state = conn->eventState & ReadEventState;
    if( state )
    {
        return;
    }

    event_assign(&(conn->readEevnt), eventBase, conn->fd, EV_READ|EV_PERSIST, readEventCallBack,  (void *) conn);

    if(event_add(&conn->readEevnt, 0) == -1)
    {
        perror(" can not add notify event \n");
        exit(0);
    }

    conn->eventState |= ReadEventState;
}

void SlaveServer::addWriteEvent( ConnCtx * conn )
{
    int state = conn->eventState & WriteEventState;
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

    conn->eventState |= WriteEventState;
}

void * SlaveServer::threadProcess( void * ctx )
{
    SlaveServer * slaveCtx = (SlaveServer *) ctx;
    slaveCtx->onThread();
    return NULL;
}

void SlaveServer::notifyReadEventCallBack( int fd, short eventType, void * ctx )
{
    SlaveServer * mServer = (SlaveServer *) ctx;
    mServer->onNotifyEvent();
}

void SlaveServer::readEventCallBack( int fd, short eventType, void * ctx )
{
    ConnCtx * conn = (ConnCtx *) ctx;
    SlaveServer * mServer = (SlaveServer *) conn->serverCtx;
    mServer->onReadEvent( conn );
}

void SlaveServer::writeEventCallBack( int fd, short eventType, void * ctx )
{
    ConnCtx * conn = (ConnCtx *) ctx;
    //
    if((eventType & EV_PERSIST) == 0)
    {
        conn->eventState &= (~WriteEventState);
    }

    SlaveServer * mServer = (SlaveServer *) conn->serverCtx;
    mServer->onWriteEvent( conn );
}

void SlaveServer::timerEventCallBack( int fd, short eventType, void * ctx )
{
    TimerEventHandler * handler = (TimerEventHandler *) ctx;
    ConnCtx * conn = handler->conn;
    SlaveServer * mServer = (SlaveServer *) conn->serverCtx;
    mServer->onTimerEvent(conn, handler);
}

void SlaveServer::onThread()
{
    event_base_dispatch(eventBase);
}

void SlaveServer::onReadEvent(ConnCtx * conn)
{
    MessageDisposer * disposer = (MessageDisposer *) (conn->disposer);
    disposer->readDataInConnBuf(conn);
}

 void SlaveServer::onWriteEvent(ConnCtx * conn)
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

        MessageDisposer * disposer = (MessageDisposer *) (conn->disposer);
        disposer->handleError(ss, conn);
        return;
    }

    //
    for(int i=0; i<buf_num; i++)
    {
        wbufCtx = conn->writeBuf;
        conn->writeBuf = wbufCtx->next;
        conn->connCtxMgr->freeBufCtx(wbufCtx);
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

void SlaveServer::onTimerEvent(ConnCtx * conn, TimerEventHandler * handler)
{
    printf("onTimerEvent %d\n",handler->rpNum);

    handler->rpNum--;
    handler->timerHandle();

    if(handler->rpNum == 0)
    {
        //
        if( handler->state != -1 )
        {
             delTimer(handler);
        }
    }
    else
    {
        addTimer(conn, handler);
    }
}

void SlaveServer::onNotifyEvent()
{
    int stop = 0;
    while( stop != 1 )
    {
        char rbuf[BufSize];

        int nread = read(notifyRecFd, rbuf, BufSize);

        if( nread != BufSize )
        {
            stop = 1;
        }

        if( nread >= NotifyCtxSize )
        {
            int llen = nread;
            char * pbuf = rbuf;
            NotifyCtx * notify = NULL;

            while( 1 )
            {
                notify = (NotifyCtx *) pbuf;
                int t = llen - NotifyCtxSize;

                if( t < 0 )
                {
                    break;
                }

                llen = t;
                handleNotify(notify);

                if( llen < NotifyCtxSize )
                {
                    break;
                }

                pbuf = pbuf + NotifyCtxSize;
            }
        }
    }
}

void SlaveServer::handleNotify( NotifyCtx * notify )
{
    switch(notify->notifyType)
    {
        case NotifyConnect:
            {
                printf("slave server %d connect new client!\n", serverId);
                ConnCtx * cliConn = getClientConn( notify->fd, ConnH5Client, (void *) this, eventBase, ConnEnableState );
                //
                OutTimerHandler * timerHandler = new OutTimerHandler();
                timerHandler->rpNum = 1;
                timerHandler->sec = 30;
                timerHandler->conn = cliConn;
                cliConn->timerHandler = (TimerEventHandler *) timerHandler;
                //
                CliServerMsgDisposer * disposer = new CliServerMsgDisposer();
                cliConn->disposer = (MessageDisposer *) disposer;

                if(cliConn->resolver == NULL)
                {
                    cliConn->resolver = (MessageResolver *) new H5MessageResolver();
                }

                prepareRead(cliConn);
                //
                addTimer(cliConn, cliConn->timerHandler);
                break;
            }
    }
}

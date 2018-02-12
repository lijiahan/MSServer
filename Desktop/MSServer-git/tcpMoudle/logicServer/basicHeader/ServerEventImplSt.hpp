#ifndef SERVEREVENTIMPLST_HPP_INCLUDED
#define SERVEREVENTIMPLST_HPP_INCLUDED

#include "MsgImplSt.hpp"

struct GateWayCtx
{
    int gateWayIndex;
    int serverId;
    ConnCtx * conn;
};

struct SlaveServerCtx
{
    int slaveSvrIndex;
    int serverId;
    ConnCtx * conn;
    int connNum;
    int weight;
    char ip[16];
    int port;
};

struct ServerDataCtx
{
    int writeInd;
    int ctxIndex;
};

//
class DataMoudleCtx
{
    public:
        DataMoudleCtx()
        {
            type = 0;
            state = 0;
            dispIndex = 0;
            data = NULL;
        }

    public:
        int type;
        int state;
        int dispIndex;
        void * data;
};

//
class LogicMoudleCtx
{
    public:
        LogicMoudleCtx()
        {
            type = 0;
            state = 0;
            dispIndex = 0;
            data = NULL;
        }

    public:
        int type;
        int state;
        int dispIndex;
        void * data;
};

//
struct ClientConnCtx
{
    int connIndex;
    ConnCtx * conn;
};


class ClientCtx
{
    public:
        ClientCtx()
        {

        }

        ~ClientCtx()
        {
            std::map<MOUDLEID, DataMoudleCtx* >::iterator itData;
            for( itData = dataMoudleMap.begin(); itData != dataMoudleMap.end(); itData++ )
            {
                if(itData->second != NULL)
                {
                    delete itData->second;
                }
            }
            dataMoudleMap.clear();

            //
            std::map<MOUDLEID, LogicMoudleCtx* >::iterator itLogic;
            for( itLogic = logicMoudleMap.begin(); itLogic != logicMoudleMap.end(); itLogic++ )
            {
                if(itLogic->second != NULL)
                {
                    delete itLogic->second;
                }
            }
            logicMoudleMap.clear();
        }

        DataMoudleCtx * getDataCtx( MOUDLEID mid )
        {
            DataMoudleCtx * dataCtx = NULL;
            std::map<MOUDLEID, DataMoudleCtx* >::iterator it = dataMoudleMap.find(mid);
            if( it != dataMoudleMap.end() )
            {
                dataCtx = it->second;
            }
            else
            {
                dataCtx = new DataMoudleCtx();
                dataMoudleMap[mid] = dataCtx;
            }

            return dataCtx;
        }

        //
        LogicMoudleCtx * getLogicCtx( MOUDLEID mid )
        {
            LogicMoudleCtx * logicCtx = NULL;
            std::map<MOUDLEID, LogicMoudleCtx* >::iterator it = logicMoudleMap.find(mid);
            if( it != logicMoudleMap.end() )
            {
                logicCtx = it->second;
            }
            else
            {
                logicCtx = new LogicMoudleCtx();
                logicMoudleMap[mid] = logicCtx;
            }

            return logicCtx;
        }

    public:
        CLIENTIDTYPE uid;
        ClientConnCtx clConnCtx;
        std::map<MOUDLEID, DataMoudleCtx *> dataMoudleMap;
        std::map<MOUDLEID, LogicMoudleCtx *> logicMoudleMap;
};

class ClientCtxMgr
{
   public:
        ClientCtxMgr()
        {

        }

        ~ClientCtxMgr()
        {
            //
            for( uint32_t i=0; i<clientFreeVec.size(); i++)
            {
                 ClientCtx * clientCtx  = clientFreeVec[i];
                 if(clientCtx != NULL)
                 {
                    delete clientCtx;
                    clientFreeVec[i] = NULL;
                 }
            }

            clientFreeVec.clear();
            //
            std::map<CLIENTIDTYPE, ClientCtx* >::iterator it;
            for( it = clientCtxMap.begin(); it != clientCtxMap.end(); it++ )
            {
                if(it->second != NULL)
                {
                    delete it->second;
                }
            }
            clientCtxMap.clear();
        }

        ClientCtx * getCtx()
        {
            ClientCtx * clientCtx = NULL;
            if(clientFreeVec.size() > 0)
            {
                clientCtx = clientFreeVec.back();
                clientFreeVec.pop_back();
            }
            else
            {
                clientCtx = new ClientCtx();
                clientCtx->uid = 0;
            }

            return clientCtx;
        }

        ClientCtx * getCtx(CLIENTIDTYPE clientId)
        {
            ClientCtx * clientCtx = NULL;
            std::map<CLIENTIDTYPE, ClientCtx* >::iterator it = clientCtxMap.find(clientId);
            if( it != clientCtxMap.end() )
            {
                clientCtx = it->second;
            }
            else
            {
                clientCtx = getCtx();
                clientCtx->uid = clientId;
                clientCtxMap[clientId] = clientCtx;
            }

            return clientCtx;
        }

        void freeCtx(ClientCtx * clientCtx)
        {
            //
            clientCtxMap.erase(clientCtx->uid);
            //
            clientCtx->uid = 0;

            clientFreeVec.push_back(clientCtx);
            //
        }

        void preDataCtx(ClientCtx * cliCtx, MOUDLEID mid)
        {

        }

    private:
        std::vector<ClientCtx *> clientFreeVec;
        std::map<CLIENTIDTYPE, ClientCtx *> clientCtxMap;
};

//
//
struct HandlerCtx
{
    int index;
    int handleState;
    InterComMsg * msg;
    ClientCtx * clCtx;
    ClientCtxMgr * clMgr;
    char msgData[1024];  //
};

class HandlerCtxMgr
{
    public:
        HandlerCtxMgr()
        {

        }

        ~HandlerCtxMgr()
        {
            for( uint32_t i=0; i<handlerCtxVec.size(); i++)
            {
                 HandlerCtx * handlerCtx = handlerCtxVec[i];
                 if(handlerCtx != NULL)
                 {
                    free(handlerCtx);
                    handlerCtxVec[i] = NULL;
                 }
            }

            handlerCtxVec.clear();

            for( uint32_t i=0; i<handlerFreeVec.size(); i++)
            {
                 HandlerCtx * handlerCtx = handlerFreeVec[i];
                 if(handlerCtx != NULL)
                 {
                     free(handlerCtx);
                     handlerFreeVec[i] = NULL;
                 }
            }

            handlerFreeVec.clear();
        }

        HandlerCtx * getCtx()
        {
            HandlerCtx * handlerCtx = NULL;
            if(handlerFreeVec.size() > 0)
            {
                handlerCtx = handlerFreeVec.back();
                handlerFreeVec.pop_back();
            }
            else
            {
                handlerCtx = (HandlerCtx *) malloc(HandlerCtxSize);
                memset(handlerCtx, 0, HandlerCtxSize);
                handlerCtx->index = handlerCtxVec.size()+1;
                handlerCtxVec.push_back(handlerCtx);
            }

            return handlerCtx;
        }

        HandlerCtx * getCtx(int index, ConnCtx * conn, InterComMsg * msg, ClientCtx * clCtx, ClientCtxMgr * clMgr)
        {
            HandlerCtx * handlerCtx = NULL;
            int ind = index - 1;

            if(ind >= 0 && ind < handlerCtxVec.size())
            {
                handlerCtx = handlerCtxVec[ind];
            }
            else
            {
                handlerCtx = getCtx();
                //
                if( handlerCtx != NULL )
                {
                    handlerCtx->handleState = 0;
                    handlerCtx->msg = msg;
                    handlerCtx->clCtx = clCtx;
                    handlerCtx->clMgr = clMgr;
                }
            }

            return handlerCtx;
        }

        void freeCtx( HandlerCtx * handlerCtx )
        {
            handlerCtx->handleState = 0;
            handlerCtx->msg = NULL;
            handlerCtx->clCtx = NULL;
            handlerCtx->clMgr = NULL;
            handlerFreeVec.push_back(handlerCtx);
            handlerCtxVec[handlerCtx->index-1] = NULL;
        }

    private:
        std::vector<HandlerCtx *> handlerCtxVec;
        std::vector<HandlerCtx *> handlerFreeVec;
    public:
        static const int HandlerCtxSize = sizeof(HandlerCtx);
};

struct MoudleDataCheck
{
    int logicId;
    int dataNum;
    int dataIndex[ModuleMaxNum];
    SlaveServerCtx * slaveCtx;
};

class LogicMoudleInterface
{
    public:
        LogicMoudleInterface(){}
        virtual ~LogicMoudleInterface(){}

        virtual int getDataCheck(ClientCtx * clientCtx, MoudleDataCheck * dataCheck)
        {
            return dataCheck->slaveCtx->slaveSvrIndex;
        }

        virtual void logicHander(HandlerCtx * handlerCtx)
        {

        }
};

class LogicMoudleMgr
{
    public:
        LogicMoudleMgr()
        {
            clientCtxMgr = new ClientCtxMgr();
             //
            handlerCtxMgr = new HandlerCtxMgr();
        }

        virtual ~LogicMoudleMgr()
        {
            delete clientCtxMgr;
            delete handlerCtxMgr;
        }

        virtual void initLogicMoudle() = 0;
        virtual SlaveServerCtx * diapatchServerInd(int blockId) = 0;
        virtual void addServerInd(int ind) = 0;
        virtual void logicMoudle(ConnCtx * conn, InterComMsg * msg) = 0;
         //
        void registerLogicMoudle(int type, LogicMoudleInterface * moudle)
        {
            logicInterfaceMap.insert(std::pair<int, LogicMoudleInterface *>(type, moudle));
        }

        LogicMoudleInterface * unregisterLogicMoudle(int type)
        {
            LogicMoudleInterface * inf = NULL;
            std::map<int, LogicMoudleInterface * >::iterator it = logicInterfaceMap.find(type);
            if( it != logicInterfaceMap.end() )
            {
                inf = it->second;
            }

            logicInterfaceMap.erase(type);
            return inf;
        }

        ClientCtx * getNewClientCtx(CLIENTIDTYPE uid, ConnCtx * conn, int connIndex)
        {
            ClientCtx * clientCtx = clientCtxMgr->getCtx(uid);
            if( clientCtx != NULL )
            {
                clientCtx->clConnCtx.conn = conn;
                clientCtx->clConnCtx.connIndex = connIndex;
            }
            return clientCtx;
        }

        LogicMoudleInterface * getLogicMoudleInf( int mid)
        {
            LogicMoudleInterface * mif = NULL;
            std::map<int, LogicMoudleInterface * >::iterator it = logicInterfaceMap.find(mid);
            if( it != logicInterfaceMap.end() )
            {
                mif = it->second;
            }

            return mif;
        }

        void freeClientCtx(ClientCtx * clientCtx)
        {
            clientCtxMgr->freeCtx(clientCtx);
        }

    public:
        std::map<int, LogicMoudleInterface *> logicInterfaceMap;
        ClientCtxMgr * clientCtxMgr;
        HandlerCtxMgr * handlerCtxMgr;
};

//
class ServerExtraInterface : public TimerEventMoulde, public MsgMoudleMgr, public LogicMoudleMgr
{
    public:
        virtual ~ServerExtraInterface()
        {
            event_del( &listenEvent );
            close( listenFd );
            event_base_free( eventBase );
            delete serverConnMgr;
        }

        //
        virtual void listenEventHandler(ConnCtx * conn) = 0;
        virtual void writeEventHandler(ConnCtx * conn) = 0;
        virtual void connError(int errCode, ConnCtx * conn) = 0;

        virtual int readEventHandler(ConnCtx * conn)
        {
            int rLen = 0;
            char * data = readData(conn, rLen);

            if(data == NULL)
            {
                readDataEnd(conn, 0);
                handleError(rLen, conn);
                return 0;
            }

            int llen = rLen;
            //
            if( llen > MsgMgr::InterComMsgLen )
            {
                char * msgbuf = data;
                while( 1 )
                {
                    InterComMsg * inMsg = (InterComMsg *) msgbuf;
                    int tlen = llen - MsgMgr::InterComMsgLen - inMsg->msgLen;

                    if( tlen < 0 )
                    {
                        break;
                    }

                    llen = tlen;

                    handleInterMsg(conn, inMsg);

                    if( llen <= MsgMgr::InterComMsgLen )
                    {
                        break;
                    }

                    msgbuf = msgbuf + MsgMgr::InterComMsgLen + inMsg->msgLen;
                }
            }

            readDataEnd(conn, llen);

            return llen;
        }
        //
        //
        static void listenEventCallBack( int fd, short eventType, void * ctx )
        {
            printf("connect one server!!\n");
            ServerExtraInterface * ein = (ServerExtraInterface *) ctx;
            ein->onListenEvent();
        }

        static void readEventCallBack( int fd, short eventType, void * ctx )
        {
            ConnCtx * conn = (ConnCtx *) ctx;
            ServerExtraInterface * ein = (ServerExtraInterface *) conn->serverCtx;
            ein->onReadEvent( conn );
        }

        static void writeEventCallBack( int fd, short eventType, void * ctx )
        {
            ConnCtx * conn = (ConnCtx *) ctx;
            //
            if((eventType & EV_PERSIST) == 0)
            {
                conn->eventState &= (~WriteEventState);
            }

            ServerExtraInterface * ein = (ServerExtraInterface *) conn->serverCtx;
            ein->onWriteEvent( conn );
        }

        static void timerEventCallBack( int fd, short eventType, void * ctx )
        {
            TimerEventHandler * handler = (TimerEventHandler *) ctx;
            ConnCtx * conn = handler->conn;
            ServerExtraInterface * ein = (ServerExtraInterface *)  conn->serverCtx;
            ein->onTimerEvent(conn, handler);
        }

        //
        void initServer( std::string ip, int port )
        {
            serverConnMgr = new ConnCtxMgr();
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

            eventBase = event_base_new();

            event_assign( &listenEvent, eventBase, listenFd, EV_READ|EV_PERSIST, listenEventCallBack, (void *)this );
            event_add( &listenEvent, 0 );
        }

        virtual void addReadEvent( ConnCtx * conn )
        {
            int state = conn->eventState & ReadEventState;
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
            conn->eventState |= ReadEventState;
        }

        virtual void addWriteEvent( ConnCtx * conn )
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

         ////
        virtual int addTimerEvent(ConnCtx * conn, TimerEventHandler * handler)
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

        //
        virtual void prepareRead(ConnCtx * conn)
        {
            addReadEvent( conn );
        }

        virtual char * readData(ConnCtx * conn, int & rLen)
        {
            BufCtx * bufCtx = conn->connCtxMgr->getReadBuf(conn);

            char * pbuf = bufCtx->buf+bufCtx->bufLen;
            int mSize = BufSize-bufCtx->bufLen;

            int nread = read(conn->fd, pbuf, mSize);

            if( nread <= 0 )
            {
                rLen = nread;
                return NULL;
            }

            bufCtx->bufLen = bufCtx->bufLen + nread;

            char * rbuf = bufCtx->buf;
            rLen  = bufCtx->bufLen;

            return rbuf;
        }

        virtual void readDataEnd(ConnCtx * conn, int lLen)
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

        virtual BufCtx * writeData(ConnCtx * conn, int wLen)
        {
            BufCtx * bufCtx = conn->connCtxMgr->getWriteBuf(conn, wLen);
            addWriteEvent(conn);
            return bufCtx;
        }

        //
        virtual void handleError(int errCode, ConnCtx * conn)
        {
            connError(errCode,conn);
            conn->connCtxMgr->connError(errCode, conn);
        }

        virtual void onListenEvent()
        {
            struct sockaddr_in clientAddr;
            socklen_t addrSize = sizeof(struct sockaddr_in);

            int serverFd = accept(listenFd, (struct sockaddr *)&clientAddr, &addrSize);
            if(serverFd != -1)
            {
                evutil_make_socket_nonblocking(serverFd);
            }

            ConnCtx * serverConn = serverConnMgr->getCtx();
            serverConn->fd = serverFd;
            serverConn->serverCtx = (void *) this;

            addReadEvent(serverConn);
            listenEventHandler(serverConn);
        }

        virtual void onReadEvent( ConnCtx * conn )
        {
            readEventHandler(conn);
        }

        virtual void onWriteEvent(ConnCtx * conn)
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
                conn->connCtxMgr->freeBufCtx(wbufCtx);
            }

            int reLen = 0;
            wbufCtx = conn->writeBuf;
            if( wbufCtx != NULL )
            {
                addWriteEvent(conn);
                reLen = wbufCtx->bufLen;
            }

            writeEventHandler(conn);

            printf("succ WriteData: send buf_num: %d, send Len: %d %d, remain len:  %d\n",buf_num, sendLen,ss, reLen);
        }

        //
        virtual void onTimerEvent(ConnCtx * conn, TimerEventHandler * handler)
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

        void startServer()
        {
            //
            event_base_dispatch(eventBase);
        }

        ConnCtx * getSvrConnCtx()
        {
            return serverConnMgr->getCtx();
        }

    protected:
        int listenFd;
        struct event_base * eventBase;
        struct event listenEvent;
        //
        ConnCtxMgr * serverConnMgr;
};

#endif // SERVEREVENTIMPLST_HPP_INCLUDED

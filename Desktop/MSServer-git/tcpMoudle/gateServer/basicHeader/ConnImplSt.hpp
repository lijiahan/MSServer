#ifndef CONNIMPLST_HPP_INCLUDED
#define CONNIMPLST_HPP_INCLUDED

#include "BaseFileHeader.h"

enum ConnType
{
    ConnGateWay =1,
    ConnLogicServer,
    ConnMLogicServer,
    ConnSLogicServer,
    ConnClient,
    ConnH5Client,
};

enum ConnState
{
    ConnDisable = 0,
    ConnHandShakeState,
    ConnEnableState,
};

enum EventState
{
    ReadEventState = 0x1,
    WriteEventState = 0x10,
    TimerEventState = 0x100,
};

struct BufCtx;
struct ConnCtx;

class ConnCtxMgr;

class DataCorrespondent
{
    public:
        virtual ~DataCorrespondent(){}
        virtual void prepareRead(ConnCtx * conn) = 0;
        virtual char * readData(ConnCtx * conn, int & rLen) = 0;
        virtual void readDataEnd(ConnCtx * conn, int lLen) = 0;
        virtual BufCtx * writeData(ConnCtx * conn, int wLen) = 0;
};

class TimerEventHandler
{
    public:
        TimerEventHandler()
        {
            rpNum = 0;
            sec = 0;
            usec = 0;
            state = 0;
        }

        virtual ~TimerEventHandler()
        {
            if( rpNum != 0 )
            {
                event_del(&timeEevnt);
            }
        }

        virtual void timerHandle() = 0;
        virtual void timerEndHandle() = 0;

    public:
        int rpNum;
        int sec;
        int usec;
        struct event timeEevnt;
        ConnCtx * conn;
        int state;
};

class TimerEventMoulde
{
    public:
        virtual ~TimerEventMoulde(){}
        virtual int addTimer(ConnCtx * conn, TimerEventHandler * handler) = 0;
        virtual int delTimer(TimerEventHandler * handler) = 0;
        virtual int resetTimer(TimerEventHandler * handler) = 0;
};

//
class MessageDisposer
{
    public:
        virtual ~MessageDisposer(){}
        virtual int readDataInConnBuf(ConnCtx * conn) = 0;
        virtual void handlerModule(int msgType, char * msgBuf, ConnCtx * conn) = 0;
        virtual void handleError(int errCode, ConnCtx * conn) = 0;
};

//
class MessageResolver
{
    public:
        virtual ~MessageResolver(){}
        virtual int handShake( char * msgBuf, char * respond, int & rLen ) = 0;
        virtual int decoder( char * msgBuf, int & pos ) = 0;
        virtual int encoder( char * msgBuf, uint32_t len ) = 0;
};

struct BufCtx
{
    char buf[BufSize];
    int bufLen;
    BufCtx * next;
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
                 BufCtx * buf = bufFreeVec[i];
                 free(buf);
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
                bufCtx->bufLen = 0;
                bufCtx->next = NULL;
                memset(bufCtx, 0, BufSize);
            }

            return bufCtx;
        }

        void freeCtx( BufCtx * bufCtx )
        {
            bufCtx->bufLen = 0;
            bufCtx->next = NULL;
            memset(bufCtx, 0, BufSize);
            bufFreeVec.push_back(bufCtx);
        }

    private:
        std::vector<BufCtx *> bufFreeVec;
    public:
        static const int BufCtxSize = sizeof(BufCtx);
};

struct ConnCtx
{
    uint32_t id;
    int index;
    int connType;
    int fd;
    int state;
    int eventState;
    //
    struct event readEevnt;
    struct event writeEevnt;
    struct event_base * eventBase;
    //
    TimerEventHandler * timerHandler;
    //
    BufCtx * readBuf;
    BufCtx * writeBuf;
    //
    ConnCtxMgr * connCtxMgr;
    //
    void * serverCtx;
    void * cnCtx;
    //
    MessageDisposer * disposer;
    MessageResolver * resolver;
};

class ConnCtxMgr
{
   public:
        ConnCtxMgr()
        {
             bufCtxMgr = new BufCtxMgr();
        }

        ~ConnCtxMgr()
        {
            for( uint32_t i=0; i<connFreeVec.size(); i++)
            {
                 ConnCtx * conn = connFreeVec[i];
                 free(conn);
                 connFreeVec[i] = NULL;
            }

            connFreeVec.clear();

            for( uint32_t i=0; i<connVec.size(); i++)
            {
                ConnCtx * conn = connVec[i];
                if( conn != NULL )
                {
                    int state = conn->eventState & (ReadEventState);
                    if( state )
                    {
                        event_del(&conn->readEevnt);
                    }

                    state = conn->eventState & (WriteEventState);
                    if(state)
                    {
                        event_del(&conn->writeEevnt);
                    }

                    if(conn->timerHandler != NULL && conn->timerHandler->rpNum != 0)
                    {
                        event_del(&conn->timerHandler->timeEevnt);
                    }

                    if(conn->timerHandler != NULL )
                    {
                        delete conn->timerHandler;
                    }

                    BufCtx * buf = conn->readBuf;

                    while(buf!= NULL)
                    {
                        BufCtx * bufTemp = buf->next;
                        free(buf);
                        buf = bufTemp;
                    }

                    buf = conn->writeBuf;
                    while(buf!= NULL)
                    {
                        BufCtx * bufTemp = buf->next;
                        free(buf);
                        buf = bufTemp;
                    }

                    if(conn->resolver != NULL)
                    {
                        delete conn->resolver;
                    }

                    if(conn->disposer != NULL)
                    {
                        delete conn->disposer;
                    }

                    close(conn->fd);

                    free(conn);
                }

                connVec[i] = NULL;
            }

            connVec.clear();

            delete bufCtxMgr;
        }

        ConnCtx * getCtx()
        {
            ConnCtx * conn = NULL;

            if(connFreeVec.size() > 0)
            {
                conn = connFreeVec.back();
                connFreeVec.pop_back();
                connVec[conn->index] = conn;
            }
            else
            {
                conn = (ConnCtx *) malloc(ConnCtxSize);
                memset(conn, 0, ConnCtxSize);
                conn->index = connVec.size();
                conn->connCtxMgr = this;
                connVec.push_back(conn);
            }

            return conn;
        }



        ConnCtx * getCtx( int nfd, int connType, void * serverCtx, struct event_base * base, int state )
        {
            ConnCtx * conn = NULL;

            if(connFreeVec.size() > 0)
            {
                conn = connFreeVec.back();
                connFreeVec.pop_back();
                connVec[conn->index] = conn;
            }
            else
            {
                conn = (ConnCtx *) malloc(ConnCtxSize);
                memset(conn, 0, ConnCtxSize);
                conn->index = connVec.size();
                conn->connCtxMgr = this;
                connVec.push_back(conn);
            }

            conn->fd = nfd;
            conn->connType = connType;
            conn->serverCtx = serverCtx;
            conn->eventBase = base;
            conn->state = state;

            return conn;
        }

        ConnCtx * getCtx( int index )
        {
            if( connVec.size() > index )
            {
                return connVec[index];
            }
            else
            {
                printf("getCtx out index !!\n");
            }

            return NULL;
        }

        void freeCtx( ConnCtx * conn )
        {
            conn->id = 0;

            BufCtx * wbuf = conn->writeBuf;

            while(wbuf != NULL)
            {
                BufCtx * buf = wbuf;
                wbuf = wbuf->next;
                bufCtxMgr->freeCtx(buf);
            }

            conn->writeBuf = NULL;

            BufCtx * rbuf = conn->readBuf;
            while(rbuf != NULL)
            {
                BufCtx * buf = rbuf;
                rbuf = rbuf->next;
                bufCtxMgr->freeCtx(buf);
            }

            conn->readBuf = NULL;

            conn->fd = 0;
            conn->connType = 0;
            conn->state = 0;
            conn->eventState = 0;
            //
            conn->eventBase = NULL;
            //
            conn->serverCtx = NULL;
            conn->cnCtx = NULL;

            connVec[conn->index] = NULL;
            connFreeVec.push_back(conn);
        }

        BufCtx * getReadBuf(ConnCtx * conn)
        {
            BufCtx * bufCtx = conn->readBuf;
            if( bufCtx == NULL )
            {
               bufCtx = bufCtxMgr->getCtx();
               conn->readBuf = bufCtx;
               return bufCtx;
            }

            return bufCtx;
        }

        BufCtx * getWriteBuf( ConnCtx * conn, int wlen )
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

        void connError(int code, ConnCtx * conn )
        {
            std::cout<<"connError code:"<<code<<std::endl;
            if( code < 0 )
            {
                if( errno == EWOULDBLOCK )
                {
                    return;
                }
            }

            int state = conn->eventState & (ReadEventState);
            if( state )
            {
                event_del(&conn->readEevnt);
            }

            state = conn->eventState & (WriteEventState);
            if(state)
            {
                event_del(&conn->writeEevnt);
            }

            if(conn->timerHandler != NULL && conn->timerHandler->rpNum != 0)
            {
                event_del(&conn->timerHandler->timeEevnt);
                conn->timerHandler->rpNum = 0;
                conn->timerHandler->state = -1;
            }

            conn->fd = 0;
            close(conn->fd);
            freeCtx(conn);
        }

    private:
        BufCtxMgr * bufCtxMgr;
        std::vector<ConnCtx *> connVec;
        std::vector<ConnCtx *> connFreeVec;
    public:
        static const int ConnCtxSize = sizeof(ConnCtx);
};

class ServerCnCtx
{
    public:
        ServerCnCtx(struct sockaddr_in * addr, int ind, int id)
        {
            server_addr = addr;
            logicSvrInd = ind;
            serverId = id;
        }

        ~ServerCnCtx()
        {

        }

        static struct sockaddr_in * getAddrByConn(ConnCtx * conn)
        {
            ServerCnCtx * cnCtx = (ServerCnCtx *) (conn->cnCtx);
            return cnCtx->server_addr;
        }

        static int getIndByConn(ConnCtx * conn)
        {
            ServerCnCtx * cnCtx = (ServerCnCtx *) (conn->cnCtx);
            return cnCtx->logicSvrInd;
        }

        static void setIndByConn(ConnCtx * conn, int ind)
        {
            ServerCnCtx * cnCtx = (ServerCnCtx *) (conn->cnCtx);
            cnCtx->logicSvrInd = ind;
        }

        static int getIdByConn(ConnCtx * conn)
        {
            ServerCnCtx * cnCtx = (ServerCnCtx *) (conn->cnCtx);
            return cnCtx->serverId;
        }

        static void setIdByConn(ConnCtx * conn, int id)
        {
            ServerCnCtx * cnCtx = (ServerCnCtx *) (conn->cnCtx);
            cnCtx->serverId = id;
        }

    public:
        struct sockaddr_in * server_addr;
        int logicSvrInd;
        int serverId;
};

class ClientCnCtx
{
    public:

    public:
        CLIENTIDTYPE clientId;
        ConnCtx * conn;
        std::map<BLOCKIDTYPE, int> logicServerMap;
};

class ClientCnCtxMgr
{
   public:
        ClientCnCtxMgr()
        {

        }

        ~ClientCnCtxMgr()
        {
            //
            for( uint32_t i=0; i<clientFreeVec.size(); i++)
            {
                 ClientCnCtx * clientCtx  = clientFreeVec[i];
                 if(clientCtx != NULL)
                 {
                    delete clientCtx;
                    clientFreeVec[i] = NULL;
                 }
            }

            clientFreeVec.clear();
            //
            std::map<CLIENTIDTYPE, ClientCnCtx* >::iterator it;
            for( it = clientCtxMap.begin(); it != clientCtxMap.end(); it++ )
            {
                delete it->second;
            }
            clientCtxMap.clear();
        }

        ClientCnCtx * getCtx()
        {
            ClientCnCtx * clientCtx = NULL;
            if(clientFreeVec.size() > 0)
            {
                clientCtx = clientFreeVec.back();
                clientFreeVec.pop_back();
            }
            else
            {
                clientCtx = new ClientCnCtx();
                clientCtx->clientId = 0;
                clientCtx->conn = NULL;
            }

            return clientCtx;
        }

        ClientCnCtx * getCtx(ConnCtx * conn)
        {
            CLIENTIDTYPE clientId = conn->id;
            ClientCnCtx * clientCtx = NULL;
            std::map<CLIENTIDTYPE, ClientCnCtx* >::iterator it = clientCtxMap.find(clientId);
            if( it != clientCtxMap.end() )
            {
                clientCtx = it->second;
            }
            else
            {
                clientCtx = getCtx();
                clientCtxMap.insert(std::map<CLIENTIDTYPE, ClientCnCtx* >::value_type(clientId, clientCtx));
            }
            //

            clientCtx->clientId = clientId;
            clientCtx->conn = conn;
            //
            conn->cnCtx = (void *) clientCtx;

            return clientCtx;
        }

        ClientCnCtx * getCtx(CLIENTIDTYPE clientId)
        {
            ClientCnCtx * clientCtx = NULL;
            std::map<CLIENTIDTYPE, ClientCnCtx* >::iterator it = clientCtxMap.find(clientId);
            if( it != clientCtxMap.end() )
            {
                clientCtx = it->second;
            }

            return clientCtx;
        }

        void freeCtx(ClientCnCtx * clientCtx)
        {
            //
            clientCtxMap.erase(clientCtx->clientId);
            //
            clientCtx->conn->cnCtx = NULL;
            //
            clientCtx->conn = NULL;
            clientCtx->logicServerMap.clear();
            clientFreeVec.push_back(clientCtx);
            //
        }

    private:
        std::vector<ClientCnCtx *> clientFreeVec;
        std::map<CLIENTIDTYPE, ClientCnCtx* > clientCtxMap;
};

struct HandlerCtx
{
    int handlerState;
    int dataLen;
    ConnCtx * conn;
    void * msg;
    char dataValue[BufSize];
};

class HandlerCtxMgr
{
    public:
        HandlerCtxMgr()
        {

        }

        ~HandlerCtxMgr()
        {
            for( uint32_t i=0; i<handlerFreeVec.size(); i++)
            {
                 HandlerCtx * handlerCtx = handlerFreeVec[i];
                 free(handlerCtx);
                 handlerFreeVec[i] = NULL;
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
            }

            return handlerCtx;
        }

        void freeCtx( HandlerCtx * handlerCtx )
        {
            handlerCtx->dataLen = 0;
            handlerCtx->conn = NULL;
            handlerCtx->msg = NULL;
            handlerFreeVec.push_back(handlerCtx);
        }

    private:
        std::vector<HandlerCtx *> handlerFreeVec;

    public:
        static const int HandlerCtxSize = sizeof(HandlerCtx);
};

class ClientMgrMoudle
{
    public:
        ClientMgrMoudle()
        {
            clientConnMgr = new ConnCtxMgr();
            clientCtxMgr  = new ClientCnCtxMgr();
            handlerCtxMgr = new HandlerCtxMgr();
        }

        virtual ~ClientMgrMoudle()
        {
            delete clientConnMgr;
            delete clientCtxMgr;
            delete handlerCtxMgr;
        }

        virtual void initClientCtx(ClientCnCtx * ctx) = 0;

        virtual ConnCtx * getClientConn( int nfd, int connType, void * serverCtx, struct event_base * base, int state )
        {
            ConnCtx * cliConn = NULL;
            cliConn = clientConnMgr->getCtx( nfd, connType, serverCtx, base, state);
            cliConn->connCtxMgr = clientConnMgr;
            return cliConn;
        }

        virtual ConnCtx * getClientConn( int index )
        {
            ConnCtx * cliConn = NULL;
            cliConn = clientConnMgr->getCtx(index);
            return cliConn;
        }

        virtual ClientCnCtx * getClientCnCtx( CLIENTIDTYPE clientId )
        {
            return clientCtxMgr->getCtx(clientId);
        }

        virtual ClientCnCtx * addClient(ConnCtx * conn, CLIENTIDTYPE clientId)
        {
            conn->id = clientId;
            ClientCnCtx * nctx = clientCtxMgr->getCtx(conn);
            conn->cnCtx = (void *) nctx;
            nctx->conn = conn;
            initClientCtx(nctx);
            return nctx;
        }

        virtual void delClient(ConnCtx * conn)
        {
            if(conn->connType != ConnH5Client)
            {
                return;
            }

            ClientCnCtx * clientCtx = (ClientCnCtx *) conn->cnCtx;

            if(clientCtx != NULL)
            {
                clientCtxMgr->freeCtx(clientCtx);
            }
        }

        virtual void clientError(int errCode, ConnCtx * conn)
        {
            delClient(conn);
            conn->connCtxMgr->connError(errCode, conn);
        }

    protected:
        ConnCtxMgr * clientConnMgr;
        ClientCnCtxMgr * clientCtxMgr;
        HandlerCtxMgr * handlerCtxMgr;
};

class ExtraInterface : public DataCorrespondent, public TimerEventMoulde, public ClientMgrMoudle
{
    public:
        virtual ~ExtraInterface()
        {

        }
        //
        virtual void prepareRead(ConnCtx * conn) = 0;
        virtual char * readData(ConnCtx * conn, int & rLen) = 0;
        virtual void readDataEnd(ConnCtx * conn, int lLen) = 0;
        virtual BufCtx * writeData(ConnCtx * conn, int wLen) = 0;

        //
        virtual int addTimer(ConnCtx * conn, TimerEventHandler * handler) = 0;
        virtual int delTimer(TimerEventHandler * handler) = 0;
        virtual int resetTimer(TimerEventHandler * handler) = 0;

        //
        virtual void initClientCtx(ClientCnCtx * ctx) = 0;
        virtual ConnCtx *getServerConnCtx( int serverInd ) = 0;
        //
        virtual void serverConnError(int errCode, ConnCtx * svrConn) = 0;

        //
        virtual void initConnByMsg(int handlerState, void *msg, ConnCtx * conn, void * resData) = 0;
};

#endif // CONNIMPLST_HPP_INCLUDED

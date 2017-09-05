#ifndef SLAVESERVER_H
#define SLAVESERVER_H

#include "basicHeader/MsgImplSt.hpp"
#include "proto/ProtoMsgHeader.h"

struct NotifyCtx
{
    int notifyType;
    int fd;
};

enum NotifyType
{
    NotifyConnect = 1,
};

class LogicServerMgr
{
    public:
        LogicServerMgr()
        {
            masterIndex = 0;
        }
        ~LogicServerMgr()
        {

        }

        void addMasterServer(int ind, ConnCtx * conn)
        {
            masterIndex = ind;
            logicServerMap[ind] = conn;
        }

        void addLogicServer(int ind, ConnCtx * conn)
        {
            logicServerMap[ind] = conn;
        }

        void delMasterSerevr()
        {
            delLogicServer(masterIndex);
            masterIndex = 0;
        }

        void delLogicServer( int ind )
        {
            std::map<int, ConnCtx *>::iterator iter = logicServerMap.find(ind);
            if(iter != logicServerMap.end())
            {
                logicServerMap.erase(iter);
            }
        }

        ConnCtx * getMasterSerevr()
        {
            return getLogicServer(masterIndex);
        }

        ConnCtx * getLogicServer( int ind )
        {
            ConnCtx * conn = NULL;
            std::map<int, ConnCtx *>::iterator iter = logicServerMap.find(ind);
            if(iter != logicServerMap.end())
            {
                conn = iter->second;
            }
            return conn;
        }

    private:
         // key : serverId  master = 0
        int masterIndex;
        std::map<int, ConnCtx *> logicServerMap;
};

class SlaveServer : public ExtraInterface
{
    public:
        SlaveServer();
        SlaveServer(int index);
        virtual ~SlaveServer();

        void initialServer(std::string masterLgIp, int masterLgPort);
        ConnCtx * connectLogicServer(std::string masterLgIp, int masterLgPort, InterComMsg * msg);
        int getNotifyFd();
        pthread_t getThreadId();

    public:
        //
        virtual void prepareRead(ConnCtx * conn);
        virtual char * readData(ConnCtx * conn, int & rLen);
        virtual void readDataEnd(ConnCtx * conn, int lLen);
        virtual BufCtx * writeData(ConnCtx * conn, int wLen);
        //
        virtual int addTimer(ConnCtx * conn, TimerEventHandler * handler);
        virtual int delTimer(TimerEventHandler * handler);
        virtual int resetTimer(TimerEventHandler * handler);
        //
        virtual void initClientCtx(ClientCnCtx * ctx);
        virtual ConnCtx *getServerConnCtx(int serverInd);

        virtual void serverConnError(int errCode, ConnCtx * svrConn);
        //
        virtual void initConnByMsg(int handlerState, void *msg, ConnCtx * conn, void * resData);
    private:
        //
        void addReadEvent( ConnCtx * conn );
        void addWriteEvent( ConnCtx * conn );
        //

        static void * threadProcess( void  * ctx );
        static void readEventCallBack( int fd, short eventType, void * ctx );
        static void writeEventCallBack( int fd, short eventType, void * ctx );
        static void timerEventCallBack( int fd, short eventType, void * ctx );
        static void notifyReadEventCallBack( int fd, short eventType, void * ctx );

        void onThread();
        void onReadEvent(ConnCtx * conn);
        void onWriteEvent(ConnCtx * conn);
        void onTimerEvent(ConnCtx * conn, TimerEventHandler * handler);
        void onNotifyEvent();
        void handleNotify( NotifyCtx * notify );
        //
    private:
        int serverId;
        pthread_t threadId;

        int notifyRecFd;
        int notifySendFd;

        struct event_base * eventBase;
        struct event notifyEvent;

        ConnCtxMgr * serverConnMgr;
        LogicServerMgr * logicSvrMgr;
};

//
class OutTimerHandler : public TimerEventHandler
{
    public:
        OutTimerHandler()
        {
            errCode = 0;
        }

        virtual void timerHandle()
        {
            ClientMgrMoudle * clientMoudle = (ClientMgrMoudle *) ((SlaveServer *)(conn->serverCtx));
            clientMoudle->clientError(errCode,conn);
        }

        virtual void timerEndHandle()
        {

        }

    public:
        int errCode;
};

class ReconnectHandler : public TimerEventHandler
{
    public:
        ReconnectHandler()
        {
        }

        virtual void timerHandle()
        {
            int suc = 0;

            if( conn->fd == 0 )
            {
                int nfd = 0;

                if( (nfd = socket(AF_INET, SOCK_STREAM, 0))  < 0)
                {
                    perror(" can not create socket \n");
                    exit(0);
                }
                evutil_make_socket_nonblocking( nfd );
                conn->fd = nfd;
            }

            int res = connect(conn->fd,  (struct sockaddr *) ServerCnCtx::getAddrByConn(conn), sizeof(sockaddr_in));

            if( res == 0 || errno == EISCONN )
            {
                suc = 1;
            }
            else
            {
                printf("connect failure errno: %s!\n", strerror(errno));

                if( errno != EINPROGRESS )
                {
                    close(conn->fd);
                    conn->fd = 0;
                }
            }


            if( suc == 1 )
            {
                //
                rpNum = 0;
                state = 0;

                conn->state = ConnEnableState;

                ExtraInterface * ein = (ExtraInterface *) (conn->serverCtx);
                ein->prepareRead(conn);

                //
                int wlen = MsgMgr::GateWayConnectMSTLen + MsgMgr::InterComMsgLen;
                BufCtx * wbufCtx = ein->writeData(conn, wlen);
                InterComMsg * msg = MsgMgr::buildInterComMsg(wbufCtx, GateWayMsgType, ReqGWConnectMS, conn->index, 0, 0, wlen, NULL, 0);
                //
                MsgMgr::buildGateWayConnectMST(msg, ReqGWConnectMS, 0, ServerCnCtx::getIdByConn(conn));
                //
                printf("reconnectHandler success........!!!\n");
            }
        }

        virtual void timerEndHandle()
        {
            if( conn->fd == 0 )
            {
                conn->connCtxMgr->connError(0, conn);
            }
        }

        static void setTimerHandlerToConn(ConnCtx * conn, int sec )
        {
            ReconnectHandler * timerHandler = new ReconnectHandler();
            timerHandler->sec = sec;
            timerHandler->conn = conn;
            conn->timerHandler = (TimerEventHandler *) timerHandler;
        }
};

#endif // SLAVESERVER_H

#ifndef MASTERSERVER_H
#define MASTERSERVER_H

#include "basicHeader/ServerEventImplSt.hpp"
#include "AsynOperationThread.h"
//
class ServerConnCtxMgr
{
    public:
        ServerConnCtxMgr()
        {
            ssIndex = 1;
            gwIndex = 1;
        }

        ~ServerConnCtxMgr()
        {
            std::map<int, SlaveServerCtx *>::iterator it1;
            for( it1 = slaveSvrMap.begin(); it1 != slaveSvrMap.end(); it1++ )
            {
                SlaveServerCtx * ssCtx = it1->second;
                if( ssCtx != NULL )
                {
                    free(ssCtx);
                }
            }
            slaveSvrMap.clear();

            std::map<int, GateWayCtx *>::iterator it2;
            for( it2 = gateWayMap.begin(); it2 != gateWayMap.end(); it2++ )
            {
                GateWayCtx * gwCtx = it2->second;
                if( gwCtx != NULL )
                {
                    free(gwCtx);
                }
            }
            gateWayMap.clear();
        }

        int addSServerCtx( ConnCtx * conn, int sid, char *ip, int port )
        {
            SlaveServerCtx * ssCtx = (SlaveServerCtx *) malloc(sizeof(SlaveServerCtx));
            ssCtx->conn =  conn;
            ssCtx->serverId = sid;
            ssCtx->slaveSvrIndex = ssIndex;
            ssCtx->connNum = 0;
            ssCtx->weight = 0;
            strcpy(ssCtx->ip, ip);
            ssCtx->port = port;

            slaveSvrMap[ssIndex] = ssCtx;

            ServerDataCtx * extraData = (ServerDataCtx *) conn->eDataCtx;
            extraData->ctxIndex = ssIndex;

            ssIndex++;
            return ssCtx->slaveSvrIndex;
        }

        SlaveServerCtx * getSServerCtx( int ind )
        {
            SlaveServerCtx * ctx = NULL;
            std::map<int, SlaveServerCtx *>::iterator iter = slaveSvrMap.find(ind);
            if(iter != slaveSvrMap.end())
            {
               ctx = iter->second;
            }

            return ctx;
        }

        SlaveServerCtx * getSServerCtx( ConnCtx * conn )
        {
            SlaveServerCtx * ctx = NULL;
            ServerDataCtx * extraData = (ServerDataCtx *) conn->eDataCtx;
            std::map<int, SlaveServerCtx *>::iterator iter = slaveSvrMap.find(extraData->ctxIndex);
            if(iter != slaveSvrMap.end())
            {
               ctx = iter->second;
            }

            return ctx;
        }

        void delSServerCtx( ConnCtx * conn )
        {
            ServerDataCtx * extraData = (ServerDataCtx *) conn->eDataCtx;
            std::map<int, SlaveServerCtx *>::iterator iter = slaveSvrMap.find(extraData->ctxIndex);
            if(iter != slaveSvrMap.end())
            {
                SlaveServerCtx * ctx = iter->second;
                free(ctx);
                slaveSvrMap.erase(iter);
            }

            extraData->ctxIndex = 0;
        }

        int addGateWayCtx( ConnCtx * conn, int sid )
        {
            GateWayCtx * gwCtx = (GateWayCtx *) malloc(sizeof(GateWayCtx));
            gwCtx->conn = conn;
            gwCtx->serverId = sid;
            gwCtx->gateWayIndex = gwIndex;
            gateWayMap[gwIndex] = gwCtx;

            ServerDataCtx * extraData = (ServerDataCtx *) conn->eDataCtx;
            extraData->ctxIndex = gwIndex;

            gwIndex++;
            return gwCtx->gateWayIndex;
        }

        GateWayCtx * getGateWayCtx( int ind )
        {
            GateWayCtx * ctx = NULL;
            std::map<int, GateWayCtx *>::iterator iter = gateWayMap.find(ind);
            if(iter != gateWayMap.end())
            {
               ctx = iter->second;
            }

            return ctx;
        }

        GateWayCtx * getGateWayCtx( ConnCtx * conn )
        {
            GateWayCtx * ctx = NULL;
            ServerDataCtx * extraData = (ServerDataCtx *) conn->eDataCtx;
            std::map<int, GateWayCtx *>::iterator iter = gateWayMap.find(extraData->ctxIndex);
            if(iter != gateWayMap.end())
            {
               ctx = iter->second;
            }

            return ctx;
        }

        void delGateWayCtx( ConnCtx * conn )
        {
            ServerDataCtx * extraData = (ServerDataCtx *) conn->eDataCtx;
            std::map<int, GateWayCtx *>::iterator iter = gateWayMap.find(extraData->ctxIndex);
            if(iter != gateWayMap.end())
            {
                GateWayCtx * ctx = iter->second;
                free(ctx);
                gateWayMap.erase(iter);
            }

            extraData->ctxIndex = 0;
        }

        void sendMsgToAllGateWay(SendMsgInfo * msgInfo, AsynOperationThread * asynThread)
        {
            std::map<int, GateWayCtx *>::iterator iter;
            for( iter = gateWayMap.begin(); iter != gateWayMap.end(); iter++ )
            {
                GateWayCtx * gwCtx = iter->second;
                if( gwCtx != NULL )
                {
                    ConnCtx * conn = gwCtx->conn;
                    ServerDataCtx * extraData = (ServerDataCtx *) (conn->eDataCtx);
                    asynThread->asynWriteMsg(msgInfo->sendMsg, extraData->writeInd);
                }
            }
        }

        void sendAllSSMsgToConn(int writeInd, AsynOperationThread * asynThread)
        {
            int wlen = MsgMgr::SlaveSvrConnectMSTLen + MsgMgr::InterComMsgLen;
            char msgBuf[1024];

            std::map<int, SlaveServerCtx *>::iterator iter;
            for( iter = slaveSvrMap.begin(); iter != slaveSvrMap.end(); iter++ )
            {
                SlaveServerCtx * ssCtx = iter->second;
                if( ssCtx != NULL )
                {
                    memset(msgBuf, 0, 1024);
                    InterComMsg * msg = MsgMgr::buildInterComMsg(msgBuf, GateWayMsgType, ReqGWConnectSS, 0, 0, 0, 0, wlen);
                    //
                    MsgMgr::buildSlaveSvrConnectMST(msg->data, ReqGWConnectSS, ssCtx->serverId, ssCtx->slaveSvrIndex, 16, ssCtx->ip, ssCtx->port);

                    asynThread->asynWriteMsg(msg, writeInd);
                }
            }
        }

    private:
        int ssIndex;
        std::map<int, SlaveServerCtx *>  slaveSvrMap;
        int gwIndex;
        std::map<int, GateWayCtx *>  gateWayMap;
};

//
class MasterServer : public ServerExtraInterface
{
    public:
        MasterServer();
        virtual ~MasterServer();
        void initialServer(std::string ip, int port);
        //
        virtual void listenEventHandler(ConnCtx * conn);
        virtual void writeEventHandler(ConnCtx * conn);
        virtual void connError(int errCode, ConnCtx * conn);
        //
        virtual int addTimer(ConnCtx * conn, TimerEventHandler * handler);
        virtual int delTimer(TimerEventHandler * handler);
        virtual int resetTimer(TimerEventHandler * handler);

        //
        virtual void initMsgMoudle();
        virtual void initServerByMsg(InterComMsg *msg, ConnCtx * conn, void * resData);
        //
        virtual void initLogicMoudle();
        //
        virtual int diapatchServerInd(int blockId);
        virtual void addServerInd(int ind);

        virtual void sendMsgToServer(SendMsgInfo * sendMsg);

    private:
        //
        int asynFd;
        ConnCtx * asynConn;
        AsynOperationThread * asynThread;
        ServerConnCtxMgr * svrConnCtxMgr;

};

#endif // MASTERSERVER_H

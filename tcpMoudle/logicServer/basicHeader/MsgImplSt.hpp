#ifndef MSGIMPLST_HPP_INCLUDED
#define MSGIMPLST_HPP_INCLUDED

#include "ConnImplSt.hpp"

enum ServerMsgType
{
    GateWayMsgType = 1,

    MainServerMsgType = 2,

    SlaveServerMsgType = 3,

    ClientLogicMsgType = 4,

    ClientGateWayMsgType = 5,
};

enum MsgMouldeType
{
    MainInitMouldeType = 0,
    LogicMouldeType = 1,
};

enum LogicBlockMoulde
{
    ShareBlockMoulde = 1,
    IndenpentBlockMoulde = 2,
};

enum GateWayHandleType
{
    ReqGWConnectMS = 11001,
    ReqGWConnectSS,
    ReqSetCliSvrInd,

    ResGWConnectMS = 12001,
    ResGWConnectSS,
};

enum ClientLogicHandleType
{
    ReqCliConnectLS = 41001,
    ReqCliDisConnectLS,
    ReqConnectShareBlock,

    ResCliConnectLS = 42001,
    ResCliDisConnectLS,
    ResConnectShareBlock,
};

enum SlaveServerHandleType
{
    ReqSSConnectMS = 31001,
    ReqCliLogicMS,

    ResSSConnectMS = 32001,
    ResCliLogicMS,
};


enum SendMsgType
{
    SendMsgToConn,
    SendMsgToAllGWServer,
    SendAllSSMsgToConn,
};

struct InterComMsg
{
    int msgMoudleType;
    int logicMoudleId;
    int connIndex;
    CLIENTIDTYPE uId;
    int handleType;
    int handlerInd;
    int msgLen;
    char data[];
};

struct GateWayConnectMST
{
    int handleType;
    int serverId;
    int logicSvrIndex;
};

struct SlaveSvrConnectMST
{
    int handleType;
    int serverId;
    int logicSvrIndex;
    char ip[16];
    int prot;
};

struct SendMsgInfo
{
    int msgType;
    ConnCtx * conn;
    InterComMsg * sendMsg;
    char msgData[SendDataSize];
};

struct CliConnectMST
{
    int connIndex;
    int logicSvrIndex;
    CLIENTIDTYPE uId;
    int logicBlockId;
};

struct CliLogicMST
{
    int handleType;
    int serverId;
    CLIENTIDTYPE uId;
    int logicBlockId;
};

class MsgMgr
{
    public:
        static void addICMsgToBufCtx(InterComMsg * SendMsg, BufCtx * bufCtx)
        {
            InterComMsg * tmsg = (InterComMsg *) (bufCtx->buf+bufCtx->bufLen);

            tmsg->msgMoudleType = SendMsg->msgMoudleType;
            tmsg->logicMoudleId = SendMsg->logicMoudleId;
            tmsg->connIndex = SendMsg->connIndex;
            tmsg->uId = SendMsg->uId;
            tmsg->handleType = SendMsg->handleType;
            tmsg->handlerInd = SendMsg->handlerInd;
            tmsg->msgLen = SendMsg->msgLen;

            memcpy(tmsg->data, SendMsg->data, SendMsg->msgLen);

            bufCtx->bufLen += tmsg->msgLen + InterComMsgLen;
        }

        static void addICProtoMsgToBufCtx(InterComMsg * head, PROTOMESSAGE * proto, BufCtx * bufCtx)
        {
            InterComMsg * msg = (InterComMsg *) (bufCtx->buf+bufCtx->bufLen);

            msg->msgMoudleType = head->msgMoudleType;
            msg->logicMoudleId = head->logicMoudleId;
            msg->connIndex = head->connIndex;
            msg->uId = head->uId;
            msg->handleType = head->handleType;
            msg->handlerInd = head->handlerInd;

            int len = proto->ByteSize();
            msg->msgLen = len;
            proto->SerializeToArray(msg->data, len);

            bufCtx->bufLen += len + InterComMsgLen;
        }

        static void makeSendDataInfo( SendMsgInfo * info, int msgType, ConnCtx * conn, InterComMsg * head, char * data, int len )
        {
            info->msgType = msgType;
            info->conn = conn;
            InterComMsg * msg = (InterComMsg *)(info->msgData);
            msg->msgMoudleType = head->msgMoudleType;
            msg->logicMoudleId = head->logicMoudleId;
            msg->connIndex = head->connIndex;
            msg->uId = head->uId;
            msg->handleType = head->handleType;
            msg->handlerInd = head->handlerInd;
            msg->msgLen = len;
            memcpy(msg->data, data, len);

            info->sendMsg = msg;
        }

        static InterComMsg * buildInterComMsg(char * msgBuf, int mtype, int htype, int hind, int ind, int mid, int uid, int mlen)
        {
            InterComMsg * msg = (InterComMsg *) (msgBuf);
            msg->msgMoudleType = mtype;
            msg->handleType = htype;
            msg->handlerInd = hind;
            msg->connIndex = ind;
            msg->logicMoudleId = mid;
            msg->uId = uid;
            msg->msgLen = mlen;
            return msg;
        }

        static GateWayConnectMST * buildGateWayConnectMST(char * mbuf, int htype, int ind, int id)
        {
            GateWayConnectMST * mst = (GateWayConnectMST *) mbuf;
            mst->handleType = htype;
            mst->logicSvrIndex = ind;
            mst->serverId = id;
            return mst;
        }

        static SlaveSvrConnectMST * buildSlaveSvrConnectMST(char * mbuf, int htype,int id, int ind, int ilen, char * ip, int port)
        {
            SlaveSvrConnectMST * mst = (SlaveSvrConnectMST *) mbuf;
            mst->handleType = htype;

            mst->serverId = id;
            mst->logicSvrIndex = ind;
            if( ilen > 0 )
            {
                strcpy(mst->ip, ip);
            }
            mst->prot = port;
        }

        static CliConnectMST * buildCliConnectMST(char * mbuf, int cInd, int sInd, CLIENTIDTYPE uId, int lInd)
        {
            CliConnectMST * mst = (CliConnectMST *) mbuf;
            mst->connIndex = cInd;
            mst->logicSvrIndex = sInd;
            mst->uId = uId;
            mst->logicBlockId = lInd;
        }

        static CliLogicMST * buildCliLogicMST(char * mbuf, int cInd, CLIENTIDTYPE uId, int sInd, int lInd)
        {
            CliLogicMST * mst = (CliLogicMST *) mbuf;
            mst->handleType = cInd;
            mst->uId = uId;
            mst->serverId = sInd;
            mst->logicBlockId = lInd;
        }

    public:
        static const int InterComMsgLen = sizeof(InterComMsg);
        static const int GateWayConnectMSTLen = sizeof(GateWayConnectMST);
        static const int SlaveSvrConnectMSTLen = sizeof(SlaveSvrConnectMST);
        static const int CliConnectMSTLen = sizeof(CliConnectMST);
        static const int CliLogicMSTLen = sizeof(CliLogicMST);
};

//
class MsgServerInterface
{
    virtual void initServerByMsg(InterComMsg *msg, ConnCtx * conn, void * resData) = 0;
    virtual void sendMsgToServer(SendMsgInfo * sendMsg) = 0;
};

//
class MsgMoudleInterface
{
    public:
        MsgMoudleInterface(){}
        virtual ~MsgMoudleInterface(){}

        virtual void moudleHandler( ConnCtx * conn, InterComMsg * inMsg ) = 0;
};

class MsgMoudleMgr : public MsgServerInterface
{
    public:
        MsgMoudleMgr()
        {
        }

        virtual ~MsgMoudleMgr(){}
        //
        virtual void initMsgMoudle() = 0;
        //
        void registerMsgMoudle(int type, MsgMoudleInterface * moudle)
        {
            moudleInterfaceMap.insert(std::pair<int, MsgMoudleInterface *>(type, moudle));
        }

        MsgMoudleInterface * unregisterMsgMoudle(int type)
        {
            MsgMoudleInterface * inf = NULL;
            std::map<int, MsgMoudleInterface * >::iterator it = moudleInterfaceMap.find(type);
            if( it != moudleInterfaceMap.end() )
            {
                inf = it->second;
            }

            moudleInterfaceMap.erase(type);
            return inf;
        }

        virtual void handleInterMsg( ConnCtx * conn, InterComMsg * inMsg )
        {
            int moudleType = inMsg->msgMoudleType;

            std::map<int, MsgMoudleInterface * >::iterator it = moudleInterfaceMap.find(moudleType);
            if( it != moudleInterfaceMap.end() )
            {
                MsgMoudleInterface * inf = it->second;
                inf->moudleHandler(conn, inMsg);
            }
            else
            {
                printf("error moudleType\n");
            }
        }

    public:
        std::map<int, MsgMoudleInterface *> moudleInterfaceMap;
};

#endif // MSGIMPLST_HPP_INCLUDED

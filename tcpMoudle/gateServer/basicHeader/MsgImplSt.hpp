#ifndef MSGIMPLST_HPP_INCLUDED
#define MSGIMPLST_HPP_INCLUDED

#include "ConnImplSt.hpp"

enum MsgMoudleType
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

enum GateWayHandleType
{
    ReqGWConnectMS = 11001,
    ReqGWConnectSS,
    ReqSetCliSvrInd,

    ResGWConnectMS = 12001,
    ResGWConnectSS,
};

enum MainServerHandleType
{

};

enum SlaveServerHandleType
{
    ReqSSConnectMS = 31001,

    ResSSConnectMS = 32001,
};

enum ClientLogicHandleType
{
    ReqCliConnectLS = 41001,
    ReqCliDisConnectLS,

    ResCliConnectLS = 42001,
    ResCliDisConnectLS,
};

enum ClientGateWayHandlerType
{
    ReqCliConnectGw = 51001,

    ResCliConnectGw = 52001,
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

struct ClientToGateMsg
{
    int msgType;
    int logicBlockId;
    char codeKey[4];
    int handlerState;
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

struct CliConnectMST
{
    int connIndex;
    int logicSvrIndex;
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
            tmsg->handleType = SendMsg->handleType;
            tmsg->handlerInd = SendMsg->handlerInd;
            tmsg->connIndex = SendMsg->connIndex;
            tmsg->uId = SendMsg->uId;
            tmsg->msgLen = SendMsg->msgLen;

            memcpy(tmsg->data, SendMsg->data, SendMsg->msgLen);

            bufCtx->bufLen += tmsg->msgLen + InterComMsgLen;
        }

        static void addICProtoMsgToBufCtx(InterComMsg &head, PROTOMESSAGE * proto, BufCtx * bufCtx)
        {
            InterComMsg * msg = (InterComMsg *) (bufCtx->buf+bufCtx->bufLen);

            msg->msgMoudleType = head.msgMoudleType;
            msg->logicMoudleId = head.logicMoudleId;
            msg->handleType = head.handleType;
            msg->handlerInd = head.handlerInd;
            msg->connIndex = head.connIndex;
            msg->uId = head.uId;

            int len = proto->ByteSize();
            msg->msgLen = len;
            proto->SerializeToArray(msg->data, len);

            bufCtx->bufLen += len + InterComMsgLen;
        }

        static void addCGMsgToBufCtx(ClientToGateMsg &head, const char * data, int len, BufCtx * bufCtx)
        {
            ClientToGateMsg * msg = (ClientToGateMsg *) (bufCtx->buf+bufCtx->bufLen);

            msg->msgType = head.msgType;
            msg->logicBlockId = head.logicBlockId;
            msg->handlerState = head.handlerState;

            msg->msgLen = len;
            memcpy(msg->data, data, len);

            bufCtx->bufLen += len + ClientToGateMsgLen;
        }

        static void addCGProtoMsgToBufCtx(int mtype, int blockId, int hsate, PROTOMESSAGE * proto, BufCtx * bufCtx)
        {
            ClientToGateMsg * msg = (ClientToGateMsg *) (bufCtx->buf+bufCtx->bufLen);

            msg->msgType = mtype;
            msg->logicBlockId = blockId;
            msg->handlerState = hsate;

            int len = proto->ByteSize();
            msg->msgLen = len;
            proto->SerializeToArray(msg->data, len);

            bufCtx->bufLen += len + ClientToGateMsgLen;
        }

        static void setCGToGLWithBufCtx(InterComMsg &head, const ClientToGateMsg * cgMsg, BufCtx * bufCtx)
        {
            InterComMsg * msg = (InterComMsg *) (bufCtx->buf+bufCtx->bufLen);

            msg->connIndex = head.connIndex;
            msg->uId = head.uId;
            msg->msgMoudleType = head.msgMoudleType;
            msg->handleType = head.handleType;

            msg->logicMoudleId = cgMsg->logicBlockId;
            msg->msgLen = cgMsg->msgLen;

            memcpy(msg->data, cgMsg->data, cgMsg->msgLen);

            bufCtx->bufLen += msg->msgLen + InterComMsgLen;
        }

        static void setGLToCGWithBufCtx(ClientToGateMsg &head, const InterComMsg * glMsg, BufCtx * bufCtx)
        {
            ClientToGateMsg * msg = (ClientToGateMsg *) (bufCtx->buf+bufCtx->bufLen);

            msg->msgType = glMsg->msgMoudleType;
            msg->logicBlockId = glMsg->logicMoudleId;
            msg->handlerState = glMsg->handleType;
            msg->msgLen = glMsg->msgLen;

            memcpy(msg->data, glMsg->data, glMsg->msgLen);

            bufCtx->bufLen += msg->msgLen + ClientToGateMsgLen;
        }

        static void setH5MsgToBufCtx(const char * data, int len, BufCtx * bufCtx)
        {
            char * msg = (char *) (bufCtx->buf+bufCtx->bufLen);
            memcpy(msg, data, len);
            bufCtx->bufLen += len;
        }

        static InterComMsg * buildInterComMsg(BufCtx * bufCtx, int mtype, int htype, int ind, int mid, int uid, int mlen, char * data, int dlen)
        {
            InterComMsg * msg = (InterComMsg *) (bufCtx->buf+bufCtx->bufLen);
            msg->msgMoudleType = mtype;
            msg->handleType = htype;
            msg->connIndex = ind;
            msg->logicMoudleId = mid;
            msg->uId = uid;
            msg->msgLen = dlen;
            if(dlen > 0)
            {
                memcpy(msg->data, data, dlen);
            }
            bufCtx->bufLen += mlen;
            return msg;
        }

        static GateWayConnectMST * buildGateWayConnectMST(InterComMsg * msg, int htype, int ind, int id)
        {
            GateWayConnectMST * mst = (GateWayConnectMST *) (msg->data);
            mst->handleType = htype;
            mst->logicSvrIndex = ind;
            mst->serverId = id;
            msg->msgLen = GateWayConnectMSTLen;
            return mst;
        }

        static SlaveSvrConnectMST * buildSlaveSvrConnectMST(char * mbuf, int htype, int id, int ind, int ilen, char * ip, int port)
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

        static CliConnectMST * buildCliConnectMST(InterComMsg * msg, int cInd, int sInd, CLIENTIDTYPE uId, int lInd)
        {
            CliConnectMST * mst = (CliConnectMST *) (msg->data);
            mst->connIndex = cInd;
            mst->logicSvrIndex = sInd;
            mst->uId = uId;
            mst->logicBlockId = lInd;
            msg->msgLen = CliConnectMSTLen;
        }

    public:
        static const int InterComMsgLen = sizeof(InterComMsg);
        static const int ClientToGateMsgLen = sizeof(ClientToGateMsg);
        static const int GateWayConnectMSTLen = sizeof(GateWayConnectMST);
        static const int CliConnectMSTLen = sizeof(CliConnectMST);
};

#endif // MSGIMPLST_HPP_INCLUDED

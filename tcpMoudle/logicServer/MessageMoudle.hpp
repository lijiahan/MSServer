#ifndef MESSAGEMOUDLE_HPP_INCLUDED
#define MESSAGEMOUDLE_HPP_INCLUDED

#include "basicHeader/ServerEventImplSt.hpp"

class MasterInitMsgMoudle : public MsgMoudleInterface
{
    public:
        MasterInitMsgMoudle()
        {

        }

        virtual ~MasterInitMsgMoudle()
        {

        }

        virtual void moudleHandler( ConnCtx * conn, InterComMsg * iMsg )
        {
            switch(iMsg->handleType)
            {
                case ReqGWConnectMS:
                {
                    GateWayConnectMST * mst = (GateWayConnectMST *) (iMsg->data);

                    ServerExtraInterface * ein = (ServerExtraInterface *) (conn->serverCtx);
                    ein->initServerByMsg(iMsg, conn, (void *) mst);
                    printf("ReqGWConnectMS:fd : %d, gateServer id: %d, connect master!!\n", conn->fd, mst->serverId);
                    //
                    resGWConnectMS(conn, iMsg);

                    break;
                }

                case ReqSSConnectMS:
                {
                    SlaveSvrConnectMST * mst = (SlaveSvrConnectMST *) (iMsg->data);

                    ServerExtraInterface * ein = (ServerExtraInterface *) (conn->serverCtx);
                    ein->initServerByMsg(iMsg, conn, (void *) mst);

                    // re
                    printf("ReqSSConnectMS: fd : %d connect logic!!\n", conn->fd);

                    resSSConnectMS(conn, iMsg);
                    break;
                }

                //
                case ReqCliConnectLS:
                {
                    //
                    ServerDataCtx * extraData = (ServerDataCtx *) (conn->eDataCtx);
                    ServerExtraInterface * ein = (ServerExtraInterface *) (conn->serverCtx);
                    ClientCtx * client = ein->getNewClientCtx(iMsg->uId, conn, iMsg->connIndex);

                    //
                    char mbuf[256];
                    memset(mbuf, 0, 256);
                    InterComMsg * rm = MsgMgr::buildInterComMsg(mbuf, ClientLogicMsgType, ResCliConnectLS,
                                             0, client->clConnCtx.connIndex, 0, client->uid, 0);
                    //
                    ConnectMsg::ClientConnect resM;
                    resM.set_uid(client->uid);
                    int len = resM.ByteSize();
                    resM.SerializeToArray(rm->data, len);
                    rm->msgLen = len;

                    SendMsgInfo msgInfo;
                    MsgMgr::makeSendDataInfo(&msgInfo, SendMsgToConn, conn, rm, rm->data, rm->msgLen);
                    ein->sendMsgToServer(&msgInfo);
                    printf("ReqCliConnectLS  client uid = %d  len = %d\n", client->uid, msgInfo.sendMsg->msgLen);

                    break;
                }


                //
                case ResSSConnectMS:
                {
                    ServerExtraInterface * ein = (ServerExtraInterface *) (conn->serverCtx);
                    ein->initServerByMsg(iMsg, conn, NULL);
                    break;
                }

                default:
                    break;
        }
    }

    void resGWConnectMS( ConnCtx * conn, InterComMsg * iMsg )
    {
        GateWayConnectMST * mst = (GateWayConnectMST *) (iMsg->data);
         // re
        iMsg->msgMoudleType = GateWayMsgType;
        iMsg->handleType = ResGWConnectMS;
        mst->handleType = ResGWConnectMS;

        SendMsgInfo msgInfo;
        MsgMgr::makeSendDataInfo(&msgInfo, SendMsgToConn, conn, iMsg, iMsg->data, iMsg->msgLen);

        ServerExtraInterface * ein = (ServerExtraInterface *) (conn->serverCtx);
        ein->sendMsgToServer(&msgInfo);
        //
        msgInfo.msgType = SendAllSSMsgToConn;
        ein->sendMsgToServer(&msgInfo);
    }

    void resSSConnectMS( ConnCtx * conn, InterComMsg * iMsg )
    {
        iMsg->handleType = ResSSConnectMS;

        SendMsgInfo msgInfo;
        MsgMgr::makeSendDataInfo(&msgInfo, SendMsgToConn, conn, iMsg, iMsg->data, iMsg->msgLen);

        ServerExtraInterface * ein = (ServerExtraInterface *) (conn->serverCtx);
        ein->sendMsgToServer(&msgInfo);

        //
        //send to gateway
        msgInfo.msgType = SendMsgToAllGWServer;
        msgInfo.sendMsg->msgMoudleType = GateWayMsgType;
        msgInfo.sendMsg->handleType = ReqGWConnectSS;

        ein->sendMsgToServer(&msgInfo);
    }
};

class MasterLogicMsgMoudle : public MsgMoudleInterface
{
    public:
        MasterLogicMsgMoudle()
        {

        }

        virtual ~MasterLogicMsgMoudle()
        {

        }

        virtual void moudleHandler( ConnCtx * conn, InterComMsg * iMsg )
        {
            //
            ServerExtraInterface * ein = (ServerExtraInterface *) (conn->serverCtx);
            ein->logicMoudle(conn, iMsg);
        }
};

class SlaveInitMsgMoudle : public MsgMoudleInterface
{
    public:
        SlaveInitMsgMoudle()
        {

        }

        virtual ~SlaveInitMsgMoudle()
        {

        }

        virtual void moudleHandler( ConnCtx * conn, InterComMsg * iMsg )
        {
            switch(iMsg->handleType)
            {
                case ReqGWConnectSS:
                {
                    SlaveSvrConnectMST * mst = (SlaveSvrConnectMST *) (iMsg->data);

                    ServerExtraInterface * ein = (ServerExtraInterface *) (conn->serverCtx);
                    ein->initServerByMsg(iMsg, conn, (void *) mst);
                    printf("ReqGWConnectSS: fd:%d, gateServer index: %d, connect master!!\n", conn->fd, mst->serverId);
                    //
                    resGWConnectSS(conn, iMsg);
                    break;
                }

                default:
                    break;
            }
        }

        void resGWConnectSS( ConnCtx * conn, InterComMsg * iMsg )
        {
            GateWayConnectMST * mst = (GateWayConnectMST *) (iMsg->data);
            // re
            iMsg->handleType = ResGWConnectSS;
            mst->handleType = ResGWConnectSS;

            SendMsgInfo msgInfo;
            MsgMgr::makeSendDataInfo(&msgInfo, SendMsgToConn, msgInfo->conn, iMsg, iMsg->data, iMsg->msgLen);

            ServerExtraInterface * ein = (ServerExtraInterface *) (conn->serverCtx);
            ein->sendMsgToServer(&msgInfo);
        }
};

class SlaveLogicMsgMoudle : public MsgMoudleInterface
{
    public:
        SlaveLogicMsgMoudle()
        {

        }

        virtual ~SlaveLogicMsgMoudle()
        {

        }

        virtual void moudleHandler( ConnCtx * conn, InterComMsg * iMsg )
        {
            //
            ServerExtraInterface * ein = (ServerExtraInterface *) (conn->serverCtx);
            ein->logicMoudle(conn, iMsg);
        }
};
#endif // MESSAGEMOUDLE_HPP_INCLUDED

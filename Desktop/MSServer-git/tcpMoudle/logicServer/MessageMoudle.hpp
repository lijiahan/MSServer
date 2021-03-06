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
                    ServerExtraInterface * ein = (ServerExtraInterface *) (conn->serverCtx);
                    ein->initServerByMsg(iMsg, conn, (void *) (iMsg->data));

                    //
                    resGWConnectMS(conn, iMsg);

                    break;
                }

                case ReqSSConnectMS:
                {
                    ServerExtraInterface * ein = (ServerExtraInterface *) (conn->serverCtx);
                    ein->initServerByMsg(iMsg, conn, (void *) (iMsg->data) );
                    // re
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
                    ClientConnect resM;
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
                case ResSSConnectMS:
                {
                    ServerExtraInterface * ein = (ServerExtraInterface *) (conn->serverCtx);
                    ein->initServerByMsg(iMsg, conn, (void *) (iMsg->data) );
                    //
                    break;
                }

                case ReqGWConnectSS:
                {
                    ServerExtraInterface * ein = (ServerExtraInterface *) (conn->serverCtx);
                    ein->initServerByMsg(iMsg, conn, (void *) (iMsg->data) );
                    //
                    break;
                }

                default:
                    break;
            }
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

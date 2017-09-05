#ifndef MESSAGEDISPOSER_HPP_INCLUDED
#define MESSAGEDISPOSER_HPP_INCLUDED

#include "ConnImplSt.hpp"


class LgServerMsgDisposer : public MessageDisposer
{
    public:
        virtual int readDataInConnBuf(ConnCtx * conn)
        {
            //
            ExtraInterface * dCorrespond = (ExtraInterface *) (conn->serverCtx);
            int rLen = 0;
            char * data = dCorrespond->readData(conn, rLen);

            if(data == NULL)
            {
                handleError(rLen, conn);
                dCorrespond->readDataEnd(conn, 0);
                return 0;
            }

            int llen = rLen;
            //
            if( llen > MsgMgr::InterComMsgLen )
            {
                char * msgbuf = data;
                while( 1 )
                {
                    InterComMsg * glMsg = (InterComMsg *) msgbuf;
                    int tlen = llen - MsgMgr::InterComMsgLen - glMsg->msgLen;

                    printf("tlen = %d llen = %d \n", tlen, llen);

                    if( tlen < 0 )
                    {
                        break;
                    }

                    llen = tlen;

                    handlerModule(glMsg->msgMoudleType, msgbuf, conn);

                    if( llen <= MsgMgr::InterComMsgLen )
                    {
                        break;
                    }

                    msgbuf = msgbuf + MsgMgr::InterComMsgLen + glMsg->msgLen;
                }
            }

            dCorrespond->readDataEnd(conn, llen);

            return llen;
        }

        virtual void handlerModule( int msgType, char * msgBuf, ConnCtx * conn )
        {
            switch( msgType )
            {
                case ClientLogicMsgType:
                    {
                        InterComMsg * glMsg = (InterComMsg *) msgBuf;
                        int ind = glMsg->connIndex;
                        ExtraInterface * clientMoudle = (ExtraInterface *) (conn->serverCtx);
                        ConnCtx * cliConn = clientMoudle->getClientConn(ind);

                        if(cliConn == NULL)
                        {
                            printf("MsgLogicToClient error!!\n");
                            return;
                        }

                        int wlen = glMsg->msgLen + MsgMgr::InterComMsgLen;
                        ExtraInterface * dCorrespond = (ExtraInterface *) (cliConn->serverCtx);
                        BufCtx * bufCtx = dCorrespond->writeData(cliConn, wlen);

                        if(bufCtx == NULL)
                        {
                            printf("can not write, conn is disable!!\n");
                            return;
                        }

                        ClientToGateMsg head;
                        MsgMgr::setGLToCGWithBufCtx(head, glMsg, bufCtx);
                        printf("********MsgLogicToClient:  conn ind:%d wbuf uid: %d \n",ind, glMsg->uId);

                        break;
                    }
                case GateWayMsgType:
                    {
                        handleLogicMsg(msgType, msgBuf, conn);
                        break;
                    }
            }
        }

        virtual void handleLogicMsg( int msgType, char * msgBuf, ConnCtx * conn )
        {
            InterComMsg * inMsg = (InterComMsg *) msgBuf;
            switch( inMsg->handleType )
            {
                case ResGWConnectMS:
                    {
                        GateWayConnectMST * msgSt = (GateWayConnectMST *) (inMsg->data);
                        printf("ResGWConnectMS ctxIndex: %d\n", msgSt->logicSvrIndex);
                        ExtraInterface * ein = (ExtraInterface *)(conn->serverCtx);
                        ein->initConnByMsg(ResGWConnectMS, (void *) msgSt, conn, NULL);
                        break;
                    }
                case ReqGWConnectSS:
                    {
                        SlaveSvrConnectMST * msgSt = (SlaveSvrConnectMST *) (inMsg->data);
                        printf("ReqGWConnectSS handleType: %d\n", msgSt->handleType);
                        ExtraInterface * ein = (ExtraInterface *)(conn->serverCtx);
                        ein->initConnByMsg(ReqGWConnectSS, (void *) inMsg, conn, NULL);
                        break;
                    }
                case ResGWConnectSS:
                    {
                        SlaveSvrConnectMST * msgSt = (SlaveSvrConnectMST *) (inMsg->data);
                        printf("ResGWConnectSS handleType: %d\n", msgSt->handleType);
                        ExtraInterface * ein = (ExtraInterface *)(conn->serverCtx);
                        ein->initConnByMsg(ReqGWConnectSS, (void *) inMsg, conn, NULL);
                        break;
                    }
                case ReqSetCliSvrInd:
                    {
                        CliConnectMST * msgSt = (CliConnectMST *) (inMsg->data);
                        printf("ReqSetCliSvrInd uid: %d logicSvrIndex: %d\n", msgSt->uId, msgSt->logicSvrIndex);
                        ExtraInterface * ein = (ExtraInterface *)(conn->serverCtx);
                        ClientCnCtx * clientCtx = ein->getClientCnCtx(msgSt->uId);
                        if( clientCtx != NULL )
                        {
                            clientCtx->logicServerMap[msgSt->logicBlockId] = msgSt->logicSvrIndex;
                        }
                        else
                        {
                            printf("not exit client\n");
                        }

                        break;
                    }
            }
        }

        virtual void handleError(int errCode, ConnCtx * conn)
        {
            ExtraInterface * ein = (ExtraInterface *)(conn->serverCtx);
            ein->serverConnError(errCode, conn);
        }
};


class CliH5ServerMsgDisposer : public MessageDisposer
{
    public:
        virtual int readDataInConnBuf(ConnCtx * conn)
        {
            //
            ExtraInterface * dCorrespond = (ExtraInterface *) (conn->serverCtx);
            int rLen = 0;
            char * data = dCorrespond->readData(conn, rLen);
            //
            if(data == NULL)
            {
                handleError(rLen, conn);
                dCorrespond->readDataEnd(conn, 0);
                return 0;
            }

            int llen = rLen;

            if(conn->state == ConnHandShakeState)
            {
                //
                char respond[1024];
                memset(respond, 0, 1024);
                int wlen = strlen(respond);
                conn->resolver->handShake( data, respond, wlen );

                ExtraInterface * dCorrespond = (ExtraInterface *) (conn->serverCtx);
                BufCtx * bufCtx = dCorrespond->writeData(conn, wlen);
                if(bufCtx == NULL)
                {
                    printf("can not write, conn is disable!!\n");
                    return 0;
                }
                MsgMgr::setH5MsgToBufCtx( respond, wlen, bufCtx);
                conn->state = ConnEnableState;
                dCorrespond->readDataEnd(conn, 0);
                return 0;
            }

            //if(rLen > MsgMgr::ClientToGateMsgLen)
            if(rLen > 0)
            {
                char * msgbuf = data;
                int pos = 0;

                while( 1 )
                {
                    int dLen = conn->resolver->decoder( msgbuf, pos );
                    char buf[1024];
                    memset(buf, 0, 1024);
                    memcpy( buf, msgbuf+pos, dLen );
                    pos += dLen;
                    //
                    printf("****dLen: %d, *****data: %s\n", dLen, buf);
                    //
                    char respond[1024];
                    memset(respond, 0, 1024);
                    char re[1024] = "helloWorld,Websocket!!";
                    int relen = strlen(re);

                    int tpos = conn->resolver->encoder( respond, relen );
                    memcpy(respond+tpos, re, relen);

                    int wlen = relen+tpos;
                    ExtraInterface * dCorrespond = (ExtraInterface *) (conn->serverCtx);
                    BufCtx * bufCtx = dCorrespond->writeData(conn, wlen);
                    if(bufCtx == NULL)
                    {
                        printf("can not write, conn is disable!!\n");
                        return 0;
                    }
                    MsgMgr::setH5MsgToBufCtx( respond, wlen, bufCtx);

                    dCorrespond->readDataEnd(conn, 0);
                    return 0;
                    //
                    ClientToGateMsg * cgMsg = (ClientToGateMsg *) msgbuf;
                    int tlen = llen - MsgMgr::ClientToGateMsgLen - cgMsg->msgLen;

                    if( tlen < 0 )
                    {
                        break;
                    }

                    llen = tlen;

                    handlerModule(cgMsg->msgType, msgbuf, conn);

                    if( llen <= MsgMgr::ClientToGateMsgLen )
                    {
                        break;
                    }

                    msgbuf = msgbuf + MsgMgr::ClientToGateMsgLen + cgMsg->msgLen;
                }
            }

            return llen;

        }

        virtual void handlerModule( int msgType, char * msgBuf, ConnCtx * conn )
        {
            switch( msgType )
            {
                case ClientLogicMsgType:
                    {
                        ClientToGateMsg * cgMsg = (ClientToGateMsg *) msgBuf;

                        int blockId = cgMsg->logicBlockId;
                        ClientCnCtx * clientCtx = (ClientCnCtx *) (conn->cnCtx);

                        int serverInd = 0;
                        std::map<BLOCKIDTYPE, int>::iterator it = clientCtx->logicServerMap.find(blockId);

                        if(it != clientCtx->logicServerMap.end())
                        {
                            serverInd =  it->second;
                        }

                        ExtraInterface * ei = (ExtraInterface *) (conn->serverCtx);
                        ConnCtx * svrConn = ei->getServerConnCtx(serverInd);

                        if(svrConn == NULL)
                        {
                           printf("connect master server error\n");
                        }
                        //
                        ExtraInterface * dCorrespond = (ExtraInterface *) (svrConn->serverCtx);
                        int wlen = cgMsg->msgLen + MsgMgr::ClientToGateMsgLen;
                        BufCtx * bufCtx = dCorrespond->writeData(svrConn, wlen);
                        if(bufCtx == NULL)
                        {
                            printf("can not write, conn is disable!!\n");
                            return;
                        }

                        InterComMsg head;

                        head.connIndex = conn->index;
                        head.uId = clientCtx->clientId;

                        MsgMgr::setCGToGLWithBufCtx(head, cgMsg, bufCtx);
                        //
                        TimerEventMoulde * tm = (TimerEventMoulde *) (conn->serverCtx);
                        tm->resetTimer(conn->timerHandler);

                        break;
                    }
                case ClientGateWayMsgType:
                    {
                        handleClientMsg(msgType, msgBuf, conn);
                        break;
                    }
            }
        }

        virtual void handleClientMsg( int msgType, char * msgBuf, ConnCtx * conn )
        {
            ClientToGateMsg * cgMsg = (ClientToGateMsg *) msgBuf;

            switch(cgMsg->handlerState)
            {
                case ReqCliConnectGw:
                    {
                        ConnectMsg::ClientConnect conMsg;
                        conMsg.ParseFromArray(cgMsg->data,cgMsg->msgLen);
                        CLIENTIDTYPE uId = conMsg.uid();
                        int fd = conn->fd;
                        printf("new client connect, uid = %u, fd = %d\n", uId,fd);
                        //
                        ExtraInterface * clientMoudle = (ExtraInterface *) (conn->serverCtx);
                        clientMoudle->addClient(conn, uId);

                        //
                        ConnectMsg::ClientConnect resM;
                        resM.set_uid(uId);

                        //
                        ExtraInterface * dCorrespond = (ExtraInterface *) (conn->serverCtx);
                        int wlen = resM.ByteSize() + MsgMgr::ClientToGateMsgLen;
                        BufCtx * bufCtx = dCorrespond->writeData(conn, wlen);
                        if(bufCtx == NULL)
                        {
                            printf("can not write, conn is disable!!\n");
                            return;
                        }

                        MsgMgr::addCGProtoMsgToBufCtx(ClientGateWayMsgType, 0, ResCliConnectGw, &resM, bufCtx);

                        //
                        ExtraInterface * tm = (ExtraInterface *) (conn->serverCtx);
                        tm->resetTimer(conn->timerHandler);

                        break;
                    }
            }
        }

        virtual void handleError(int errCode, ConnCtx * conn)
        {
            ExtraInterface * clientMoudle = (ExtraInterface *)(conn->serverCtx);
            clientMoudle->clientError(errCode, conn);
        }

};

class CliServerMsgDisposer : public MessageDisposer
{
    public:
        virtual int readDataInConnBuf(ConnCtx * conn)
        {
            //
            ExtraInterface * dCorrespond = (ExtraInterface *) (conn->serverCtx);
            int rLen = 0;
            char * data = dCorrespond->readData(conn, rLen);
            //
            //if(rLen > MsgMgr::ClientToGateMsgLen)
            if(data == NULL)
            {
                handleError(rLen, conn);
                dCorrespond->readDataEnd(conn, 0);
                return 0;
            }

            int llen = rLen;
            if(rLen > 0)
            {
                char * msgbuf = data;

                while( 1 )
                {
                    ClientToGateMsg * cgMsg = (ClientToGateMsg *) msgbuf;
                    int tlen = llen - MsgMgr::ClientToGateMsgLen - cgMsg->msgLen;

                    if( tlen < 0 )
                    {
                        break;
                    }

                    llen = tlen;

                    handlerModule(cgMsg->msgType, msgbuf, conn);

                    if( llen <= MsgMgr::ClientToGateMsgLen )
                    {
                        break;
                    }

                    msgbuf = msgbuf + MsgMgr::ClientToGateMsgLen + cgMsg->msgLen;
                }
            }

            dCorrespond->readDataEnd(conn, llen);
            return llen;

        }

        virtual void handlerModule( int msgType, char * msgBuf, ConnCtx * conn )
        {
            switch( msgType )
            {
                case ClientLogicMsgType:
                    {
                        ClientToGateMsg * cgMsg = (ClientToGateMsg *) msgBuf;

                        int blockId = cgMsg->logicBlockId;
                        ClientCnCtx * clientCtx = (ClientCnCtx *) (conn->cnCtx);

                        int serverInd = 0;
                        std::map<BLOCKIDTYPE, int>::iterator it = clientCtx->logicServerMap.find(blockId);

                        if(it != clientCtx->logicServerMap.end())
                        {
                            serverInd =  it->second;
                        }

                        ExtraInterface * ei = (ExtraInterface *) (conn->serverCtx);
                        ConnCtx *  svrConn = ei->getServerConnCtx(serverInd);

                        if(svrConn == NULL)
                        {
                           printf("connect master server error\n");
                        }

                        ExtraInterface * dCorrespond = (ExtraInterface *) (svrConn->serverCtx);
                        int wlen = cgMsg->msgLen + MsgMgr::InterComMsgLen;
                        BufCtx * bufCtx = dCorrespond->writeData(svrConn, wlen);
                        if(bufCtx == NULL)
                        {
                            printf("can not write, conn is disable!!\n");
                            return;
                        }

                        MsgMgr::buildInterComMsg(bufCtx, LogicMouldeType,
                                                cgMsg->handlerState, conn->index, cgMsg->logicBlockId,
                                                clientCtx->clientId, wlen, cgMsg->data, cgMsg->msgLen);

                        printf("ClientLogicMsgType send num %d!!\n", bufCtx->bufLen);
                        //
                        ExtraInterface * tm = (ExtraInterface *) ((conn->serverCtx));
                        tm->resetTimer(conn->timerHandler);

                        break;
                    }
                case ClientGateWayMsgType:
                    {
                        handleClientMsg(msgType, msgBuf, conn);
                        break;
                    }
            }
        }

        virtual void handleClientMsg( int msgType, char * msgBuf, ConnCtx * conn )
        {
            ClientToGateMsg * cgMsg = (ClientToGateMsg *) msgBuf;

            switch(cgMsg->handlerState)
            {
                case ReqCliConnectGw:
                    {
                        cliConnectGateWay(cgMsg,conn);
                        //
                        break;
                    }
            }
        }

        virtual void handleError(int errCode, ConnCtx * conn)
        {
            ExtraInterface * clientMoudle = (ExtraInterface *)(conn->serverCtx);
            clientMoudle->clientError(errCode, conn);
        }

        void cliConnectGateWay(ClientToGateMsg * cgMsg, ConnCtx * conn)
        {
            ConnectMsg::ClientConnect conMsg;
            conMsg.ParseFromArray(cgMsg->data,cgMsg->msgLen);
            CLIENTIDTYPE uId = conMsg.uid();
            int fd = conn->fd;
            printf("new client connect, uid = %u, fd = %d\n", uId,fd);
            //
            ExtraInterface * ein = (ExtraInterface *)((conn->serverCtx));
            ein->addClient(conn, uId);

            //
            ConnectMsg::ClientConnect resM;
            resM.set_uid(uId);

            //
            int wlen = resM.ByteSize() + MsgMgr::ClientToGateMsgLen;
            BufCtx * bufCtx = ein->writeData(conn, wlen);
            if(bufCtx == NULL)
            {
                printf("can not write, conn is disable!!\n");
                return;
            }

            MsgMgr::addCGProtoMsgToBufCtx(ClientGateWayMsgType, 0, ResCliConnectGw, &resM, bufCtx);

            //
            ein->resetTimer(conn->timerHandler);

            cliConnectLogicServer(uId,conn);
        }

        void cliConnectLogicServer(int uId, ConnCtx * conn)
        {
            //
            ExtraInterface * ein = (ExtraInterface *)((conn->serverCtx));
            ConnCtx * svrConn = ein->getServerConnCtx(0);

            if(svrConn == NULL)
            {
               printf("connect master server error\n");
               return;
            }
            //
            int wlen = MsgMgr::InterComMsgLen + MsgMgr::CliConnectMSTLen;
            ExtraInterface * svrein = (ExtraInterface *) (svrConn->serverCtx);
            BufCtx * bufCtx = svrein->writeData(svrConn, wlen);
            if(bufCtx == NULL)
            {
                printf("can not write, conn is disable!!\n");
                return;
            }

            InterComMsg * reM = MsgMgr::buildInterComMsg(bufCtx, MainInitMouldeType,
                                                         ReqCliConnectLS, conn->index,
                                                         0, uId, wlen, NULL, 0);
            MsgMgr::buildCliConnectMST(reM, 0, 0, uId, 0);
        }

};




#endif // MESSAGEDISPOSER_HPP_INCLUDED

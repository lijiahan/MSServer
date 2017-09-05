#include "client.h"

client::client()
{
    //ctor
}

client::~client()
{
    //dtor

    freeHandleCtx();

    for( int i=0; i<bufFreeVec.size(); i++)
    {
         BufCtx * buf = bufFreeVec[i];
         delete buf;
    }

    bufFreeVec.clear();

    event_del(&(clientCtx->readEevnt));
    event_del(&(clientCtx->writeEevnt));

    BufCtx * buf = clientCtx->readBuf;
    while(buf!= NULL)
    {
        BufCtx * bufTemp = buf->next;
        delete buf;
        buf = bufTemp;
    }

    buf = clientCtx->writeBuf;
    while(buf!= NULL)
    {
        BufCtx * bufTemp = buf->next;
        delete buf;
        buf = bufTemp;
    }

    delete clientCtx;
    event_base_free(eventBase);
}

void client::initialClient( int ind,std::string gateWayIp, int gateWayPort )
{
    clientIndex = ind;
    //
    eventBase = event_base_new();
    //
    clientCtx = new connCtx();
    clientCtx->index = 0;
    clientCtx->connType = 0;
    clientCtx->state = 0;
    clientCtx->ctx = (void *) this;
    clientCtx->readBuf = new BufCtx();
    clientCtx->writeBuf = new BufCtx();
    //
    if( ( clientCtx->fd = socket(AF_INET, SOCK_STREAM, 0))  < 0)
    {
        perror(" can not create socket \n");
        exit(0);
    }

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(gateWayPort);

    if( inet_pton(AF_INET, gateWayIp.c_str(), &server_addr.sin_addr ) < 1)
    {
        perror(" inet_pton error \n");
        exit(0);
    }

    if( connect(clientCtx->fd ,  (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
    {
        perror(" connect logic master  server error \n");
        exit(0);
    }

    evutil_make_socket_nonblocking( clientCtx->fd );

    //
    setReadEvent();

    reqMsgCtx = new reqHandlerCtx();

    BufCtx * wbufCtx = clientCtx->writeBuf;
    ClientToGateMsg * msg = (ClientToGateMsg *) (wbufCtx->buf);
    msg->msgType = ClientGateWayMsgType;
    msg->handlerState = ReqCliConnectGw;

    ConnectMsg::ClientConnect cl;
    uint32_t id = 1000 + clientIndex;
    cl.set_uid(id);
    cl.set_type(1);

    int len = cl.ByteSize();
    cl.SerializeToArray(msg->data, len);
    msg->msgLen = len;
    wbufCtx->bufLen = len + ClientToGateMsgLen;
    setWriteEvent();

    event_base_dispatch(eventBase);
}

void client::setReadEvent()
{
    event_assign(&(clientCtx->readEevnt), eventBase, clientCtx->fd, EV_READ|EV_PERSIST, readEventCallBack,  (void *) clientCtx);

    if(event_add(&clientCtx->readEevnt, 0) == -1)
    {
        perror(" can not add notify event \n");
        exit(0);
    }
}

void client::setWriteEvent()
{
    event_assign(&(clientCtx->writeEevnt), eventBase, clientCtx->fd, EV_WRITE, writeEventCallBack, (void *) clientCtx);
    if( event_add(&(clientCtx->writeEevnt), 0) == -1 )
    {
        perror(" can not add Write event  \n");
        exit(0);
    }
}

void client::setTimerEvent( struct timeval & t )
{
    event_assign(&(clientCtx->timeEevnt), eventBase, clientCtx->fd, EV_TIMEOUT|EV_PERSIST, timerEventCallBack, (void *) clientCtx);
    if( event_add(&(clientCtx->timeEevnt), &t) == -1 )
    {
        perror(" can not add Timer event  \n");
        exit(0);
    }
}

void client::readEventCallBack( int fd, short eventType, void * ctx )
{
    connCtx * conn = (connCtx *) ctx;
    client * c = (client *) conn->ctx;
    c->onReadEvent(conn);
}

void client::writeEventCallBack( int fd, short eventType, void * ctx )
{
    connCtx * conn = (connCtx *) ctx;
    client * cl = (client *) conn->ctx;
    cl->onWriteEvent(conn);
}

void client::timerEventCallBack( int fd, short eventType, void * ctx )
{
    connCtx * conn = (connCtx *) ctx;
    client * cl = (client *) conn->ctx;
    cl->onTimerEvent(conn);
}

void client::onReadEvent(connCtx * ctx)
{
    BufCtx * bufCtx = ctx->readBuf;
    ctx->state = ConnReadState;

    while( ctx->state ==  ConnReadState )
    {
        char * rbuf = bufCtx->buf+bufCtx->bufLen;
        int maxSize = BufSize-bufCtx->bufLen;

        int nread = read(ctx->fd, rbuf, maxSize);

        if( nread == maxSize )
        {
            ctx->state = ConnReadState;
        }
        else
        {
            if( nread <= 0 )
            {
                readError(nread, ctx);
                return;
            }
            else
            {
                ctx->state = ConnWaitReadState;
            }
        }

        bufCtx->bufLen = bufCtx->bufLen + nread;

        if( nread > ClientToGateMsgLen )
        {
            int llen = bufCtx->bufLen;
            char * pbuf = bufCtx->buf;
            ClientToGateMsg * msg = (ClientToGateMsg *) pbuf;

            while( 1 )
            {
                int t = llen - ClientToGateMsgLen - msg->msgLen;

                if( t < 0 )
                {
                    break;
                }

                llen = t;

                reqHandlerCtx * handle = getHandleCtx(msg->handlerState, msg, ctx);

                handleMsg(handle);

                if( llen <= ClientToGateMsgLen )
                {
                    break;
                }

                pbuf = pbuf + ClientToGateMsgLen + msg->msgLen;
                msg = (ClientToGateMsg *) pbuf;
            }

            if( llen > 0 )
            {
                int rl = bufCtx->bufLen - llen;
                memmove(bufCtx->buf, bufCtx->buf+rl, llen);
                bufCtx->bufLen = llen;
            }
            else
            {
                bufCtx->bufLen = 0;
            }
        }
    }
 }

 void client::onWriteEvent(connCtx * ctx)
 {
    BufCtx * wbufCtx =  ctx->writeBuf;

    while(wbufCtx != NULL)
    {
        int nw = write(ctx->fd, wbufCtx->buf, wbufCtx->bufLen);

        printf(" write num %d, wbufCtx->bufLen = %d \n", nw, wbufCtx->bufLen );

        if( nw == wbufCtx->bufLen )
        {
            ctx->writeBuf = wbufCtx->next;
            freeBufCtx(wbufCtx);
            wbufCtx = ctx->writeBuf;
        }
        else
        {
            int llen = wbufCtx->bufLen - nw;

            if(wbufCtx->bufLen > llen)
            {
                memmove(wbufCtx->buf, (wbufCtx->buf+nw), llen);
                wbufCtx->bufLen = llen;
                event_assign(&(ctx->writeEevnt), eventBase, ctx->fd, EV_WRITE, writeEventCallBack, (void *) ctx );
                if( event_add(&(ctx->writeEevnt), 0) == -1 )
                {
                    perror(" can not add Write event  \n");
                    exit(0);
                }
                break;
            }
            else
            {
                perror(" close connect ! \n");
                close(ctx->fd);
                while(wbufCtx != NULL)
                {
                    BufCtx * bufCtx = wbufCtx;
                    wbufCtx = wbufCtx->next;
                    freeBufCtx(bufCtx);
                }
                ctx->writeBuf = NULL;
            }
        }
    }
 }

 void client::onTimerEvent(connCtx * ctx)
 {
    BufCtx * wbufCtx =  ctx->writeBuf;
    if(wbufCtx == NULL)
    {
        if(bufFreeVec.size() > 0)
        {
            wbufCtx = bufFreeVec.back();
            bufFreeVec.pop_back();
        }
        else
        {
            wbufCtx = new BufCtx();
        }
        ctx->writeBuf = wbufCtx;
    }

    ClientToGateMsg * msg = (ClientToGateMsg *) wbufCtx->buf;
    msg->msgType = ClientLogicMsgType;
    msg->logicBlockId = ShareBlockMoulde;
    msg->handlerState = 0;

    ConnectMsg::ClientConnect cl;
    uint32_t id = 1000 + clientIndex;
    cl.set_uid(id);
    cl.set_type(1);

    int len = cl.ByteSize();
    msg->msgLen = len;
    cl.SerializeToArray(msg->data, len);

    wbufCtx->bufLen = ClientToGateMsgLen + len;
    setWriteEvent();
 }

 void client::readError(int nr, connCtx * ctx)
 {
    std::cout<<"readError"<<nr<<std::endl;
    event_del(&(ctx->readEevnt));
    close(ctx->fd);
 }

 reqHandlerCtx * client::getHandleCtx(int state, ClientToGateMsg * msg, connCtx * ctx)
 {
    reqMsgCtx->handleState = state;
    ClientToGateMsg * pm = (ClientToGateMsg *) reqMsgCtx->dataValue;
    pm->msgType = msg->msgType;
    pm->msgLen = msg->msgLen;
    pm->handlerState = msg->handlerState;
    memcpy(pm->data, msg->data, msg->msgLen);
    reqMsgCtx->dataLen = msg->msgLen + ClientToGateMsgLen;
    reqMsgCtx->conn = ctx;

    return reqMsgCtx;
 }

 void client::freeHandleCtx()
 {
     delete reqMsgCtx;
 }

 BufCtx * client::getBufCtx()
 {
    BufCtx * buf = NULL;

    if(bufFreeVec.size() > 0)
    {
        buf = bufFreeVec.back();
        bufFreeVec.pop_back();
    }
    else
    {
        buf = new BufCtx();
    }

    return buf;
 }

 void client::freeBufCtx( BufCtx * ctx )
 {
    ctx->bufLen = 0;
    ctx->next = NULL;
    memset(ctx, 0, BufSize);
    bufFreeVec.push_back(ctx);
 }

 void client::handleMsg(reqHandlerCtx * handle)
 {
    ClientToGateMsg * pm = (ClientToGateMsg *) handle->dataValue;

    printf("client %d handleMsg %d \n", clientIndex, handle->handleState);

    switch(handle->handleState)
    {
        case ResCliConnectLS:
            {
                ConnectMsg::ClientConnect cl;
                cl.ParseFromArray( pm->data, pm->msgLen );

                printf("client %d ***  ResCliConnectLS uId = %d\n",  clientIndex, cl.uid());
                gettimeofday(&stop, NULL);
                long st = ((long)start.tv_sec)*1000+(long)start.tv_usec/1000;
                long et = ((long)stop.tv_sec)*1000+(long)stop.tv_usec/1000;
                printf("Start time: %ld ms end time: %ld ms cost time: %ld ms\n", st,et,et-st);
                break;
            }
        case ResCliConnectGw:
            {
                ConnectMsg::ClientConnect conMsg;
                conMsg.ParseFromArray(pm->data,pm->msgLen);
                printf("client %d *** ResCliConnectGw id=%d\n",clientIndex, conMsg.uid());
                //
                ConnectMsg::ClientConnect cl;
                uint32_t id = 1000 + clientIndex;
                cl.set_uid(id);
                cl.set_type(1);

                int mlen = cl.ByteSize();
                int wlen = mlen + ClientToGateMsgLen;
                BufCtx * bufCtx = NULL;

                if( clientCtx->writeBuf == NULL )
                {
                    bufCtx = getBufCtx();
                    clientCtx->writeBuf = bufCtx;

                }
                else
                {
                    bufCtx = clientCtx->writeBuf;

                    while(bufCtx->next != NULL)
                    {
                        bufCtx = bufCtx->next;
                    }

                    if(BufSize - bufCtx->bufLen < wlen)
                    {
                        bufCtx->next = getBufCtx();
                        bufCtx = bufCtx->next;
                    }
                }

                ClientToGateMsg * msg = (ClientToGateMsg *) (bufCtx->buf + bufCtx->bufLen);
                msg->msgType = ClientLogicMsgType;
                msg->handlerState = ReqCliConnectLS;
                msg->logicBlockId = ShareBlockMoulde;

                msg->msgLen = mlen;
                cl.SerializeToArray(msg->data, mlen);
                bufCtx->bufLen = wlen;
                setWriteEvent();
                //
                gettimeofday(&start, NULL);
            }
        default:
            break;
    }
 }

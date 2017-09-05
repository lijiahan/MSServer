#ifndef EVENTIMPLST_HPP_INCLUDED
#define EVENTIMPLST_HPP_INCLUDED

#include "MsgImplSt.hpp"

//
class EventExtraInterface : public TimerEventMoulde, public MsgMoudleMgr
{
    public:
        EventExtraInterface()
        {
            eventBase = event_base_new();
        }

        virtual ~EventExtraInterface()
        {
            event_base_free( eventBase );
        }

        //
        virtual void initMsgMoudle() = 0;
        virtual void initServerByMsg(InterComMsg *msg, ConnCtx * conn, void * resData)=0;

        //
        virtual void writeEventHandler(ConnCtx * conn)=0;
        virtual void connError(int errCode, ConnCtx * conn)=0;
         //
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

        static void readEventCallBack( int fd, short eventType, void * ctx )
        {
            ConnCtx * conn = (ConnCtx *) ctx;
            EventExtraInterface * ein = (EventExtraInterface *) conn->serverCtx;
            ein->onReadEvent(conn);
        }

        static void writeEventCallBack( int fd, short eventType, void * ctx )
        {
            ConnCtx * conn = (ConnCtx *) ctx;
            //
            if((eventType & EV_PERSIST) == 0)
            {
                conn->eventState &= (~WriteEventState);
            }

            EventExtraInterface * ein = (EventExtraInterface *) conn->serverCtx;
            ein->onWriteEvent( conn );
        }

        static void timerEventCallBack( int fd, short eventType, void * ctx )
        {

        }

        void startEventModule()
        {
            event_base_dispatch(eventBase);
        }

        virtual int addTimer(ConnCtx * conn, TimerEventHandler * handler)
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
        void addReadEvent( ConnCtx * conn )
        {
            int state = conn->eventState & ReadEventState;
            if( state )
            {
                return;
            }

            event_assign(&(conn->readEevnt), eventBase, conn->fd, EV_READ|EV_PERSIST, readEventCallBack,  (void *) conn);

            if(event_add(&conn->readEevnt, 0) == -1)
            {
                perror(" can not add notify event \n");
                exit(0);
            }

            conn->eventState |= ReadEventState;
        }

        void addWriteEvent( ConnCtx * conn )
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

        virtual int delTimer(TimerEventHandler * handler)
        {
            event_del(&handler->timeEevnt);
            delete handler;

            return 0;
        }

        virtual int resetTimer(TimerEventHandler * handler)
        {
            if(handler->rpNum > 0)
            {
                event_del(&handler->timeEevnt);
            }
            //
            addTimer(handler->conn, handler);
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
                rLen = 0;
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

        virtual void onReadEvent(ConnCtx * conn)
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
        ////
    public:
         struct event_base * eventBase;
};

//
#endif // EVENTIMPLST_HPP_INCLUDED

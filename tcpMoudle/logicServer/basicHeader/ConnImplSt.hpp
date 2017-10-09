#ifndef CONNIMPLST_HPP_INCLUDED
#define CONNIMPLST_HPP_INCLUDED

#include "BaseFileHeader.h"

enum ConnType
{
    ConnGateWay =1,
    ConnClient,
    ConnAsynThread,
    ConnMaster,
    ConnSlave,
};

enum EventState
{
    ReadEventState  = 0x001,
    WriteEventState = 0x010,
    TimerEventState = 0x100,
};

//
struct ConnCtx;
class ConnCtxMgr;

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
    pthread_mutex_t * mutex;
    //
    TimerEventHandler * timerHandler;
    //
    BufCtx * readBuf;
    BufCtx * writeBuf;
    //
    ConnCtxMgr * connCtxMgr;
    //
    void * serverCtx;
    //extra data
    void * eDataCtx;
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

                    close(conn->fd);

                    if(conn->mutex != NULL)
                    {
                        free(conn->mutex);
                    }

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
            conn->eDataCtx = NULL;

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

#endif // CONNIMPLST_HPP_INCLUDED

#ifndef CLIENT_H
#define CLIENT_H

#include "baseStructHeader.h"
#include <time.h>
#include <sys/time.h>

class client
{
    public:
        client();
        virtual ~client();
        void initialClient( int ind, std::string gateWayIp, int gateWayPort );
        void setReadEvent();
        void setWriteEvent();
        void setTimerEvent(struct timeval & t );
        void readError( int nr, connCtx * ctx );
        reqHandlerCtx * getHandleCtx(int state, ClientToGateMsg * msg, connCtx * ctx);
        void freeHandleCtx();
        BufCtx * getBufCtx();
        void freeBufCtx( BufCtx * ctx );
        static void readEventCallBack( int fd, short eventType, void * ctx );
        static void writeEventCallBack( int fd, short eventType, void * ctx );
        static void timerEventCallBack( int fd, short eventType, void * ctx );
        virtual void onReadEvent(connCtx * con);
        virtual void onWriteEvent(connCtx * con);
        virtual void onTimerEvent(connCtx * con);
        virtual void handleMsg( reqHandlerCtx * handle );
    private:
        int clientIndex;
        reqHandlerCtx * reqMsgCtx;
        struct event_base * eventBase;
        connCtx * clientCtx;
        std::vector<BufCtx *> bufFreeVec;
        struct timeval start;
        struct timeval stop;
};

#endif // CLIENT_H

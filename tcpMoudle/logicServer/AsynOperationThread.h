#ifndef ASYNOPERATIONTHREAD_H
#define ASYNOPERATIONTHREAD_H

#include "basicHeader/EventImplSt.hpp"
#include "proto/ProtoMsgHeader.h"

class AsynOperationThread : public EventExtraInterface
{
    public:
        AsynOperationThread();
        virtual ~AsynOperationThread();

        void initialThread();
        int getSocketPairFd();
        //
        void onThread();
        virtual void writeEventHandler(ConnCtx * conn);
        virtual void connError(int errCode, ConnCtx * conn);
        //
        virtual void onWriteEvent(ConnCtx * conn);

        //
        virtual void initServerByMsg(InterComMsg *msg, ConnCtx * conn, void * resData);
        virtual void initMsgMoudle();

        //
        int addWriteConn( ConnCtx * conn );
        void delWriteConn( int writeInd );
        //
        void asynWriteMsg(InterComMsg * msg, int writeId);

        //
        static void * threadProcess( void  * ctx );
    private:
        int socketPairFd[2];
        pthread_t threadId;
        //
        ConnCtxMgr * connCtxmgr;
};

#endif // ASYNOPERATIONTHREAD_H

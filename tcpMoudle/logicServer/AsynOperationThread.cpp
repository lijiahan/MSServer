#include "AsynOperationThread.h"

AsynOperationThread::AsynOperationThread()
{
    //ctor
}

AsynOperationThread::~AsynOperationThread()
{
    //dtor
    delete connCtxmgr;
    close(socketPairFd[0]);
    close(socketPairFd[1]);
}

void AsynOperationThread::initialThread()
{
    //
    connCtxmgr = new ConnCtxMgr();

    //
    int suc = socketpair( AF_UNIX, SOCK_STREAM, 0, socketPairFd );

    if(suc < 0)
    {
        perror(" create socketpair error \n");
        exit(0);
    }

    ConnCtx * asynConn = connCtxmgr->getCtx();
    asynConn->fd = socketPairFd[1];
    asynConn->connType = ConnAsynThread;
    asynConn->serverCtx = (void *) this;
    asynConn->eventBase = eventBase;
    prepareRead(asynConn);

    pthread_attr_t attr;
    int ret = 0;
    pthread_attr_init(&attr);
    ret = pthread_create(&threadId, &attr, threadProcess, (void *) this);

    if(  ret != 0  )
    {
        perror(" create asyn thread error \n");
        exit(0);
    }
}

int AsynOperationThread::getSocketPairFd()
{
    return socketPairFd[0];
}

void * AsynOperationThread::threadProcess( void  * ctx )
{
    AsynOperationThread * threadCtx = (AsynOperationThread *) ctx;
    threadCtx->onThread();
    return NULL;
}

void AsynOperationThread::onThread()
{
    startEventModule();
}

void AsynOperationThread::writeEventHandler(ConnCtx * conn)
{

}

void AsynOperationThread::connError(int errCode, ConnCtx * conn)
{

}

//
void AsynOperationThread::initServerByMsg(InterComMsg *msg, ConnCtx * conn, void * resData)
{

}

//
void AsynOperationThread::initMsgMoudle()
{

}

void AsynOperationThread::onWriteEvent( ConnCtx * conn )
{
    struct msghdr  msg;
    memset(&msg, 0, sizeof(struct msghdr));
    struct iovec io[10];

    int buf_num =0;

    pthread_mutex_t * mutex = conn->mutex;
    pthread_mutex_lock(mutex);

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

    if( sendmsg(conn->fd,  &msg, 0) == -1 )
    {
         perror(" sendmsg error \n");
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

    pthread_mutex_unlock(mutex);

    printf("succ asynWrite: fd %d, send buf_num: %d, send Len: %d remain len:  %d\n",conn->fd, buf_num, sendLen, reLen);
}

int AsynOperationThread::addWriteConn( ConnCtx * conn )
{
    printf("addWriteConn! fd = %d\n", conn->fd);

    ConnCtx * newConn = connCtxmgr->getCtx();

    //
    newConn->connType = conn->connType;
    newConn->fd = conn->fd;
    newConn->serverCtx = (void *) this;

    //
    if(newConn->mutex == NULL)
    {
        pthread_mutex_t * mutex = (pthread_mutex_t *) malloc( sizeof(pthread_mutex_t));
        pthread_mutex_init( mutex, NULL );
        newConn->mutex = mutex;
    }

    return newConn->index;
}

void AsynOperationThread::delWriteConn( int writeInd )
{
    printf("delWriteConn! writeInd = %d\n",writeInd);
    ConnCtx * asynWConn = connCtxmgr->getCtx(writeInd);
    connCtxmgr->connError(0, asynWConn);
}

void AsynOperationThread::asynWriteMsg(InterComMsg * msg, int writeId)
{
    ConnCtx * asynWConn = connCtxmgr->getCtx(writeId);

    pthread_mutex_t * mutex = asynWConn->mutex;

    if( mutex == NULL)
    {
        printf("mutex error!!");
        return;
    }

    pthread_mutex_lock(mutex);

    BufCtx * bufCtx = writeData(asynWConn, msg->msgLen);

    if( bufCtx != NULL )
    {
        printf("asynWriteMsg! handlerType: %d, len: %d bufCtx: %d\n",msg->handleType, msg->msgLen, bufCtx->bufLen);
        MsgMgr::addICMsgToBufCtx(msg, bufCtx);
    }

    pthread_mutex_unlock(mutex);
}

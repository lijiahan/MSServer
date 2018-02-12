#ifndef HANDLERIMP_HPP_INCLUDED
#define HANDLERIMP_HPP_INCLUDED
#include "CommMsgMoudle.hpp"

//EntryDTModule
class NewEntryHandler : public Handler
{
public:
    virtual char * handle( void * entry, char * msg, int len )
    {
        EntryDTModule * emdl = EntryDTModule::getMoudle();
        return (char *) emdl->newEntry();
    }
};

class GetEntryHandler : public Handler
{
public:
    virtual char * handle( void * entry, char * msg, int len )
    {
        int mind = * ((int *) msg);
        EntryDTModule * emdl = EntryDTModule::getMoudle();
        return (char *) emdl->getEntry(mind);
    }
};

//ServerModule



//RedisModule
struct AsyRedisConnCmd
{
    int cmd;
    int fd;
};

struct AsyRedisConnRes
{

};

//
struct AsyWriteConnCmd
{
    int cmd;
    int fd;
};

struct AsyWriteConnRes
{

};

class WriteThreadConn : public BaseObject
{
public:
    ConnectCtx * conn;
};

class AsyWriteThreadHandler : public Handler
{
public:
    virtual char * handle( void * entry, char * msg, int len )
    {
        AsyWriteConnCmd * cmdMsg = (AsyWriteConnCmd *) msg;

        switch( cmdMsg->cmd )
        {
        case 1: // write thread
            {

             break;
            }

        case 2:  // redis thread
            {

            break;
            }
        }
    }
private:
    ObjectVectorSlab<WriteThreadConn> wthreadConn;
};


#endif // HANDLERIMP_HPP_INCLUDED

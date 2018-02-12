#ifndef COMMMSGMOUDLE_HPP_INCLUDED
#define COMMMSGMOUDLE_HPP_INCLUDED

#include "MoudleImp.hpp"
#include "EventImp.hpp"

class BaseObject
{
public:
    void setUid(CLIENTIDTYPE id)
    {
        uid = id;
    }

    CLIENTIDTYPE getUid()
    {
        return uid;
    }

    void setIndex(int ind)
    {
        index = ind;
    }

    int getIndex()
    {
        return index;
    }

    void storeData(char * key, char * data)
    {

    }

    char * loadData(char * key)
    {

    }

protected:
    CLIENTIDTYPE uid;
    int index;
};

class MdlEntry : public BaseObject
{
public:
    MdlEntry()
    {
        memset(moudleIndex, 0, sizeof(int) * 10);
    }

    void setMdlInd(int mdl, int ind)
    {
        moudleIndex[mdl] = ind;
    }

    int getMdlInd(int mdl)
    {
        return moudleIndex[mdl];
    }

private:
   int moudleIndex[10];
};

class EntryDTModule : public MoudleFactory<EntryDTModule>
{
public:
    EntryDTModule()
    {
        rduid = 100;
    }

    virtual ~EntryDTModule()
    {
    }

    MdlEntry * newEntry()
    {
        int uid = getUid();
        MdlEntry * m = slab.alloc(uid);
        return m;
    }

    MdlEntry * getEntry(int ind)
    {
        MdlEntry * m = slab.getObjectByInd(ind);
        return m;
    }

    CLIENTIDTYPE getUid()
    {
        return rduid++;
    }

private:
    int rduid;

private:
    ObjectVectorSlab<MdlEntry> slab;
};

class ServerModule : public MoudleFactory<ServerModule>, public ServerEventImp
{
public:
    ServerModule()
    {
    }

    virtual ~ServerModule()
    {

    }

    virtual void enterModule( ConnectCtx * conn, CommMsg * cmsg )
    {
        //
        char * entry = NULL;
        DTMoudleManager * dmgr = DTMoudleManager::getMoudle();
        if(conn->uid == 0)
        {
            entry = dmgr->entryMoudle(DTModuleLS, 1, NULL, NULL, 0);
        }
        else
        {
            int ind = conn->entryInd;
            entry = dmgr->entryMoudle(DTModuleLS, 2, NULL, (char *)(&ind), 0);
        }

    }

    virtual void newEvent( ConnectCtx * conn )
    {

    }

    virtual void delEvent( ConnectCtx * conn )
    {

    }
};

class GateWayData : public BaseObject
{
public:
    ConnectCtx * conn;
    int gatewayInd;
};

class GateWayModule : public MoudleFactory<GateWayModule>
{
public:
    GateWayModule()
    {

    }

    virtual ~GateWayModule()
    {

    }

private:
    ObjectVectorSlab<GateWayData> gateWayObj;
};

class RedisModule : public MoudleFactory<RedisModule>, public ServerEventImp
{
public:
    RedisModule()
    {
    }

    virtual ~RedisModule()
    {

    }
    ConnectCtx * setUpAsyConnect( int fd )
    {
        if(fd != -1)
        {
            evutil_make_socket_nonblocking(fd);
        }

        ConnectCtx * nconn = connMgr->getCtx();
        nconn->fd = fd;
        nconn->eventImp = (ServerEventImp *) (this);
        nconn->connState = ConnUseState;

        addReadEvent(nconn);
        return nconn;
    }
};


#endif // COMMMSGMOUDLE_HPP_INCLUDED

#ifndef MOUDLEIMP_HPP_INCLUDED
#define MOUDLEIMP_HPP_INCLUDED

#include"ImpHeader.h"

class Handler
{
public:
   virtual char * handle( void * entry, char * msg, int len ) = 0;
};

class MoudleInterface
{
public:
    virtual void newMoudle() = 0;
    virtual void delMoudle() = 0;
    virtual char * entryMoudle( int mdl, int cmd, void * entry, char * msg, int len ) = 0;
};

//
template <typename T>
class MemArray
{
public:
    MemArray(int num)
    {
        marray = (T **) malloc(sizeof(T *) * num);
        for(int i=0; i<num; i++)
        {
            marray[i] = NULL;
        }

        mnum = num;
    }

    ~MemArray()
    {
        free(marray);
    }

    void setM(int mind, T * m)
    {
        int tn = mnum;

        while(mind >= tn)
        {
            tn = tn * 2;
            T ** tmarry = (T **) malloc(sizeof(T *) * tn);
            int i = 0;
            for(; i<mnum; i++)
            {
                tmarry[i] = marray[i];
            }

            for(; i<tn; i++)
            {
                tmarry[i] = NULL;
            }

            free(marray);
            marray = tmarry;
            mnum = tn;
        }
        marray[mind] = m;
    }

    T * getM(int mind)
    {
        if(mnum > mind)
        {
            return marray[mind];
        }

        return NULL;
    }

    void freeM(int mind)
    {
        if(mnum > mind)
        {
            marray[mind] = NULL;
        }
    }

private:
    T ** marray;
    int mnum;
};

template <typename M>
class MoudleFactory : public MoudleInterface
{
public:
    MoudleFactory()
    {
        harray = new MemArray<Handler>(20);
    }

    virtual ~MoudleFactory()
    {
        delete harray;
    }

    static M * getMoudle()
    {
        if(m_moudle == NULL)
        {
            m_moudle = new M();
        }

        return m_moudle;
    }

    static void freeMoudle()
    {
        delete m_moudle;
        m_moudle = NULL;
    }

    virtual void registerHandler(int hind, Handler * hdl)
    {
        harray->setM(hind, hdl);
    }

    virtual char * entryMoudle( int mind, int cmd, void * entry, char * msg, int len )
    {
        printf("Handler\n");
        Handler * hdl = harray->getM(cmd);
        hdl->handle(entry, msg, len);
    }

private:
    MoudleFactory(const MoudleFactory&){}

    MoudleFactory & operator = (const MoudleFactory&){}

private:
    static M * m_moudle;

private:
    MemArray<Handler> * harray;
};

template <typename M> M * MoudleFactory<M>::m_moudle = NULL;

class LGMoudleManager: public MoudleFactory<LGMoudleManager>
{
public:
    LGMoudleManager()
    {
        marray = new MemArray<MoudleInterface>(20);
    }

    virtual ~LGMoudleManager()
    {
        delete  marray;
    }

    virtual char * entryMoudle(int mind, int cmd, void * entry, char * msg, int len)
    {
        MoudleInterface * mdl = marray->getM(mind);
        mdl->entryMoudle(mind, cmd, entry, msg, len);
    }
public:
    void registerMoudle(int mind, MoudleInterface * mdl)
    {
        marray->setM(mind, mdl);
    }

private:
    MemArray<MoudleInterface> * marray;
};

class DTMoudleManager: public MoudleFactory<DTMoudleManager>
{
public:
    DTMoudleManager()
    {
        marray = new MemArray<MoudleInterface>(20);
    }
    virtual ~DTMoudleManager()
    {
        delete  marray;
    }
    virtual char * entryMoudle(int mind, int cmd, void * entry, char * msg, int len)
    {
        MoudleInterface * mdl = marray->getM(mind);
        if(mdl != NULL)
        {
            printf("DTMoudleManager\n");
            mdl->entryMoudle(mind, cmd, entry, msg, len);
        }
    }
public:
    void registerMoudle(int mind, MoudleInterface * mdl)
    {
        marray->setM(mind, mdl);
    }

private:
    MemArray<MoudleInterface> * marray;
};

template <typename Ob>
class ObjectVectorSlab
{
public:
    ~ObjectVectorSlab()
    {
        for( uint32_t i=0; i<ObjectVec.size(); i++)
        {
             Ob * obj = ObjectVec[i];
             delete obj;
        }
        ObjectVec.clear();

        for( uint32_t i=0; i<ObjectFreeVec.size(); i++)
        {
             Ob * obj = ObjectFreeVec[i];
             delete obj;
        }
        ObjectFreeVec.clear();
    }

    Ob * alloc(CLIENTIDTYPE uid)
    {
        Ob * obj = NULL;
        if(ObjectFreeVec.size() > 0)
        {
            obj = ObjectFreeVec.back();
            ObjectFreeVec.pop_back();
            int ind = obj->getIndex();
            ObjectVec[ind] = obj;
        }
        else
        {
            obj = new Ob();
            obj->setIndex(ObjectVec.size());
            ObjectVec.push_back(obj);
        }

        obj->setUid(uid);
        return obj;
    }

    Ob * getObjectByUid(CLIENTIDTYPE uid)
    {
        for( uint32_t i=0; i<ObjectFreeVec.size(); i++)
        {
            Ob * obj = ObjectFreeVec[i];
            if(obj->getUid() == uid)
            {
                return obj;
            }
        }

        return NULL;
    }

    Ob * getObjectByInd(int index)
    {
        if( ObjectVec.size() > index )
        {
            return ObjectVec[index];
        }
        return NULL;
    }

private:
    std::vector<Ob *> ObjectVec;
    std::vector<Ob *> ObjectFreeVec;
};

#endif // MOUDLEIMP_HPP_INCLUDED

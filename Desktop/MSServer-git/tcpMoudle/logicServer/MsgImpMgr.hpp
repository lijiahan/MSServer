
enum CommMsgType
{
    GateWayMsgType = 1,

    MainServerMsgType = 2,

    SlaveServerMsgType = 3,

    ClientMsgType = 4,
};

struct CommMsg
{
    int msgType;
    int msgCmd;
    int checkCode;
    int msgLen;
    char data[];
};

class CommCtx
{
    public:
        virtual int sendMsg(int len, char * data)
        {

        }
};

class CommMsgResolver
{
    public:
        virtual void resolveMsg( CommMsg * cmsg ) = 0;
};

class CMsgResolverMgr
{
    public:
        CMsgResolverMgr()
        {

        }

        virtual ~CMsgResolverMgr()
        {

        }
    public:
        virtual void initResolverMgr() = 0;

    public:
        void RegisterResolver(CommMsgResolver * resolver)
        {

        }

    private:
        std::map<int, CommMsgResolver *> CMsgResolverMap;
};


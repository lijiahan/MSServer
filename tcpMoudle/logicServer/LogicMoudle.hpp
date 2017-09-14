#ifndef LOGICMOUDLE_HPP_INCLUDED
#define LOGICMOUDLE_HPP_INCLUDED

#include "basicHeader/ServerEventImplSt.hpp"

class MasterShareBlockLogic : public LogicMoudleInterface
{
    public:
        MasterShareBlockLogic()
        {

        }
        //
        ~MasterShareBlockLogic()
        {

        }

        //
        virtual int getDataCheck(ClientCtx * clientCtx, MoudleDataCheck * dataCheck)
        {
            int mid = clientCtx->getDataCtx(1)->dispIndex;
            return mid;
        }

        virtual void logicHander(HandlerCtx * handlerCtx)
        {

        }
};

class MasterIndenpentBlockLogic : public LogicMoudleInterface
{
    public:
        MasterIndenpentBlockLogic()
        {

        }

        ~MasterIndenpentBlockLogic()
        {

        }

        virtual int getDataCheck(ClientCtx * clientCtx, MoudleDataCheck * dataCheck)
        {
            printf("MasterIndenpentBlockLogic\n");
            return 0;
        }

        virtual void logicHander(HandlerCtx * handlerCtx)
        {

        }
};

class SlaveShareBlockLogic : public LogicMoudleInterface
{
    public:
        SlaveShareBlockLogic()
        {

        }
        //
        ~SlaveShareBlockLogic()
        {

        }

        //
        virtual void logicHander(HandlerCtx * handlerCtx)
        {

        }
};

class SlaveIndenpentBlockLogic : public LogicMoudleInterface
{
    public:
        SlaveIndenpentBlockLogic()
        {

        }

        ~SlaveIndenpentBlockLogic()
        {

        }

        virtual void logicHander(HandlerCtx * handlerCtx)
        {

        }
};

#endif // LOGICMOUDLE_HPP_INCLUDED

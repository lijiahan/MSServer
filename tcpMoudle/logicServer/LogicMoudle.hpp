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
            if( mid == 0 )
            {
                return dataCheck->slaveCtx->slaveSvrIndex;
            }
            return mid;
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
            printf("SlaveShareBlockLogic\n");
            //
            int htype = handlerCtx->msg->handleType;
            switch(htype)
            {
                case ReqCliLogicMS:
                {
                    //

                    break;
                }
            }
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

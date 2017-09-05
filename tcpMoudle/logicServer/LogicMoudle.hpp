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
        virtual void logicMoudleHandler(HandlerCtx * handlerCtx)
        {
            printf("MasterShareBlockLogic\n");
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

        virtual void logicMoudleHandler(HandlerCtx * handlerCtx)
        {
            printf("MasterIndenpentBlockLogic\n");
        }
};


#endif // LOGICMOUDLE_HPP_INCLUDED

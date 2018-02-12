#ifndef INITMOUDLE_HPP_INCLUDED
#define INITMOUDLE_HPP_INCLUDED
#include "HandlerImp.hpp"

void regMoudle()
{
    //
    LGMoudleManager * lgmgr = LGMoudleManager::getMoudle();

    //
    ServerModule * slgmdl = ServerModule::getMoudle();
    lgmgr->registerMoudle(LGModuleLS, static_cast<MoudleInterface *> (slgmdl));
    //
    DTMoudleManager * dtmgr = DTMoudleManager::getMoudle();
    //
    EntryDTModule * edtmdl =  EntryDTModule::getMoudle();
    dtmgr->registerMoudle(DTModuleLS, static_cast<MoudleInterface *> (edtmdl));
}

void regHandler()
{
    //
    EntryDTModule * edtmdl =  EntryDTModule::getMoudle();
    NewEntryHandler * nentryhdl = new NewEntryHandler();
    edtmdl->registerHandler(1, static_cast<Handler *> (nentryhdl));
    GetEntryHandler * gentryhdl = new GetEntryHandler();
    edtmdl->registerHandler(2, static_cast<Handler *> (gentryhdl));
}

void initServer(std::string ip, int port)
{
    regMoudle();
    regHandler();
    ServerModule * slgmdl = ServerModule::getMoudle();
    slgmdl->initListen(ip, port);
    slgmdl->startServer();
}

#endif // INITMOUDLE_HPP_INCLUDED

#include <iostream>
#include "MasterServer.h"

using namespace std;

int main()
{
    cout << "start gateServer!" << endl;
    MasterServer * server = new MasterServer();
    std::string ip = "192.168.75.128";
    int port = 8889;
    server->initServer(ip, port);
    delete server;
    return 0;
}

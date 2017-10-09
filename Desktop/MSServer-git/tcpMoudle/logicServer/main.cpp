#include <iostream>

#include "MasterServer.h"
#include "SlaveServer.h"

using namespace std;

int main()
{
    cout << "Hello Server!" << endl;

    std::string ip = "192.168.75.128";
    int port = 8888;
    int typeId = 1;

    cout << "input Server typeId!" << endl;
    cin >> typeId;

    if( typeId == 1 )
    {
         MasterServer * master = new MasterServer();
         master->initialServer(ip, port);
    }
    else
    {
        int lport = 0;
        cout << "input Server lport!" << endl;
        cin >> lport;
        SlaveServer * slave = new SlaveServer();
        slave->initialServer(ip, lport, ip, port);
    }


    return 0;
}

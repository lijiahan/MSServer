#include <iostream>
#include "client.h"

using namespace std;

int main()
{
    cout << "connect!" << endl;
    std::string ip = "192.168.75.128";
    int port = 8889;
    int index = 1;

    for( ;index < 10; index++ ){
        int id = fork();
        if( id == 0 ){
            break;
        }
    }

    client * cl = new client();
    cl->initialClient(index, ip, port);
    delete cl;
    return 0;
}

#include <iostream>
#include "InitMoudle.hpp"

using namespace std;

int main()
{
    cout << "Hello Server!" << endl;
    std::string ip = "10.0.6.217";
    int port = 8888;
    initServer(ip, port);

    return 0;
}

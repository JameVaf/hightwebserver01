#include<iostream>
#include "../include/EasyTcpServer.hpp"



int main(){
    EasyTcpServer server;
    server.initSocket();
    server.Bind(1234);
    server.Listen();
    
    while(server.isRun()){
        server.OnRun();
    }





    return 0;
}
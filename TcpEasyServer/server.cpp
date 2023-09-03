#define WIN32_LEAN_AND_MEAN
#include<iostream>
#include<windows.h>
#include<winsock2.h>
#pragma comment(lib,"ws2_32.lib")
int ret = -1;

#define READBUFF 1024
char readBuff [READBUFF] = {0};

int main(void){

    WORD ver = MAKEWORD(2,2);
    WSADATA dat;
    WSAStartup(ver,&dat);
    //1.创建socket套接字
    SOCKET serverfd = socket(AF_INET,SOCK_STREAM,0);
    if(SOCKET_ERROR == serverfd){
        perror("socket 失败!");
    }

    //2.bind 绑定ip地址
    struct sockaddr_in server_addr; 
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.S_un.S_addr = ADDR_ANY;
    server_addr.sin_port = htons(1234);
    ret = bind(serverfd,(sockaddr*)&server_addr,sizeof(server_addr));
    if(-1 == ret){
        perror("bind 失败");
    }

    //3.进行listen 监听
    ret = listen(serverfd,1000);
    if(-1 == ret){
        perror("listen 失败");
    }

    printf("服务器开始工作....\n");

    //4.accept接受客户端的数据
    while(1){
        struct sockaddr_in client_addr = {};
        int client_addr_len = sizeof(client_addr);
        SOCKET clientfd = accept(serverfd,(sockaddr*)&client_addr,&client_addr_len);
        if(SOCKET_ERROR == clientfd){
            perror("accept 失败");
        }
        
        printf("接收到新的连接,ip:%s,port:%u...\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
        //5.读取客户端的命令
        int readLen = recv(clientfd,readBuff,READBUFF,0);
        if(readLen <= 0){
            perror("read 失败...");
        }
        if(0 == strcmp("quit",readBuff)){
            memset(readBuff,0,READBUFF);
            closesocket(clientfd);
            break;
        }else if(0 == strcmp("whoami",readBuff)){
            int sendLen = send(clientfd,"jame",5,0);
            if(sendLen <= 0){
                 perror("send 失败...");
            }
        }else{
            int sendLen = send(clientfd,"未知的命令",16,0);
            if(sendLen <= 0){
                 perror("send 失败...");
            }
        }
        memset(readBuff,0,READBUFF);
    }




    printf("服务器关闭...\n");
    closesocket(serverfd);



    WSACleanup();
    return 0;
}


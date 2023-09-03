#define WIN32_LEAN_AND_MEAN
#include<iostream>
#include<windows.h>
#include<winsock2.h>
#pragma comment(lib,"ws2_32.lib")
int ret = -1;
#define CMDBUFF 128
#define READBUFF 128
char cmdBuff [CMDBUFF] = {0};
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

  

    //2.进程connect连接
    struct sockaddr_in serveraddr;
    serveraddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    serveraddr.sin_port = htons(1234);
    serveraddr.sin_family = AF_INET;

    ret = connect(serverfd,(sockaddr*)&serveraddr,sizeof(serveraddr));
    int error =  WSAGetLastError();
    printf("error:%d\n",error);

    if(SOCKET_ERROR == ret){
        printf("ret = %d\n",ret);
        perror("connect 失败...");
       
    }

    printf("客户端开始连接...\n");
    
    while(1){
        printf("请输入命令控制服务器:");
        scanf("%s\n",cmdBuff);
        //3.将命令发送给服务器
        int sendLen = send(serverfd,cmdBuff,CMDBUFF,0);
        if(sendLen <= 0){
            perror("send 失败");
        }
        if(0 == strcmp("quit",cmdBuff)){
            memset(cmdBuff,0,CMDBUFF);
            break;
        }
        //4.从服务器读取结果
        int readLen = recv(serverfd,readBuff,READBUFF,0);
        if(readLen <= 0){
            perror("recv 读取服务器失败...");
        }

        printf("server send:%s\n",readBuff);
        memset(readBuff,0,READBUFF);


    }


    printf("客户端进行关闭...\n");
    closesocket(serverfd);
    WSACleanup();

    return 0;
}
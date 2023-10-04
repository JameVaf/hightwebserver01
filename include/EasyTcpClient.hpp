#ifndef _EASYTCPCLIENT_H
#define _EASYTCPCLIENT_H

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include<windows.h>
    #include<winsock2.h>
#else
    #include<unistd.h> 
    #include<arpa/inet.h>
    #include<sys/socket.h>
    #include<sys/types.h>
    #include<string.h>

    #define SOCKET int
    #define INVALID_SOCKET	(SOCKET)(~0)
    #define SOCKET_ERROR	(-1)
#endif

#include<iostream>
#include<thread>
#include<string.h>
#include "MessageHeader.hpp"

//接受缓冲区的大小
#ifndef BUFF_SIZE 
#define BUFF_SIZE   10240    //接受缓冲区的大小
#endif





class EasyTcpClient{
public:
    EasyTcpClient(){
        _sock = INVALID_SOCKET;
        memset(_recvBuff,0,sizeof(_recvBuff));
        memset(_msgBuff,0,10 * BUFF_SIZE);
        _lastPos = 0;
    };


    virtual ~EasyTcpClient(){
        Close();
    };
    //初始化socket套接字
    int initSocket(){
        if(_sock != INVALID_SOCKET){
            printf("<socket=%d >关闭旧的连接\n",_sock);
            Close();
        }

        //启动win sock 2.x环境
        #ifdef _WIN32

        WORD ver = MAKEWORD(2,2);
        WSADATA dat;
        WSAStartup(ver,&dat);
        #else

        #endif

        _sock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
        if(INVALID_SOCKET == _sock){
            printf("socket 创建失败\n");
            return -1;
        }
        else{
            printf("socket 套接字创建成功\n");
            return 0;
        }
    };
    //连接服务器
    int Connect(char* ip,unsigned short port){
        if(_sock == INVALID_SOCKET){
            printf("没有初始化socket,开始初始化socket\n");
            initSocket();
        }

        //2.进程connect连接
        struct sockaddr_in serveraddr;
        serveraddr.sin_addr.s_addr = inet_addr(ip);
        serveraddr.sin_port = htons(port);
        serveraddr.sin_family = AF_INET;

       int  ret = connect(_sock,(sockaddr*)&serveraddr,sizeof(serveraddr));
       if(-1 == ret){
            printf("connect 连接失败\n");
       }else{
            printf("connect 连接成功\n");
       }

       return ret;

    };

    //关闭socket
    int Close(){
        if(_sock != INVALID_SOCKET){
        
        #ifdef _WIN32
        //关闭 win sock 2.x 环境
        closesocket(_sock);
        WSACleanup();
        #else
        close(_sock);
        #endif
        }
        _sock = INVALID_SOCKET;
        return 0;

    };
  
    
   
    //查询网络消息
    bool OnRun(){
        if(isRun()){
            fd_set fdRead;
            FD_ZERO(&fdRead);
            FD_SET(_sock,&fdRead);
            timeval t = {1,0};
            int ret = select(_sock+1,&fdRead,nullptr,nullptr,&t);
            if(-1 == ret){
                printf("<socket = %d>select任务结束\n",_sock);
                return false;
            }
            if(FD_ISSET(_sock,&fdRead)){
                FD_CLR(_sock,&fdRead);

                if(-1 == RecvData(_sock)){
                    printf("<socket = %d>select任务结束\n",_sock);
                    Close(); 
                    return false;
                }
            }
             
        return true;
        }
        return false;
    };

    //接受网络消息
     int RecvData(SOCKET cSock){
        
        //接受数据到数据缓冲区
        int nLen = (int)recv(cSock,this->_recvBuff,BUFF_SIZE,0);
        if(nLen < 0){
            printf("与服务器断开,任务结束\n");
            return -1;
        }
        //将已接受的数据存贮到消息缓冲区
        memcpy(this->_msgBuff + this->_lastPos ,this->_recvBuff ,nLen);
        //更新msgBuff 消息缓冲区的实际存贮位置
        this->_lastPos += nLen;
        
        //判断消息缓冲区存贮的数据是否达到一个消息头的大小
        while(_lastPos >= sizeof(DataHeader)){
            //读取头部的消息
            DataHeader* header = (DataHeader* )_msgBuff;
            //判断msgBuff 消息缓冲区的大小是否是一个完整的数据包
            if( _lastPos >= header->dataLength){
                //得到未处理缓冲区的大小
                size_t nLen = _lastPos - header->dataLength;
                
                //进行消息处理
                OnNetMsg(header);
                //将为处理的消息缓冲区进行前移
                memcpy(_msgBuff,_msgBuff + header->dataLength,nLen);
                //更新lasPos的位置
                _lastPos = nLen;
               
            }else{  //不能得到完整的消息体
                
                break;
            }
        }
        

        return 0;
    }

    //处理网络消息
    virtual void  OnNetMsg( DataHeader* header){
        switch (header->cmd)
        {
        case CMD_JOIN: {//server发送的是join信息
        //读取接受缓冲区里的数据
        printf("\n新加入一个连接,ip:%s\n",((Join*)header)->ipAddress);
        break;
        }
        case CMD_LOGIN_RESULT:{
           
           
            //server服务器返回的结果
            printf("\n登录server返回的结果为:%d\n",((LogInResult*)header)->result);
            break;
        }
        case CMD_LOGOUT_RESULT:{
           
            //server服务器返回的结果
            printf("登出server返回的结果为:%d\n",((LogOutResult*)header)->result);
            break;
        }
        default:{
            printf("未知的命令\n");
            break;
        }
        
        }
   

    };

    //发出数据
    int   Send(DataHeader* header){
        if( isRun() && header){
            return send(_sock,(char*)header,header->dataLength,0 );
        }
        return -1;
    }

    //判断套接字是否有效
    bool isRun(){
        return _sock != INVALID_SOCKET;
    };

private:
    SOCKET _sock;               //server服务的套接字
    char _recvBuff [BUFF_SIZE];      //recv 接受缓冲区
    char _msgBuff [BUFF_SIZE * 10];      //消息缓冲区,用于处理粘包的网络问题
    size_t _lastPos;                    //消息缓冲区的实际存贮位置 
};





#endif  //_EASYTCPCLIENT_H
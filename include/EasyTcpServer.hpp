#ifndef EASYTCPSERVER_H
#define EASYTCPSERVER_H

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define FD_SETSIZE 1024 //修改select 套接字集数组的大小,改变默认的64为1024 
    #include<windows.h>
    #include<winsock2.h>
    #include<ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include<unistd.h> 
    #include<arpa/inet.h>
    #include<sys/socket.h>
    #include<string.h>
    
    #define SOCKET int
    #define INVALID_SOCKET	(SOCKET)(~0)
    #define SOCKET_ERROR	(-1)
#endif

#include<iostream>
#include<thread>
#include<vector>
#include<algorithm>
#include<atomic>
#include "MessageHeader.hpp"

#ifndef BUFF_SIZE
#define BUFF_SIZE   1024    //读写缓冲区的大小
#endif


//客户连接端
class ClientSock{
public:
    ClientSock(SOCKET cSock = INVALID_SOCKET){
        _socket = cSock;
        memset(_msgBuff,0,BUFF_SIZE * 10);
        _lastPos = 0;
    }
    SOCKET getSock(){ return _socket;};
    char* getMsg(){ return _msgBuff;};
    size_t getPos(){ return _lastPos;};
    void setPos(size_t pos){ _lastPos = pos; };


private:

    char _msgBuff[BUFF_SIZE * 10];//消息缓冲区
    size_t _lastPos;            //消息缓冲区实际存贮到的位置
    SOCKET _socket;
};

class EasyTcpServer{
public:
EasyTcpServer(){
    _sock = INVALID_SOCKET;
    memset(_recvBuff,0,sizeof(_recvBuff));
   
}
virtual ~EasyTcpServer(){

    //主要将sockList中已连接的客户端堆内存进行释放
    for(auto iter: _sockList){
        delete iter;
        iter = nullptr;
    }
    //清空vector内的指针
    _sockList.clear();

}

//初始化socket 套接字
int initSocket(){
    if(_sock != INVALID_SOCKET){
        printf("关闭旧的套接字\n");
        Close();
    }
    #ifdef _WIN32
    WORD ver = MAKEWORD(2, 2);
    WSADATA dat;
    WSAStartup(ver, &dat);
    #else

    #endif 
    _sock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if( INVALID_SOCKET == _sock){
        printf("socket 创建失败\n");
        return -1;
    }else{
        printf("socket 套接字创建成功\n");
        return 0;
    }

}

//bind 绑定套接字
int Bind(unsigned short port){
    if(_sock == INVALID_SOCKET){
    printf("创建套接字\n");
        initSocket();
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    int ret = bind(_sock, (sockaddr *)&server_addr, sizeof(server_addr));
    if (-1 == ret)
    {
        printf("bind 失败\n");
        return -1;
    }else{
        printf("bind 成功\n");
        return 0;
    }
}

//listen监听套接字
int Listen(){
    if(isRun()){
        int  ret = listen(_sock, 1000);
        if (-1 == ret)
        {
            printf("listen 失败\n");
            return -1;
        }
        else{
            printf("listen 成功\n");
            return 0;
        }
    }else{
        printf("server套接字已关闭\n");
        return -1;
    }
    
}


//Accept 接收
SOCKET Accept(){
    struct sockaddr_in client_addr = {0};
    memset(&client_addr,0,sizeof(sockaddr_in));
    socklen_t len = sizeof(sockaddr_in);
    SOCKET newCFd = accept(_sock,(sockaddr*)&client_addr,&len);
    if(-1 == newCFd){
        printf("accept 失败\n");
        return -1;
    }else{
        //构造性的ClientSock
        ClientSock* newClient = new ClientSock(newCFd);
        // 将新加入的客户端信息发送给其他的客户端,实现知道对面上线的功能
        Join join;
        memset(join.ipAddress, 0, sizeof(join.ipAddress));
        memcpy(join.ipAddress, inet_ntoa(client_addr.sin_addr), strlen(inet_ntoa(client_addr.sin_addr)));
        for (auto iter : _sockList)
        {
            // 向已连接的每一个客户端发送新加入客户端的Join的具体信息
            int ret = (int)send(iter->getSock(), (char *)&join, sizeof(struct Join), 0);
            printf("发送一个join信息,addr : %s\n",join.ipAddress);
            if (-1 == ret)
            {
                printf("send error\n");
            }
        }
        _sockList.push_back(newClient);
    }
   
    printf("接收到新的客户端连接,ip:%s,port:%u...\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    return newCFd;
}

//close 套接字
int Close(){
    if(_sock != INVALID_SOCKET){
        #ifdef _WIN32
        //关闭 win sock 2.x 环境和服务器套接字
        closesocket(_sock);
        WSACleanup();
        #else
        close(_sock);
        #endif  

    
    //主要将sockList中已连接的客户端堆内存进行释放
    for(auto iter: _sockList){
        delete iter;
        iter = nullptr;
    }
    //清空vector内的指针
    _sockList.clear();
    
    }
    _sock = INVALID_SOCKET;
    return 0;
}

//查询网络的信息
bool OnRun(){
    if(_sock != INVALID_SOCKET){
        fd_set fdRead;
        FD_ZERO(&fdRead);
        FD_SET(_sock,&fdRead);

        SOCKET maxSock = _sock;
        //添加已连接套接字到select中,同时找到最大的sock
        for(auto iter: _sockList){
            FD_SET(iter->getSock(),&fdRead);
            if(maxSock < iter->getSock()){
                maxSock = iter->getSock();
            }
        }
        timeval t ={1,0};
        int ret = select(maxSock+1,&fdRead,nullptr,nullptr,&t);
        if(ret < 0){
            printf("select 任务结束\n");
            Close();
            return -1;
        }else{
            //判断server套接字是否被触发
            if(FD_ISSET(_sock,&fdRead)){
                FD_CLR(_sock,&fdRead);
                //调用accept 接受
                Accept();
            
            };
            //判断已连接套接字是否有数据
            for(size_t i = 0;i<_sockList.size();i++){
                if(FD_ISSET(_sockList[i]->getSock(),&fdRead)){
                    //接受套接字的数据
                    int ret = RecvData(_sockList[i]);
                    //如果断开连接
                    if(-1 == ret){
                        //在select中清除套接字,在以连接的数组中清除
                        FD_CLR(_sockList[i]->getSock(),&fdRead);
                        auto tempSock = _sockList.erase(_sockList.begin()+i);
                        //关闭无用的套接字
                        #ifdef _WIN32
                            closesocket((*tempSock)->getSock());
                        #else
                            close((*tempSock)->getSock());
                        #endif
                    }
                }
            }
            return true;

        }
  
    }
    return false;


}

//接收网络数据
int RecvData(ClientSock* cSock){
    if(_sock != INVALID_SOCKET){
       
        //接受缓冲区读取socket 套接字缓冲区的内容
        memset(this->_recvBuff,0,BUFF_SIZE);
        int nLen = (int)recv(cSock->getSock(),_recvBuff,BUFF_SIZE,0);//读取数据
        if(nLen < 0){
            printf("<socket = %d>客户端断开连接,任务结束\n",cSock->getSock());
            return -1;  //recv读取数据失败,客户端断开连接
        }

        //将接受缓冲区的数据同步至消息缓冲区
        memcpy(cSock->getMsg() + cSock->getPos(),_recvBuff,nLen);
        //更新客户端的Pos
        cSock->setPos(nLen + cSock->getPos());
        while(cSock->getPos() >= sizeof(DataHeader)){
            DataHeader* header = (DataHeader*)cSock->getMsg();
            if(cSock->getPos() >= header->dataLength){
                //得到未处理的消息缓冲区的长度
                size_t nLen = cSock->getPos() - header->dataLength;
                //得到一个完整的消息体,将消息体的内容进行处理
                OnNetMsg(cSock,header);
                //将消息缓冲区的Pos向前移已处理的长度
                memcpy(cSock->getMsg(),cSock->getMsg() + header->dataLength,header->dataLength);
                //更新Pos的位置
                cSock->setPos(nLen);


            }else{
                //只得到一个完整的头部消息,没有得到完整的消息体
                break;
            }

        }
        return 0;//没有读到完整的数据包
    }
    return -1;//服务端套接字关闭,不进行数据读取
}

//处理网络消息
virtual void OnNetMsg(ClientSock* cSock,DataHeader* header){
    char sendBuff [1024] = {0};
    switch (header->cmd)
    {
    
    case CMD_LOGIN:{
        //server服务器返回的结果
        //printf("登录server返回的结果为:%d\n",((LogInResult*)header)->result);
        printf("LOGIN 用户名:%s,用户密码:%s,数据长度%d\n", ((Login*)header)->userName, ((Login*)header)->passWord,header->dataLength);
        LogInResult ret;
        ret.result = 0;
        memcpy(sendBuff,&ret,sizeof(LogInResult));
        int sendNumber =  (int)send(cSock->getSock(), sendBuff, sizeof(LogInResult), 0);
        printf("Send client  :%d bytes\n",sendNumber);
        break;
    }
    case CMD_LOGOUT:{
        
        printf("客户端 %s退出成功...\n", ((Logout*)header)->userName);
        LogOutResult ret;
        ret.result = 0;
        send(cSock->getSock(), (const char *)&ret, sizeof(LogInResult), 0);
        break;
    }
    case CMD_QUIT:
    {
        printf("收到的cmd:CMD_QUIT\n");

        printf("服务器关闭...\n");
        Close();
        break;
        
    }
    // case 100:{
    //     printf("收到test 1000 byte数据\n");
    //     LogOutResult ret;
    //     ret.result = 0;
    //     send(cSock->getSock(), (const char *)&ret, sizeof(LogInResult), 0);
    //     break;
    // }
    default:
    {
        printf("???\n");
        break;

    }
    
    
    }


};

//判断是否server套接字存在
bool isRun(){    //
    return _sock != INVALID_SOCKET;
}

//发送数据
int Send(SOCKET _cSock,DataHeader* header){
    if( isRun() && header){
        return send(_cSock,(char*)header,header->dataLength,0 );
    }
    return -1;
}

//对所有已连接客户端发送数据
int SendToAll(DataHeader* header){
    if( isRun() && header){
        for (auto iter : _sockList)
        {
            // 向每一个客户端发送Join的具体信息
            int ret = (int)send(iter->getSock(), (char *)&header, header->dataLength, 0);
            if (-1 == ret)
            {
                printf("send error\n");
            }
            printf("发送一个信息\n");
        }
    }
    return -1;
}


private:
    SOCKET _sock;
    char _recvBuff [BUFF_SIZE];      //recv 接受缓冲区
    std::vector<ClientSock*> _sockList;
    static std::atomic<bool> s_is_run;  //线程间进行通信,线程判断程序是否继续运行
   
};




#endif //EASYTCPSERVER_H
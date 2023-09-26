#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <windows.h>
#include <winsock2.h>
#include <vector>
#include <algorithm>
#include <string.h>
#pragma comment(lib, "ws2_32.lib")
int ret = -1;

#define READBUFF 1024
char readBuff[READBUFF] = {0};
std::vector<SOCKET> sock_list; // 存放接受连接的socket套接字

struct fd_set fdRead;
struct fd_set fdWrite;
struct fd_set fdExcept;

// 命令宏
enum CMD
{
    CMD_LOGIN = 1,     // 登录命令
    CMD_LOGOUT,        // 登出命令
    CMD_ERROR,         // 错误的命令
    CMD_QUIT,          // 退出命令
    CMD_LOGIN_RESULT,  // 登录返回结果命令
    CMD_LOGOUT_RESULT, // 登出的返回结果
    CMD_JOIN           // 其他用户加入服务器的
};

// 消息头
struct DataHeader
{
    short dataLength; // 数据的长度
    short cmd;        // 命令
};

// 登录头
struct Login : public DataHeader
{

    Login() : userName({0}), passWord({0})
    {
        dataLength = sizeof(Login);
        cmd = CMD_LOGIN;
    }
    char userName[32];
    char passWord[32];
};

// 登录返回结果头
struct LogInResult : public DataHeader
{

    LogInResult() : result(0)
    {
        dataLength = sizeof(LogInResult);
        cmd = CMD_LOGIN_RESULT;
    }
    int result;
};

// 登出头
struct Logout : public DataHeader
{

    Logout() : userName({0})
    {
        dataLength = sizeof(Logout);
        cmd = CMD_LOGOUT;
    }
    char userName[32];
};

// 登出返回结果头
struct LogOutResult : public DataHeader
{

    LogOutResult() : result(0)
    {
        dataLength = sizeof(LogOutResult);
        cmd = CMD_LOGOUT_RESULT;
    }
    int result;
};

// JOIN结构体
struct Join : public DataHeader
{
    Join() : ipAddress({0})
    {
        dataLength = sizeof(Join);
        cmd = CMD_JOIN;
    }
    char ipAddress[32];
};

// 添加套接字到select
int addFdSelect(SOCKET fd);

// 删除套接字
int delFdSelect(SOCKET fd);

// 处理cmd的程序
int processCmd(SOCKET _sock);

int main(void)
{

    WORD ver = MAKEWORD(2, 2);
    WSADATA dat;
    WSAStartup(ver, &dat);
    // 1.创建socket套接字
    SOCKET serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (SOCKET_ERROR == serverfd)
    {
        perror("socket 失败!");
    }

    // 2.bind 绑定ip地址
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.S_un.S_addr = ADDR_ANY;
    server_addr.sin_port = htons(1234);
    ret = bind(serverfd, (sockaddr *)&server_addr, sizeof(server_addr));
    if (-1 == ret)
    {
        perror("bind 失败");
    }

    // 3.进行listen 监听
    ret = listen(serverfd, 1000);
    if (-1 == ret)
    {
        perror("listen 失败");
    }

    printf("服务器开始工作....\n");

    // 将server套接字加入select中

    while (1)
    {
        // 清空套接字集合
        FD_ZERO(&fdRead);
        FD_ZERO(&fdWrite);
        FD_ZERO(&fdExcept);

        addFdSelect(serverfd);
        // 将sock_list套接字加入其中
        for (auto iter : sock_list)
        {
            addFdSelect(iter);
        }

        // select阻塞的时间
        timeval t = {3, 0};
        // nfds 是一个整数值,是fd_set集合中所有描述符的范围,而不是数量
        int ret = select(serverfd + 1, &fdRead, &fdWrite, &fdExcept, &t);
        if (ret < 0)
        {
            printf("select 任务结束.\n");
            break;
        }
        if (FD_ISSET(serverfd, &fdRead))
        { // server有读的操作,即有链接的请求
            delFdSelect(serverfd);
            // 4.accept接受客户端的数据
            struct sockaddr_in client_addr = {};
            int client_addr_len = sizeof(client_addr);
            SOCKET clientfd = accept(serverfd, (sockaddr *)&client_addr, &client_addr_len);
            if (SOCKET_ERROR == clientfd)
            {
                perror("accept 失败");
            }

            // serverfd执行读取命令
            // processCmd(serverfd);

            // 将新加入的客户端信息发送给其他的客户端,实现知道对面上线的功能
            Join join;
            memset(join.ipAddress, 0, sizeof(join.ipAddress));
            memcpy(join.ipAddress, inet_ntoa(client_addr.sin_addr), sizeof(inet_ntoa(client_addr.sin_addr)));
            for (auto iter : sock_list)
            {
                // 向每一个客户端发送Join的具体信息
                int ret = send(iter, (char *)&join, sizeof(struct Join), 0);
                printf("发送一个join信息\n");
                if (-1 == ret)
                {
                    printf("send error\n");
                }
            }

            sock_list.push_back(clientfd);
            printf("接收到新的客户端连接,ip:%s,port:%u...\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        }

        // 每一个fdRead里的套接字进行processCmd
        for (size_t i = 0; i < fdRead.fd_count; i++)
        {
            if (-1 == processCmd(fdRead.fd_array[i]))
            {
                // 这个套接字断开连接
                auto iter = find(sock_list.begin(), sock_list.end(), fdRead.fd_array[i]);
                if (iter != sock_list.end())
                {

                    sock_list.erase(iter);
                    closesocket(fdRead.fd_array[i]);
                }
            }
        }
        printf("server 空闲时间执行其他的任务...\n");

        // //将接受到的客户端加入select监听
        // for(size_t n = 0;n < sock_list.size();n++){
        //     addFdSelect(sock_list[n]);
        // }
        // //将serverfd加入select监听
        // addFdSelect(serverfd);
    }

    // 关闭sock_list 中的套接字
    for (size_t n = 0; n < sock_list.size(); n++)
    {
        closesocket(sock_list[n]);
    }

    sock_list.clear();

    closesocket(serverfd);

    WSACleanup();
    return 0;
}
// 添加套接字到select
int addFdSelect(SOCKET fd)
{
    FD_SET(fd, &fdRead);
    FD_SET(fd, &fdWrite);
    FD_SET(fd, &fdExcept);
    return 0;
}

// 删除select套接字
int delFdSelect(SOCKET fd)
{
    FD_CLR(fd, &fdRead);
    FD_CLR(fd, &fdWrite);
    FD_CLR(fd, &fdExcept);
    return 0;
}

// 处理cmd的程序
int processCmd(SOCKET _sock)
{
    // 5.读取客户端的命令
    DataHeader header = {0};
    int readLen = recv(_sock, (char *)&header, sizeof(DataHeader), 0);
    if (readLen <= 0)
    {
        perror("read 失败...");
        return -1;
    }

    // printf("读取到的指令为:%s\n",readBuff);

   
    switch (header.cmd)
    {
        case CMD_LOGIN:
        {
             printf("收到的cmd:CMD_LOGIN,数据长度:%d\n", header.dataLength);

            struct Login login;
            recv(_sock, (char *)&login + sizeof(struct DataHeader), sizeof(struct Login) - sizeof(struct DataHeader), 0);

            // 省略判断用户的过程
            LogInResult ret;
            ret.result = 0;
            send(_sock, (const char *)&ret, sizeof(LogInResult), 0);

            printf("客户端登录成功...\n");
            printf("用户名:%s,用户密码:%s\n", login.userName, login.passWord);

            break;
            return 0;
        }

        case CMD_LOGOUT:
        {
            printf("收到的cmd:CMD_LOGOUT,数据长度:%d\n",  header.dataLength);

            Logout logout = {};
            recv(_sock, (char *)&logout + sizeof(struct DataHeader), sizeof(struct Logout) - sizeof(struct DataHeader), 0);
            // 省略判断用户的过程
            LogOutResult ret;
            ret.result = 0;
            send(_sock, (const char *)&ret, sizeof(LogInResult), 0);

            printf("客户端 %s退出成功...\n", logout.userName);
            break;
           
        }

        case CMD_QUIT:
        {
            printf("收到的cmd:CMD_QUIT,数据长度:%d\n",  header.dataLength);

            printf("服务器关闭...\n");
            break;
            
        }

        default:
        {
            printf("???\n");
            break;
            
        }
           
    }

    // if(0 == strcmp("quit",readBuff)){
    //     memset(readBuff,0,READBUFF);
    //     closesocket(clientfd);
    //     break;
    // }else if(0 == strcmp("whoami",readBuff)){
    //     int sendLen = send(clientfd,"jame",5,0);
    //     if(sendLen <= 0){
    //          perror("send 失败...");
    //     }
    // }else{
    //     int sendLen = send(clientfd,"未知的命令",16,0);
    //     if(sendLen <= 0){
    //          perror("send 失败...");
    //     }
    // }
    // memset(readBuff,0,READBUFF);

    return 0;
}
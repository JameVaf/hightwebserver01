#define WIN32_LEAN_AND_MEAN
#ifdef _WIN32
    #include<windows.h>
    #include<winsock2.h>
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

#pragma comment(lib,"ws2_32.lib")
int ret = -1;
#define CMDBUFF 128
#define READBUFF 128
char cmdBuff [CMDBUFF] = {0};
char readBuff [READBUFF] = {0};

static int testnumnber = 0;

//命令宏
enum CMD{
    CMD_LOGIN=1,    //登录命令
    CMD_LOGOUT,     //登出命令
    CMD_ERROR,      //错误的命令
    CMD_QUIT,       //退出命令
    CMD_LOGIN_RESULT,//返回结果命令
    CMD_LOGOUT_RESULT,
    CMD_JOIN        //其他用户加入服务器的
};

//消息头
struct DataHeader{
    short dataLength;//数据的长度
    short cmd ; //命令
};

//登录头
struct Login:public DataHeader{
    
    Login():userName({0}),passWord({0}){
        dataLength = sizeof(Login);
        cmd = CMD_LOGIN;
    }
    char userName [32];
    char passWord [32];
    
};

//登录返回结果头
struct LogInResult:public DataHeader{
    
    LogInResult():result(0){
        dataLength = sizeof(LogInResult);
        cmd = CMD_LOGIN_RESULT;
    }
    int result;
    
};


//登出头
struct Logout:public DataHeader{
    Logout():userName({0}){
        dataLength = sizeof(Logout);
        cmd = CMD_LOGOUT;
    }
    char userName [32];
 
    
};

//登出返回结果头
struct LogOutResult:public DataHeader{
    
    LogOutResult():result(0){
        dataLength = sizeof(LogOutResult);
        cmd =CMD_LOGOUT_RESULT;
    }
    int result;
    
};

//JOIN结构体
struct Join:public DataHeader{
    Join():ipAddress({0}){
        dataLength = sizeof(Join);
        cmd = CMD_JOIN;
       
    }
    char ipAddress [32] ;
};
//读取werver数据的函数
int process(SOCKET serverfd,const DataHeader& header);
//CMD命令的线程工作函数
int working(SOCKET serverfd);


int main(void){
    #ifdef _WIN32

    WORD ver = MAKEWORD(2,2);
    WSADATA dat;
    WSAStartup(ver,&dat);
    #else

    #endif
    //1.创建socket套接字
    SOCKET serverfd = socket(AF_INET,SOCK_STREAM,0);
    if(INVALID_SOCKET == serverfd){
        perror("socket 失败!");
    }

  

    //2.进程connect连接
    struct sockaddr_in serveraddr;
    serveraddr.sin_addr.s_addr = inet_addr("192.168.189.57 ");
    serveraddr.sin_port = htons(1234);
    serveraddr.sin_family = AF_INET;

    ret = connect(serverfd,(sockaddr*)&serveraddr,sizeof(serveraddr));
    int error =  WSAGetLastError();
    //printf("error:%d\n",error);

    if(SOCKET_ERROR == ret){
        printf("ret = %d\n",ret);
        perror("connect 失败...");
       
    }

    printf("客户端开始连接...\n");
    //向server发送CMD
    std::thread t(working,serverfd);
    t.detach();
    

    while(1){
        


        fd_set fdReads;
        FD_ZERO(&fdReads);
        FD_SET(serverfd,&fdReads);
        
        printf("第%d次select 开始工作\n",++testnumnber);
        int ret = select(serverfd,&fdReads,nullptr,nullptr,0);
        printf("ret = %d\n",ret);
        if( ret < 0){
            printf("seletc 任务结束\n");
            break;
        }
       
        if(FD_ISSET(serverfd,&fdReads)){//服务器发来的数据
            FD_CLR(serverfd,&fdReads);
            //读取数据的缓冲区
            //char readbuff[512] = {0};
            //构造datahead接受数据
            struct DataHeader  clientHead = {0};
            if(-1 == recv(serverfd,(char*)&clientHead,sizeof(struct DataHeader),0)){
                process(serverfd,clientHead);
            }
          
            
        }
     



       

    
        
        
        // int sendLen = send(serverfd,cmdBuff,strlen(cmdBuff)+1,0);
        // if(sendLen <= 0){
        //     perror("send 失败");
        // }
        // if(0 == strcmp("quit",cmdBuff)){
        //     memset(cmdBuff,0,CMDBUFF);
        //     break;
        // }
        // //4.从服务器读取结果
        // int readLen = recv(serverfd,readBuff,READBUFF,0);
        // if(readLen <= 0){
        //     perror("recv 读取服务器失败...");
        // }
        // printf("server send:%s\n",readBuff);
        // memset(readBuff,0,READBUFF);


    }


    
    #ifdef _WIN32
    closesocket(serverfd);
    WSACleanup();
    #else
    close(serverfd);
    #endif

    return 0;
}

//处理server数据的process的实现
int process(SOCKET serverfd,const DataHeader& header){
    //1.判断server传过来的命令
    printf("process 开始工作\n");
    switch (header.cmd)
    {
    case CMD_JOIN: {//server发送的是join信息
    //读取接受缓冲区里的数据
    char readBuff [124] = {0}; 
    int ret = recv(serverfd,readBuff,sizeof(readBuff),0);
    if(-1 == ret){
        printf("server发过来的数据接受失败!\n");
        return -1;
    }
    printf("新加入一个连接,ip:%s\n",readBuff);
    break;
    }
    case CMD_LOGIN_RESULT:{
        int result = -1;
        int ret = recv(serverfd,(char*)&result,sizeof(result),0);
        if(-1 == ret){
            printf("接受结果失败");
        }
        //server服务器返回的结果
        printf("登录server返回的结果为:%d\n",result);
        break;
    }
    case CMD_LOGOUT_RESULT:{
        int result = -1;
        int ret = recv(serverfd,(char*)&result,sizeof(result),0);
        if(-1 == ret){
            printf("接受结果失败");
        }
        //server服务器返回的结果
        printf("登出server返回的结果为:%d\n",result);
        break;
    }
    default:
        printf("服务器数据异常\n");
        break;
    }
    return 0;
}

//线程里的工作函数
int working(SOCKET serverfd){
    printf("线程开始工作\n");
    while(true){
        printf("请输入命令:");
        std::cin>>cmdBuff;
        if(0 == strcmp(cmdBuff,"login")){
            //创建连接头
            Login login = {};
            login.cmd = CMD_LOGIN;
            
            memset(cmdBuff,0,CMDBUFF);
            printf("请输入username:");
            std::cin>>cmdBuff;
            strncpy(login.userName,cmdBuff,strlen(cmdBuff)+1);

            memset(cmdBuff,0,CMDBUFF);
            printf("请输入password:");
            std::cin>>cmdBuff;
            strncpy(login.passWord,cmdBuff,strlen(cmdBuff)+1);
            //scanf("%s\n",cmdBuff);
            //3.将命令发送给服务器
            int ret = send(serverfd,(char*)&login,sizeof(struct Login),0);
            if( -1 == ret){
                printf("send Login命令到server失败\n");
                return -1;
            }

            

        }else if(0 == strcmp(cmdBuff,"logout")){
            
            Logout logout = {};
            logout.cmd = CMD_LOGOUT;
            printf("请输入要退出的用户名:");
            std::cin>>cmdBuff;
            strncpy(logout.userName,cmdBuff,strlen(cmdBuff)+1);
            //发送指令
            int ret = send(serverfd,(const char*)&logout,sizeof(struct Logout),0);
            if( -1 == ret){
                printf("send Login命令到server失败\n");
                return -1;
            }
          

        }else if(0 == strcmp(cmdBuff,"quit")){
            struct DataHeader quit01;
            quit01.cmd = CMD_QUIT;
            //发送指令
            
            send(serverfd,(const char*)&quit01,sizeof(DataHeader),0);
            printf("客户端开始关闭...");
            return 0;
            
        }else{
            
            printf("本次输入为未知命令!!!\n");
          
            
        }
    }
  
    return 0;
}
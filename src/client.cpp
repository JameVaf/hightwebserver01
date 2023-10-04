#include<iostream>
#include<thread>
#include "../include/EasyTcpClient.hpp"
#define CMDBUFF 1024
char cmdBuff[CMDBUFF] = {0};
// struct Test :public DataHeader{
//     Test(){
//         cmd = 100;
//         dataLength = sizeof(Test);
//     }
//     char testChar[1000] = {'5'};
// };

int working(EasyTcpClient* client ){
    
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
            int ret = client->Send((DataHeader*)&login);
            if( -1 == ret){
                printf("send Login命令到server失败\n");
                return -1;
            }
            
            printf("已发送数据:%d bytes,username:%s,password:%s\n",ret,login.userName,login.passWord);

            

        }else if(0 == strcmp(cmdBuff,"logout")){
            
            Logout logout = {};
            logout.cmd = CMD_LOGOUT;
            printf("请输入要退出的用户名:");
            std::cin>>cmdBuff;
            strncpy(logout.userName,cmdBuff,strlen(cmdBuff)+1);
            //发送指令
            int ret = client->Send((DataHeader*)&logout);
            if( -1 == ret){
                printf("send Login命令到server失败\n");
                return -1;
            }
          

        }else if(0 == strcmp(cmdBuff,"quit")){
            struct DataHeader quit01;
            quit01.cmd = CMD_QUIT;
            //发送指令
            
            client->Send((DataHeader*)&quit01);
            client->Close();
            printf("客户端开始关闭...\n");
            
            return 0;
            
        }else{
            
            printf("本次输入为未知命令!!!\n");
          
            
        }
    }
  return 0;

};
  

int main(void){
    EasyTcpClient client;
    client.initSocket();
    client.Connect("192.168.1.8",1234);
    //启动发送线程
    std::thread t(working,&client);
    t.detach();
   
    //Test y;
    while(client.isRun()){
        client.OnRun();
       // client.Send((DataHeader*)&y);
    }


}
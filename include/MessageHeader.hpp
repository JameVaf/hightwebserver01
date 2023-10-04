#ifndef MESSAGEHEADER_H
#define MESSAGEHEADER_H
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
    
    Login(){
        dataLength = sizeof(Login);
        cmd = CMD_LOGIN;
    }
    char userName [32] = {0};
    char passWord [32] = {0};
    
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
    Logout(){
        dataLength = sizeof(Logout);
        cmd = CMD_LOGOUT;
    }
    char userName [32] = {0};
 
    
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
    Join(){
        dataLength = sizeof(Join);
        cmd = CMD_JOIN;
       
    }
    char ipAddress [32] = {0} ;
};

#endif //MESSAGEHEADER_H
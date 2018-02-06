/*************************************************************************
	> File Name: tiny_server.c
	> Author: 
	> Description: 
	> Mail: 
	> Created Time: 2018年02月07日 星期三 00时10分44秒
 ************************************************************************/
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/wait.h>

/**
 * 标准输入、输出以及标准出错定义
 **/
#define STDIN   0                   //标准输入
#define STDOUT  1                   //标准输出
#define STDERR  2                   //标准出错

/**
 * 服务器名称（\r\n是为了兼容windows下的换行格式，暂时并未测试可能在linux下出现错误，之后可通过宏定义变成跨平台版本）
 **/
#define SERVER_STRING   "Server: tiny_server/0.1\r\n"

/**
 *  函数功能：  程序异常终止，并输出相应的log信息
 *  函数参数：  msg     需要传递的错误信息
 *  返回值：    函数无返回值
 *              但程序返回值为1
 **/
void error_exit(const char *msg)
{
    perror(msg);
    exit(1);
}

/**
 * 函数功能：   对函数返回值进行判断
 * 函数参数：   ret     函数返回值
 *              abort   需要进行比较的条件，<
 *              msg     错误发生时的log信息
 * 返回值：     无
 *              错误发生主程序返回1
 **/
void ret_judge(int ret, int abort,const char *msg)
{
    if(ret < abort)
    {
        error_exit(msg);
    }
}

/**
 * socket建立链接的过程
 *
 *              客户端          服务端
 *              socket          socket      （获取socket）
 *                ↓                ↓
 *                ↓              bind        (绑定socket)
 *                ↓                ↓
 *                ↓             listen      （监听端口）
 *                ↓                ↓
 *                ↓             accept      （接受链接）
 *                ↓     建立连接   ↓
 *             connect←------------↓        （建立链接）
 *                ↓                ↓               
 *                ↓ ←-------------→↓
 *                ↓   互相读写数据 ↓        
 *                ↓ ←-------------→↓
 *                ↓                ↓
 *              close            close      （关闭socket,服务端）
 **/

/**
 * 函数功能：   开启服务器，包括获取服务端socket，绑定socket，监听端口
 * 函数参数：   port    端口号
 * 返回值：     正常返回服务器的socket
 *              错误则程序终止，并打印出错信息
 **/
int startup(u_short *port)
{
    int ret = 0;
    int server_socket = -1;
    int on = 1;
    struct sockaddr_in server;

    server_socket = socket( PF_INET, SOCK_STREAM, 0);       //使用IPV4，数据流传输自动选择传输协议
    if(-1 == server_socket)
    {
        error_exit("server socket create failed!");
    }

    memset(&server, 0, sizeof(server));
    
    //初始化server
    server.sin_family = AF_INET;
    server.sin_port = htos(*port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    
    ret = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));        //设置通用socket，允许重用本地地址和端口
    ret_judge( ret, 0, "set socket failed");

    ret = bind(server_socket, (struct sockaddr *)&server, sizeof(server));
    ret_judge( ret, 0, "bind socket fialed");
    if(0 == *port)                  //动态分配端口
    {
        socklen_t server_len = sizeof(server);
        ret = getsockname(server_socket, (struct sockaddr *)&server, &server_len);
        ret_judge( ret, 0, "allocate port dynamicaly failed");
        *port = ntohs(server.sin_port);
    }

    ret = listen( server_socket, 5);            //监听端口
    ret_judge( ret, 0, "listen port failed");

    return server_socket;
}

int main(void)
{
    int server_socket = -1;                     //服务端socket
    int client_socket = -1;                     //客户端socket
    u_short server_port = 4040;                 //服务器端口号
    struct sockaddr_in client;
    
    socklen_t client_len = sizeof(client);
    pthread_t main_thread;

    server_socket = startup(&port);             //开启服务并获取服务端socket
    printf("server start on port %d\n",server_port);
    while(1)
    {
        client_socket = accept( server_socket, (struct sockaddr *)&client, &client_len);
        ret_judge( client_socket, 0, "accept client failed");
        int ret = pthread_create( &main_thread, NULL, (void *)accept_request, (void *)(intptr_t)client_socket);               //建立新线程管理客户端
        if(ret != 0)
        {
            error_exit("pthread create fialed");
        }
    }

    close(server_socket);                       //关闭服务端socket
    return 0;
}

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

#define IS_SPACE(x) isspace((int)(x))
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
 *              op      操作符
 *              abort   需要进行比较的条件，<
 *              msg     错误发生时的log信息
 * 返回值：     无
 *              错误发生主程序返回1
 **/
void ret_judge( int ret, char op, int abort, const char *msg)
{
    switch(op):
    {
    case '=':
        if(ret == abort)  
            error_exit(msg);
        break;
    case '<':
        if(ret < abort)
            error_exit(msg);
        break;
    case '!':
        if(ret != abort)
            error_exit(msg);
        break;
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

    server_socket = socket( PF_INET, SOCK_STREAM, 0);                                  //使用IPV4，数据流传输自动选择传输协议
    ret_judge( server_socket, '=', -1, "server socket create failed");
    memset(&server, 0, sizeof(server));
    
    //初始化server
    server.sin_family = AF_INET;
    server.sin_port = htos(*port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    
    ret = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));        //设置通用socket，允许重用本地地址和端口
    ret_judge( ret, '<', 0, "set socket failed");

    ret = bind(server_socket, (struct sockaddr *)&server, sizeof(server));
    ret_judge( ret, '<', 0, "bind socket fialed");
    if(0 == *port)                                                                     //动态分配端口
    {
        socklen_t server_len = sizeof(server);
        ret = getsockname(server_socket, (struct sockaddr *)&server, &server_len);
        ret_judge( ret, '=', -1, "allocate port dynamicaly failed");
        *port = ntohs(server.sin_port);
    }

    ret = listen( server_socket, 5);                                                   //监听端口
    ret_judge( ret, '<', 0, "listen port failed");

    return server_socket;
}
/**
 * 函数功能：   处理客户端请求
 * 函数参数：   args    在线程创建时传递，这里是client_socket
 **/
void accept_request(void *arg)
{
    int client_socket = (intptr_t)arg;

    char buf[1024];                             //所有数据缓冲区
    char method[255];                           //用于存储请求方法
    char url[255];                              //用于存储URL
    char path[255];
    char *query_desc = NULL;                    //请求描述
    
    struct stat state;

    size_t i = 0;                                      //索引
    size_t j = 0;

    int cgi_flag = 0;

    size_t buf_size = get_line(client_socket, buf, sizeof(buf));
    while( !IS_SPACE(buf[i] && (i < sizeof(method) - 1)))
    {
        method[i] = buf[i];
        i ++;
    }

    method[i] = '\0';
    j = i;
    if(strcasecmp( method, "GET") && strcasecmp( method, "POST"))//无法识别的请求方式
    {   
        unknown_method( client_socket); 
        return;
    }

    if(strcasecmp(method, "POST") == 0)
    {
        cgi_flag = 1;
    }

    i = 0;
    while(IS_SPACE(buf[j]) && (j < len))
    {
        j ++;
    }

    while((!IS_SPACE(buf[j])) && (i < sizeof(url) - 1) && (j < len))
    {
        url[i++] = buf[j++];
    }

    url[i] = '\0';
    if(strcasecmp( method, "GET") == 0)             //GET请求方法
    {
        query_string = url;
        while((*query_string != '?') && (*query_string != '\0'))
        {
            query_string ++;
        }

        if(*query_string == '?')
        {
            cgi = 1;
            *query_string = '\0';
            query_string ++;
        }
    }

    sprintf( path, "htdocs%s", url);
    if(path[strlen(path) - 1] == '/')
    {
        strcat(path, "index.html");
    }

    if(stat( path, &state) == -1)       //获取文件信息失败
    {
        while((len > 0) && strcmp("\n", buf))
        {
            len = get_line( client_socket, buf, sizeof(buf));
        }

        not_found( client_socket);
    }
    else                                //获取文件信息成功
    {
        if((state.st_mode & S_IFMT) == S_IFDIR)     //路径文件是个目录
        {
            strcat( path, "/index.html");
        }
        
        if((state.st_mode & S_IXGRP) || (state.st_mode & S_IXOTH))  //用户组拥有可执行权限或者其他组成员拥有可执行权限
        {
            cgi = 1;
        }

        if(!cgi)
        {
            serve_file(client_socket, path)
        }
        else
        {
            execute_cgi( client_socket, path, method, query_string);
        }
    }

    close(client_socket);
}

/**
 * 函数功能：   从客户端接受数据存入buf缓冲区
 * 函数参数：   client  客户端socket
 *              buf     预存缓冲区
 *              size    缓冲区大小
 * 返回值：     读取的数据的长度
 **/
size_t get_line(int client, char *buf, size_t size)
{
    int i = 0;
    int len = 0;
    char ch = '\0';
    while((i < size - 1) && (ch != '\n'))       //最后一位作为字符串结束标志位'\0'
    {
        len = recv( client, &ch, 1, 0);      //从缓冲区读取数据并擦除缓冲区
        if(len > 0)
        {
            if(ch == '\r')                  //兼容windows换行格式
            {
                len = recv( client, &ch, 1, MSG_PEEK);      //从缓冲区读取数据并不擦除缓冲区
                if( (len > 0) && (ch != '\n'))
                {
                    recv( client, &ch, 1, 0);               //从缓冲区读取数据并擦除缓冲区
                }   
                else
                {
                    ch = '\n';
                }
            }

            buf[i] = ch;
            i ++;
        } 
        else
        {
            ch = '\n';
        }
    }

    buf[i] = '\0';              //结束符
    return i;
}

/**
 * 函数功能：   向客户端发送不支持响应
 * 函数参数：   client  客户端socket
 * 函数返回值： 无
 **/
void unknown_method(int client) 
{
    char buf[1024];
    sprintf( buf, "HTTP/1.0 501 Method Can Not Be Recognized\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf( buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf( buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);

    sprintf( buf, "\r\n");
    send(client, buf, strlen(buf), 0);

    sprintf( buf, "<html><head><title>Method Can Not Be Recognized\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf( buf, "</title></head>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf( buf, "<body><p>HTTP Request Method Do Not Supported.</p>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf( buf, "</body></html>\r\n");
    send(client, buf, strlen(buf), 0);
}
/**
 * 函数功能：   对无法访问站点资源进行响应
 * 函数参数：   client  客户端socket
 * 返回值： 无
 **/
void not_found(int client)
{
    char buf[1024];
    sprintf(buf,"HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf,SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf,"Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);

    sprintf(buf,"\r\n");
    send(client, buf, strlen(buf), 0);

    sprintf(buf,"<html><title>404 Not Found</title>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf,"<body><p>The Server Could Not Found</p>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf,"your request becase the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf,"is unavailable or nonexistent\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf,"</body></html>\r\n");
    send(client, buf, strlen(buf), 0);
    
}

/**
 * 函数功能：   
 * 函数参数：
 * 返回值：
 **/
void serve_file( int client, const char *filename)
{
    FILE *resource = NULL;
    int len = 1;
    char buf[1024];

    buf[0] = 'A';                   //??
    buf[1] = '\0';
    while( len > 1 && strcmp('\n' buf))
    {
        len = get_line( client, buf, sizeof(buf));
    }

    resource = fopen(filename, "r");            //只读模式打开文件
    if(resouce == NULL)                 //文件错误
    {
        not_found(client);
    }
    else
    {
        headers(client, filename);
        cat( client, resource);
    }

    fclose(resource);
}

/**
 * 函数功能：   发送响应头
 * 函数参数：   client  客户端socket
 * 返回值：     无
 **/
void headers(int client, const char *filename)
{
    char buf[1024];
    (void)filename;                 //??
    
    sprintf( buf, "HTTP/1.0 200 OK\r\n");
    send( client, buf, strlen(buf), 0);
    sprintf( buf, SERVER_STRING);
    send( client, buf, strlen(buf), 0);
    sprintf( buf, "Content-Type: text/html\r\n");
    send( client, buf, strlen(buf), 0);
    sprintf( buf, "\r\n");
    send( client, buf, strlen(buf), 0);

}

/**
 * 函数功能：   从resource文件中读取数据发送个客户端
 * 函数参数：   resource    资源文件描述符
 * 返回值：     无
 **/
void cat(int client, FILE *resource)
{
    char buf[1024];
    fgets(buf, sizeof(buf), resource);
    while(!feof(resource))
    {
        send(client,buf,strlen(buf),0);
        fgets(buf, sizeof(buf), resource);
    }
}

/**
 * 函数功能：   执行CGI
 * 函数参数：   client          客户端socket
 *              path            文件路径
 *              method          请求方式
 *              query_string    请求描述
 * 返回值：     无
 **/
void execute_cgi( int client, const char *path, const char *method, const char *query_string)
{
    char buf[1024];
    char cgi_in[2];
    char cgi_out[2];

    int len = 0;
    int content_length = -1;

    pid_t pid = 0;

    char ch =  '';
    int status;

    int i = 0;

    buf[0] = 'A';
    buf[1] = '\0';
    if(strcasecmp(method, "GET") == 0)              //GET方法
    {
        while((len > 0) && strcmp( "\n", buf))
        {
            len = get_line( client, buf, sizeof(buf));
        }
    }
    else if(strcasecmp(method, "POST") == 0)        //POST方法
    {
        len = get_line( client, buf, sizeof(buf));
        while((len > 0) && strcmp("\n", buf))
        {
            buf[15] = '\0';
            if(strcasecmp(buf, "Content-Length") == 0)
            {
                content_length = atoi(&buf[16]);
            }

            len = get_line(client, buf, sizeof(buf));
        }

        if(content_length == -1)
        {
            bad_request(client);
            return;
        }
    }
    else
    {}

    if(pipe(cgi_in) < 0)
    {
        cannot_execute(client);
        return;
    }

    if(pipe(cgi_out) < 0)
    {
        connot_execute(client);
        return;
    }

    if((pid = fork()) < 0)
    {
        connot_execute(client);
        return;
    }

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, sizeof(buf), 0);
    if(pid == 0)                //子进程
    {
        char method_env[255];  
        char query_env[255];  
        char length_env[255];  

        close(cgi_in[1]);
        close(cgi_out[0]);

        dup2(cgi_out[1], STDOUT);
        dup2(cgi_in[0], STDIN);

        sprintf( method_env, "REQUEST_METHOD=%s", method);
        putenv(method_env);
        if(strcasecmp( method, "GET") == 0)
        {
            sprintf( query_env, "QUERY_STRING=%s", query_string);
            putenv(query_env);
        }
        else
        {
            sprintf(length_env, "CONTENT-LENGTH=%d", content-length);
            putenv(length_env);
        }

        execl(path, NULL);
        exit(0);
    }
    else                        //父进程
    {
        close(cgi_in[0]);
        close(cgi_out[1]);

        if(strcasecmp(method, "POST") == 0)
        {
            for(i = 0;i < content_length;i ++)
            {
                recv(client, &ch, 1, 0);
                write( cgi_in[1], &ch, 1);
            }
        }

        while(read(cgi_out[0], &ch, 1) > 0)
        {
            send(client, &ch, 1, 0);
        }

        close(cgi_in[1]);
        close(cgi_out[0]);
        waitpid(pis, &status, 0);
    }
}

/**
 * 函数功能：   向客户端响应无法运行CGI
 * 函数参数：   client  客户端socket
 * 返回值：     无
 **/
void cannot_execute(int client)
{
    char buf[1024];
    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-tyoe: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);

    sprintf(buf, "<p>Error Prohibited CGI execution</p>\r\n");
    send(client, buf, strlen(buf), 0);
}

/**
 * 函数功能：   对用户的错误请求发出响应
 * 函数参数：   client  客户端socket
 * 返回值：     无
 **/
void bad_request(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 400 Bad Request\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, sizeof(buf), 0);

    sprintf(buf, "<p>Your Request Can Not Be Analysed,\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "Such As A POST Without A Content-Length.\r\n");
    send(client, buf, sizeof(buf), 0);
}

/**
 * 主线程负责打开网络监听，接受客户端链接，为客户端创建执行线程
 **/ 
int main(void)
{
    int server_socket = -1;                     //服务端socket
    int client_socket = -1;                     //客户端socket
    u_short server_port = 4040;                 //服务器端口号
    struct sockaddr_in client;
    
    socklen_t client_len = sizeof(client);
    pthread_t client_thread;

    server_socket = startup(&port);             //开启服务并获取服务端socket
    printf("server start on port %d\n",server_port);
    while(1)
    {
        client_socket = accept( server_socket, (struct sockaddr *)&client, &client_len);
        ret_judge( client_socket, '=', -1, "accept client failed");
        int ret = pthread_create( &client_thread, NULL, (void *)accept_request, (void *)(intptr_t)client_socket);               //建立新线程管理客户端
        ret_judge( ret, '!', 0, "pthread create fialed");
    }

    close(server_socket);                       //关闭服务端socket
    return 0;
}

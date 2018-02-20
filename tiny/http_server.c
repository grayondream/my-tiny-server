/*************************************************************************
    > File Name: http_server.c
    > Author:
    > Description:
    > Mail:
    > Created Time: 2018年02月20日 星期二 22时24分11秒
 ************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/*---------------------------------------defination---------------------------------------------*/
/**
 * test if the character is a space character
 **/
#define is_space(ch)    \
    ((ch == ' ') || (ch > '\t' && ch < '\r'))

/**
 * the enter
 **/
#define END_CH  "\r\n"

/**
 * output the log
 **/
#define LLOG(format, ...)    \
    fprintf( stderr, " %s:%s:%d<> error %d:%s" format END_CH,\
            __FILE__, __func__, __LINE__, errno, strerror(errno), ##__VA_ARGS__)

/**
 * error exit
 **/
#define CERROR_EXIT(format, ...)    \
    LLOG(format, ##__VA_ARGS__), exit(EXIT_FAILURE)

/**
 * test return value from expression
 */
#define CHECK_RET(ret)  \
    if((ret) < 0)     \
        CERROR_EXIT(#ret)

#define BUF_SIZE (1024)                 //size of buffer

#define LISTEN_LEN (10)                 //the length of listen quene

/*---------------------------------------function defination-----**-----------------------------*/

int startup(__uint16_t *port);

void *request_accept(void *arg);

void response_400(int sock);            //request error,return 400
void response_404(int sock);            //the file is error,return 404
void response_501(int sock);            //unsupported request,return 501
void response_500(int sock);            //internal error,return 500
void response_200(int sock);            //ok,return 200

int get_line(int fd, char *buf, int size);

void response_file(int fd, const char *path);

void *request_accept(void *arg);

void request_cgi(int sock, const char *path, const char *type, const char *query);

void request_cgi(int sock, const char *path, const char *type, const char *query);

int main(int argc, char **argv)
{
    pthread_attr_t thread;
    __uint16_t  port = 4040;

    int sock = startup(&port);
    printf("http server listen on port %u!\n", port);

    //initialize thread
    pthread_attr_init(&thread);
    pthread_attr_setdetachstate(&thread, PTHREAD_CREATE_DETACHED);

    while(1)
    {
        pthread_t t_id;
        struct sockaddr_in c_addr;
        socklen_t c_len = sizeof(c_addr);

        int c_sock = accept(sock, (struct sockaddr *)&c_addr, &c_len);
        if(c_sock < 0)
        {
            LLOG("accept socket %d failed!\b", c_sock);
            break;
        }

        int ret = pthread_create(&t_id, &thread, request_accept, (void *)c_sock);
        if(ret < 0)
        {
            LLOG("thread create fialed!\n");
        }
        else
        {
            LLOG("a connection is done!");
        }
    }

    //destroy the thread and close socket
    pthread_attr_destroy(&thread);
    close(sock);

    return 0;
}

/**
 * startup service and listen internet port
 * @param port port of server socket
 * @return server socket
 */
int startup(__uint16_t *port)
{
    struct sockaddr_in addr = {AF_INET};

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    CHECK_RET(sock);

    addr.sin_port = !port || !*port ? 0 : htons(*port);
    addr.sin_addr.s_addr = INADDR_ANY;

    int ret = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    CHECK_RET(ret);
    if(port && !*port)
    {
        socklen_t len = sizeof(addr);
        ret = getsockname(sock, (struct sockaddr *)&addr, &len);
        CHECK_RET(ret);
        *port = ntohs(&addr.sin_port);
    }

    ret = listen(sock, LISTEN_LEN);
    CHECK_RET(ret);
    return sock;
}

/**
 * request error
 * @param sock client socket
 */
void response_400(int sock)
{
    const char *res = "HTTP/1.0 400 BAD REQUEST"END_CH
                      "Server: tiny httpd 1.0"END_CH
                      "Content-Type: text/html"END_CH
                      END_CH
                      "<p>the request is wrong somewhere,please check your request!</p>"END_CH;

    write(sock, res,strlen(res));
}

/**
 * request is error, return 404
 * @param sock client socket
 */
void response_404(int sock)
{
    const char *res = "HTTP/1.0 404 NOT FOUND"END_CH
                      "Server: tiny httpd 1.0"END_CH
                      "Content-Type: text/html"END_CH
                      END_CH
                      "<html>"
                      "<head><title>the page you request can be responsed!</title></head>"END_CH
                      "<body><p>404:check your request!</p></body>"
                      "</html>";

    write(sock, res,strlen(res));
}

/**
 * the request is not supported and return 501
 * @param sock client socket
 */
void response_501(int sock)
{
    const char *res = "HTTP/1.0 501 Method Not Implemented"END_CH
            "Server: tiny httpd 1.0"END_CH
            "Content-Type: text/html"END_CH
            END_CH
            "<html>"
            "<head><title>your request is not supported!</title></head>"END_CH
            "<body><p>revise your request!</p></body>"
            "</html>";

    write(sock, res,strlen(res));
}

/**
 * there are some internal error in server return 500
 * @param sock client socket
 */
void response_500(int sock)
{
    const char *res = "HTTP/1.0 500 Internal Server Error"END_CH
            "Server: tiny httpd 1.0"END_CH
            "Content-Type: text/html"END_CH
            END_CH
            "<html>"
            "<head><title>Sorry!We are some problem!</title></head>"END_CH
            "<body><p>Please wait!We will fix it!</p></body>"
            "</html>";

    write(sock, res,strlen(res));
}

/**
 * request successfully return 200 to client
 * @param sock client socket
 */
void response_200(int sock)
{
    const char *res = "HTTP/1.0 200 OK"END_CH
            "Server: tiny httpd 1.0"END_CH
            "Content-Type: text/html"END_CH
            END_CH;

    write(sock, res,strlen(res));
}

/**
 * get content
 * @param fd description flag
 * @param buf buffer
 * @param size length of buffer
 * @return length of buffer after receiving
 */
int get_line(int fd, char *buf, int size)
{
    char *tmp = buf;
    char ch = ' ';
    int ret = 0;
    while( (tmp - buf) < (size - 1))
    {
        ret = read(fd, &ch, 1);
        if(ret <= 0) break;

        if(ch == '\r')
        {
            ret = recv(fd, &ch, 1, MSG_PEEK);
            //ret = read(fd, &ch, 1);
            if(ret > 0 && ch == '\n')
                read(fd, &ch, 1);
            else
                *tmp++ = '\n';
            break;
        }

        *tmp++ = ch;
    }

    *tmp = '\0';
    return tmp - buf;
}

/**
 * send file to client
 * @param fd the description flag of file
 * @param path path of file
 */
void response_file(int fd, const char *path)
{
    FILE *fp = NULL;
    char buf[BUF_SIZE];

    bzero(buf, sizeof(buf));
    while(get_line(fd, buf, sizeof(buf)) > 0 && strcmp("\n", buf));
    fp = fopen(path, "r");
    if(fp == NULL)
    {
        response_404(fd);
    }
    else
    {
        response_200(fd);
        while(!feof(fp) && fgets(buf, sizeof(buf), fp))
        {
            write(fd, buf, strlen(buf));
        }
    }

    fclose(fp);
}

/**
 * response the request from client
 * @param arg client socket
 * @return void
 */
void *request_accept(void *arg)
{
    char buf[BUF_SIZE], path[BUF_SIZE >> 1], type[BUF_SIZE >> 5];

    char *query = NULL;
    char *left = NULL;
    char *right = NULL;
    char *tmp = buf;

    int cgi = 0;
    struct stat state;;
    int sock = (int)arg;

    bzero(buf,sizeof(buf));
    bzero(path,sizeof(path));
    bzero(type,sizeof(type));

    int ret = get_line(sock, buf, sizeof(buf));
    if(ret <= 0)
    {
        response_501(sock);
        close(sock);
        return NULL;
    }

    //get request method, post or get
    for(left = type, right = tmp; !is_space(*right) && (left - type) < sizeof(type) - 1; *left++ = *right++)
    {;}

    *left = '\0';
    cgi = strcasecmp(type, "POST");
    if(cgi && strcasecmp(type, "GET"))
    {
        response_501(sock);
        close(sock);
        return NULL;
    }

    //delete the space in buffer
    while(*right && is_space(*right))
    {
        ++right;
    }

    //get the path
    *path = '.';
    for(left = path + 1;(left - path) < sizeof(path) - 1 && !is_space(*right); *left++ = *right++)
    {;}

    *left = '\0';
    if(cgi != 0)            //GET
    {
        for(query = path; *query && *query != '?'; ++query)
            ;

        if(*query == '?')
        {
            cgi = 0;
            *query++ = '\0';
        }
    }

    //test the path
    ret = stat(path, &state);
    if(ret < 0)
    {
        while(get_line(sock, buf, sizeof(buf)) > 0 && strcmp("\n", buf))
            ;

        response_404(sock);
        close(sock);
        return NULL;
    }

    if(state.st_mode & S_IXUSR || (state.st_mode & S_IXGRP) || state.st_mode & S_IXOTH)
    {
        cgi = 0;
    }

    if(cgi)
    {
        response_file(sock, path);
    }
    else
    {
        request_cgi(sock, path, type, query);
    }

    close(sock);
    return NULL;
}

void request_cgi(int sock, const char *path, const char *type, const char *query)
{
    char buf[BUF_SIZE];
    pid_t pid;
    int len = 0;
    char ch;
    int in[2],out[2];

    bzero(buf, sizeof(buf));
    if(strcasecmp(type ,"POST") == 0)
    {
        while(get_line(sock, buf, sizeof(buf)) > 0 && strcmp("\n", buf))
        {
            buf[15] = '\0';
            if(!strcasecmp(buf, "Content-Length:"))
            {
                len = atoi(buf + 16);
            }
        }

        if(len == -1)
        {
            response_400(sock);
            return;
        }
    }
    else
    {
        while(get_line(sock, buf, sizeof(buf)) > 0 && strcmp("\n", buf))
            ;
    }

    if(pipe(out) < 0)
    {
        response_500(sock);
        return;
    }

    if(pipe(in) < 0)
    {
        close(out[0]);
        close(out[1]);
        response_500(sock);
        return;
    }

    pid = fork();
    if(pid < 0)
    {
        close(in[0]);
        close(in[1]);
        close(out[0]);
        close(out[1]);
        response_500(sock);
        return;
    }

    if(pid == 0)
    {
        dup2(out[1], STDOUT_FILENO);
        dup2(in[0], STDIN_FILENO);

        close(out[0]);
        close(in[1]);

        sprintf(buf, "REQUEST_METHOD=%s", type);
        putenv(buf);
        if(strcasecmp(buf,"POST") == 0)
        {
            sprintf(buf, "CONTENT_LENGTH=%d", len);
        }
        else
        {
            sprintf(buf, "QUERY_SRING=%s", query);
        }

        putenv(buf);
        execl(path,path,NULL);
        exit(EXIT_SUCCESS);
    }

    //patent
    write(sock, "HTTP/1.0 200 OK"END_CH, 17);
    close(out[1]);
    close(in[0]);

    if(strcasecmp(type, "POST") == 0)
    {
        int i = 0;
        for(int i = 0;i < len;i ++)
        {
            read(sock, &ch, 1);
            write(in[1], &ch, 1);
        }
    }

    while(read(out[0], &ch, 1) > 0)
    {
        write(sock, &ch, 1);
    }

    close(out[0]);
    close(in[1]);

    waitpid(pid, NULL, 0);
}


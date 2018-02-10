/*************************************************************************
	> File Name: tiny_client.c
	> Author: 
	> Description: 
	> Mail: 
	> Created Time: 2018年02月08日 星期四 02时26分02秒
 ************************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int client_socket;
    int result;
    char ch = 'A';
    struct sockaddr_in address;
    int port = 9734;
    char ip[] = "127.0.0.1";

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(client_socket == -1)
    {
        printf("client socket create fialed,error exit!");
        printf("error: %s errno: %d\n", strerror(errno),errno);
        exit(1);
    }

    printf("create socket successfully!\n");
    memset(&address,0,sizeof(address));
    printf("%d\n",__LINE__);
    address.sin_family = AF_INET;
    printf("%d\n",__LINE__);
    address.sin_port = htons(port);
    printf("%d\n",__LINE__);
    inet_pton(AF_INET, ip, &address.sin_addr);
    printf("%d\n",__LINE__);
    
    result = connect( client_socket, (struct sockaddr *)&address, sizeof(address));
    printf("%d\n",__LINE__);
    if(result == -1)
    {
        printf("%d\n",__LINE__);
        printf("client connect failed!\n");
        printf("%d\n",__LINE__);
        printf("error: %s errno: %d\n", strerror(errno),errno);
        printf("%d\n",__LINE__);
        close(client_socket);
        exit(1);
    }

    printf("%d\n",__LINE__);
    printf("the client connected to %s:%d successfully!\n",ip,port);
    write(client_socket, &ch, 1);
    read(client_socket, &ch, 1);
    printf("server echo: %c", ch);
    close(client_socket);

    printf("close socket and exit client!");
    exit(0);
}


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

int main(int argc, char *argv[])
{
    int len = 0;
    int client_socket;
    int result;
    char ch = 'A';
    struct sockaddr_in address;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons(9090);
    len = sizeof(address);
    result = connect( client_socket, (struct sockaddr *)&address, len);
    if(result == -1)
    {
        perror("client connect failed\n");
        exit(1);
    }

    write(client_socket, &ch, 1);
    read(client_socket, &ch, 1);
    printf("server echo: %c", ch);
    close(client_client);
    exit(0);
}


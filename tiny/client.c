/*************************************************************************
	> File Name: client.c
	> Author: 
	> Description: 
	> Mail: 
	> Created Time: 2018年02月21日 星期三 00时23分39秒
 ************************************************************************/
#include<stdio.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define END_CH "\r\n"

#define CERR(format, ...) \
    fprintf(stderr,"%s:%s:%d\terror %d:%s" format END_CH,\
         __FILE__, __func__, __LINE__, errno, strerror(errno),##__VA_ARGS__)

#define CERR_EXIT(format,...) \
    CERR(format,##__VA_ARGS__),exit(EXIT_FAILURE)

#define IF_CHECK(ret)    \
    if((ret) < 0) \
        CERR_EXIT(#ret)

#define _STR_HTTP_1 "GET /index.html HTTP/1.0\r\nUser-Agent: Happy is good.\r\nHost: 127.0.0.1:"
#define _STR_HTTP_3 "\r\nConnection: close\r\n\r\n"

int main(int argc, char* argv[])
{
	char buf[1024];
	int sfd;
	struct sockaddr_in saddr = { AF_INET };
	int len, port;
	
	if((argc != 2) || (port=atoi(argv[1])) <= 0 )
		CERR_EXIT("Usage: %s [port]", argv[0]);
	
	IF_CHECK(sfd = socket(PF_INET, SOCK_STREAM, 0));
	saddr.sin_port = htons(port);
	saddr.sin_addr.s_addr = INADDR_ANY;
	IF_CHECK(connect(sfd, (struct sockaddr*)&saddr, sizeof saddr));
	
	strcpy(buf, _STR_HTTP_1);
	strcat(buf, argv[1]);
	strcat(buf, _STR_HTTP_3);
	write(sfd, buf, strlen(buf));
	
	while((len = read(sfd, buf, sizeof buf - 1))){
		buf[len] = '\0';
		printf("%s", buf);
	}
	putchar('\n');
	
	close(sfd);
	return 0;
}
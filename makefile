all: httpd client
LIBS=-lpthread
httpd:tiny_server.c
	gcc -g -W -Wall $(LIBS) -o $@ $<

client: tiny_client.c
	gcc -W -Wall -o $@ $<

clean:
	rm tiny_server

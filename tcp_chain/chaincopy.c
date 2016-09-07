#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h> 
#include <time.h>

#include "statistics.h"

#ifndef TCP_QUICKACK
	#define TCP_QUICKACK 12
#endif

char buf[4096];
char *ptr = buf;

#define PAYLOAD 1024

void error(const char *msg)
{
    	perror(msg);
    	exit(0);
}

void write_all(int wsock) {
	int byte, cnt = 0;

	while (cnt < PAYLOAD) {
                byte = write(wsock, buf + cnt, PAYLOAD - cnt);
             	if ( byte < 0 )
                         error("ERROR writing to socket");
                cnt += byte;
	}
	ptr += PAYLOAD;
}

void read_all(int rsock) {
	int byte, cnt = 0;
	//setsockopt(rsock, IPPROTO_TCP, TCP_QUICKACK, (int[]){1}, sizeof(int));
        while (cnt < PAYLOAD) {
        	byte = read(rsock, buf + cnt, PAYLOAD - cnt);
        	if ( byte < 0 )
        		error("ERROR reading from socket");
        	cnt += byte;
        }
	ptr += PAYLOAD;
}

int setupsender(struct hostent *server, int portno)
{
	/*sender setup*/
        struct sockaddr_in writeaddr;
        int writesock, reuse;
        writesock= socket(AF_INET, SOCK_STREAM, 0);
	reuse = 1;

        if (writesock < 0)
                error("ERROR opening socket1");
	
        if ( setsockopt(writesock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1 )
                error("setsockopt");

	bzero((char *) &writeaddr, sizeof(writeaddr));
        writeaddr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&writeaddr.sin_addr.s_addr, server->h_length);
        writeaddr.sin_port = htons(portno);
        /*handshake with my next node*/
	printf("connecting to my next chain node %s...\n", inet_ntoa(writeaddr.sin_addr));
        if (connect(writesock,(struct sockaddr *) &writeaddr, sizeof(writeaddr)) < 0)
                error("ERROR connecting");
	printf("connected to my next chain node %s.\n", inet_ntoa(writeaddr.sin_addr));

	return writesock;
}

int setuprecver(int *listensock, int portno)
{
	/*recver setup*/
	struct sockaddr_in readaddr, writeaddr;
        int readsock, writelen, reuse;
        *listensock = socket(AF_INET, SOCK_STREAM, 0);
	reuse = 1;
	
        if (*listensock < 0)
                error("ERROR opening socket2");

	if ( setsockopt(*listensock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1 )
    		error("setsockopt");

        bzero((char *) &readaddr, sizeof(readaddr));
        readaddr.sin_family = AF_INET;
        readaddr.sin_addr.s_addr = INADDR_ANY;
        readaddr.sin_port = htons(portno);
	
        if (bind(*listensock, (struct sockaddr *) &readaddr, sizeof(readaddr)) < 0)
                error("ERROR on binding");
        /*wait for my prev node*/
        listen(*listensock, 5);
        writelen = sizeof(writeaddr);
	/*accept my prev node*/
        printf("connecting to my previous chain node...\n");
        readsock = accept(*listensock, (struct sockaddr *) &writeaddr, &writelen);
	printf("connected to my previous chain node %s.\n", inet_ntoa(writeaddr.sin_addr));
	setsockopt(readsock, IPPROTO_TCP, TCP_QUICKACK, (int[]){1}, sizeof(int));

        if (readsock < 0)
                error("ERROR on accept");

	return readsock;
}

void starts(int rsock, int wsock)
{
	int cnt = 0;

	while (cnt < 4096) {
		read_all(rsock);
		write_all(wsock);
		cnt += PAYLOAD;
		//if( tmp[0] == 0x7b)
		//	break;
	}
}

void startc(int rsock, int wsock)
{
	struct timespec ts0, ts1;
	int cnt = 0;
	
	for(int i=0; i< 4096; i++) 
		buf[i] = (char)(i%26+97);

	while (cnt < 4096) {

		//memset(tmp, 0, size);

		//clock_gettime(CLOCK_REALTIME, &ts0);
		write_all(wsock);
		read_all(rsock);
		cnt += PAYLOAD;
		//clock_gettime(CLOCK_REALTIME, &ts1);
		//long diff = ts1.tv_nsec - ts0.tv_nsec;
		//rec[i] = diff > 0 ? diff : 1000000000 + diff; 
		//printf("all %d lat %ld\n", all, ts1.tv_nsec - ts0.tv_nsec);

		//tmp[0] = 0x7b;
		//write_all(writesock, tmp, all);
		//read_all(readsock);
		//if (tmp[0] != 0x7b)
		//	error("Unknown error");

		//print_report_lat(all, t, rec);
	}
}

int main(int argc, char *argv[])
{
        if(argc < 4)
        {
                fprintf(stderr,"usage %s <hostname> <-c/-s> <-p#>\n", argv[0]);
                exit(0);
        }

        int readsock, writesock, listensock;
	struct hostent *server = gethostbyname(argv[1]);
        char role = *(argv[2]+1);
	int portno = atoi(argv[3]+2);

        switch(role)
        {
        case 'c':		
                writesock = setupsender( server, portno );
                readsock = setuprecver( &listensock, portno );
		printf(RESULT_LINE);
		printf(RESULT_FMT);
		startc( readsock, writesock);
                break;
        case 's':
		close(listensock);
		close(readsock);
		close(writesock);
			
                readsock = setuprecver( &listensock, portno );
                writesock = setupsender( server, portno );
		starts( readsock, writesock);
                break;
        default:
                break;
        }

	close(readsock);
	close(writesock);
        return 0;
}

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

void error(const char *msg)
{
    	perror(msg);
    	exit(0);
}

void inline write_all(int writesock, char *tmp, int all) {
	int byte, wr;
	
	wr = 0;
	while (wr < all) {
                byte = write(writesock, tmp + wr, all - wr);
             	if ( byte < 0 )
                         error("ERROR writing to socket");
                wr += byte;
	}
}

void inline read_all(int readsock, char *tmp, int all) {
	int byte, re;
	setsockopt(readsock, IPPROTO_TCP, TCP_QUICKACK, (int[]){1}, sizeof(int));
	re = 0;
        while (re < all) {
        	byte = read(readsock, tmp + re, all - re);
        	if ( byte < 0 )
        		error("ERROR reading from socket");
        	re += byte;
        }
}

int setupsender(struct hostent *server, int portno)
{
	/*sender setup*/
        struct sockaddr_in writeaddr;
        int writesock, reuse;
        writesock= socket(AF_INET, SOCK_STREAM, 0);
	reuse = 1;

        if (writesock < 0)
	{
                error("ERROR opening socket1");
	}
	
        if ( setsockopt(writesock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1 )
        {
                error("setsockopt");
        }

	bzero((char *) &writeaddr, sizeof(writeaddr));
        writeaddr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&writeaddr.sin_addr.s_addr, server->h_length);
        writeaddr.sin_port = htons(portno);
        /*handshake with my next node*/
	printf("connecting to my next chain node %s...\n", inet_ntoa(writeaddr.sin_addr));
        if (connect(writesock,(struct sockaddr *) &writeaddr, sizeof(writeaddr)) < 0)
	{
                error("ERROR connecting");
	}
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
        {
                error("ERROR opening socket2");
        }

	if ( setsockopt(*listensock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1 )
	{
    		error("setsockopt");
	}

        bzero((char *) &readaddr, sizeof(readaddr));
        readaddr.sin_family = AF_INET;
        readaddr.sin_addr.s_addr = INADDR_ANY;
        readaddr.sin_port = htons(portno);
	
        if (bind(*listensock, (struct sockaddr *) &readaddr, sizeof(readaddr)) < 0)
	{
                error("ERROR on binding");
	}
        /*wait for my prev node*/
        listen(*listensock, 5);
        writelen = sizeof(writeaddr);
	/*accept my prev node*/
        printf("connecting to my previous chain node...\n");
        readsock = accept(*listensock, (struct sockaddr *) &writeaddr, &writelen);
	printf("connected to my previous chain node %s.\n", inet_ntoa(writeaddr.sin_addr));

        if (readsock < 0)
	{
                error("ERROR on accept");
	}

	return readsock;
}

void starts(int readsock, int writesock, int all, const int size)
{
	char tmp[size];
	while (all < size) {
		if (all)
			all = all << 1;
		else
			all = size;
		while (1) {
			read_all(readsock, tmp, all);
			write_all(writesock, tmp, all);
			
			if( tmp[0] == 0x7b)
				break;
		}
	}
}

void startc(int readsock, int writesock, const int t, int all, const int size)
{
	struct timespec ts0, ts1;
	char tmp[size];
	long rec[t];
	int i;
	
	while (all < size) {

		memset(tmp, 0, size);
		if (all)
			all = all << 1;
		else 
			all = size;

		i = 0;
		for (i; i < t; i++) {
			clock_gettime(CLOCK_REALTIME, &ts0);
			write_all(writesock, tmp, all);
			read_all(readsock, tmp, all);
			clock_gettime(CLOCK_REALTIME, &ts1);
			long diff = ts1.tv_nsec - ts0.tv_nsec;
			rec[i] = diff > 0 ? diff : 1000000000 + diff; 
			//printf("all %d lat %ld\n", all, ts1.tv_nsec - ts0.tv_nsec);
		}

		tmp[0] = 0x7b;
		write_all(writesock, tmp, all);
		read_all(readsock, tmp, all);
		if (tmp[0] != 0x7b)
			error("Unknown error");

		print_report_lat(all, t, rec);
	}
}

int main(int argc, char *argv[])
{
        if(argc < 4)
        {
                fprintf(stderr,"usage %s <hostname> <-c#/-s> <-p#>\n", argv[0]);
                exit(0);
        }

        int readsock, writesock, listensock;
	struct hostent *server = gethostbyname(argv[1]);
        char role = *(argv[2]+1);
        int t = atoi(argv[2]+2);
	int portno = atoi(argv[3]+2);

        switch(role)
        {
        case 'c':		
                writesock = setupsender( server, portno );
                readsock = setuprecver( &listensock, portno );
		printf(RESULT_LINE);
		printf(RESULT_FMT);
		startc( readsock, writesock, t, 1, 4096);
                break;
        case 's':
		close(listensock);
		close(readsock);
		close(writesock);
			
                readsock = setuprecver( &listensock, portno );
                writesock = setupsender( server, portno );
		starts( readsock, writesock, 1, 4096 );
                break;
        default:
                break;
        }

	close(readsock);
	close(writesock);
        return 0;
}

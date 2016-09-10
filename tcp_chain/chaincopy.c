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

#define FILESIZE 102784090  
#define PAYLOAD 1448*10

#define h_addr h_addr_list[0] 

char buf[FILESIZE];
char *rbuf = buf;
char *wbuf = buf;

void error(const char *msg)
{
    	perror(msg);
    	exit(0);
}

static inline void write_all(int wsock, int payload) {
	int byte, cnt = 0;

	while (cnt < payload) {
                byte = write(wsock, wbuf + cnt, payload - cnt);
             	if ( byte < 0 )
                         error("ERROR writing to socket");
                cnt += byte;
	}
	wbuf += payload;
}

static inline void read_all(int rsock, int payload) {
	int byte, cnt = 0;
	setsockopt(rsock, IPPROTO_TCP, TCP_QUICKACK, (int[]){1}, sizeof(int));
        while (cnt < payload) {
        	byte = read(rsock, rbuf + cnt, payload - cnt);
        	if ( byte < 0 )
        		error("ERROR reading from socket");
        	cnt += byte;
        }
	rbuf += payload;
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

void starts(int rsock, int wsock, int last)
{
	FILE *f;
	int payload, cnt = 0;
	struct timespec ts0, ts1;

	clock_gettime(CLOCK_REALTIME, &ts0);
	int i;
	for(i=0; i<30; i++) {
		cnt = 0; wbuf = buf; rbuf = buf;
		while (cnt < FILESIZE) {
			payload = PAYLOAD < FILESIZE - cnt ? PAYLOAD : FILESIZE - cnt;
			read_all(rsock, payload);

			if(!last)
				write_all(wsock, payload);

			cnt += payload;
		}
		if(i%10 == 0)
			printf("%d\n", i);
	}
	clock_gettime(CLOCK_REALTIME, &ts1);
	printf("%ld\n",(ts1.tv_sec - ts0.tv_sec) );
	/**
	f = fopen("doc2", "wb");
        if(fwrite(buf, FILESIZE ,1,f) != 1) {
                fclose(f);
                error("write file");
        }
        fclose(f);
	**/
}

void startc(int rsock, int wsock)
{
	FILE *f;
	struct timespec ts0, ts1;
	int payload, cnt = 0;
	long total = 0;

	f = fopen("graphic.pdf", "rb");
	if(!f)
		error("graphic.pdf");
	if(fread(buf ,FILESIZE, 1, f) != 1) {
		fclose(f);
		error("read doc");
	}
	fclose(f);

	clock_gettime(CLOCK_REALTIME, &ts0);
	int i;
	for(i=0; i<30; i++) {
		cnt = 0;
		wbuf = buf;
		rbuf = buf;
		while (cnt < FILESIZE) {

			//memset(tmp, 0, size);
			payload = PAYLOAD < FILESIZE - cnt ? PAYLOAD : FILESIZE - cnt;
			write_all(wsock, payload);
			//read_all(rsock, payload);
			cnt += payload;
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
		total += cnt;
		if(i%5 == 0)
			printf("%d\n", i);
	}
	clock_gettime(CLOCK_REALTIME, &ts1);
	printf("%d %ld\n",(ts1.tv_sec - ts0.tv_sec), total );
}

int main(int argc, char *argv[]) {
        if(argc < 5) {
                fprintf(stderr,"usage %s <hostname> <-c/-s> <-p#> <-l#>\n", argv[0]);
                exit(0);
        }
	
        int readsock, writesock, listensock;
	struct hostent *server = gethostbyname(argv[1]);
        char role = *(argv[2]+1);
	int portno = atoi(argv[3]+2);
	int last = atoi(argv[4]+2);

        switch(role) {
        case 'c':		
                writesock = setupsender( server, portno );
                readsock = setuprecver( &listensock, portno );
		printf(RESULT_LINE);
		printf(RESULT_FMT);
		startc( readsock, writesock);
                break;
        case 's':
                readsock = setuprecver( &listensock, portno );
                writesock = setupsender( server, portno );
		starts( readsock, writesock, last);
                break;
        default:
                break;
        }

	close(readsock);
	close(writesock);
        return 0;
}

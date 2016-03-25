.PHONY: clean

CFLAGS  := -Wall -g -o
LDFLAGS := -lrdmacm -libverbs -lpthread
APPS    := server client

all: ${APPS}

clean:
	rm -f ${APPS}

server: server.c
	gcc ${CFLAGS} server server.c ${LDFLAGS}
client: client.c
	gcc ${CFLAGS} client client.c ${LDFLAGS}

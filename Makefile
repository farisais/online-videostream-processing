CC = gcc -std=c99
AVDIR = av
GOFILE = http/server.go
CFILE = ${AVDIR}/av.c ${AVDIR}/av.h

all: $(CFILE) $(GOFILE)
	$(CC) -c ${AVDIR}/av.c
	ar cru ${AVDIR}/libavtest.a ${AVDIR}/av.o
	go get -d
	go build http/server.go

clean:
	rm -rf *.o
	rm -rf *.a
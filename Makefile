CC = gcc -std=c99

all: av.c av.h server.go
	$(CC) -c av.c
	ar cru libavtest.a av.o
	go get -d
	go build http/server.go

clean:
	rm -rf *.o
	rm -rf *.a
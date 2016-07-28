CC = gcc -std=c99
AVDIR = av
CFILE = ${AVDIR}/av.c ${AVDIR}/av.h

all: 
	$(CC) -c -I ${AVDIR} ${AVDIR}/av.c
	ar cru http/libavlib.a av.o
	go build -o server http/*.go
	
clean:
	rm -rf *.o
	rm -rf *.a
	rm -rf http/*.a
	rm -rf http/*.o
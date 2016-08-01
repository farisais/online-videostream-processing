CC = gcc -std=c99
AVDIR = av
MEMDIR = mem
CFILE = ${AVDIR}/av.c ${AVDIR}/av.h

all: 
	$(CC) -c -I $(MEMDIR) ${MEMDIR}/fmemopen.c
	$(CC) -c -I ${AVDIR} ${MEMDIR} ${AVDIR}/av.c
	ar cru http/libavlib.a av.o fmemopen.o
	go build -o server http/*.go
	
install:
	$(shell ./download-dep.sh)

clean:
	rm -rf *.o
	rm -rf *.a
	rm -rf http/*.a
	rm -rf http/*.o
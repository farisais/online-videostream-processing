CC = gcc -std=c99
AVDIR = av
TEST = test
CFILE = ${AVDIR}/av.c ${AVDIR}/av.h

all: 
	$(CC) -c -I ${AVDIR} ${AVDIR}/av.c ${AVDIR}/fmemopen.c
	ar cru http/libavlib.a av.o fmemopen.o
	go build -o server http/*.go

test: ${TEST}/test_motion_decode.c ${AVDIR}/av.c ${AVDIR}/av.h
	$(CC) -I ${AVDIR} ${TEST}/test_motion_decode.c -o ${TEST}/test_motion_decode -Lhttp -lavlib -lavformat -lavcodec -lx264 -lavdevice -lavutil -lswscale -lswresample -lz -lm

install:
	$(shell ./download-dep.sh)

clean:
	rm -rf *.o
	rm -rf *.a
	rm -rf http/*.a
	rm -rf http/*.o
/*
 *  Motion decoder module testing
 *  Copyright (c) 2016 - Faris Rahman <farisais@hotmail.com> 
 *  
 *  Utilizing libavcodec, libavutil, libswscale which is
 *  part of the ffmpeg open source project 
 *  see http://ffmpeg.org for more information
 */
#include "av.h"

void avDecodeCallback(char* result, size_t size, char* secret){
	printf("got data with size %d\n", size);
}

int main(int argc, char* argv[]){
	char* codec = argv[1];
	init_decoder(codec, "1234");
	printf("Begin testing ...\n");
	/*
	 * Opening test file
	 */
	FILE* f = fopen("sample.mp4", "r+");
	if(f == NULL){
		printf("Couldn't find sample.mp4\n");
		return 1;
	}
	
	fseek(f, 0, SEEK_END);
	unsigned long flen = ftell(f);
	fseek(f, 0, SEEK_SET);

	char* buffer = (char*)malloc(flen * sizeof(char));
	int nbytes = fread(buffer, flen, 1, f);

	/*
	 * Testing decode video
	 */
	decode_video2(buffer, (size_t)flen);
	printf("Complete testing ...\n");
	return 0;
}
/*
 *  Video Decoder & Encoder
 *  Copyright (c) 2016 - Faris Rahman <farisais@hotmail.com> 
 *
 * 	Utilizing libavcodec, libavutil, libswscale which is
 * 	part of the ffmpeg open source project 
 * 	see http://ffmpeg.org for more information
 */

#ifndef MPEG_DECODER_H
#define MPEG_DECODER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_AV_CONFIG_H
#undef HAVE_AV_CONFIG_H
#endif

#include <libavcodec/avcodec.h>
#include <libavutil/mathematics.h>
#include <libswscale/swscale.h>

/*
 * Buffer to store video stream to be decoded
 */
#define INBUF_SIZE 409600

/*
 * Data structure to store decoding result
 */
typedef struct DecodeResult {
	AVFrame* picture;
	int width;
	int height;
	uint8_t* ppm_frame_buffer;
	size_t ppm_frame_size;
	int got_picture;
	int encoded;
} DecodeResult;

/*
 * Data structure to store encoding result
 */
typedef struct EncodeResult {
	size_t size;
	uint8_t* data;
	int encoded;
} EncodeResult;

void init_decoder(char* codec_name);
DecodeResult* decode_video(char* data, size_t size);
void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame);
void construct_ppm_frame(uint8_t* buf, AVFrame* pframe, int width, int height);
void construct_ppm_frame_gray(uint8_t* buf, AVFrame* pframe, int width, int height);
EncodeResult* encode_video(char* data, size_t size);
void init_encoder();
void free_decode_struct(DecodeResult* ptr);
void free_encode_struct(EncodeResult* ptr);
void gcollect();
#endif
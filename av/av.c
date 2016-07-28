/*
 *  Video Decoder & Encoder
 *  Copyright (c) 2016 - Faris Rahman <farisais@hotmail.com> 
 *  
 * 	Utilizing libavcodec, libavutil, libswscale which is
 * 	part of the ffmpeg open source project 
 * 	see http://ffmpeg.org for more information
 */

#include "av.h"

/*
 * Global variabel decoder
 * Need to be declared as global variable hence no need to reinit
 * when processing video stream data
 */

AVCodec *codec;
AVCodecContext *ctx;
uint8_t* inbuf;
char buf[1024];
AVPacket avpkt;
static int frame;
AVFrame *picture;
AVFrame *pictureRGB;
DecodeResult* decodeResultBuf;
struct SwsContext *sws_ctx = NULL;
uint8_t *picBuffer = NULL;
AVCodecParserContext* parserCtx;
int current_stream_size;


/* 
 * Global variabel encoder
 * Need to be declared as global variable hence no need to reinit
 * when processing video stream data
 */

AVCodec* encodec;
AVCodecContext* enctx = NULL;
AVFrame* enframeYUV;
AVFrame* tframe;
AVPacket enpkt;
uint8_t endcode[] = {0,0,1,0xb7};
uint8_t* enbuffer;
uint8_t* tbuffer;
static int64_t enframe_count;
EncodeResult* encodeResult;
struct SwsContext* sws_enctx = NULL;

/*
 * Function to initialise encoder codec context
 */
void init_encoder(){
	encodec = avcodec_find_encoder(AV_CODEC_ID_MPEG1VIDEO);
	if (!encodec) {
		fprintf(stderr, "Codec for encoder not found\n");
     exit(1);
	}

	enctx = avcodec_alloc_context3(encodec);
	if (!enctx){
		fprintf(stderr, "Could not allocate video codec context for encoder\n");
    exit(1);
	}

	// Encode using MPEG-1
	enctx->bit_rate = 400000;
	enctx->width = 640;
	enctx->height = 480;
	enctx->time_base = (AVRational){1,30};
	enctx->gop_size = 10;
	enctx->max_b_frames = 1;
	enctx->pix_fmt = AV_PIX_FMT_YUV420P;

	if(avcodec_open2(enctx, encodec, NULL) < 0){
		fprintf(stderr, "Could not open encoder codec\n");
    exit(1);
	}

	enframeYUV = av_frame_alloc();
	if(!enframeYUV){
		fprintf(stderr, "Could not allocate video frame YUV for encoder\n");
    exit(1);
	}

	enframeYUV->format = enctx->pix_fmt;
	enframeYUV->width = enctx->width;
	enframeYUV->height = enctx->height;
	int nbytes = avpicture_get_size(AV_PIX_FMT_RGB24, enctx->width, enctx->height);	
	tbuffer = (uint8_t*)av_malloc(sizeof(uint8_t) * nbytes);
	int numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, enctx->width, enctx->height);
	enbuffer = (uint8_t*)av_malloc(sizeof(uint8_t) * numBytes);
	avpicture_fill((AVPicture *)enframeYUV, enbuffer, AV_PIX_FMT_YUV420P,
									enctx->width, enctx->height);

	int ret;
	ret = av_image_alloc(enframeYUV->data, enframeYUV->linesize, enctx->width, enctx->height,
                        enctx->pix_fmt, 32);
  if (ret < 0) {
      fprintf(stderr, "Could not allocate raw picture YUV buffer\n");
      exit(1);
  }

  sws_enctx = sws_getContext(
  	640, 
		480,
		AV_PIX_FMT_RGB24,
		enctx->width,
		enctx->height,
		AV_PIX_FMT_YUV420P,
		SWS_BICUBIC,
		NULL,
		NULL,
		NULL
	);

	tframe = av_frame_alloc();
	if(!tframe){
		fprintf(stderr, "Could not allocate video tframe for encoder\n");
    exit(1);
	}

	tframe->format = AV_PIX_FMT_RGB24;
	tframe->width = enctx->width;
	tframe->height = enctx->height;

  encodeResult = (struct EncodeResult*)malloc(sizeof(struct EncodeResult));
  encodeResult->data = (uint8_t*)malloc(sizeof(uint8_t) * 409600);
  encodeResult->encoded = 0;
  enframe_count = 0;
}

/*
 * Function to initialise decoder codec context
 */
void init_decoder(char* codec_name){
	av_register_all();
	//memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);
	//memset(inbuf + INBUF_SIZE, 0, FF_INPUT_BUFFER_PADDING_SIZE);
	printf("Init video decoder\n");
	if(strcmp(codec_name, "h264") == 0){
			codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	} 
	else if(strcmp(codec_name, "mpeg1video") == 0){
		codec = avcodec_find_decoder(AV_CODEC_ID_MPEG1VIDEO);
	}
	
	if(!codec){
		fprintf(stderr, "Codec not found\n");
		exit(1);
	}

	ctx = avcodec_alloc_context3(codec);
	picture = av_frame_alloc();
	pictureRGB = av_frame_alloc();

	if(picture == NULL && pictureRGB == NULL){
		fprintf(stderr, "Couldn't allocate picture\n");
		exit(1);
	}

	if(codec->capabilities & CODEC_CAP_TRUNCATED)
		ctx->flags |= CODEC_FLAG_TRUNCATED;

	if(avcodec_open2(ctx, codec, NULL) < 0){
		fprintf(stderr, "Couldn't open codec\n");
		exit(1);
	}

	av_init_packet(&avpkt);
	avpkt.size = 0;
	avpkt.data = NULL;
	parserCtx = av_parser_init(ctx->codec_id);
	if (parserCtx == NULL) {
		fprintf(stderr, "Failed initializing parser\n");
		exit(1);
	}

	ctx->width = 640;
	ctx->height = 480;

	decodeResultBuf = (struct DecodeResult*)malloc(sizeof(struct DecodeResult));
	decodeResultBuf->got_picture = 0;
	decodeResultBuf->ppm_frame_size = 15 + (ctx->width * 3 * ctx->height);
	decodeResultBuf->ppm_frame_buffer = (uint8_t*)malloc(sizeof(uint8_t) * 
		decodeResultBuf->ppm_frame_size);

	decodeResultBuf->width = ctx->width;
	decodeResultBuf->height = ctx->height;

	sws_ctx = sws_getContext(ctx->width, 
		ctx->height,
		AV_PIX_FMT_YUV420P,
		ctx->width,
		ctx->height,
		AV_PIX_FMT_RGB24,
		SWS_BILINEAR,
		NULL,
		NULL,
		NULL
	);
	
	int numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, ctx->width, ctx->height);
	picBuffer = (uint8_t*)malloc(numBytes * sizeof(uint8_t));

	frame = 0;
	fprintf(stderr, "Success init decoder\n");
}

DecodeResult* decode_video(char* data, size_t size){
	int got_picture, len;
	decodeResultBuf->got_picture = 0;

	/*
	 * Allocate memory buffer to store the receiving frame
	 */
	inbuf = (uint8_t*)malloc((size +  AV_INPUT_BUFFER_PADDING_SIZE) * sizeof(uint8_t));

	/*
	 * Offset should be added to the buffer
	 * because some optimized bitstream readers read 32 or 64 bits at once 
	 * and could read over the end.
	 * 
	 * The end of the input buffer should be set to 0 to ensure that no 
	 * overreading happens for damaged MPEG streams.
	 */
	memset(inbuf + size, 0, AV_INPUT_BUFFER_PADDING_SIZE);

	memcpy(inbuf, data, size);
	avpkt.data = inbuf;
	avpkt.size = size;
	len = avcodec_decode_video2(ctx, picture, &got_picture, &avpkt);
	if(len < 0){
		fprintf(stderr, "Error while decoding frame %d\n", frame);
	}
	decodeResultBuf->picture = picture;
	if(got_picture){
		frame++;

		/*
		 * Convert the color space from YUV to RGB
		 */
		avpicture_fill((AVPicture*)pictureRGB, picBuffer, AV_PIX_FMT_RGB24,
									ctx->width, ctx->height);

		sws_scale(sws_ctx, (uint8_t const * const *)picture->data,
						picture->linesize, 0, ctx->height,
						pictureRGB->data, pictureRGB->linesize);

		construct_ppm_frame(decodeResultBuf->ppm_frame_buffer, pictureRGB, 
			ctx->width, ctx->height);
		
		decodeResultBuf->got_picture = 1;
	} 
	free(inbuf);
	return decodeResultBuf;
}

/*
 * Convert the frame to portable pixmap format to be passed
 * to python context to be processed
 */
void construct_ppm_frame(uint8_t* buf, AVFrame* pframe, int width, int height){
	char* header = "P6\n640 480\n255\n";
	memcpy(buf, header, 15);
	memcpy((buf+15), pframe->data[0], width * 3 * height);
}

void construct_ppm_frame_gray(uint8_t* buf, AVFrame* pframe, int width, int height){
	char* header = "P5\n640 480\n255\n";
	memcpy(buf, header, 15);
	memcpy((buf+15), pframe->data[0], width * height);
}

/*
 * Encode the processed video
 */
EncodeResult* encode_video(char* data, size_t size){
	av_init_packet(&enpkt);
	enpkt.data = NULL;
	enpkt.size = 0;
	encodeResult->encoded = 0;
	memcpy(tbuffer, (uint8_t*)(data + 15), size - 15);	
	avpicture_fill((AVPicture *)tframe, tbuffer, AV_PIX_FMT_RGB24,
									enctx->width, enctx->height);

	sws_scale(sws_enctx, (uint8_t const * const *)tframe->data,
						tframe->linesize, 0, enctx->height,
						enframeYUV->data, enframeYUV->linesize);
	enframeYUV->pts = enframe_count;
	int got_output, ret;
	ret = avcodec_encode_video2(enctx, &enpkt, enframeYUV, &got_output);
  if (ret < 0) {
      fprintf(stderr, "Error encoding frame\n");
      exit(1);
  }

  if (got_output) {
      encodeResult->size = enpkt.size;
      encodeResult->encoded = 1;
      memcpy(encodeResult->data, enpkt.data, enpkt.size);
      av_packet_unref(&enpkt);
  }
  enframe_count++;
  return encodeResult;
}

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame){
	FILE *pFile; 
	char szFilename[32];
	int y;

	sprintf(szFilename, "frame%d.ppm", iFrame);
	pFile = fopen(szFilename, "wb");
	if(pFile == NULL)
		return;

	fprintf(pFile, "P6\n%d %d\n255\n", width, height);

  for(y=0; y<height; y++)
    fwrite(pFrame->data[0] + y * pFrame->linesize[0], 1, width*3, pFile);

  fclose(pFile);
}

/*
 * Freeing all pointers
 */
void gcollect(){
	avcodec_close(ctx);
	av_free(ctx);
	av_frame_free(&picture);
	av_frame_free(&pictureRGB);
	free_decode_struct(decodeResultBuf);
	free(picBuffer);

	avcodec_close(enctx);
	av_frame_free(&enframeYUV);
	av_frame_free(&tframe);
	free(enbuffer);
	free_encode_struct(encodeResult);
}

/*
 * Freeing decode result sruct
 */
void free_decode_struct(DecodeResult* ptr){
	av_frame_free(&ptr->picture);
	free(ptr->ppm_frame_buffer);
	free(ptr);
}

/*
 * Freeing encode result struct
 */
void free_encode_struct(EncodeResult* ptr){
	free(ptr->data);
	free(ptr);
}

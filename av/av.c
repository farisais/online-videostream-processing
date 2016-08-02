/*
 *  Video Decoder & Encoder
 *  Copyright (c) 2016 - Faris Rahman <farisais@hotmail.com> 
 *  
 *  Utilizing libavcodec, libavutil, libswscale which is
 *  part of the ffmpeg open source project 
 *  see http://ffmpeg.org for more information
 */

#include "av.h"
#include "fmemopen.h"

/*
 * Global variabel decoder
 * Need to be declared as global variable hence no need to reinit
 * when processing video stream data
 */
 #define OUTWIDTH 640
 #define OUTHEIGHT 480

AVCodec *codec;
AVCodecContext* ctx;

AVCodec* dcodec = NULL;
AVIOContext* IOCtx;
AVCodecContext* dctx = NULL;
bool isDCodecInit;
AVFormatContext* pFormatCtx;
uint8_t* dbuffer = NULL;
struct SwsContext* dsws_ctx = NULL;
uint8_t* IOBuffer = NULL;

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
 * Secret key as user process identifier
 */
char* secret;

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
	enctx->width = OUTWIDTH;
	enctx->height = OUTHEIGHT;
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
  	OUTWIDTH, 
		OUTHEIGHT,
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
void init_decoder(char* codec_name, char* gosecret){
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

	pFormatCtx = avformat_alloc_context();

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

	ctx->width = OUTWIDTH;
	ctx->height = OUTHEIGHT;

	decodeResultBuf = (struct DecodeResult*)malloc(sizeof(struct DecodeResult));
	decodeResultBuf->got_picture = 0;
	decodeResultBuf->ppm_frame_size = 15 + (ctx->width * 3 * ctx->height);
	decodeResultBuf->ppm_frame_buffer = (uint8_t*)malloc(sizeof(uint8_t) * 
		decodeResultBuf->ppm_frame_size);

	decodeResultBuf->width = ctx->width;
	decodeResultBuf->height = ctx->height;

	/*
   * Deprecated if using new decode function
   */
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

	isDCodecInit = false;
	secret = gosecret;

	frame = 0;
	fprintf(stderr, "Success init decoder\n");
}

/*
 * Function to read bytes from stream pointer to buffer. 
 * Ressembling fread() in C std lib
 */
static inline int readFunction(void* opaque, uint8_t* buf, int buf_size){
	FILE* f = (FILE*)opaque;

	int numbytes = fread(buf, buf_size, 1, f);

	printf("READ : [ ");
	for(int j=0;j<100;j++){
		printf("%hhu ", buf[j]);
	}
	printf("]\n");
	/*
	 * fread returning num of data successfully read with size of buf_size
	 * hence we need to multiply with buf size as the return value for the 
	 * avio_alloc_context to fill the buffer with
	 */
	return numbytes * buf_size;
}

/*
 * New function to compensate multiple frame in one stream data
 * Will be using callback to communicate with Go.
 */
void decode_video2(char* data, size_t size){
	/*
	 * Cast binary data buffer to FILE data pointer
	 */
	FILE* f = fmemopen((const void*)data, size * sizeof(char), "r+");

	fseek(f, 0, SEEK_END);
	unsigned long len = ftell(f);
	fseek(f, 0, SEEK_SET);
	printf("file length : %d\n", len);

	printf("alloc io buffer\n");
	IOBuffer = (uint8_t*)av_malloc((len + FF_INPUT_BUFFER_PADDING_SIZE) * sizeof(uint8_t));
	printf("finish alloc io buffer\n");
	//printf("io buffer @ %p\n", *IOBuffer);
	IOCtx = avio_alloc_context(IOBuffer, len, 0, (void*)f, &readFunction, NULL, NULL);

	pFormatCtx->pb = IOCtx;
	printf("open input  video frame ...\n");
	if (avformat_open_input(&pFormatCtx, "tmp", NULL, NULL) != 0){
		printf("Error when opening input format ...\n");
		return;
	}
	printf("open input  video frame ...\n");
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0){
		printf("Couldn't find stream info ...\n");
		return;
	}
	printf("finding video frame ...\n");
	int i = 0;
	/*
	 * Finding the Video frame within the stream data
	 */
	printf("finding video frame ...\n");
	int videoStream = -1;
	for(i=0;i<pFormatCtx->nb_streams;i++){
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	}
	printf("video frame : %d\n", videoStream);
	/*
	 * If video frame is found, then decode
	 */
	if (videoStream > -1){
		/*
		 * Need to initialize codec context, codec, buffer, and sws_context for 
		 * filling the frame if hasn't been initialized 
		 */
		if(!isDCodecInit){
			dctx = pFormatCtx->streams[videoStream]->codec;
			dcodec = avcodec_find_decoder(dctx->codec_id);
			if(dcodec != NULL){
				if(avcodec_open2(dctx, dcodec, NULL) >= 0){
					int numbytes = avpicture_get_size(AV_PIX_FMT_RGB24, OUTWIDTH, 
																OUTHEIGHT);
					dbuffer = (uint8_t*)av_malloc(numbytes * sizeof(uint8_t));
					/*
					 * Initialize sws context, we need to scale it to 640x480
					 */
					dsws_ctx = sws_getContext(
						dctx->width, 
						dctx->height,
						dctx->pix_fmt,
						OUTWIDTH,
						OUTHEIGHT,
						AV_PIX_FMT_RGB24,
						SWS_BILINEAR,
						NULL,
						NULL,
						NULL
					);

					isDCodecInit = true;
				}
			}
		}

		/*
		 * Recheck if the context and codec is initialized
		 */
		printf("Recheck ...\n");
		if(dctx != NULL && dcodec != NULL){
			/*
			 * Begin decoding
			 */
			AVPacket dpacket;
			avpicture_fill((AVPicture *)pictureRGB, dbuffer, AV_PIX_FMT_RGB24,
									OUTWIDTH, OUTHEIGHT);
			int i = 0;
			int got_picture;
			while(av_read_frame(pFormatCtx, &dpacket) >= 0){
				avcodec_decode_video2(dctx, picture, &got_picture, &dpacket);
				if(got_picture){
					printf("got pic ...\n");
					sws_scale(dsws_ctx, (uint8_t const * const *)picture->data,
									picture->linesize, 0, dctx->height,
									pictureRGB->data, pictureRGB->linesize);
					construct_ppm_frame(decodeResultBuf->ppm_frame_buffer, pictureRGB, 
						ctx->width, ctx->height);

					/* 
					 * Sending back to Go
					 */
					avDecodeCallback((char*)decodeResultBuf->ppm_frame_buffer, 
						decodeResultBuf->ppm_frame_size,secret);
				} else {
					printf("got no pic ...\n");
				}
				printf("freeing packet ...\n");
				av_free_packet(&dpacket);
				printf("complete freeing packet ...\n");
			}

		} else {
			printf("IO Context is not initialized\n");
		}
	}

	/*
	 * Clean the buffer and IO Context to be used for the next 
	 * data stream
	 */
	printf("freeing iobuffer ...\n");
	av_free(IOBuffer);
	printf("complete freeing iobuffer ...\n");
	printf("freeing ioctx ...\n");
	av_free(IOCtx->buffer);
	printf("complete freeing ioctx ...\n");
	printf("freeing file ...\n");
	fclose(f);
	printf("complete freeing file ...\n");
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

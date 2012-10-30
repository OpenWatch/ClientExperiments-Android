/*
 * Copyright 2011 - Churn Labs, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * This is mostly based off of the FFMPEG tutorial:
 * http://dranger.com/ffmpeg/
 * With a few updates to support Android output mechanisms and to update
 * places where the APIs have shifted.
 */

#include <jni.h>
#include <string.h>
#include <stdio.h>
#include <android/log.h>

#include <math.h>

#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>


#define  LOG_TAG    "FFMPEGSample"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

/* This file is adapted from the 'decoding_encoding' example
 * bundled with ffmpeg in /doc/examples/
 */

AVCodec *pCodec;
AVCodecContext *pCodecCtx;
int frameNumber, ret, got_output;
FILE *pFile;
AVFrame *pFrame;
AVPacket pkt;


void Java_net_openwatch_openwatch2_video_ffmpegTest_testFFMPEG(JNIEnv * env, jobject this, jstring filename){

	AVCodec *codec;
	AVCodecContext *c= NULL;
	int i, out_size, x, y, outbuf_size;
	FILE *f;
	AVFrame *picture;
	uint8_t *outbuf;
	int had_output=0;

	//printf("Encode video file %s\n", filename);

	LOGI("1. method called!");

	int codec_id = CODEC_ID_MPEG1VIDEO;

	LOGI("2. codec set");

	codec = avcodec_find_encoder(codec_id);
	LOGI("2a. avcodec_alloc_context3(codec)");
	if (!codec) {
		LOGI("2b. codec not found");
		//fprintf(stderr, "codec not found\n");
		exit(1);
	}

	c = avcodec_alloc_context3(codec);
	LOGI("3. avcodec_alloc_context3(codec)");
	picture= avcodec_alloc_frame();
	LOGI("4. avcodec_alloc_frame()");

	c->bit_rate = 400000;
	c->width = 352;
	c->height = 288;
	c->time_base= (AVRational){1,25};
	c->gop_size = 10;
	c->max_b_frames=1;
	c->pix_fmt = PIX_FMT_YUV420P;

	LOGI("5. set codec context");

	if(codec_id == CODEC_ID_H264)
		av_opt_set(c->priv_data, "preset", "slow", 0);

	LOGI("6.av_opt_set(c->priv_data ...");

	if (avcodec_open2(c, codec, NULL) < 0) {
		//fprintf(stderr, "could not open codec\n");
		exit(1);
	}

	f = fopen(filename, "wb");
	if (!f) {
		//fprintf(stderr, "could not open %s\n", filename);
		exit(1);
	}

	outbuf_size = 100000 + 12*c->width*c->height;
	outbuf = malloc(outbuf_size);


	av_image_alloc(picture->data, picture->linesize,
				   c->width, c->height, c->pix_fmt, 1);

	for(i=0;i<25;i++) {
		//fflush(stdout);

		for(y=0;y<c->height;y++) {
			for(x=0;x<c->width;x++) {
				picture->data[0][y * picture->linesize[0] + x] = x + y + i * 3;
			}
		}

		for(y=0;y<c->height/2;y++) {
			for(x=0;x<c->width/2;x++) {
				picture->data[1][y * picture->linesize[1] + x] = 128 + y + i * 2;
				picture->data[2][y * picture->linesize[2] + x] = 64 + x + i * 5;
			}
		}

		out_size = avcodec_encode_video(c, outbuf, outbuf_size, picture);
		had_output |= out_size;
		//printf("encoding frame %3d (size=%5d)\n", i, out_size);
		fwrite(outbuf, 1, out_size, f);
	}

	for(; out_size || !had_output; i++) {
		//fflush(stdout);

		out_size = avcodec_encode_video(c, outbuf, outbuf_size, NULL);
		had_output |= out_size;
		//printf("write frame %3d (size=%5d)\n", i, out_size);
		fwrite(outbuf, 1, out_size, f);
	}

	outbuf[0] = 0x00;
	outbuf[1] = 0x00;
	outbuf[2] = 0x01;
	outbuf[3] = 0xb7;
	fwrite(outbuf, 1, 4, f);
	fclose(f);
	free(outbuf);

	avcodec_close(c);
	av_free(c);
	av_free(picture->data[0]);
	av_free(picture);
	//printf("\n");

}

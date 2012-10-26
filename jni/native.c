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

#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/audioconvert.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
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


void Java_net_openwatch_openwatch2_VideoSoftwareRecorder_initializeOutputFile(JNIEnv * env, jobject this, jstring filename) {
	// expects String filename

	printf("Encode video file %s\n", filename);

	int codec_id = AV_CODEC_ID_H264;

	/* find the target video encoder */
	pCodec = avcodec_find_encoder(codec_id);
	if (!pCodec) {
		fprintf(stderr, "Codec not found\n");
		exit(1);
	}

	pCodecCtx = avcodec_alloc_context3(pCodec);

	/* put sample parameters */
	pCodecCtx->bit_rate = 400000;
	/* resolution must be a multiple of two */
	pCodecCtx->width = 352;
	pCodecCtx->height = 288;
	/* frames per second */
	pCodecCtx->time_base= (AVRational){1,25};
	pCodecCtx->gop_size = 10; /* emit one intra frame every ten frames */
	pCodecCtx->max_b_frames=1;
	pCodecCtx->pix_fmt = PIX_FMT_YUV420P;

	if(codec_id == AV_CODEC_ID_H264)
		av_opt_set(pCodecCtx->priv_data, "preset", "slow", 0);

	/* open it */
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		fprintf(stderr, "Could not open codec\n");
		exit(1);
	}

	pFile = fopen(filename, "wb");
	if (!pFile) {
		fprintf(stderr, "Could not open %s\n", filename);
		exit(1);
	}

	pFrame = avcodec_alloc_frame();
	if (!pFrame) {
		fprintf(stderr, "Could not allocate video frame\n");
		exit(1);
	}
	pFrame->format = pCodecCtx->pix_fmt;
	pFrame->width  = pCodecCtx->width;
	pFrame->height = pCodecCtx->height;

	/* the image can be allocated by any means and av_image_alloc() is
	 * just the most convenient way if av_malloc() is to be used */
	ret = av_image_alloc(pFrame->data, pFrame->linesize, pCodecCtx->width, pCodecCtx->height,
			pCodecCtx->pix_fmt, 32);
	if (ret < 0) {
		fprintf(stderr, "Could not allocate raw picture buffer\n");
		exit(1);
	}

}

void Java_net_openwatch_openwatch2_VideoSoftwareRecorder_encodeFrame(JNIEnv * env, jobject this) {
	/* encode 1 frame of video */

	av_init_packet(&pkt);
	pkt.data = NULL;    // packet data will be allocated by the encoder
	pkt.size = 0;

	fflush(stdout);

	int x, y;

	/* Y */
	for (y = 0; y < pCodecCtx->height; y++) {
		for (x = 0; x < pCodecCtx->width; x++) {
			pFrame->data[0][y * pFrame->linesize[0] + x] = x + y + frameNumber * 3;
		}
	}

	/* Cb and Cr */
	for (y = 0; y < pCodecCtx->height / 2; y++) {
		for (x = 0; x < pCodecCtx->width / 2; x++) {
			pFrame->data[1][y * pFrame->linesize[1] + x] = 128 + y + frameNumber * 2;
			pFrame->data[2][y * pFrame->linesize[2] + x] = 64 + x + frameNumber * 5;
		}
	}

	pFrame->pts = frameNumber;

	/* encode the image */
	ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_output);
	if (ret < 0) {
		// fprintf(stderr, "Error encoding frame\n");
		LOGE("Error encoding frame");
		exit(1);
	}

	if (got_output) {
		printf("Write frame %3d (size=%5d)\n", frameNumber, pkt.size);
		fwrite(pkt.data, 1, pkt.size, pFile);
		av_free_packet(&pkt);
	}

}

void Java_net_openwatch_openwatch2_VideoSoftwareRecorder_finalizeFile(JNIEnv * env, jobject this) {

	uint8_t endcode[] = { 0, 0, 1, 0xb7 };

	/* get the delayed frames */
	// what's going on here?
	for (got_output = 1; got_output; frameNumber++) {
		fflush(stdout);

		ret = avcodec_encode_video2(pCodecCtx, &pkt, NULL, &got_output);
		if (ret < 0) {
			fprintf(stderr, "Error encoding frame\n");
			exit(1);
		}

		if (got_output) {
			printf("Write frame %3d (size=%5d)\n", frameNumber, pkt.size);
			fwrite(pkt.data, 1, pkt.size, pFile);
			av_free_packet(&pkt);
		}
	}

	/* add sequence end code to have a real mpeg file */
	fwrite(endcode, 1, sizeof(endcode), pFile);
	fclose(pFile);

	avcodec_close(pCodecCtx);
	av_free(pCodecCtx);
	av_freep(&pFrame->data[0]);
	avcodec_free_frame(&pFrame);
}


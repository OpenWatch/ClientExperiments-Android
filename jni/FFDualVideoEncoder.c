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

#define LOG_TAG "FFVideoEncoder"
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

AVCodec *codec;
AVCodecContext *c_hq, *c_lq= NULL;
AVFrame *frame_hq, *frame_lq;
AVPacket pkt;
int i, out_size_hq, out_size_lq, x, y, outbuf_size_hq, outbuf_size_lq;
FILE *f_hq, *f_lq;
uint8_t *outbuf_hq, *outbuf_lq;
int had_output_hq=0, had_output_lq=0;

void Java_net_openwatch_openwatch2_video_FFVideoEncoder_initializeEncoder(JNIEnv * env, jobject this, jstring filepath, jint width, jint height){

	// Convert Java types
	const jbyte *native_filename;
	native_filepath = (*env)->GetStringUTFChars(env, filename, NULL);

	const char native_filename_hq[] = native_filepath + "_HQ.mpg";
	const char native_filename_lq[] = native_filepath + "_LQ.mpg";

	int native_height_hq = (int) height;
	int native_width_hq = (int) width;

	int native_height_lq = native_height_hq / 2;
	int native_width_lq = native_width_hq / 2;

	avcodec_register_all();

	int codec_id = CODEC_ID_MPEG1VIDEO;

	LOGI("Encode video files to %s", native_filepath);
	/* find the mpeg1 video encoder */
	codec = avcodec_find_encoder(codec_id);
	if (!codec) {
		//fprintf(stderr, "codec not found\n");
		LOGI("codec not found");
		exit(1);
	}

	LOGI("alloc AVFrame and AVCodecContext");

	sws_ctx = sws_getContext(native_width_hq, native_height_hq, PIX_FMT_YUV420P, native_width_lq, native_height_lq, PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);

	if (!sws_ctx) {
		fprintf(stderr,
				"Impossible to create scale context for the conversion "
				"fmt:%s s:%dx%d -> fmt:%s s:%dx%d\n",
				av_get_pix_fmt_name(PIX_FMT_YUV420P), native_width_hq, inputSize.height_hq,
				av_get_pix_fmt_name(PIX_FMT_YUV420P), native_width_lq, outputSize.height_lq);
		ret = AVERROR(EINVAL);
		exit(1);
	}

	// HQ
	frame_hq= avcodec_alloc_frame();
	c_hq = avcodec_alloc_context3(codec);
	/* put sample parameters */
	c_hq->bit_rate = 400000;
	/* resolution must be a multiple of two */
	c_hq->width = native_width_hq;
	c_hq->height = native_height_hq;
	/* frames per second */
	c_hq->time_base= (AVRational){1,25};
	c_hq->gop_size = 10; /* emit one intra frame every ten frames */
	c_hq->max_b_frames=1;
	c_hq->pix_fmt = PIX_FMT_YUV420P;

	// LQ
	frame_lq= avcodec_alloc_frame();
	c_lq = avcodec_alloc_context3(codec);
	/* put sample parameters */
	c_lq->bit_rate = 200000;
	/* resolution must be a multiple of two */
	c_lq->width = native_width_lq;
	c_lq->height = native_height_lq;
	/* frames per second */
	c_lq->time_base= (AVRational){1,25};
	c_lq->gop_size = 10; /* emit one intra frame every ten frames */
	c_lq->max_b_frames=1;
	c_lq->pix_fmt = PIX_FMT_YUV420P;


	LOGI("AVCodecContext initialize");

	//if(codec_id == CODEC_ID_H264)
		//av_opt_set(c->priv_data, "preset", "slow", 0);

	/* open it */
	if (avcodec_open2(c_hq, codec, NULL) < 0 || avcodec_open2(c_lq, codec, NULL) < 0) {
		//fprintf(stderr, "could not open codec\n");
		LOGE("could not open codec");
		exit(1);
	}

	LOGI("open avcodec");

	f_hq = fopen(native_filename_hq, "wb");
	f_lq = fopen(native_filename_lq, "wb");
	if (!f_hq || !f_lq) {
		//fprintf(stderr, "could not open %s\n", filename);
		LOGE("could not open %s or %s", native_filename_hq, native_filename_lq);
		exit(1);
	}

	LOGI("open file");

	/* alloc image and output buffer */
	outbuf_size_hq = 100000 + 12*c_hq->width*c_hq->height;
	outbuf_hq = malloc(outbuf_size_hq);

	outbuf_size_lq = 100000 + 12*c_lq->width*c_lq->height;
		outbuf_lq = malloc(outbuf_size_lq);

	LOGI("malloc outbuf");

	/* the image can be allocated by any means and av_image_alloc() is
	 * just the most convenient way if av_malloc() is to be used */
	av_image_alloc(frame_hq->data, frame_hq->linesize,
				   c_hq->width, c_hq->height, c_hq->pix_fmt, 1);
	av_image_alloc(frame_lq->data, frame_lq->linesize,
					   c_lq->width, c_lq->height, c_lq->pix_fmt, 1);

	LOGI("malloc av_image");

}

void Java_net_openwatch_openwatch2_video_FFVideoEncoder_encodeFrame(JNIEnv * env, jobject this, jbyteArray frame_data){
	LOGI("Encode frame");
	// Convert Java types
	int frame_data_length = (*env)->GetArrayLength(env, frame_data);
	jboolean is_copy;
	jbyte *native_frame_data = (*env)->GetByteArrayElements(env, frame_data, &is_copy);

	LOGI("Get native frame: is_copy: %d", is_copy);

		// Encode HQ frame
		fflush(stdout);
		/* prepare a dummy image */
		/* Y */
		for(y=0;y<c->height;y++) {
			for(x=0;x<c->width;x++) {
				frame_hq->data[0][y * frame_hq->linesize[0] + x] = native_frame_data[0];
				native_frame_data++;
			}
		}

		/* Cb and Cr */
		for(y=0;y<c->height/2;y++) {
			for(x=0;x<c->width/2;x++) {
				frame_hq->data[2][y * frame_hq->linesize[2] + x] = native_frame_data[0];
				frame_hq->data[1][y * frame_hq->linesize[1] + x] = native_frame_data[1];
				native_frame_data+=2;
			}
		}

		// TODO: Resize and encode LQ frame

		/* convert to destination format */
		    sws_scale(sws_ctx, (const uint8_t * const*)frame->data,
		              frame->linesize, 0, inputSize.height, scaledFrame->data, scaledFrame->linesize);

		/* encode the image */
		out_size_hq = avcodec_encode_video(c_hq, outbuf_hq, outbuf_size_hq, frame_hq);
		had_output_hq |= out_size_hq;

		out_size_lq = avcodec_encode_video(c_hq, outbuf_hq, outbuf_size_hq, frame_hq);
		had_output_lq |= out_size_lq;

		//printf("encoding frame %3d (size=%5d)\n", i, out_size);
		fwrite(outbuf_hq, 1, out_size_hq, f_hq);
		fwrite(outbuf_lq, 1, out_size_lq, f_lq);

		(*env)->ReleaseByteArrayElements(env, frame_data, native_frame_data, 0);
}

void Java_net_openwatch_openwatch2_video_FFVideoEncoder_finalizeEncoder(JNIEnv * env, jobject this){
	LOGI("finalize encoder");

	/* get the delayed frames */
	for(; out_size_hq || !had_output_hq; i++) {
		//fflush(stdout);

		out_size_hq = avcodec_encode_video(c_hq, outbuf_hq, outbuf_size_hq, NULL);
		had_output_hq |= out_size_hq;
		//printf("write frame %3d (size=%5d)\n", i, out_size);
		LOGI("write frame %3d (size=%5d)", i, out_size);
		fwrite(outbuf_hq, 1, out_size_hq, f_hq);
	}

	/* add sequence end code to have a real mpeg file */
	outbuf[0] = 0x00;
	outbuf[1] = 0x00;
	outbuf[2] = 0x01;
	outbuf[3] = 0xb7;

	// HQ
	fwrite(outbuf_hq, 1, 4, f_hq);
	fclose(f_hq);
	free(outbuf_hq);

	avcodec_close(c_hq);
	av_free(c_hq);
	av_free(frame_hq->data[0]);
	av_free(frame_hq);

	// LQ
	fwrite(outbuf_lq, 1, 4, f_lq);
	fclose(f_lq);
	free(outbuf_lq);

	avcodec_close(c_lq);
	av_free(c_lq);
	av_free(frame_lq->data[0]);
	av_free(frame_lq);
	//printf("\n");

	// free scaling context
	sws_freeContext(sws_ctx);

}


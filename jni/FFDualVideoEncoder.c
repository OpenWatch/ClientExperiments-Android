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
#include <libswscale/swscale.h>

#define LOG_TAG "FFDualVideoEncoder"
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

AVCodec *codec;
AVCodecContext *c_hq, *c_lq= NULL;
AVFrame *frame_hq, *frame_lq;
AVPacket pkt;
int i, out_size_hq, out_size_lq, x, y, outbuf_size_hq, outbuf_size_lq, ret;
FILE *f_hq, *f_lq;
uint8_t *outbuf_hq, *outbuf_lq;
int had_output_hq=0, had_output_lq=0, native_height_hq, native_width_hq;

struct SwsContext *sws_ctx;

void Java_net_openwatch_openwatch2_video_FFDualVideoEncoder_initializeEncoder(JNIEnv * env, jobject this, jstring file_name_hq, jstring file_name_lq, jint width, jint height){

	// Convert Java types
	const jbyte *native_filename_hq;
	const jbyte *native_filename_lq;

	native_filename_hq = (*env)->GetStringUTFChars(env, file_name_hq, NULL);
	native_filename_lq = (*env)->GetStringUTFChars(env, file_name_lq, NULL);

	native_height_hq = (int) height;
	native_width_hq = (int) width;

	int native_height_lq = native_height_hq / 2;
	int native_width_lq = native_width_hq / 2;

	avcodec_init();
	avcodec_register_all();

	int codec_id = CODEC_ID_MPEG1VIDEO;
	//int codec_id = CODEC_ID_MPEG4;

	LOGI("Encode video files %s , %s ", native_filename_hq, native_filename_lq);
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
		LOGE("Impossible to create scale context for the conversion fmt:%s s:%dx%d -> fmt:%s s:%dx%d\n",
				av_get_pix_fmt_name(PIX_FMT_YUV420P), native_width_hq, native_height_hq,
				av_get_pix_fmt_name(PIX_FMT_YUV420P), native_width_lq, native_height_lq);
		ret = AVERROR(EINVAL);
		exit(1);
	}

	// HQ
	frame_hq = avcodec_alloc_frame();
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
	/*
	if(codec_id == CODEC_ID_H264)
		        av_opt_set(c_hq->priv_data, "preset", "slow", 0);

	in .8.1 we've got to set av_opt_set_dict
	*/
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
	/*
	if(codec_id == CODEC_ID_H264)
			        av_opt_set(c_lq->priv_data, "preset", "slow", 0);
	*/

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
	ret = av_image_alloc(frame_hq->data, frame_hq->linesize,
				   c_hq->width, c_hq->height, c_hq->pix_fmt, 1);
	 if (ret < 0) {
	        LOGE("Could not allocate raw picture buffer\n");
	        exit(1);
	}
	ret = av_image_alloc(frame_lq->data, frame_lq->linesize,
					   c_lq->width, c_lq->height, c_lq->pix_fmt, 1);
	 if (ret < 0) {
	        LOGE("Could not allocate scaled picture buffer");
	        exit(1);
	}

	LOGI("malloc av_image");

}

void Java_net_openwatch_openwatch2_video_FFDualVideoEncoder_encodeFrame(JNIEnv * env, jobject this, jbyteArray frame_data){
	LOGI("Encode frame");
	// Convert Java types
	int frame_data_length = (*env)->GetArrayLength(env, frame_data);
	jboolean is_copy;
	jbyte *native_frame_data = (*env)->GetByteArrayElements(env, frame_data, &is_copy);

	LOGI("Get native frame: is_copy: %d", is_copy);

		// Encode HQ frame
		//fflush(stdout);
		/* prepare a dummy image */
		/* Y */
		for(y=0;y<c_hq->height;y++) {
			for(x=0;x<c_hq->width;x++) {
				frame_hq->data[0][y * frame_hq->linesize[0] + x] = native_frame_data[0];
				native_frame_data++;
			}
		}

		/* Cb and Cr */
		for(y=0;y<c_hq->height/2;y++) {
			for(x=0;x<c_hq->width/2;x++) {
				frame_hq->data[2][y * frame_hq->linesize[2] + x] = native_frame_data[0];
				frame_hq->data[1][y * frame_hq->linesize[1] + x] = native_frame_data[1];
				native_frame_data+=2;
			}
		}

		/* scale to destination format */

		sws_scale(sws_ctx, 									// the scaling context previously created with sws_getContext()
				(const uint8_t * const*)frame_hq->data,		// the array containing the pointers to the planes of the source slice
				  frame_hq->linesize, 						// the array containing the strides for each plane of the source image
				  0, 										// the position in the source image of the slice to process, that is the number (counted starting from zero) in the image of the first row of the slice
				  native_height_hq, 						// the height of the source slice, that is the number of rows in the slice
				  frame_lq->data, 							// the array containing the pointers to the planes of the destination image
				  frame_lq->linesize);						// the array containing the strides for each plane of the destination image

		/* encode hq image */
		out_size_hq = avcodec_encode_video(c_hq, outbuf_hq, outbuf_size_hq, frame_hq);
		had_output_hq |= out_size_hq;

		/* encode lq image */
		out_size_lq = avcodec_encode_video(c_lq, outbuf_lq, outbuf_size_lq, frame_lq);
		had_output_lq |= out_size_lq;

		fwrite(outbuf_hq, 1, out_size_hq, f_hq);
		fwrite(outbuf_lq, 1, out_size_lq, f_lq);

		(*env)->ReleaseByteArrayElements(env, frame_data, native_frame_data, 0);
}

void Java_net_openwatch_openwatch2_video_FFDualVideoEncoder_finalizeEncoder(JNIEnv * env, jobject this){
	LOGI("finalize encoder");

	/* get the delayed frames */
	for(; out_size_hq || !had_output_hq; i++) {
		//fflush(stdout);

		out_size_hq = avcodec_encode_video(c_hq, outbuf_hq, outbuf_size_hq, NULL);
		had_output_hq |= out_size_hq;
		//printf("write frame %3d (size=%5d)\n", i, out_size);
		LOGI("write hq frame %3d (size=%5d)", i, out_size_hq);
		fwrite(outbuf_hq, 1, out_size_hq, f_hq);
	}

	for(; out_size_lq || !had_output_lq; i++) {
			//fflush(stdout);

			out_size_lq = avcodec_encode_video(c_lq, outbuf_lq, outbuf_size_lq, NULL);
			had_output_lq |= out_size_lq;
			//printf("write frame %3d (size=%5d)\n", i, out_size);
			LOGI("write lq frame %3d (size=%5d)", i, out_size_lq);
			fwrite(outbuf_lq, 1, out_size_lq, f_lq);
	}
	// the out_size_lq and out_size_hq should be equal in size so
	// we probably don't need two separate loops.

	// HQ
	/* add sequence end code to have a real mpeg file */
	outbuf_hq[0] = 0x00;
	outbuf_hq[1] = 0x00;
	outbuf_hq[2] = 0x01;
	outbuf_hq[3] = 0xb7;
	fwrite(outbuf_hq, 1, 4, f_hq);
	fclose(f_hq);
	free(outbuf_hq);

	avcodec_close(c_hq);
	av_free(c_hq);
	av_free(frame_hq->data[0]);
	av_free(frame_hq);

	// LQ
	/* add sequence end code to have a real mpeg file */
	outbuf_lq[0] = 0x00;
	outbuf_lq[1] = 0x00;
	outbuf_lq[2] = 0x01;
	outbuf_lq[3] = 0xb7;
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



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

#define LOG_TAG "FFChunkedVideoEncoder"
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

AVCodec *codec;
AVCodecContext *c= NULL;
AVFrame *frame;
AVPacket pkt;
int i, out_size, x, y, outbuf_size;
FILE *f1, *f2; // f1 is the file used for writing, f2 is a buffer file ready for rapid swapping
uint8_t *outbuf;
int had_output=0;

const jbyte *native_output_file1;
const jbyte *native_output_file2;

AVCodecContext* initializeAVCodecContext(AVCodecContext *c);

void Java_net_openwatch_openwatch2_video_FFChunkedVideoEncoder_internalInitializeEncoder(JNIEnv * env, jobject this, jstring filename1, jstring filename2, jint width, jint height){

	// Convert Java types
	const jbyte *native_filename1, *native_filename2;
	native_filename1 = (*env)->GetStringUTFChars(env, filename1, NULL);
	native_filename2 = (*env)->GetStringUTFChars(env, filename2, NULL);

	int native_height = (int) height;
	int native_width = (int) width;

	avcodec_register_all();

	int codec_id = CODEC_ID_MPEG1VIDEO;

	LOGI("Encode video file %s", native_filename1);
	/* find the mpeg1 video encoder */
	codec = avcodec_find_encoder(codec_id);
	if (!codec) {
		//fprintf(stderr, "codec not found\n");
		LOGI("codec not found");
		exit(1);
	}

	c = avcodec_alloc_context3(codec);
	frame= avcodec_alloc_frame();

	LOGI("alloc AVFrame and AVCodecContext");

	/* put sample parameters */
	c->bit_rate = 400000;
	/* resolution must be a multiple of two */
	c->width = native_width;
	c->height = native_height;
	/* frames per second */
	c->time_base= (AVRational){1,25};
	c->gop_size = 10; /* emit one intra frame every ten frames */
	c->max_b_frames=1;
	c->pix_fmt = PIX_FMT_YUV420P;

	LOGI("AVCodecContext initialize");

	//if(codec_id == CODEC_ID_H264)
		//av_opt_set(c->priv_data, "preset", "slow", 0);

	/* open it */
	if (avcodec_open2(c, codec, NULL) < 0) {
		//fprintf(stderr, "could not open codec\n");
		LOGE("could not open codec");
		exit(1);
	}

	LOGI("open avcodec");

	native_output_file1 = native_filename1;
	native_output_file2 = native_filename2;

	f1 = fopen(native_filename1, "wb");
	if (!f1) {
		//fprintf(stderr, "could not open %s\n", filename);
		LOGE("could not open %s", native_filename1);
		exit(1);
	}
	f2 = fopen(native_filename2, "wb");
	if (!f2) {
		LOGE("could not open %s", native_filename2);
		exit(1);
	}

	LOGI("open file");

	/* alloc image and output buffer */
	outbuf_size = 100000 + 12*c->width*c->height;
	outbuf = malloc(outbuf_size);

	LOGI("malloc outbuf");

	/* the image can be allocated by any means and av_image_alloc() is
	 * just the most convenient way if av_malloc() is to be used */
	av_image_alloc(frame->data, frame->linesize,
				   c->width, c->height, c->pix_fmt, 1);

	LOGI("malloc av_image");

}

void Java_net_openwatch_openwatch2_video_FFChunkedVideoEncoder_shiftEncoders(JNIEnv * env, jobject this, jstring new_filename){
	// Point the hot file to the buffer file
	// Point the new buffer at the given new_filename
	// Must be called after finalizeEncoder();

	c = initializeAVCodecContext(c);
	if(c == NULL){
		LOGE("error initializing AVCC");
		exit(1);
	}

	f1 = f2;
	native_output_file1 = native_output_file2;
	LOGI("ShiftEncoders");
	const jbyte *native_new_filename;
		native_new_filename = (*env)->GetStringUTFChars(env, new_filename, NULL);
	native_output_file2 = native_new_filename;
	f2 = fopen(native_new_filename, "wb");
	if (!f2) {
		LOGE("could not open %s", native_new_filename);
		exit(1);
	}
}

void Java_net_openwatch_openwatch2_video_FFChunkedVideoEncoder_encodeFrame(JNIEnv * env, jobject this, jbyteArray frame_data, jint encoder){
	//LOGI("Encode frame");
	// Convert Java types
	int frame_data_length = (*env)->GetArrayLength(env, frame_data);
	jboolean is_copy;
	jbyte *native_frame_data = (*env)->GetByteArrayElements(env, frame_data, &is_copy);

	//LOGI("Get native frame: is_copy: %d", is_copy);

	/* encode 1 second of video */
		//fflush(stdout);
		/* Y */
		for(y=0;y<c->height;y++) {
			for(x=0;x<c->width;x++) {
				frame->data[0][y * frame->linesize[0] + x] = native_frame_data[0];
				native_frame_data++;
			}
		}

		/* Cb and Cr */
		for(y=0;y<c->height/2;y++) {
			for(x=0;x<c->width/2;x++) {
				frame->data[2][y * frame->linesize[2] + x] = native_frame_data[0];
				frame->data[1][y * frame->linesize[1] + x] = native_frame_data[1];
				native_frame_data+=2;
			}
		}

		/* encode the image */
		out_size = avcodec_encode_video(c, outbuf, outbuf_size, frame);
		had_output |= out_size;
		//printf("encoding frame %3d (size=%5d)\n", i, out_size);
		fwrite(outbuf, 1, out_size, f1);

		(*env)->ReleaseByteArrayElements(env, frame_data, native_frame_data, 0);
}

void Java_net_openwatch_openwatch2_video_FFChunkedVideoEncoder_finalizeEncoder(JNIEnv * env, jobject this, jint is_final){
	LOGI("finalize file %s", native_output_file1);

	int native_is_final = (int) is_final;

	/* get the delayed frames */
	for(; out_size || !had_output; i++) {
		//fflush(stdout);

		out_size = avcodec_encode_video(c, outbuf, outbuf_size, NULL);
		had_output |= out_size;
		//printf("write frame %3d (size=%5d)\n", i, out_size);
		LOGI("write delayed frame %3d (size=%5d)", i, out_size);
		fwrite(outbuf, 1, out_size, f1);
	}

	/* add sequence end code to have a real mpeg file */
	outbuf[0] = 0x00;
	outbuf[1] = 0x00;
	outbuf[2] = 0x01;
	outbuf[3] = 0xb7;
	fwrite(outbuf, 1, 4, f1);
	fclose(f1);

	if(native_is_final != 0){
		free(outbuf);
		avcodec_close(c);
		av_free(c);
		av_free(frame->data[0]);
		av_free(frame);

		unlink(native_output_file2); // remove unused buffer file
	}

}

AVCodecContext* initializeAVCodecContext(AVCodecContext *c){

	AVCodecContext *new_c;
	new_c = avcodec_alloc_context3(codec);
	//frame = avcodec_alloc_frame();

	LOGI("alloc AVFrame and AVCodecContext");

	/* put sample parameters */
	new_c->bit_rate = 400000;
	/* resolution must be a multiple of two */
	new_c->width = c->width;
	new_c->height = c->height;
	/* frames per second */
	new_c->time_base= (AVRational){1,25};
	new_c->gop_size = 10; /* emit one intra frame every ten frames */
	new_c->max_b_frames=1;
	new_c->pix_fmt = PIX_FMT_YUV420P;

	if (avcodec_open2(new_c, codec, NULL) < 0) {
			//fprintf(stderr, "could not open codec\n");
			LOGE("could not open codec");
			exit(1);
		}

	return new_c;

}

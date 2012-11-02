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

#define LOG_TAG "FFAudioEncoder"
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

AVCodec *codec;
AVCodecContext *c= NULL;
int frame_size, i, j, out_size, outbuf_size;
FILE *f;
short *samples;
//short *samples_buffer;
uint8_t *outbuf;

jint Java_net_openwatch_openwatch2_audio_FFAudioEncoder_initializeEncoder(JNIEnv * env, jobject this, jstring filename, jint width, jint height){

	// Convert Java types
	const jbyte *native_filename;
	native_filename = (*env)->GetStringUTFChars(env, filename, NULL);

	avcodec_register_all();

	codec = avcodec_find_encoder(CODEC_ID_MP2);
	//codec = avcodec_find_encoder(CODEC_ID_AAC);
	if (!codec) {
		//fprintf(stderr, "codec not found\n");
		LOGI("codec not found");
		exit(1);
	}

	c = avcodec_alloc_context3(codec);

	/* put sample parameters */
	c->bit_rate = 64000;
	c->sample_rate = 44100;
	c->channels = 1;
	c->sample_fmt = AV_SAMPLE_FMT_S16;

	/* open it */
	if (avcodec_open2(c, codec, NULL) < 0) {
		//fprintf(stderr, "could not open codec\n");
		LOGI("could not open codec");
		exit(1);
	}

	/* the codec gives us the frame size, in samples */
	frame_size = c->frame_size;
	LOGI("Frame size: %d samples", c->frame_size);
	//int num_samples = frame_size * 2 * c->channels;
	// buffer size as short[]
	//int buffer_size = av_samples_get_buffer_size(NULL, c->channels, c->frame_size,
	//                                             c->sample_fmt, 0) / 2;

	samples = malloc(frame_size * 2 * c->channels);

	int buffer_size = av_get_bytes_per_sample(c->sample_fmt) * frame_size * 2 * c->channels;
	outbuf_size = 10000;
	outbuf = malloc(outbuf_size);

	f = fopen(native_filename, "wb");
	if (!f) {
		//fprintf(stderr, "could not open %s\n", filename);
		LOGI("could not open %s", native_filename);
		exit(1);
	}


	return frame_size;
}

void Java_net_openwatch_openwatch2_audio_FFAudioEncoder_encodeFrame(JNIEnv * env, jobject this, jshortArray frame_data){
	LOGI("Encode frame");
	// Convert Java types
	int frame_data_length = (*env)->GetArrayLength(env, frame_data);
	jboolean is_copy;
	jshort *native_frame_data = (*env)->GetShortArrayElements(env, frame_data, &is_copy);

	LOGI("Get native frame: is_copy: %d", is_copy);

	for(j=0;j<frame_size;j++) {
		samples[j] = (int)(native_frame_data[0]);
		native_frame_data++;
	}

	/* encode the samples */
	out_size = avcodec_encode_audio(c, outbuf, outbuf_size, samples);
	fwrite(outbuf, 1, out_size, f);

	(*env)->ReleaseShortArrayElements(env, frame_data, native_frame_data, 0);
}

void Java_net_openwatch_openwatch2_audio_FFAudioEncoder_finalizeEncoder(JNIEnv * env, jobject this){
	LOGI("finalize encoder");
	fclose(f);
	free(outbuf);
	free(samples);

	avcodec_close(c);
	av_free(c);
}

void Java_net_openwatch_openwatch2_audio_FFAudioEncoder_testFFMPEG(JNIEnv * env, jclass clazz, jstring filename){

	AVCodec *codec;
	AVCodecContext *c= NULL;
	int frame_size, i, j, out_size, outbuf_size;
	FILE *f;
	short *samples;
	float t, tincr;
	uint8_t *outbuf;

	// Convert Java types
	const jbyte *native_filename;
	native_filename = (*env)->GetStringUTFChars(env, filename, NULL);

	avcodec_register_all();

	//printf("Encode audio file %s\n", filename);
	LOGI("Encode audio file %s",native_filename);

	/* find the MP2 encoder */
	codec = avcodec_find_encoder(CODEC_ID_MP2);
	if (!codec) {
		//fprintf(stderr, "codec not found\n");
		LOGI("codec not found");
		exit(1);
	}

	c = avcodec_alloc_context3(codec);

	/* put sample parameters */
	c->bit_rate = 64000;
	c->sample_rate = 44100;
	c->channels = 2;
	c->sample_fmt = AV_SAMPLE_FMT_S16;

	/* open it */
	if (avcodec_open2(c, codec, NULL) < 0) {
		//fprintf(stderr, "could not open codec\n");
		LOGI("could not open codec");
		exit(1);
	}

	/* the codec gives us the frame size, in samples */
	frame_size = c->frame_size;
	samples = malloc(frame_size * 2 * c->channels);
	outbuf_size = 10000;
	outbuf = malloc(outbuf_size);

	f = fopen(native_filename, "wb");
	if (!f) {
		//fprintf(stderr, "could not open %s\n", filename);
		LOGI("could not open %s", native_filename);
		exit(1);
	}

	/* encode a single tone sound */
	t = 0;
	tincr = 2 * M_PI * 440.0 / c->sample_rate;
	for(i=0;i<200;i++) {
		for(j=0;j<frame_size;j++) {
			samples[2*j] = (int)(sin(t) * 10000);
			samples[2*j+1] = samples[2*j];
			t += tincr;
		}
		/* encode the samples */
		out_size = avcodec_encode_audio(c, outbuf, outbuf_size, samples);
		fwrite(outbuf, 1, out_size, f);
	}
	fclose(f);
	free(outbuf);
	free(samples);

	avcodec_close(c);
	av_free(c);
	LOGI("test audio encode complete");
}

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

// Muxing
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

#define LOG_TAG "FFChunkedAudioVideoEncoder"
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Video Encoding

AVCodec *codec;
AVCodecContext *c= NULL;
AVFrame *frame;
AVPacket pkt;
int i, out_size, x, y, outbuf_size;
FILE *f1, *f2; // f1 is the file used for writing, f2 is a buffer file ready for rapid swapping
uint8_t *outbuf;
int had_output=0;

// Muxing

AVOutputFormat *fmt;
AVFormatContext *oc;
AVStream *audio_st, *video_st;
double audio_pts, video_pts;

const jbyte *native_output_file1;
const jbyte *native_output_file2;

int output_width;
int output_height;

AVCodecContext* initializeAVCodecContext(AVCodecContext *c);

/***********************************
 *   AUXILLARY MUXING METHODS      *
 *  from ffmpeg's muxing-example.c *
 ***********************************/
#undef exit

/* 5 seconds stream duration */
#define STREAM_DURATION   5.0
#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_NB_FRAMES  ((int)(STREAM_DURATION * STREAM_FRAME_RATE))
#define STREAM_PIX_FMT PIX_FMT_YUV420P /* default pix_fmt */

static int sws_flags = SWS_BICUBIC;

/**************************************************************/
/* audio output */

float t, tincr, tincr2;
int16_t *samples;
uint8_t *audio_outbuf;
int audio_outbuf_size;
int audio_input_frame_size;

/*
 * add an audio output stream
 */
static AVStream *add_audio_stream(AVFormatContext *oc, enum CodecID codec_id)
{
    AVCodecContext *c;
    AVStream *st;

    st = av_new_stream(oc, 1);
    if (!st) {
        fprintf(stderr, "Could not alloc stream\n");
        exit(1);
    }

    c = st->codec;
    c->codec_id = codec_id;
    c->codec_type = AVMEDIA_TYPE_AUDIO;

    /* put sample parameters */
    c->sample_fmt = AV_SAMPLE_FMT_S16;
    c->bit_rate = 64000;
    c->sample_rate = 44100;
    c->channels = 1;

    // some formats want stream headers to be separate
    if(oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;

    return st;
}

static void open_audio(AVFormatContext *oc, AVStream *st)
{
    AVCodecContext *c;
    AVCodec *codec;

    c = st->codec;

    /* find the audio encoder */
    codec = avcodec_find_encoder(c->codec_id);
    if (!codec) {
        fprintf(stderr, "codec not found\n");
        exit(1);
    }

    /* open it */
    if (avcodec_open(c, codec) < 0) {
        fprintf(stderr, "could not open codec\n");
        exit(1);
    }

    /* init signal generator */
    t = 0;
    tincr = 2 * M_PI * 110.0 / c->sample_rate;
    /* increment frequency by 110 Hz per second */
    tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;

    audio_outbuf_size = 10000;
    audio_outbuf = av_malloc(audio_outbuf_size);

    /* ugly hack for PCM codecs (will be removed ASAP with new PCM
       support to compute the input frame size in samples */
    if (c->frame_size <= 1) {
        audio_input_frame_size = audio_outbuf_size / c->channels;
        switch(st->codec->codec_id) {
        case CODEC_ID_PCM_S16LE:
        case CODEC_ID_PCM_S16BE:
        case CODEC_ID_PCM_U16LE:
        case CODEC_ID_PCM_U16BE:
            audio_input_frame_size >>= 1;
            break;
        default:
            break;
        }
    } else {
        audio_input_frame_size = c->frame_size;
    }
    samples = av_malloc(audio_input_frame_size * 2 * c->channels);
}

/* prepare a 16 bit dummy audio frame of 'frame_size' samples and
   'nb_channels' channels */
static void get_audio_frame(int16_t *samples, int frame_size, int nb_channels)
{
    int j, i, v;
    int16_t *q;

    q = samples;
    for(j=0;j<frame_size;j++) {
        v = (int)(sin(t) * 10000);
        for(i = 0; i < nb_channels; i++)
            *q++ = v;
        t += tincr;
        tincr += tincr2;
    }
}

static void write_audio_frame(AVFormatContext *oc, AVStream *st)
{
    AVCodecContext *c;
    AVPacket pkt;
    av_init_packet(&pkt);

    c = st->codec;

    // we'll populate samples in encodeFrame(...)
    //get_audio_frame(samples, audio_input_frame_size, c->channels);

    pkt.size= avcodec_encode_audio(c, audio_outbuf, audio_outbuf_size, samples);

    if (c->coded_frame && c->coded_frame->pts != AV_NOPTS_VALUE)
        pkt.pts= av_rescale_q(c->coded_frame->pts, c->time_base, st->time_base);
    pkt.flags |= AV_PKT_FLAG_KEY;
    pkt.stream_index= st->index;
    pkt.data= audio_outbuf;

    /* write the compressed frame in the media file */
    if (av_interleaved_write_frame(oc, &pkt) != 0) {
        fprintf(stderr, "Error while writing audio frame\n");
        exit(1);
    }
}

static void close_audio(AVFormatContext *oc, AVStream *st)
{
    avcodec_close(st->codec);

    av_free(samples);
    av_free(audio_outbuf);
}

/**************************************************************/
/* video output */

AVFrame *picture, *tmp_picture;
uint8_t *video_outbuf;
int frame_count, video_outbuf_size;

/* add a video output stream */
static AVStream *add_video_stream(AVFormatContext *oc, enum CodecID codec_id)
{
    AVCodecContext *c;
    AVStream *st;

    st = av_new_stream(oc, 0);
    if (!st) {
        fprintf(stderr, "Could not alloc stream\n");
        exit(1);
    }

    c = st->codec;
    c->codec_id = codec_id;
    c->codec_type = AVMEDIA_TYPE_VIDEO;

    /* put sample parameters */
    c->bit_rate = 400000;
    /* resolution must be a multiple of two */
    c->width = 320;
    c->height = 240;
    /* time base: this is the fundamental unit of time (in seconds) in terms
       of which frame timestamps are represented. for fixed-fps content,
       timebase should be 1/framerate and timestamp increments should be
       identically 1. */
    c->time_base.den = STREAM_FRAME_RATE;
    c->time_base.num = 1;
    c->gop_size = 12; /* emit one intra frame every twelve frames at most */
    c->pix_fmt = STREAM_PIX_FMT;
    if (c->codec_id == CODEC_ID_MPEG2VIDEO) {
        /* just for testing, we also add B frames */
        c->max_b_frames = 2;
    }
    if (c->codec_id == CODEC_ID_MPEG1VIDEO){
        /* Needed to avoid using macroblocks in which some coeffs overflow.
           This does not happen with normal video, it just happens here as
           the motion of the chroma plane does not match the luma plane. */
        c->mb_decision=2;
    }
    // some formats want stream headers to be separate
    if(oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;

    return st;
}

static AVFrame *alloc_picture(enum PixelFormat pix_fmt, int width, int height)
{
    AVFrame *picture;
    uint8_t *picture_buf;
    int size;

    picture = avcodec_alloc_frame();
    if (!picture)
        return NULL;
    size = avpicture_get_size(pix_fmt, width, height);
    picture_buf = av_malloc(size);
    if (!picture_buf) {
        av_free(picture);
        return NULL;
    }
    avpicture_fill((AVPicture *)picture, picture_buf,
                   pix_fmt, width, height);
    return picture;
}

static void open_video(AVFormatContext *oc, AVStream *st)
{
    AVCodec *codec;
    AVCodecContext *c;

    c = st->codec;

    /* find the video encoder */
    codec = avcodec_find_encoder(c->codec_id);
    if (!codec) {
        fprintf(stderr, "codec not found\n");
        exit(1);
    }

    /* open the codec */
    if (avcodec_open(c, codec) < 0) {
        fprintf(stderr, "could not open codec\n");
        exit(1);
    }

    video_outbuf = NULL;
    if (!(oc->oformat->flags & AVFMT_RAWPICTURE)) {
        /* allocate output buffer */
        /* XXX: API change will be done */
        /* buffers passed into lav* can be allocated any way you prefer,
           as long as they're aligned enough for the architecture, and
           they're freed appropriately (such as using av_free for buffers
           allocated with av_malloc) */
        video_outbuf_size = 200000;
        video_outbuf = av_malloc(video_outbuf_size);
    }

    /* allocate the encoded raw picture */
    picture = alloc_picture(c->pix_fmt, c->width, c->height);
    if (!picture) {
        fprintf(stderr, "Could not allocate picture\n");
        exit(1);
    }

    /* if the output format is not YUV420P, then a temporary YUV420P
       picture is needed too. It is then converted to the required
       output format */
    tmp_picture = NULL;
    if (c->pix_fmt != PIX_FMT_YUV420P) {
        tmp_picture = alloc_picture(PIX_FMT_YUV420P, c->width, c->height);
        if (!tmp_picture) {
            fprintf(stderr, "Could not allocate temporary picture\n");
            exit(1);
        }
    }
}

/* prepare a dummy image */
static void fill_yuv_image(AVFrame *pict, int frame_index, int width, int height)
{
    int x, y, i;

    i = frame_index;

    /* Y */
    for(y=0;y<height;y++) {
        for(x=0;x<width;x++) {
            pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;
        }
    }

    /* Cb and Cr */
    for(y=0;y<height/2;y++) {
        for(x=0;x<width/2;x++) {
            pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
            pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
        }
    }
}

static void write_video_frame(AVFormatContext *oc, AVStream *st)
{
    int out_size, ret;
    AVCodecContext *c;
    static struct SwsContext *img_convert_ctx;

    c = st->codec;

    if (frame_count >= STREAM_NB_FRAMES) {
        /* no more frame to compress. The codec has a latency of a few
           frames if using B frames, so we get the last frames by
           passing the same picture again */
    } else {
    	/* DO NOT NEED. WE ALWAYS ROLL W/ YUV420P

        if (c->pix_fmt != PIX_FMT_YUV420P) {
            // as we only generate a YUV420P picture, we must convert it
            //   to the codec pixel format if needed
            if (img_convert_ctx == NULL) {
                img_convert_ctx = sws_getContext(c->width, c->height,
                                                 PIX_FMT_YUV420P,
                                                 c->width, c->height,
                                                 c->pix_fmt,
                                                 sws_flags, NULL, NULL, NULL);
                if (img_convert_ctx == NULL) {
                    fprintf(stderr, "Cannot initialize the conversion context\n");
                    exit(1);
                }
            }
            fill_yuv_image(tmp_picture, frame_count, c->width, c->height);
            sws_scale(img_convert_ctx, tmp_picture->data, tmp_picture->linesize,
                      0, c->height, picture->data, picture->linesize);
        } else {
            fill_yuv_image(picture, frame_count, c->width, c->height);
        }
        */
    }


    if (oc->oformat->flags & AVFMT_RAWPICTURE) {
        /* raw video case. The API will change slightly in the near
           futur for that */
        AVPacket pkt;
        av_init_packet(&pkt);

        pkt.flags |= AV_PKT_FLAG_KEY;
        pkt.stream_index= st->index;
        pkt.data= (uint8_t *)picture;
        pkt.size= sizeof(AVPicture);

        ret = av_interleaved_write_frame(oc, &pkt);
    } else {
    	//LOGI("Encoding image");
        /* encode the image */
        out_size = avcodec_encode_video(c, video_outbuf, video_outbuf_size, picture);
        /* if zero size, it means the image was buffered */
        if (out_size > 0) {
            AVPacket pkt;
            av_init_packet(&pkt);

            if (c->coded_frame->pts != AV_NOPTS_VALUE)
                pkt.pts= av_rescale_q(c->coded_frame->pts, c->time_base, st->time_base);
            if(c->coded_frame->key_frame)
                pkt.flags |= AV_PKT_FLAG_KEY;
            pkt.stream_index= st->index;
            pkt.data= video_outbuf;
            pkt.size= out_size;
            //LOGI("Prepared to write interleaved frame");
            /* write the compressed frame in the media file */
            ret = av_interleaved_write_frame(oc, &pkt);
            //LOGI("Wrote interleaved frame");
        } else {
            ret = 0;
        }
    }
    if (ret != 0) {
        fprintf(stderr, "Error while writing video frame\n");
        exit(1);
    }
    frame_count++;
}

static void close_video(AVFormatContext *oc, AVStream *st)
{
    avcodec_close(st->codec);
    av_free(picture->data[0]);
    av_free(picture);
    if (tmp_picture) {
        av_free(tmp_picture->data[0]);
        av_free(tmp_picture);
    }
    av_free(video_outbuf);
}

/***********************************
 *            END OF
 *   AUXILLARY MUXING METHODS      *
 *  from ffmpeg's muxing-example.c *
 ***********************************/

void Java_net_openwatch_openwatch2_recorder_FFChunkedAudioVideoEncoder_internalInitializeEncoder(JNIEnv * env, jobject this, jstring filename1, jstring filename2, jint width, jint height){

	// Convert Java types
	//const jbyte *native_filename1, *native_filename2;
	native_output_file1 = (*env)->GetStringUTFChars(env, filename1, NULL);
	native_output_file2 = (*env)->GetStringUTFChars(env, filename2, NULL);

	output_height = (int) height;
	output_width = (int) width;

	LOGI("1. internalInitializeEncoder");

	initializeAVFormatContext();

}

void Java_net_openwatch_openwatch2_recorder_FFChunkedAudioVideoEncoder_shiftEncoders(JNIEnv * env, jobject this, jstring new_filename){
	// Point the hot file to the buffer file
	// Point the new buffer at the given new_filename
	// Must be called after finalizeEncoder();
	/* FROM OLD CHUNKED VIDEO CODE
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
	*/

	finalizeAVFormatContext();

	const jbyte *native_new_filename;
	native_new_filename = (*env)->GetStringUTFChars(env, new_filename, NULL);

	// Shift output file
	native_output_file1 = native_output_file2;
	native_output_file2 = native_new_filename;

	initializeAVFormatContext();

}

void Java_net_openwatch_openwatch2_recorder_FFChunkedAudioVideoEncoder_encodeFrame(JNIEnv * env, jobject this, jbyteArray video_frame_data, jshortArray audio_frame_data){
	//LOGI("Encode frame");
	// Convert Java types
	// video
	jbyte *native_video_frame_data = (*env)->GetByteArrayElements(env, video_frame_data, NULL);
	// audio
	jshort *native_audio_frame_data = (*env)->GetShortArrayElements(env, audio_frame_data, NULL);

	// write video_frame_data to AVFrame
	if(video_st){
		c = video_st->codec; // don't need to do this each frame?

		for(y=0;y<c->height;y++) {
			for(x=0;x<c->width;x++) {
				picture->data[0][y * picture->linesize[0] + x] = native_video_frame_data[0];
				native_video_frame_data++;
			}
		}

		/* Cb and Cr */
		for(y=0;y<c->height/2;y++) {
			for(x=0;x<c->width/2;x++) {
				picture->data[2][y * picture->linesize[2] + x] = native_video_frame_data[0];
				picture->data[1][y * picture->linesize[1] + x] = native_video_frame_data[1];
				native_video_frame_data+=2;
			}
		}
	}
	// write audio_frame_data to another AVFrame
	if(audio_st){
		for(y=0;y<audio_input_frame_size;y++){
			samples[y] = (int)(native_audio_frame_data[0]);
			native_audio_frame_data++;
		}

	}

	//LOGI("Format frame data");

	/* compute current audio and video time */
	if (audio_st)
		audio_pts = (double)audio_st->pts.val * audio_st->time_base.num / audio_st->time_base.den;
	else
		audio_pts = 0.0;

	if (video_st)
		video_pts = (double)video_st->pts.val * video_st->time_base.num / video_st->time_base.den;
	else
		video_pts = 0.0;
	/*
	if ((!audio_st || audio_pts >= STREAM_DURATION) &&
		(!video_st || video_pts >= STREAM_DURATION))
		break;
 	*/

	if (!audio_st && !video_st){
		LOGE("No audio OR video stream :(");
			return;
	}

	/* write interleaved audio and video frames */
	if (!video_st || (video_st && audio_st && audio_pts < video_pts)) {
		write_audio_frame(oc, audio_st);
		//LOGI("wrote audio frame");
	} else {
		write_video_frame(oc, video_st);
		//LOGI("wrote video frame");
	}

	//LOGI("Get native frame: is_copy: %d", is_copy);

	(*env)->ReleaseByteArrayElements(env, video_frame_data, native_video_frame_data, 0);
}

void Java_net_openwatch_openwatch2_recorder_FFChunkedAudioVideoEncoder_finalizeEncoder(JNIEnv * env, jobject this, jint is_final){
	LOGI("finalize file %s", native_output_file1);

	int native_is_final = (int) is_final;

	/* No longer needed
	// get the delayed frames
	for(; out_size || !had_output; i++) {
		//fflush(stdout);

		out_size = avcodec_encode_video(c, outbuf, outbuf_size, NULL);
		had_output |= out_size;
		//printf("write frame %3d (size=%5d)\n", i, out_size);
		LOGI("write delayed frame %3d (size=%5d)", i, out_size);
		fwrite(outbuf, 1, out_size, f1);
	}

	// add sequence end code to have a real mpeg file
	outbuf[0] = 0x00;
	outbuf[1] = 0x00;
	outbuf[2] = 0x01;
	outbuf[3] = 0xb7;
	fwrite(outbuf, 1, 4, f1);
	 */
	//fclose(f1);
	//fclose(f2);

	// BEGIN MUXING CODE
	/* write the trailer, if any.  the trailer must be written
	 * before you close the CodecContexts open when you wrote the
	 * header; otherwise write_trailer may try to use memory that
	 * was freed on av_codec_close() */


	    if(native_is_final != 0){ // if is final finalize
			/* Don't think is needed anymore
			free(outbuf);
			avcodec_close(c);
			av_free(c);
			av_free(frame->data[0]);
			av_free(frame);
			*/
			unlink(native_output_file2); // remove unused buffer file
	    }
	    finalizeAVFormatContext();

	    return 0;
}

void finalizeAVFormatContext(){

	av_write_trailer(oc);

	// close each codec
	if (video_st)
		close_video(oc, video_st);
	if (audio_st)
		close_audio(oc, audio_st);

	// free the streams
	for(i = 0; i < oc->nb_streams; i++) {
		av_freep(&oc->streams[i]->codec);
		av_freep(&oc->streams[i]);
	}

	if (!(fmt->flags & AVFMT_NOFILE)) {
		// close the output file
		avio_close(oc->pb);
	}

	// free the stream
	av_free(oc);

}

// Method to be called after beginning new chunk
// to initialize AVFormatContext with new chunk filename
void initializeAVFormatContext(){

	/*
	// test file opening
	FILE *test_file = fopen(native_output_file1, "wb");
	if (!test_file) {
		//fprintf(stderr, "could not open %s\n", filename);
		LOGE("could not open %s", native_output_file1);
		exit(1);
	}
	LOGI("successfully opened file.");
	fclose(test_file);


	// TEST file for avformat_alloc_output
	const char test_path[] = "/sdcard/ffmpeg_testing/1223.mpeg";
	native_output_file1 = test_path;
	// END TEST
	*/

	av_register_all();

	LOGI("2. initializingAVFormatContext with file: %s", native_output_file1);

	/* allocate the output media context */
	avformat_alloc_output_context2(&oc, NULL, NULL, ((const char*) native_output_file1));
	if (!oc) {
		//printf("Could not deduce output format from file extension: using MPEG.\n");
		LOGI("Could not deduce output format from file extension: using MPEG.");
		avformat_alloc_output_context2(&oc, NULL, "mpeg", ((const char*)native_output_file1));
	}
	if (!oc) {
		LOGE("a.0.e Could not init AVFormatContext");
		exit(1);
	}
	fmt= oc->oformat;

	LOGI("3. initializeAVFormatContext");

	/* add the audio and video streams using the default format codecs
	   and initialize the codecs */
	video_st = NULL;
	audio_st = NULL;
	if (fmt->video_codec != CODEC_ID_NONE) {
		video_st = add_video_stream(oc, fmt->video_codec);
	}
	if (fmt->audio_codec != CODEC_ID_NONE) {
		audio_st = add_audio_stream(oc, fmt->audio_codec);
	}

	LOGI("4. set AVFormat codecs");

	av_dump_format(oc, 0, native_output_file1, 1);

	/* now that all the parameters are set, we can open the audio and
	   video codecs and allocate the necessary encode buffers */
	if (video_st)
		open_video(oc, video_st);
	if (audio_st)
		open_audio(oc, audio_st);

	/* open the output file, if needed */
	if (!(fmt->flags & AVFMT_NOFILE)) {
		if (avio_open(&oc->pb, native_output_file1, AVIO_FLAG_WRITE) < 0) {
			//fprintf(stderr, "Could not open '%s'\n", native_filename1);
			LOGE("Could not open '%s'", native_output_file1);
			exit(1);
		}else{
			LOGI("4a. output file opened successfully");
		}
	}


	/* write the stream header, if any */
	av_write_header(oc);
	LOGI("5. write file header");
}

// From ChunkedVideo demo
/*
AVCodecContext* initializeAVCodecContext(AVCodecContext *c){

	AVCodecContext *new_c;
	new_c = avcodec_alloc_context3(codec);
	//frame = avcodec_alloc_frame();

	LOGI("alloc AVFrame and AVCodecContext");

	// put sample parameters
	new_c->bit_rate = 400000;
	// resolution must be a multiple of two
	new_c->width = c->width;
	new_c->height = c->height;
	// frames per second
	new_c->time_base= (AVRational){1,25};
	new_c->gop_size = 10; // emit one intra frame every ten frames
	new_c->max_b_frames=1;
	new_c->pix_fmt = PIX_FMT_YUV420P;

	if (avcodec_open2(new_c, codec, NULL) < 0) {
			//fprintf(stderr, "could not open codec\n");
			LOGE("could not open codec");
			exit(1);
		}

	return new_c;

}
*/

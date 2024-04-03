#include "MediaDecoder.h"

#include "ImageResizer.h"
#include "Internal.h"
#include "SoundResampler.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <memory.h>

#define DISABLE_HARDWARE_ACCELERATION 1

typedef struct
{
	MediaDecoderContext ctx;
	AVFormatContext* format;
	AVCodecContext* codecVideo;
	AVCodecContext* codecAudio;
	AVPacket* packet;
	AVFrame* frame;
#ifndef DISABLE_HARDWARE_ACCELERATION
	AVFrame* frame2;
#endif

	int (*funcDecodeFrame)(MediaDecoderContext* ctx);

	struct ImageResizerContext* resizer;
	struct SoundResamplerContext* resampler;

	// playback info
	int didPlaybackStart;
	double startTime;
	double lastTime;
	int loopCount;
	int isImage;
} InternalContext;

static enum AVPixelFormat hw_pix_fmt;
static int hw_decoder_init(AVCodecContext* ctx, const enum AVHWDeviceType type)
{
	int err = 0;
	AVBufferRef* hw_device_ctx = NULL;
	if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type, NULL, NULL, 0)) < 0)
	{
		fprintf(stderr, "Failed to create specified HW device.\n");
		return err;
	}
	ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
	return err;
}
static enum AVPixelFormat get_hw_format(AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts)
{
	const enum AVPixelFormat* p;
	for (p = pix_fmts; *p != AV_PIX_FMT_NONE; p++)
	{
		if (*p == hw_pix_fmt)
			return *p;
	}
	fprintf(stderr, "Failed to get HW surface format.\n");
	return AV_PIX_FMT_NONE;
}

static float GetAudioSample(const AVCodecContext* codecCtx, uint8_t* buffer, int sampleIndex)
{
	int64_t val = 0;
	float ret = 0;
	int sampleSize = av_get_bytes_per_sample(codecCtx->sample_fmt);
	switch (sampleSize)
	{
	case 1:
		// 8bit samples are always unsigned
		val = ((uint8_t*)buffer)[sampleIndex];
		// make signed
		val -= 127;
		break;

	case 2:
		val = ((uint16_t*)buffer)[sampleIndex];
		break;

	case 4:
		val = ((uint32_t*)buffer)[sampleIndex];
		break;

	case 8:
		val = ((uint64_t*)buffer)[sampleIndex];
		break;

	default:
		fprintf(stderr, "Invalid sample size %d.\n", sampleSize);
		return 0;
	}

	// Check which data type is in the sample.
	switch (codecCtx->sample_fmt)
	{
	case AV_SAMPLE_FMT_U8:
	case AV_SAMPLE_FMT_S16:
	case AV_SAMPLE_FMT_S32:
	case AV_SAMPLE_FMT_U8P:
	case AV_SAMPLE_FMT_S16P:
	case AV_SAMPLE_FMT_S32P:
		// integer => Scale to [-1, 1] and convert to float.
		ret = val / (float)((1 << (sampleSize * 8 - 1)) - 1);
		break;

	case AV_SAMPLE_FMT_FLT:
	case AV_SAMPLE_FMT_FLTP:
		ret = *(float*)&val;
		break;

	case AV_SAMPLE_FMT_DBL:
	case AV_SAMPLE_FMT_DBLP:
		ret = (float)(*(double*)&val);
		break;

	default:
		return 0.0f;
	}

	return ret;
}

static int MediaDecoder_NextFrame_Common(InternalContext* ctx, uint32_t streamIndex)
{
	if (streamIndex != -1)
	{
		if (ctx->frame->pts != AV_NOPTS_VALUE)
		{
			AVStream* stream = ctx->format->streams[streamIndex];
			ctx->ctx.playback.position = (double)ctx->frame->pts * stream->time_base.num / stream->time_base.den;
		}
	}
	return 0;
}

static int MediaDecoder_NextFrame_Video(MediaDecoderContext* context)
{
	InternalContext* ctx = (InternalContext*)context;
	AVFrame* frame = ctx->frame;

	if (context->video.decodedWidth < 1)
		context->video.decodedWidth = frame->width;
	if (context->video.decodedHeight < 1)
		context->video.decodedHeight = frame->height;

	// we tell ImageResizer_SetParameters() that pixFmt is actually AVPixelFormat by or'ing 0x10000.
	enum MediaDecoderPixelFormat pixFmt = frame->format | 0x10000;

	// convert to size and format that we want
	ImageResizer_SetParameters(
		ctx->resizer, frame->width, frame->height, pixFmt, context->video.decodedWidth, context->video.decodedHeight,
		context->video.decodedPixelFormat
	);

	if (!context->video.frameBuffer)
	{
		context->video.bytesPerFrame = av_image_get_buffer_size(
			MapPixelFormat(ctx->ctx.video.decodedPixelFormat), context->video.decodedWidth,
			context->video.decodedHeight, 1
		);

		if (context->video.bytesPerFrame < 1)
			return -1;

		// create frame buffer if it doesnt already exist
		context->video.frameBuffer = malloc(context->video.bytesPerFrame);
		if (!context->video.frameBuffer)
			return -1;
	}

	uint8_t* outImageData[] = {context->video.frameBuffer, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	// int linesize = av_image_get_linesize(context->video.decodedPixelFormat, context->video.decodedWidth, 1);
	int outImageLineSize[] = {0, 0, 0, 0, 0, 0, 0, 0};

	outImageLineSize[0] = GetPixelFormatSize(context->video.decodedPixelFormat);
	if (outImageLineSize[0] == -1)
		return -1;

	outImageLineSize[0] *= context->video.decodedWidth;
	ImageResizer_Resize(ctx->resizer, (const uint8_t**)frame->data, frame->linesize, outImageData, outImageLineSize);

	MediaDecoder_NextFrame_Common(ctx, context->playback.selectedVideoStream);

	return 0;
}

static int MediaDecoder_NextFrame_Audio(MediaDecoderContext* context)
{
	InternalContext* ctx = (InternalContext*)context;
	AVFrame* frame = ctx->frame;

	if (context->audio.originalSampleRate <= 0)
	{
		context->audio.originalSampleRate = frame->sample_rate;
		if (frame->sample_rate <= 0)
		{
			context->audio.originalSampleRate = 44100;
		}

		if (context->audio.decodedSampleRate <= 0)
		{
			context->audio.decodedSampleRate = context->audio.originalSampleRate;
		}

		context->audio.originalChannelLayout = FromChannelLayoutToEnum(frame->ch_layout);

		if (context->audio.decodedChannelLayout <= 0)
		{
			context->audio.decodedChannelLayout = context->audio.originalChannelLayout;
		}

		context->audio.channelCount = frame->ch_layout.nb_channels;
	}

	// resample and reformat
	SoundResampler_SetParameters(
		ctx->resampler, frame->sample_rate, FromChannelLayoutToEnum(frame->ch_layout),
		(enum MediaDecoderSampleFormat)frame->format, context->audio.decodedSampleRate,
		(uint64_t)context->audio.decodedChannelLayout, context->audio.decodedSampleFormat
	);

	// samples in read frame
	uint32_t inSamplesPerChannel = frame->nb_samples;

	// samples in buffer after resampling
	uint32_t outSamplesPerChannel = SoundResampler_FindMaxOutputSamples(ctx->resampler, inSamplesPerChannel);

	enum AVSampleFormat rawFormat = MapSampleFormat(context->audio.decodedSampleFormat);

	if (!context->audio.frameBuffer)
	{
		context->audio.bytesPerSample = av_get_bytes_per_sample(rawFormat);

		// count number of channels
		context->audio.channelCount = 0;
		for (int i = 0; i < sizeof(context->audio.channelCount) * 8; i++)
		{
			if ((context->audio.decodedChannelLayout >> i) & 1)
			{
				context->audio.channelCount++;
			}
		}

		uint32_t bytesInFrame =
			av_samples_get_buffer_size(NULL, context->audio.channelCount, outSamplesPerChannel, rawFormat, 0);
		if (bytesInFrame < 0)
			return -1;

		context->audio.frameBuffer = malloc(bytesInFrame);
		if (!context->audio.frameBuffer)
			return -1;
		context->audio.sampleCapacityPerChannel = outSamplesPerChannel;
	}
	else if (outSamplesPerChannel > context->audio.sampleCapacityPerChannel)
	{
		// make buffer larger if needed
		uint32_t bytesInFrame =
			av_samples_get_buffer_size(NULL, context->audio.channelCount, outSamplesPerChannel, rawFormat, 0);
		if (bytesInFrame < 0)
			return -1;
		void* tmp = realloc(context->audio.frameBuffer, bytesInFrame);
		context->audio.sampleCapacityPerChannel = outSamplesPerChannel;
		if (!tmp)
			return -1;
		context->audio.frameBuffer = tmp;
	}
	context->audio.sampleCountPerChannel = SoundResampler_Resample(
		ctx->resampler, (const uint8_t**)frame->extended_data, inSamplesPerChannel, &context->audio.frameBuffer,
		outSamplesPerChannel
	);

	MediaDecoder_NextFrame_Common(ctx, context->playback.selectedAudioStream);

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MediaDecoderContext* MediaDecoder_Open(const char* url)
{
	InternalContext* ctx = malloc(sizeof(*ctx));
	if (!ctx)
		return NULL;

	// open container file and loop through each stream
	ctx->format = avformat_alloc_context();
	int ret;
	ret = avformat_open_input(&ctx->format, url, NULL /*autodetect fileformat*/, NULL /*no options*/);
	if (ret < 0)
		return NULL;

	// allocate packet and frame, so we can use them when decoding
	ctx->packet = av_packet_alloc();
	ctx->frame = av_frame_alloc();
#ifndef DISABLE_HARDWARE_ACCELERATION
	ctx->frame2 = av_frame_alloc();
#endif

	// set default values for context
	MediaDecoderPlaybackInfo* playback = &ctx->ctx.playback;
	playback->streamCount = ctx->format->nb_streams;
	playback->selectedVideoStream = playback->selectedAudioStream = playback->selectedSubtitleStream = -1;

	ctx->codecVideo = NULL;
	ctx->ctx.video.frameBuffer = NULL;
	ctx->resizer = NULL;

	ctx->codecAudio = NULL;
	ctx->ctx.audio.frameBuffer = NULL;
	ctx->ctx.audio.sampleCountPerChannel = 0;
	ctx->ctx.audio.channelCount = 0;
	ctx->ctx.audio.frameBuffer = NULL;
	ctx->isImage = 0;
	ctx->resampler = NULL;

	// TODO: use av_find_best_stream(...)
	// av_find_best_stream(ctx->format, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, NULL);

	for (unsigned int i = 0; i < ctx->format->nb_streams; i++)
	{
		AVStream* stream = ctx->format->streams[i];
		if (playback->selectedVideoStream == -1 && stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			playback->selectedVideoStream = i;
			ctx->ctx.video.originalWidth = ctx->ctx.video.decodedWidth = stream->codecpar->width;
			ctx->ctx.video.originalHeight = ctx->ctx.video.decodedHeight = stream->codecpar->height;
			ctx->ctx.video.decodedPixelFormat = PIXEL_FORMAT_R8G8B8A8_UINT;
			if (ctx->ctx.video.decodedWidth > 0 && ctx->ctx.video.decodedHeight > 0)
			{
				ctx->ctx.video.bytesPerFrame = av_image_get_buffer_size(
					MapPixelFormat(ctx->ctx.video.decodedPixelFormat), ctx->ctx.video.decodedWidth,
					ctx->ctx.video.decodedHeight, 1
				);
			}
			else
			{
				ctx->ctx.video.bytesPerFrame = 0;
			}
		}
		else if (playback->selectedAudioStream == -1 && stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			playback->selectedAudioStream = i;
		}
		else if (playback->selectedSubtitleStream == -1 && stream->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE)
		{
			playback->selectedSubtitleStream = i;
		}
	}

	playback->position = 0.0;
	playback->duration = 0.0;

	if (playback->selectedVideoStream != -1 && playback->selectedAudioStream == -1 &&
		ctx->format->duration == AV_NOPTS_VALUE)
	{
		// assume that if we can not know or estimate duration of media,
		// then it probably only has one frame, which means that it must be an image.
		ctx->isImage = 1;
	}

	for (int i = 0; i < sizeof(playback->selectedStreams) / sizeof(*playback->selectedStreams); i++)
	{
		if (playback->selectedStreams[i] == -1)
			continue;

		AVStream* stream = ctx->format->streams[playback->selectedStreams[i]];

		if (stream->duration != AV_NOPTS_VALUE)
		{
			double d = (double)stream->duration * stream->time_base.num / stream->time_base.den;
			if (d > playback->duration)
				playback->duration = d / 1000.0;

			AVRational r = stream->avg_frame_rate;
			if (r.den > 0)
			{
				int64_t frameCount = stream->duration * r.num / r.den;
				// ctx->isImage = frameCount == 1;
			}
		}
		if (ctx->format->duration != AV_NOPTS_VALUE)
		{
			double d = (double)ctx->format->duration * stream->time_base.num / stream->time_base.den;
			if (d > playback->duration)
				playback->duration = d / 1000.0;
		}

		if (stream->nb_frames != 0)
		{
			// ctx->isImage = stream->nb_frames == 1;
		}
	}

	if (playback->selectedVideoStream != -1)
	{
		// prepare correct video codec, try to setup hardware accelerated decoder
		const AVCodecParameters* codecParams = ctx->format->streams[playback->selectedVideoStream]->codecpar;
		const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);

		ctx->codecVideo = avcodec_alloc_context3(codec);
		avcodec_parameters_to_context(ctx->codecVideo, codecParams);

		enum AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;

#ifndef DISABLE_HARDWARE_ACCELERATION
		int i = 0;
		while (i >= 0)
		{
			const AVCodecHWConfig* hwConfig = avcodec_get_hw_config(codecVideo, i);
			if (!hwConfig)
			{
				break;
			}

			if (hwConfig->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX)
			{
				type = AV_HWDEVICE_TYPE_NONE;
				while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
				{
					if (hwConfig->device_type == type)
					{
						hw_pix_fmt = hwConfig->pix_fmt;
						if (hw_decoder_init(ctx->codecVideo, type) < 0)
						{
							// couldnt initialize hardware decoder, continue searching
							hw_pix_fmt = AV_PIX_FMT_NONE;
							continue;
						}
						i = -1;
						break;
					}
				}
				if (i == -1)
					break;
			}

			i++;
		}
#endif

		if (type == AV_HWDEVICE_TYPE_NONE)
		{
			// printf("using softwaredecoding.\n");
		}

		avcodec_open2(ctx->codecVideo, codec, NULL /*no options*/);

		ctx->resizer = ImageResizer_CreateContext();
	}

	if (playback->selectedAudioStream != -1)
	{
		const AVCodecParameters* codecParams = ctx->format->streams[playback->selectedAudioStream]->codecpar;
		const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);

		ctx->codecAudio = avcodec_alloc_context3(codec);
		ret = avcodec_parameters_to_context(ctx->codecAudio, codecParams);
		ret = avcodec_open2(ctx->codecAudio, codec, NULL /*no options*/);
		ctx->ctx.audio.channelCount = ctx->codecAudio->ch_layout.nb_channels;
		ctx->ctx.audio.decodedSampleRate = ctx->ctx.audio.originalSampleRate = ctx->codecAudio->sample_rate;

		ctx->ctx.audio.decodedChannelLayout = ctx->ctx.audio.originalChannelLayout =
			FromChannelLayoutToEnum(ctx->codecAudio->ch_layout);
		ctx->ctx.audio.decodedSampleFormat = SAMPLE_FORMAT_FLOAT;

		if (ctx->ctx.audio.channelCount <= 0 || ctx->ctx.audio.originalSampleRate <= 0 ||
			ctx->ctx.audio.originalChannelLayout <= 0 || ctx->ctx.audio.decodedSampleFormat <= 0)
		{
			// in case that one value is not set, set all to zero, these will be later set when first frame is read
			ctx->ctx.audio.channelCount = ctx->ctx.audio.decodedSampleRate = ctx->ctx.audio.originalSampleRate =
				ctx->ctx.audio.decodedChannelLayout = ctx->ctx.audio.originalChannelLayout = 0;
		}

		ctx->resampler = SoundResampler_CreateContext();
	}

	ctx->didPlaybackStart = 0;

	return (MediaDecoderContext*)ctx;
}

int MediaDecoder_IsImage(MediaDecoderContext* context)
{
	InternalContext* ctx = (InternalContext*)context;
	return ctx->isImage == 1 || ctx->isImage == 2;
}

int MediaDecoder_Play(MediaDecoderContext* context, double time)
{
	InternalContext* ctx = (InternalContext*)context;
	if (!ctx->didPlaybackStart)
	{
		ctx->didPlaybackStart = 1;
		ctx->loopCount = 0;
		ctx->lastTime = time;
		ctx->startTime = time;
	}

	time -= ctx->startTime;

	int ret;
	if (context->playback.position <= time)
	{
		ret = MediaDecoder_NextFrame(context, NULL);
		if (ret == 1)
		{
			ret = MediaDecoder_Seek(context, 0);
			if (!ret)
			{
				ctx->startTime = time + ctx->startTime;
				ctx->lastTime = 0;
				ctx->loopCount++;
				return ret;
			}
		}
	}
	else
	{
		ret = 0;
	}

	if (ret == 0)
		ctx->lastTime = time;
	return ret;
}

int MediaDecoder_NextFrame(MediaDecoderContext* context, uint32_t* streamIndex)
{
	InternalContext* ctx = (InternalContext*)context;

	if (ctx->isImage == 2)
	{
		// detected stream with a single frame
		return 1;
	}

	// read next frame
	int ret;
	AVFrame* softwareFrame = ctx->frame;
	AVCodecContext* codec = NULL;
	while ((ret = av_read_frame(ctx->format, ctx->packet)) == 0)
	{
		if (ctx->packet->stream_index == context->playback.selectedVideoStream)
			codec = ctx->codecVideo;
		else if (ctx->packet->stream_index == context->playback.selectedAudioStream)
			codec = ctx->codecAudio;
		else
			codec = NULL;

		if (!codec)
		{
			av_packet_unref(ctx->packet);
			continue;
		}

		ret = avcodec_send_packet(codec, ctx->packet);
		ret = avcodec_receive_frame(codec, ctx->frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		{
			av_packet_unref(ctx->packet);
			continue;
		}

		av_packet_unref(ctx->packet);

		// one frame was read

#ifndef DISABLE_HARDWARE_ACCELERATION
		if (ctx->frame->format == hw_pix_fmt)
		{
			if (av_hwframe_transfer_data(ctx->frame2, ctx->frame, AV_HWFRAME_TRANSFER_DIRECTION_FROM) < 0)
				return -1;

			softwareFrame = ctx->frame2;
		}
#endif

		ret = 0;
		break;
	}

	if (ret != 0)
	{
		if (ret == AVERROR_EOF)
		{
			// update if duration is inaccurate
			context->playback.duration = context->playback.position;

			// no need to seek, because its only one frame
			if (context->playback.duration == 0.0)
			{
				ctx->isImage = 2;
			}

			return 1;
		}
		else
		{
			char* str = av_err2str(ret);
			// error reading/decoding/demuxing frame
			return -1;
		}
	}

	if (codec == ctx->codecVideo)
	{
		if (streamIndex)
			*streamIndex = ctx->ctx.playback.selectedVideoStream;

		// always update original size to support different sized frames
		context->video.originalWidth = softwareFrame->width;
		context->video.originalHeight = softwareFrame->height;

		ctx->funcDecodeFrame = &MediaDecoder_NextFrame_Video;
	}
	else if (codec == ctx->codecAudio)
	{
		if (streamIndex)
			*streamIndex = ctx->ctx.playback.selectedAudioStream;

		// always update original sample rate to support variable sample rate
		context->audio.originalSampleRate = softwareFrame->sample_rate > 0 ? softwareFrame->sample_rate : 44100;

		ctx->funcDecodeFrame = &MediaDecoder_NextFrame_Audio;
	}
	else
	{
		// should never happen
		ret = -100;
	}

	return ret;
}

int MediaDecoder_DecodeFrame(MediaDecoderContext* context)
{
	InternalContext* ctx = (InternalContext*)context;
	return ctx->funcDecodeFrame(context);
}

int MediaDecoder_Seek(MediaDecoderContext* context, double time)
{
	InternalContext* ctx = (InternalContext*)context;

	for (int si = 0; si < sizeof(context->playback.selectedStreams) / sizeof(*context->playback.selectedStreams); si++)
	{
		if (context->playback.selectedStreams[si] != -1)
		{
			AVStream* stream = ctx->format->streams[context->playback.selectedStreams[si]];

			AVRational timebase = stream->time_base;
			int64_t shortTime = (int64_t)(0.1 * timebase.den / timebase.num);
			if (shortTime < 1)
				shortTime = 1;

			int64_t ts = (int64_t)(time * timebase.den / timebase.num);
			int64_t min = ts - shortTime;
			int64_t max = ts + shortTime;

			int ret = avformat_seek_file(ctx->format, -1, INT64_MIN, ts, INT64_MAX, 0);
			if (ret < 0)
			{
				char* str = av_err2str(ret);
				return -1;
			}
			break;
		}
	}

	if (MediaDecoder_NextFrame(context, NULL))
		return -1;
	if (MediaDecoder_NextFrame(context, NULL))
		return -1;

	context->playback.position = time;
	return 0;
}

int MediaDecoder_Close(MediaDecoderContext** context)
{
	if (!context || !*context)
		return 0;

	InternalContext* ctx = (InternalContext*)*context;
	if (ctx->ctx.video.frameBuffer)
		free(ctx->ctx.video.frameBuffer);
	if (ctx->ctx.audio.frameBuffer)
		free(ctx->ctx.audio.frameBuffer);
	ImageResizer_ReleaseContext(&ctx->resizer);
	av_packet_free(&ctx->packet);
#ifndef DISABLE_HARDWARE_ACCELERATION
	av_frame_free(&ctx->frame2);
#endif
	av_frame_free(&ctx->frame);
	if (ctx->codecVideo)
		avcodec_free_context(&ctx->codecVideo);
	if (ctx->codecAudio)
		avcodec_free_context(&ctx->codecAudio);
	avformat_free_context(ctx->format);
	free(*context);
	*context = NULL;
	return 0;
}

int MediaDecoder_TakeAudioSamples(MediaDecoderContext* context, float* buffer, uint32_t sampleCountPerChannel)
{
	// int readSamples;
	// if (context->audio.sampleCountPerChannel >= sampleCountPerChannel)
	//	readSamples = sampleCountPerChannel * context->audio.channelCount;
	// else
	//	readSamples = context->audio.sampleCountPerChannel * context->audio.channelCount;

	// if (readSamples)
	//{
	//	memcpy(buffer, context->audio.frameBuffer, sizeof(float) * readSamples);
	//	size_t diff = (context->audio.sampleCountPerChannel * (size_t)context->audio.channelCount) - readSamples;
	//	memmove(context->audio.frameBuffer, context->audio.frameBuffer + sizeof(float) * readSamples, diff);
	//	context->audio.sampleCountPerChannel -= (readSamples / context->audio.channelCount);
	// }

	// return readSamples;
	return 0;
}

#include "SoundResampler.h"
#include "Internal.h"
#include <libswresample/swresample.h>

typedef struct
{
	struct SwrContext* ctx;
	int cacheInSampleRate;
	enum MediaDecoderChannelLayout cacheInChannelLayout;
	enum MediaDecoderSampleFormat cacheInFormat;
	int cacheOutSampleRate;
	enum MediaDecoderChannelLayout cacheOutChannelLayout;
	enum MediaDecoderSampleFormat cacheOutFormat;
} InternalState;

SoundResamplerContext* SoundResampler_CreateContext()
{
	InternalState* ctx = malloc(sizeof(*ctx));
	ctx->ctx = NULL;
	ctx->cacheInSampleRate = -1;
	ctx->cacheInChannelLayout = CHANNEL_LAYOUT_STEREO;
	ctx->cacheInFormat = AV_SAMPLE_FMT_NONE;
	ctx->cacheOutSampleRate = -1;
	ctx->cacheOutChannelLayout = CHANNEL_LAYOUT_STEREO;
	ctx->cacheOutFormat = AV_SAMPLE_FMT_NONE;

	return (SoundResamplerContext*)ctx;
}

bool SoundResampler_SetParameters(SoundResamplerContext* context,
	int inSampleRate, enum MediaDecoderChannelLayout inChannelLayout, enum MediaDecoderSampleFormat inFormat,
	int outSampleRate, enum MediaDecoderChannelLayout outChannelLayout, enum MediaDecoderSampleFormat outFormat)
{
	InternalState* ctx = (InternalState*)context;

	if (ctx->ctx)
	{
		if (inSampleRate == ctx->cacheInSampleRate && inChannelLayout == ctx->cacheInChannelLayout && inFormat == ctx->cacheInFormat &&
			outSampleRate == ctx->cacheOutSampleRate && outChannelLayout == ctx->cacheOutChannelLayout && outFormat == ctx->cacheOutFormat)
		{
			return ctx->ctx;
		}
	}

	ctx->cacheInSampleRate = inSampleRate;
	ctx->cacheInChannelLayout = inChannelLayout;
	ctx->cacheInFormat = inFormat;
	ctx->cacheOutSampleRate = outSampleRate;
	ctx->cacheOutChannelLayout = outChannelLayout;
	ctx->cacheOutFormat = outFormat;
	swr_free(&ctx->ctx);
	//ctx->ctx = swr_alloc_set_opts(ctx->ctx, outChannelLayout, outFormat, outSampleRate, inChannelLayout, inFormat, inSampleRate, 0, NULL);

	struct AVChannelLayout inChLay = FromEnumToChannelLayout(inChannelLayout);
	struct AVChannelLayout outChLay = FromEnumToChannelLayout(outChannelLayout);

	if (!swr_alloc_set_opts2(&ctx->ctx,
		&outChLay, (enum AVSampleFormat)outFormat, outSampleRate,
		&inChLay, (enum AVSampleFormat)inFormat, inSampleRate,
		0, NULL))
	{
		swr_init(ctx->ctx);
	}

	return ctx->ctx != NULL;
}

int SoundResampler_FindMaxOutputSamples(SoundResamplerContext* context, int inSampleCountPerChannel)
{
	InternalState* ctx = (InternalState*)context;
	return swr_get_out_samples(ctx->ctx, inSampleCountPerChannel);
}

int SoundResampler_Resample(SoundResamplerContext* context, const uint8_t** inSoundData, int inSampleCountPerChannel, uint8_t** outSoundData, int outSampleCountPerChannel)
{
	InternalState* ctx = (InternalState*)context;
	return swr_convert(ctx->ctx,
		outSoundData, outSampleCountPerChannel,
		inSoundData, inSampleCountPerChannel);
}

void SoundResampler_ReleaseContext(SoundResamplerContext** context)
{
	InternalState* ctx = (InternalState*)context;
	swr_free(&ctx->ctx);
	free(ctx);
	*context = NULL;
}

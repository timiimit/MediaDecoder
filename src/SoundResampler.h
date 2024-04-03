#pragma once

#include "MediaDecoder.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct SoundResamplerContext SoundResamplerContext;

#ifdef __cplusplus
extern "C"
{
#endif
	SoundResamplerContext* SoundResampler_CreateContext();

	bool SoundResampler_SetParameters(
		SoundResamplerContext* context, int inSampleRate, enum MediaDecoderChannelLayout inChannelLayout,
		enum MediaDecoderSampleFormat inFormat, int outSampleRate, enum MediaDecoderChannelLayout outChannelLayout,
		enum MediaDecoderSampleFormat outFormat
	);

	int SoundResampler_FindMaxOutputSamples(SoundResamplerContext* context, int inSampleCountPerChannel);

	int SoundResampler_Resample(
		SoundResamplerContext* context, const uint8_t** inSoundData, int inSampleCountPerChannel,
		uint8_t** outSoundData, int outSampleCountPerChannel
	);

	void SoundResampler_ReleaseContext(SoundResamplerContext** context);
#ifdef __cplusplus
}
#endif

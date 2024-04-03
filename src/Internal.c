#include "MediaDecoder.h"
#include <libavutil/channel_layout.h>
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>

enum MediaDecoderChannelLayout FromChannelLayoutToEnum(struct AVChannelLayout channelLayout)
{
	static AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
	static AVChannelLayout mono = AV_CHANNEL_LAYOUT_MONO;
	if (!av_channel_layout_compare(&channelLayout, &stereo))
	{
		return CHANNEL_LAYOUT_STEREO;
	}
	else if (!av_channel_layout_compare(&channelLayout, &mono))
	{
		return CHANNEL_LAYOUT_MONO;
	}
	else
	{
		return CHANNEL_LAYOUT_STEREO;
	}
}

struct AVChannelLayout FromEnumToChannelLayout(enum MediaDecoderChannelLayout channelLayout)
{
	struct AVChannelLayout ret;
	if (channelLayout == CHANNEL_LAYOUT_MONO)
	{
		struct AVChannelLayout mono = AV_CHANNEL_LAYOUT_MONO;
		av_channel_layout_copy(&ret, &mono);
	}
	else
	{
		struct AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
		av_channel_layout_copy(&ret, &stereo);
	}

	return ret;
}

enum AVPixelFormat MapPixelFormat(enum MediaDecoderPixelFormat pixelFormat)
{
	switch (pixelFormat)
	{
	case PIXEL_FORMAT_R8_UINT:
		return AV_PIX_FMT_GRAY8;
	case PIXEL_FORMAT_R8G8B8_UINT:
		return AV_PIX_FMT_RGB24;
	case PIXEL_FORMAT_R8G8B8A8_UINT:
		return AV_PIX_FMT_RGBA;
	case PIXEL_FORMAT_R16G16B16_UINT:
		return AV_PIX_FMT_RGB48;
	case PIXEL_FORMAT_R16G16B16A16_UINT:
		return AV_PIX_FMT_RGBA64;
	case PIXEL_FORMAT_R16G16B16A16_FLOAT:
		return AV_PIX_FMT_RGBAF16;
	case PIXEL_FORMAT_R32G32B32A32_FLOAT:
		return AV_PIX_FMT_RGBAF32;

	default:
		return AV_PIX_FMT_NONE;
	}
}

enum AVSampleFormat MapSampleFormat(enum MediaDecoderSampleFormat sampleFormat)
{
	switch (sampleFormat)
	{
	case SAMPLE_FORMAT_UINT8:
		return AV_SAMPLE_FMT_U8;
	case SAMPLE_FORMAT_INT16:
		return AV_SAMPLE_FMT_S16;
	case SAMPLE_FORMAT_INT32:
		return AV_SAMPLE_FMT_S32;
	case SAMPLE_FORMAT_FLOAT:
		return AV_SAMPLE_FMT_FLT;
	case SAMPLE_FORMAT_DOUBLE:
		return AV_SAMPLE_FMT_DBL;

	default:
		return AV_SAMPLE_FMT_NONE;
	}
}

int GetPixelFormatSize(enum MediaDecoderPixelFormat pixelFormat)
{
	switch (pixelFormat)
	{
	case PIXEL_FORMAT_R8_UINT:
		return 1;
	case PIXEL_FORMAT_R8G8B8_UINT:
		return 3;
	case PIXEL_FORMAT_R8G8B8A8_UINT:
		return 4;
	case PIXEL_FORMAT_R16G16B16_UINT:
		return 6;
	case PIXEL_FORMAT_R16G16B16A16_UINT:
	case PIXEL_FORMAT_R16G16B16A16_FLOAT:
		return 8;
	case PIXEL_FORMAT_R32G32B32A32_FLOAT:
		return 16;

	default:
		return -1;
	}
}
#include "MediaDecoder.h"
#include <libavutil/channel_layout.h>

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
#pragma once
#include <stdint.h>

typedef struct MediaDecoderPlaybackInfo
{
	uint32_t streamCount;
	union
	{
		struct
		{
			uint32_t selectedVideoStream;
			uint32_t selectedAudioStream;
			uint32_t selectedSubtitleStream;
		};
		uint32_t selectedStreams[3];
	};

	double duration;
	double position;
} MediaDecoderPlaybackInfo;

typedef enum MediaDecoderPixelFormat
{
	RGB24 = 2,
	BGR24 = 3,
	GRAY8 = 8,
	ARGB32 = 25,
	RGBA32 = 26,
	ABGR32 = 27,
	BGRA32 = 28,
} MediaDecoderPixelFormat;

typedef enum MediaDecoderSampleFormat
{
	SAMPLE_FORMAT_UINI8 = 0,
	SAMPLE_FORMAT_INI16 = 1,
	SAMPLE_FORMAT_INI32 = 2,
	SAMPLE_FORMAT_FLOAT = 3,
	SAMPLE_FORMAT_DOUBLE = 4,
} MediaDecoderSampleFormat;

typedef enum MediaDecoderChannelLayout
{
	CHANNEL_LAYOUT_MONO = 4,
	CHANNEL_LAYOUT_STEREO = 3,
	//Surround51,
	//Surround71,
} MediaDecoderChannelLayout;

typedef struct MediaDecoderVideoInfo
{
	uint32_t originalWidth;
	uint32_t originalHeight;
	uint32_t decodedWidth;
	uint32_t decodedHeight;
	MediaDecoderPixelFormat decodedPixelFormat;

	uint8_t* frameBuffer;

	uint32_t bytesPerFrame;
} MediaDecoderVideoInfo;

typedef struct MediaDecoderAudioInfo
{
	uint32_t originalSampleRate;
	MediaDecoderChannelLayout originalChannelLayout;
	uint32_t decodedSampleRate;
	MediaDecoderChannelLayout decodedChannelLayout;
	MediaDecoderSampleFormat decodedSampleFormat;

	uint32_t bytesPerSample;

	uint8_t* frameBuffer;
	uint32_t sampleCapacityPerChannel;
	uint32_t sampleCountPerChannel;
	uint32_t channelCount;
} MediaDecoderAudioInfo;

typedef enum MediaDecoderStreamType
{
	UNKNOWN_STREAM,
	VIDEO_STREAM,
	AUDIO_STREAM,
} MediaDecoderStreamType;

typedef struct MediaDecoderStreamInfo
{
	MediaDecoderStreamType type;
	uint32_t width;
	uint32_t height;
	uint32_t bitrate;
	double duration;
} MediaDecoderStreamInfo;

typedef struct MediaDecoderContext
{
	MediaDecoderPlaybackInfo playback;
	MediaDecoderVideoInfo video;
	MediaDecoderAudioInfo audio;
} MediaDecoderContext;

#define MEDIADECODER_EXPORT //__declspec(dllexport)

#ifdef __cplusplus
extern "C"
{
#endif
	MEDIADECODER_EXPORT MediaDecoderContext* MediaDecoder_Open(const char* url);
	//MEDIADECODER_EXPORT MediaDecoderStreamInfo MediaDecoder_GetStreamInfo(MediaDecoderContext* context, uint32_t streamIndex);
	//MEDIADECODER_EXPORT int MediaDecoder_SelectVideoStream(MediaDecoderContext* context, uint32_t steramIndex);
	//MEDIADECODER_EXPORT int MediaDecoder_SelectAudioStream(MediaDecoderContext* context, uint32_t steramIndex);
	MEDIADECODER_EXPORT int MediaDecoder_IsImage(MediaDecoderContext* context);
	MEDIADECODER_EXPORT int MediaDecoder_Play(MediaDecoderContext* context, double time);
	MEDIADECODER_EXPORT int MediaDecoder_NextFrame(MediaDecoderContext* context, uint32_t* streamIndex);
	MEDIADECODER_EXPORT int MediaDecoder_Seek(MediaDecoderContext* context, double time);
	MEDIADECODER_EXPORT int MediaDecoder_Close(MediaDecoderContext** context);
#ifdef __cplusplus
}
#endif



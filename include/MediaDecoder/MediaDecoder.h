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
	PIXEL_FORMAT_UNKNOWN,
	PIXEL_FORMAT_R8_UINT,
	PIXEL_FORMAT_R8G8B8_UINT,
	PIXEL_FORMAT_R8G8B8A8_UINT,
	PIXEL_FORMAT_R16G16B16_UINT,
	PIXEL_FORMAT_R16G16B16A16_UINT,
	PIXEL_FORMAT_R16G16B16A16_FLOAT,
	PIXEL_FORMAT_R32G32B32A32_FLOAT,
} MediaDecoderPixelFormat;

typedef enum MediaDecoderSampleFormat
{
	SAMPLE_FORMAT_UNKNOWN,
	SAMPLE_FORMAT_UINT8,
	SAMPLE_FORMAT_INT16,
	SAMPLE_FORMAT_INT32,
	SAMPLE_FORMAT_FLOAT,
	SAMPLE_FORMAT_DOUBLE,
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

	/// @brief Read next frame
	/// @param context Context returned by MediaDecoder_Open
	/// @param streamIndex [out] index of stream in current frame
	/// @return
	MEDIADECODER_EXPORT int MediaDecoder_NextFrame(MediaDecoderContext* context, uint32_t* streamIndex);

	/// @brief Decode current frame
	/// @param context Context returned by MediaDecoder_Open
	/// @return 0 if frame was successfully decoded
	MEDIADECODER_EXPORT int MediaDecoder_DecodeFrame(MediaDecoderContext* context);
	MEDIADECODER_EXPORT int MediaDecoder_Seek(MediaDecoderContext* context, double time);
	MEDIADECODER_EXPORT int MediaDecoder_Close(MediaDecoderContext** context);
#ifdef __cplusplus
}
#endif



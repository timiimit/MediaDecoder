
#include "MediaDecoder.h"
#include <libavformat/avformat.h>

enum MediaDecoderChannelLayout FromChannelLayoutToEnum(struct AVChannelLayout channelLayout);
struct AVChannelLayout FromEnumToChannelLayout(enum MediaDecoderChannelLayout channelLayout);

enum AVPixelFormat MapPixelFormat(enum MediaDecoderPixelFormat pixelFormat);
enum AVSampleFormat MapSampleFormat(enum MediaDecoderSampleFormat sampleFormat);
int GetPixelFormatSize(enum MediaDecoderPixelFormat pixelFormat);
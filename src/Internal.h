
#include "MediaDecoder.h"
#include <libavformat/avformat.h>

enum MediaDecoderChannelLayout FromChannelLayoutToEnum(struct AVChannelLayout channelLayout);
struct AVChannelLayout FromEnumToChannelLayout(enum MediaDecoderChannelLayout channelLayout);
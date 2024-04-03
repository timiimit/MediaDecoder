#include "ImageResizer.h"
#include "Internal.h"
#include <libswscale/swscale.h>

typedef struct
{
	struct SwsContext* ctxScale;
	int cacheInWidth;
	int cacheInHeight;
	enum MediaDecoderPixelFormat cacheInFormat;
	int cacheOutWidth;
	int cacheOutHeight;
	enum MediaDecoderPixelFormat cacheOutFormat;
} InternalState;

static enum AVPixelFormat FixDeprecatedFormat(enum AVPixelFormat format)
{
	switch (format)
	{
	case AV_PIX_FMT_YUVJ420P:
		format = AV_PIX_FMT_YUV420P;
		break;
	case AV_PIX_FMT_YUVJ422P:
		format = AV_PIX_FMT_YUV422P;
		break;
	case AV_PIX_FMT_YUVJ444P:
		format = AV_PIX_FMT_YUV444P;
		break;
	case AV_PIX_FMT_YUVJ440P:
		format = AV_PIX_FMT_YUV440P;
		break;
	default:
		break;
	}

	return format;
}

ImageResizerContext* ImageResizer_CreateContext()
{
	InternalState* ctx = malloc(sizeof(*ctx));
	ctx->ctxScale = NULL;
	ctx->cacheInWidth = -1;
	ctx->cacheInHeight = -1;
	ctx->cacheInFormat = PIXEL_FORMAT_UNKNOWN;
	ctx->cacheOutWidth = -1;
	ctx->cacheOutHeight = -1;
	ctx->cacheOutFormat = PIXEL_FORMAT_UNKNOWN;

	return (ImageResizerContext*)ctx;
}

bool ImageResizer_SetParameters(
	ImageResizerContext* context, int inWidth, int inHeight, enum MediaDecoderPixelFormat inFormat, int outWidth,
	int outHeight, enum MediaDecoderPixelFormat outFormat
)
{
	InternalState* ctx = (InternalState*)context;

	enum AVPixelFormat inFormatRaw = MapPixelFormat(inFormat);
	if (inFormatRaw == AV_PIX_FMT_NONE)
		inFormatRaw = ((enum AVPixelFormat)inFormat) & 0xFFFF;

	enum AVPixelFormat outFormatRaw = MapPixelFormat(outFormat);
	if (outFormatRaw == AV_PIX_FMT_NONE)
		outFormatRaw = ((enum AVPixelFormat)outFormat) & 0xFFFF;

	inFormatRaw = FixDeprecatedFormat(inFormatRaw);
	outFormatRaw = FixDeprecatedFormat(outFormatRaw);

	if (ctx->ctxScale)
	{
		if (inWidth == ctx->cacheInWidth && inHeight == ctx->cacheInHeight && inFormat == ctx->cacheInFormat &&
			outWidth == ctx->cacheOutWidth && outHeight == ctx->cacheOutHeight && outFormat == ctx->cacheOutFormat)
		{
			return ctx->ctxScale;
		}
	}

	ctx->cacheInWidth = inWidth;
	ctx->cacheInHeight = inHeight;
	ctx->cacheInFormat = inFormat;
	ctx->cacheOutWidth = outWidth;
	ctx->cacheOutHeight = outHeight;
	ctx->cacheOutFormat = outFormat;

	sws_freeContext(ctx->ctxScale);
	ctx->ctxScale = sws_getContext(
		inWidth, inHeight, inFormatRaw, outWidth, outHeight, outFormatRaw, SWS_BILINEAR, NULL, NULL, NULL
	);

	return ctx->ctxScale != NULL;
}

int ImageResizer_Resize(
	ImageResizerContext* context, const uint8_t* const* inImageData, const int* inImageStride,
	uint8_t* const* outImageData, const int* outImageStride
)
{
	InternalState* ctx = (InternalState*)context;
	return sws_scale(ctx->ctxScale, inImageData, inImageStride, 0, ctx->cacheInHeight, outImageData, outImageStride);
}

void ImageResizer_ReleaseContext(ImageResizerContext** context)
{
	InternalState* ctx = (InternalState*)*context;
	sws_freeContext(ctx->ctxScale);
	free(*context);
	*context = NULL;
}
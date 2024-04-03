#pragma once

#include "MediaDecoder.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct ImageResizerContext ImageResizerContext;

#ifdef __cplusplus
extern "C"
{
#endif
	ImageResizerContext* ImageResizer_CreateContext();

	bool ImageResizer_SetParameters(
		ImageResizerContext* context, int inWidth, int inHeight, enum MediaDecoderPixelFormat inFormat, int outWidth,
		int outHeight, enum MediaDecoderPixelFormat outFormat
	);

	int ImageResizer_Resize(
		ImageResizerContext* context, const uint8_t* const* inImageData, const int* inImageStride,
		uint8_t* const* outImageData, const int* outImageStride
	);

	void ImageResizer_ReleaseContext(ImageResizerContext** context);
#ifdef __cplusplus
}
#endif

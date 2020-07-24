#pragma once

#include <algorithm>

#define QMaxSlice 4
#define ALIGNMENT 32
#define ALIGN_SIZE(size, align) (((size)+(align-1)) & (~(align-1)))

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#define MAKE_FOURCC(a, b, c, d) ( \
    (static_cast<uint32_t>(a)) | (static_cast<uint32_t>(b) << 8) | \
    (static_cast<uint32_t>(c) << 16) | (static_cast<uint32_t>(d) << 24))

namespace
{
	enum QeFourCC
	{
		/* i420	planar */
		FOURCC_I420 = MAKE_FOURCC('I', '4', '2', '0'),
		FOURCC_YU12 = MAKE_FOURCC('Y', 'U', '1', '2'),  //same with FOURCC_I420.
		FOURCC_YV12 = MAKE_FOURCC('Y', 'V', '1', '2'),

		FOURCC_NV12 = MAKE_FOURCC('N', 'V', '1', '2'),
		FOURCC_NV21 = MAKE_FOURCC('N', 'V', '2', '1'),  // 

		// 4:4:4 planar
		FOURCC_I444 = MAKE_FOURCC('I', '4', '4', '4'),
		// 4:0:0
		FOURCC_Y800 = MAKE_FOURCC('Y', '8', '0', '0'),

		//rgb32
		FOURCC_RGBA = MAKE_FOURCC('R', 'G', 'B', 'A'),
		FOURCC_BGRA = MAKE_FOURCC('B', 'G', 'R', 'A'),

		FOURCC_MJPG = MAKE_FOURCC('M', 'J', 'P', 'G'),
		FOURCC_H264 = MAKE_FOURCC('H', '2', '6', '4'),

		FOURCC_ANY = -1,
		FOURCC_Unknown = 0,
	};
}

namespace video {
	inline uint32_t CalBufNeedSize(int width, int height, int format)
	{
		int halfwidth = (width + 1) >> 1;
		int halfheight = (height + 1) >> 1;
		int len1 = 0;
		int len2 = 0;
		int len3 = 0;
		switch (format) {
		case FOURCC_Unknown:
			return 0;
		case FOURCC_I420:
		case FOURCC_YU12:
		case FOURCC_YV12:
			len1 = ALIGN_SIZE(width * height, ALIGNMENT);
			len2 = ALIGN_SIZE(halfwidth * halfheight, ALIGNMENT);
			len3 = ALIGN_SIZE(halfwidth * halfheight, ALIGNMENT);
			return len1 + len2 + len3;
		case FOURCC_NV12:
		case FOURCC_NV21:
			len1 = ALIGN_SIZE(width * height, ALIGNMENT);
			len2 = ALIGN_SIZE(halfwidth * 2 * halfheight, ALIGNMENT);
			return len1 + len2;
		case FOURCC_Y800:
			len1 = ALIGN_SIZE(width * height, ALIGNMENT);
			return len1;
		case FOURCC_RGBA:
		case FOURCC_BGRA:
			len1 = ALIGN_SIZE(width * height * 4, ALIGNMENT);
			return len1;
		case FOURCC_I444:
			len1 = ALIGN_SIZE(width * height, ALIGNMENT) * 3;
			return len1;
		}
		return 0;
	}

	inline void FillVideoFrameInfo(uint8_t* buffer, int width, int height, int format, uint8_t* data[QMaxSlice], uint32_t linesize[QMaxSlice])
	{
		int halfwidth = (width + 1) >> 1;
		int halfheight = (height + 1) >> 1;

		int len1 = 0;
		int len2 = 0;
		int len3 = 0;
		switch (format) {
		case FOURCC_Unknown:
			break;

		case FOURCC_I420:
		case FOURCC_YU12:
		case FOURCC_YV12:
			len1 = ALIGN_SIZE(width * height, ALIGNMENT);
			len2 = ALIGN_SIZE(halfwidth * halfheight, ALIGNMENT);
			len3 = ALIGN_SIZE(halfwidth * halfheight, ALIGNMENT);

			data[0] = buffer;
			data[1] = data[0] + len1;
			data[2] = data[1] + len2;
			linesize[0] = width;
			linesize[1] = halfwidth;
			linesize[2] = halfwidth;
			break;
		case FOURCC_NV12:
		case FOURCC_NV21:
			len1 = ALIGN_SIZE(width * height, ALIGNMENT);
			len2 = ALIGN_SIZE(halfwidth * 2 * halfheight, ALIGNMENT);

			data[0] = buffer;
			data[1] = data[0] + len1;
			linesize[0] = width;
			linesize[1] = halfwidth * 2;
			break;
		case FOURCC_Y800:
			len1 = ALIGN_SIZE(width * height, ALIGNMENT);
			data[0] = buffer;
			linesize[0] = width;
			break;
		case FOURCC_RGBA:
		case FOURCC_BGRA:
			len1 = ALIGN_SIZE(width * height * 4, ALIGNMENT);
			data[0] = buffer;
			linesize[0] = width * 4;
			break;
		case FOURCC_I444:
			int size = ALIGN_SIZE(width * height, ALIGNMENT);
			data[0] = buffer;
			data[1] = data[0] + size;
			data[2] = data[1] + size;
			linesize[0] = width;
			linesize[1] = width;
			linesize[2] = width;
			break;
		}
	}

	inline void CopyPlane(uint8_t* dst, int dst_stride,
		const uint8_t* src, int src_stride,
		int width, int height)
	{
		if (width == src_stride && width == dst_stride)
		{
			width *= height;
			height = 1;
		}
		for (int i = 0; i < height; ++i)
		{
			memcpy(dst, src, width);
			dst += dst_stride;
			src += src_stride;
		}
	}

	inline void CopyVideoFrame(uint8_t* dstData[QMaxSlice], uint32_t dst_linesize[QMaxSlice],
		const uint8_t* const srcData[QMaxSlice], uint32_t const src_linesize[QMaxSlice],
		int format, uint32_t width, uint32_t height)
	{
		int halfwidth = (width + 1) >> 1;
		int halfheight = (height + 1) >> 1;

		switch (format) {
		case FOURCC_Unknown:
			return;

		case FOURCC_I420:
		case FOURCC_YU12:
		case FOURCC_YV12:
			CopyPlane(dstData[0], dst_linesize[0], srcData[0], src_linesize[0], width, height);
			CopyPlane(dstData[1], dst_linesize[1], srcData[1], src_linesize[1], halfwidth, halfheight);
			CopyPlane(dstData[2], dst_linesize[2], srcData[2], src_linesize[2], halfwidth, halfheight);
			break;

		case FOURCC_NV12:
		case FOURCC_NV21:
			CopyPlane(dstData[0], dst_linesize[0], srcData[0], src_linesize[0], width, height);
			CopyPlane(dstData[1], dst_linesize[1], srcData[1], src_linesize[1], width, halfheight);
			break;

		case FOURCC_Y800:
			CopyPlane(dstData[0], dst_linesize[0], srcData[0], src_linesize[0], width, height);
			break;
		case FOURCC_RGBA:
		case FOURCC_BGRA:
			CopyPlane(dstData[0], dst_linesize[0], srcData[0], src_linesize[0], width * 4, height);
			break;

		case FOURCC_I444:
			CopyPlane(dstData[0], dst_linesize[0], srcData[0], src_linesize[0], width, height);
			CopyPlane(dstData[1], dst_linesize[1], srcData[1], src_linesize[1], width, height);
			CopyPlane(dstData[2], dst_linesize[2], srcData[2], src_linesize[2], width, height);
			break;
		}
	}
}

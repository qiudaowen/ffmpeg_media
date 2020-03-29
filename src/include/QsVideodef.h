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
    //FOURCC define
    /* yuyu		4:2:2 16bit, y-u-y-v, packed*/
    enum QeFourCC
    {
        FOURCC_YUY2 = MAKE_FOURCC('Y', 'U', 'Y', '2'),
        FOURCC_YUYV = MAKE_FOURCC('Y', 'U', 'Y', 'V'),
        /* uyvy		4:2:2 16bit, u-y-v-y, packed */
        FOURCC_UYVY = MAKE_FOURCC('U', 'Y', 'V', 'Y'),
        /* i420		y-u-v, planar */
        FOURCC_I420 = MAKE_FOURCC('I', '4', '2', '0'),
        FOURCC_I444 = MAKE_FOURCC('I', '4', '4', '4'),
        FOURCC_IYUV = MAKE_FOURCC('I', 'Y', 'U', 'V'),
        /* yv12		y-v-u, planar */
        FOURCC_YV12 = MAKE_FOURCC('Y', 'V', '1', '2'),

        FOURCC_NV12 = MAKE_FOURCC('N', 'V', '1', '2'),
        FOURCC_Y800 = MAKE_FOURCC('Y', '8', '0', '0'),

        FOURCC_HDYC = MAKE_FOURCC('H', 'D', 'Y', 'C'), //等同于FOURCC_UYVY，但颜色范围不同，先忽略不计

        FOURCC_RGB24 = MAKE_FOURCC('R', 'G', 'B', '3'),
        FOURCC_24BG = MAKE_FOURCC('2', '4', 'B', 'G'),
        FOURCC_RGB32 = MAKE_FOURCC('R', 'G', 'B', '4'),
        FOURCC_ARGB = MAKE_FOURCC('A', 'R', 'G', 'B'),
        FOURCC_ABGR = MAKE_FOURCC('A', 'B', 'G', 'R'),
        FOURCC_RGBA = MAKE_FOURCC('R', 'G', 'B', 'A'),
        FOURCC_BGRA = MAKE_FOURCC('B', 'G', 'R', 'A'),

        FOURCC_RGB565 = MAKE_FOURCC('R', 'G', 'B', 'P'),
        FOURCC_RGB555 = MAKE_FOURCC('R', 'G', 'B', 'O'),

        FOURCC_MJPG = MAKE_FOURCC('M', 'J', 'P', 'G'),
        FOURCC_H264 = MAKE_FOURCC('H', '2', '6', '4'),

        FOURCC_ANY = -1,
        FOURCC_Unknown = 0,
    };
}

namespace video {
	enum video_format {
		VIDEO_FORMAT_NONE = FOURCC_Unknown,

		/* planar 420 format */
		VIDEO_FORMAT_I420 = FOURCC_I420, /* three-plane */
		VIDEO_FORMAT_NV12 = FOURCC_NV12, /* two-plane, luma and packed chroma */

	    /* packed 422 formats */
		VIDEO_FORMAT_YVYU = FOURCC_YUYV,
		VIDEO_FORMAT_YUY2 = FOURCC_YUY2, /* YUYV */
		VIDEO_FORMAT_UYVY = FOURCC_UYVY,

		/* packed uncompressed formats */
		VIDEO_FORMAT_BGRX = FOURCC_RGB32,
		VIDEO_FORMAT_Y800 = FOURCC_Y800, /* grayscale */

		/* planar 4:4:4 */
		VIDEO_FORMAT_I444 = FOURCC_I444,
	};

	inline uint32_t CalBufNeedSize(int width, int height, int format)
	{
        int halfwidth = (width + 1) >> 1;
        int halfheight = (height + 1) >> 1;
		int len1 = 0;
		int len2 = 0;
		int len3 = 0;
		switch (format) {
		case VIDEO_FORMAT_NONE:
			return 0;
        
		case VIDEO_FORMAT_I420:
			len1 = ALIGN_SIZE(width * height, ALIGNMENT);
			len2 = ALIGN_SIZE(halfwidth * halfheight, ALIGNMENT);
			len3 = ALIGN_SIZE(halfwidth * halfheight, ALIGNMENT);
			return len1 + len2 + len3;
			break;

		case VIDEO_FORMAT_NV12:
			len1 = ALIGN_SIZE(width * height, ALIGNMENT);
			len2 = ALIGN_SIZE(halfwidth * halfheight * 2, ALIGNMENT);
			return len1 + len2;
			break;

		case VIDEO_FORMAT_Y800:
			len1 = ALIGN_SIZE(width * height, ALIGNMENT);
			return len1;
			break;

		case VIDEO_FORMAT_YVYU:
		case VIDEO_FORMAT_YUY2:
		case VIDEO_FORMAT_UYVY:
			len1 = ALIGN_SIZE(width * height * 2, ALIGNMENT);
			return len1;
			break;

		case VIDEO_FORMAT_BGRX:
			len1 = ALIGN_SIZE(width * height * 4, ALIGNMENT);
			return len1;
			break;

		case VIDEO_FORMAT_I444:
			len1 = ALIGN_SIZE(width * height, ALIGNMENT) * 3;
			return len1;
			break;
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
		case VIDEO_FORMAT_NONE:
			break;

		case VIDEO_FORMAT_I420:
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

		case VIDEO_FORMAT_NV12:
			 len1 = ALIGN_SIZE(width * height, ALIGNMENT);
			 len2 = ALIGN_SIZE(halfwidth * halfheight * 2, ALIGNMENT);

			data[0] = buffer;
			data[1] = data[0] + len1;
			linesize[0] = width;
			linesize[1] = width;
			break;

		case VIDEO_FORMAT_Y800:
			 len1 = ALIGN_SIZE(width * height, ALIGNMENT);
			data[0] = buffer;
			linesize[0] = width;
			break;

		case VIDEO_FORMAT_YVYU:
		case VIDEO_FORMAT_YUY2:
		case VIDEO_FORMAT_UYVY:
			 len1 = ALIGN_SIZE(width * height * 2, ALIGNMENT);
			data[0] = buffer;
			linesize[0] = width * 2;
			break;

		case VIDEO_FORMAT_BGRX:
			 len1 = ALIGN_SIZE(width * height * 4, ALIGNMENT);
			data[0] = buffer;
			linesize[0] = width * 4;
			break;

		case VIDEO_FORMAT_I444:
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
            memcpy(dstData[0], srcData[0], width * height);
        else
        {
            for (int i = 0; i < height; ++i)
            {
                memcpy(dst, src, width);
                dst += dst_stride;
                src += src_stride;
            }
        }
    }

	inline void CopyVideoFrame(uint8_t* dstData[QMaxSlice], uint32_t dst_linesize[QMaxSlice],
		uint8_t* srcData[QMaxSlice], uint32_t src_linesize[QMaxSlice],
		int format, uint32_t width, uint32_t height)
	{
        int halfwidth = (width + 1) >> 1;
        int halfheight = (height + 1) >> 1;

		switch (format) {
		case VIDEO_FORMAT_NONE:
			return;

		case VIDEO_FORMAT_I420:
            CopyPlane(dstData[0], dst_linesize[0], srcData[0], src_linesize[0], width, height);
            CopyPlane(dstData[1], dst_linesize[1], srcData[1], src_linesize[1], halfwidth, halfheight);
            CopyPlane(dstData[2], dst_linesize[2], srcData[2], src_linesize[2], halfwidth, halfheight);
			break;

		case VIDEO_FORMAT_NV12:
            CopyPlane(dstData[0], dst_linesize[0], srcData[0], src_linesize[0], width, height);
            CopyPlane(dstData[0], dst_linesize[1], srcData[1], src_linesize[1], width, halfheight);
			break;

		case VIDEO_FORMAT_Y800:
		case VIDEO_FORMAT_YVYU:
		case VIDEO_FORMAT_YUY2:
		case VIDEO_FORMAT_UYVY:
		case VIDEO_FORMAT_BGRX:
            CopyPlane(dstData[0], dst_linesize[0], srcData[0], src_linesize[0], width, height);
			break;

		case VIDEO_FORMAT_I444:
            CopyPlane(dstData[0], dst_linesize[0], srcData[0], src_linesize[0], width, height);
            CopyPlane(dstData[1], dst_linesize[1], srcData[1], src_linesize[1], width, height);
            CopyPlane(dstData[2], dst_linesize[2], srcData[2], src_linesize[2], width, height);
			break;
		}
	}
}

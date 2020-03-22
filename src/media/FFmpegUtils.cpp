#include "FFmpegUtils.h"
#include "videodef.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#ifdef __cplusplus
};
#endif

extern "C" unsigned int fourccToPixelFormat(int);
extern "C" unsigned int pixelFormatToFourcc(int);

void FFmpegUtils::init()
{
    av_register_all();
}

static AVRational gContextBaseTime = { 1, AV_TIME_BASE };
const AVRational* FFmpegUtils::contextBaseTime()
{
	return &gContextBaseTime;
}

int FFmpegUtils::fourccToFFmpegFormat(int fourcc)
{
    return fourccToPixelFormat(fourcc);
}

int FFmpegUtils::ffmpegFormatToFourcc(int pixelFormat)
{
    return pixelFormatToFourcc(pixelFormat);
}

AVPacketPtr FFmpegUtils::allocAVPacket()
{
	return AVPacketPtr(av_packet_alloc(), [](AVPacket* pPkt) {
		av_packet_free(&pPkt);
	});
}

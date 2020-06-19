#include "FFmpegUtils.h"
#include <chrono>
#include "QmMacro.h"

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

QmRunBeforeMain(av_register_all);

void FFmpegUtils::init()
{
    //av_register_all();
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

int FFmpegUtils::ToFFmpegAudioFormat(QeSampleFormat iFormat)
{
    return iFormat;
}

QeSampleFormat FFmpegUtils::FromFFmpegAudioFormat(int iFFmpegFormat)
{
    return (QeSampleFormat)iFFmpegFormat;
}

AVPacketPtr FFmpegUtils::allocAVPacket()
{
	return AVPacketPtr(av_packet_alloc(), [](AVPacket* pPkt) {
		av_packet_free(&pPkt);
	});
}

int FFmpegUtils::currentMilliSecsSinceEpoch()
{
	using namespace std::chrono;
	auto timePoint = steady_clock::now();
	auto cur = timePoint.time_since_epoch();
	milliseconds ms = duration_cast<milliseconds>(cur);
	return (int)ms.count();
}

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

int FFmpegUtils::toFFmpegAudioFormat(QeSampleFormat iFormat)
{
	return iFormat;
}

QeSampleFormat FFmpegUtils::fromFFmpegAudioFormat(int iFFmpegFormat)
{
	return (QeSampleFormat)iFFmpegFormat;
}

AVPacketPtr FFmpegUtils::allocAVPacket()
{
	return AVPacketPtr(av_packet_alloc(), [](AVPacket* pPkt) {
		av_packet_free(&pPkt);
		});
}

int64_t FFmpegUtils::currentMilliSecsSinceEpoch()
{
	using namespace std::chrono;
	auto timePoint = steady_clock::now();
	auto cur = timePoint.time_since_epoch();
	milliseconds ms = duration_cast<milliseconds>(cur);
	return (int)ms.count();
}

int FFmpegUtils::toFFmpegEncoderID(const char* name)
{
	auto codec = avcodec_find_encoder_by_name(name);
	return codec ? codec->id : -1;
}

void FFmpegUtils::enumCodec(QeCodecFlag flags, std::function<void(int codecID, int type, const char* name)> cb)
{
	void* opaque = 0;
	while (auto codec = av_codec_iterate(&opaque))
	{
		if (codec->type == AVMEDIA_TYPE_VIDEO && (flags & kVideoCodec))
		{
			if ((av_codec_is_encoder(codec) && (flags & kVideoEncoder))
				|| (av_codec_is_decoder(codec) && (flags & kVideoDecoder)))
				cb(codec->id, codec->type, codec->name);
		}
		else if (codec->type == AVMEDIA_TYPE_AUDIO && (flags & kAudioCodec))
		{
			if ((av_codec_is_encoder(codec) && (flags & kAudioEncoder))
				|| (av_codec_is_decoder(codec) && (flags & kAudioDecoder)))
				cb(codec->id, codec->type, codec->name);
		}

	}
}

int FFmpegUtils::toFFmpegDecoderID(const char* name)
{
	auto codec = avcodec_find_decoder_by_name(name);
	return codec ? codec->id : -1;
}

void FFmpegUtils::enumSampleFormat(int codecID, std::function<void(int ffmpegFormat)> cb)
{
	auto codec = avcodec_find_decoder((AVCodecID)codecID);
	if (codec->sample_fmts)
	{
		int index = 0;
		for (; codec->sample_fmts[index] != AV_SAMPLE_FMT_NONE; )
		{
			cb(codec->sample_fmts[index]);
			++index;
		}
	}
}
void FFmpegUtils::enumSampleRate(int codecID, std::function<void(int sampleRate)> cb)
{
	auto codec = avcodec_find_decoder((AVCodecID)codecID);
	if (codec->supported_samplerates)
	{
		int index = 0;
		for (; codec->supported_samplerates[index]; )
		{
			cb(codec->supported_samplerates[index]);
			++index;
		}
	}
}

int64_t FFmpegUtils::recommendBitRate(int w, int h, int fps)
{
	//TODO.
	const uint64_t defalut1080P_60FPS_BitRate = 1024 * 1024 * 20;
	return defalut1080P_60FPS_BitRate * (w * h * fps) / (1920 * 1080 * 60);
}

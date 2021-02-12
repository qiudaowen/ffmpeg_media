#pragma once

#include "media_global.h"
#include "QsMediaInfo.h"
#include "QsAudiodef.h"
#include "QsVideodef.h"
#include <functional>

#define QmBaseTimeToSecondTime(value, base) ( (value) * double(base.num) )/(base.den)
#define QmSecondTimeToBaseTime(value, base) (int64_t)(((value) * double(base.den) )/(base.num))
#define QmBaseTimeToMSTime(value, base) int(1000 * ((value) * double(base.num) )/(base.den))
#define QmMSTimeToBaseTime(value, base) (int64_t)(((value) * double(base.den) )/(base.num * 1000))

#define QmToFFmpegAudioFormat(value) ((AVSampleFormat)FFmpegUtils::toFFmpegAudioFormat(value))

struct AVRational;

class MEDIA_API FFmpegUtils
{
public:
    static void init();
	static const AVRational* contextBaseTime();
    static int fourccToFFmpegFormat(int);
    static int ffmpegFormatToFourcc(int);
    static int toFFmpegAudioFormat(QeSampleFormat iFormat);
    static QeSampleFormat fromFFmpegAudioFormat(int iFFmpegFormat);

    static int toFFmpegEncoderID(const char* name);
    static int toFFmpegDecoderID(const char* name);

    static void enumSampleFormat(int codecID, std::function<void(int ffmpegFormat)>);
    static void enumSampleRate(int codecID, std::function<void(int sampleRate)>);

    //ffmpeg AVCodecID, AVMediaType type,
    enum QeCodecFlag
    {
        kVideoEncoder = 1 << 0,
        kVideoDecoder = 1 << 1,
        kAudioEncoder = 1 << 2,
        kAudioDecoder = 1 << 3,

        kAllEncoder = kAudioEncoder | kVideoEncoder,
        kAllDecoder = kAudioDecoder | kVideoDecoder,
        kVideoCodec = kVideoEncoder | kVideoDecoder,
        kAudioCodec = kAudioEncoder | kAudioDecoder,
        kAllCodec = kVideoCodec | kAudioCodec,
    };
    static void enumCodec(QeCodecFlag flags, std::function<void(int codecID, int type, const char* name)>);

    static int64_t recommendBitRate(int w, int h, int fps);

	static AVPacketPtr allocAVPacket();
	static int64_t currentMilliSecsSinceEpoch();
};
 

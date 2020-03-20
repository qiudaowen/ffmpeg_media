#include "FFmpegUtils.h"
#include "videodef.h"

extern "C" unsigned int fourccToPixelFormat(int);
extern "C" unsigned int pixelFormatToFourcc(int);

void FFmpegUtils::init()
{
    av_register_all();
}

int FFmpegUtils::fourccToFFmpegFormat(int fourcc)
{
    return fourccToPixelFormat(fourcc);
}

int FFmpegUtils::ffmpegFormatToFourcc(int pixelFormat)
{
    return pixelFormatToFourcc(pixelFormat);
}

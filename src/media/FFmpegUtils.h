#pragma once

#define QmBaseTimeToSecondTime(value, base) (value * double(base.num) )/(base.den)
#define QmSecondTimeToBaseTime(value, base) (value * double(base.den) )/(base.num)
#define QmBaseTimeToMSTime(value, base) int(1000 * (value * double(base.num) )/(base.den))
#define QmMSTimeToBaseTime(value, base) (value * double(base.den) )/(base.num * 1000)

class FFmpegUtils
{
public:
    static void init();
    static int fourccToFFmpegFormat(int);
    static int ffmpegFormatToFourcc(int);
};

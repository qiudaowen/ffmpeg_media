#pragma once

#include <stdint.h>

enum QeSampleFormat
{
	kSampleFormatNone = -1,
	kSampleFormatU8 = 0,
	kSampleFormatS16,
	kSampleFormatS32,
	kSampleFormatFloat,
	kSampleFormatDouble,

	kSampleFormatU8P,
	kSampleFormatS16P,
	kSampleFormatS32P,
	kSampleFormatFloatP,
	kSampleFormatDoubleP,

    kSampleFormatS64,
    kSampleFormatS64P,        ///< signed 64 bits, planar
};
struct QsAudioParam
{
    int codecID = 0;
    int bitRate = 0;
	int sampleRate = 0;
	QeSampleFormat sampleFormat = kSampleFormatNone;
	int nChannels = 0;
	bool operator==(const QsAudioParam& para) const
	{
		return sampleRate == para.sampleRate
			&& sampleFormat == para.sampleFormat
			&& nChannels == para.nChannels;
	}
	bool operator!=(const QsAudioParam& para) const
	{
		return !(*this == para);
	}
};

#define QmAudioPlanes 8
struct QsAudioData
{
    int sampleRate;
    QeSampleFormat sampleFormat;
    int nChannels;

    const uint8_t* data[QmAudioPlanes];
    uint32_t nSamples;

    uint64_t timestamp;
};
inline int getBytesPerSample(QeSampleFormat format)
{
    switch (format)
    {
    case kSampleFormatU8P:
    case kSampleFormatU8:
        return 1;
    case kSampleFormatS16:
    case kSampleFormatS16P:
        return 2;
    case kSampleFormatS32:
    case kSampleFormatS32P:
        return 4;
    case kSampleFormatFloat:
    case kSampleFormatFloatP:
        return 4;
    case kSampleFormatDouble:
    case kSampleFormatDoubleP:
        return 8;
    }
    return 0;
}
inline int getAudioBufferSize(QeSampleFormat format, int nChannels, int nb_samples)
{
	return getBytesPerSample(format) * nChannels * nb_samples;
}
inline QeSampleFormat toNonPlanarFormat(QeSampleFormat format)
{
    switch (format)
    {
    case kSampleFormatU8:
    case kSampleFormatU8P:
        return kSampleFormatU8;
    case kSampleFormatS16:
    case kSampleFormatS16P:
        return kSampleFormatS16;
    case kSampleFormatS32:
    case kSampleFormatS32P:
        return kSampleFormatS32;
    case kSampleFormatFloat:
    case kSampleFormatFloatP:
        return kSampleFormatFloat;
    case kSampleFormatDouble:
    case kSampleFormatDoubleP:
        return kSampleFormatDouble;
    }
    return kSampleFormatNone;
}
inline QeSampleFormat toPlanarFormat(QeSampleFormat format)
{
    switch (format)
    {
    case kSampleFormatU8:
    case kSampleFormatU8P:
        return kSampleFormatU8P;
    case kSampleFormatS16:
    case kSampleFormatS16P:
        return kSampleFormatS16P;
    case kSampleFormatS32:
    case kSampleFormatS32P:
        return kSampleFormatS32P;
    case kSampleFormatFloat:
    case kSampleFormatFloatP:
        return kSampleFormatFloatP;
    case kSampleFormatDouble:
    case kSampleFormatDoubleP:
        return kSampleFormatDoubleP;
    }
    return kSampleFormatNone;
}
inline bool IsAudioPlanarFormat(QeSampleFormat iFormat)
{
    switch (iFormat)
    {
    case kSampleFormatU8:
    case kSampleFormatS16:
    case kSampleFormatS32:
    case kSampleFormatFloat:
    case kSampleFormatDouble:
        return false;
    }
    return true;
}

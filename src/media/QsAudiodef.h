#ifndef QS_AUDIO_PARA_H
#define QS_AUDIO_PARA_H

enum QeSampleFormat
{
	eSampleFormatNone = -1,
	eSampleFormatU8 = 0,
	eSampleFormatS16,
	eSampleFormatS32,
	eSampleFormatFloat,
	eSampleFormatDouble,

	eSampleFormatU8P,
	eSampleFormatS16P,
	eSampleFormatS32P,
	eSampleFormatFloatP,
	eSampleFormatDoubleP,
};
struct QsAudioPara
{
	int iSamplingFreq = 0;
	QeSampleFormat eSample_fmt = eSampleFormatNone;
	int nChannel = 0;
	bool operator==(const QsAudioPara& para) const
	{
		return iSamplingFreq == para.iSamplingFreq
			&& eSample_fmt == para.eSample_fmt
			&& nChannel == para.nChannel;
	}
	bool operator!=(const QsAudioPara& para) const
	{
		return !(*this == para);
	}
};
inline int getBytesPerSample(QeSampleFormat format)
{
    switch (format)
    {
    case eSampleFormatU8P:
    case eSampleFormatU8:
        return 1;
    case eSampleFormatS16:
    case eSampleFormatS16P:
        return 2;
    case eSampleFormatS32:
    case eSampleFormatS32P:
        return 4;
    case eSampleFormatFloat:
    case eSampleFormatFloatP:
        return 4;
    case eSampleFormatDouble:
    case eSampleFormatDoubleP:
        return 8;
    }
    return 0;
}
inline int getAudioBufferSize(const QsAudioPara& para, int nb_samples)
{
	return getBytesPerSample(para.eSample_fmt) * para.nChannel * nb_samples;
}
inline QeSampleFormat toNonPlanarFormat(QeSampleFormat format)
{
    switch (format)
    {
    case eSampleFormatU8:
    case eSampleFormatU8P:
        return eSampleFormatU8;
    case eSampleFormatS16:
    case eSampleFormatS16P:
        return eSampleFormatS16;
    case eSampleFormatS32:
    case eSampleFormatS32P:
        return eSampleFormatS32;
    case eSampleFormatFloat:
    case eSampleFormatFloatP:
        return eSampleFormatFloat;
    case eSampleFormatDouble:
    case eSampleFormatDoubleP:
        return eSampleFormatDouble;
    }
    return eSampleFormatNone;
}
inline QeSampleFormat toPlanarFormat(QeSampleFormat format)
{
    switch (format)
    {
    case eSampleFormatU8:
    case eSampleFormatU8P:
        return eSampleFormatU8P;
    case eSampleFormatS16:
    case eSampleFormatS16P:
        return eSampleFormatS16P;
    case eSampleFormatS32:
    case eSampleFormatS32P:
        return eSampleFormatS32P;
    case eSampleFormatFloat:
    case eSampleFormatFloatP:
        return eSampleFormatFloatP;
    case eSampleFormatDouble:
    case eSampleFormatDoubleP:
        return eSampleFormatDoubleP;
    }
    return eSampleFormatNone;
}
inline bool IsAudioPlanarFormat(QeSampleFormat iFormat)
{
    switch (iFormat)
    {
    case eSampleFormatU8:
    case eSampleFormatS16:
    case eSampleFormatS32:
    case eSampleFormatFloat:
    case eSampleFormatDouble:
        return false;
    }
    return true;
}
#endif

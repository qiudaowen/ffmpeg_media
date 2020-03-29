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
	int iSamplingFreq;
	QeSampleFormat eSample_fmt;
	int nChannel;
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

int ToFFmpegAudioFormat(QeSampleFormat iFormat);
QeSampleFormat FromFFmpegAudioFormat(int iFFmpegFormat);

void AudioParaToFFmpegPara(const QsAudioPara& para, int& iSampleRate, int& iSampleFormat, int& iChannelLayout);
void FFmpegParaToAudioPara(QsAudioPara& para, int iSampleRate, int iSampleFormat, int iChannelLayout);

bool IsAudioPlanarFormat(QeSampleFormat iFormat);
int CalAudioBufferSize(const QsAudioPara& para, int nb_samples);
int CalAudioSample(const QsAudioPara& para, int nBytesCount);
int CalAudioSample(int iSrcSamples, int iSrcRate, int iDestRate);
bool FindBestAudioPara(int iFFMPegCodec, QsAudioPara& para);

#endif

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>


void AudioParaToFFmpegPara(const QsAudioPara& para, int& iSampleRate, int& iFFmpegFormat, int& iChannelLayout)
{
    iSampleRate = para.iSamplingFreq;
    iFFmpegFormat = ToFFmpegAudioFormat(para.eSample_fmt);
	iChannelLayout = av_get_default_channel_layout(para.nChannel);
}

void FFmpegParaToAudioPara(QsAudioPara& para, int iSampleRate, int iFFmpegFormat, int iChannelLayout)
{
    para.iSamplingFreq = iSampleRate;
    para.eSample_fmt = FromFFmpegAudioFormat(iFFmpegFormat);
    para.nChannel = av_get_channel_layout_nb_channels(iChannelLayout);
}

int CalAudioBufferSize(const QsAudioPara& para, int nb_samples)
{
    return av_samples_get_buffer_size(NULL, para.nChannel, nb_samples, (AVSampleFormat)ToFFmpegAudioFormat(para.eSample_fmt), 1);
}

int CalAudioSample(const QsAudioPara& para, int nBytesCount)
{
	return nBytesCount/(av_get_bytes_per_sample((AVSampleFormat)ToFFmpegAudioFormat(para.eSample_fmt)) * 2);
}

int CalAudioSample(int iSrcSamples, int iSrcRate, int iDestRate)
{
	return av_rescale_rnd(iSrcSamples, iDestRate, iSrcRate, AV_ROUND_UP);
}

bool FindBestAudioPara(int iFFMPegCodec, QsAudioPara& para)
{
	AVCodec* pAudioCodec = avcodec_find_encoder(AVCodecID(iFFMPegCodec));
	if (pAudioCodec == NULL)
		return false;

	int m_iSampleRate;
	int m_iSampleFormat;
	int m_iChannelLayout;
	AudioParaToFFmpegPara(para, m_iSampleRate, m_iSampleFormat, m_iChannelLayout);

	if (pAudioCodec->supported_samplerates)
	{
		int iSel = 0;
		int nDiff = 0x7fffffff;
		for (int i = 0; pAudioCodec->supported_samplerates[i]; i++)
		{
			int iCurDiff = std::abs(para.iSamplingFreq - pAudioCodec->supported_samplerates[i]);
			if (iCurDiff == 0)
			{
				iSel = -1;
				break;
			}
			if (iCurDiff < nDiff)
			{
				iSel = i;
				nDiff = iCurDiff;
			}
		}
		if (iSel != -1)
			para.iSamplingFreq = pAudioCodec->supported_samplerates[iSel];
	}
	if (pAudioCodec->sample_fmts)
	{
		bool bFind = false;
		for (int i = 0; pAudioCodec->sample_fmts[i] != -1; ++i)
		{
			if (pAudioCodec->sample_fmts[i] == (AVSampleFormat)m_iSampleFormat)
			{
				bFind = true;
				break;
			}
		}
		if (!bFind)
			para.eSample_fmt = FromFFmpegAudioFormat(pAudioCodec->sample_fmts[0]);
	}

	
	if (pAudioCodec->channel_layouts)
	{
		int iSel = 0;
		int nDiffChannel = 0x7ffffff;
		for (int i = 0; pAudioCodec->channel_layouts[i]; i++)
		{
			int iCurDiff = para.nChannel - av_get_channel_layout_nb_channels(pAudioCodec->channel_layouts[i]);
			if (iCurDiff == 0)
			{
				iSel = -1;
				break;
			}
			if (iCurDiff > 0 && iCurDiff < nDiffChannel)
			{
				iSel = i;
				nDiffChannel = iCurDiff;
			}
		}
		if (iSel != -1)
			para.nChannel = av_get_channel_layout_nb_channels(pAudioCodec->channel_layouts[iSel]);
	}
	return true;
}

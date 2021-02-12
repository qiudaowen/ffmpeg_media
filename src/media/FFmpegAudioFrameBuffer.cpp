#include "FFmpegAudioFrameBuffer.h"
#include "FFmpegUtils.h"
#include <algorithm>

extern "C" {
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

FFmpegAudioFrameBuffer::FFmpegAudioFrameBuffer(const QsAudioParam& param, int nSampleBuffer)
	: m_param(param)
	, m_nSampleFrame(nSampleBuffer)
	, m_nSampleRemain(0)
{

}
FFmpegAudioFrameBuffer::~FFmpegAudioFrameBuffer()
{

}

int FFmpegAudioFrameBuffer::push(uint8_t* const* src, int nSrcSample, std::function<int(AVFrameRef frame)> cb)
{
	//flush.
	if (nSrcSample == 0 && src == nullptr)
	{
		if (m_buffer) {
			m_buffer->nb_samples = m_nSampleRemain;
		}
		return cb(m_buffer);
	}

	int iRet = 0;
	int srcOffset = 0;
	int dstOffset = m_nSampleRemain;
	while (nSrcSample > 0) {
		if (!m_buffer) {
			m_buffer = AVFrameRef::allocAudioFrame(m_nSampleFrame, m_param.sampleRate, m_param.nChannels, m_param.sampleFormat);
			dstOffset = 0;
		}
		int nSamplesCopy = std::min(m_nSampleFrame - dstOffset, nSrcSample);
		av_samples_copy(m_buffer->data, (uint8_t* const*)src
			, dstOffset, srcOffset
			, nSamplesCopy, m_param.nChannels, QmToFFmpegAudioFormat(m_param.sampleFormat));
		
		srcOffset += nSamplesCopy;
		dstOffset += nSamplesCopy;

		nSrcSample -= nSamplesCopy;
		if (dstOffset == m_nSampleFrame)
		{
			iRet = cb(m_buffer);
			m_buffer = AVFrameRef();
			dstOffset = 0;
		}
	}
	m_nSampleRemain = dstOffset;

	return iRet;
}
#include "FFmpegAudioEncoder.h"
#include "QsAudioDef.h"
#include "FFmpegUtils.h"
#include "AVFrameRef.h"
#include "QcAudioTransformat.h"
#include "FFmpegAudioFrameBuffer.h"

extern "C" {
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

FFmpegAudioEncoder::FFmpegAudioEncoder()
	: m_pCodecCtx(NULL)
	, m_pEncodeCodec(NULL)
	, m_startPts(AV_NOPTS_VALUE)
{
	setCodecTimeBase({ 1,1000 });
}

FFmpegAudioEncoder::~FFmpegAudioEncoder()
{
	close();
}

void FFmpegAudioEncoder::setCodecTimeBase(const QsTimeBase& timebase)
{
	m_timebase = timebase;
}

void FFmpegAudioEncoder::setEncodeParam(const QsAudioParam& param)
{
	m_encodeParam = param;
}

bool FFmpegAudioEncoder::paramValid() const
{
	return m_encodeParam.bitRate > 0;
}

bool FFmpegAudioEncoder::open()
{
	close();
	bool bRet = false;
	do
	{
		const QsAudioParam& para = m_encodeParam;

		m_pEncodeCodec = avcodec_find_encoder((AVCodecID)para.codecID);
		if (m_pEncodeCodec == NULL)
		{
			break;
		}

		m_pCodecCtx = avcodec_alloc_context3(m_pEncodeCodec);
		if (m_pCodecCtx == NULL)
		{
			break;
		}

		m_pCodecCtx->time_base = { m_timebase.num, m_timebase.den };
		m_pCodecCtx->bit_rate = para.bitRate;
		m_pCodecCtx->sample_rate = para.sampleRate;
		m_pCodecCtx->sample_fmt = QmToFFmpegAudioFormat(para.sampleFormat);
		m_pCodecCtx->channel_layout = av_get_default_channel_layout(para.nChannels);
		m_pCodecCtx->channels = av_get_channel_layout_nb_channels(m_pCodecCtx->channel_layout);

		// av_opt_set(m_pCodecCtx->priv_data, "tune", "zerolatency", 0); //no delay encode.
		if (avcodec_open2(m_pCodecCtx, m_pEncodeCodec, NULL) < 0)
			break;

		if (!(m_pEncodeCodec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE))
		{
			m_frameBuffer = new FFmpegAudioFrameBuffer(para, m_pCodecCtx->frame_size);
		}

		bRet = true;
	} while (0);
	return bRet;
}

void FFmpegAudioEncoder::close()
{
	if (m_pCodecCtx)
	{
		if (m_pEncodeCodec)
		{
			avcodec_close(m_pCodecCtx);
			m_pEncodeCodec = NULL;
		}
		av_free(m_pCodecCtx);
		m_pCodecCtx = NULL;
	}
	if (m_transformat)
	{
		delete m_transformat;
		m_transformat = nullptr;
	}
	if (m_frameBuffer)
	{
		delete m_frameBuffer;
		m_frameBuffer = nullptr;
	}
	m_startPts = AV_NOPTS_VALUE;
	m_nSampleCount = 0;
}

AVFrameRef FFmpegAudioEncoder::transformat(AVFrameRef frame)
{
	//flush buffer.
	if (!frame)
	{
		AVFrameRef ret;
		if (m_transformat)
			m_transformat->transformat(nullptr, 0, ret);
		return ret;
	}

	if (frame.sampleCount() != m_pCodecCtx->sample_rate
		|| frame.format() != m_pCodecCtx->sample_fmt
		|| frame.channelCount() != m_pCodecCtx->channels) {
		QsAudioParam srcParam;
		srcParam.sampleRate = frame.sampleRate();
		srcParam.sampleFormat = FFmpegUtils::fromFFmpegAudioFormat(frame.format());
		srcParam.nChannels = frame.channelCount();
		if (m_transformat == nullptr) {
			m_transformat = new QcAudioTransformat();
		}
		if (m_transformat->srcPara() != srcParam)
		{
			QsAudioParam dstParam;
			dstParam.sampleRate = m_pCodecCtx->sample_rate;
			dstParam.sampleFormat = FFmpegUtils::fromFFmpegAudioFormat(m_pCodecCtx->sample_fmt);
			dstParam.nChannels = m_pCodecCtx->channels;
			m_transformat->init(srcParam, dstParam);
		}
		AVFrameRef ret;
		if (!m_transformat->transformat(frame->data, frame->nb_samples, ret))
			return AVFrameRef();
		ret->pts = frame->pts;
		frame = ret;
	}
	return frame;
}

int FFmpegAudioEncoder::encode(AVFrameRef frame)
{
	bool bFlush = false;
	if (frame) {
		if (m_startPts == AV_NOPTS_VALUE)
			m_startPts = 0;
		frame = transformat(frame);
	}
	else {
		frame = transformat(frame);
		bFlush = true;
	}
	int iRet = 0;
	if (m_frameBuffer)
	{
		uint8_t* const* src = frame ? frame->data : nullptr;
		int nSamples = frame ? frame->nb_samples : 0;
		iRet = m_frameBuffer->push(src, nSamples, [this](AVFrameRef frame) {
			double fSecond = m_nSampleCount / (double)m_pCodecCtx->sample_rate;
			frame->pts = QmSecondTimeToBaseTime(fSecond, m_pCodecCtx->time_base);
			m_nSampleCount += frame->nb_samples;
			return avcodec_send_frame(m_pCodecCtx, frame);
			});
	}
	else {
		if (frame) {
			double fSecond = m_nSampleCount / (double)m_pCodecCtx->sample_rate;
			frame->pts = QmSecondTimeToBaseTime(fSecond, m_pCodecCtx->time_base);
			m_nSampleCount += frame->nb_samples;
		}
		iRet = avcodec_send_frame(m_pCodecCtx, frame);
	}

	if (bFlush)
	{
		//flush again.
		iRet = avcodec_send_frame(m_pCodecCtx, nullptr);
	}
	return iRet;
}

int FFmpegAudioEncoder::recv(AVPacketPtr& pkt)
{
	AVPacketPtr newPkt = FFmpegUtils::allocAVPacket();
	int ret = avcodec_receive_packet(m_pCodecCtx, newPkt.get());
	switch (ret)
	{
	case AVERROR_EOF:
		return kEOF;
	case AVERROR(EAGAIN):
		return kAgain;
	case 0:
		pkt = newPkt;
		return kOk;
	}
	return kOtherError;

}

void FFmpegAudioEncoder::flush()
{
	if (m_pCodecCtx)
		avcodec_flush_buffers(m_pCodecCtx);
}

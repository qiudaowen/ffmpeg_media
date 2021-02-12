#include "FFmpegVideoEncoder.h"
#include "QsVideoDef.h"
#include "FFmpegUtils.h"
#include "AVFrameRef.h"
#include "FFmpegVideoTransformat.h"

extern "C" {
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

FFmpegVideoEncoder::FFmpegVideoEncoder()
	: m_pCodecCtx(NULL)
	, m_pEncodeCodec(NULL)
	, m_startPts(AV_NOPTS_VALUE)
{
	setCodecTimeBase({ 1,1000 });
}

FFmpegVideoEncoder::~FFmpegVideoEncoder()
{
	close();
}

void FFmpegVideoEncoder::setCodecTimeBase(const QsTimeBase& timebase)
{
	m_timebase = timebase;
}

void FFmpegVideoEncoder::setEncodeParam(const QsVideoParam& para)
{
	m_encodeParam = para;
}

bool FFmpegVideoEncoder::paramValid() const
{
	return m_encodeParam.width > 0 && m_encodeParam.height > 0;
}

int FFmpegVideoEncoder::getExtradata(uint8_t*& extradata) const
{
	if (m_pCodecCtx == nullptr)
		return 0;

	extradata = m_pCodecCtx->extradata;
	return m_pCodecCtx->extradata_size;
}

bool FFmpegVideoEncoder::open()
{
	close();
	bool bRet = false;
	do
	{
		const QsVideoParam& para = m_encodeParam;
		if (para.width == 0 || para.height == 0)
			break;

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
		m_pCodecCtx->width = para.width;
		m_pCodecCtx->height = para.height;
		m_pCodecCtx->gop_size = para.fps * 4;
		m_pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

		m_pCodecCtx->bit_rate = para.bitRate;
		//m_pCodecCtx->rc_min_rate = m_pCodecCtx->bit_rate;
		//m_pCodecCtx->rc_max_rate = m_pCodecCtx->bit_rate;
		//m_pCodecCtx->bit_rate_tolerance = m_pCodecCtx->bit_rate;
		//m_pCodecCtx->rc_buffer_size = m_pCodecCtx->bit_rate;
		//m_pCodecCtx->rc_initial_buffer_occupancy = m_pCodecCtx->rc_buffer_size * 3 / 4;
		//m_pCodecCtx->rc_buffer_aggressivity = (float)1.0;
		//m_pCodecCtx->rc_initial_cplx = 0.5;

		//m_pCodecCtx->qcompress = (float)1.0;
		//m_pCodecCtx->qmin = para.iQMin;
		//m_pCodecCtx->qmax = para.iQMax;

		m_pCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		// av_opt_set(m_pCodecCtx->priv_data, "tune", "zerolatency", 0); //no delay encode.
		if (avcodec_open2(m_pCodecCtx, m_pEncodeCodec, NULL) < 0)
			break;

		bRet = true;
	} while (0);
	return bRet;
}

void FFmpegVideoEncoder::close()
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
	m_startPts = AV_NOPTS_VALUE;
}

AVFrameRef FFmpegVideoEncoder::transformat(AVFrameRef frame)
{
	if (frame.width() != m_pCodecCtx->width
		|| frame.height() != m_pCodecCtx->height
		|| frame.format() != m_pCodecCtx->pix_fmt)
	{
		AVFrameRef ret = AVFrameRef::allocFrame(m_pCodecCtx->width, m_pCodecCtx->height, m_pCodecCtx->pix_fmt, frame->pts);
		if (m_transformat == nullptr) {
			m_transformat = new FFmpegVideoTransformat();
		}
		m_transformat->transformat(frame.width(), frame.height(), frame.format(), frame.data(), frame.linesize()
			, ret.width(), ret.height(), ret.format(), ret.data(), ret.linesize());

		frame = ret;
	}
	return frame;
}

int FFmpegVideoEncoder::encode(AVFrameRef frame)
{
	if (frame) {
		if (m_startPts == AV_NOPTS_VALUE)
			m_startPts = frame->pts;
		if (m_startPts > frame->pts)
			return kOtherError;
		frame->pts = frame->pts - m_startPts;

		frame = transformat(frame);
	}
	return avcodec_send_frame(m_pCodecCtx, frame);
}

int FFmpegVideoEncoder::recv(AVPacketPtr& pkt)
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

void FFmpegVideoEncoder::flush()
{
	if (m_pCodecCtx)
		avcodec_flush_buffers(m_pCodecCtx);
}

#include "QcFFmpegMuxer.h"
#include "FFmpegUtils.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

QcFFmpegMuxer::QcFFmpegMuxer()
	: m_oc(0)
	, m_audioStream(0)
	, m_videoStream(0)
	, m_fileLength(0)
	, m_startTimeMs(AV_NOPTS_VALUE)
{
	m_videoParam.width = 0;
	m_videoParam.height = 0;

	m_audioParam.sampleRate = 0;
}

QcFFmpegMuxer::~QcFFmpegMuxer()
{
	close();
}

void QcFFmpegMuxer::setVideoHeader(const uint8_t* pbuf, int len)
{
	if (len <= 0 || pbuf == NULL)
		return;

	m_videoHeadBuffer.write(pbuf, len);
}

void QcFFmpegMuxer::setAudioHeader(const uint8_t* pbuf, int len)
{
	if (len <= 0 || pbuf == NULL)
		return;

	m_audioHeadBuffer.write(pbuf, len);
}

void QcFFmpegMuxer::setVideoCodecTimeBase(const QsTimeBase& timebase)
{
	m_videoTimebase = timebase;
}
void QcFFmpegMuxer::setAudioCodecTimeBase(const QsTimeBase& timebase)
{
	m_audioTimebase = timebase;
}

void QcFFmpegMuxer::setVideoFormat(const QsVideoParam& param)
{
	m_videoParam = param;
}

void QcFFmpegMuxer::setAudioFormat(const QsAudioParam& param)
{
	m_audioParam = param;
}

bool QcFFmpegMuxer::open(const char* file)
{
	close();
	do
	{
		AVOutputFormat* output_format = av_guess_format(nullptr, file, nullptr);
		if (output_format == nullptr) {
			break;
		}
		avformat_alloc_output_context2(&m_oc, output_format, nullptr, nullptr);
		if (!m_oc) {
			return false;
		}

		if (m_videoParam.width > 0 && m_videoParam.height > 0) {
			addVideoStream();
		}
		if (m_audioParam.sampleRate > 0) {
			addAudioStream();
		}

		if (!(m_oc->oformat->flags & AVFMT_NOFILE)) {
			int ret = avio_open(&m_oc->pb, file, AVIO_FLAG_WRITE);
			if (ret < 0) {
				return false;
			}
		}

		avformat_write_header(m_oc, NULL);

		m_fileLength = 0;

		return true;
	} while (0);

	close();
	return false;
}

void QcFFmpegMuxer::close()
{
	if (m_oc) {
		if (m_fileLength > 0) {
			int ret = av_write_trailer(m_oc);
			ret = ret;
		}

		if (m_oc->pb && (!(m_oc->oformat->flags & AVFMT_NOFILE))) {
			avio_closep(&(m_oc->pb));
		}

		avformat_free_context(m_oc);
		m_oc = NULL;
	}
	m_startTimeMs = AV_NOPTS_VALUE;
	m_muxerTime = 0;
}

bool QcFFmpegMuxer::newStream(AVCodecID id, AVStream** stream)
{
	AVCodec* codec = avcodec_find_encoder(id);
	if (!codec) {
		return false;
	}
	*stream = avformat_new_stream(m_oc, codec);
	if (!*stream) {
		return false;
	}
	(*stream)->id = m_oc->nb_streams - 1;
	return true;
}

bool QcFFmpegMuxer::addVideoStream()
{
	if (!newStream((AVCodecID)m_videoParam.codecID, &m_videoStream))
		return false;

	m_oc->oformat->video_codec = (AVCodecID)m_videoParam.codecID;
#if 0
	AVCodecParameters* codecpar = m_videoStream->codecpar;
	codecpar->codec_id = m_oc->oformat->video_codec;
	codecpar->bit_rate = m_videoParam.bitRate;
	codecpar->width = m_videoParam.width;
	codecpar->height = m_videoParam.height;
	codecpar->extradata = m_videoHeadBuffer.data();
	codecpar->extradata_size = m_videoHeadBuffer.size();
#endif

	AVCodecContext* context = m_videoStream->codec;
	context->bit_rate = m_videoParam.bitRate;
	context->width = m_videoParam.width;
	context->height = m_videoParam.height;
	context->coded_width = m_videoParam.width;
	context->coded_height = m_videoParam.height;
	context->time_base = { m_videoTimebase.num, m_videoTimebase.den };

	if (m_videoHeadBuffer.size()) {
		context->extradata = (uint8_t*)av_memdup(m_videoHeadBuffer.data(), m_videoHeadBuffer.size());
	}
	else {
		context->extradata = nullptr;
	}
	context->extradata_size = m_videoHeadBuffer.size();
	/* Some formats want stream headers to be separate. */
	if (m_oc->oformat->flags & AVFMT_GLOBALHEADER)
		context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	// avcodec_parameters_from_context

	m_videoStream->time_base = context->time_base;
	m_videoStream->avg_frame_rate = av_inv_q(m_videoStream->time_base);

	return true;
}

bool QcFFmpegMuxer::addAudioStream()
{
	if (!newStream((AVCodecID)m_audioParam.codecID, &m_audioStream))
		return false;

	int channel_layout = av_get_default_channel_layout(m_audioParam.nChannels);
	//AVlib default channel layout for 4 channels is 4.0 ; fix for quad
	if (m_audioParam.nChannels == 4)
		channel_layout = av_get_channel_layout("quad");
	//AVlib default channel layout for 5 channels is 5.0 ; fix for 4.1
	if (m_audioParam.nChannels == 5)
		channel_layout = av_get_channel_layout("4.1");

	m_oc->oformat->audio_codec = (AVCodecID)m_audioParam.codecID;
#if 0
	AVCodecParameters* codecpar = m_audioStream->codecpar;
	codecpar->bit_rate = m_audioParam.bitRate;
	codecpar->sample_rate = m_audioParam.sampleRate;
	codecpar->channels = m_audioParam.nChannels;
	codecpar->channel_layout = channel_layout;
	codecpar->extradata = m_audioHeadBuffer.data();
	codecpar->extradata_size = m_audioHeadBuffer.size();
#endif

	AVCodecContext* context = m_audioStream->codec;
	context->bit_rate = m_audioParam.bitRate;
	context->channels = m_audioParam.nChannels;
	context->sample_rate = m_audioParam.sampleRate;
	context->sample_fmt = (AVSampleFormat)FFmpegUtils::toFFmpegAudioFormat(m_audioParam.sampleFormat);
	context->time_base = { m_audioTimebase.num, m_audioTimebase.den };
	context->extradata = m_audioHeadBuffer.data();
	context->extradata_size = m_audioHeadBuffer.size();
	context->channel_layout = channel_layout;
	/* Some formats want stream headers to be separate. */
	if (m_oc->oformat->flags & AVFMT_GLOBALHEADER)
		context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	// avcodec_parameters_from_context
	m_audioStream->time_base = context->time_base;
	return true;
}

bool QcFFmpegMuxer::writeVideo(int32_t pts, const uint8_t* buf, int size, bool bKeyFrame)
{
	return writeVideo(pts, pts, buf, size, bKeyFrame);
}

bool QcFFmpegMuxer::writeVideo(int32_t pts, int32_t dts, const uint8_t* buf, int size, bool bKeyFrame)
{
	if (!buf || size < 0 || !m_oc || !m_videoStream) {
		return false;
	}

	if (!m_foundKeyFrame)
	{
		if (!bKeyFrame)
			return false;
		m_foundKeyFrame = true;
	}

	AVRational timebase = m_videoStream->codec->time_base;
	int64_t timeMs = QmBaseTimeToMSTime(pts, timebase);
	int64_t timeDtsMs = QmBaseTimeToMSTime(dts, timebase);
	if (m_startTimeMs == AV_NOPTS_VALUE)
	{
		m_startTimeMs = timeMs;
	}

	timeMs -= m_startTimeMs;
	timeDtsMs -= m_startTimeMs;
	if (timeMs > m_muxerTime.load())
		m_muxerTime.store(timeMs);

	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.stream_index = m_videoStream->index;
	pkt.data = (uint8_t*)buf;
	pkt.size = size;
	pkt.pts = QmMSTimeToBaseTime(timeMs, m_videoStream->time_base);
	pkt.dts = QmMSTimeToBaseTime(timeDtsMs, m_videoStream->time_base);

	/* rescale output packet timestamp values from codec to stream timebase */
	// av_packet_rescale_ts(&pkt, m_videoStream->codec->time_base, m_videoStream->time_base);

	if (bKeyFrame)
		pkt.flags |= AV_PKT_FLAG_KEY;

	int ret = av_interleaved_write_frame(m_oc, &pkt);
	if (ret != 0) {
		return false;
	}
	m_fileLength += size;
	return true;
}


bool QcFFmpegMuxer::writeAudio(int32_t pts, const uint8_t* buf, int size)
{
	if (!buf || size < 0 || !m_oc || !m_audioStream) {
		return false;
	}

	AVRational timebase = m_audioStream->codec->time_base;
	int64_t timeMs = QmBaseTimeToMSTime(pts, timebase);
	if (m_startTimeMs == AV_NOPTS_VALUE)
	{
		m_startTimeMs = timeMs;
	}
	timeMs -= m_startTimeMs;
	if (timeMs > m_muxerTime.load())
		m_muxerTime.store(timeMs);

	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = (uint8_t*)buf;
	pkt.size = size;
	pkt.flags |= AV_PKT_FLAG_KEY;
	pkt.stream_index = m_audioStream->index;
	pkt.pts = QmMSTimeToBaseTime(timeMs, m_audioStream->time_base);
	pkt.dts = pkt.pts;
	// av_packet_rescale_ts(&pkt, m_audioStream->codec->time_base, m_audioStream->time_base);

	int ret = av_interleaved_write_frame(m_oc, &pkt);
	if (ret != 0) {
		return false;
	}

	m_fileLength += size;

	return true;
}


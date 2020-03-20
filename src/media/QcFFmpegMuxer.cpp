#include "QmMacro.h"
#include "QcFFmpegMuxer.h"
#include "x264.h"
#include <dwbase/log.h>

enum
{
	AUDIO_BUF_FLAG = 0x01,
	VIDEO_BUF_FLAG = 0x02,
};

QcFFmpegMuxer::QcFFmpegMuxer()
	: m_fmt(0),
	m_oc(0),
	m_audio_st(0),
	m_video_st(0),
	m_width(640),
	m_height(480),
	m_fps(10),
	m_channels(2),
	m_audio_bits(16),
	m_audio_samples(44100),
	m_audio_bitrate(128),
	m_fileLength(0),
	m_dwStartTime(0),
	m_dwStartTimeAudio(0)
{

}

QcFFmpegMuxer::~QcFFmpegMuxer()
{
	StopWaitFor();
	close();
}

void QcFFmpegMuxer::setVideoHeader(const uint8_t *pbuf, int len)
{
	if (len <= 0 || pbuf == NULL)
		return;

	m_headBuffer.write(pbuf, len);
}

void QcFFmpegMuxer::setVideoFormat(int width, int height, int fps, int bitrate)
{
	m_width = width;
	m_height = height;
	m_fps = fps;
	m_bitrate = bitrate;
}

void QcFFmpegMuxer::setAudioFormat(unsigned char channels, unsigned short bits, int samples, int bitrate)
{
	m_channels = channels;
	m_audio_bits = bits;
	m_audio_samples = samples;
	m_audio_bitrate = bitrate;
}


bool QcFFmpegMuxer::open(const char *file)
{
	avformat_alloc_output_context2(&m_oc, NULL, NULL, file);
	if (!m_oc) {
		avformat_alloc_output_context2(&m_oc, NULL, "mp4", file);
		if (!m_oc) {
			return false;
		}
	}

	m_fmt = m_oc->oformat;
	if (!m_fmt) {
		return false;
	}

	if (m_fmt->video_codec == AV_CODEC_ID_NONE ||
		m_fmt->audio_codec == AV_CODEC_ID_NONE)
	{
		return false;
	}

	m_fmt->audio_codec = AV_CODEC_ID_MP3;
	m_fmt->video_codec = AV_CODEC_ID_H264;
	if (!add_video_stream(m_fmt->video_codec) ||
		!add_audio_stream(m_fmt->audio_codec))
	{
		return false;
	}

	if (!(m_fmt->flags & AVFMT_NOFILE)) {
		int ret = avio_open(&m_oc->pb, file, AVIO_FLAG_WRITE);
		if (ret < 0) {
			return false;
		}
	}

	m_dataHandle = CreateEventW(NULL, FALSE, FALSE, NULL);

	m_video_st->codec->extradata_size = m_headBuffer.getDataSize();
	m_video_st->codec->extradata = m_headBuffer.data();

	avformat_write_header(m_oc, NULL);

	m_fileLength = 0;
	m_dwStartTime = 0;
	m_lastpts = 0;

	return true;
}

void QcFFmpegMuxer::close()
{
	if (m_oc) {
		if (m_fileLength > 0) av_write_trailer(m_oc);

		close_video();
		close_audio();

		for (unsigned int i = 0; i < m_oc->nb_streams; i++) {
			av_freep(&m_oc->streams[i]->codec);
			av_freep(&m_oc->streams[i]);
		}

		if (m_oc->pb && (!(m_fmt->flags & AVFMT_NOFILE))) {
			avio_close(m_oc->pb);
		}
		av_free(m_oc);
		m_oc = NULL;
	}
}

void QcFFmpegMuxer::close_audio()
{
	if (m_audio_st) 
	{
		avcodec_close(m_audio_st->codec);
		m_audio_st = NULL;
	}
}
void QcFFmpegMuxer::close_video()
{
	if (m_video_st) 
	{
		avcodec_close(m_video_st->codec);
		m_video_st = NULL;
	}
}

bool QcFFmpegMuxer::add_video_stream(enum AVCodecID codec_id) 
{
	m_video_st = avformat_new_stream(m_oc, NULL);
	if (!m_video_st) {
		return false;
	}

	m_video_st->id = m_oc->nb_streams - 1;
	AVCodecContext *c = m_video_st->codec;
	c->codec_id = codec_id;
	c->codec_type = AVMEDIA_TYPE_VIDEO;
	c->bit_rate = m_bitrate * 1000;
	c->width = m_width;
	c->height = m_height;

	c->time_base.den = 1000;
	c->time_base.num = 1;
	m_video_st->time_base = c->time_base;
	c->gop_size = static_cast<int>(m_fps); /* emit one intra frame every second */
	c->pix_fmt = AV_PIX_FMT_YUV420P;

	if (m_oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;

	return true;
}

bool QcFFmpegMuxer::add_audio_stream(enum AVCodecID codec_id)
{
	m_audio_st = avformat_new_stream(m_oc, NULL);
	if (!m_audio_st) {
		return false;
	}

	m_audio_st->id = m_oc->nb_streams - 1;
	AVCodecContext *c = m_audio_st->codec;
	c->codec_id = codec_id;
	c->codec_type = AVMEDIA_TYPE_AUDIO;
	c->sample_fmt = AV_SAMPLE_FMT_S16;
	c->bit_rate = m_audio_bitrate;
	c->sample_rate = m_audio_samples;
	c->channels = m_channels;
	c->time_base.den = m_audio_samples;
	c->time_base.num = 1;

	if (m_oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= (CODEC_FLAG_GLOBAL_HEADER | AVFMT_VARIABLE_FPS);
	return true;
}

void QcFFmpegMuxer::addAudio(QcMediaBuffer& buffer)
{
	buffer.m_type = AUDIO_BUF_FLAG;
	QmCsLocker(m_cs);
	m_bufferQueue.push_back_swap(buffer, true);
	::SetEvent(m_dataHandle);
}

void QcFFmpegMuxer::addVideo(QcMediaBuffer& buffer)
{
	buffer.m_type = VIDEO_BUF_FLAG;
	QmCsLocker(m_cs);
	m_bufferQueue.push_back_swap(buffer, true);
	::SetEvent(m_dataHandle);
}

void QcFFmpegMuxer::OnRun()
{
	while (1)
	{
		DWORD ret = WaitFor(&m_dataHandle, 1);
		if (ret == WAIT_OBJECT_0)
		{
			flush();
		}
		else
		{
			break;
		}
	}
	flush();
}

void QcFFmpegMuxer::flush()
{
	while (1)
	{
		{
			QmCsLocker(m_cs);
			if (!m_bufferQueue.pop_swap(m_mediaBuffer))
				break;
			QmAssertLogBefore(m_mediaBuffer.m_type == AUDIO_BUF_FLAG || m_mediaBuffer.m_type == VIDEO_BUF_FLAG);
		}
		if (m_mediaBuffer.m_type == AUDIO_BUF_FLAG)
			writeAudio((uint32_t)m_mediaBuffer.m_tm, m_mediaBuffer.data(), m_mediaBuffer.getDataSize());
		else if (m_mediaBuffer.m_type == VIDEO_BUF_FLAG)
			writeVideo((uint32_t)m_mediaBuffer.m_tm, m_mediaBuffer.data(), m_mediaBuffer.getDataSize(), m_mediaBuffer.m_flag);
		else
		{
			QmAssertLogBefore(false);
		}
	}
}

bool QcFFmpegMuxer::writeVideo(unsigned int pts, uint8_t *buf, int size, int flag)
{
	if (!buf || size < 0 || !m_oc || !m_video_st) {
		QmAssertLogBefore(false);
		return false;
	}

	if (m_dwStartTime == 0) {
		m_dwStartTime = pts;
		m_frameCount = 0;
	}

	AVPacket pkt;
	av_init_packet(&pkt);

	pkt.stream_index = m_video_st->index;
	pkt.data = buf;
	pkt.size = size;
	pkt.pts = pts - m_dwStartTime;
	av_packet_rescale_ts(&pkt, m_video_st->codec->time_base, m_video_st->time_base);
	pkt.dts = pkt.pts;

	//  Utils::DebugPrint("writeVideo m_dwStartTime:%d pkt.pts: %d\n", m_dwStartTime, pkt.pts);
	if (flag == X264_TYPE_IDR)
		pkt.flags |= AV_PKT_FLAG_KEY;

	int ret = av_interleaved_write_frame(m_oc, &pkt);
	if (ret != 0) {
		return false;
	}

	m_fileLength += size;

	return true;
}


bool QcFFmpegMuxer::writeAudio(unsigned int pts, uint8_t *buf, int size)
{
	if (!buf || size < 0 || !m_oc || !m_audio_st) {
		QmAssertLogBefore(false);
		return false;
	}

	//   if( m_lastpts == 0 ) m_lastpts = pts;
	//   if( m_dwStartTime == 0 || pts < m_dwStartTime ) return true;
	if (m_lastpts > pts) return true;
	if (m_dwStartTimeAudio == 0)
		m_dwStartTimeAudio = pts;

	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = buf;
	pkt.size = size;
	pkt.flags |= AV_PKT_FLAG_KEY;
	pkt.stream_index = m_audio_st->index;

	AVRational bq;
	bq.num = 1;
	bq.den = 1000;
	//  pkt.pts = (pts - m_dwStartTimeAudio)*m_audio_samples/1000; 
	pkt.pts = av_rescale_q(pts - m_dwStartTimeAudio, bq, m_audio_st->codec->time_base);
	pkt.dts = pkt.pts;

	//pkt.pts = m_lastpts; //size / m_audio_st->codec->sample_rate / m_channels; //av_rescale_q(1, bq, m_audio_st->codec->time_base);
	//m_lastpts += size / m_channels / av_get_bytes_per_sample( m_audio_st->codec->sample_fmt );
	//   pkt.dts = pkt.pts; 

	int ntime = pts - m_lastpts;
	if (ntime <= 0) ntime = 20;
	pkt.duration = ntime;
	m_lastpts = pts;
	//Utils::DebugPrint("writeAudio m_dwStartTime:%d pkt.pts: %d pkt.duration: %d\n", m_dwStartTime,  (int)pkt.pts, (int)pkt.duration);

	int ret = av_interleaved_write_frame(m_oc, &pkt);
	if (ret != 0) {
		return false;
	}

	m_fileLength += size;

	return true;
}


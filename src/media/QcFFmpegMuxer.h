#pragma once

#include "Thread.h"
#include "QcCycleQueue.h"
#include "QcBuffer.h"
#include "win_scope_handle.h"
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

class QcFFmpegMuxer : public Thread
{
public:
	QcFFmpegMuxer();
	~QcFFmpegMuxer();

	void setVideoFormat(int width, int height, int fps, int bitrate);
	void setAudioFormat(unsigned char channels, unsigned short bits, int samples, int bitrate);
	void setVideoHeader(const uint8_t *pbuf, int len);

	bool open(const char *file);
	void close();

	void addAudio(QcMediaBuffer& buffer);
	void addVideo(QcMediaBuffer& buffer);
protected:
	virtual void OnRun();
	void close_audio();
	void close_video();
	bool add_video_stream(enum AVCodecID codec_id);
	bool add_audio_stream(enum AVCodecID codec_id);

	void flush();
	bool writeVideo(unsigned int pts, uint8_t *buf, int size, int flag);
	bool writeAudio(unsigned int pts, uint8_t *buf, int size);
protected:
	QcMediaBuffer m_mediaBuffer;
	QcCycleQueue<QcMediaBuffer> m_bufferQueue;
	QcCriticalLock m_cs;
    win_scope_handle m_dataHandle;

	AVOutputFormat  *m_fmt;
	AVFormatContext *m_oc;
	//AVDictionary    *m_opt;
	AVStream *m_audio_st;
	AVStream *m_video_st;
	int64_t m_frameCount;
	int             m_width;
	int             m_height;
	int             m_fps;
	int             m_bitrate;
	unsigned char   m_channels;
	unsigned short  m_audio_bits;
	unsigned int    m_audio_samples;
	unsigned int    m_audio_bitrate;
	long long       m_fileLength;
	unsigned int    m_dwStartTime;
	unsigned int    m_dwStartTimeAudio;
	unsigned int    m_lastpts;
	QcBuffer        m_headBuffer;
};

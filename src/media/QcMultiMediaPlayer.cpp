#include "QcMultiMediaPlayer.h"
extern "C"{
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
#include "FFmpegDemuxer.h"
#include "FFmpegVideoDecoder.h"
#include "FFmpegAudioDecoder.h"


static const char *get_error_text(const int error)
{
    static char error_buffer[255];
    av_strerror(error, error_buffer, sizeof(error_buffer));
    return error_buffer;
}

QcMultiMediaPlayer::QcMultiMediaPlayer(IMultiMediaNotify* pNotify)
    : m_pNotify(pNotify)

{

}

QcMultiMediaPlayer::~QcMultiMediaPlayer()
{
    Close();
}

bool QcMultiMediaPlayer::Open(const char* pFile)
{
    Close();

	m_pDemuxer = std::make_unique<FFmpegDemuxer>();
	bool bOk = m_pDemuxer->open(pFile);
	if (bOk)
	{
		AVStream* pVideoStream = m_pDemuxer->videoStream();
		if (pVideoStream)
		{
			m_pVideoDecoder = std::make_unique<FFmpegVideoDecoder>(pVideoStream->codecpar);
		}
		AVStream* pAudioStream = m_pDemuxer->audioStream();
		if (pAudioStream)
		{
			m_pAudioDecoder = std::make_unique<FFmpegAudioDecoder>(pAudioStream->codecpar);
		}
	}
    return true;
}

bool QcMultiMediaPlayer::Close()
{
	m_pVideoDecoder = nullptr;
	m_pAudioDecoder = nullptr;
	m_pDemuxer = 0;
    return true;
}

void QcMultiMediaPlayer::Play()
{
    
}

void QcMultiMediaPlayer::Pause()
{

}

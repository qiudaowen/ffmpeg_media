#include "wasapi/WASAPIPlayer.h"
#include "QcAudioPlayer.h"

struct QcAudioPlayerPrivate
{
    bool m_isOpen = false;
    QsAudioParam m_openParas;
    std::unique_ptr<WASAPIPlayer> m_player;
};

QcAudioPlayer::QcAudioPlayer()
    : m_ptr(new QcAudioPlayerPrivate())
{

}

QcAudioPlayer::~QcAudioPlayer()
{
    close();
}

bool QcAudioPlayer::open(const wchar_t* deviceID, const QsAudioParam* para, QsAudioParam* pClosestMatch)
{
	close();
    m_ptr->m_player.reset(new WASAPIPlayer());
    m_ptr->m_isOpen = m_ptr->m_player->init(deviceID, para, pClosestMatch);
	if (m_ptr->m_isOpen)
	{
        m_ptr->m_openParas = pClosestMatch ? *pClosestMatch : *para;
	}
	else
	{
        m_ptr->m_player.reset(nullptr);
	}
	return m_ptr->m_isOpen;
}

bool QcAudioPlayer::isOpen() const
{
	return m_ptr->m_isOpen;
}

const QsAudioParam& QcAudioPlayer::getAudioPara() const
{
	return m_ptr->m_openParas;
}

void QcAudioPlayer::start()
{
    if (m_ptr->m_player && m_ptr->m_isOpen)
        m_ptr->m_player->start();
}

void QcAudioPlayer::stop()
{
    if (m_ptr->m_player && m_ptr->m_isOpen)
        m_ptr->m_player->stop();
}

void QcAudioPlayer::close()
{
    m_ptr->m_isOpen = false;
    m_ptr->m_player.reset(nullptr);
}

void QcAudioPlayer::playAudio(const uint8_t* pcm, int nSamples)
{
	if (m_ptr->m_player)
	{
        int nChannel = m_ptr->m_openParas.nChannels;
        auto format = m_ptr->m_openParas.sampleFormat;
        m_ptr->m_player->playAudio(pcm, getAudioBufferSize(format, nChannel, nSamples));
	}   
}

void QcAudioPlayer::setVolume(float fVolume)
{
    if (m_ptr->m_player)
        m_ptr->m_player->SetVolume(fVolume);
}

float QcAudioPlayer::volume() const
{
    if (m_ptr->m_player)
        return m_ptr->m_player->Volume();
    return 1.0f;
}

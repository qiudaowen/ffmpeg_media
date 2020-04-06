#include "QcAudioPlayer.h"
#include "WSAPIPlayer.h"


QcAudioPlayer::QcAudioPlayer()
{

}

QcAudioPlayer::~QcAudioPlayer()
{
    close();
}

bool QcAudioPlayer::open(const wchar_t* deviceID, const QsAudioPara& para, QsAudioPara* pClosestMatch)
{
	close();
    m_player.reset(new WSAPIPlayer());
	m_isOpen =  m_player->init(deviceID, para, pClosestMatch);
	if (m_isOpen)
	{
		m_openParas = pClosestMatch ? *pClosestMatch : para;
	}
	else
	{
		m_player.reset(nullptr);
	}
	return m_isOpen;
}

bool QcAudioPlayer::isOpen() const
{
	return m_isOpen;
}

const QsAudioPara& QcAudioPlayer::getAudioPara() const
{
	return m_openParas;
}

void QcAudioPlayer::start()
{
    if (m_player && m_isOpen)
        m_player->start();
}

void QcAudioPlayer::stop()
{
    if (m_player && m_isOpen)
        m_player->stop();
}

void QcAudioPlayer::close()
{
	m_isOpen = false;
    m_player.reset(nullptr);
}

void QcAudioPlayer::playAudio(const uint8_t* pcm, int nSamples)
{
	if (m_player)
	{
		m_player->playAudio(pcm, getAudioBufferSize(m_openParas, nSamples));
	}   
}

void QcAudioPlayer::setVolume(float fVolume)
{
    if (m_player)
        m_player->SetVolume(fVolume);
}

float QcAudioPlayer::volume() const
{
    if (m_player)
        return m_player->Volume();
    return 1.0f;
}

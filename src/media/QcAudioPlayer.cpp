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
    return m_player->init(deviceID, para, pClosestMatch);
}

void QcAudioPlayer::start()
{
    if (m_player)
        m_player->start();
}

void QcAudioPlayer::stop()
{
    if (m_player)
        m_player->stop();
}

void QcAudioPlayer::close()
{
    m_player.reset(nullptr);
}

void QcAudioPlayer::playAudio(const char* pcm, int nLen)
{
    if (m_player)
        m_player->playAudio(pcm, nLen);
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

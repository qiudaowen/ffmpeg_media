#include "QcMultiMediaPlayer.h"
#include "QcMultiMediaPlayerPrivate.h"

QcMultiMediaPlayer::QcMultiMediaPlayer(IMultiMediaNotify* pNotify)
    : m_ptr(new QcMultiMediaPlayerPrivate(pNotify))

{

}

QcMultiMediaPlayer::~QcMultiMediaPlayer()
{
    delete m_ptr;
}

bool QcMultiMediaPlayer::open(const char* pFile)
{
    return m_ptr->open(pFile);
}

bool QcMultiMediaPlayer::close()
{
    return m_ptr->close();
}

bool QcMultiMediaPlayer::isPlaying() const
{
    return m_ptr->isPlaying();
}

bool QcMultiMediaPlayer::isEnd() const
{
	return m_ptr->isEnd();
}

bool QcMultiMediaPlayer::readFrame(bool bVideo, AVFrameRef& frame)
{
    return m_ptr->readFrame(bVideo, frame);
}

const QsMediaInfo& QcMultiMediaPlayer::getMediaInfo() const
{
	return m_ptr->getMediaInfo();
}

bool QcMultiMediaPlayer::hasVideo() const
{
    return m_ptr->hasVideo();
}

bool QcMultiMediaPlayer::hasAudio() const
{
    return m_ptr->hasAudio();
}

int QcMultiMediaPlayer::getCurTime() const
{
    return m_ptr->getCurTime();
}

int QcMultiMediaPlayer::getTotalTime() const
{
    return m_ptr->getTotalTime();
}

void QcMultiMediaPlayer::play()
{
    return m_ptr->play();
}

void QcMultiMediaPlayer::pause()
{
    return m_ptr->pause();
}

void QcMultiMediaPlayer::seek(int msTime)
{
    return m_ptr->seek(msTime);
}

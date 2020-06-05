#pragma once

#include "media_global.h"

class QcMultiMediaPlayerPrivate;
class AVFrameRef;
struct QsMediaInfo;

class IMultiMediaNotify
{
public:
	virtual bool OnVideoFrame(const AVFrameRef& frame) = 0;
    virtual bool OnAudioFrame(const AVFrameRef& frame) = 0;
	virtual void ToEndSignal() = 0;
};

class MEDIA_API QcMultiMediaPlayer
{
public:
    QcMultiMediaPlayer(IMultiMediaNotify* pNotify);
    ~QcMultiMediaPlayer();

	bool open(const char* pFile);
	void play();
	void pause();
	void seek(int msTime);
	bool close();
    bool isPlaying() const;
	bool isEnd() const;
	bool readFrame(bool bVideo, AVFrameRef& frame);

	const QsMediaInfo* getMediaInfo() const;
    bool hasVideo() const;
    bool hasAudio() const;
    int getCurTime() const;
	int getTotalTime() const;
protected: 
    QcMultiMediaPlayerPrivate* m_ptr;
};

#pragma once

#include "media_global.h"

class QcMultiMediaPlayerPrivate;
class AVFrameRef;
struct QsMediaInfo;
struct AVBufferRef;

class IMultiMediaNotify
{
public:
	virtual bool onVideoFrame(const AVFrameRef& frame) = 0;
    virtual bool onAudioFrame(const AVFrameRef& frame) = 0;
	virtual void toEndSignal() = 0;
};

class MEDIA_API QcMultiMediaPlayer
{
public:
    QcMultiMediaPlayer(IMultiMediaNotify* pNotify);
    ~QcMultiMediaPlayer();

	void setHwDevice(AVBufferRef* device_ctx);
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

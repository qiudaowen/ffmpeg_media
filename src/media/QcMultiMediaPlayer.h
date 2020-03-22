#ifndef QC_MULTI_MEDIA_H
#define QC_MULTI_MEDIA_H

#include <memory>

class IMultiMediaNotify
{
public:
    virtual void onMediaInfo() = 0;
	virtual void OnVideoFrame(const AVFrameRef& frame) = 0;
    virtual void OnAudioFrame(const AVFrameRef& frame) = 0;
	virtual void ToEndSignal() = 0;
};

class QcMultiMediaPlayer
{
public:
    enum QsPlayState
    {
        eReady,
        ePlaying,
        ePause,
        eRecordExitThread,
    };
    QcMultiMediaPlayer(IMultiMediaNotify* pNotify);
    ~QcMultiMediaPlayer();

	bool Open(const char* pFile);
	void Play();
	void Pause();
	void Seek(double fPos);
	bool Close();
    bool IsPlaying() const {return m_eState == ePlaying;}

    bool HasVideo() const {return m_pVideoDecoder != nullptr;}
    bool HasAudio() const  {return m_pAudioDecoder != nullptr; }

	//ms
	int GetCurTime() const;
	double GetTotalTime() const;
protected:
	void demuxeThread();
	void videoDecodeThread();
	void audioDecodeThread();
protected: 
	std::unique_ptr<FFmpegDemuxer> m_pDemuxer;
	std::unique_ptr<FFmpegVideoDecoder> m_pVideoDecoder;
	std::unique_ptr<FFmpegAudioDecoder> m_pAudioDecoder;

    IMultiMediaNotify* m_pNotify = nullptr;

	int m_iPlayTime = 0;
};
#endif

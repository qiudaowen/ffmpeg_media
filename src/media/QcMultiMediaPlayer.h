#ifndef QC_MULTI_MEDIA_H
#define QC_MULTI_MEDIA_H

struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct AVCodec;
struct AVFrame;
struct AVPacket;
struct QsVideoFrame;
class QcAudioTransformat;

typedef std::unique_ptr<AVCodecContext, std::function<void(AVCodecContext*)>> AVCodecContextPtr;
typedef std::unique_ptr<AVCodecParameters, std::function<void(AVCodecParameters*)>> AVCodecParametersPtr;


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
        ePlayToEnd,
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
    QsPlayState GetState() const { return m_eState;}
    void SynState(QsPlayState eState);

    bool HasVideo() const {return m_pVideoStream != 0;}
    bool HasAudio() const  {return m_iAudioStream != 0; }

	//ms
	int GetCurTime() const { return m_iVideoCurTime > m_iAudioCurTime ? m_iVideoCurTime : m_iAudioCurTime; }
	double GetTotalTime() const { return m_contextTotalTime; }
protected:
	void run();
	
	void CallBack();
	bool DecodeVideo(AVPacket* pVideoPacket, bool bShowFrame = true);
	bool DecodeAudio(AVPacket* pAudioPacket);
	void RecoveryFrameBuf();
	AVPacket* GetUsablePacket(int iPacketType);

    bool openVideoStream(int);
    bool openAudioStream(int);
protected: 
	AVFormatContext* m_pFormatContext;
	AVStream* m_pVideoStream;
	AVStream* m_pAudioStream;
	AVCodecContext* m_pVideoContext;
	AVCodecContext* m_pAudioContext;

    QsMediaInfo m_mediaInfo;

	int m_iVideoStream;
	int m_iAudioStream;

	int m_videoTotalTime;
	int m_audioTotalTime;

	int m_contextStartTime;
	int m_contextTotalTime;

	int m_iVideoCurTime;
	int m_iAudioCurTime;
	int m_iSeekTime;
	QTime m_startTm;

	std::deque<AVPacket*> m_DecodeVideoFrameBuf;
	std::vector<AVPacket*> m_UsableVideoFrameBuf;

	std::deque<AVPacket*> m_DecodeAudioFrameBuf;
	std::vector<AVPacket*> m_UsableAudioFrameBuf;
    IMultiMediaNotify* m_pNotify;

	int m_iVideoPacketSize;
	int m_iAudioPacketSize;
protected:
	QsPlayState m_eState;
	QsPlayState m_eThreadState;
	int m_iToEndFlag;
};
#endif

#ifndef QC_AUDIO_PLAYER_H
#define QC_AUDIO_PLAYER_H

class QAudioOutput;
struct QsAudioPara;
class QIODevice;
class QcAudioBuffer;
class QcRingBuffer;
class QcAudioPlayer
{
public:
	QcAudioPlayer();
	~QcAudioPlayer();

	void Create(QsAudioPara& para);
	void Close();

    void Play(bool bPlay);

	void PlayAudio(const char* pcm, int nLen);
	void SetVolume(float fVolume);
    float Volume() const;
protected:
	QAudioOutput* m_audioOutput;
	QIODevice* m_output;

	QcRingBuffer* m_pRingBuffer;
	char* m_pReadBuffer;
};
#endif

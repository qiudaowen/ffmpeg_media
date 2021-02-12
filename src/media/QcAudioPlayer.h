#ifndef QC_AUDIO_PLAYER_H
#define QC_AUDIO_PLAYER_H

#include <memory>
#include "media_global.h"
#include "QsAudiodef.h"

struct QsAudioParam;
class WASAPIPlayer;
struct QcAudioPlayerPrivate;
class MEDIA_API QcAudioPlayer
{
public:
	QcAudioPlayer();
	~QcAudioPlayer();

	bool open(const wchar_t* deviceID, const QsAudioParam* paras, QsAudioParam* pClosestMatch = nullptr);
	bool isOpen() const;
	const QsAudioParam& getAudioPara() const;
    void start();
    void stop();
	void close();
	
	void playAudio(const uint8_t* pcm, int nSamples);
	void setVolume(float fVolume);
    float volume() const;
protected:
    QcAudioPlayerPrivate* m_ptr;
};
#endif

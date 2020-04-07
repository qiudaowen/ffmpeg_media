#ifndef QC_AUDIO_PLAYER_H
#define QC_AUDIO_PLAYER_H

#include <memory>
#include "mediaPub.h"
#include "QsAudiodef.h"

struct QsAudioPara;
class WSAPIPlayer;
struct QcAudioPlayerPrivate;
class MEDIA_API QcAudioPlayer
{
public:
	QcAudioPlayer();
	~QcAudioPlayer();

	bool open(const wchar_t* deviceID, const QsAudioPara& paras, QsAudioPara* pClosestMatch = nullptr);
	bool isOpen() const;
	const QsAudioPara& getAudioPara() const;
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

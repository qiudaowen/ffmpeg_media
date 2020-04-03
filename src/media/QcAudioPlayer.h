#ifndef QC_AUDIO_PLAYER_H
#define QC_AUDIO_PLAYER_H

#include <memory>

struct QsAudioPara;
class WSAPIPlayer;
class QcAudioPlayer
{
public:
	QcAudioPlayer();
	~QcAudioPlayer();

	bool open(const wchar_t* deviceID, const QsAudioPara& paras, QsAudioPara* pClosestMatch = nullptr);
    void start();
    void stop();
	void close();

	void playAudio(const char* pcm, int nLen);
	void setVolume(float fVolume);
    float volume() const;
protected:
    std::unique_ptr<WSAPIPlayer> m_player;
};
#endif

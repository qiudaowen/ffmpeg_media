#pragma once

#include <functional>
#include "QsAudiodef.h"
#include "Utils/QcNotify.h"

struct QsAudioParam;
class AVFrameRef;
class WASAPICapture;
using AudioFrameCb = std::function<void(const AVFrameRef& )>;
class AudioMuxerModel
{
public:
	AudioMuxerModel();
	~AudioMuxerModel();

	void setParam(const QsAudioParam& param);
	void start();
	void stop();
	void waitStop();
	bool isStop() const { return m_bStop; }

	int32_t addNotify(AudioFrameCb cb);
	void removeNotify(int32_t notify);
protected:
	void muxerLoop();
protected:
	bool m_bStop = true;

	QsAudioParam m_audioParam;
	QcNotify<AudioFrameCb> m_frameNotify;

	WASAPICapture* m_audioCapture = nullptr;
};
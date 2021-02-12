#pragma once

#include <thread>

class QcVideoRecord;
class AVFrameRef;
class WASAPICapture;
class RecordModel
{
public:
	RecordModel();

	void start();
	void stop();
	void waitStop();
	int32_t recordTime() const;

	void pushVideoFrame(AVFrameRef videoFrame);
	void pushAudioFrame(AVFrameRef audioFrame);
protected:
	QcVideoRecord* m_videoRecord = nullptr;
	std::shared_ptr<WASAPICapture> m_audioCapture;
};
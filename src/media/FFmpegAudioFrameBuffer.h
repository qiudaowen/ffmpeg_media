#pragma once

#include <functional>
#include "QsAudiodef.h"
#include "AVFrameRef.h"

class FFmpegAudioFrameBuffer
{
public:
	FFmpegAudioFrameBuffer(const QsAudioParam& param, int nSampleBuffer);
	~FFmpegAudioFrameBuffer();

	int push(uint8_t* const* src, int nSample, std::function<int(AVFrameRef frame)> cb);
protected:
	QsAudioParam m_param;
	int m_nSampleFrame;
	int m_nSampleRemain;
	AVFrameRef m_buffer;
};
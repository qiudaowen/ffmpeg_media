#pragma once

#include <memory>

struct AVPacket;
typedef std::shared_ptr<AVPacket> AVPacketPtr;

struct QsMediaInfo
{
	int frameRate;
	int videoFormat;
	int videoWidth;
	int videoHeight;
	int videoTotalTime;

	int sampleRate;
	int audioFormat;
	int nChannels;
	int audioTotalTime;

	int iFileTotalTime;
};
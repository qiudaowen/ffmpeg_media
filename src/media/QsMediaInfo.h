#pragma once

#include "QmMacro.h"
#include <memory>

struct AVPacket;
typedef std::shared_ptr<AVPacket> AVPacketPtr;

struct QsMediaInfo
{
	double frameRate;
	int videoFormat;
	int videoWidth;
	int videoHeight;
	int videoTotalTime;
	int rotation;

	int sampleRate;
	int audioFormat;
	int nChannels;
	int audioTotalTime;

	int iFileTotalTime;
};

enum QsThreadState
{
	eReady = 0,
	ePlaying,
	ePause,
	eExitThread,
};

#define QmStdMutexLocker(mutex1) std::lock_guard<std::mutex> QmUniqueVarName(mutex1)
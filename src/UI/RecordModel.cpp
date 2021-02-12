#include "RecordModel.h"
#include <memory>
#include "libmedia/QcVideoRecord.h"
#include "libmedia/FFmpegUtils.h"
#include "libmedia/AVFrameRef.h"

RecordModel::RecordModel()
{
	FFmpegUtils::enumCodec(FFmpegUtils::kVideoEncoder, [](int codecID, int type, const char* name) {
		char buffer[512];
		sprintf_s(buffer, "codecID=%d type=%d name=%s\n", codecID, type, name);
		OutputDebugStringA(buffer);
		});
	FFmpegUtils::enumCodec(FFmpegUtils::kAudioEncoder, [](int codecID, int type, const char* name) {
		char buffer[512];
		sprintf_s(buffer, "codecID=%d type=%d name=%s\n", codecID, type, name);
		OutputDebugStringA(buffer);
		});

	m_videoRecord = new QcVideoRecord();
	QsVideoParam param;
	param.width = 1920;
	param.height = 1080;
	param.fps = 60;
	param.bitRate = (int32_t)FFmpegUtils::recommendBitRate(param.width, param.height, param.fps);
	param.codecID = FFmpegUtils::toFFmpegEncoderID("libx264");
	m_videoRecord->setVideoParam(param);

	QsAudioParam audioParam;
	audioParam.codecID = FFmpegUtils::toFFmpegEncoderID("aac");
	audioParam.bitRate = 128000;
	audioParam.sampleRate = 44100;
	audioParam.sampleFormat = kSampleFormatFloatP;
	audioParam.nChannels = 2;
	m_videoRecord->setAudioParam(audioParam);

#if 0
	FFmpegUtils::enumSampleFormat(audioParam.codecID, [this](int sampleFormat) {
		char buffer[512];
		sprintf_s(buffer, "sampleFormat=%d\n", sampleFormat);
		OutputDebugStringA(buffer);
		});
#endif
}

void RecordModel::start()
{
	waitStop();
	m_videoRecord->start("C:\\temp\\test.mp4");
}
void RecordModel::stop()
{
	m_videoRecord->stop();
}

int32_t RecordModel::recordTime() const
{
	if (m_videoRecord)
		return m_videoRecord->recordTime();
	return 0;
}

void RecordModel::waitStop()
{
	m_videoRecord->waitStop();
}

void RecordModel::pushVideoFrame(AVFrameRef videoFrame)
{
	m_videoRecord->pushVideoFrame(videoFrame);
}
void RecordModel::pushAudioFrame(AVFrameRef audioFrame)
{
	m_videoRecord->pushAudioFrame(audioFrame);
}
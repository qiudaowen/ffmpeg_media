#pragma once

#include "media_global.h"
#include <memory>

struct AVFrame;
class MEDIA_API AVFrameRef
{
public:
    AVFrameRef();
    ~AVFrameRef();

	static AVFrameRef allocFrame();
	static AVFrameRef allocFrame(int w, int h, int format, int64_t pts = 0);
	static AVFrameRef allocFrame(const uint8_t* data, int w, int h, int format, int64_t pts = 0);
    static AVFrameRef allocAudioFrame(int nb_samples, int sampleRate, int nChannel, int format, int64_t pts = 0);
	static AVFrameRef fromHWFrame(const AVFrameRef& hwFrame);

    operator AVFrame* () {
        return m_pAVFrame.get();
    }
	operator const AVFrame* () const {
		return m_pAVFrame.get();
	}
    AVFrame* operator->() {
        return m_pAVFrame.get();
    }

	uint8_t** data() const;
	int* linesize() const;

	uint8_t* data(int index) const;
	int linesize(int) const;
	int width() const;
	int height() const;
	//FFmpeg format, not fourcc format.
	int format() const;
	bool isHWFormat() const;

	int sampleRate() const;
	int sampleCount() const;
	uint64_t channelLayout() const;
	int channelCount() const;

	int ptsMsTime() const;
	void setPtsMsTime(int msTime);
private:
    std::shared_ptr<AVFrame> m_pAVFrame = nullptr;
	int m_msPts = 0;
};
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
	static AVFrameRef allocFrame(int w, int h, int format, int pts = 0);
    static AVFrameRef allocAudioFrame(int nb_samples, int nChannel, int format, int pts = 0);
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
	int format() const;
	bool isHWFormat() const;

	int sampleCount() const;
	uint64_t channelLayout() const;
	int channelCount() const;

	int ptsMsTime() const;
	void setPtsMsTime(int msTime);
private:
    std::shared_ptr<AVFrame> m_pAVFrame = nullptr;
	int m_ptsSystemTime = 0;
	int m_msPts = 0;
};
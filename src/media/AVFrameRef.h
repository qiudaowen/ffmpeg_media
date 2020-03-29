#pragma once

#include <memory>

struct AVFrame;
class AVFrameRef
{
public:
    AVFrameRef();
    ~AVFrameRef();

	static AVFrameRef allocFrame();
	static AVFrameRef allocFrame(int w, int h, int format, int pts = 0);

    operator AVFrame* () {
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

	int ptsSystemTime() const;
	void setPtsSystemTime(int sysPts);
private:
    std::shared_ptr<AVFrame> m_pAVFrame = nullptr;
	int m_ptsSystemTime = 0;
};
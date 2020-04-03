#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#ifdef __cplusplus
};
#endif
#include "AVFrameRef.h"


AVFrameRef::AVFrameRef()
{
}

AVFrameRef::~AVFrameRef()
{

}

AVFrameRef AVFrameRef::allocFrame()
{
	AVFrameRef ret;
	ret.m_pAVFrame.reset(av_frame_alloc(), [](AVFrame* frame) {
		av_frame_free(&frame);
	});
	return ret;
}

AVFrameRef AVFrameRef::allocFrame(int w, int h, int format, int pts)
{
	AVFrameRef ret = allocFrame();
	ret->width = w;
	ret->height = h;
	ret->format = format;
	ret->pts = pts;
	av_frame_get_buffer(ret, 0);
	return ret;
}

AVFrameRef allocFrame(int nb_samples, int channel_layout, int format, int pts = 0)
{
    AVFrameRef ret = allocFrame();
    ret->nb_samples = nb_samples;
    ret->channel_layout = channel_layout;
    ret->format = format;
    ret->pts = pts;
    av_frame_get_buffer(ret, 0);
    return ret;
}

uint8_t* AVFrameRef::data(int index) const
{
	return m_pAVFrame ? m_pAVFrame->data[index] : 0;
}

uint8_t** AVFrameRef::data() const
{
	return m_pAVFrame ? m_pAVFrame->data : 0;
}

int AVFrameRef::linesize(int index) const
{
	return m_pAVFrame ? m_pAVFrame->linesize[index] : 0;
}

int* AVFrameRef::linesize() const
{
	return m_pAVFrame ? m_pAVFrame->linesize : 0;
}

int AVFrameRef::width() const
{
	return m_pAVFrame ? m_pAVFrame->width : 0;
}

int AVFrameRef::height() const
{
	return m_pAVFrame ? m_pAVFrame->height : 0;
}

int AVFrameRef::format() const
{
	return m_pAVFrame ? m_pAVFrame->format : 0;
}

int AVFrameRef::ptsSystemTime() const
{
	return m_ptsSystemTime;
}

void AVFrameRef::setPtsSystemTime(int sysPts)
{
	m_ptsSystemTime = sysPts;
}


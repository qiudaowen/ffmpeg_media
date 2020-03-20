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


AVFrameRef::AVFrameRef(bool bAllocFrame)
{
    if (bAllocFrame)
    {
        AVFrame* pFrame = av_frame_alloc();
        m_pFrame.reset(pFrame, [](AVFrame* frame) {
            av_frame_free(&frame);
        });
    }
}

AVFrameRef::~AVFrameRef()
{

}


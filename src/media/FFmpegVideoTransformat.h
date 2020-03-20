#pragma once

#include <stdint.h>

struct SwsContext;
class FFmpegVideoTransformat
{
public:
    FFmpegVideoTransformat();
    ~FFmpegVideoTransformat();

    bool Transformat(int srcW, int srcH, int srcFormat, const uint8_t* const srcSlice[], const int srcStride[],
        int dstW, int dstH, int destFormat, uint8_t *const dstSlice[], const int dstStride[]);

protected:
    bool OpenSwsContext(int srcW, int srcH, int srcFormat, int dstW, int dstH, int destFormat);
    void CloseSwsContext();
protected:
    int m_srcW = 0;
    int m_srcH = 0;
    int m_srcFormat = 0;
    
    int m_dstW = 0;
    int m_dstH = 0;
    int m_dstFormat = 0;
    SwsContext* m_pSwsCtx = nullptr;
};

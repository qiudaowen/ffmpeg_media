#pragma once

#include <memory>

struct AVFrame;
class AVFrameRef
{
public:
    AVFrameRef(bool bAllocFrame = true);
    ~AVFrameRef();

    operator AVFrame* () {
        return m_pFrame.get();
    }
    AVFrame* operator->() {
        return m_pFrame.get();
    }
private:
    std::shared_ptr<AVFrame> m_pFrame = nullptr;
};
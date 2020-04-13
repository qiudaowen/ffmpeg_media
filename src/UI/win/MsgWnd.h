#pragma once

#include <vector>
#include <mutex>

class MsgWnd
{
    using PostFun = std::function<void(void)>;
public:
    MsgWnd();
    ~MsgWnd();

    static MsgWnd* mainMsgWnd();
    void post(PostFun&& fun);
protected:
    static LRESULT CALLBACK MsgWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
protected:
    HWND m_msgWnd = nullptr;

    std::mutex m_funMutex;
    std::vector<PostFun> m_asynCallList;
};

#include <windows.h>
#include "MsgWnd.h"
#include "QmMacro.h"
#include <memory>

#define QmMagicNum 0x12348d45
#define QmMagicMsg (WM_APP + 1)

static MsgWnd gMainMsgWnd;

MsgWnd::MsgWnd()
{
    m_msgWnd = ::CreateWindowW(L"STATIC", L"", WS_POPUP, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

    ::SetWindowLongPtr(m_msgWnd, GWLP_USERDATA, (LONG_PTR)this);
    ::SetWindowLongPtr(m_msgWnd, GWLP_WNDPROC, (LONG_PTR)MsgWnd::MsgWindowProc);
}

MsgWnd::~MsgWnd()
{
    ::DestroyWindow(m_msgWnd);
}

MsgWnd* MsgWnd::mainMsgWnd()
{
    return &gMainMsgWnd;
}

void MsgWnd::post(PostFun&& fun)
{
    std::unique_lock<std::mutex> lock(m_funMutex);
    m_asynCallList.emplace_back(fun);
    if (m_asynCallList.size() == 1)
    {
        PostMessageW(m_msgWnd, QmMagicMsg, QmMagicNum, 0);
    }
}

LRESULT CALLBACK MsgWnd::MsgWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    MsgWnd *pThis = (MsgWnd*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (uMsg == QmMagicMsg && wParam == QmMagicNum)
    {
        std::vector<PostFun> temp;
        {
            std::unique_lock<std::mutex> lock(pThis->m_funMutex);
            temp.swap(pThis->m_asynCallList);
        }
        for (auto& cb : temp)
        {
            cb();
        }
        return 1;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

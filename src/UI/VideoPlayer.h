
// VideoPlayer.h: PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
#endif

#include "resource.h"		// 主符号
#include <memory>

// CVideoPlayerApp:
// 有关此类的实现，请参阅 VideoPlayer.cpp
//
class CVideoPlayerDlg;
class CVideoPlayerApp : public CWinApp
{
public:
	CVideoPlayerApp();
private:
	std::unique_ptr<CVideoPlayerDlg> m_playDlg;
	CShellManager * m_pShellManager;
// 重写
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance(); // return app exit code
// 实现

	DECLARE_MESSAGE_MAP()
};

extern CVideoPlayerApp theApp;

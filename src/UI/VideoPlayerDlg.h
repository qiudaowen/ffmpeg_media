
// VideoPlayerDlg.h: 头文件
//

#pragma once

#include <memory>
#include "VideoPlayerModel.h"
#include "QcComInit.h"
#include "CNiceSlider.h"

class VideoPlayerModel;
// CVideoPlayerDlg 对话框
class CVideoPlayerDlg : public CDialogEx
{
// 构造
public:
	CVideoPlayerDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_VIDEOPLAYER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnDropFiles(HDROP hDropInfo);
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedButtonPlay();
	afx_msg void OnLbnSelchangeListVideo();
	afx_msg void OnBnClickedButtonShowList();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnTimer(UINT_PTR nIDEvent);

protected:
	std::shared_ptr<VideoPlayerModel> m_playerModel;
	QcComInit comInit;

	bool m_bInitDialog = false;
	CListBox m_videoFileList;

	CRect getDlgItemRect(int id);
	void setDlgItemRect(int id, int x, int y, int w, int h, int flag = 0);
	void moveDlgItem(int id, int x, int y);
	void adjustControlPos();
	void addVideoFileList(const std::vector<std::wstring>& fileList);
public:
	CNiceSliderCtrl m_videoProgressSlider;
	CNiceSliderCtrl m_volSliderCtrl;
	CString m_curTime;
	CString m_totalTime;
};

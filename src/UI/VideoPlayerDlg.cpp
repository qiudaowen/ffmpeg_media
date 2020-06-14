
// VideoPlayerDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "VideoPlayer.h"
#include "VideoPlayerDlg.h"
#include "afxdialogex.h"
#include "VideoPlayerModel.h"
#include "QcComInit.h"
#include "win/MsgWnd.h"
#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define QmPlayTimerID (1)

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CVideoPlayerDlg 对话框



CVideoPlayerDlg::CVideoPlayerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_VIDEOPLAYER_DIALOG, pParent)
	, m_curTime(_T(""))
	, m_totalTime(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CVideoPlayerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_VIDEO, m_videoFileList);
	DDX_Control(pDX, IDC_SLIDER_VIDEO, m_videoProgressSlider);
	DDX_Control(pDX, IDC_SLIDER_VOLUME, m_volSliderCtrl);
	DDX_Text(pDX, IDC_PLAYTIME, m_curTime);
	DDX_Text(pDX, IDC_TOTALTIME, m_totalTime);
}

BEGIN_MESSAGE_MAP(CVideoPlayerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_WM_TIMER()
	ON_WM_DROPFILES()
	
	ON_BN_CLICKED(IDC_PLAY_PAUSE, &CVideoPlayerDlg::OnBnClickedButtonPlay)

	ON_LBN_SELCHANGE(IDC_LIST_VIDEO, &CVideoPlayerDlg::OnLbnSelchangeListVideo)
	ON_BN_CLICKED(IDC_ADD_VIDEOFILE, &CVideoPlayerDlg::OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_SHOW_LIST, &CVideoPlayerDlg::OnBnClickedButtonShowList)
END_MESSAGE_MAP()


// CVideoPlayerDlg 消息处理程序

BOOL CVideoPlayerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	UpdateWindow();

	m_bInitDialog = true;
	MsgWnd::mainMsgWnd()->post([this]() {
		adjustControlPos();
		UpdateWindow();
	});

	m_videoProgressSlider.SetRange(0, 10000, TRUE);
	m_volSliderCtrl.SetRange(0, 10000, TRUE);
	

	CWnd* pWnd = GetDlgItem(IDC_VIDEOWND);
	m_playerModel = std::make_shared<VideoPlayerModel>();
	m_playerModel->init(pWnd->GetSafeHwnd());
	//m_playerModel->open(L"D:\\wow1080p60fps.mp4");

	DragAcceptFiles(TRUE);

	SetTimer(QmPlayTimerID, 500, NULL);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CVideoPlayerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

void CVideoPlayerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

void CVideoPlayerDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	if (m_bInitDialog)
		adjustControlPos();
}

HCURSOR CVideoPlayerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CVideoPlayerDlg::OnDropFiles(HDROP hDropInfo)
{
	int count_droppedfile = DragQueryFileW(hDropInfo, 0xFFFFFFFF, NULL, 0);
	std::vector<std::wstring> fileList;
	fileList.reserve(count_droppedfile);
	for (int i = 0; i < count_droppedfile; ++i)
	{
		wchar_t filepath[MAX_PATH] = { 0 };
		if (DragQueryFileW(hDropInfo, i, filepath, MAX_PATH) > 0)
		{
			fileList.push_back(filepath);
		}
	}
	addVideoFileList(fileList);
}

static std::vector<std::wstring> getOpenFileList(const wchar_t* szFilterList, CWnd* parent = nullptr, int nMaxFiles = 100)
{
	CFileDialog fileDlg(TRUE, NULL, NULL, OFN_ALLOWMULTISELECT | OFN_ENABLESIZING | OFN_HIDEREADONLY, szFilterList, parent);
	fileDlg.m_ofn.nMaxFile = nMaxFiles * MAX_PATH;
	CString buffer;
	fileDlg.m_ofn.lpstrFile = buffer.GetBuffer(fileDlg.m_ofn.nMaxFile + 16);
	fileDlg.m_ofn.lpstrTitle = L"打开文件";
	int iRet = fileDlg.DoModal();

	std::vector<std::wstring> fileList;
	if (iRet == IDOK)
	{
		POSITION pos_file = fileDlg.GetStartPosition();
		while (NULL != pos_file)
		{
			CString pathName = fileDlg.GetNextPathName(pos_file);
			fileList.emplace_back(CT2CW(pathName.GetBuffer()));
		}
	}
	buffer.ReleaseBuffer();
	return fileList;
}

void CVideoPlayerDlg::OnBnClickedButtonAdd()
{
	TCHAR szFilter[] = _T("视频文件(*.FLV,*.mp4,*.avi,*.wmv,*.mkv)|*.FLV;*.mp4;*.avi;*.wmv;*.mkv|所有文件(*.*)|*.*||");
	std::vector<std::wstring> fileList = getOpenFileList(szFilter, this);
	std::vector<std::wstring> wfileList;
	for (const auto& item : fileList)
	{
		wfileList.push_back(item);
	}
	addVideoFileList(wfileList);
}

void CVideoPlayerDlg::addVideoFileList(const std::vector<std::wstring>& fileList)
{
	for (const auto& item : fileList)
	{
		m_videoFileList.AddString(CW2CT(item.c_str()));
	}
	m_playerModel->addVideoFileList(fileList);
}


void CVideoPlayerDlg::OnBnClickedButtonPlay()
{
	m_playerModel->trigger();
}

void CVideoPlayerDlg::OnLbnSelchangeListVideo()
{
	// TODO: 在此添加控件通知处理程序代码
}


void CVideoPlayerDlg::OnBnClickedButtonShowList()
{
	m_videoFileList.ShowWindow(m_videoFileList.IsWindowVisible() ? SW_HIDE : SW_SHOW);
	adjustControlPos();
}

void CVideoPlayerDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CSliderCtrl* pSlider = (CSliderCtrl*)pScrollBar;
	double fPos = pSlider->GetPos() / (double)pSlider->GetRangeMax();

	switch (pSlider->GetDlgCtrlID())
	{
	case IDC_SLIDER_VIDEO:
	{
		if (m_playerModel)
			m_playerModel->setProgress(fPos);
		break;
	}
	case IDC_SLIDER_VOLUME:
	{
		if (m_playerModel)
			m_playerModel->setVolume(fPos);
		break;
	}
	}
}

void CVideoPlayerDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (m_playerModel)
	{
		double fPos = m_playerModel->getProgress();
		m_videoProgressSlider.SetPos(fPos * m_videoProgressSlider.GetRangeMax());

		int curTime = m_playerModel->getCurTime();
		int totalTime = m_playerModel->getTotalTime();


		m_curTime = CTimeSpan(curTime / 1000).Format("%H:%M:%S");
		m_totalTime = CTimeSpan(totalTime / 1000).Format("%H:%M:%S");
		UpdateData(FALSE);
	}
	CDialog::OnTimer(nIDEvent);
}

CRect CVideoPlayerDlg::getDlgItemRect(int id)
{
	HWND hWnd;
	CRect rect;
	GetDlgItem(id, &hWnd);
	::GetWindowRect(hWnd, &rect);
	::MapWindowPoints(hWnd, ::GetParent(hWnd), (LPPOINT)&rect, 2);
	return rect;
}

void CVideoPlayerDlg::setDlgItemRect(int id, int x, int y, int w, int h, int flag)
{
	HWND hWnd;
	GetDlgItem(id, &hWnd);
	::SetWindowPos(hWnd, NULL, x, y, w, h, SWP_NOZORDER | flag);
}

void CVideoPlayerDlg::moveDlgItem(int id, int x, int y)
{
	setDlgItemRect(id, x, y, 0, 0, SWP_NOSIZE);
}

void CVideoPlayerDlg::adjustControlPos()
{
	CRect rect;
	GetClientRect(&rect);

	CRect videoRect = getDlgItemRect(IDC_VIDEOWND);
	CRect playTimeRect = getDlgItemRect(IDC_PLAYTIME);
	CRect sliderVideoRect = getDlgItemRect(IDC_SLIDER_VIDEO);
	CRect totalTimeRect = getDlgItemRect(IDC_TOTALTIME);
	CRect playBtnRect = getDlgItemRect(IDC_PLAY_PAUSE);
	CRect volumeTextRect = getDlgItemRect(IDC_VOL_TEXT);
	CRect sliderVolumeRect = getDlgItemRect(IDC_SLIDER_VOLUME);
	
	CRect videoListRect = getDlgItemRect(IDC_LIST_VIDEO);
	CRect addVideoFileBtnRect = getDlgItemRect(IDC_ADD_VIDEOFILE);
	CRect showVideoListBtnRect = getDlgItemRect(IDC_SHOW_LIST);

	int totalW = rect.Width();
	int totalH = rect.Height();
	int bottomItemH = playBtnRect.Height() + 10;
	int bottomItemY = totalH - bottomItemH + 10;
	int videoFileListW = m_videoFileList.IsWindowVisible() ? videoListRect.Width() : 0;

	//video and processSlider
	int sliderVideoH = sliderVideoRect.Height();
	int videoW = totalW - videoFileListW;
	int videoH = totalH - sliderVideoH - bottomItemH;
	setDlgItemRect(IDC_VIDEOWND, 0, 0, videoW, videoH);
	moveDlgItem(IDC_PLAYTIME, 0, videoH);
	setDlgItemRect(IDC_SLIDER_VIDEO, playTimeRect.Width(), videoH, videoW - (playTimeRect.Width() + totalTimeRect.Width()), sliderVideoH);
	moveDlgItem(IDC_TOTALTIME, videoW - totalTimeRect.Width(), videoH);

	if (m_videoFileList.IsWindowVisible())
		setDlgItemRect(IDC_LIST_VIDEO, totalW - videoFileListW, 0, videoFileListW, totalH - bottomItemH);

	int bottomItemsTotalW = playBtnRect.Width() + volumeTextRect.Width() + sliderVolumeRect.Width() + showVideoListBtnRect.Width() + addVideoFileBtnRect.Width();
	int halfSpaceW = (totalW - bottomItemsTotalW) / 2;
	int bottomItemX = halfSpaceW;
	moveDlgItem(IDC_PLAY_PAUSE, bottomItemX, bottomItemY);
	bottomItemX += playBtnRect.Width();
	moveDlgItem(IDC_VOL_TEXT,   bottomItemX, bottomItemY);
	bottomItemX += volumeTextRect.Width();
	moveDlgItem(IDC_SLIDER_VOLUME, bottomItemX, bottomItemY);
	bottomItemX += sliderVolumeRect.Width();

	bottomItemX += halfSpaceW;
	moveDlgItem(IDC_ADD_VIDEOFILE, bottomItemX, bottomItemY);
	bottomItemX += showVideoListBtnRect.Width();
	moveDlgItem(IDC_SHOW_LIST, bottomItemX, bottomItemY);
}



// my-mag-sampleDlg.cpp : implementation file
//

#include "stdafx.h"

#include <stdexcept>
#include <set>
#include <chrono>
#include <sstream>
#include <functional>
#include <iostream>
#include <iomanip>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <dwmapi.h>

#include "WinVersionHelper.h"
#include "Win32WindowEnumeration.h"
#include "CapUtility.h"
#include "MagCapture.h"
#include "GDICapture.h"
#include "DXGICapture.h"
#include "WGCCapture.h"
#include "DWMCapture.h"

#include "my-mag-sample.h"
#include "my-mag-sampleDlg.h"
#include "afxdialogex.h"

#include "logger.h"

#pragma comment(lib, "dwmapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
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


// CmymagsampleDlg dialog

std::vector<Window> _wndList;

CmymagsampleDlg::CmymagsampleDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_MYMAGSAMPLE_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    _appContext = std::make_unique<AppContext>();
}

void CmymagsampleDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_COMBO_WNDLIST, _wndListCombobox);
    DDX_Control(pDX, IDC_STATIC_PVWWND, _previewWnd);
    DDX_Control(pDX, IDC_STATIC_WININFO, _winRectInfoText);
    DDX_Control(pDX, IDC_COMBO_SYSTEM, _sysComboBox);
}

BEGIN_MESSAGE_MAP(CmymagsampleDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BTN_WNDCAP, &CmymagsampleDlg::OnBnClickedBtnWndcap)
    ON_BN_CLICKED(IDC_BUTTON_FINDWIND, &CmymagsampleDlg::OnBnClickedButtonFindwind)
    ON_BN_CLICKED(IDC_BTN_SCREENCAP, &CmymagsampleDlg::OnBnClickedBtnScreencap)
    ON_BN_CLICKED(IDC_BTN_STOP, &CmymagsampleDlg::OnBnClickedBtnStop)
    ON_MESSAGE(WM_DISPLAYCHANGE, &CmymagsampleDlg::OnDisplayChanged)
    ON_MESSAGE(WM_DPICHANGED, &CmymagsampleDlg::OnDPIChanged)
    ON_CBN_SELCHANGE(IDC_COMBO_WNDLIST, &CmymagsampleDlg::OnCbnSelchangeComboWndlist)
    ON_WM_SIZE()
    ON_WM_CLOSE()
    END_MESSAGE_MAP()


// CmymagsampleDlg message handlers

BOOL CmymagsampleDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

    SetWindowDisplayAffinity(GetSafeHwnd(), WDA_EXCLUDEFROMCAPTURE);
    
    BOOL peek = FALSE;
    DwmSetWindowAttribute(GetSafeHwnd(), DWMWA_EXCLUDED_FROM_PEEK, &peek, sizeof(peek));

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
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

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
    DWORD pid = GetCurrentProcessId();

    std::wstring title = L"my-mag-sample - " + std::to_wstring(pid);
    SetWindowTextW(title.c_str());

    std::vector<std::wstring> systemList = { L"Win11",      // 0
                                             L"Win10-1903", // 1
                                             L"Win10",      // 2
                                             L"Win8.1",     // 3
                                             L"Win8",       // 4
                                             L"Win7SP1",    // 5
                                             L"Win7",       // 6
                                             L"WInVista",   // 7
                                             L"WInXP" };    // 8

    for (auto &it : systemList) {
        _sysComboBox.AddString(it.c_str());
    }
    _sysComboBox.SetCurSel(0);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CmymagsampleDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CmymagsampleDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CmymagsampleDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CmymagsampleDlg::OnCaptureFrame(VideoFrame *frame)
{
    auto &render = _appContext->render;
    if (!render.d3d11render)
        return;
    render.d3d11render->display(*frame);
    render.statics->append();
}

void CaptureCallback(VideoFrame *frame, void *args)
{
    CmymagsampleDlg *pDlg = reinterpret_cast<CmymagsampleDlg *>(args);
    pDlg->OnCaptureFrame(frame);
}

LRESULT CmymagsampleDlg::OnDPIChanged(WPARAM wParam, LPARAM lParam)
{
    long dpi = LOWORD(wParam);
    RECT newRect = *(reinterpret_cast<const RECT *>(lParam));

    return 0;
}

LRESULT CmymagsampleDlg::OnDisplayChanged(WPARAM wParam, LPARAM lParam)
{
    auto &cpt = _appContext->cpt;

    cpt.rect.set_width(LOWORD(lParam));
    cpt.rect.set_width(HIWORD(lParam));

    if (cpt.host.get()) {
        if (cpt.winID) {
            OnBnClickedBtnWndcap();
        }
        else if (cpt.screenID) {
            OnBnClickedBtnScreencap();
        }
    }

    return 0;
}


void CmymagsampleDlg::OnBnClickedButtonFindwind()
{
    int curSel = 0;
    auto &cpt = _appContext->cpt;

    _wndListCombobox.ResetContent();
    _wndList.clear();
    _wndList = EnumerateWindows();

    for (auto &wnd : _wndList) {
        std::wstringstream ss;
        ss << std::hex << std::setw(8) << std::setfill(L'0') << (uint32_t)wnd.Hwnd() << L" | " << wnd.ClassName()
           << L" | " << wnd.Title();

        int index = _wndListCombobox.AddString(ss.str().c_str());
        if (wnd.Hwnd() == cpt.winID) {
            curSel = index;
        }
    }

    _wndListCombobox.SetCurSel(curSel);
}


std::unique_ptr<CCapture> CmymagsampleDlg::_createCaptureBackend(bool isScreenCapture, int sysIdx)
{
    if (isScreenCapture) {
        if (Platform::IsWin10_1903OrGreater() && sysIdx < 2)
            return std::make_unique<WGCCapture>();
        if (Platform::IsWindows8OrGreater() && sysIdx < 5)
            return std::make_unique<DXGICapture>();
        if (Platform::IsWindowsVistaOrGreater() && sysIdx < 8)
            return std::make_unique<MagCapture>();
        return std::make_unique<GDICapture>();
    }
    else {
        if (Platform::IsWin10_1903OrGreater() && sysIdx < 2)
            return std::make_unique<WGCCapture>();
        if (Platform::IsWindows7OrGreater() && sysIdx < 7)
            return std::make_unique<DWMCapture>();
        if (Platform::IsWindowsVistaOrGreater() && sysIdx < 8)
            return std::make_unique<MagCapture>();
        return std::make_unique<GDICapture>();
    }
}

void CmymagsampleDlg::_initRender()
{
    auto &render = _appContext->render;

    if (render.d3d11render) {
        render.d3d11render.reset();
        render.statics.reset();
    }

    render.d3d11render = std::make_unique<d3d11render>();
    render.statics = std::make_unique<utils::SlidingWindow>(2000);
    render.d3d11render->init(_previewWnd.GetSafeHwnd());
    render.d3d11render->setMode(2);
}


void CmymagsampleDlg::OnBnClickedBtnWndcap()
{
    auto &cpt = _appContext->cpt;
    auto idx = _sysComboBox.GetCurSel();

    OnBnClickedBtnStop();

    try {
        cpt.winID = _wndList.at(_wndListCombobox.GetCurSel()).Hwnd();
    }
    catch (std::out_of_range &e) {
        return;
    }
    cpt.screenID = nullptr;

    cpt.host = _createCaptureBackend(false, idx);
    cpt.host->setCallback(CaptureCallback, this);
    cpt.statics = std::make_unique<utils::SlidingWindow>(2000);

    std::vector<HWND> es = { GetSafeHwnd() };
    cpt.host->setExcludeWindows(es);

    _initRender();

    if (!cpt.host->startCaptureWindow(cpt.winID)) {
        // Fallback to GDI
        cpt.host = std::make_unique<GDICapture>();
        cpt.host->setCallback(CaptureCallback, this);
        std::vector<HWND> es2 = { GetSafeHwnd() };
        cpt.host->setExcludeWindows(es2);
        if (!cpt.host->startCaptureWindow(cpt.winID)) {
            FlashWindowEx(FLASHW_ALL, 3, 300);
            return;
        }
    }
}

void CmymagsampleDlg::OnBnClickedBtnScreencap()
{
    auto &cpt = _appContext->cpt;
    auto idx = _sysComboBox.GetCurSel();

    OnBnClickedBtnStop();

    cpt.screenID = MonitorFromWindow(GetSafeHwnd(), MONITOR_DEFAULTTONEAREST);
    CapUtility::DisplaySetting settings = CapUtility::enumDisplaySettingByMonitor(cpt.screenID);
    cpt.rect = DesktopRect::MakeRECT(settings.rect());
    cpt.winID = 0;

    cpt.host = _createCaptureBackend(true, idx);
    cpt.host->setCallback(CaptureCallback, this);
    cpt.statics = std::make_unique<utils::SlidingWindow>(2000);
    cpt.host->updateRect(cpt.rect);

    _initRender();

    if (!cpt.host->startCaptureScreen(cpt.screenID)) {
        // Fallback to GDI
        cpt.host = std::make_unique<GDICapture>();
        cpt.host->setCallback(CaptureCallback, this);
        cpt.host->updateRect(cpt.rect);
        if (!cpt.host->startCaptureScreen(cpt.screenID)) {
            FlashWindowEx(FLASHW_ALL, 3, 300);
            return;
        }
    }
}


void CmymagsampleDlg::OnBnClickedBtnStop()
{
    auto &cpt = _appContext->cpt;
    auto &render = _appContext->render;

    if (cpt.host.get()) {
        cpt.host->stop();
        cpt.host.reset(nullptr);
    }

    if (render.d3d11render) {
        render.d3d11render.reset(nullptr);
    }

    _previewWnd.InvalidateRect(NULL);
}


void CmymagsampleDlg::OnCbnSelchangeComboWndlist()
{
    HWND hWnd = nullptr;

    try {
        hWnd = _wndList.at(_wndListCombobox.GetCurSel()).Hwnd();
    }
    catch (std::out_of_range &e) {
        return;
    }

    if (hWnd) 
    {
        CRect tOrigRect;
        WINDOWINFO info;
        info.cbSize = sizeof(WINDOWINFO);
        ::GetWindowRect(hWnd, &tOrigRect);
        ::GetWindowInfo(hWnd, &info);

        BOOL bEnable;
        if (S_OK == DwmIsCompositionEnabled(&bEnable)) {
        }

        CRect tDwmRect;
        if (bEnable && (S_OK != DwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, &tDwmRect, sizeof(RECT)))) {
        }

        CRect tWithoutBorder;
        CapUtility::GetWindowRectAccuracy(hWnd, tWithoutBorder);

        WCHAR strInfo[MAX_PATH];
        // clang-format off 
        swprintf_s(strInfo, MAX_PATH,
                   L"Info: GetWindowRect = %d,%d,%d,%d(%dx%d), "
                   L"WinInfo: (%dx%d)%d,%d,%d,%d\n"
                   L"DwmGetWindowAttribute = %d,%d,%d,%d(%dx%d), "
                   L"my = %d,%d,%d,%d(%dx%d)",
                   tOrigRect.left, tOrigRect.top, tOrigRect.right, tOrigRect.bottom, tOrigRect.Width(),tOrigRect.Height(), 
            info.cxWindowBorders, info.cyWindowBorders,info.rcWindow.left,info.rcWindow.top, info.rcWindow.right, info.rcWindow.bottom, 
            tDwmRect.left, tDwmRect.top,tDwmRect.right, tDwmRect.bottom, tDwmRect.Width(), tDwmRect.Height(), 
            tWithoutBorder.left, tWithoutBorder.top, tWithoutBorder.right, tWithoutBorder.bottom, tWithoutBorder.Width(),tWithoutBorder.Height()
        );
        // clang-format on

        _winRectInfoText.SetWindowTextW(strInfo);
    }
}

void CmymagsampleDlg::OnSize(UINT nType, int cx, int cy)
{
    if (_appContext->render.d3d11render) {
        _appContext->render.d3d11render->onResize(cx, cy);
    }
    CDialogEx::OnSize(nType, cx, cy);
}

void CmymagsampleDlg::OnClose()
{
    OnBnClickedBtnStop();
    CDialogEx::OnClose();
}

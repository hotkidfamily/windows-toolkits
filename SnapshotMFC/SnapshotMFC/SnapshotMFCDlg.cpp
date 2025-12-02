
// SnapshotMFCDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "SnapshotMFC.h"
#include "SnapshotMFCDlg.h"
#include "afxdialogex.h"

#include <sstream>
#include <iomanip>
#include <chrono>


#include "Win32WindowEnumeration.h"
#include "Win32DispalyEnumeration.h"
#include "Snapshot.Imp.h"
#include "Snapshot.Utils.h"
#include "Snapshot.DPI.Utils.h"
#include "logger.h"

#pragma comment(lib, "dwmapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


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


// CSnapshotMFCDlg 对话框



CSnapshotMFCDlg::CSnapshotMFCDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SNAPSHOTMFC_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSnapshotMFCDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_SNAPshot, _ProcLists);
    DDX_Control(pDX, IDC_COMBO_APILEVEL, _comboxAPI);
    DDX_Control(pDX, IDC_COMBO_WNDLIST, _wndListCbbox);
}

BOOL CSnapshotMFCDlg::PreTranslateMessage(MSG* msg)
{
	if (msg->message == WM_KEYDOWN && msg->wParam == VK_ESCAPE) {
		return TRUE;
	}
	return CWnd::PreTranslateMessage(msg);
}

BEGIN_MESSAGE_MAP(CSnapshotMFCDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_MESSAGE(WM_DPICHANGED, OnDPIMessage)
	ON_BN_CLICKED(IDC_BUTTON_EnumWnd, &CSnapshotMFCDlg::OnBnClickedButtonEnumwnd)
    ON_BN_CLICKED(IDC_BUTTON_EnumMonitors, &CSnapshotMFCDlg::OnBnClickedButtonEnummonitors)
    ON_BN_CLICKED(IDC_BTN_KEEP, &CSnapshotMFCDlg::OnBnClickedBtnKeep)
    ON_CBN_DROPDOWN(IDC_COMBO_WNDLIST, &CSnapshotMFCDlg::OnCbnDropdownComboWndlist)
    END_MESSAGE_MAP()

LRESULT CSnapshotMFCDlg::OnDPIMessage(WPARAM wParam, LPARAM lParam)
{
    RECT *const prcNewWindow = (RECT *)lParam;
    ::SetWindowPos(this->GetSafeHwnd(), NULL, prcNewWindow->left, prcNewWindow->top,
                   prcNewWindow->right - prcNewWindow->left,
                   prcNewWindow->bottom - prcNewWindow->top, SWP_NOZORDER | SWP_NOACTIVATE);
    ::InvalidateRect(this->GetSafeHwnd(), nullptr, TRUE);

    auto newDPI = HIWORD(wParam);
    return 0;
}

// CSnapshotMFCDlg 消息处理程序

BOOL CSnapshotMFCDlg::OnInitDialog()
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
    _normal = std::make_unique<CImageList>();
    auto ret = _normal->Create(_ImgSz.cx, _ImgSz.cy, ILC_COLOR32, 1, 1);
    _ProcLists.SetImageList(_normal.get(), LVSIL_NORMAL);
    _ProcLists.SetImageList(_normal.get(), LVSIL_SMALL);
	
	const wchar_t *apis[] = { L"DWM", L"GDI", L"Desktop" };
    for (auto &v : apis)
    {
        _comboxAPI.AddString(v);
	}

	_comboxAPI.SetCurSel(0);

    Snapshot::DPI::LoadAPI();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CSnapshotMFCDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CSnapshotMFCDlg::OnPaint()
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

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CSnapshotMFCDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

static CBitmap *BmpstreamToBitmap(uint8_t *data, int data_len)
{
    CImage img;
    IStream *pStream = SHCreateMemStream(data, static_cast<UINT>(data_len));
    if (!pStream)
    {
        return false;
    }
    if (img.Load(pStream) != S_OK)
    {
        pStream->Release();
        return NULL;
    }

    CBitmap *pBitmap = new CBitmap();
    pBitmap->Attach(img.Detach());
    pStream->Release();
    return pBitmap;
}
static CBitmap *BmpToBitmap(LPCTSTR pszFilePath)
{
	CImage img;
	if (img.Load(pszFilePath) != S_OK)
	{
        return NULL;
	}

    CBitmap *pBitmap = new CBitmap();
    pBitmap->Attach(img.Detach());

    return pBitmap;
}

void CSnapshotMFCDlg::OnBnClickedButtonEnumwnd()
{
    auto selected = _comboxAPI.GetCurSel();
    auto start = std::chrono::high_resolution_clock::now();
	std::wstringstream wss;
    using snaps = struct
    {
        HWND hwnd;
        std::wstring path;
        std::wstring title;
        Snapshot::snap_img_desc desc;
    };
	auto v = EnumerateWindows();
    logger::logErrorW(L"          -------------------------           ");
    auto middle1 = std::chrono::high_resolution_clock::now();
    std::vector<snaps> paths;
	for (auto& it : v) {
        //if (::IsIconic(it.Hwnd()))
        //    ::SendMessageW(it.Hwnd(), WM_SYSCOMMAND, SC_RESTORE, 0);
        logger::logInfoW(L"Enum: %s", it.Title().c_str());
        wss << LR"(D:/temp/)" << std::setw(8) << std::setfill(L'0') << std::hex << (uint32_t)it.Hwnd() << L"("
            << it.PID() << L")" << L".bmp";
        Snapshot::snap_img_desc dest;
        if (selected == 0)
        {
            Snapshot::ImpGDI::CaptureAnImageDWM(it.Hwnd(), _ImgSz.cx, _ImgSz.cy, dest);
        }
        else if (selected == 1)
        {
            Snapshot::ImpGDI::CaptureAnImageGDI(it.Hwnd(), _ImgSz.cx, _ImgSz.cy, dest);
        }
        else
        {
            Snapshot::ImpGDI::CaptureAnImageGDIFromDesktop(it.Hwnd(), _ImgSz.cx, _ImgSz.cy, dest);
        }
        paths.push_back({it.Hwnd(), wss.str(), it.Title(), dest});
		wss.str(L"");
	}
    auto middle2 = std::chrono::high_resolution_clock::now();
    _ProcLists.DeleteAllItems();

    if (CImageList *pImgList = _ProcLists.GetImageList(LVSIL_SMALL))
    {
        pImgList->Remove(-1); // -1 表示删除所有图像
    }
    if (CImageList *pImgList = _ProcLists.GetImageList(LVSIL_NORMAL))
    {
        pImgList->Remove(-1); // -1 表示删除所有图像
    }

    for (auto &wt : paths)
    {
        auto bmp = BmpstreamToBitmap(wt.desc.data, wt.desc.data_len);
        wss.str(L"");
        wss << wt.title << " 0x"<< (uint32_t)wt.hwnd;
        int nItem = _ProcLists.InsertItem(_ProcLists.GetItemCount(), wss.str().c_str(), 0);

        auto ridx = _normal->Add(bmp, RGB(0, 0, 0));
        if (ridx != -1)
        {
			_ProcLists.SetItem(nItem, 0, LVIF_IMAGE, NULL, ridx, 0, 0, 0);
        }
        else
        {
			_ProcLists.SetItem(nItem, 0, LVIF_IMAGE, NULL, 0, 0, 0, 0);
        }
        delete bmp;
        bmp = nullptr;
        Snapshot::ImpGDI::FreeImage(wt.desc);
    }

	auto end = std::chrono::high_resolution_clock::now();
    auto d = std::chrono::duration_cast<std::chrono::milliseconds>(middle1 - start).count();
    auto d3 = std::chrono::duration_cast<std::chrono::milliseconds>(middle2 - middle1).count();
    auto d2 = std::chrono::duration_cast<std::chrono::milliseconds>(end - middle2).count();
    auto d4 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    if (d == d2)
    {
		
	}
    else
    {
        //d4 = d2;
	}

	std::wostringstream oss;
    oss << d << L" / " << d3 << L" / " << d2 << L" / " << d4;
    logger::logErrorW(L"cost - %s", oss.str().c_str());
}

void CSnapshotMFCDlg::OnBnClickedButtonEnummonitors()
{
    auto monitors = enumMonitor();
    auto start = std::chrono::high_resolution_clock::now();

    using snaps = struct
    {
        HMONITOR hm;
        Snapshot::snap_img_desc desc;
    };
    std::vector<snaps> paths;
    for (auto &v : monitors)
    {
        Snapshot::snap_img_desc dest;
        Snapshot::ImpGDI::CaptureMonitorByGDI(v.hMonitor(), _ImgSz.cx, _ImgSz.cy, dest);
        paths.push_back({ v.hMonitor(), dest });
    }

    auto middle = std::chrono::high_resolution_clock::now();
     _ProcLists.DeleteAllItems();

    if (CImageList *pImgList = _ProcLists.GetImageList(LVSIL_SMALL))
    {
        pImgList->Remove(-1); // -1 表示删除所有图像
    }
    if (CImageList *pImgList = _ProcLists.GetImageList(LVSIL_NORMAL))
    {
        pImgList->Remove(-1); // -1 表示删除所有图像
    }

    std::wstringstream wss;

    for (auto &wt : paths)
    {
        auto bmp = BmpstreamToBitmap(wt.desc.data, wt.desc.data_len);
        wss.str(L"");
        wss << wt.hm;
        int nItem = _ProcLists.InsertItem(_ProcLists.GetItemCount(), wss.str().c_str(), 0);

        auto ridx = _normal->Add(bmp, RGB(0, 0, 0));
        if (ridx != -1)
        {
            _ProcLists.SetItem(nItem, 0, LVIF_IMAGE, NULL, ridx, 0, 0, 0);
        }
        else
        {
            _ProcLists.SetItem(nItem, 0, LVIF_IMAGE, NULL, 0, 0, 0, 0);
        }
        delete bmp;
        bmp = nullptr;
        Snapshot::ImpGDI::FreeImage(wt.desc);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto d = std::chrono::duration_cast<std::chrono::milliseconds>(middle - start).count();
    auto d2 = std::chrono::duration_cast<std::chrono::milliseconds>(end - middle).count();
    auto d4 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::wostringstream oss;
    oss << d << L" / " << d2 << L" / " << d4;
    logger::logErrorW(L"cost - %s", oss.str().c_str());
}

void CSnapshotMFCDlg::OnBnClickedBtnKeep()
{
    auto idx = _wndListCbbox.GetCurSel();
    HWND hWnd = reinterpret_cast<HWND>(_wndListCbbox.GetItemData(idx));

    _keeper = std::make_unique<WndKeeper>(hWnd);
}

void CSnapshotMFCDlg::OnCbnDropdownComboWndlist()
{
    auto wnds = EnumerateWindows();
    _wndListCbbox.ResetContent();
    
    for (auto &v : wnds)
    {
        auto idx = _wndListCbbox.AddString(v.Title().c_str());
        _wndListCbbox.SetItemData(idx, reinterpret_cast<DWORD_PTR>(v.Hwnd()));
    }
}


// SnapshotMFCDlg.h: 头文件
//

#pragma once

#include <memory>
#include "WndKeeper.h"

// CSnapshotMFCDlg 对话框
class CSnapshotMFCDlg : public CDialogEx
{
// 构造
public:
	CSnapshotMFCDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SNAPSHOTMFC_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

	BOOL PreTranslateMessage(MSG*) override;

// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
    afx_msg LRESULT OnDPIMessage(WPARAM wParam, LPARAM lParam);

  public:
	afx_msg void OnBnClickedButtonEnumwnd();
    afx_msg void OnBnClickedButtonEnummonitors();
    afx_msg void OnCbnDropdownComboWndlist();
  
	CListCtrl _ProcLists;
    std::unique_ptr<CImageList> _normal;
    CSize _ImgSz{ 156, 88 };
    CComboBox _comboxAPI;
    afx_msg void OnBnClickedBtnKeep();

	std::unique_ptr<WndKeeper> _keeper{};
    CComboBox _wndListCbbox;
};

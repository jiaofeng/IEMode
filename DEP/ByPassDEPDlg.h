
// ByPassDEPDlg.h : 头文件
//

#pragma once
#include "afxwin.h"


// CByPassDEPDlg 对话框
class CByPassDEPDlg : public CDialogEx
{
// 构造
public:
	CByPassDEPDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_BYPASSDEP_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
  // 控件绑定变量
  CString m_strPath;
  // 控件变量
  CString m_strFirefoxVersion;
  afx_msg void OnBnClickedButtonHandle();
private:
  bool FindFirefox(bool autoFind);
  bool HandleFile(const CString& strPath);
public:
  CEdit m_edtLog;
  void Log(const CString& strMsg);
  afx_msg void OnBnClickedButtonBrowse();
  afx_msg void OnBnClickedButtonAutoHanle();
};

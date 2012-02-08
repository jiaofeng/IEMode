
// ByPassDEPDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"


// CByPassDEPDlg �Ի���
class CByPassDEPDlg : public CDialogEx
{
// ����
public:
	CByPassDEPDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_BYPASSDEP_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
  // �ؼ��󶨱���
  CString m_strPath;
  // �ؼ�����
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

// IEModePlugin.h : main header file for the IEModePlugin DLL
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

#define STR_WINDOW_CLASS_NAME	_T("IEMode")	// CIEHostWindow��������

/** ��CStringת��ΪUTF8�ַ�����
 *  ʹ����Ϻ������delete[]�ͷ��ַ���
 */
char* CStringToUTF8String(const CString &str);

/** ��UTF8�ַ���תΪCString*/
CString UTF8ToCString(const char* szUTF8);

// CIEModePluginApp
// See IEModePlugin.cpp for the implementation of this class
//

/**
* ģ��ƥ������ URL.
* http://my.com/path/file.html#123 �� http://my.com/path/file.html ����Ϊ��ͬһ�� URL
* http://my.com/path/query?p=xyz �� http://my.com/path/query ����Ϊ��ͬһ�� URL
*/
BOOL FuzzyUrlCompare (LPCTSTR lpszUrl1, LPCTSTR lpszUrl2);

class CIEModePluginApp : public CWinApp
{
public:
	CIEModePluginApp();

// Overrides
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
  virtual int ExitInstance();
};

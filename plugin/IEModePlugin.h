// IEModePlugin.h : main header file for the IEModePlugin DLL
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

#define STR_WINDOW_CLASS_NAME	_T("IEMode")	// CIEHostWindow窗口类名

/** 将CString转换为UTF8字符串，
 *  使用完毕后，需调用delete[]释放字符串
 */
char* CStringToUTF8String(const CString &str);

/** 将UTF8字符串转为CString*/
CString UTF8ToCString(const char* szUTF8);

// CIEModePluginApp
// See IEModePlugin.cpp for the implementation of this class
//

/**
* 模糊匹配两个 URL.
* http://my.com/path/file.html#123 和 http://my.com/path/file.html 会认为是同一个 URL
* http://my.com/path/query?p=xyz 和 http://my.com/path/query 不认为是同一个 URL
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

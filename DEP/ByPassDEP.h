
// ByPassDEP.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CByPassDEPApp:
// �йش����ʵ�֣������ ByPassDEP.cpp
//

class CByPassDEPApp : public CWinApp
{
public:
	CByPassDEPApp();

  CString GetAppPath();
// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CByPassDEPApp theApp;
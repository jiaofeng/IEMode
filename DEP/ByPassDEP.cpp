
// ByPassDEP.cpp : ����Ӧ�ó��������Ϊ��
//

#include "stdafx.h"
#include "ByPassDEP.h"
#include "ByPassDEPDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CByPassDEPApp

BEGIN_MESSAGE_MAP(CByPassDEPApp, CWinApp)
  ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CByPassDEPApp ����

CByPassDEPApp::CByPassDEPApp()
{
  // TODO: �ڴ˴���ӹ�����룬
  // ��������Ҫ�ĳ�ʼ�������� InitInstance ��
}


// Ψһ��һ�� CByPassDEPApp ����

CByPassDEPApp theApp;


// CByPassDEPApp ��ʼ��

BOOL CByPassDEPApp::InitInstance()
{
  
  // ���һ�������� Windows XP �ϵ�Ӧ�ó����嵥ָ��Ҫ
  // ʹ�� ComCtl32.dll �汾 6 ����߰汾�����ÿ��ӻ���ʽ��
  //����Ҫ InitCommonControlsEx()�����򣬽��޷��������ڡ�
  INITCOMMONCONTROLSEX InitCtrls;
  InitCtrls.dwSize = sizeof(InitCtrls);
  // ��������Ϊ��������Ҫ��Ӧ�ó�����ʹ�õ�
  // �����ؼ��ࡣ
  InitCtrls.dwICC = ICC_WIN95_CLASSES;
  InitCommonControlsEx(&InitCtrls);

  CWinApp::InitInstance();


  // ���� shell ���������Է��Ի������
  // �κ� shell ����ͼ�ؼ��� shell �б���ͼ�ؼ���
  CShellManager *pShellManager = new CShellManager;

  // ��׼��ʼ��
  // ���δʹ����Щ���ܲ�ϣ����С
  // ���տ�ִ���ļ��Ĵ�С����Ӧ�Ƴ�����
  // ����Ҫ���ض���ʼ������
  // �������ڴ洢���õ�ע�����
  // TODO: Ӧ�ʵ��޸ĸ��ַ�����
  // �����޸�Ϊ��˾����֯��
  SetRegistryKey(_T("Ӧ�ó��������ɵı���Ӧ�ó���"));

  CByPassDEPDlg dlg;
  m_pMainWnd = &dlg;
  INT_PTR nResponse = dlg.DoModal();
  if (nResponse == IDOK)
  {
    // TODO: �ڴ˷��ô����ʱ��
    //  ��ȷ�������رնԻ���Ĵ���
  }
  else if (nResponse == IDCANCEL)
  {
    // TODO: �ڴ˷��ô����ʱ��
    //  ��ȡ�������رնԻ���Ĵ���
  }

  // ɾ�����洴���� shell ��������
  if (pShellManager != NULL)
  {
    delete pShellManager;
  }

  // ���ڶԻ����ѹرգ����Խ����� FALSE �Ա��˳�Ӧ�ó���
  //  ����������Ӧ�ó������Ϣ�á�
  return FALSE;
}

CString CByPassDEPApp::GetAppPath()  

{   
  CString    sPath;   
  GetModuleFileName(NULL,sPath.GetBufferSetLength(MAX_PATH+1),MAX_PATH);   
  sPath.ReleaseBuffer();   
  int    nPos;   
  nPos=sPath.ReverseFind('\\');   
  sPath=sPath.Left(nPos);   
  return sPath;   
}


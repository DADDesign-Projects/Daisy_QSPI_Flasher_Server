
// FlasherServerDlg.h : fichier d'en-tête
//

#pragma once
#include "cServer.h"

// boîte de dialogue de CFlasherServerDlg
class CFlasherServerDlg : public CDialogEx
{
// Construction
public:
	CFlasherServerDlg(CWnd* pParent = nullptr);	// constructeur standard

// Données de boîte de dialogue
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FLASHERSERVER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// Prise en charge de DDX/DDV


// Implémentation
protected:
	HICON m_hIcon;
	CWinThread* m_pFlashThread = nullptr;

	// Fonctions générées de la table des messages
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CStatic m_TitreListeFiles;
	CStatic m_TireListeCom;
	CListBox m_ListeFiles;
	CListBox m_ListeCOM;
	CProgressCtrl m_Progress;
	CEdit m_Edit;
	CButton m_ButtonFlash;
	afx_msg void OnBnClickedAddFile();
	afx_msg void OnBnClickedDeleteFile();
	afx_msg void OnBnClickedFlash();
	afx_msg void OnBnClickedRefresh();
	afx_msg void OnLbnSelchangeListCom();

};

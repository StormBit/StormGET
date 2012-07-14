#pragma once
#include "afxwin.h"


// ReimuGETURLBox dialog

class ReimuGETURLBox : public CDialogEx
{
	DECLARE_DYNAMIC(ReimuGETURLBox)

public:
	ReimuGETURLBox(CWnd* pParent = NULL);   // standard constructor
	virtual ~ReimuGETURLBox();

// Dialog Data
	enum { IDD = IDD_URLBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedAdd();
	CEdit m_URLList;
};

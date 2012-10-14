#pragma once
#include "afxwin.h"


// StormGETURLBox dialog

class StormGETURLBox : public CDialogEx
{
	DECLARE_DYNAMIC(StormGETURLBox)

public:
	StormGETURLBox(CWnd* pParent = NULL);   // standard constructor
	virtual ~StormGETURLBox();

// Dialog Data
	enum { IDD = IDD_URLBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedAdd();
	CEdit m_URLList;
};

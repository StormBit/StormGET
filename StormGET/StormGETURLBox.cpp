// StormGETURLBox.cpp : implementation file
//

#include "stdafx.h"
#include "StormGET.h"
#include "StormGETURLBox.h"
#include "afxdialogex.h"


// StormGETURLBox dialog

IMPLEMENT_DYNAMIC(StormGETURLBox, CDialogEx)

StormGETURLBox::StormGETURLBox(CWnd* pParent /*=NULL*/)
	: CDialogEx(StormGETURLBox::IDD, pParent)
{

}

StormGETURLBox::~StormGETURLBox()
{
}

void StormGETURLBox::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST, m_URLList);
}


BEGIN_MESSAGE_MAP(StormGETURLBox, CDialogEx)
	ON_BN_CLICKED(IDC_ADD, &StormGETURLBox::OnBnClickedAdd)
END_MESSAGE_MAP()


// StormGETURLBox message handlers

CString GetLine(CString &s)
{
	int Pos = s.Find(L"\n");
	CString Line = Pos >= 0 ? s.Left(Pos) : s;
	s = Pos >= 0 ? s.Mid(Pos + 1) : L"";
	return Line;
}

void StormGETURLBox::OnBnClickedAdd()
{	
	CWnd* pwnd = AfxGetMainWnd(); // Pointer to main window
	HWND hWnd = pwnd->GetSafeHwnd();

	CListCtrl* m_FileQueue = (CListCtrl*)pwnd->GetDlgItem(IDC_LIST3);
	CSliderCtrl* m_Connections = (CSliderCtrl*)pwnd->GetDlgItem(IDC_SLIDER1);
	CString FileURLs;

	this->GetDlgItemTextW(IDC_LIST,FileURLs);

	while (!FileURLs.IsEmpty()) {
		LVITEM lvItem;

		int nItem = m_FileQueue->GetItemCount();

		lvItem.mask = LVIF_TEXT;
		lvItem.iItem = nItem;
		nItem++;
		lvItem.iSubItem = 0;

		lvItem.pszText = L"Queued";
		nItem = m_FileQueue->InsertItem(&lvItem);

		CString connPos;
		connPos.Format(L"%i", m_Connections->GetPos());

		m_FileQueue->SetItemText(nItem, 1, connPos);
		m_FileQueue->SetItemText(nItem, 2, GetLine(FileURLs));
	}
}

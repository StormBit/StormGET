// ReimuGETPluginConfig.cpp : implementation file
//

#include "stdafx.h"
#include "ReimuGET.h"
#include "ReimuGETPluginConfig.h"
#include "afxdialogex.h"


// CReimuGETPluginConfig dialog

IMPLEMENT_DYNAMIC(CReimuGETPluginConfig, CDialog)

CReimuGETPluginConfig::CReimuGETPluginConfig(CWnd* pParent /*=NULL*/)
	: CDialog(CReimuGETPluginConfig::IDD, pParent)
{

}

CReimuGETPluginConfig::~CReimuGETPluginConfig()
{
}

void CReimuGETPluginConfig::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CReimuGETPluginConfig, CDialog)
END_MESSAGE_MAP()


// CReimuGETPluginConfig message handlers


BOOL CReimuGETPluginConfig::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

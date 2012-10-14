// StormGETPluginConfig.cpp : implementation file
//

#include "stdafx.h"
#include "StormGET.h"
#include "StormGETPluginConfig.h"
#include "afxdialogex.h"


// CStormGETPluginConfig dialog

IMPLEMENT_DYNAMIC(CStormGETPluginConfig, CDialog)

CStormGETPluginConfig::CStormGETPluginConfig(CWnd* pParent /*=NULL*/)
	: CDialog(CStormGETPluginConfig::IDD, pParent)
{

}

CStormGETPluginConfig::~CStormGETPluginConfig()
{
}

void CStormGETPluginConfig::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CStormGETPluginConfig, CDialog)
END_MESSAGE_MAP()


// CStormGETPluginConfig message handlers


BOOL CStormGETPluginConfig::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

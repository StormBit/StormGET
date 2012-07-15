#pragma once


// CReimuGETPluginConfig dialog

class CReimuGETPluginConfig : public CDialog
{
	DECLARE_DYNAMIC(CReimuGETPluginConfig)

public:
	CReimuGETPluginConfig(CWnd* pParent = NULL);   // standard constructor
	virtual ~CReimuGETPluginConfig();

// Dialog Data
	enum { IDD = IDD_PLUGINCONFIG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
};

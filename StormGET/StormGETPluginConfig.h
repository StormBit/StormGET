#pragma once


// CStormGETPluginConfig dialog

class CStormGETPluginConfig : public CDialog
{
	DECLARE_DYNAMIC(CStormGETPluginConfig)

public:
	CStormGETPluginConfig(CWnd* pParent = NULL);   // standard constructor
	virtual ~CStormGETPluginConfig();

// Dialog Data
	enum { IDD = IDD_PLUGINCONFIG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
};

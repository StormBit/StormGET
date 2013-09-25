
// StormGETDlg.cpp : implementation file
//

#include <process.h>
#include "stdafx.h"
#include "StormGET.h"
#include "StormGETDlg.h"
#include "StormGETURLBox.h"
#include "Resource.h"

#include "afxdialogex.h"

#include <iostream>
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif 

#define BUFSIZE 4096 
 
// Prototypes
UINT ExitStormGET(LPVOID pParam);
UINT InitStormGET(LPVOID pParam);
UINT DownloadFiles(LPVOID pParam);
int wildcmp(const char *wild, const char *string);

bool isDling = false, queueRunning = false, assumeError = true, ExitOK = false, killPlugin = false, inPlugin = false, stopDownloading = false;
int CurrentFile;
CWinThread* pDownloadFiles;

// CAboutDlg dialog used for App About

int CALLBACK SortItemURLs(LPARAM lParam1, LPARAM lParam2, 
	LPARAM lParamSort)
{
	UNREFERENCED_PARAMETER(lParamSort);

	return (int)(lParam1 - lParam2);
}

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CStormGETDlg dialog




CStormGETDlg::CStormGETDlg(CWnd* pParent /*=NULL*/)
	: CTrayDialog(CStormGETDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CStormGETDlg::DoDataExchange(CDataExchange* pDX)
{
	CTrayDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST3, m_FileQueue);
	DDX_Control(pDX, IDC_EDIT1, m_URL);
	DDX_Control(pDX, IDC_SLIDER1, m_Connections);
	DDX_Control(pDX, IDC_PROGRESS1, m_progBar);
}

BEGIN_MESSAGE_MAP(CStormGETDlg, CTrayDialog)

	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON2, &CStormGETDlg::OnBnClickedButton2)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER1, &CStormGETDlg::OnNMCustomdrawSlider1)
	ON_BN_CLICKED(IDC_BUTTON3, &CStormGETDlg::OnBnClickedButton3)
	ON_NOTIFY(LVN_INSERTITEM, IDC_LIST3, &CStormGETDlg::OnLvnInsertitemList3)
//	ON_WM_CLOSE()
ON_WM_CLOSE()
ON_COMMAND(ID_EXIT, &CStormGETDlg::OnMenuExit)
ON_COMMAND(ID_HELP_ABOUTStormGET, &CStormGETDlg::OnHelpAbout)
ON_COMMAND(ID_SESSION_ADDFROMCLIPBOARD, &CStormGETDlg::OnAddFromClipboard)
ON_COMMAND(ID_STOPDOWNLOAD, &CStormGETDlg::OnStopDownload)
ON_COMMAND(ID_RESET, &CStormGETDlg::OnReset)
ON_COMMAND(LOADSESSION, &CStormGETDlg::OnLoadSession)
ON_COMMAND(SAVESESSION, &CStormGETDlg::OnSaveSession)
ON_COMMAND(LOADSESSIONRESET, &CStormGETDlg::OnLoadSessionReset)
END_MESSAGE_MAP()


// CStormGETDlg message handlers

BOOL CStormGETDlg::OnInitDialog()
{
	CTrayDialog::OnInitDialog();

	// Add "About..." menu item to system menu.
	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	HGLOBAL hResourceLoaded;  // handle to loaded resource
    HRSRC   hRes;              // handle/ptr to res. info.
    char    *lpResLock;        // pointer to resource data
    DWORD   dwSizeRes;
	FILE * outputRes;

	CreateDirectory(L"Plugins", NULL);
	CreateDirectory(L"Plugins\\Binaries", NULL);

    hRes = FindResource(NULL,MAKEINTRESOURCE(IDR_BIN2),CString(L"BIN"));
    hResourceLoaded = LoadResource(NULL, hRes);
    lpResLock = (char *) LockResource(hResourceLoaded);
    dwSizeRes = SizeofResource(NULL, hRes);
	if(outputRes = _wfopen(L"Plugins\\host_bandcamp.dll", L"wb")) {
		fwrite ((const char *) lpResLock,1,dwSizeRes,outputRes);
		fclose(outputRes);
	}

	hRes = FindResource(NULL,MAKEINTRESOURCE(IDR_BIN3),CString(L"BIN"));
    hResourceLoaded = LoadResource(NULL, hRes);
    lpResLock = (char *) LockResource(hResourceLoaded);
    dwSizeRes = SizeofResource(NULL, hRes);
	if(outputRes = _wfopen(L"Plugins\\host_xdccget.dll", L"wb")) {
		fwrite ((const char *) lpResLock,1,dwSizeRes,outputRes);
		fclose(outputRes);
	}

	hRes = FindResource(NULL,MAKEINTRESOURCE(IDR_BIN4),CString(L"BIN"));
    hResourceLoaded = LoadResource(NULL, hRes);
    lpResLock = (char *) LockResource(hResourceLoaded);
    dwSizeRes = SizeofResource(NULL, hRes);
	if(outputRes = _wfopen(L"Plugins\\proto_http_aria2.dll", L"wb")) {
		fwrite ((const char *) lpResLock,1,dwSizeRes,outputRes);
		fclose(outputRes);
	}

	m_Connections.SetRange(1, 16);
	m_Connections.SetPos(16);

	LVCOLUMN lvColumn;
	
	lvColumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.cx = 80;
	lvColumn.pszText = L"Status";
	m_FileQueue.InsertColumn(1, &lvColumn);

	lvColumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.cx = 80;
	lvColumn.pszText = L"Connections";
	m_FileQueue.InsertColumn(1, &lvColumn);

	lvColumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.cx = 278;
	lvColumn.pszText = L"URL";
	m_FileQueue.InsertColumn(2, &lvColumn);

	TraySetIcon(IDR_MAINFRAME);
	TraySetToolTip(L"StormGET");

	CWnd* bGetFile = GetDlgItem(IDC_BUTTON3);
	bGetFile->EnableWindow(FALSE);
	bGetFile = GetDlgItem(IDC_BUTTON2);
	bGetFile->EnableWindow(FALSE);

	CWnd* pwnd = AfxGetMainWnd(); // Pointer to main window
	CWinThread* pInitStormGET = AfxBeginThread(InitStormGET,THREAD_PRIORITY_NORMAL);

	typedef bool (*PluginInit)();
	CFileFind InitPlugins;
	BOOL bWorking = InitPlugins.FindFile(L"Plugins\\host_*.dll");
	while (bWorking) {
		bWorking = InitPlugins.FindNextFile();
		HMODULE StormGETPluginDLL = LoadLibrary(CString(L"Plugins\\" + InitPlugins.GetFileName()));
		PluginInit StormGETPluginInit = (PluginInit)GetProcAddress(StormGETPluginDLL,"StormGETPluginInit");
		StormGETPluginInit();
		FreeLibrary(StormGETPluginDLL);
	}
	bWorking = InitPlugins.FindFile(L"Plugins\\proto_*.dll");
	while (bWorking) {
		bWorking = InitPlugins.FindNextFile();
		HMODULE StormGETPluginDLL = LoadLibrary(CString(L"Plugins\\" + InitPlugins.GetFileName()));
		PluginInit StormGETPluginInit = (PluginInit)GetProcAddress(StormGETPluginDLL,"StormGETPluginInit");
		StormGETPluginInit();
		FreeLibrary(StormGETPluginDLL);
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

UINT InitStormGET(LPVOID pParam) {
	CWnd* pwnd = AfxGetMainWnd(); // Pointer to main window
	HWND hWnd = pwnd->GetSafeHwnd();

	CListCtrl* m_FileQueue = (CListCtrl*)pwnd->GetDlgItem(IDC_LIST3);
	CProgressCtrl* m_progBar = (CProgressCtrl*)pwnd->GetDlgItem(IDC_PROGRESS1);

	if( PathFileExists(L"StormGET.csv")) {
		int restore = AfxMessageBox(L"Would you like to restore your previous StormGET session?", MB_YESNO|MB_ICONQUESTION);
		if (restore == IDYES) {
			pwnd->SetDlgItemTextW(IDC_STATUS, CString(L"Restoring session..."));

			FILE * Session = _wfopen(L"StormGET.csv",L"r");

			int c=0;while(!fscanf(Session,"%*[^\n]%*c"))c++;fseek(Session,0,SEEK_SET);

			m_progBar->SetRange32(0, c);

			char line[8192];
			char segment[8192];

			int currLine = 0;
			while(fgets(line,8192,Session)) {
				currLine++;

				int nItem = m_FileQueue->GetItemCount();

				LVITEM lvItem;
				lvItem.mask = LVIF_TEXT;
				lvItem.iItem = nItem;
				nItem++;
				lvItem.iSubItem = 0;
				lvItem.pszText = L"";
				nItem = m_FileQueue->InsertItem(&lvItem);

				strcpy(segment,strtok(line,","));
				m_FileQueue->SetItemText(nItem, 0, CString(segment));
				strcpy(segment,strtok(NULL,","));
				m_FileQueue->SetItemText(nItem, 1,CString(segment));
				strcpy(segment,strtok(NULL,"\n"));
				m_FileQueue->SetItemText(nItem, 2, CString(segment));

				m_progBar->SetPos(currLine);
			}
		}
	}
	
	CWnd* bGetFile = pwnd->GetDlgItem(IDC_BUTTON3);
	bGetFile->EnableWindow(TRUE);
	bGetFile = pwnd->GetDlgItem(IDC_BUTTON2);
	bGetFile->EnableWindow(TRUE);
	m_progBar->SetRange(0, 100);
	m_progBar->SetPos(0);

	pwnd->SetDlgItemTextW(IDC_STATUS, CString(L""));

	return 0;
}

void CStormGETDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CTrayDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CStormGETDlg::OnPaint()
{

	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CTrayDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CStormGETDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CStormGETDlg::OnBnClickedButton2()
{
	// Add URL:

	CString dlgURL;
	GetDlgItemText(IDC_EDIT1, dlgURL);
	
	if (dlgURL != L"") {
		int nItem = m_FileQueue.GetItemCount();

		SetDlgItemText(IDC_EDIT1, L"");

		LVITEM lvItem;

		lvItem.mask = LVIF_TEXT;
		lvItem.iItem = ++nItem;
		nItem++;
		lvItem.iSubItem = 0;

		lvItem.pszText = L"Queued";
		nItem = m_FileQueue.InsertItem(&lvItem);

		CString connPos;
		connPos.Format(L"%i", m_Connections.GetPos());

		m_FileQueue.SetItemText(nItem, 1, connPos);
		m_FileQueue.SetItemText(nItem, 2, dlgURL);
	} else {
		AfxMessageBox(L"Please enter a URL.");
	}
}


void CStormGETDlg::OnNMCustomdrawSlider1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);


	CString connPos;
	connPos.Format(L"%i", m_Connections.GetPos());

	SetDlgItemText(IDC_CONN, connPos);
	
	// TODO: Add your control notification handler code here
	
	
	*pResult = 0;
}

UINT DownloadFiles(LPVOID pParam) {	
	CWnd* pwnd = AfxGetMainWnd(); // Pointer to main window

	CListCtrl* m_FileQueue = (CListCtrl*)pwnd->GetDlgItem(IDC_LIST3);

	CButton* bGetFile;

	bGetFile = (CButton*)pwnd->GetDlgItem(IDC_BUTTON3);
	bGetFile->EnableWindow(FALSE);

	CProgressCtrl* m_Prog = (CProgressCtrl*)pwnd->GetDlgItem(IDC_PROGRESS1);

	queueRunning = true;

	for (int i = 0; i < m_FileQueue->GetItemCount(); i++){
		if (stopDownloading) { queueRunning = false; break; }
		CurrentFile = i;
		if (m_FileQueue->GetItemText(i, 0) != L"Done") {
			char* conditions;
			// Loop through and poll all of our .dlls, and see if any will handle this.
			typedef char* (*PluginEnumerateConditions)();
			CFileFind EnumeratePluginConditions;
			BOOL bWorking = EnumeratePluginConditions.FindFile(L"Plugins\\host_*.dll");
			BOOL PluginFound = false;
			CString PluginDLL;

			CString DownloadDir;
			pwnd->GetDlgItemText(IDC_MFCEDITBROWSE1, DownloadDir);

			CStringA FileURL = CStringA(m_FileQueue->GetItemText(i, 2).GetBuffer());

			while (bWorking) {
				bWorking = EnumeratePluginConditions.FindNextFile();
				HMODULE StormGETPluginDLL = LoadLibrary(CString(L"Plugins\\" + EnumeratePluginConditions.GetFileName()));
				PluginEnumerateConditions StormGETPluginEnumerateConditions = (PluginEnumerateConditions)GetProcAddress(StormGETPluginDLL,"StormGETPluginEnumerateConditions");
				conditions = StormGETPluginEnumerateConditions();
				if (wildcmp(conditions, (const char *)FileURL)) {
					PluginFound = true;
					PluginDLL = EnumeratePluginConditions.GetFileName();
				}
				FreeLibrary(StormGETPluginDLL);
			}
			if (!PluginFound) {
				bWorking = EnumeratePluginConditions.FindFile(L"Plugins\\proto_*.dll");

				while (bWorking) {
					bWorking = EnumeratePluginConditions.FindNextFile();
					HMODULE StormGETPluginDLL = LoadLibrary(CString(L"Plugins\\" + EnumeratePluginConditions.GetFileName()));
					PluginEnumerateConditions StormGETPluginEnumerateConditions = (PluginEnumerateConditions)GetProcAddress(StormGETPluginDLL,"StormGETPluginEnumerateConditions");
					conditions = StormGETPluginEnumerateConditions();
					if (wildcmp(conditions, (const char *)FileURL)) {
						PluginFound = true;
						PluginDLL = EnumeratePluginConditions.GetFileName();
					}
					FreeLibrary(StormGETPluginDLL);
				}
			}
			if (PluginFound) {
				inPlugin = true;

				typedef void (*PluginDownload)(CString, CString);
				typedef char * (*PluginStatus)();
				typedef int (*PluginProgress)();
				typedef bool (*PluginStillRunning)();
				typedef char * (*PluginStatusLine2)();
				typedef char * (*PluginGetName)();
				typedef bool (*PluginStop)();

				HMODULE StormGETPluginDLL = LoadLibrary(CString(L"Plugins\\" + PluginDLL));
				PluginDownload StormGETPluginDownload = (PluginDownload)GetProcAddress(StormGETPluginDLL,"StormGETPluginDownload");
				PluginStatus StormGETPluginGetStatus = (PluginStatus)GetProcAddress(StormGETPluginDLL,"StormGETPluginGetStatus");
				PluginStatusLine2 StormGETPluginGetStatusLine2 = (PluginStatus)GetProcAddress(StormGETPluginDLL,"StormGETPluginGetStatusLine2");
				PluginGetName StormGETPluginGetName = (PluginStatus)GetProcAddress(StormGETPluginDLL,"StormGETPluginGetName");
				PluginProgress StormGETPluginGetProgress = (PluginProgress)GetProcAddress(StormGETPluginDLL,"StormGETPluginGetProgress");
				PluginStillRunning StormGETPluginStillRunning = (PluginStillRunning)GetProcAddress(StormGETPluginDLL,"StormGETPluginStillRunning");
				PluginStop StormGETPluginStop = (PluginStop)GetProcAddress(StormGETPluginDLL,"StormGETPluginStop");

				m_FileQueue->SetItemText(i, 0, L"Downloading");
				if (StormGETPluginGetStatusLine2() != NULL) pwnd->SetDlgItemTextW(IDC_ETA, CString(StormGETPluginGetStatusLine2()));
				else if (StormGETPluginGetName() != NULL) pwnd->SetDlgItemTextW(IDC_ETA, L"Downloading with " + CString(StormGETPluginGetName()));
				else pwnd->SetDlgItemTextW(IDC_ETA, L"Downloading with plugin " + PluginDLL);
				pwnd->SetDlgItemTextW(IDC_STATUS, L"Initializing...");

				StormGETPluginDownload(m_FileQueue->GetItemText(i, 2),DownloadDir);

				bool exitCode = true;

				while(1) { // Main loop to update the interface. Should update at minimum every 500ms. 
					if (!StormGETPluginStillRunning()) {
						break;
					}

					Sleep(100);
					if (killPlugin) {
						StormGETPluginStop();

						exitCode = false;
						killPlugin = false;
					}

					char * cBuffer;
					cBuffer = StormGETPluginGetStatus();

					if (StormGETPluginGetStatusLine2() != NULL) pwnd->SetDlgItemTextW(IDC_ETA, CString(StormGETPluginGetStatusLine2()));
					pwnd->SetDlgItemTextW(IDC_STATUS, CString(cBuffer));

					m_Prog->SetPos(StormGETPluginGetProgress());
				}

				inPlugin = false;

				m_FileQueue->SetItemText(i, 0, L"Cleaning up...");
				m_Prog->SetPos(0);
				Sleep(1000);
				FreeLibrary(StormGETPluginDLL);

				if (exitCode) {
					m_FileQueue->SetItemText(i, 0, L"Done");
				} else {
					m_FileQueue->SetItemText(i, 0, L"Error!");
				}

				pwnd->SetDlgItemTextW(IDC_STATUS, L"");
				pwnd->SetDlgItemTextW(IDC_ETA, L"");

			} else {
				AfxMessageBox(L"No plugin was found that can handle the URL Provided.");
			}
		}
	}

	queueRunning = false;

	bGetFile = (CButton*)pwnd->GetDlgItem(IDC_BUTTON3);
	bGetFile->EnableWindow(TRUE);

	return 1;
}

void CStormGETDlg::OnBnClickedButton3()
{
	CWnd* pwnd = AfxGetMainWnd(); // Pointer to main window
	pDownloadFiles = AfxBeginThread(DownloadFiles,pwnd,THREAD_PRIORITY_NORMAL);
}


void CStormGETDlg::OnLvnInsertitemList3(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;

	//m_FileQueue.SortItems(SortItemURLs, 0);
}


BOOL CStormGETDlg::PreTranslateMessage(MSG* pMsg)
{
	if ((pMsg->message == WM_KEYDOWN)) {
		if (pMsg->wParam == VK_RETURN) {
			CStormGETDlg::OnBnClickedButton2();
			return TRUE;
		}
	}

	return CTrayDialog::PreTranslateMessage(pMsg);
}

void CStormGETDlg::OnClose()
{
	CTrayDialog::OnClose();
}

void CStormGETDlg::OnCancel()
{
	bool doQuitNow = false;

	if (queueRunning) {
		int doQuit = AfxMessageBox(L"Are you sure you want to quit?", MB_YESNO|MB_ICONQUESTION);
		if (doQuit == IDYES) {
			doQuitNow = true;
		}
	} else {
		doQuitNow = true;
	}

	if (doQuitNow == true) {
		CWinThread* pExitStormGET = AfxBeginThread(ExitStormGET,THREAD_PRIORITY_NORMAL);
	}
}

UINT ExitStormGET(LPVOID pParam) {	
	CWnd* pwnd = AfxGetMainWnd(); // Pointer to main window
	HWND hWnd = pwnd->GetSafeHwnd();

	CWnd* bGetFile = pwnd->GetDlgItem(IDC_BUTTON3);
	bGetFile->EnableWindow(FALSE);
	bGetFile = pwnd->GetDlgItem(IDC_BUTTON2);
	bGetFile->EnableWindow(FALSE);

	pwnd->SetDlgItemTextW(IDC_ETA, CString(L"Shutting down StormGET..."));
	ExitOK = false;

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	ZeroMemory( &pi, sizeof(pi) );

	pwnd->SetDlgItemTextW(IDC_STATUS, CString(L"Cleaning up: Stopping plugins..."));

	killPlugin = true;
	while (inPlugin) {
		Sleep(10);
	}

	pwnd->SetDlgItemTextW(IDC_STATUS, CString(L"Cleaning up: Cleaning up plugins..."));

	typedef bool (*PluginExit)();
	CFileFind ExitPlugins;
	BOOL bWorking = ExitPlugins.FindFile(L"Plugins\\host_*.dll");
	while (bWorking) {
		bWorking = ExitPlugins.FindNextFile();
		HMODULE StormGETPluginDLL = LoadLibrary(CString(L"Plugins\\" + ExitPlugins.GetFileName()));
		PluginExit StormGETPluginExit = (PluginExit)GetProcAddress(StormGETPluginDLL,"StormGETPluginExit");
		StormGETPluginExit();
		FreeLibrary(StormGETPluginDLL);
	}

	bWorking = ExitPlugins.FindFile(L"Plugins\\proto_*.dll");
	while (bWorking) {
		bWorking = ExitPlugins.FindNextFile();
		HMODULE StormGETPluginDLL = LoadLibrary(CString(L"Plugins\\" + ExitPlugins.GetFileName()));
		PluginExit StormGETPluginExit = (PluginExit)GetProcAddress(StormGETPluginDLL,"StormGETPluginExit");
		StormGETPluginExit();
		FreeLibrary(StormGETPluginDLL);
	}

	DeleteFile(L"Plugins\\host_bandcamp.dll");
	DeleteFile(L"Plugins\\host_xdccget.dll");
	DeleteFile(L"Plugins\\proto_http_aria2.dll");
	RemoveDirectory(L"Plugins\\Binaries");
	RemoveDirectory(L"Plugins");

	CListCtrl* m_FileQueue = (CListCtrl*)pwnd->GetDlgItem(IDC_LIST3);
	DeleteFile(L"StormGET.csv");
	if (m_FileQueue->GetItemCount()) {
		pwnd->SetDlgItemTextW(IDC_STATUS, CString(L"Saving session..."));
		CProgressCtrl* m_Prog = (CProgressCtrl*)pwnd->GetDlgItem(IDC_PROGRESS1);
		m_Prog->SetRange32(0, m_FileQueue->GetItemCount());

		CWnd* pwnd = AfxGetMainWnd(); // Pointer to main window
		CListCtrl* m_FileQueue = (CListCtrl*)pwnd->GetDlgItem(IDC_LIST3);
		
		FILE* fSession = _wfopen(L"StormGET.csv",L"w");
		
		for (int i = 0; i < m_FileQueue->GetItemCount(); i++){
			fputws(CString(m_FileQueue->GetItemText(i, 0) + L"," + m_FileQueue->GetItemText(i, 1) + L"," + m_FileQueue->GetItemText(i, 2) + L"\n"),fSession);
			m_Prog->SetPos(i);
		}
		fclose(fSession);
	}

	pwnd->SetDlgItemTextW(IDC_STATUS, CString(L"Exiting..."));

	exit(0);
}

void CStormGETDlg::OnMenuExit()
{
	CWinThread* pExitStormGET = AfxBeginThread(ExitStormGET,THREAD_PRIORITY_NORMAL);
}




void CStormGETDlg::OnHelpAbout()
{
	CAboutDlg dlgAbout;
	dlgAbout.DoModal();
}


void CStormGETDlg::OnAddFromClipboard()
{
	StormGETURLBox* AddFromClipboard;
	AddFromClipboard = new StormGETURLBox();

	AddFromClipboard->DoModal();
}

void CStormGETDlg::OnStopDownload()
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	ZeroMemory( &pi, sizeof(pi) );

	CListCtrl* m_FileQueue = (CListCtrl*)GetDlgItem(IDC_LIST3);

	stopDownloading = true;

	killPlugin = true;
	SetDlgItemTextW(IDC_STATUS, L"Stopping plugins...");
	if (inPlugin) while (killPlugin);

	while(queueRunning);
	stopDownloading = false;

	SetDlgItemTextW(IDC_STATUS, L"");
	SetDlgItemTextW(IDC_ETA, L"");
}


void CStormGETDlg::OnReset()
{
	CStormGETDlg::OnStopDownload();

	CListCtrl* m_FileQueue = (CListCtrl*)GetDlgItem(IDC_LIST3);

	m_FileQueue->DeleteAllItems();
}

void CStormGETDlg::OnLoadSession()
{
	CListCtrl* m_FileQueue = (CListCtrl*)GetDlgItem(IDC_LIST3);
	CProgressCtrl* m_progBar = (CProgressCtrl*)GetDlgItem(IDC_PROGRESS1);

	TCHAR szFilters[]= _T("StormGET CSV Session File (*.csv)|*.csv|All Files (*.*)|*.*||");
	CFileDialog fileDlg(true, _T("csv"), _T("StromGET.csv"), OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, szFilters);

	if(fileDlg.DoModal() == IDOK) {
		CString pathName = fileDlg.GetPathName();
		
		SetDlgItemTextW(IDC_STATUS, L"Restoring session...");

		FILE * Session = _wfopen(pathName,L"r");

		int c=0;while(!fscanf(Session,"%*[^\n]%*c"))c++;fseek(Session,0,SEEK_SET);

		INT32 nLower, nUpper;
		m_progBar->GetRange( nLower, nUpper );

		m_progBar->SetRange32(0, c);

		char line[8192];
		char segment[8192];

		int currLine = 0;
		while(fgets(line,8192,Session)) {
			currLine++;

			int nItem = m_FileQueue->GetItemCount();

			LVITEM lvItem;
			lvItem.mask = LVIF_TEXT;
			lvItem.iItem = nItem;
			nItem++;
			lvItem.iSubItem = 0;
			lvItem.pszText = L"";
			nItem = m_FileQueue->InsertItem(&lvItem);

			strcpy(segment,strtok(line,","));
			m_FileQueue->SetItemText(nItem, 0, CString(segment));
			strcpy(segment,strtok(NULL,","));
			m_FileQueue->SetItemText(nItem, 1,CString(segment));
			strcpy(segment,strtok(NULL,"\n"));
			m_FileQueue->SetItemText(nItem, 2, CString(segment));

			m_progBar->SetPos(currLine);
		}

		SetDlgItemTextW(IDC_STATUS, L"");
		m_progBar->SetRange32(nLower, nUpper);
		m_progBar->SetPos(0);
		stopDownloading = false;
	}
}

void CStormGETDlg::OnLoadSessionReset()
{
	CStormGETDlg::OnReset();

	CStormGETDlg::OnLoadSession();
}

void CStormGETDlg::OnSaveSession()
{
	CListCtrl* m_FileQueue = (CListCtrl*)GetDlgItem(IDC_LIST3);

	if (m_FileQueue->GetItemCount()) {
		TCHAR szFilters[]= _T("StormGET CSV Session File (*.csv)|*.csv|All Files (*.*)|*.*||");
		CFileDialog fileDlg(false, _T("csv"), _T("StromGET.csv"), NULL, szFilters);

		if(fileDlg.DoModal() == IDOK) {
			CString pathName = fileDlg.GetPathName();
		
			SetDlgItemTextW(IDC_STATUS, CString(L"Saving session..."));
			CProgressCtrl* m_Prog = (CProgressCtrl*)GetDlgItem(IDC_PROGRESS1);
			
			INT32 nLower, nUpper;
			m_Prog->GetRange( nLower, nUpper );

			m_Prog->SetRange32(0, m_FileQueue->GetItemCount());
		
			FILE* fSession = _wfopen(CString(pathName),L"w");
		
			for (int i = 0; i < m_FileQueue->GetItemCount(); i++){
				fputws(CString(m_FileQueue->GetItemText(i, 0) + L"," + m_FileQueue->GetItemText(i, 1) + L"," + m_FileQueue->GetItemText(i, 2) + L"\n"),fSession);
				m_Prog->SetPos(i);
			}
			fclose(fSession);
			m_Prog->SetPos(0);
			SetDlgItemTextW(IDC_STATUS, CString(L""));

			m_Prog->SetRange32(nLower, nUpper);
			m_Prog->SetPos(0);
		}
	} else {
		AfxMessageBox(L"You cannot save an empty queue!");
	}
}




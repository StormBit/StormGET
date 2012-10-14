
// StormGETDlg.cpp : implementation file
//

#include <process.h>
#include "stdafx.h"
#include "StormGET.h"
#include "StormGETDlg.h"
#include "StormGETURLBox.h"
#include "StormGETPluginConfig.h"
#include "Resource.h"

#include "afxdialogex.h"

#include <iostream>
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif 

#define BUFSIZE 4096 
 
HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;

HANDLE g_hInputFile = NULL;

// Prototypes
UINT DownloadFiles(LPVOID pParam);
UINT ParseOutput(LPVOID pParam);
UINT ExitStormGET(LPVOID pParam);
UINT InitStormGET(LPVOID pParam);
int wildcmp(const char *wild, const char *string);

bool isDling = false, queueRunning = false, assumeError = true, killAria = false, ExitOK = false;
int CurrentFile, Aria2PID = 0;

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
ON_COMMAND(ID_PLUGINCONFIG, &CStormGETDlg::OnPluginConfig)
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
    hRes = FindResource(NULL,MAKEINTRESOURCE(IDR_BIN1),CString(L"BIN"));
    hResourceLoaded = LoadResource(NULL, hRes);
    lpResLock = (char *) LockResource(hResourceLoaded);
    dwSizeRes = SizeofResource(NULL, hRes);
	FILE * outputRes;
	if(outputRes = _wfopen(L"StormGET_temp_aria2c.exe", L"wb")) {
		fwrite ((const char *) lpResLock,1,dwSizeRes,outputRes);
		fclose(outputRes);
	}

	CreateDirectory(L"Plugins", NULL);
	
    hRes = FindResource(NULL,MAKEINTRESOURCE(IDR_BIN2),CString(L"BIN"));
    hResourceLoaded = LoadResource(NULL, hRes);
    lpResLock = (char *) LockResource(hResourceLoaded);
    dwSizeRes = SizeofResource(NULL, hRes);
	if(outputRes = _wfopen(L"Plugins\\host_bandcamp.dll", L"wb")) {
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

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	SECURITY_ATTRIBUTES sa; 

	sa.nLength = sizeof(SECURITY_ATTRIBUTES); 
	sa.bInheritHandle = TRUE; 
	sa.lpSecurityDescriptor = NULL; 

	if ( ! CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &sa, 0) ) 
	exit(1); 

	// Ensure the read handle to the pipe for STDOUT is not inherited.

	if ( ! SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0) )
	exit(1); 

	// Create a pipe for the child process's STDIN. 

	if (! CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &sa, 0)) 
	exit(1); 

	// Ensure the write handle to the pipe for STDIN is not inherited. 

	if ( ! SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) )
	exit(1); 

	CButton* bGetFile;

	bGetFile = (CButton*)pwnd->GetDlgItem(IDC_BUTTON3);
	bGetFile->EnableWindow(FALSE);

	CProgressCtrl* m_Prog = (CProgressCtrl*)pwnd->GetDlgItem(IDC_PROGRESS1);

	queueRunning = true;

	for (int i = 0; i < m_FileQueue->GetItemCount(); i++){
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
			if (PluginFound) {
				typedef void (*PluginDownload)(CString, CString);
				typedef char * (*PluginStatus)();
				typedef int (*PluginProgress)();
				typedef bool (*PluginStillRunning)();
				HMODULE StormGETPluginDLL = LoadLibrary(CString(L"Plugins\\" + PluginDLL));
				PluginDownload StormGETPluginDownload = (PluginDownload)GetProcAddress(StormGETPluginDLL,"StormGETPluginDownload");
				PluginStatus StormGETPluginGetStatus = (PluginStatus)GetProcAddress(StormGETPluginDLL,"StormGETPluginGetStatus");
				PluginProgress StormGETPluginGetProgress = (PluginProgress)GetProcAddress(StormGETPluginDLL,"StormGETPluginGetProgress");
				PluginStillRunning StormGETPluginStillRunning = (PluginStillRunning)GetProcAddress(StormGETPluginDLL,"StormGETPluginStillRunning");
				m_FileQueue->SetItemText(i, 0, L"Downloading");
				pwnd->SetDlgItemTextW(IDC_ETA, L"Downloading with plugin " + PluginDLL);
				pwnd->SetDlgItemTextW(IDC_STATUS, L"Initializing...");

				StormGETPluginDownload(m_FileQueue->GetItemText(i, 2),DownloadDir);

				bool exitCode = true;

				while(1) { // Main loop to update the interface. Should update at minimum every 500ms. 
					if (!StormGETPluginStillRunning()) {
						break;
					}

					char * cBuffer;
					cBuffer = StormGETPluginGetStatus();

					pwnd->SetDlgItemTextW(IDC_STATUS, CString(cBuffer));

					m_Prog->SetPos(StormGETPluginGetProgress());
				}

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
				DWORD exitCode = 0;
				ZeroMemory( &si, sizeof(si) );
				si.cb = sizeof(si);
				ZeroMemory( &pi, sizeof(pi) );

				si.hStdError = g_hChildStd_OUT_Wr;
				si.hStdOutput = g_hChildStd_OUT_Wr;
				si.hStdInput = g_hChildStd_IN_Rd;

				si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES; // STARTF_USESTDHANDLES is Required.
				si.wShowWindow = SW_HIDE; // Prevents cmd window from flashing. Requires STARTF_USESHOWWINDOW in dwFlags.
			
				if (DownloadDir == L"") {
					DownloadDir = L".";
				}

				int Value = CreateProcess(NULL, CString(L"StormGET_temp_aria2c.exe --file-allocation=none --check-certificate=false --dir \"" + DownloadDir + L"\" --max-connection-per-server " + m_FileQueue->GetItemText(i, 1) + L" --min-split-size 1M --split " + m_FileQueue->GetItemText(i, 1) + L" " + m_FileQueue->GetItemText(i, 2)).GetBuffer(), NULL, NULL, true, 0, NULL, NULL, &si, &pi);
				
				Aria2PID = pi.dwProcessId;

				m_FileQueue->SetItemText(i, 0, L"Downloading");
			
				CWinThread* pParseOutput = AfxBeginThread(ParseOutput,THREAD_PRIORITY_NORMAL);
		
				WaitForSingleObject(pi.hProcess, INFINITE);

				Aria2PID = 0;

				GetExitCodeProcess(pi.hProcess,&exitCode);

				if (exitCode == 0) {
					assumeError = false;
				}

				pwnd->SetDlgItemTextW(IDC_ETA, L"");
				pwnd->SetDlgItemTextW(IDC_STATUS, L"");
				m_Prog->SetPos(0);
			
				if (assumeError == false) {
					m_FileQueue->SetItemText(i, 0, L"Done");
				} else { 
					m_FileQueue->SetItemText(i, 0, L"Error!");
				}
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
	CWinThread* pDownloadFiles = AfxBeginThread(DownloadFiles,pwnd,THREAD_PRIORITY_NORMAL);
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

UINT ParseOutput(LPVOID pParam) {	
	CWnd* pwnd = AfxGetMainWnd(); // Pointer to main window
	HWND hWnd = pwnd->GetSafeHwnd();

	CListCtrl* m_FileQueue = (CListCtrl*)pwnd->GetDlgItem(IDC_LIST3);

	char cBuffer[BUFSIZE];
	char *cLinePos, *cLine;
	CString currLine;

	char * cToken;
	
	CString Downloaded, Total, Percent, Speed, ETA, ETAFormatted, Connections, FileCurrent, FileTotal;
	char *downloaded, *total, *percent, *speed, *eta, *connections;
	int Progress = 0;

	CProgressCtrl* m_Prog = (CProgressCtrl*)pwnd->GetDlgItem(IDC_PROGRESS1);

	DWORD dwRead; 
	BOOL bSuccess = FALSE;
	HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	for (;;) { 
		ZeroMemory(&cBuffer,BUFSIZE);

		bSuccess = ReadFile( g_hChildStd_OUT_Rd, cBuffer, BUFSIZE, &dwRead, NULL);
		if( ! bSuccess || dwRead == 0 ) break; 
			   
		for (cLine = strtok_s(cBuffer, "\r\n", &cLinePos); cLine; cLine = strtok_s(NULL, "\r\n", &cLinePos)) {
			if (cLine[0] == '[' && cLine[1] == '#') {
				cToken = strtok (cLine,":");
					
				downloaded = strtok(NULL,"/");
				if (downloaded != NULL) {
					Downloaded = CString(downloaded);
				}
					
				total = strtok(NULL,"(");
				if (total != NULL) {
					Total = CString(total);
				}

				percent = strtok(NULL,"%");
				if (percent != NULL) {
					Percent = CString(percent);
					Progress = _wtoi(Percent);
				}
						
				strtok(NULL,":");

				connections = strtok(NULL," ");
				if (connections != NULL) {
					Connections = CString(connections);
				}

				strtok(NULL,":");
				
				speed = strtok(NULL," ");
				if (speed != NULL) {
					Speed = CString(speed);
				}

				strtok(NULL,":");
				
				eta = strtok(NULL,"]");
				if (eta != NULL) {
					ETA = CString(eta);
					ETAFormatted = L"about ";
					CString ETAToken;
					CString Token1 = L"", Token2 = L"", Token3 = L"";
					int curPos = 0, numTokens = 0;

					ETAToken = ETA.Tokenize(_T("hms"),curPos);
					while (ETAToken != _T("")) {
						numTokens++;
						if (numTokens == 1) {
							Token1 = ETAToken;
						} else if (numTokens == 2) {
							Token2 = ETAToken;
						} else if (numTokens == 3) {
							Token3 = ETAToken;
						}

						ETAToken = ETA.Tokenize(_T("hms"), curPos);
					}

					if (Token3.GetLength() > 0) {
						ETAFormatted += CString(Token1 + L" hours, " + Token2 + L" minutes, " + Token3 + L" seconds remaining");
					} else if (Token2.GetLength() > 0) {
						ETAFormatted += CString(Token1 + L" minutes, " + Token2 + L" seconds remaining");
					} else {
						ETAFormatted += CString(Token1 + L" seconds remaining");
					}
				}

				FileTotal.Format(L"%d", m_FileQueue->GetItemCount());
				FileCurrent.Format(L"%d", CurrentFile + 1);
			}
		}
		m_Prog->SetPos(Progress);

		pwnd->SetDlgItemTextW(IDC_STATUS, CString(L"[" + Percent + L"%] Downloaded " + Downloaded + L"/" + Total + L" at " + Speed + L" with " + Connections + L" connection(s)."));
		pwnd->SetDlgItemTextW(IDC_ETA, CString(L"Downloading file " + FileCurrent + L" of " + FileTotal + L", " + ETAFormatted));
	}

	m_Prog->SetPos(0);

	pwnd->SetDlgItemTextW(IDC_STATUS, CString(L""));
	pwnd->SetDlgItemTextW(IDC_ETA, CString(L""));

	return 1;
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
	killAria = true;

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	ZeroMemory( &pi, sizeof(pi) );

	pwnd->SetDlgItemTextW(IDC_STATUS, CString(L"Cleaning up..."));

	if (Aria2PID != 0) {
		CString pid;
		pid.Format(L"%d",Aria2PID);

		CreateProcess(NULL, CString(L"taskkill.exe /F /PID " + pid).GetBuffer(), NULL, NULL, false, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
		WaitForSingleObject(pi.hProcess, INFINITE);
	}

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

	DeleteFile(L"StormGET_temp_aria2c.exe");
	DeleteFile(L"Plugins\\host_bandcamp.dll");
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

	if (Aria2PID != 0) {
		CString pid;
		pid.Format(L"%d",Aria2PID);
		
		CreateProcess(NULL, CString(L"taskkill.exe /F /PID " + pid).GetBuffer(), NULL, NULL, false, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
		WaitForSingleObject(pi.hProcess, INFINITE);
	}

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

	SetDlgItemTextW(IDC_STATUS, L"");
	SetDlgItemTextW(IDC_ETA, L"");
}


void CStormGETDlg::OnReset()
{
	CStormGETDlg::OnStopDownload();

	CListCtrl* m_FileQueue = (CListCtrl*)GetDlgItem(IDC_LIST3);

	m_FileQueue->DeleteAllItems();
}


void CStormGETDlg::OnPluginConfig()
{
	CStormGETPluginConfig* ConfigurePlugins;
	ConfigurePlugins = new CStormGETPluginConfig();

	ConfigurePlugins->DoModal();
}

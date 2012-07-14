// host_bandcamp.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define BUFSIZE 4096 
 
HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;

HANDLE g_hInputFile = NULL;

char cBuffer[BUFSIZE];
char cBufferArchive[BUFSIZE];

bool CheckOutput = false, Downloading = false;;

int lolwat = 0;

UINT ParseOutput(LPVOID pParam) {
	Downloading = true;

	DWORD dwRead; 
	BOOL bSuccess = FALSE;
	HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	for (;;) { 
		while (CheckOutput) {
			Sleep(50);
		}

		ZeroMemory(&cBuffer,BUFSIZE);

		bSuccess = ReadFile( g_hChildStd_OUT_Rd, cBuffer, BUFSIZE, &dwRead, NULL);
		if( ! bSuccess || dwRead == 0 ) break; 

		CheckOutput = true;
	}

	Downloading = false;

	return 0;
}

extern "C" _declspec(dllexport) bool ReimuGETPluginInit() {
	return TRUE;
}

extern "C" _declspec(dllexport) bool ReimuGETPluginExit() {
	return TRUE;
}

extern "C" _declspec(dllexport) bool ReimuGETPluginConfigure() {
	AfxMessageBox(L"Configuration window goes here");
	return TRUE;
}

extern "C" _declspec(dllexport) PROCESS_INFORMATION ReimuGETPluginDownload(CString FileURL, CString DownloadDir) {
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

	DWORD exitCode = 0;
	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	ZeroMemory( &pi, sizeof(pi) );

	si.hStdError = g_hChildStd_OUT_Wr;
	si.hStdOutput = g_hChildStd_OUT_Wr;
	si.hStdInput = g_hChildStd_IN_Rd;

	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES; // STARTF_USESTDHANDLES is Required.
	si.wShowWindow = SW_HIDE; // Prevents cmd window from flashing. Requires STARTF_USESHOWWINDOW in dwFlags.

	int Value = CreateProcess(NULL, CString(L"Plugins\\host_bandcamp\\bandcampdl.exe " + FileURL).GetBuffer(), NULL, NULL, true, 0, NULL, NULL, &si, &pi);

	CWinThread* pParseOutput = AfxBeginThread(ParseOutput,THREAD_PRIORITY_NORMAL);

	//GetExitCodeProcess(pi.hProcess,&exitCode);

	//if (exitCode == 0) {
	//	return true;
	//}

	return pi;
}

extern "C" _declspec(dllexport) char* ReimuGETPluginGetStatus() {
	if (!CheckOutput) {
		Sleep(500);
	}

	if ((unsigned)strlen(cBuffer) > 16) {
		strcpy(cBufferArchive,cBuffer);
	}

	CheckOutput = false;

	return cBufferArchive;
}

extern "C" _declspec(dllexport) int ReimuGETPluginGetProgress() {
	return 0;
	


	lolwat++;
	return lolwat;
}

extern "C" _declspec(dllexport) char * ReimuGETPluginEnumerateConditions() {
	return "http://*.bandcamp.com/*";
}
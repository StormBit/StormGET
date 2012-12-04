// host_bandcamp.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define BUFSIZE 128 
 
HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;

PROCESS_INFORMATION pi;

HANDLE g_hInputFile = NULL;

char cBuffer[BUFSIZE];
char cBufferArchive[BUFSIZE];

bool CheckOutput = false, Downloading = false;

int BandcampDLPID = 0;

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

		char * pch;
		pch = strtok(cBuffer,"\n");
		if (pch != NULL) pch = strtok(NULL,"\n");
		if (pch != NULL) strcpy(cBuffer,pch);
		if (strlen(cBuffer) > 5) CheckOutput = true;
	}

	Downloading = false;

	return 0;
}

HMODULE GetCurrentModuleHandle() {
    HMODULE hMod = NULL;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&GetCurrentModuleHandle),&hMod);
     return hMod;
}

extern "C" _declspec(dllexport) bool StormGETPluginInit() {
	HGLOBAL hResourceLoaded;  // handle to loaded resource
    HRSRC   hRes;              // handle/ptr to res. info.
    char    *lpResLock;        // pointer to resource data
    DWORD   dwSizeRes;
    hRes = FindResource(GetCurrentModuleHandle(),MAKEINTRESOURCE(IDR_BIN1),CString(L"BIN"));
    hResourceLoaded = LoadResource(GetCurrentModuleHandle(), hRes);
    lpResLock = (char *) LockResource(hResourceLoaded);
    dwSizeRes = SizeofResource(GetCurrentModuleHandle(), hRes);
	FILE * outputRes;
	if(outputRes = _wfopen(L"xdccget.exe", L"wb")) {
		fwrite ((const char *) lpResLock,1,dwSizeRes,outputRes);
		fclose(outputRes);
	}

	return TRUE;
}

extern "C" _declspec(dllexport) bool StormGETPluginExit() {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	ZeroMemory( &pi, sizeof(pi) );

	if (BandcampDLPID != 0) {
		CString pid;
		pid.Format(L"%d",BandcampDLPID);

		CreateProcess(NULL, CString(L"taskkill.exe /F /PID " + pid).GetBuffer(), NULL, NULL, false, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
		WaitForSingleObject(pi.hProcess, INFINITE);
	}

	DeleteFile(L"xdccget.exe");

	return TRUE;
}

extern "C" _declspec(dllexport) bool StormGETPluginConfigure() {
	AfxMessageBox(L"The plugin host_xdccget requires no configuration!");
	return TRUE;
}

extern "C" _declspec(dllexport) bool StormGETPluginStillRunning() {
	DWORD exitCode;

	GetExitCodeProcess(pi.hProcess,&exitCode);

	if (exitCode != STILL_ACTIVE) {
		return false;
	}

	return TRUE;
}

extern "C" _declspec(dllexport) void StormGETPluginDownload(CString FileURL, CString DownloadDir) {
	STARTUPINFO si;
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

	if (GetFileAttributes(DownloadDir.GetBuffer()) != INVALID_FILE_ATTRIBUTES) {
		int Value = CreateProcess(NULL, CString(L"xdccget.exe " + FileURL).GetBuffer(), NULL, NULL, true, 0, NULL, DownloadDir, &si, &pi);
	} else {
		int Value = CreateProcess(NULL, CString(L"xdccget.exe " + FileURL).GetBuffer(), NULL, NULL, true, 0, NULL, NULL, &si, &pi);
	}

	BandcampDLPID = pi.dwProcessId;

	CWinThread* pParseOutput = AfxBeginThread(ParseOutput,THREAD_PRIORITY_NORMAL);
}

extern "C" _declspec(dllexport) char* StormGETPluginGetStatus() {
	if (!CheckOutput) {
		Sleep(500);
	}

	if ((unsigned)strlen(cBuffer) > 3) {
		strcpy(cBufferArchive,cBuffer);
	}

	CheckOutput = false;

	return cBufferArchive;
}

extern "C" _declspec(dllexport) int StormGETPluginGetProgress() {
	/*char cProgressDisect[4096];
	ZeroMemory(cProgressDisect, 4096);

	int cStringLength;
	strcpy(cProgressDisect,cBufferArchive);
	cStringLength = strlen(cProgressDisect);

	if (cProgressDisect[cStringLength - 1] == '.' && cProgressDisect[cStringLength - 2] == '.' && cProgressDisect[cStringLength - 3] == '.' && cProgressDisect[cStringLength - 4] == ')') {
		int LastOpen;
		int LastClose;

		for(int i = 0; cProgressDisect[i]; i++) {
			if (cProgressDisect[i] == '(') LastOpen = i;
			if (cProgressDisect[i] == ')') LastClose = i;
		}

		for(int i = 0; i < LastOpen; i++) {
			memmove (cProgressDisect, cProgressDisect+1, strlen (cProgressDisect+1));
			cProgressDisect[strlen(cProgressDisect) - 1] = 0;
		}

		memmove (cProgressDisect, cProgressDisect+1, strlen (cProgressDisect+1));
		cProgressDisect[strlen(cProgressDisect) - 5] = 0;

		char * cProgressToken;
		int CurrTrack, TotalTracks;

		cProgressToken = strtok(cProgressDisect,"/");
		CurrTrack = atoi(cProgressToken);

		cProgressToken = strtok(NULL,"/");
		TotalTracks = atoi(cProgressToken);

		CurrTrack--;
		TotalTracks--;

		return (CurrTrack * 100) / TotalTracks;
	}*/

	return 0;
}

extern "C" _declspec(dllexport) char * StormGETPluginEnumerateConditions() {
	return "xdcc://*";
}
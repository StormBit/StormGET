// proto_http_aria2.cpp : Defines the initialization routines for the DLL.
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

PROCESS_INFORMATION pi;

HANDLE g_hInputFile = NULL;

char cBuffer[BUFSIZE];
char cBufferArchive[BUFSIZE];

bool CheckOutput = false, Downloading = false;

int Aria2PID = 0, ProgressPercent = 0;

UINT ParseOutput(LPVOID pParam) {
	char cBufferLocal[BUFSIZE];
	char *cLinePos, *cLine;
	CString currLine;

	char * cToken;
	
	CString Downloaded, Total, Percent, Speed, ETA, ETAFormatted, Connections, FileCurrent, FileTotal;
	char *downloaded, *total, *percent, *speed, *eta, *connections;
	int Progress = 0;

	DWORD dwRead; 
	BOOL bSuccess = FALSE;
	HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	for (;;) { 
		while (CheckOutput) {
			Sleep(50);
		}
			
		ZeroMemory(&cBuffer,BUFSIZE);

		bSuccess = ReadFile( g_hChildStd_OUT_Rd, cBufferLocal, BUFSIZE, &dwRead, NULL);
		if( ! bSuccess || dwRead == 0 ) break; 
			   
		for (cLine = strtok_s(cBufferLocal, "\r\n", &cLinePos); cLine; cLine = strtok_s(NULL, "\r\n", &cLinePos)) {
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
			}
		}
		ProgressPercent = Progress;

		strcpy(cBuffer,CStringA(L"[" + Percent + L"%] Downloaded " + Downloaded + L"/" + Total + L" at " + Speed + L" with " + Connections + L" connection(s)."));
		CheckOutput = true;
	}


	return 1;
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
    hRes = FindResource(GetCurrentModuleHandle(),MAKEINTRESOURCE(IDR_ARIA2),CString(L"BIN"));
    hResourceLoaded = LoadResource(GetCurrentModuleHandle(), hRes);
    lpResLock = (char *) LockResource(hResourceLoaded);
    dwSizeRes = SizeofResource(GetCurrentModuleHandle(), hRes);
	FILE * outputRes;
	if(outputRes = _wfopen(L"Plugins\\Binaries\\aria2c.exe", L"wb")) {
		fwrite ((const char *) lpResLock,1,dwSizeRes,outputRes);
		fclose(outputRes);
	}

	return TRUE;
}

extern "C" _declspec(dllexport) bool StormGETPluginStop() {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	ZeroMemory( &pi, sizeof(pi) );

	if (Aria2PID != 0) {
		CString pid;
		pid.Format(L"%d",Aria2PID);

		CreateProcess(NULL, CString(L"taskkill.exe /F /PID " + pid).GetBuffer(), NULL, NULL, false, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
		WaitForSingleObject(pi.hProcess, INFINITE);
	}

	return TRUE;
}

extern "C" _declspec(dllexport) bool StormGETPluginExit() {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	ZeroMemory( &pi, sizeof(pi) );

	if (Aria2PID != 0) {
		CString pid;
		pid.Format(L"%d",Aria2PID);

		CreateProcess(NULL, CString(L"taskkill.exe /F /PID " + pid).GetBuffer(), NULL, NULL, false, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
		WaitForSingleObject(pi.hProcess, INFINITE);
	}

	DeleteFile(L"Plugins\\Binaries\\aria2c.exe");

	return TRUE;
}

extern "C" _declspec(dllexport) bool StormGETPluginConfigure() {
	AfxMessageBox(L"This plugin requires no configuration!");
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
			
	if (DownloadDir == L"") {
		DownloadDir = L".";
	}

	int Value = CreateProcess(NULL, CString(L"Plugins\\Binaries\\aria2c.exe --file-allocation=none --check-certificate=false --dir \"" + DownloadDir + L"\" --max-connection-per-server 16" /* + m_FileQueue->GetItemText(i, 1) */ + L" --min-split-size 1M --split 16" /* + m_FileQueue->GetItemText(i, 1)*/ + L" " + FileURL).GetBuffer(), NULL, NULL, true, 0, NULL, NULL, &si, &pi);
				
	Aria2PID = pi.dwProcessId;

	CWinThread* pParseOutput = AfxBeginThread(ParseOutput,THREAD_PRIORITY_NORMAL);
}

extern "C" _declspec(dllexport) char* StormGETPluginGetStatus() {
	if (!CheckOutput) {
		Sleep(500);
	}

	if ((unsigned)strlen(cBuffer) > 16) {
		strcpy(cBufferArchive,cBuffer);
	}

	CheckOutput = false;

	return cBufferArchive;
}

extern "C" _declspec(dllexport) char* StormGETPluginGetStatusLine2() {
	return NULL;
}

extern "C" _declspec(dllexport) char* StormGETPluginGetName() {
	return "StormGET HTTP Plugin (aria2c)";
}

extern "C" _declspec(dllexport) int StormGETPluginGetProgress() {
	return ProgressPercent;
}

extern "C" _declspec(dllexport) char * StormGETPluginEnumerateConditions() {
	return "http://*";
}
#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Headers. 

#include <winsock2.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <windows.h>
#include <iostream>
#include <cstring>
#include <fstream>
#include <ostream>
#include <sstream>
#include <process.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <stdint.h>

using namespace std;

#pragma comment (lib, "ws2_32.lib") 
#pragma comment (lib, "libeay32MT.lib")
#pragma comment	(lib, "ssleay32MT.lib")

#define _CRT_SECURE_NO_WARNINGS

using namespace std;

typedef struct {
	char *szNick;
	char *szUser;
	char *szHost;
	char *szCmd;
	char *szArg[15];
} IRCMSG, *PIRCMSG;

SOCKET strSocket;
SOCKET dccSocket;
SSL *ssl;
SSL_CTX *ctx;

bool ircChannelJoined = false;
bool DCCResume = true;
long long int DCCIP = 0, DCCPort = 0, DCCSize = 0;
char* DCCFilename;

char *DCCNick;

int		ircTimeout;
char*	ircNick;
char*	ircServer;
bool	useSSL;
int		ircPort;
char*	ircChan;
char*	dccNick;
char*	dccNum;
int		percentDone = 0;
bool	stillRunning = true;
bool	dccFuncRun = false;

char	getStatus[1024] = "Initializing...";
char	getStatus2[1024] = "Initializing StormGET XDCC Downloader...";

PIRCMSG SplitIrcMessage(char *szMsg);
bool arrayShift(char *cBuffer);
int socketConnect(SOCKET *connection, char *HName, int port);
int sockPrint(SOCKET *strSocket, char* cMessage, ...);
void parseMessage(SOCKET *strSocket, char *cBuffer);
void SleepJoin(void *derp);
void MainThread(void *derp);

bool speedCalculated = false;
UINT64 recvTotal = 0;
UINT64 recvSpeed = 0;

char *int2ip(int ip) {
  char * result = (char*) malloc (17);

  sprintf(result, "%d.%d.%d.%d",
    (ip >> 24) & 0xFF,
    (ip >> 16) & 0xFF, 
    (ip >>  8) & 0xFF,
    (ip      ) & 0xFF);

  return result;
}

uint64_t fsize(char* fName) {
	FILE *pFile = NULL;
	pFile=fopen (fName,"rb");
	if (pFile != NULL) {
		_fseeki64( pFile, 0, SEEK_END );
		uint64_t size = _ftelli64( pFile );
		fclose( pFile );
		return size;
	} else return 0;
}

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

char *URL;

HMODULE GetCurrentModuleHandle() {
    HMODULE hMod = NULL;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&GetCurrentModuleHandle),&hMod);
     return hMod;
}

extern "C" _declspec(dllexport) bool StormGETPluginInit() {
	return TRUE;
}

extern "C" _declspec(dllexport) bool StormGETPluginStop() {
	closesocket(dccSocket);
	sockPrint(&strSocket, "QUIT :\r\n");
	closesocket(strSocket);
	return TRUE;
}

extern "C" _declspec(dllexport) bool StormGETPluginExit() {
	StormGETPluginStop();
	return TRUE;
}

extern "C" _declspec(dllexport) bool StormGETPluginConfigure() {
	AfxMessageBox(L"The plugin host_xdccget requires no configuration!");
	return TRUE;
}

extern "C" _declspec(dllexport) bool StormGETPluginStillRunning() {
	return stillRunning;
}

extern "C" _declspec(dllexport) char* StormGETPluginName() {
	return "StormGET XDCC Downloader";
}

extern "C" _declspec(dllexport) void StormGETPluginDownload(CString FileURL, CString DownloadDir) {
	URL = strdup(CStringA(FileURL).GetBuffer());
	_beginthread(MainThread, 0, NULL);
}

extern "C" _declspec(dllexport) char* StormGETPluginGetStatus() {
	return getStatus;
}

extern "C" _declspec(dllexport) char* StormGETPluginGetStatusLine2() {
	return getStatus2;
}

extern "C" _declspec(dllexport) int StormGETPluginGetProgress() {
	return percentDone;
}

extern "C" _declspec(dllexport) char * StormGETPluginEnumerateConditions() {
	return "xdcc://*";
}

#define CHUNKSIZE 1024

void MainThread(void *derp) {
	char portStr[128];

	for (int i=0;i<7;i++) arrayShift(URL);

	char * pch;
	pch = strtok (URL,":");
	if (pch != NULL) ircServer = strdup(pch);
	pch = strtok (NULL,"/");
	if (pch != NULL) {
		if (pch[0] == '+') {
			useSSL = true;
			strcpy(portStr,pch);
			arrayShift(portStr);
			ircPort = atoi(portStr);
		} else {
			useSSL = false;
			strcpy(portStr,pch);
			ircPort = atoi(pch);
		}
	}	
	pch = strtok (NULL,"/");
	if (pch != NULL) ircNick = strdup(pch);
	pch = strtok (NULL,"/");
	if (pch != NULL) ircChan = strdup(pch);
	pch = strtok (NULL,"/");
	if (pch != NULL) dccNick = strdup(pch);
	pch = strtok (NULL,"/");
	if (pch != NULL) dccNum = strdup(pch);

	ircTimeout	= 10;

    char cBuffer[128000];
    char cOutBuffer[512];
    char *lineSPLIT[512];
    char *args[10];
    int bRecv;
    PIRCMSG sMessage;

	ZeroMemory(getStatus,1024);
	strcpy(getStatus,CStringA("Connecting to server " + CStringA(ircServer) + ":" + portStr + "...").GetBuffer());

    int tries;
    for(tries=1; socketConnect(&strSocket, ircServer, ircPort)!=0; tries++) {
		ZeroMemory(getStatus,1024);
        sprintf(getStatus,"Connection attempt %d failed...",tries);
        Sleep(ircTimeout*1000);
    }

	ZeroMemory(getStatus,1024);
	strcpy(getStatus,"Logging in...");

	sockPrint(&strSocket, "NICK %s\n\n", ircNick);
    sockPrint(&strSocket, "USER %s * * :%s\n\n", ircNick, ircNick);

	ZeroMemory(getStatus,1024);
    strcpy(getStatus,"Waiting for server...");

    while(1) {
        memset(cBuffer, 0, 128000);
        memset(cOutBuffer, 0, 512);

		if (useSSL) bRecv = SSL_read(ssl, cBuffer, 128000);
        else bRecv = recv(strSocket, cBuffer, 128000, 0);
        if( (bRecv == 0) || (bRecv == SOCKET_ERROR) ) break;

		char * pch;
		pch = strtok (cBuffer,"\n\n");
		while (pch != NULL) {
			//cout << pch << "\n";

			if(pch[0] == 'P' && pch[1] == 'I' && pch[2] == 'N' && pch[3] == 'G') {
				strcpy(cOutBuffer, pch);
				cOutBuffer[0] = 'P';
				cOutBuffer[1] = 'O';
				cOutBuffer[2] = 'N';
				cOutBuffer[3] = 'G';
				sockPrint(&strSocket, cOutBuffer);
				//cout << "\n";
			} else {
				parseMessage(&strSocket, pch);
			}  

			pch = strtok (NULL, "\n\n");
		}

    }
	if (useSSL) SSL_shutdown(ssl);
    closesocket(strSocket);
	if (useSSL) SSL_free(ssl);
    //return (1);
}

void SpeedCalc (void *derp) {
	UINT64 snap1 = 0, snap2 = 0, snap3 = 0, snap4 = 0, snap5 = 0, loopCount = 0;

	while (stillRunning) {
		loopCount++;

		snap5 = snap4;
		snap4 = snap3;
		snap3 = snap2;
		snap2 = snap1;

		snap1 = recvTotal;

		recvSpeed = ((snap1 - snap2) + (snap2 - snap3) + (snap3 - snap4) + (snap4 - snap5)) / 5; 

		if (loopCount >= 5) {
			speedCalculated = true;
		}

		Sleep(1000);
	}
}

void DCCGet(void *derp) {
	if (dccFuncRun) return;

	dccFuncRun = true;

	SOCKADDR_IN dccServerInfo;
    WSADATA wsaData;
    LPHOSTENT hostEntry;
	char cFileCache[CHUNKSIZE];
	int bRecv;
	FILE * pFile;

	long long int totalRecieved = 0;

    WSAStartup(MAKEWORD(1, 1), &wsaData);

	if ((dccSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
		ZeroMemory(getStatus,1024);
		strcpy(getStatus,CStringA(L"Error " + GetLastError()).GetBuffer());
	} else {
		dccServerInfo.sin_family = AF_INET;
		dccServerInfo.sin_addr.s_addr = ntohl(DCCIP);
		dccServerInfo.sin_port = htons(DCCPort);

		char strDCCPort[8];
		ZeroMemory(strDCCPort,8);
		itoa(DCCPort,strDCCPort,10);

		ZeroMemory(getStatus,1024);
		strcpy(getStatus,CStringA("Connecting to " + CStringA(inet_ntoa(dccServerInfo.sin_addr)) + " on port " + CStringA(strDCCPort) + "...").GetBuffer());

		if (connect(dccSocket, (LPSOCKADDR)&dccServerInfo, sizeof(struct sockaddr)) == SOCKET_ERROR) {
			ZeroMemory(getStatus,1024);
			strcpy(getStatus,CStringA(L"Error " + GetLastError()).GetBuffer());
		}

		ZeroMemory(getStatus,1024);
		strcpy(getStatus,CStringA("Transferring data...").GetBuffer());

		ZeroMemory(getStatus2,1024);
		strcpy(getStatus2,CStringA("Recieving file " + CStringA(DCCFilename) + "...").GetBuffer());
		
		if (DCCFilename[0] == '"') {
			arrayShift(DCCFilename);
			DCCFilename[sizeof(DCCFilename)] = '\0';
		}

		if (DCCResume == true) {
			totalRecieved = fsize(DCCFilename);
			pFile = fopen (DCCFilename,"ab");
		} else {
			pFile = fopen (DCCFilename,"wb");
		}

		if (pFile!=NULL) {
			_beginthread(SpeedCalc, 0, NULL);
			while(1) {
				memset(cFileCache, 0, CHUNKSIZE);

				bRecv = recv(dccSocket, cFileCache, sizeof(cFileCache), 0);
				if( (bRecv == 0) || (bRecv == SOCKET_ERROR) ) break;
				fwrite (cFileCache, 1, bRecv, pFile);

				totalRecieved += bRecv;
				recvTotal += bRecv;

				percentDone = (totalRecieved / (DCCSize / 100));

				char buf1[128];
				if ((totalRecieved / 1024) > 1048576) {
					_itoa(totalRecieved / 1073741824, buf1, 10);
					strcat(buf1," GB");
				} else if ((totalRecieved / 1024) > 1024) {
					_itoa(totalRecieved / 1048576, buf1, 10);
					strcat(buf1," MB");
				} else if (totalRecieved > 1024) {
					_itoa(totalRecieved / 1024, buf1, 10);
					strcat(buf1," kB");
				} else {				
					_itoa(totalRecieved, buf1, 10);
					strcat(buf1," bytes");
				}

				char buf2[128];
				if ((DCCSize / 1024) > 1048576) {
					_itoa(DCCSize / 1073741824, buf2, 10);
					strcat(buf2," GB");
				} else if ((DCCSize / 1024) > 1024) {
					_itoa(DCCSize / 1048576, buf2, 10);
					strcat(buf2," MB");
				} else if (DCCSize > 1024) {
					_itoa(DCCSize / 1024, buf2, 10);
					strcat(buf2," kB");
				} else {				
					_itoa(DCCSize, buf2, 10);
					strcat(buf2," bytes ");
				}

				ZeroMemory(getStatus,1024);
				if (!speedCalculated) strcpy(getStatus,CStringA("Downloaded " + CStringA(buf1) + " of " + CStringA(buf2) + " (Calculating...)").GetBuffer());
				else {
					char buf3[128];
					
					if ((recvSpeed / 1024) > 1048576) {
						_itoa(recvSpeed / 1073741824, buf3, 10);
						strcat(buf3," GB/sec");
					} else if ((recvSpeed / 1024) > 1024) {
						_itoa(recvSpeed / 1048576, buf3, 10);
						strcat(buf3," MB/sec");
					} else if (recvSpeed > 1024) {
						_itoa(recvSpeed / 1024, buf3, 10);
						strcat(buf3," kB/sec");
					} else {				
						_itoa(recvSpeed, buf3, 10);
						strcat(buf3," bytes/sec");
					}

					UINT64 dataLeft = DCCSize - totalRecieved;
					if (recvSpeed == 0) recvSpeed = 1; 
					UINT64 secondsLeft = dataLeft / recvSpeed;

					int secs =  secondsLeft			% 60;
					int mins = (secondsLeft / 60)	% 60;
					int hour = (secondsLeft / 3600)	% 24;
					int days =  secondsLeft / 86400;

					char strSecs[64], strMins[64], strHour[64], strDays[64];

					_itoa(secs, strSecs, 10);
					_itoa(mins, strMins, 10);
					_itoa(hour, strHour, 10);
					_itoa(days, strDays, 10);

					char timeStr[256] = "";

					if (days > 0) if (days != 1) strcat(timeStr,CStringA(CStringA(strDays) + " days, ").GetBuffer());
					if (days > 0) if (days == 1) strcat(timeStr,CStringA(CStringA(strDays) + " day, ").GetBuffer());

					if (days > 0 || hour > 0) if (hour != 1) strcat(timeStr,CStringA(CStringA(strHour) + " hours, ").GetBuffer());
					if (days > 0 || hour > 0) if (hour == 1) strcat(timeStr,CStringA(CStringA(strHour) + " hour, ").GetBuffer());

					if (days > 0 || mins > 0 || hour > 0) if (mins != 1) strcat(timeStr,CStringA(CStringA(strMins) + " minutes, ").GetBuffer());
					if (days > 0 || mins > 0 || hour > 0) if (mins == 1) strcat(timeStr,CStringA(CStringA(strMins) + " minute, ").GetBuffer());
					
					if (secs != 1) strcat(timeStr,CStringA(CStringA(strSecs) + " seconds remaining").GetBuffer());
					if (secs == 1) strcat(timeStr,CStringA(CStringA(strSecs) + " second remaining").GetBuffer());

					strcpy(getStatus,CStringA("Downloaded " + CStringA(buf1) + " of " + CStringA(buf2) + " at " + CStringA(buf3) + " (" + timeStr + ")").GetBuffer());
				}

				if (DCCSize == totalRecieved) {
					char buffer[65];
					_itoa(DCCSize, buffer, 10);
					send(dccSocket, buffer, strlen(buffer), 0);

					stillRunning = false;
				}
			}
			
			fclose(pFile);
		} else {
			strcpy(getStatus,"Error opening file");
			closesocket(dccSocket);
		}
	}

	char buf1[65];
	_itoa(totalRecieved / 1024, buf1, 10);

	char buf2[65];
	_itoa(DCCSize / 1024, buf2, 10);
	
	ZeroMemory(getStatus,1024);
	sockPrint(&strSocket, "QUIT :\r\n");
	stillRunning = false;
}

bool arrayShift(char *cBuffer) {
    for(int i=0; i<strlen(cBuffer); i++) cBuffer[i]=cBuffer[i+1];
    cBuffer[strlen(cBuffer)] = '\0';
    return (1);
}

int socketConnect(SOCKET *connection, char *HName, int port) {
    SOCKADDR_IN ircServerInfo;
    WSADATA wsaData;
    LPHOSTENT hostEntry;
    if (WSAStartup(MAKEWORD(1, 1), &wsaData) == -1) return (-1);
	if (useSSL) {
		ZeroMemory(getStatus,1024);
		strcpy(getStatus,"Initializing OpenSSL...");
	}
	if (useSSL) SSL_load_error_strings();
	if (useSSL) SSL_library_init();
    if (!(hostEntry = gethostbyname(HName))) {
        WSACleanup();
        return (-1);
    }
    if ((*connection = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
        WSACleanup();
        return (-1);
    }
    ircServerInfo.sin_family = AF_INET;
    ircServerInfo.sin_addr = *((LPIN_ADDR)*hostEntry->h_addr_list);
    ircServerInfo.sin_port = htons(port);
    if (connect(*connection, (LPSOCKADDR)&ircServerInfo, sizeof(struct sockaddr)) == SOCKET_ERROR) {
        WSACleanup();
        return (-1);
    }

	if (useSSL) {
		SSL_CTX* ctx = SSL_CTX_new(SSLv23_client_method());
		if(!ctx) {
			ZeroMemory(getStatus,1024);
			strcpy(getStatus,"OpenSSL: Error initializing SSL_CTX object");
			return (-1);
		}

		ssl = SSL_new(ctx);
		SSL_CTX_free(ctx);

		if(!ssl) {
			ZeroMemory(getStatus,1024);
			strcpy(getStatus,"OpenSSL: Error initializing SSL object");
			return (-1);
		}

		SSL_set_fd(ssl, (int)*connection);
		SSL_set_verify(ssl, SSL_VERIFY_NONE, NULL);
		if (useSSL) {
			ZeroMemory(getStatus,1024);
			strcpy(getStatus,"Performing SSL authentication...");
		}
		if(SSL_connect(ssl) != 1) {
			ZeroMemory(getStatus,1024);
			strcpy(getStatus,"OpenSSL: Error during SSL authentication");
			return (-1);
		}

	}
    return (0);
}

int sockPrint(SOCKET *strSocket, char* cMessage, ...) {
    char cBuffer[512];
    int iError;
    va_list va;
    va_start(va, cMessage);
    vsprintf(cBuffer, cMessage, va);
    va_end(va);
	//cout << ">> " << cBuffer;

	if (useSSL) {
		SSL_write(ssl, cBuffer, strlen(cBuffer));
		SSL_write(ssl, "\n\n", strlen("\n\n"));
	} else {
		send(*strSocket, cBuffer, strlen(cBuffer), 0);
		send(*strSocket, "\n\n", strlen("\n\n"), 0);
	}
    return 1;
}


void parseMessage(SOCKET *strSocket, char *sMessage) {
	/* Struct: 	
		char *szNick;
		char *szUser;
		char *szHost;
		char *szCmd;
		char *szArg[15];
	*/
	//cout << ">> " << sMessage << "\n\n";
	PIRCMSG cMessage = SplitIrcMessage(sMessage);

	if (!strcmp(cMessage->szCmd,"422") || !strcmp(cMessage->szCmd,"376")) {
		if (!ircChannelJoined) {
			ZeroMemory(getStatus,1024);
			sprintf(getStatus,"Joining %s...", ircChan);
			sockPrint(strSocket, "JOIN %s\n\n", ircChan);

			ZeroMemory(getStatus,1024);
			sprintf(getStatus,"Requesting pack %s from %s...", dccNum, dccNick);
			sockPrint(strSocket, "PRIVMSG %s :XDCC SEND %s\n\n", dccNick, dccNum);

			ircChannelJoined = true;
		}
	}
	if (cMessage->szArg[1] != NULL) {
		if (!strcmp(cMessage->szArg[1],"\1VERSION\1")) {
			sockPrint(strSocket, "NOTICE %s :\1VERSION XChat 2.8.9 (compatible; StormGET v1.1)\1\n\n", cMessage->szNick);
		}
	} 	

	if (cMessage->szArg[1] != NULL) {	
		char* messageParams[5];
		for(int x = 0; x < 6; x++) {
			messageParams[x] = NULL;
		}
	
		char cMsgArray[512];
		ZeroMemory(&cMsgArray,512);
		strcpy(cMsgArray,cMessage->szArg[1]);
		strcat(cMsgArray," ");

		char* szPos = strdup(cMsgArray);

		for(int x = 0; x < 6; x++) {
			if(*szPos == '"') {
				char cQuotedMessage[512];
				ZeroMemory(cQuotedMessage,512);
				
				char *szSp = strstr(szPos, " ");
				if(szSp != NULL) {
					*szSp= 0;
					strcat(cQuotedMessage,szPos);
					szPos += (strlen(szPos) + 1);
				} else {
					messageParams[x] = cMessage->szArg[1];
					break;
				}
				
				break;	
			}
			char *szSp = strstr(szPos, " ");
		
			if(szSp != NULL) {
				*szSp= 0;
				messageParams[x] = szPos;
				szPos += (strlen(szPos) + 1);
			} else {
				messageParams[x] = cMessage->szArg[1];
				break;
			}
		}
		if (!strcmp(messageParams[0],"\1DCC") && !strcmp(messageParams[1],"SEND")) {
			if (messageParams[5] != NULL) {
				DCCSize = _atoi64(messageParams[5]);
			} 
			if (messageParams[2] != NULL && messageParams[3] != NULL && messageParams[4] != NULL) {
				ZeroMemory(getStatus,1024);
				strcpy(getStatus,CStringA("Recieved offer for file " + CStringA(messageParams[2])).GetBuffer());
				
				DCCFilename = strdup(messageParams[2]);
				DCCNick = strdup(cMessage->szNick);
				DCCIP = _atoi64(messageParams[3]);
				DCCPort = _atoi64(messageParams[4]);

				if (fsize(DCCFilename) == DCCSize) {
					ZeroMemory(getStatus,1024);
					strcpy(getStatus,"File already fully downloaded, nothing to do.");
					sockPrint(strSocket, "PRIVMSG %s :XDCC CANCEL\n\n", dccNick);
					sockPrint(strSocket, "QUIT :\r\n");
					stillRunning = false;
				} else if (fsize(DCCFilename) == 0) {
					_beginthread(DCCGet, 0, NULL);
				} else {
					ZeroMemory(getStatus,1024);
					strcpy(getStatus,"File exists, resuming...");
					ostringstream outStr;
					outStr << "PRIVMSG " << cMessage->szNick << " :\1DCC RESUME " << DCCFilename << " " << DCCPort << " " <<  fsize(DCCFilename) << "\1\n\n";
					sockPrint(strSocket,(char *)outStr.str().c_str());

					// Any XDCC bot out there should support resumes, so at this point we wait for an ACCEPT and then connect.
					DCCResume = true;
				}
			}
		} else if (!strcmp(messageParams[0],"\1DCC") && !strcmp(messageParams[1],"ACCEPT")) {
			_beginthread(DCCGet, 0, NULL);
		}
	}
}

PIRCMSG SplitIrcMessage(char *szMsg) {
	PIRCMSG pMsg = new IRCMSG;
	char *szPos = szMsg;

	if(szMsg[0] == ':') {
		char *szNickS = strstr(szPos, "!");

		if(szNickS != NULL) {
			*szNickS = 0;
			pMsg->szNick = (szPos + 1);
			szPos += (strlen(szPos) + 1);
		} else {
			pMsg->szNick = "NONICK";
		}

		char *szUserS = strstr(szPos, "@");

		if(szUserS != NULL) {
			*szUserS = 0;
			pMsg->szUser = szPos;
			szPos += (strlen(szPos) + 1);	
		} else { 
			pMsg->szUser = "NOUSER";
		}

		char *szSpaceS = strstr(szPos, " ");
		
		if(szSpaceS != NULL) {
			*szSpaceS = 0;
			pMsg->szHost = szPos;
			szPos += (strlen(szPos) + 1);
		} else {
			pMsg->szHost = "no-hostmask.bot";
		}
	} else {
		pMsg->szNick = "NONICK";
		pMsg->szUser = "NOUSER";
		pMsg->szHost = "no-hostmask.bot";
	}

	char *szSpaceTwo = strstr(szPos, " ");
	
	if(szSpaceTwo != NULL) {
		*szSpaceTwo = 0;
		pMsg->szCmd = szPos;
		szPos += (strlen(szPos) + 1);
	} else {
		pMsg->szCmd = "UNKNOWN";
	}


	for(int x = 0; x < 15; x++) {
		pMsg->szArg[x] = NULL;
	}
	
	for(int x = 0; x < 15; x++) {
		if(*szPos == ':') {
			pMsg->szArg[x] = (szPos + 1);
			break;
		}

		char *szSp = strstr(szPos, " ");
		
		if(szSp != NULL) {
			*szSp= 0;
			pMsg->szArg[x] = szPos;
			szPos += (strlen(szPos) + 1);
		} else {
			pMsg->szArg[x] = szPos;
			break;
		}
	}

	return pMsg;
}
#include "ReimuDCC.h"

#define CHUNKSIZE 1024

void DCCGet(void *derp) {
	SOCKET dccSocket;
	SOCKADDR_IN dccServerInfo;
    WSADATA wsaData;
    LPHOSTENT hostEntry;
	char cFileCache[CHUNKSIZE];
	int bRecv;
	FILE * pFile;

	long int totalRecieved = 0;

    WSAStartup(MAKEWORD(1, 1), &wsaData);

	if ((dccSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
		cout << "Error " << GetLastError();
	} else {
		dccServerInfo.sin_family = AF_INET;
		dccServerInfo.sin_addr.s_addr = ntohl(DCCIP);
		dccServerInfo.sin_port = htons(DCCPort);

		cout << "Connecting to " << inet_ntoa(dccServerInfo.sin_addr) << " on port " << DCCPort << ": ";

		if (connect(dccSocket, (LPSOCKADDR)&dccServerInfo, sizeof(struct sockaddr)) == SOCKET_ERROR) {
			cout << "Error " << GetLastError();
		}

		cout << "\nRecieving file " << DCCFilename << "...";

		if (DCCFilename[0] == '"') {
			arrayShift(DCCFilename);
			DCCFilename[sizeof(DCCFilename)] = '\0';
		}
		pFile = fopen (DCCFilename,"w");
		if (pFile!=NULL) {
			while(1) {
				memset(cFileCache, 0, CHUNKSIZE);

				bRecv = recv(dccSocket, cFileCache, CHUNKSIZE, 0);
				if( (bRecv == 0) || (bRecv == SOCKET_ERROR) ) break;
				fwrite (cFileCache, 1, bRecv, pFile);

				totalRecieved += bRecv;
				char buffer[65];

				_itoa(totalRecieved, buffer, 10);
				send(dccSocket, buffer, strlen(buffer), 0);
				cout << (totalRecieved / 1024) << " kB / " << (DCCSize / 1024) << " kB\n";
			}
			
			fclose(pFile);
		} else {
			cout << "\nError opening file";
			closesocket(dccSocket);
		}
	}

	cout << "\nDone\n";
}

int main(int argc, char *argv[]) {
    char cBuffer[128000];
    char cOutBuffer[512];
    char *lineSPLIT[512];
    char *args[10];
    int bRecv;
    PIRCMSG sMessage;

	cout << "ReimuGET DCC Client 1.0a1\n\n * Connecting to server " << ircServer << ":" << ircPort << "... ";

	for(int i = 1; i < (80 - (37 + sizeof(ircServer) + sizeof(ircPort))); i++) {
		cout << " ";
	}

    int tries;
    for(tries=1; socketConnect(&strSocket, ircServer, ircPort)!=0; tries++) {
        printf("Failed attempt: %d\r\n",tries);
        Sleep(ircTimeout*1000);
    }

	printf("[ DONE ]\n * Logging in...                                                      ");
	sockPrint(&strSocket, "NICK %s\r\n", ircNick);
    sockPrint(&strSocket, "USER %s * * :%s\r\n", ircNick, ircNick);
    printf("[ DONE ]\n * Waiting for server...                                              ");

    while(1) {
        memset(cBuffer, 0, 128000);
        memset(cOutBuffer, 0, 512);

        bRecv = recv(strSocket, cBuffer, 128000, 0);
        if( (bRecv == 0) || (bRecv == SOCKET_ERROR) ) break;

		char * pch;
		pch = strtok (cBuffer,"\r\n");
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

			pch = strtok (NULL, "\r\n");
		}

    }
    closesocket(strSocket);
	Sleep(1000);
    return (1);
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
    send(*strSocket, cBuffer, strlen(cBuffer), 0);
	send(*strSocket, "\r\n", strlen("\r\n"), 0);
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

	PIRCMSG cMessage = SplitIrcMessage(sMessage);

	if (!strcmp(cMessage->szCmd,"422") || !strcmp(cMessage->szCmd,"376")) {
		if (!ircChannelJoined) {
			printf("[ DONE ]\n * Joining %s... ", ircChan);
			for(int i = 1; i < (80 - (23 + sizeof(ircChan))); i++) {
				cout << " ";
			}
			sockPrint(strSocket, "JOIN %s\r\n", ircChan);
			sockPrint(strSocket, "PRIVMSG Asuha :XDCC SEND 15\r\nPRIVMSG Asuha :XDCC SEND 16\r\nPRIVMSG Asuha :XDCC SEND 17\r\nPRIVMSG Asuha :XDCC SEND 18\r\nPRIVMSG Asuha :XDCC SEND 19\r\nPRIVMSG Asuha :XDCC SEND 20\r\nPRIVMSG Asuha :XDCC SEND 21\r\nPRIVMSG Asuha :XDCC SEND 22\r\nPRIVMSG Asuha :XDCC SEND 23\r\nPRIVMSG Asuha :XDCC SEND 24\r\nPRIVMSG Asuha :XDCC SEND 25\r\nPRIVMSG Asuha :XDCC SEND 26\r\nPRIVMSG Asuha :XDCC SEND 27\r\nPRIVMSG Asuha :XDCC SEND 28\r\n");

			printf("[ DONE ]\n");
			ircChannelJoined = true;
		}
	}
	if (cMessage->szArg[1] != NULL) {
		if (!strcmp(cMessage->szArg[1],"\1VERSION\1")) {
			sockPrint(strSocket, "NOTICE %s :irssi v0.8.15 - running on Windows x86_64\r\n", cMessage->szNick);
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
				ZeroMemory(cQuotedMessage,512,0);
				
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
				cout << "Recieved offer for file " << messageParams[2] << "\n";
				
				DCCFilename = strdup(messageParams[2]);

				DCCIP = _atoi64(messageParams[3]);
				DCCPort = _atoi64(messageParams[4]);

				_beginthread(DCCGet, 0, NULL);
			}
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
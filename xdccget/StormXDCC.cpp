#include "StormXDCC.h"

#define CHUNKSIZE 16384

void DCCGet(void *derp) {
	SOCKET dccSocket;
	SOCKADDR_IN dccServerInfo;
    WSADATA wsaData;
    LPHOSTENT hostEntry;
	char cFileCache[CHUNKSIZE];
	int bRecv;
	FILE * pFile;

	long long int totalRecieved = 0;

    WSAStartup(MAKEWORD(1, 1), &wsaData);

	if ((dccSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
		cout << "Error " << GetLastError();
	} else {
		dccServerInfo.sin_family = AF_INET;
		dccServerInfo.sin_addr.s_addr = ntohl(DCCIP);
		dccServerInfo.sin_port = htons(DCCPort);

		cout << "Connecting to " << inet_ntoa(dccServerInfo.sin_addr) << " on port " << DCCPort << "...\n";

		if (connect(dccSocket, (LPSOCKADDR)&dccServerInfo, sizeof(struct sockaddr)) == SOCKET_ERROR) {
			cout << "Error " << GetLastError();
		}

		cout << "Recieving file " << DCCFilename << "...\n";
		
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
			while(1) {
				memset(cFileCache, 0, CHUNKSIZE);

				bRecv = recv(dccSocket, cFileCache, sizeof(cFileCache), 0);
				if( (bRecv == 0) || (bRecv == SOCKET_ERROR) ) break;
				fwrite (cFileCache, 1, bRecv, pFile);

				totalRecieved += bRecv;

				if (DCCSize == totalRecieved) {
					char buffer[65];
					_itoa(DCCSize, buffer, 10);
					send(dccSocket, buffer, strlen(buffer), 0);
				}
				cout << (totalRecieved / 1024) << " kB / " << (DCCSize / 1024) << " kB\n";
			}
			
			fclose(pFile);
		} else {
			cout << "Error opening file\n";
			closesocket(dccSocket);
		}
	}

	cout << (totalRecieved / 1024) << " kB / " << (DCCSize / 1024) << " kB - Transfer complete! Exiting...\n";
	exit(0);
}

int main(int argc, char *argv[]) {
	if (argc < 1) {
		printf("No URI provided.");
		exit(1);
	}

	char *URL = strdup(argv[1]);

	for (int i=0;i<7;i++) arrayShift(URL);

	char * pch;
	pch = strtok (URL,":");
	if (pch != NULL) ircServer = strdup(pch);
	pch = strtok (NULL,"/");
	if (pch != NULL) {
		if (pch[0] == '+') {
			useSSL = true;
			char *portStr = strdup(pch);
			arrayShift(portStr);
			ircPort = atoi(portStr);
		} else {
			useSSL = false;
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

	cout << "StormGET XDCC Client 1.0a1\n\nConnecting to server " << ircServer << ":" << ircPort << "...\n";

    int tries;
    for(tries=1; socketConnect(&strSocket, ircServer, ircPort)!=0; tries++) {
        printf("Failed attempt: %d\n\n",tries);
        Sleep(ircTimeout*1000);
    }

	printf("Logging in...\n");
	sockPrint(&strSocket, "NICK %s\n\n", ircNick);
    sockPrint(&strSocket, "USER %s * * :%s\n\n", ircNick, ircNick);
    printf("Waiting for server...\n");

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
	if (useSSL) printf("Initializing OpenSSL...\n");
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
			printf("OpenSSL: Error initializing SSL_CTX object\n");
			return (-1);
		}

		ssl = SSL_new(ctx);
		SSL_CTX_free(ctx);

		if(!ssl) {
			printf("OpenSSL: Error initializing SSL object\n");
			return (-1);
		}

		SSL_set_fd(ssl, (int)*connection);
		SSL_set_verify(ssl, SSL_VERIFY_NONE, NULL);
		if (useSSL) printf("Performing SSL authentication...\n");
		if(SSL_connect(ssl) != 1) {
			printf("OpenSSL: Error during SSL authentication\n");
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
			printf("Joining %s...\n", ircChan);
			sockPrint(strSocket, "JOIN %s\n\n", ircChan);

			printf("Requesting pack %s from %s...\n", dccNum, dccNick);
			sockPrint(strSocket, "PRIVMSG %s :XDCC SEND %s\n\n", dccNick, dccNum);

			ircChannelJoined = true;
		}
	}
	if (cMessage->szArg[1] != NULL) {
		if (!strcmp(cMessage->szArg[1],"\1VERSION\1")) {
			sockPrint(strSocket, "NOTICE %s :irssi v0.8.15 - running on Windows x86_64\n\n", cMessage->szNick);
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
				cout << "Recieved offer for file " << messageParams[2] << "\n";
				
				DCCFilename = strdup(messageParams[2]);
				DCCNick = strdup(cMessage->szNick);
				DCCIP = _atoi64(messageParams[3]);
				DCCPort = _atoi64(messageParams[4]);
			
				if (fsize(DCCFilename) == DCCSize) {
					printf("File already fully downloaded, nothing to do.");
					sockPrint(strSocket, "PRIVMSG %s :XDCC CANCEL\n\n", dccNick);
					sockPrint(strSocket, "QUIT :\n\n", dccNick);
				} else if (fsize(DCCFilename) == 0) {
					_beginthread(DCCGet, 0, NULL);
				} else {
					printf("File exists, resuming...\n");
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
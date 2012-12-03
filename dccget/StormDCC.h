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

#include "Config.h"

#pragma comment (lib, "ws2_32.lib") 
#pragma comment (lib, "libeay32.lib")
#pragma comment	(lib, "ssleay32.lib")

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
SSL *ssl;
SSL_CTX *ctx;

bool ircChannelJoined = false;
bool DCCResume = true;
long long int DCCIP = 0, DCCPort = 0, DCCSize = 0;
char* DCCFilename;

char *DCCNick;

PIRCMSG SplitIrcMessage(char *szMsg);
bool arrayShift(char *cBuffer);
int socketConnect(SOCKET *connection, char *HName, int port);
int sockPrint(SOCKET *strSocket, char* cMessage, ...);
void parseMessage(SOCKET *strSocket, char *cBuffer);
void SleepJoin(void *derp);

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
// Headers. 

#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <process.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>

#include "Config.h"

#pragma comment (lib, "ws2_32.lib") 

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
bool ircChannelJoined = false;

long long DCCIP = 0, DCCPort = 0, DCCSize = 0;
char* DCCFilename;

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

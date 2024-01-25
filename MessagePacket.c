#include"MessagePacket.h"
#include"errorCode.h"
#include<stdio.h>
#include<string.h>
int Packetize(short msgID, char* msgBuf, short msgLen, char* pktBuf, int pktBufSize)
{
	if ((sizeof(msgID)+sizeof(msgLen) + msgLen)>pktBufSize){
		return ILLEAGAL_MESSAGE_SIZE;
	}
	memset(pktBuf,'\0',pktBufSize);
	memcpy(pktBuf,&msgID,sizeof(msgID));
	memcpy(pktBuf+sizeof(msgID),&msgLen,sizeof(msgLen));
	memcpy(pktBuf+sizeof(msgID)+sizeof(msgLen),msgBuf,msgLen);
	return(sizeof(msgID)+sizeof(msgLen)+msgLen);
}

int Depacketize(char* pktBuf,int pktLen, short* msgID, char* msgBuf, short msgBufSize)
{
	short receivedMsgSize;
	memcpy(msgID,pktBuf,sizeof(short));
	memcpy(&receivedMsgSize,pktBuf+sizeof(short),sizeof(receivedMsgSize));
	if(receivedMsgSize>msgBufSize){
		return ILLEAGAL_MESSAGE_SIZE;
	}
	memset(msgBuf, '\0', msgBufSize);
	memcpy(msgBuf,pktBuf+sizeof(short)+sizeof(short),receivedMsgSize);
	return receivedMsgSize;
}


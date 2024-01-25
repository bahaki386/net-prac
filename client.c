#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include"errorCode.h"
#include"MessagePacket.h"
#define MAX 3000
#define TIMEOUT (2)				/* select関数のタイムアウト値 [秒] */

/* キーボードからの文字列入力・サーバへの送信処理関数 */
int SendMessage(int sock, struct sockaddr_in *pServAddr);

/* ソケットからのメッセージ受信・表示処理関数 */
int ReceiveMessage(int sock, struct sockaddr_in *pServAddr);

int uid=-1;

int main(int argc, char *argv[])
{
  char *servIP;					/* サーバのIPアドレス */
  unsigned short servPort;		/* サーバのポート番号 */
  
  int sock;						/* ソケットディスクリプタ */
  struct sockaddr_in servAddr;	/* サーバ用アドレス構造体 */

  int maxDescriptor;			/* select関数が扱うディスクリプタの最大値 */
  fd_set fdSet;					/* select関数が扱うディスクリプタの集合 */
  struct timeval tout;			/* select関数におけるタイムアウト設定用構造体 */

  /* 引数の数を確認する．*/
  if ((argc < 2) || (argc > 3)) {
    fprintf(stderr,"Usage: %s <Server IP> [<Echo Port>]\n", argv[0]);
    exit(1);
  }
  
  /* 第1引数からサーバのIPアドレスを取得する．*/
  servIP = argv[1];

  /* 第2引数からサーバのポート番号を取得する．*/
  if (argc == 3) {
    /* 引数が在ればサーバのポート番号として使用する．*/
    servPort = atoi(argv[2]);
  }
  else {
    servPort = 9000;
  }

  /* メッセージの送受信に使うソケットを作成する．*/
  sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0) {
    fprintf(stderr, "socket() failed");
    exit(1);
  }
  
  /* エコーサーバ用アドレス構造体へ必要な値を格納する．*/
  memset(&servAddr, 0, sizeof(servAddr));			/* 構造体をゼロで初期化 */
  servAddr.sin_family		= AF_INET;				/* インターネットアドレスファミリ */
  servAddr.sin_addr.s_addr	= inet_addr(servIP);	/* サーバのIPアドレス */
  servAddr.sin_port			= htons(servPort);		/* サーバのポート番号 */

  /* select関数で処理するディスクリプタの最大値として，ソケットの値を設定する．*/
  maxDescriptor = sock;

  /* 文字列入力・メッセージ送信，およびメッセージ受信・表示処理ループ */
  for (;;) {

    /* ディスクリプタの集合を初期化して，キーボートとソケットを設定する．*/
    FD_ZERO(&fdSet);				/* ゼロクリア */
    FD_SET(STDIN_FILENO, &fdSet);	/* キーボード(標準入力)用ディスクリプタを設定 */
    FD_SET(sock, &fdSet);			/* ソケットディスクリプタを設定 */

    /* タイムアウト値を設定する．*/
    tout.tv_sec  = TIMEOUT; /* 秒 */
    tout.tv_usec = 0;       /* マイクロ秒 */

    /* 各ディスクリプタに対する入力があるまでブロックする．*/
    if (select(maxDescriptor + 1, &fdSet, NULL, NULL, &tout) == 0) {
      /* タイムアウト */
      continue;
    }

    /* キーボードからの入力を確認する．*/
    if (FD_ISSET(STDIN_FILENO, &fdSet)) {
     /* キーボードからの入力があるので，文字列を読み込み，サーバへ送信する．*/
      if (SendMessage(sock, &servAddr) < 0) {
        break;
      }
    }
    
    /* ソケットからの入力を確認する．*/
    if (FD_ISSET(sock, &fdSet)) {
      /* ソケットからの入力があるので，メッセージを受信し，表示する．*/
      if (ReceiveMessage(sock, &servAddr) < 0) {
        break;
      }
    }
  }

  /* ソケットを閉じ，プログラムを終了する．*/
  close(sock);
  exit(0);
}
/*
 * キーボードからの文字列入力・サーバへのメッセージ送信処理関数
 */
int SendMessage(int sock, struct sockaddr_in *pServAddr)
{
  char string[MAX];	/* サーバへ送信する文字列 */
  memset(string,'\0',MAX);
  int stringLen;			/* サーバへ送信する文字列の長さ */
  int sendMsgLen;				/* 送信メッセージの長さ */
  char msg[MAX],pkt[MAX];
  memset(msg,'\0',MAX);
  memset(pkt,'\0',MAX);
  const char *joinstr="!join";
  const char *leavestr="!leave";
  char ls[7];
  char name[255];
  char msgflg=0;
  short msgid=MID_NONE;
  short testid;
  /* キーボードからの入力を読み込む．(※改行コードも含まれる．) */
  if (fgets(string, MAX, stdin) == NULL) {
    /*「Control + D」が入力された．またはエラー発生．*/
    return -1;
  }

  /* 入力文字列の長さを確認する．*/
  stringLen = strlen(string);
  if (stringLen < 1) {
    fprintf(stderr,"No input string.\n");
    return -1;
  }
  if(stringLen>6){
    strncpy(ls,string,5);
    if(strcmp(ls,joinstr)==0){
      sscanf(string,"!join %s\n",msg);
      msgflg=1;
      msgid=MID_JOIN_REQUEST;
    }else{
      strncpy(ls, string,6);
      if(strcmp(ls,leavestr)==0){
        if(uid==-1){
          printf("please join\n");
        }else{
          sprintf(msg,"%d",uid);
          msgflg=1;
          msgid=MID_LEAVE_REQUEST;
        }
      }
    }
  }
  if(!msgflg){
    if(uid==-1){
      printf("please join\n");
    }else{
      sprintf(msg,"%d:%s",uid,string);
      msgid=MID_CHAT_TEXT;
    }
  }
  sendMsgLen=Packetize(msgid,msg,(short)strlen(msg)+1,pkt,(int)sizeof(pkt));
  memcpy(&testid,pkt,sizeof(short));
  /* サーバへメッセージ(入力された文字列)を送信する．*/
  sendMsgLen = sendto(sock, pkt, sendMsgLen, 0,
    (struct sockaddr*)pServAddr, sizeof(*pServAddr));

  return 0;
}

/*
 * ソケットからのメッセージ受信・表示処理関数
 */
int ReceiveMessage(int sock, struct sockaddr_in *pServAddr)
{
  struct sockaddr_in fromAddr;	/* メッセージ送信元用アドレス構造体 */
  unsigned int fromAddrLen;		/* メッセージ送信元用アドレス構造体の長さ */
  char msgBuffer[MAX];	/* メッセージ送受信バッファ */
  memset(msgBuffer,'\0',MAX);
  int recvMsgLen;				/* 受信メッセージの長さ */
  short msgid;
  char contBuffer[MAX];
  memset(contBuffer,'\0',MAX);
  int contlen;
  /* エコーメッセージ送信元用アドレス構造体の長さを初期化する．*/
  fromAddrLen = sizeof(fromAddr);

  /* エコーメッセージを受信する．*/
  recvMsgLen = recvfrom(sock, msgBuffer, MAX, 0,
    (struct sockaddr*)&fromAddr, &fromAddrLen);
  if (recvMsgLen < 0) {
    fprintf(stderr, "recvfrom() failed");
    return -1;
  }
  contlen=Depacketize(msgBuffer,recvMsgLen,&msgid,contBuffer,MAX);
  switch(msgid){
    case MID_CHAT_TEXT:
      printf("%s\n",contBuffer);
      break;
    case MID_JOIN_RESPONSE:
      sscanf(contBuffer,"%d",&uid);
      printf("join:id=%d\n",uid);
      break;
    case MID_LEAVE_RESPONSE:
      printf("leaved\n");
      uid=-1;
  }
  return 0;
}

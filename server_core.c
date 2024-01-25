#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <errno.h>
#include <signal.h>
#include"errorCode.h"
#include"MessagePacket.h"
#include"userList.h"


#define MAX (3000)

int sock;							/* ソケットディスクリプタ */
void IOSignalHandler(int signo);	/* SIGIO 発生時のシグナルハンドラ */
User* ulHead;

int main(int argc, char *argv[])
{
  ulHead = mkList();
  unsigned short servPort;			/* サーバ(ローカル)のポート番号 */
  struct sockaddr_in servAddr;		/* サーバ(ローカル)用アドレス構造体 */
  struct sigaction sigAction;		/* シグナルハンドラ設定用構造体 */

  /* 引数の数を確認する．*/
  if (argc != 2) {
    fprintf(stderr,"Usage: %s <Echo Port>\n", argv[0]);
    exit(1);
  }
  servPort = atoi(argv[1]);

  /* メッセージの送受信に使うソケットを作成する．*/
  sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0) {
    perror("socket() failed");
    exit(1);
  }
  memset(&servAddr, 0, sizeof(servAddr));		/* 構造体をゼロで初期化 */
  servAddr.sin_family = AF_INET;				/* インターネットアドレスファミリ */
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY);	/* ワイルドカード */
  servAddr.sin_port = htons(servPort);			/* ローカルポート番号 */

  if (bind(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
    perror("bind() failed");
    exit(1);
  }

  /* シグナルハンドラを設定する．*/
  sigAction.sa_handler = IOSignalHandler;

  /* ハンドラ内でブロックするシグナルを設定する(全てのシグナルをブロックする)．*/
  if (sigfillset(&sigAction.sa_mask) < 0) {
    perror("sigfillset() failed\n");
    exit(1);
  }
  /* シグナルハンドラに関するオプション指定は無し．*/
  sigAction.sa_flags = 0;

  /* シグナルハンドラ設定用構造体を使って，シグナルハンドラを登録する．*/
  if (sigaction(SIGIO, &sigAction, 0) < 0) {
    perror("sigaction() failed\n");
    exit(1);
  }
  /* このプロセスがソケットに関するシグナルを受け取るための設定を行う．*/
  if (fcntl(sock, F_SETOWN, getpid()) < 0) {
    perror("Unable to set process owner to us\n");
    exit(1);
  }

  /* ソケットに対してノンブロッキングと非同期I/Oの設定を行う．*/
  if (fcntl(sock, F_SETFL, O_NONBLOCK | FASYNC) < 0) {
    perror("Unable to put the sock into nonblocking/async mode\n");
    exit(1);
  }

  /* メッセージの受信と送信以外の処理をする．*/
  for (;;) {
    sleep(2);
  }

}

/* SIGIO 発生時のシグナルハンドラ */
void IOSignalHandler(int signo)
{
  struct sockaddr_in clntAddr;	/* クライアント用アドレス構造体 */
  unsigned int clntAddrLen;		/* クライアント用アドレス構造体の長さ */
  char* msgBuffer=(char*)malloc(MAX);
  memset(msgBuffer,'\0',MAX);
  int recvMsgLen;				/* 受信メッセージの長さ */
  int sendMsgLen;				/* 送信メッセージの長さ */
  int recvContLen;
  short recv_msgID,send_msgID;
  char* contentBuffer=(char*)malloc(MAX);
  memset(contentBuffer,'\0',MAX);
  int uid;
  char* sbuf=(char*)malloc(MAX);
  memset(sbuf,'\0',MAX);
  char* name=(char*)malloc(255);
  memset(name,'\0',255);
  int sl;
  char* res=(char*)malloc(10);
  memset(res,'\0',10);
  char* msg=(char*)malloc(MAX);
  memset(msg,'\0',MAX);
  char* smsg=(char*)malloc(MAX);
  memset(smsg,'\0',MAX);
  int rclen;
  User* ue=ulHead;
  int sendl;
  /* 受信データがなくなるまで，受信と送信を繰り返す．*/
  do {
    /* クライアント用アドレス構造体の長さを初期化する．*/
    clntAddrLen = sizeof(clntAddr);

    /* クライアントからメッセージを受信する．(※この呼び出しはブロックしない) */
    recvMsgLen = recvfrom(sock, msgBuffer, MAX, 0,
      (struct sockaddr*)&clntAddr, &clntAddrLen);
    /* 受信メッセージの長さを確認する．*/
    if (recvMsgLen < 0) {
      /* errono が EWOULDBLOCK である場合，受信データが無くなったことを示す．*/
      /* EWOULDBLOCK は，許容できる唯一のエラー．*/
      if (errno != EWOULDBLOCK) {
        perror("recvfrom() failed\n");
        exit(1);
      }
    } else {
      /* クライアントのIPアドレスを表示する．*/
      printf("Handling client %s\n", inet_ntoa(clntAddr. sin_addr));
      recvContLen=Depacketize(msgBuffer,recvMsgLen,&recv_msgID,contentBuffer,MAX);
      fprintf(stderr,"%d\n",recv_msgID);
      switch(recv_msgID){
        case MID_CHAT_TEXT:
          sscanf(contentBuffer,"%d:%s",&uid,msg);
          name=getNameByID(ulHead,uid);
          sprintf(smsg,"%s:%s",name,msg);
          rclen=Packetize(MID_CHAT_TEXT,smsg,MAX,sbuf,MAX);
          while(ue->next!=NULL){
            sendl=sendto(ue->next->socket, sbuf, rclen, 0,
        	    (struct sockaddr*)(ue->next->addr), sizeof(ue->next->addr));
            fprintf(stderr,"sendl:%d\n",sendl);
            ue=ue->next;
          }
          break;
        case MID_JOIN_REQUEST:
          strncpy(contentBuffer,name,strlen(name)+1);
          uid=addUser(ulHead,sock,name,&clntAddr);
          sprintf(res,"%d",uid);
          rclen=Packetize(MID_JOIN_RESPONSE,res,strlen(res),sbuf,MAX);
          fprintf(stderr,"%d:%d\n",rclen,strlen(res));
          sendto(sock, sbuf, rclen, 0,
            (struct sockaddr*)&clntAddr, sizeof(clntAddr));
          free(name);
          break;
        case MID_LEAVE_REQUEST:
          sscanf(contentBuffer,"%d",&uid);
          delUser(ulHead,uid);
          rclen=Packetize(MID_LEAVE_RESPONSE,"OK",strlen("OK"),sbuf,MAX);
          sendto(sock, sbuf, rclen, 0,
            (struct sockaddr*)&clntAddr, sizeof(clntAddr));
          break;
      }
    }
  } while (recvMsgLen >= 0);
  free(sbuf);
  free(msgBuffer);
  free(msg);
  free(smsg);
}

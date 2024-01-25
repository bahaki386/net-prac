#include"userList.h"
#include<stdlib.h>
#include<string.h>
int addUser(User* start,int socket, char* name,struct sockaddr_in *a){
    User* cp=start;
    while(cp->next!=NULL){
        cp=cp->next;
    }
    User* c=(User*)malloc(sizeof(User));
    c->next=NULL;
    c->prev=cp;
    cp->next=c;
    c->userID=cp->userID+1;
    c->socket=socket;
    char* n=(char*)malloc(strlen(name)+1);
    memset(n,0,strlen(name)+1);
    memcpy(n,name,strlen(name));
    cp->name=n;
    struct sockaddr_in *addr=malloc(sizeof(struct sockaddr_in));
    memcpy(addr,a,sizeof(struct sockaddr_in));
    cp->addr=addr;
    return c->userID;
}
int getSocketByID(User* start,int userID){
    User* cp=start;
    while(cp!=NULL){
        if(cp->userID==userID){
            return cp->socket;
        }
        cp=cp->next;
    }
    return 0;
}
char* getNameByID(User* start,int userID){
    User* cp=start;
    while(cp!=NULL){
        if(cp->userID==userID){
            return cp->name;
        }
        cp=cp->next;
    }
    return NULL;
}
int delUser(User* start,int userID){
    User* cp=start;
    while(cp->userID!=userID){
        cp=cp->next;
    }
    cp->prev->next=cp->next;
    cp->next->prev=cp->next;
    free(cp->name);
    free(cp->addr);
    free(cp);
    return 0;
}

User* mkList(void){
    User* start=(User*)malloc(sizeof(User));
    start->userID=0;
    return start;
}

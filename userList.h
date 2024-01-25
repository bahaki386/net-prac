#ifndef _USER_LIST_H_
#define _USER_LIST_H_
#include<arpa/inet.h>
typedef struct User
{
	struct User* prev;
	struct User* next;
	char* name;
	int userID;
	int socket;
	struct sockaddr_in *addr;
}User;
	
int addUser(User* start,int socket,char *name,struct sockaddr_in *a);
int delUser(User* start,int userID);
int getSocketByID(User* start,int userID);
char* getNameByID(User* start,int userID);
User* mkList(void);

#endif

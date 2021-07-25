#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "util.h"

int main()
{
	struct user_info user;

	int fd = open("login_info",O_CREAT|O_RDWR,0777);
	if(fd==-1)
	{
		perror("OPEN ERROR");
		return -1;
	}
	strcpy(user.username,"Admin");
	strcpy(user.password,"admin@1234");
	user.status = 1;
	user.type=1;//admin
	strcpy(user.acc_num,"0");
	user.session_var=false;

	int n = write(fd,&user,sizeof(struct user_info));
	if(n==-1)
	{
		perror("OPEN ERROR");
		return -1;
	}
	close(fd);

	int fd1 = creat("transaction_info",0777);
	if(fd1==-1)
	{
		perror("CREAT ERROR");
		return -1;
	}
	close(fd1);

	int fd2 = creat("account_info",0777);
	if(fd2==-1)
	{
		perror("CREAT ERROR");
		return -1;
	}
	close(fd2);


	return 0;


}

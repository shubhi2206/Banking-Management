#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <stdbool.h>
#include <time.h>
#include "util.h"

#define MSG_BYTES 512
char accno[512];

void postMsgToClient(int sockfd_client, char *str,int flag)
{
	int numPacketsToSend = (strlen(str)-1)/MSG_BYTES + 1;
    int n = write(sockfd_client, &numPacketsToSend, sizeof(int));

    write(sockfd_client,&flag,sizeof(int));

    char *msgToSend = (char*)malloc(numPacketsToSend*MSG_BYTES);
    strcpy(msgToSend, str);
    int i;
    for(i = 0; i < numPacketsToSend; ++i) {
        int n = write(sockfd_client, msgToSend, MSG_BYTES);
        msgToSend += MSG_BYTES;
    }
}
char* getMsgFromClient(int sockfd_client)
{	
	int numPacketsToReceive = 0;
    int n = read(sockfd_client, &numPacketsToReceive, sizeof(int));
    if(n <= 0) {
        //shutdown(clientFD, SHUT_WR);
        return NULL;
    }
    char *str = (char*)malloc(numPacketsToReceive*MSG_BYTES);
    memset(str, 0, numPacketsToReceive*MSG_BYTES);
    char *str_p = str;
    int i;
    for(i = 0; i < numPacketsToReceive; ++i) {
        int n = read(sockfd_client, str, MSG_BYTES);
        str = str+MSG_BYTES;
    }
    return str_p;
}

void lock_unlock_file(int fd,short type)
{
	struct flock lock;
	lock.l_type=type;
	lock.l_whence=SEEK_SET;
	lock.l_start=0;
	lock.l_len=0;
	lock.l_pid=getpid();
	
	fcntl(fd,F_SETLKW,&lock);
	
}

struct user_info verifyUser(char *uname,char *pass)//for login verification
{
	printf("Authorizing the user......\n");
	struct user_info user;
	int read_var,retVar=-1;

	int fd = open("login_info",O_RDONLY);
	
	lock_unlock_file(fd,F_RDLCK);
	while((read_var=read(fd,&user,sizeof(user)))>0)
	{
		if(strcmp(user.username,uname)==0)
		{
			if(strcmp(user.password,pass)==0)
			{
				lock_unlock_file(fd,F_UNLCK);
				close(fd);
				return user;

			}
			else
				break;

		}
	}
	lock_unlock_file(fd,F_UNLCK);
	close(fd);
	user.type=-1;
	user.status=1;
	return user;
}

bool checkIfUserExists(char *uname,int fd)
{
	int read_var;
	struct user_info user;
	lseek(fd,0,SEEK_SET);

	lock_unlock_file(fd,F_RDLCK);
	while((read_var=read(fd,&user,sizeof(user)))>0)
	{
		if(strcmp(user.username,uname)==0)
		{
			lock_unlock_file(fd,F_UNLCK);
			return true;
		}
	}
	lock_unlock_file(fd,F_UNLCK);
	return false;
}

char* generateAccountNo()
{
	unsigned long var = (unsigned long)time(NULL);

	sprintf(accno,"%lu",var);
	return accno;
}

bool isJointAccount(int sockfd_client)
{
	postMsgToClient(sockfd_client,"Press 'J' for Joint account or 'I' for Individual account: ",1);
	char *choice = getMsgFromClient(sockfd_client);
	if(strncasecmp(choice,"J",1)==0)
		return true;
	else if(strncasecmp(choice,"I",1)==0)
		return false;
}
void addAccount(int sockfd_client)
{
	int fd = open("login_info",O_RDWR);
	int fd1 = open("account_info",O_RDWR|O_APPEND);

	struct user_info user1;
	struct user_info user2;
	struct account_info account;

	char *accno = generateAccountNo();
	//Joint account?
	if(isJointAccount(sockfd_client))
	{
		//create Joint account
		postMsgToClient(sockfd_client,"Enter the following details for new Joint Account:\n",0);
		postMsgToClient(sockfd_client,"\nUserName1 (should be unique): ",1);
		char *uname1 = getMsgFromClient(sockfd_client);
		if(checkIfUserExists(uname1,fd))
		{
			postMsgToClient(sockfd_client,"\nOOPS!! Username already exists. Please try different one!!\n\n",0);
			close(fd);
			close(fd1);
			return;
		}
		postMsgToClient(sockfd_client,"\nUserName2 (should be unique): ",1);
		char *uname2 = getMsgFromClient(sockfd_client);
		if(checkIfUserExists(uname2,fd))
		{
			postMsgToClient(sockfd_client,"\nOOPS!! Username already exists. Please try different one!!\n\n",0);
			close(fd);
			close(fd1);
			return;
		}

		postMsgToClient(sockfd_client,"\nJoint Password: ",1);
		char *pass = getMsgFromClient(sockfd_client);

		
		strcpy(user1.username,uname1);
		strcpy(user2.username,uname2);
		strcpy(user1.password,pass);
		strcpy(user2.password,pass);
		strcpy(user1.acc_num,accno);
		strcpy(user2.acc_num,accno);
	
		user1.status=1;
		user2.status=1;
		user1.type =2; //normal user
		user2.type =2;
		user1.session_var=false;
		user2.session_var=false;

		strcpy(account.acc_num,accno);
		account.balance=0.0;
		account.status=1;
		account.joint_acc=true;
		
		lock_unlock_file(fd,F_WRLCK);
		lock_unlock_file(fd1,F_WRLCK);
		write(fd,&user1,sizeof(user1));
		write(fd,&user2,sizeof(user2));
		write(fd1,&account,sizeof(account));
		lock_unlock_file(fd,F_UNLCK);
		lock_unlock_file(fd1,F_UNLCK);
		
	}
	else
	{
		postMsgToClient(sockfd_client,"Enter the following details for new account:\n",0);
		postMsgToClient(sockfd_client,"\nUserName (should be unique): ",1);
		char *uname = getMsgFromClient(sockfd_client);
		if(checkIfUserExists(uname,fd))
		{
			postMsgToClient(sockfd_client,"\nOOPS!! Username already exists. Please try different one!!\n\n",0);
			close(fd);
			close(fd1);
			return;
		}
		postMsgToClient(sockfd_client,"\nPassword: ",1);
		char *pass = getMsgFromClient(sockfd_client);
	
		
		strcpy(user1.username,uname);
		strcpy(user1.password,pass);
		strcpy(user1.acc_num,accno);
	
		user1.status=1;
		user1.type =2; //normal user
		user1.session_var=false;

		strcpy(account.acc_num,accno);
		account.balance=0.0;
		account.status=1;
		account.joint_acc=false;

		lock_unlock_file(fd,F_WRLCK);

		write(fd,&user1,sizeof(user1));
		lock_unlock_file(fd,F_UNLCK);
		lock_unlock_file(fd1,F_WRLCK);
		write(fd1,&account,sizeof(account));
		lock_unlock_file(fd1,F_UNLCK);
		
		
	}
	close(fd);
	close(fd1);
	postMsgToClient(sockfd_client,"********************* New Account created: Account No = ",0);
	postMsgToClient(sockfd_client,accno,0);
	return;

}
struct account_info getAccount(char *accnum)
{
	int read_var;
	struct account_info acc;
	int fd = open("account_info",O_RDONLY);

	lock_unlock_file(fd,F_RDLCK);
	while((read_var=read(fd,&acc,sizeof(acc)))>0)
	{
		if(strcmp(acc.acc_num,accnum)==0)
		{
			lock_unlock_file(fd,F_UNLCK);
			close(fd);
			return acc;
		}
	}
	lock_unlock_file(fd,F_UNLCK);
	close(fd);
	acc.status =0;
	return acc;
}

void updateUserStatus(char *accnum)
{
	int fd = open("login_info",O_RDWR);
	int read_var;
	struct user_info user;

	struct flock lock;
	lock.l_type =F_WRLCK;
	lock.l_pid = getpid();
	lock.l_whence = SEEK_CUR;
	lock.l_len = sizeof(struct user_info);

	while((read_var=read(fd,&user,sizeof(user)))>0)
	{
		if((strcmp(user.acc_num,accnum)==0) && user.status==1)
		{
			lock.l_start = -1*sizeof(user);
			fcntl(fd,F_SETLKW,&lock);

			user.status =0;
			lseek(fd,-1*sizeof(user),SEEK_CUR);
			write(fd,&user,sizeof(user));

			lock.l_type = F_UNLCK;
			fcntl(fd,F_SETLKW,&lock);
			close(fd);
			return;
		}

	}

	close(fd);


}
bool updateAccountStatus(int sockfd_client,char *accnum)
{
	int fd = open("account_info",O_RDWR);
	int read_var;
	struct account_info acc;

	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_pid=getpid();
	lock.l_whence= SEEK_CUR;
	lock.l_len= sizeof(struct account_info);

	while((read_var=read(fd,&acc,sizeof(acc)))>0)
	{
		if((strcmp(acc.acc_num,accnum)==0) && acc.status==1)
		{
			lock.l_start = -1*sizeof(acc);
			fcntl(fd,F_SETLKW,&lock);
			if(acc.balance >0)
			{
				postMsgToClient(sockfd_client,"This Account has some balance available. Press 'Y' to delete the account or press 'N' to cancel : ",1);
				char *choice = getMsgFromClient(sockfd_client);
				if(strncasecmp(choice,"Y",1)==0)
				{
					acc.status = 0;
					acc.balance =0.0;
					lseek(fd,-1*sizeof(acc),SEEK_CUR);
					write(fd,&acc,sizeof(acc));

					lock.l_type = F_UNLCK;
					fcntl(fd,F_SETLKW,&lock);

					close(fd);
					return true;
				}
				else if(strncasecmp(choice,"N",1)==0)
				{
					lock.l_type = F_UNLCK;
					fcntl(fd,F_SETLKW,&lock);

					close(fd);
					return false;
				}
			}
			else
			{
				acc.status = 0;
				
				lseek(fd,-1*sizeof(acc),SEEK_CUR);
				write(fd,&acc,sizeof(acc));

				lock.l_type = F_UNLCK;
				fcntl(fd,F_SETLKW,&lock);
				close(fd);
				return true;
			}
			
		}

	}
	lock.l_type = F_UNLCK;
	fcntl(fd,F_SETLKW,&lock);
	close(fd);
	return false;


}

void deleteAccount(int sockfd_client)
{
	
	struct account_info acc;
	int read_var;

	postMsgToClient(sockfd_client,"Enter the Account Number of the account you want to delete: ",1);
	char *accnum = getMsgFromClient(sockfd_client);

	acc = getAccount(accnum);
	if(acc.status==0)
	{
		postMsgToClient(sockfd_client,"\nEntered Account Number doesn't exist or has been already deleted!!!\n",0);
		return;
	}
	//update Account status
	if(updateAccountStatus(sockfd_client,accnum))
	{	
	//check if account is joint or not
		if(acc.joint_acc)
		{
		//Search for users n make status to 0
			updateUserStatus(accnum);
			updateUserStatus(accnum);
		}
		else
		{
		//Search for user n make status to 0
			updateUserStatus(accnum);
		}
	

		postMsgToClient(sockfd_client,"********************* Account deleted: Account No = ",0);
		postMsgToClient(sockfd_client,accnum,0);
	}

	return;

}
void showAllDetailsByUname(int sockfd_client,char *uname)
{
	struct user_info user;
	struct account_info account;
	int read_var,flag=0;

	int fd = open("login_info",O_RDONLY);
	int fd1 = open("account_info",O_RDONLY);

	lock_unlock_file(fd,F_RDLCK);
	while((read_var=read(fd,&user,sizeof(user)))>0)
	{
		if(strcmp(user.username,uname)==0)
		{
			flag=1;
			break;
		}
	}
	lock_unlock_file(fd,F_UNLCK);
	if(flag==0)//Username not found
	{
		postMsgToClient(sockfd_client,"***Sorry !! No user exist with the given username***",0);
		close(fd);
		close(fd1);
		return;
	}

	lock_unlock_file(fd1,F_RDLCK);
	while((read_var=read(fd1,&account,sizeof(account)))>0)
	{
		if(strcmp(account.acc_num,user.acc_num)==0)
			break;
	}
	lock_unlock_file(fd1,F_UNLCK);

	
	char *str= (char *)malloc(10*sizeof(char));
	if(user.status==1)
		strcpy(str,"Active");
	else
		strcpy(str,"Inactive");
	

	char *str1 =(char *)malloc(10*sizeof(char));
	if(account.joint_acc==true)
		strcpy(str1,"Joint Account");
	else
		strcpy(str1,"Individual Account");
	

	char *bal =(char *)malloc(10*sizeof(char));
	sprintf(bal,"%lf",account.balance);
	//print all details
	
	postMsgToClient(sockfd_client,"********** DETAILS **********\n",0);
	postMsgToClient(sockfd_client,"USERNAME: ",0);
	postMsgToClient(sockfd_client,user.username,0);
	postMsgToClient(sockfd_client,"\nPASSWORD: ",0);
	postMsgToClient(sockfd_client,user.password,0);
	postMsgToClient(sockfd_client,"\nUSER STATUS: ",0);
	postMsgToClient(sockfd_client,str,0);
	postMsgToClient(sockfd_client,"\nACCOUNT NUMBER: ",0);
	postMsgToClient(sockfd_client,user.acc_num,0);
	postMsgToClient(sockfd_client,"\nACCOUNT TYPE: ",0);
	postMsgToClient(sockfd_client,str1,0);
	postMsgToClient(sockfd_client,"\nBALANCE: Rs ",0);
	postMsgToClient(sockfd_client,bal,0);
	postMsgToClient(sockfd_client,"\n*********************************\n",0);

	close(fd);
	close(fd1);


}
void showAllDetailsByAccno(int sockfd_client,char *accnum)
{
	struct user_info user1;
	struct user_info user2;
	struct account_info account;
	int read_var,flag=0;

	int fd = open("login_info",O_RDONLY);
	int fd1 = open("account_info",O_RDONLY);

	lock_unlock_file(fd1,F_RDLCK);
	while((read_var=read(fd1,&account,sizeof(account)))>0)
	{
		if(strcmp(account.acc_num,accnum)==0)
		{
			flag=1;
			break;
		}
	}
	lock_unlock_file(fd1,F_UNLCK);
	if(flag==0)//Accnum not found
	{
		postMsgToClient(sockfd_client,"***Sorry !! No account exists with the given Account number***",0);
		close(fd);
		close(fd1);
		return;
	}
	char *str= (char *)malloc(10*sizeof(char));
	if(account.status==1)
		strcpy(str,"Active");
	else
		strcpy(str,"Inactive");
	
	char *str1 =(char *)malloc(10*sizeof(char));
	if(account.joint_acc==true)
		strcpy(str1,"Joint Account");
	else
		strcpy(str1,"Individual Account");
	

	char *bal =(char *)malloc(10*sizeof(char));
	sprintf(bal,"%lf",account.balance);

	postMsgToClient(sockfd_client,"********** ACCOUNT DETAILS **********\n",0);
	postMsgToClient(sockfd_client,"ACCOUNT NUMBER: ",0);
	postMsgToClient(sockfd_client,account.acc_num,0);
	postMsgToClient(sockfd_client,"\nACCOUNT STATUS: ",0);
	postMsgToClient(sockfd_client,str,0);
	postMsgToClient(sockfd_client,"\nACCOUNT TYPE: ",0);
	postMsgToClient(sockfd_client,str1,0);
	postMsgToClient(sockfd_client,"\nBALANCE: Rs ",0);
	postMsgToClient(sockfd_client,bal,0);
	postMsgToClient(sockfd_client,"\n************USER INFO**************\n",0);
	if(account.joint_acc)
	{
		lock_unlock_file(fd,F_RDLCK);
		while((read_var=read(fd,&user1,sizeof(user1)))>0)
		{
			if(strcmp(account.acc_num,user1.acc_num)==0)
				break;
		}
		while((read_var=read(fd,&user2,sizeof(user2)))>0)
		{
			if(strcmp(account.acc_num,user2.acc_num)==0)
				break;
		}
		lock_unlock_file(fd,F_UNLCK);
		//print details
		memset(str,0,10*sizeof(char));
		memset(str1,0,10*sizeof(char));
		if(user1.status==1)
			strcpy(str,"Active");
		else
		strcpy(str,"Inactive");
		

		if(user2.status==1)
			strcpy(str1,"Active");
		else
		strcpy(str1,"Inactive");
		

		postMsgToClient(sockfd_client,"\nUSERNAME 1: ",0);
		postMsgToClient(sockfd_client,user1.username,0);
		postMsgToClient(sockfd_client,"\t\t\t\tUSERNAME 2: ",0);
		postMsgToClient(sockfd_client,user2.username,0);
		postMsgToClient(sockfd_client,"\nPASSWORD: ",0);
		postMsgToClient(sockfd_client,user1.password,0);
		postMsgToClient(sockfd_client,"\t\t\t\tPASSWORD: ",0);
		postMsgToClient(sockfd_client,user2.password,0);
		postMsgToClient(sockfd_client,"\nSTATUS: ",0);
		postMsgToClient(sockfd_client,str,0);
		postMsgToClient(sockfd_client,"\t\t\t\tSTATUS: ",0);
		postMsgToClient(sockfd_client,str1,0);

	}
	else
	{	lock_unlock_file(fd,F_RDLCK);
		while((read_var=read(fd,&user1,sizeof(user1)))>0)
		{
			if(strcmp(account.acc_num,user1.acc_num)==0)
				break;
		}
		lock_unlock_file(fd,F_UNLCK);
		memset(str,0,10*sizeof(char));
		if(user1.status==1)
			strcpy(str,"Active");
		else
		strcpy(str,"Inactive");
		

		postMsgToClient(sockfd_client,"\nUSERNAME: ",0);
		postMsgToClient(sockfd_client,user1.username,0);
		postMsgToClient(sockfd_client,"\nPASSWORD: ",0);
		postMsgToClient(sockfd_client,user1.password,0);
		postMsgToClient(sockfd_client,"\nUSER STATUS: ",0);
		postMsgToClient(sockfd_client,str,0);
	}
	postMsgToClient(sockfd_client,"\n***********************************************************\n",0);
	close(fd);
	close(fd1);
}

void searchByUsername(int sockfd_client)
{
	postMsgToClient(sockfd_client,"\nEnter UserName: ",1);
	char *uname = getMsgFromClient(sockfd_client);

	showAllDetailsByUname(sockfd_client,uname);

}
void searchByAccNo(int sockfd_client)
{
	postMsgToClient(sockfd_client,"\nEnter Account Number: ",1);
	char *accnum = getMsgFromClient(sockfd_client);

	showAllDetailsByAccno(sockfd_client,accnum);
}

void searchAccount(int sockfd_client)
{
	postMsgToClient(sockfd_client,"\nChoose an option\n1. Search by UserName\n2. Search by Account Number\nPress any key to exit\n",1);
	char *msg = getMsgFromClient(sockfd_client);
	int choice =atoi(msg);
	switch(choice)
	{
		case 1:
			searchByUsername(sockfd_client);	
			break;
		case 2:
			searchByAccNo(sockfd_client);	
			break;
		default:
			break;
	}
	return;
}
void convertAccount(int sockfd_client,char *accno)
{
	int fd = open("login_info",O_RDWR);
	int fd1 = open("account_info",O_RDWR);

	struct user_info user;
	struct account_info account;
	int read_var;



	postMsgToClient(sockfd_client,"\nEnter UserName for the Joint user(should be unique): ",1);
	char *uname = getMsgFromClient(sockfd_client);
	if(checkIfUserExists(uname,fd))
	{
		postMsgToClient(sockfd_client,"\nOOPS!! Username already exists. Please try different one!!\n\n",0);
		close(fd);
		close(fd1);
		return;
	}
	postMsgToClient(sockfd_client,"\nEnter Password for the Joint user: ",1);
	char *pass = getMsgFromClient(sockfd_client);

	strcpy(user.username,uname);
	strcpy(user.password,pass);
	strcpy(user.acc_num,accno);
	
	user.status=1;
	user.type =2; //normal user
	user.session_var=false;

	lock_unlock_file(fd,F_WRLCK);
	write(fd,&user,sizeof(user));
	lock_unlock_file(fd,F_UNLCK);

	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_pid = getpid();
	lock.l_whence = SEEK_CUR;
	lock.l_len =sizeof(struct account_info);

	while((read_var=read(fd1,&account,sizeof(account)))>0)
	{
		if(strcmp(account.acc_num,accno)==0)
		{
			lock.l_start= -1*sizeof(account);
			fcntl(fd1,F_SETLKW,&lock);

			account.joint_acc =true;
			lseek(fd1,-1*sizeof(account),SEEK_CUR);
			write(fd1,&account,sizeof(account));

			lock.l_type =F_UNLCK;
			fcntl(fd1,F_SETLKW,&lock);
			break;
		}
	}
	close(fd);
	close(fd1);

}
void modifyAcc(int sockfd_client)
{
	postMsgToClient(sockfd_client,"\nEnter Account Number of the account you want to convert: ",1);
	char *accnum = getMsgFromClient(sockfd_client);

	struct account_info account =getAccount(accnum);
	if(account.status==0)
	{
		postMsgToClient(sockfd_client,"Account either doesn't exist or is deactivated!!! ",0);
		return;
	}
	if(account.joint_acc)
	{
		postMsgToClient(sockfd_client,"Oops.... This account is already a Joint Account!!!! ",0);
		return;
	}
	showAllDetailsByAccno(sockfd_client,accnum);
	convertAccount(sockfd_client,accnum);
	postMsgToClient(sockfd_client,"Yayyyy!! Above account is converted to Joint Account\n",0);
	showAllDetailsByAccno(sockfd_client,accnum);
}

void processAdmin(int sockfd_client,struct user_info user)
{
	postMsgToClient(sockfd_client,"********************************** Welcome ",0);
	postMsgToClient(sockfd_client,user.username,0);
	while(1)
	{
		postMsgToClient(sockfd_client,"\nChoose an option\n1. Add an account\n2. Delete an account\n3. Convert Individual Account to Joint\n4. Search Account\nPress any other key to logout\n",1);
		char *msg = getMsgFromClient(sockfd_client);
		int choice =atoi(msg);
		int flag=0;
		switch(choice)
		{
			case 1:
				addAccount(sockfd_client);
				break;
			case 2:
				deleteAccount(sockfd_client);
				break;
			case 3:
				modifyAcc(sockfd_client);
				break;
			case 4:
				searchAccount(sockfd_client);
				break;
			default:
				flag=1;
				break;

		}
		if(flag==1)	
			break;
	}
	

	return;
}
void printTransaction(int sockfd_client,struct transaction_info trans)
{	
	char *amount =(char *)malloc(100*sizeof(char));
	sprintf(amount,"%lf",trans.amount);

	char *avlbal =(char *)malloc(100*sizeof(char));
	sprintf(avlbal,"%lf",trans.avl_bal);

	
	postMsgToClient(sockfd_client,strcat(trans.acc_num,"\t"),0);
	postMsgToClient(sockfd_client,strcat(trans.t_type,"\t\t"),0);
	postMsgToClient(sockfd_client,strcat(amount,"\t"),0);
	postMsgToClient(sockfd_client,strcat(avlbal,"\t"),0);
	postMsgToClient(sockfd_client,trans.date,0);
	postMsgToClient(sockfd_client,"****************************************************************\n",0);
}

void updateTransaction(int sockfd_client,struct account_info acc,char ch,double amt,char *dnt,int flag)
{
	struct transaction_info t;
	int fd = open("transaction_info",O_WRONLY|O_APPEND);
	//printf("Trans acc num=%s\n",acc.acc_num);
	strcpy(t.acc_num,acc.acc_num);
	if(ch=='D')
		strcpy(t.t_type,"Credit");
	else if(ch=='W')
		strcpy(t.t_type,"Debit");

	t.amount = amt;
	t.avl_bal= acc.balance;
	strcpy(t.date,dnt);

	lock_unlock_file(fd,F_WRLCK);
	write(fd,&t,sizeof(t));
	lock_unlock_file(fd,F_UNLCK);

	if(flag)
	{
	postMsgToClient(sockfd_client,"Transaction Successful...\n\n",0);
	postMsgToClient(sockfd_client,"****************************************************************\n",0);
	postMsgToClient(sockfd_client,"ACCOUNT NUMBER\t",0);
	postMsgToClient(sockfd_client,"TRANSACTION TYPE\t",0);
	postMsgToClient(sockfd_client,"AMOUNT\t",0);
	postMsgToClient(sockfd_client,"AVAILABLE BALANCE\t",0);
	postMsgToClient(sockfd_client,"TRANSACTION DATE\n",0);

	printTransaction(sockfd_client,t);
	}
	
	close(fd);
}

void updateAccountBal(int sockfd_client,char *accno,double amt,char ch)
{
	struct account_info account;
	//printf("in updateaacountbal\n");
	//printf("ACCNO = %s\n",accno);
	int fd = open("account_info",O_RDWR);
	int read_var;
	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_pid=getpid();
	lock.l_whence= SEEK_CUR;
	lock.l_len= sizeof(struct account_info);

	while((read_var=read(fd,&account,sizeof(account)))>0)
	{
		if(strcmp(account.acc_num,accno)==0)
		{	
			lock.l_start= -1*sizeof(account);
			//printf("Outside lock\n");
			fcntl(fd,F_SETLKW,&lock);
			//printf("Inside lock\n");
			
			//printf("CH= %c\n",ch);
			if(ch=='D')
			{
				//printf("Before Account bal = %lf\n",account.balance);
				account.balance += amt;
				//printf("After Account bal = %lf\n",account.balance);
				lseek(fd,-1*sizeof(account),SEEK_CUR);
				write(fd,&account,sizeof(account));
				lock.l_type = F_UNLCK;
				fcntl(fd,F_SETLKW,&lock);
				//printf("Updated\n");
				break;
			}
			else if(ch=='W')
			{
				if((account.balance - amt)<0)
				{
					postMsgToClient(sockfd_client,"--------INSUFFICIENT BALANCE: Cannot withdraw--------\n\n",0);
					lock.l_type = F_UNLCK;
					fcntl(fd,F_SETLKW,&lock);
					close(fd);
					return;
				}
				else
				{
					account.balance -= amt;
					lseek(fd,-1*sizeof(account),SEEK_CUR);
					write(fd,&account,sizeof(account));
					//printf("Updated\n");
					lock.l_type = F_UNLCK;
					fcntl(fd,F_SETLKW,&lock);
					break;
				}
			}
			
		}
	}
	//printf("Account num = %s\n",account.acc_num);
	//printf("Account bal = %lf\n",account.balance);
	time_t t = time(NULL);
	char *dnt = ctime(&t);

	updateTransaction(sockfd_client,account,ch,amt,dnt,1);
	close(fd);

}
void deposit(int sockfd_client,char *accnum)
{
	//printf("ACCNUM = %s\n", accnum);
	postMsgToClient(sockfd_client,"\nEnter the amount you want to deposit: ",1);
	char *input = getMsgFromClient(sockfd_client);
	double amt =strtod(input,NULL);
	//printf("Amount = %lf\n",amt);

	if(amt<=0)
	{
		postMsgToClient(sockfd_client,"Kindly enter appropriate amount!!!\n",0);
		return;
	}
	updateAccountBal(sockfd_client,accnum,amt,'D');

}
void withdraw(int sockfd_client,char *accnum)
{
	//printf("ACCNUM = %s\n", accnum);
	postMsgToClient(sockfd_client,"\nEnter the amount you want to withdraw: ",1);
	char *input = getMsgFromClient(sockfd_client);
	double amt =strtod(input,NULL);
	//printf("Amount = %lf\n",amt);

	if(amt<=0)
	{
		postMsgToClient(sockfd_client,"Kindly enter appropriate amount!!!\n",0);
		return;
	}
	updateAccountBal(sockfd_client,accnum,amt,'W');

}

void viewAllTrans(int sockfd_client,char *accnum)
{
	postMsgToClient(sockfd_client,"****************************************************************\n",0);
	postMsgToClient(sockfd_client,"ACCOUNT NUMBER\t",0);
	postMsgToClient(sockfd_client,"TYPE\t",0);
	postMsgToClient(sockfd_client,"AMOUNT\t",0);
	postMsgToClient(sockfd_client,"AVAILABLE BALANCE\t",0);
	postMsgToClient(sockfd_client,"TRANSACTION DATE\n",0);

	int fd = open("transaction_info",O_RDONLY);
	int read_var;
	struct transaction_info t;

	lock_unlock_file(fd,F_RDLCK);
	while((read_var=read(fd,&t,sizeof(t)))>0)
	{
		if(strcmp(t.acc_num,accnum)==0)
		{
			printTransaction(sockfd_client,t);
		}
	}
	lock_unlock_file(fd,F_UNLCK);
	close(fd);
}
void changePass(int sockfd_client,struct user_info user)
{
	int fd = open("login_info",O_RDWR);
	int read_var;
	struct user_info u;
	postMsgToClient(sockfd_client,"\nEnter the old password: ",1);
	char *oldpass = getMsgFromClient(sockfd_client);

	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_pid=getpid();
	lock.l_whence= SEEK_CUR;
	lock.l_len= sizeof(struct user_info);

	if(strcmp(user.password,oldpass)==0)
	{
		postMsgToClient(sockfd_client,"\nEnter the new password: ",1);
		char *newpass = getMsgFromClient(sockfd_client);

		while((read_var=read(fd,&u,sizeof(u)))>0)
		{
			if(strcmp(user.username,u.username)==0)
			{
				lock.l_start = -1*sizeof(u);
				fcntl(fd,F_SETLKW,&lock);

				strcpy(u.password,newpass);
				lseek(fd,-1*sizeof(u),SEEK_CUR);
				write(fd,&u,sizeof(u));

				lock.l_type = F_UNLCK;
				fcntl(fd,F_SETLKW,&lock);

				postMsgToClient(sockfd_client,"PASSWORD CHANGED SUCCESSFULLY!!\n",0);
				break;
			}
		}

	}
	else
	{
		postMsgToClient(sockfd_client,"INCORRECT PASSWORD!!\n",0);
		close(fd);
		return;
	}
	close(fd);
	return;
}

void updateAccountBal_transfer(int sockfd_client,char *src_accnum,char *dest_accnum,double amount)
{
	int fd = open("account_info",O_RDWR);
	struct account_info acc1,acc2;
	int read_var;
	char *dnt1,*dnt2;

	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_pid=getpid();
	lock.l_whence= SEEK_CUR;
	lock.l_len= sizeof(struct account_info);

	//deduct money from source
	while((read_var=read(fd,&acc1,sizeof(acc1)))>0)
	{
		if(strcmp(acc1.acc_num,src_accnum)==0)
		{
			lock.l_start = -1*sizeof(acc1);
			fcntl(fd,F_SETLKW,&lock);

			if((acc1.balance - amount)<0)
			{
				postMsgToClient(sockfd_client,"--------INSUFFICIENT BALANCE: Cannot Transfer--------\n\n",0);
				lock.l_type = F_UNLCK;
				fcntl(fd,F_SETLKW,&lock);
				close(fd);
				return;
			}
			else
			{
				acc1.balance -= amount;
				lseek(fd,-1*sizeof(acc1),SEEK_CUR);
				write(fd,&acc1,sizeof(acc1));
				printf("Updated\n");
				lock.l_type = F_UNLCK;
				fcntl(fd,F_SETLKW,&lock);

				time_t t = time(NULL);
				dnt1 = ctime(&t);
				break;
			}	
		}
	}
	lseek(fd,0,SEEK_SET);
	//Add money to dest
	while((read_var=read(fd,&acc2,sizeof(acc2)))>0)
	{
		if(strcmp(acc2.acc_num,dest_accnum)==0)
		{
			lock.l_start = -1*sizeof(acc2);
			fcntl(fd,F_SETLKW,&lock);

		
			acc2.balance += amount;
			lseek(fd,-1*sizeof(acc2),SEEK_CUR);
			write(fd,&acc2,sizeof(acc2));
			printf("Updated in dest\n");
			
			lock.l_type = F_UNLCK;
			fcntl(fd,F_SETLKW,&lock);

			time_t t = time(NULL);
			dnt2 = ctime(&t);
			break;		
		}
	}
	updateTransaction(sockfd_client,acc1,'W',amount,dnt1,1);
	updateTransaction(sockfd_client,acc2,'D',amount,dnt2,0);

}
void transferFund(int sockfd_client,char *src_accnum)
{
	postMsgToClient(sockfd_client,"\nEnter the Account Number of the account you want to transfer fund to: ",1);
	char *dest_accnum = getMsgFromClient(sockfd_client);

	struct account_info dest_acc = getAccount(dest_accnum);
	if(dest_acc.status==0)
	{
		postMsgToClient(sockfd_client,"Oops...Entered Account does not exist!!!\n",0);
		return;
	}
	postMsgToClient(sockfd_client,"\nEnter the amount you want to transfer: ",1);
	char *input = getMsgFromClient(sockfd_client);
	double amt =strtod(input,NULL);
	//printf("Amount = %lf\n",amt);

	if(amt<=0)
	{
		postMsgToClient(sockfd_client,"Kindly enter appropriate amount!!!\n",0);
		return;
	}
	updateAccountBal_transfer(sockfd_client,src_accnum,dest_acc.acc_num,amt);


}

void processUser(int sockfd_client,struct user_info user)
{
	postMsgToClient(sockfd_client,"**************************** Welcome ",0);
	postMsgToClient(sockfd_client,user.username,0);
	postMsgToClient(sockfd_client,"\t(Account Number: ",0);
	postMsgToClient(sockfd_client,user.acc_num,0);
	postMsgToClient(sockfd_client,")",0);
	while(1)
	{
		postMsgToClient(sockfd_client,"\nChoose an option\n1. Deposit\n2. Withdraw\n3. Balance Enquiry\n4. Change Secret Key\n5. View All Transactions\n6. Transfer Funds\nPress any other key to logout\n",1);
		char *msg = getMsgFromClient(sockfd_client);
		int choice =atoi(msg);
		int flag=0;
		switch(choice)
		{
			case 1:
				deposit(sockfd_client,user.acc_num);
				break;
			case 2:
				withdraw(sockfd_client,user.acc_num);
				break;
			case 3:
			{
				struct account_info acc;
				acc = getAccount(user.acc_num);
				char *str = (char *)malloc(100*sizeof(char));
				sprintf(str,"%lf",acc.balance);
				postMsgToClient(sockfd_client,"\n****************************************************************\nCurrent Balance in your account: Rs ",0);
				postMsgToClient(sockfd_client,str,0);
				postMsgToClient(sockfd_client,"\n****************************************************************",0);
				break;
			}
				
			case 4:
				changePass(sockfd_client,user);
				break;
			case 5:
				viewAllTrans(sockfd_client,user.acc_num);
				break;
			case 6:
				transferFund(sockfd_client,user.acc_num);
				break;
			default:
				flag=1;
				break;

		}
		if(flag==1)	
			break;
	}
	return;
}
void updateSessionVar(struct user_info u,bool sess_val)
{
	int fd= open("login_info",O_RDWR);

	int read_var;
	struct user_info user;

	
	while((read_var=read(fd,&user,sizeof(user)))>0)
	{
		if(strcmp(user.username,u.username)==0)
		{
			user.session_var = sess_val;
			lseek(fd,-1*sizeof(user),SEEK_CUR);
			write(fd,&user,sizeof(user));
			
			close(fd);
			return;
		}
	}

	close(fd);

}

void login(int sockfd_client)
{
	char *uname,*pass;
	struct user_info user;

	postMsgToClient(sockfd_client,"Enter your UserName: ",1);
	uname = getMsgFromClient(sockfd_client);

	postMsgToClient(sockfd_client,"Enter your Password: ",1);
	pass = getMsgFromClient(sockfd_client);
	//printf(" %s\t%s\n",uname,pass);
	user = verifyUser(uname,pass);
	//printf("Type of user: %d\n",user.type);
	//check for user status
	if(user.status==1)//Active User
	{
		switch(user.type)
	{
		case -1:
			//Unauthorized user
			postMsgToClient(sockfd_client,"UNAUTHORIZED USER: Kindly check your Username/Password\n",0);
			break;
		case 1:
			//admin
			if(user.session_var)
			{
				postMsgToClient(sockfd_client,"\nExiting the system : You are already logged in from another window!!! \n",0);
			}
			else
			{
				//update session var to true
				updateSessionVar(user,true);
				processAdmin(sockfd_client,user);
				postMsgToClient(sockfd_client,"\nExiting the system....\n",0);
				updateSessionVar(user,false);
				//update session var to false
			}
			
			break;
		case 2:
			//normal user
			if(user.session_var)
			{
				postMsgToClient(sockfd_client,"\nExiting the system : You are already logged in from another window!!! \n",0);
			}
			else
			{
				//update session var to true
				updateSessionVar(user,true);
				processUser(sockfd_client,user);
				postMsgToClient(sockfd_client,"\nExiting the system....\n",0);
				updateSessionVar(user,false);
			}
				break;
		default:
			postMsgToClient(sockfd_client,"UNAUTHORIZED USER: Kindly check your Username/Password\n",0);
			break;
	}

	}
	else//INACTIVE USER
	{
		postMsgToClient(sockfd_client,"UNAUTHORIZED / DEACTIVATED USER!!!!\n",0);
	}
	
	return;

}

int main(int argc, char const *argv[])
{
	int sockfd_server,sockfd_client,port_num;

	port_num = atoi(argv[1]);
	struct sockaddr_in server_addr,client_addr;
	int addrlen = sizeof(client_addr);

	sockfd_server = socket(AF_INET,SOCK_STREAM,0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_num);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	bind(sockfd_server,(struct sockaddr *)&server_addr,sizeof(server_addr));
	listen(sockfd_server,5);

	while(1)
	{	
		printf("Waiting for another client....\n");
		if((sockfd_client = accept(sockfd_server,(struct sockaddr *)&client_addr,(socklen_t *)&addrlen)) < 0)
		{
			printf("Error in accepting the request\n");
			exit(EXIT_FAILURE);
		}
		printf("Connected to a client...\n");
		switch(fork())
		{
			case -1:
				printf("Error in fork()\n");
				break;
			case 0:
				//child process
				close(sockfd_server);
				login(sockfd_client);
				close(sockfd_client);
				printf("Server child Exiting\n");
				exit(EXIT_SUCCESS);
				break;
			default:
				//parent process
				close(sockfd_client);
				break;
		}

	}
	return 0; 
}

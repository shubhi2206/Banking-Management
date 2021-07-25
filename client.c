#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#define MSG_BYTES 512

struct rcv_msg
{
	char *msg;
	int flagVar;
};

struct rcv_msg getMsgFromServer(int sockfd_client)
{
	struct rcv_msg m;
	int numPacketsToReceive = 0,flag=0;
    int n = read(sockfd_client, &numPacketsToReceive, sizeof(int));
    
    read(sockfd_client,&flag,sizeof(int));

    char *str = (char*)malloc(numPacketsToReceive*MSG_BYTES);
    memset(str, 0, numPacketsToReceive*MSG_BYTES);
    char *str_p = str;
    int i;
    for(i = 0; i < numPacketsToReceive; ++i) {
        int n = read(sockfd_client, str, MSG_BYTES);
        str = str+MSG_BYTES;
    }
    
    m.flagVar=flag;
    m.msg =str_p;

    return m;
}
void postMsgToServer(int sockfd_client,char *str)
{
	int numPacketsToSend = (strlen(str)-1)/MSG_BYTES + 1;
    int n = write(sockfd_client, &numPacketsToSend, sizeof(int));
    char *msgToSend = (char*)malloc(numPacketsToSend*MSG_BYTES);
    strcpy(msgToSend, str);
    int i;
    for(i = 0; i < numPacketsToSend; ++i) {
        int n = write(sockfd_client, msgToSend, MSG_BYTES);
        msgToSend += MSG_BYTES;
    }
	
}

int main(int argc, char const *argv[])
{
	int sockfd_client,port_num;

	port_num = atoi(argv[2]);
	struct sockaddr_in server_addr;
	struct rcv_msg var;
	char msg_snd[512];

	sockfd_client = socket(AF_INET,SOCK_STREAM,0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_num);
	server_addr.sin_addr.s_addr = inet_addr(argv[1]);

	printf("Establishing connection with server.....\n");
	int ret = connect(sockfd_client,(struct sockaddr *)&server_addr,sizeof(server_addr));
	if(ret==-1)
	{
		printf("Connection Failed: Please try again\n");
		return -1;
	}
	else
		printf("****CONNECTION ESTABLISHED****\n");

	printf("---------------------------------------Welcome to Online Banking---------------------------------------\n\n");
	printf("-----------LOGIN ZONE-----------\n\n");

	while(1)
	{
		
		struct rcv_msg var = getMsgFromServer(sockfd_client);
		if(var.msg==NULL)
			break;
		printf("%s",var.msg);
		if(strncmp(var.msg,"UNAUTHORIZED",12)==0 || strncmp(var.msg,"\nExit",5)==0)
			break;
		
		
		if(var.flagVar==1)
		{
			
			memset(msg_snd,0,sizeof(msg_snd));
			if(strcasestr(var.msg,"password")!=NULL)
			{
				//msg_snd = getpass("");
				strcpy(msg_snd,getpass(""));
				postMsgToServer(sockfd_client,msg_snd);
			}
			else
			{
				scanf("%s",msg_snd);
				postMsgToServer(sockfd_client,msg_snd);
			}

		}
	

	}
	close(sockfd_client);
	printf("CONNECTION CLOSED......\n");
	return 0;


}
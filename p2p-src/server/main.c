#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <JEANP2PPRO.h>

#define PORT1 61000
#define ip1   "192.168.1.216"
#define ip2   "192.168.1.116"


static char pathname[50] = "./natinfo.log";
static int sfd;
static struct sockaddr_in sin, recv_sin;	
static int sin_len, recv_sin_len;
static char recv_str[100];
static int port = PORT1;
static char Uname[10];
static char Passwd[10];

int local_net_init(){
	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	//sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_addr.s_addr = inet_addr(ip1);

	sin.sin_port = htons(port);
	sin_len = sizeof(sin);

	sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(!sfd) return -1;

	if(bind(sfd, (struct sockaddr *)&sin, sizeof(sin)) != 0){
		printf("bind erro\n");
		return -2;
	}	

	printf("bind to port [%d]\n", port);

	return 0;
}

void init_recv_sin(){
	bzero(&recv_sin, sizeof(recv_sin));
	recv_sin.sin_family = AF_INET;
	recv_sin.sin_addr.s_addr = inet_addr("1.1.1.1");
	recv_sin.sin_port = htons(10000);
	recv_sin_len = sizeof(recv_sin);
}

int main(){
	int ret = 0;
	char Ctl_W[1];

	ret = local_net_init();
	if(ret < 0){
		printf("local net init failed!!\n");
		return ret;
	}

	init_recv_sin();

	printf("-------------------Welcome JEAN P2P SYSTEM---------------------\n");

	while(1){		
		recvfrom(sfd, recv_str, sizeof(recv_str), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		sscanf(recv_str, "%s %s", Uname, Passwd);
		printf("Recieve from %s [%d]:%s %s\n", inet_ntoa(recv_sin.sin_addr), ntohs(recv_sin.sin_port), Uname, Passwd);

		Ctl_W[0] = GET_OK; 
		sendto(sfd, Ctl_W, 1, 0, (struct sockaddr *)&recv_sin, recv_sin_len);
		printf("Send response.\n");
		
	}

	close(sfd);

	return 0;
}
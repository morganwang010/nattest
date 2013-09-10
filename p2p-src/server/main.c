#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <JEANP2PPRO.h>
#include <List.h>

#define MAX_TRY 10
#define PORT1 61000
#define ip1   "192.168.1.216"
#define ip2   "192.168.1.116"

#define PEER_SHEET_LEN 200
#define UNAME "wang"
#define PASSWD "123456"

extern char * t_name;

static char pathname[50] = "./natinfo.log";
static int sfd;
static struct sockaddr_in sin, recv_sin;	
static int sin_len, recv_sin_len;
static char recv_str[50];
static int port = PORT1;
static char Uname[10];
static char Passwd[10];

static int  Peers_Sheet_Index = 0;
static struct node_net *Peer_Login;

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

void set_rec_timeout(int usec, int sec){
	struct timeval tv_out;
    tv_out.tv_sec = sec;
    tv_out.tv_usec = usec;

	setsockopt(sfd,SOL_SOCKET,SO_RCVTIMEO,&tv_out, sizeof(tv_out));
}

struct node_net * Find_Peer(char * user){
	return find_item(user);
}

void Send_CMD(char Ctl, char res){
	char Ctl_W;
	char RESP[50];
	char RESP_res = res;
	Ctl_W = Ctl; 
	sprintf(RESP, "%c %c", Ctl_W, RESP_res);
	sendto(sfd, RESP, sizeof(RESP), 0, (struct sockaddr *)&recv_sin, recv_sin_len);
}

int Send_S_IP(char * name){
	char RESP[50];
	char GET_W;
	char res;
	struct node_net * tmp_node;
	int i;
	tmp_node = find_item(name);
	if(tmp_node == NULL) return -1;

	RESP[0] = S_IP;
	memcpy(RESP + 1, tmp_node->recv_sin_s, sizeof(struct sockaddr_in));
	
	set_rec_timeout(0, 1);	
	for(i = 0; i < MAX_TRY; i++){
		sendto(sfd, RESP, sizeof(RESP), 0, (struct sockaddr *)tmp_node->recv_sin_m, recv_sin_len);
		printf("Send slave ip to master!%s\n", inet_ntoa(tmp_node->recv_sin_s->sin_addr));
		recvfrom(sfd, recv_str, sizeof(recv_str), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		GET_W = recv_str[0];
		res = recv_str[2];
		if(GET_W == GET_REQ && res == 0x08){
			Send_CMD(GET_REQ, 0x09);
		   	break;
		}
	}
	set_rec_timeout(0, 0);	
}

int Peer_Login_Init(){
	Peer_Login = (struct node_net *)malloc(sizeof(struct node_net));
	if(Peer_Login == NULL) return -1;
	Peer_Login->Uname = (char *)malloc(10);
	Peer_Login->Passwd = (char *)malloc(10);
	Peer_Login->sin_len = sizeof(struct sockaddr_in);
	Peer_Login->recv_sin_s = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
	Peer_Login->recv_sin_m = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));

	if(Peer_Login->Uname && Peer_Login->Passwd && Peer_Login->sin_len && Peer_Login->recv_sin_s && Peer_Login->recv_sin_m) return 0;
	else return -1;
}

int Peer_Set(char * name, char * passwd, struct sockaddr_in * r){
	int ret = Peer_Login_Init();
	if(ret < 0){
		printf("Malloc failed when init Peer login sheet!!\n");
		return INIT_PEER_LOGIN_FAIL;
	}

	strcpy(Peer_Login->Uname, name);
	strcpy(Peer_Login->Passwd, passwd);
	memcpy(Peer_Login->recv_sin_m, r, sizeof(r));
	
	add_item(Peer_Login);

	return 0;
}

int Peer_Set_Slave(char * name, struct sockaddr_in * r){
	struct node_net *tmp_node = find_item(name);
	if(tmp_node != NULL){
		memcpy(tmp_node->recv_sin_s, r, sizeof(struct sockaddr_in));
		return 0;
	}
	return -1;
}

int main(){
	int ret = 0;
	char Get_W;
	char res;
	
	init_list();
	
	ret = local_net_init();
	if(ret < 0){
		printf("local net init failed!!\n");
		return ret;
	}

	init_recv_sin();

	printf("------------------- Welcome to JEAN P2P SYSTEM ---------------------\n");

	while(1){	

		recvfrom(sfd, recv_str, sizeof(recv_str), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		Get_W = recv_str[0];
		printf("OPCODE = %d\n", Get_W);

		switch(Get_W){
			case V_UAP:
				sscanf(recv_str, "%c %s %s", &Get_W, Uname, Passwd);
				//printf("Recieve from %s [%d]:%d %s %s\n", inet_ntoa(recv_sin.sin_addr), ntohs(recv_sin.sin_port), Get_W, Uname, Passwd);
				if(Uname == NULL){
					printf("Error:User name is NULL!!");
					break;
				}

				printf("Recieve from %s [%d]:%d %s ***\n", inet_ntoa(recv_sin.sin_addr), ntohs(recv_sin.sin_port), Get_W, Uname);
				printf("Verify result: Uname = %d Passwd = %d\n", strcmp(UNAME, Uname), strcmp(PASSWD, Passwd));

				if((strcmp(UNAME, Uname) != 0) || (strcmp(PASSWD, Passwd) != 0)){
					printf("Username or password error!!\n");
					Send_CMD(V_RESP, 0x2);
					printf("Send response.\n");
				}
				else{
					printf("Username and password verifying passed!!\n");

					int Insert_Success = 0;
					if(Find_Peer(Uname) == 0){
						if(Peers_Sheet_Index < PEER_SHEET_LEN){
							ret = Peer_Set(Uname, Passwd, &recv_sin);
							if(ret < 0){
								printf("Set peer failed!\n");
								return ret;
							}

							Peers_Sheet_Index++;
							Insert_Success = 1;
							printf("Registor success!! Now index at %d\n", Peers_Sheet_Index);
						}
						else{
							Insert_Success = 0;
							printf("Registor error!! Now index at %d\n", Peers_Sheet_Index);
						}
					}

					if(!Insert_Success)
						Send_CMD(V_RESP, 0x3);
					else
						Send_CMD(V_RESP, 0x1);
					printf("Send response.\n");
				}
				break;

			case V_UAP_S:
				sscanf(recv_str, "%c %s %s", &Get_W, Uname, Passwd);
				//printf("Recieve from %s [%d]:%d %s %s\n", inet_ntoa(recv_sin.sin_addr), ntohs(recv_sin.sin_port), Get_W, Uname, Passwd);
				if(Uname == NULL){
					printf("Error:User name is NULL!!");
					break;
				}

				printf("Recieve from %s [%d]:%d %s ***\n", inet_ntoa(recv_sin.sin_addr), ntohs(recv_sin.sin_port), Get_W, Uname);
				printf("Verify result: Uname = %d Passwd = %d\n", strcmp(UNAME, Uname), strcmp(PASSWD, Passwd));

				if((strcmp(UNAME, Uname) != 0) || (strcmp(PASSWD, Passwd) != 0)){
					printf("Username or password error!!\n");
					Send_CMD(GET_REQ, 0x5);
					printf("Send response.\n");
				}
				else{
					printf("Username and password verifying passed!!\n");

					int Find_Success = 0;
					if(Find_Peer(Uname) == 0){
							Find_Success = 1;
							printf("Node find success!! Now index at %d\n", Peers_Sheet_Index);
					}

					if(!Find_Success){
						Send_CMD(GET_REQ, 0x4);
						printf("Send response.\n");
						ret = Peer_Set_Slave(Uname, &recv_sin);
						if(ret < 0){
							printf("Login sheet is broken!\n");
							return LOGIN_SHEET_BROKEN;
						}
							
						Send_S_IP(Uname);
					}
					else{
						Send_CMD(GET_REQ, 0x6);
						printf("Send response.\n");
					}
				}
				break;


			case KEEP_CON:
				Send_CMD(GET_REQ, 0x03);
				printf("KEEP_CON has already responsed.\n");
				break;

			case GET_REQ:
				Get_W = recv_str[0];
				res = recv_str[2];
				if(res == 0x08){
					Send_CMD(GET_REQ, 0x9);
					printf("IP confirm pack has already responsed.\n");
				}
				break;


		}
	}

	empty_item();
	close(sfd);
	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <linux/mptcp.h>
#include <linux/tcp.h>
#include <arpa/inet.h>
#define SOL_TCP 6 /* TCP level */
int main(int argc, char** argv)
{
	int PORT;
	const char* FILE_NAME = "recv_file";

	int server_sock, client_sock;
	struct sockaddr_in server_addr, client_addr;
	int len, addr_len, recv_len, ret;

	FILE *file;
	int fsize = 0, nsize = 0;
	char buffer[1024];

	int enable = 1;

	if(argc != 2){
		fprintf(stderr, "usage: %s [port_number]\n", argv[0]);
		return -1;
	}
	PORT = atoi(argv[1]);

	server_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(server_sock < 0){
		perror("[server] socket() ");
		return -1;
	}

	// MPTCP_ENABLED
	ret = setsockopt(server_sock, SOL_TCP, MPTCP_ENABLED, &enable, sizeof(int));
	if(ret < 0){
		perror("[server] setsockopt() ");
		return -1;
	}

	memset(&server_addr, 0x00, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);

	ret = bind(server_sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
	if(ret < 0){
		perror("[server] bind() ");
		return -1;
	}	

	ret = listen(server_sock, 5);
	if(ret < 0){
		perror("[server] listen() ");
		return -1;
	}

	addr_len = sizeof(struct sockaddr_in);
	client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);	
	if(client_sock < 0){
		perror("[server] accept() ");
		return -1;
	}
	printf("[server] connected client\n");

	file = fopen(FILE_NAME, "wb");
	if(file == NULL){
		perror("[server] fopen() ");
		return -1;
	}

	do{
		nsize = recv(client_sock, buffer, 1024, 0);
		fwrite(buffer, sizeof(char), nsize, file);
	}while(nsize!=0);
	printf("[server] received file\n");
	
	fclose(file);
	close(client_sock);
	close(server_sock);

	return 0;
}

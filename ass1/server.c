#include <stdio.h>	
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
// #include <sys/types.h>
#define BACKLOG 15


void transferFile(int newHandle,char *filename,FILE *request){
	char buffer[1024];
	char header[1024];
	char msg[1024];
	bzero(header,sizeof(header));
	bzero(msg,sizeof(msg));
	bzero(buffer,sizeof(buffer));
	strcpy(msg,"no such file exists\n");
			int bytesRead;
			FILE * fp;
			if( (fp = fopen(filename,"r"))==NULL){
				sprintf(header, "%d", strlen(msg));
				send(newHandle,header,sizeof(header),0);
				send(newHandle,msg,sizeof(msg),0);
				return;
			}
			else{
				fseek(fp, 0L, SEEK_END);
				int sz = ftell(fp);
				sprintf(header, "%d", sz);
				// tostring(header, sz);
				send(newHandle,header,sizeof(header),0);
				fseek(fp, 0L, SEEK_SET);
				while((bytesRead=fread(buffer,1,sizeof(buffer),fp))>0){
					printf("%d\n",bytesRead);
					if (send(newHandle,buffer,sizeof(buffer),0)<0){
					// if (fwrite(buffer,1,sizeof(buffer),request)<0){
					perror("Error: Send Failed!!!");
					return;
					}
					printf("%d\n",bytesRead);
					bzero(buffer,sizeof(buffer));
				}
				return;
			}
}
int main(int argc,char *argv[])
{
	  if (argc != 2){
    	fprintf(stderr, "please give port number\n");
    	exit(1);
  }
	int port = strtol(argv[1], NULL, 0);
	// printf("%d\n", port);
	struct sockaddr_in serverAddr,clientAddr;
	socklen_t clientAddrSize;  
	int socketHandle = socket(PF_INET,SOCK_STREAM,0);
	if (socketHandle<0){
		perror("Error: Could not get socket handle!!!\n");
		exit(1);
	}
	
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	inet_pton(AF_INET,"127.0.0.1",&(serverAddr.sin_addr));
	if(bind(socketHandle,(struct sockaddr *)&serverAddr,sizeof(serverAddr))<0){
		perror("Error: Failed binding socket!!!");
		exit(1);
	}

	listen(socketHandle,BACKLOG);
	clientAddrSize = sizeof(clientAddr);
	while(1){
		int newHandle = accept(socketHandle,(struct sockaddr *)&clientAddr,&clientAddrSize);
		if (newHandle<0){
			perror("Error: Couldn't accept request from client");
			exit(1);
		}
		char msg[1024];
		bzero(msg,sizeof(msg));
		strcpy(msg,"Welcome to library.Please give a file name!!!");
		if (send(newHandle,msg,sizeof(msg),0)<0){
			perror("Error: Send Failed!!!");
			exit(1);
		}
		while(1){
			char filename[1024];
			bzero(filename,sizeof(filename));
			FILE *request = fdopen(newHandle, "r");
			if (fread(filename,1,sizeof(filename),request)<=0){
				perror("Error: Could not recieve!!!");
				break;
			}
			printf("%s\n",filename);
			transferFile(newHandle,filename,request);	
			printf("%s\n",filename);
		}
		printf("closing connection\n");
		close(newHandle);
	}
}
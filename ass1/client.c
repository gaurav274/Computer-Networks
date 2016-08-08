#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
int main(int argc, char const *argv[])
{
	struct addrinfo hints, *serverAddrInfo;
	int status;
	if (argc !=3){
		printf("Error: ip addr + port required");
		exit(1);
	}
	memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_STREAM;
    
    if (getaddrinfo(argv[1],argv[2], &hints, &serverAddrInfo) != 0) {
        perror("Error: Failed to get server addr: %s\n");
        exit(1);
    }
    
    int socketHandle = socket(serverAddrInfo->ai_family, serverAddrInfo->ai_socktype, serverAddrInfo->ai_protocol);
    
    if (socketHandle<0){
		perror("Error: Could not get socket handle!!!\n");
		exit(1);
	}
	
	if(connect(socketHandle,serverAddrInfo->ai_addr,serverAddrInfo->ai_addrlen)<0){
		perror("Error: Failed connecting to socket!!!\n");
		exit(1);
	}
	freeaddrinfo(serverAddrInfo);
	


	char buffer[1024];
	bzero(buffer,sizeof(buffer));
	FILE *request = fdopen(socketHandle, "ab+");
	
	fread(buffer,1,sizeof(buffer),request);
	// recv(socketHandle, buffer, 1024, 0);
  	fwrite(buffer,1,sizeof(buffer),stdout);
  	printf("%d\n", socketHandle);
  	int bytesRec;
  	while(1){
  		printf("goj\n");
  		char filename[1024];
  		bzero(filename,sizeof(filename));
  		scanf("%s",filename);
  		printf("%s\n",filename );
  		// if (send(socketHandle,filename,sizeof(filename),0)<0){
  		if (fwrite(filename,1,sizeof(filename),request)<0){
			perror("Error: Sending filename failed!!!");
			exit(1);
		}
		
		char header[1024];
		bzero(header,sizeof(header));
		// int fileSize = recv(socketHandle,header,sizeof(header),0);
		int fileSize = fread(header,1,sizeof(header),request);
		printf("hdkhk%s\n",header);
		
		int sz = atoi(header);
		printf("%d\n",sz );
		
		int count = sz ;
		bzero(buffer,sizeof(buffer));
		// bytesRec = recv(socketHandle, buffer, sizeof(buffer), 0);
		// printf("fuc%s\n", buffer);
		// while ((bytesRec = recv(socketHandle, buffer, sizeof(buffer), 0))>0){
			 while ((bytesRec = fread(buffer,1,sizeof(buffer),request))>0){
			printf("Printing------>%d\n",bytesRec );
			// printf("%s\n",buffer);
			if (count<bytesRec){
				fwrite(buffer, 1, count, stdout);	
				break;
			}
			fwrite(buffer, 1, sizeof(buffer), stdout);
			count-=bytesRec;
			printf("left->>>>>>%d\n",count );
			bzero(buffer,sizeof(buffer));
			if (count<0) break;
			}
  	}
	return 0;
}
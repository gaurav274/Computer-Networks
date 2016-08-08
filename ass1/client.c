#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#define BUFFER_SIZE 1024
int main(int argc, char const *argv[])
{
	struct addrinfo hints, *serverAddrInfo;
	int status;
	
	if (argc !=3){
		printf("Error: ip addr + port required");
		exit(1);
	}
	// Standard stuff
	memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_STREAM;
    
    // get address info of the server
    if (getaddrinfo(argv[1],argv[2], &hints, &serverAddrInfo) != 0) {
        perror("Error: Failed to get server addr: %s\n");
        exit(1);
    }

    // socket handle for client
    int socketHandle = socket(serverAddrInfo->ai_family, serverAddrInfo->ai_socktype, serverAddrInfo->ai_protocol);
    
    if (socketHandle<0){
		perror("Error: Could not get socket handle!!!\n");
		exit(1);
	}
	
	// connect with the server
	if(connect(socketHandle,serverAddrInfo->ai_addr,serverAddrInfo->ai_addrlen)<0){
		perror("Error: Failed connecting to socket!!!\n");
		exit(1);
	}

	freeaddrinfo(serverAddrInfo);
	


	char buffer[BUFFER_SIZE];
	bzero(buffer,sizeof(buffer));
	
	// get the file descriptor for sending and receiving
	FILE *request = fdopen(socketHandle, "ab+");
	
	// Reading the welcome message
	if(fread(buffer,1,sizeof(buffer),request)!=BUFFER_SIZE){
		perror("Error: Could not recieve welcome message!!!");
		exit(1);
	}

	fwrite(buffer,1,sizeof(buffer),stdout);
  
  	int bytesRec;

  	// Request files
  	while(1){
  		
  		// Read the filename from the stdin
  		char filename[BUFFER_SIZE];
  		bzero(filename,sizeof(filename));
  		scanf("%s",filename);
  		
  		// Sending the filename
  		if (fwrite(filename,1,sizeof(filename),request)!=BUFFER_SIZE){
			perror("Error: Sending filename failed!!!");
			exit(1);
		}
		
		//Receiving the file size in the header
		char header[BUFFER_SIZE];
		bzero(header,sizeof(header));
		if(fread(header,1,sizeof(header),request)!=BUFFER_SIZE){
			perror("Error: Could not recieve header!!!");
			exit(1);
		}
		int sz = atoi(header);
		
		int remainingSize = sz ;
		bzero(buffer,sizeof(buffer));
		
		// Receiving the file data
		// remainingSize stores the number of bytes yet to be received
		while ((fread(buffer,1,sizeof(buffer),request))>0){

			// if error occurs close connection and shut down client
			if(ferror(request)) { printf("Fatal Error!!!"); close(socketHandle); exit(1);} 
			
			// Last buffer as the remaining bytes is less than BUFFERSIZE
			if (remainingSize<BUFFER_SIZE){
				fwrite(buffer, 1, remainingSize, stdout);	
				break;
			}
			
			//print to the stdout
			fwrite(buffer, 1, sizeof(buffer), stdout);
			remainingSize -= BUFFER_SIZE;
			bzero(buffer,sizeof(buffer));
			// break if remaining bytes are <0
			if (remainingSize<0) break;
			}

		// Capuring next file message and printing to screen
		bzero(buffer,sizeof(buffer));
		if(fread(buffer,1,sizeof(buffer),request)!=BUFFER_SIZE){
			perror("Error: Could not recieve welcome message!!!");
			exit(1);
		}
		fwrite(buffer,1,sizeof(buffer),stdout);
  
  	}
	return 0;
}
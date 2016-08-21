
#include <stdio.h>	
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#define BACKLOG 10
#define BUFFER_SIZE 1024
/*transferFile sends the file requested by client
	Input:
		newHandle:- socket on which data is received and send
		filename:- name of the file to be send
		request:- file pointer of the socket
*/
void transferFile(int newHandle,char *filename,FILE *request){
	char buffer[BUFFER_SIZE]; // buffer used for sending file
	char header[BUFFER_SIZE]; // buffer used for sending header which is the size of the file
	char msg[BUFFER_SIZE];	  // error message to be send if file not found 
	FILE * fp;					
	
	bzero(header,sizeof(header));
	bzero(msg,sizeof(msg));
	bzero(buffer,sizeof(buffer));

	// storing the message to be send
	strcpy(msg,"no such file exists");
	
	

	/* If file couldn't be opened then: 
		header = size of the error message
		and write the error msg to the socket file
	*/ 
	if( (fp = fopen(filename,"r"))==NULL){
		sprintf(header, "%zu", strlen(msg));
		
		if (fwrite(header,1,sizeof(header),request)!=BUFFER_SIZE){
			perror("Error: Sending Header failed!!!");
			close(newHandle);
			exit(1);
		}
		if (fwrite(msg,1,sizeof(msg),request)!=BUFFER_SIZE){
			perror("Error: Sending message failed!!!");
			close(newHandle);
			exit(1);
		}
		return;
	}

	/*
		header = size of the file
		buffer = read file buffer by buffer and write the socket file till the end	
	*/
	else{
		
		// finding the number of bytes in the file 
		fseek(fp, 0L, SEEK_END);
		int sz = ftell(fp);

		sprintf(header, "%d", sz); 		// put this size of file to the header
		
		// write this header to the socket file
		if (fwrite(header,1,sizeof(header),request)!=BUFFER_SIZE){
			perror("Error: Sending Header failed!!!");
			close(newHandle);
			exit(1);
		}

		// setting file pointer back to start
		fseek(fp, 0L, SEEK_SET);
		
		// writing the file to the socket buffer by buffer	
		while((fread(buffer,1,sizeof(buffer),fp))>0){
			// if reading failed shut down server and close the connection
			if(ferror(fp)) { printf("Fatal Error\n"); close(newHandle); exit(1);}

			if (fwrite(buffer,1,sizeof(buffer),request)!=BUFFER_SIZE){
				perror("Error: Send Failed!!!");
				return;
			}
			bzero(buffer,sizeof(buffer));
		}
		return;
	}
}


int main(int argc,char *argv[])
{
	  if (argc != 2){
    	fprintf(stderr, "please give port number");
    	exit(1);
 	 }

	int port = strtol(argv[1], NULL, 0);
	struct sockaddr_in serverAddr,clientAddr;
	socklen_t clientAddrSize;  
	
	// getting socket handle
	int socketHandle = socket(PF_INET,SOCK_STREAM,0);
	if (socketHandle<0){
		perror("Error: Could not get socket handle!!!");
		exit(1);
	}
	// binding socket to the port
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	if(bind(socketHandle,(struct sockaddr *)&serverAddr,sizeof(serverAddr))<0){
		perror("Error: Failed binding socket!!!");
		exit(1);
	}
	// listening on the port
	if(listen(socketHandle,BACKLOG)<0){
		perror("Error: Listen failed!!!");
		exit(1);
	}
	clientAddrSize = sizeof(clientAddr);

	while(1){
		printf("Listening------->\n");
		// Get the new port over which fie transfer will occur
		int newHandle = accept(socketHandle,(struct sockaddr *)&clientAddr,&clientAddrSize);
		if (newHandle<0){
			perror("Error: Couldn't accept request from client");
			exit(1);
		}
		// Open the socket file
		FILE *request = fdopen(newHandle, "ab+");
		
		// msg:- stands for the welcome message send to client
		char msg[BUFFER_SIZE];
		bzero(msg,sizeof(msg));
		strcpy(msg,"Welcome to library.Please give a file name!!!\n");
		
		// sending the welcome message
		if (fwrite(msg,1,sizeof(msg),request)!=BUFFER_SIZE){
			perror("Error: Send Failed!!!");
			exit(1);
		}
		
		// Looping till the client dies  
		while(1){
			// filename stores the file required by client
			char filename[BUFFER_SIZE];
			bzero(filename,sizeof(filename));
			
			// receiving file name from client
			if (fread(filename,1,sizeof(filename),request)!=BUFFER_SIZE){
				perror("Error: Could not recieve!!!");
				break;
			}
			printf("%s\n",filename);
			transferFile(newHandle,filename,request);	
			printf("Success\n");
			
			bzero(msg,sizeof(msg));
			strcpy(msg,"\nGive another file!!!\n");
			if (fwrite(msg,1,sizeof(msg),request)!=BUFFER_SIZE){
			perror("Error: Send Failed!!!");
			exit(1);
			}
		}
		
		//Closing connection with current client	
		printf("Closing connection\n");
		close(newHandle);
		}
	
	return 0;
}
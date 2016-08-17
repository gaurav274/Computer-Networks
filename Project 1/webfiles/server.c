/*
* @Author: gaurav
* @Date:   2016-08-13 21:50:53
* @Last Modified by:   gaurav
* @Last Modified time: 2016-08-18 01:10:53
*/

#include <stdio.h>
#include <stdio.h>	
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#define BACKLOG 10
#define BUFFER_SIZE 1024

char *badFile = "HTTP/1.0 404 Not Found\r\n";
char *okMessage = "HTTP/1.0 200 OK\r\n";
char *badRequestProtocol = "HTTP/1.0 400 Bad Request\r\n";
char *badRequestMethod = "HTTP/1.0 501 Not Implemented\r\n";
char *badRequest = "HTTP/1.0 400 Bad Request\r\n";
char *badURIRequest = "HTTP/1.0 414 Request-URI Too Long\r\n";

char* getContentType(char* filename){
	if(strstr(filename,".pdf")){
		return "Application/pdf";
	}
	else if(strstr(filename,".html")||strstr(filename,".htm")){
		return "text/html";	
	}
	else if(strstr(filename,".txt")){
		return "text/plain";	
	}
	else if(strstr(filename,".jpeg")||strstr(filename,".jpg")){
		return "image/jpeg";	
	}
	else if(strstr(filename,".gif")){
		return "image/gif";	
	}
	else{
		return "application/octet-stream";
	}
}
void handleGet(int newHandle,char* uri){
	//printf("%s\n",uri );
	char filepath[5*BUFFER_SIZE];
	strcpy(filepath,getenv("PWD"));
	FILE *fp;
	char buffer[BUFFER_SIZE];

	if (!strcmp(uri,"/")||!strcmp(uri,"index.html")){
		uri = "/index.html";
	}
	// getting file path to send
	strcpy(&filepath[strlen(filepath)],uri);
	// sprintf(filename,"%s",uri+1);
	printf("FILE %s \nURI:%s\n",filepath,uri );
	if((fp = fopen(filepath,"rb"))==NULL){
		// file not found error
		send(newHandle,badFile,BUFFER_SIZE,0);
		return;
	}
	else{
		// file found
		send(newHandle,okMessage,strlen(okMessage),0);
		// preparing header
		char* contentType = getContentType(uri);
		// content length
		fseek(fp, 0L, SEEK_END);
		int contentLength = ftell(fp);
		fseek(fp, 0L, SEEK_SET);
		bzero(buffer,sizeof(buffer));
		sprintf(buffer,"Content-Length: %d\r\nContent-Type: %s\r\n\r\n",contentLength,contentType);
		send(newHandle,buffer,strlen(buffer),0);
		int bytesRead;
		// sending file
		while((bytesRead = fread(buffer,1,sizeof(buffer),fp))>0){
			// if reading failed shut down server and close the connection
			if(ferror(fp)) { printf("Fatal Error\n"); close(newHandle); exit(1);}

			if (send(newHandle,buffer,bytesRead,0)<0){
				perror("Error: Send Failed!!!");
				return;
			}
			bzero(buffer,sizeof(buffer));
		}
		return;
	}
}

void handleRequest(int newHandle){
	int bytesRead;
	char buffer[BUFFER_SIZE];
	bzero(buffer,sizeof(buffer));
	char method[10],uri[BUFFER_SIZE],version[10];
	// handles request line of max length 4*BUFFER_SIZE
	char requestLine[4*BUFFER_SIZE+1];
	bzero(requestLine,sizeof(requestLine));
	// GIve an error if URI is too long for the server to handle
	while(strstr(buffer,"\r\n")==NULL){
		
		if(strlen(requestLine)== 4*BUFFER_SIZE){
			send(newHandle,badURIRequest,BUFFER_SIZE,0);
			return;
		}
		bzero(buffer,sizeof(buffer));
		bytesRead = recv(newHandle,buffer,BUFFER_SIZE,0);
		// recv failed
		if(bytesRead==-1){
			send(newHandle, badRequest,BUFFER_SIZE,0);
			return;
		}
		// client closed connection before sending complete request
		else if(bytesRead==0){
			return;
		}
		// all good
		else{
			strcat(requestLine,buffer);
		}
	}
		// parsing method uri version out of the request message
		sscanf(requestLine,"%s %s %s",method,uri,version);
		//bzero(buffer,sizeof(buffer));
		// Loop till end of request message is found
		while(strstr(buffer, "\r\n\r\n") == NULL){
			bytesRead = recv(newHandle,buffer,BUFFER_SIZE,0);
			// recv failed
			if(bytesRead==-1){
				send(newHandle, badRequest,BUFFER_SIZE,0);
				return;
			}
			// client closed connection before sending complete request
			else if(bytesRead==0){
				return;
			}
		}
		// request not GET, reject
		if (strcmp(method,"GET")){
			send(newHandle,badRequestMethod,BUFFER_SIZE,0);
			return;
		}
		
		// protocol version != HTTP1.1
		if (strcmp(version,"HTTP/1.1")){
			printf("version\n");
			send(newHandle,badRequestProtocol,BUFFER_SIZE,0);
			return;
		}
		
		// all good now handle get request
		handleGet(newHandle,uri);
}




int main(int argc, char **argv ) {

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

	while(1){
		printf("Listening------->\n");
		// Get the new port over which file transfer will occur
		int newHandle = accept(socketHandle,(struct sockaddr *)&clientAddr,&clientAddrSize);
		if (newHandle<0){
			perror("Error: Couldn't accept request from client");
			exit(1);
		}
		int child;
		if((child = fork())==0){
			// Child thread

			//child doesn't need to listen on server socket so closing it
			close(socketHandle);
			//handle the request from client
			
			while(1){
				handleRequest(newHandle);
			}
			printf("NOT REACHABLE\n");
			//close the new socket
			shutdown (newHandle, SHUT_RDWR);
			close(newHandle);
			exit(0);
			}
		else if(child!=0) {
			//parent thread
			// parent thread doesnot need the new socket as child handles it
			printf("FORK%d\n", child);
			printf("handle%d\n", newHandle);
			close(newHandle);
		}
		else{
			//error occured
			perror("Error: Fork failed");
			//exit(1); 
		}

	}
    return 0;
}
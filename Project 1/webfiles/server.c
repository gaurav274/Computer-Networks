/*
* @Author: gaurav
* @Date:   2016-08-13 21:50:53
* @Last Modified by:   gaurav
* @Last Modified time: 2016-08-19 05:29:52
*/

#include <stdio.h>
#include <stdio.h>	
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#define BACKLOG 10
#define BUFFER_SIZE 1024
#define SERVER "project1Server"
char *badFile = "File not found";
char *okMessage = "Success";
char *badRequestMethod = "Unsupported method";
char *badRequest = "Malformed request";
char *badURIRequest = "Too Long";
char *serverError = "HTTP/1.0 500 Unexpected error\r\n";

// Thanks to http://stackoverflow.com/questions/4553012/checking-if-a-file-is-a-directory-or-just-a-file
int isDirectory(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}

void sendStatusMessage(int socket, char* protocol,int status, char *reason, char *comment) {
  char error[BUFFER_SIZE];
  bzero(error,sizeof(error));
  // donot send Content length in ok message
  if (status==200){
  	sprintf(error,"%s %d %s\r\n",protocol,status,reason);	
  }
  else{
  	sprintf(error,"%s %d %s\r\nContent-type: text/txt\r\nContent-Length: %d\r\n\r\n%s",protocol,status,reason,(int)strlen(comment),comment);
  }
  if(send(socket,error,strlen(error),0)!=strlen(error)){
  		send(socket,serverError,strlen(serverError),0);
  		close(socket);
  		exit(1);
  }
  return;
}

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

void showDir(char *filepath,int newHandle,char *protocol,char* connectionType,char *uri){
	DIR *dir;
	char directory[5*BUFFER_SIZE];
	bzero(directory,sizeof(directory));
	struct dirent *ent;
	char * ref = "<HEAD><TITLE>%s</TITLE></HEAD><BODY> %s</BODY>";
	if ((dir = opendir (filepath)) != NULL) {
	  /* print all the files and directories within directory */
	  while ((ent = readdir (dir)) != NULL) {

	  	if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                continue;
	  	
		char tmp[BUFFER_SIZE];
		char res[BUFFER_SIZE];
		bzero(tmp,sizeof(tmp));
		bzero(res,sizeof(res));
		strcpy(res,uri);
		strcat(res,"/");
		printf("RES%s\n", res);
		printf("%s\n", ent->d_name);
		sprintf(tmp,"<a class=\"icon file \"  href=\"%s\">%s</a><br/>",strcat(res,ent->d_name),ent->d_name);
	  	printf("%s\n",tmp);
	  	if (strlen(directory)+strlen(ent->d_name)<5*BUFFER_SIZE){
	  		strcat(directory,tmp);
	  	}
	  	// can't handle directory greater than buffer size
	  	else{
	  		sendStatusMessage(newHandle,protocol,500,"Internal Server Error","Unexpected error");
			close(newHandle);
			exit(1);
	  	}
	  }
	  printf("%s\n",directory );
	  closedir (dir);
	} 
	else {
	  /* could not open directory */
	  sendStatusMessage(newHandle, protocol,404,"Not Found",badFile);
	  return;
	}
	printf("%s\n",directory);
	sendStatusMessage(newHandle,protocol,200,"OK",okMessage);
	time_t now = time(NULL);
  	char date[200];
  	strftime(date, sizeof(date),"%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
	char buffer[2*BUFFER_SIZE];
	bzero(buffer,sizeof(buffer));
	sprintf(buffer,"Server: %s\r\nDate: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: %s\r\n\r\n",SERVER,date,(int)strlen(directory),"text/html",connectionType);
	strcat(buffer,directory);
		if(send(newHandle,buffer,strlen(buffer),0)!=strlen(buffer)){
			sendStatusMessage(newHandle,protocol,500,"Internal Server Error","Unexpected error");
			close(newHandle);
			exit(1);
		}
	return;	
}
void handleGet(int newHandle,char* uri,char* protocol,char * connectionType,char *root){
	//printf("%s\n",uri );
	char filepath[5*BUFFER_SIZE];
	strcpy(filepath,root);
	FILE *fp;
	char buffer[BUFFER_SIZE];
	
	// getting file path to send
	strcpy(&filepath[strlen(filepath)],uri);
	printf("FILEBEFORE%s\n",filepath );
	if (isDirectory(filepath)){
		// show index.html
		if (filepath[strlen(filepath)-1]=='/')
			strcat(filepath,"/index.html");
		// show directory
		else{
			showDir(filepath,newHandle, protocol,connectionType,uri);
			return;
		}
		printf("URI-->%s\n", filepath);
	}
	
	printf("FILE%sROOT%s\n",filepath,root );
	if((fp = fopen(filepath,"r"))==NULL){
		// file not found error
		sendStatusMessage(newHandle, protocol,404,"Not Found",badFile);
		return;
	}
	else{
		// file found
		sendStatusMessage(newHandle,protocol,200,"OK",okMessage);
		// preparing header
		char* contentType = getContentType(filepath);
		// content length
		fseek(fp, 0L, SEEK_END);
		int contentLength = ftell(fp);
		fseek(fp, 0L, SEEK_SET);
		bzero(buffer,sizeof(buffer));
		
		time_t now = time(NULL);
  		char date[200];
  		strftime(date, sizeof(date),"%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));


		sprintf(buffer,"Server: %s\r\nDate: %s\r\nContent-Length: %d\r\nContent-Type: %s \r\nConnection: %s\r\n\r\n",SERVER,date,contentLength,contentType,connectionType);
		if(send(newHandle,buffer,strlen(buffer),0)!=strlen(buffer)){
			sendStatusMessage(newHandle,protocol,500,"Internal Server Error","Unexpected error");
			close(newHandle);
			exit(1);
		}

		int bytesRead;
		bzero(buffer,sizeof(buffer));
		// sending file
		while((bytesRead = fread(buffer,1,sizeof(buffer),fp))>0){
			// if reading failed shut down server and close the connection sending error message
			if (send(newHandle,buffer,bytesRead,0)!=bytesRead || ferror(fp)){
				sendStatusMessage(newHandle,protocol,500,"Internal Server Error","Unexpected error");
				close(newHandle); 
				exit(1);
			}
			bzero(buffer,sizeof(buffer));
		}
		return;
	}
}

void handleRequest(int newHandle,char * root){
	// persistent connection
	// connection =1 if keep-alive else connection=0
	int connection = 1;
	while(1){
		printf("START\n%d",getpid());
		int bytesRead;
		// by default assumed to be http1.1
		char *protocol = "HTTP/1.1";
		char buffer[BUFFER_SIZE];
		bzero(buffer,sizeof(buffer));
		char method[10],uri[8*BUFFER_SIZE],version[10];
		// handles request  of max length 8*BUFFER_SIZE
		char request[8*BUFFER_SIZE+1];
		bzero(request,sizeof(request));
		// GIve an error if URI is too long for the server to handle
		while(strstr(request,"\r\n")==NULL){
			
			if(strlen(request)== 4*BUFFER_SIZE){
				sendStatusMessage(newHandle,protocol,414 ,"Request-URI",badURIRequest);
				return;
			}
			bzero(buffer,sizeof(buffer));
			bytesRead = recv(newHandle,buffer,BUFFER_SIZE,0);
			// recv failed
			if(bytesRead<0){
				// client closed the connection with error
				if (errno==ECONNRESET||errno==ECONNABORTED){
					close(newHandle);
					exit(1);
				}
				sendStatusMessage(newHandle,protocol,400,"Bad Request",badRequest);
				return;
			}
			// client closed connection before sending complete request
			else if(bytesRead==0){
				return;
			}
			// all good
			else{
				strcat(request,buffer);
			}
		}
			// parsing method uri version out of the request message
			sscanf(request,"%s %s %s",method,uri,version);
			protocol = version;
			
			// Loop till end of request message is found
			while(strstr(buffer, "\r\n\r\n") == NULL){
				bytesRead = recv(newHandle,buffer,BUFFER_SIZE,0);
				// recv failed
				if(bytesRead<0){
					if (errno==ECONNRESET||errno==ECONNABORTED){
						close(newHandle);
						exit(1);
					}
					sendStatusMessage(newHandle,protocol,400,"Bad Request",badRequest);
					return;
				}
				// client closed connection before sending complete request
				else if(bytesRead==0){
					return;
				}
				// all good
				else{
					strcat(request,buffer);
				}

			}



			// retrieving connection status from request
			char connectionType[10] = "keep-alive";
			char *tmp;
			
			if((tmp = strstr(request,"Connection:"))!=NULL){
				char *end = strstr(tmp+12,"\r\n");
				char* start = tmp+12;
				tmp =start;	
				for ( ; tmp<=end; ++tmp) *tmp = tolower(*tmp);
				if (tmp) 
					strncpy(connectionType, start, end-start);
			}
			printf("%s\n",request );
			printf("connectionType %s\n", connectionType);
			// close connection checking
			if (!strcmp(connectionType,"close"))
				printf("CONNE%s\n",connectionType );
				connection = 0;

			// request not GET, reject
			if (strcmp(method,"GET")){
				sendStatusMessage(newHandle,protocol,501,"Not Implemented",badRequestMethod);
				return;
			}
			
			// protocol version != HTTP1.1
			if (strcmp(version,"HTTP/1.0")&&strcmp(version,"HTTP/1.1")){
				sendStatusMessage(newHandle,protocol,400,"Bad Request",badRequest);
				return;
			}
			
			// all good now handle get request
			handleGet(newHandle,uri,protocol,connectionType,root);
			
	}
}




int main(int argc, char **argv ) {

	int port = 4000;
	char * root = getenv("PWD");
	char c; 
	//Parsing the command line arguments
    while ((c = getopt (argc, argv, "p:r:")) != -1)
        switch (c)
        {
            case 'r':
                root = malloc(strlen(optarg));
                strcpy(root,optarg);
                break;
            case 'p':
                port = strtol(optarg, NULL, 0);
                break;
            case '?':
                fprintf(stderr,"Wrong arguments given!!!\n");
                exit(1);
            default:
                exit(1);
        }
	
	
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
	int newHandle;
	while(1){
		printf("Listening------->\n");
		// Get the new port over which file transfer will occur
		newHandle = accept(socketHandle,(struct sockaddr *)&clientAddr,&clientAddrSize);
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
			
			
		    handleRequest(newHandle,root);
			printf("SUCCESS EXIT\n");
			
			//close the new socket
			//shutdown (newHandle, SHUT_RDWR);
			close(newHandle);
			exit(0);
			}
		else if(child!=0) {
			//parent thread
			// parent thread doesnot need the new socket as child handles it
			//close(newHandle);
		}
		else{
			//error occured
			perror("Error: Fork failed");
			//exit(1); 
		}

	}
    return 0;
}
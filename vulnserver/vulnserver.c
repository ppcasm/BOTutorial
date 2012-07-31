//SVWSVR - Simple Vulnerable WebserVeR by: ppcasm

/*

****WARNING****: This program is purposely vulnerable (as the name implies), as
such, please don't be stupid and run this on your internet connected machine, you
_WILL_ get hacked. This program is for educational purposes only, and should be 
taken as an example of what _NOT_ to do when writing software; thanks. 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <mswsock.h>
#include <windows.h>
#include <ws2tcpip.h>

//Protos...
int process_request(unsigned char *buf, SOCKET acceptsock);
int process_GET(unsigned char *getbuf, SOCKET acceptsock);

int main(int argc, char *argv[])
{ 
  if(argc!=2)
  {
      printf("Wrong Usage: %s <port>\n", argv[0]);
      return -1;
  }
  
  //initializations and winsock startup.
  unsigned short port = atoi(argv[1]);
  SOCKET listensock;
  struct sockaddr_in addr;
  WSADATA wsadata;
  WSAStartup(MAKEWORD(1, 1), &wsadata);
  
  //Temp buf for send/recv.
  unsigned char buf[1024];
  
  //Set up listen socket.
  listensock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  
  if(listensock == INVALID_SOCKET)
  {
      printf("Error: Socket.\n");
      closesocket(listensock);
      WSACleanup();
      return -1;
  }
  
  //Set addresses.
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);
  
  //Bind listening socket.
  if(bind(listensock, (struct sockaddr *)&addr, sizeof(addr))==SOCKET_ERROR)
  {
      printf("Error: Bind.\n");
      closesocket(listensock);
      WSACleanup();
      return -1;
  }
  
  //Listen for connection, max connections.
  if(listen(listensock, SOMAXCONN)==SOCKET_ERROR)
  {
      printf("Error: Listen.\n");
      closesocket(listensock);
      WSACleanup();
      return -1;
  }
  
  //Get host name to show what host is being used.
  if(gethostname(buf, sizeof(buf))==SOCKET_ERROR)
  {
      printf("Error: Gethostname.\n");
      closesocket(listensock);
      WSACleanup();
      return -1;
  }
  
  printf("Webserver started: (HOST: %s) - (PORT: %d)\n", buf, port);
  
  //Setup accepting socket.
  SOCKET acceptsock;
  acceptsock = accept(listensock, NULL, NULL);
  if(acceptsock == INVALID_SOCKET)
  {
      printf("Error: Accept.\n");
      closesocket(listensock);
      WSACleanup();
      return -1;
  }

  //Clear temp buffer.
  memset(buf, 0, sizeof(buf));
  
  //Recieve data from client and store in temp buffer.
  if(recv(acceptsock, buf, sizeof(buf)-1, 0) == SOCKET_ERROR)
  {
      printf("Error: Recv.\n");
      closesocket(listensock);
      closesocket(acceptsock);
      WSACleanup();
      return -1;
  }
  
  //printf("%s\n", buf);
  
  //Process client request.
  if(process_request(buf, acceptsock)<0)
  {
      printf("Error: process_request.\n");
      closesocket(listensock);
      closesocket(acceptsock);
      WSACleanup();
      return -1;
  } 
  
  closesocket(listensock);
  closesocket(acceptsock);  
  WSACleanup();
  return 0;
}

int process_request(unsigned char *buf, SOCKET acceptsock)
{
    //Check for GET request.
    if(!strncmp(buf, "GET ", sizeof("GET ")-1))
    {
        //If the request is a GET request, process it.
        printf("Processing GET request...\n");
        if(process_GET(&buf[sizeof("GET ")-1], acceptsock)==-1)
        {
            printf("Error: process_GET.\n");
            return 0;
        }
        return 0;
    } 
    
    //Fallback and handle "unknown requests."
    else 
    {
        //Signal error flag in main.
        return -1;
    }
    
  return 0;
}

int process_GET(unsigned char *getbuf, SOCKET acceptsock)
{
     int i = 0;
     unsigned long flsize = 0;
     unsigned char filename[40];
     FILE *fp;
     
     //Parse out GET header filename.
     while(getbuf[i]!=' ') 
     {
         filename[i]=getbuf[i];
         i++;
     }
     
     if(i==1)
     {
         //At this point we assume no filename was given and treat test.html as root.
         printf("Assume test.html as root.\n");
         strncpy(&filename[0], "/test.html", sizeof("/test.html"));
         i = sizeof("/test.html");
     }
     
     //Null terminate filename since filename ended with return.
     filename[i]=0;
     
     //Print filename...
     printf("GET requested file: %s\n", filename);
          
     //Now, open file, if it doesn't exist, print simple error.
     fp=fopen(&filename[1], "rb"); //Truncate filename in this case.
     if(fp==NULL)
     {
        if(send(acceptsock, "File Does Not Exist: GET FAILED!\n", sizeof("File Does Not Exist: GET FAILED!\n")-1, 0) == SOCKET_ERROR)
        {
            closesocket(acceptsock);
            WSACleanup();
            fclose(fp);
            return -1;
        } 
       return 0;
     }
     
     //Get file size.
     fseek(fp, 0L, SEEK_END);
     flsize = ftell(fp);
     fseek(fp, 0L, SEEK_SET);
     
     //Allocate file buffer (static, for now.)
     unsigned char buffer[flsize];
     
     //Read file into buffer.
     fread(buffer, sizeof(buffer), 1, fp);
     
     //Send file over socket.
     if(send(acceptsock, buffer, sizeof(buffer)-1, 0) == SOCKET_ERROR)
        {
            closesocket(acceptsock);
            WSACleanup();
            fclose(fp);
            return -1;
        } 
     
     
   fclose(fp);
   return 0;  
}

#ifndef CLIENTTCP_HEADER
#define CLIENTTCP_HEADER

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <netdb.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 21
#define ADRESS "ftp.up.pt"
#define URL "ftp://anonymous:@ftp.up.pt/debian/README"


struct socket_response {
    char response[1024];
    char code[3];
};

struct args
{
  char user[128];       
  char password[128];  
  char host[256];       
  char path[240];       
  char file_name[128];  
  char host_name[128]; 
  char ip[128];         

};


#define h_addr h_addr_list[0]	

int parseArgs(char *url, struct args *args);

int getFileName(struct args * args);

int connection(char *ip,int port);

int get_response(int sockfd, struct socket_response * response);

int write_to_socket(int sockfd, int hasBody, char *header, char *body);

int write_command(int sockfd, char* header, char* body, int reading_file, struct socket_response* response);

int passive_mode(int socketfd);

int download_file(int sockfd, int data_sockfd, struct args * parsed_args);


#endif 
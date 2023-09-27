/**      (C)2000-2021 FEUP
 *       tidy up some includes and parameters
 * */
#include "clientTCP.h"

int parseArgs(char *url, struct args *args){
    //ftp://ftp.up.pt/pub/...
    //ftp://[<user>:<password>@]<host>/<url-path>
    //ftp://anonymous:password@ftp.up.pt/debian/README
    //ftp://anonymous:@ftp.up.pt/debian/README
    printf("PARSE FUNCTION\n");
    
    char* ftp = strtok(url, "/");       // ftp:
    char* urlrest = strtok(NULL, "/");  // [<user>:<password>@]<host>  
    char* path = strtok(NULL, "");      // <url-path>
  
    if (ftp == NULL || urlrest == NULL || path == NULL) {
        fprintf(stderr, "Invalid URL!\n");
        return 1;
    }

    if (strcmp(ftp, "ftp:") != 0){
        printf("Error: ftp\n");
        return 1;
    }

    char* login_data;
    char* user;
    char* pass;
    char* host;

    //ftp://[<user>:<password>@]<host>/<url-path>
    
    if(strchr(urlrest,'@')){
        printf("ENTREI NO IF\n");
        
        login_data = strtok(urlrest, "@");
        host = strtok(NULL, "/");

        user = strtok(urlrest, ":");
        pass=  strtok(NULL, "");

        if (pass == NULL){
            printf("NAO EXISTE PASSWORD");
            pass = "pass";
        }  

        strcpy(args->host, host);
    }

    else{

        user = "anonymous";
        pass = "pass";
        strcpy(args->host, urlrest);

    }
    
    strcpy(args->path, path);
    strcpy(args->user, user);
    strcpy(args->password, pass);

    if (getFileName(args) != 0){
        printf("Error: getFileName()\n");
        return -1;
    }


    struct hostent * h;

    if ((h = gethostbyname(args->host)) == NULL) {  
        herror("gethostbyname");
        return -1;
    }

    printf("GOT HOST NAME\n");

    char * host_name = h->h_name;
    strcpy(args->host_name, host_name);
    printf("HOST NAME: %s\n",host_name);
    

    char * ip = inet_ntoa(*((struct in_addr *)h->h_addr));
    strcpy(args->ip, ip);
    printf("IP: %s\n",ip);
    
    
    printf("\n||||||||||||||||||||||||||||||||||||||||||||\n");
    
    printf("\nUser: %s\n", args->user);
    printf("Password: %s\n", args->password);
    printf("Host: %s\n", args->host);
    printf("Path: %s\n", args->path);
    printf("File name: %s\n", args->file_name);
    printf("Host name  : %s\n", args->host_name);
    printf("IP Address : %s\n\n", args->ip);
    return 0;
    
}


int getFileName(struct args * args){

    char fullpath[256];
    strcpy(fullpath, args->path);

    char* token = strtok(fullpath, "/");

    while( token != NULL ) {

        strcpy(args->file_name, token);
        token = strtok(NULL, "/");   
    }

    printf("NOME DO FICHEIRO: %s\n",args->file_name);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////



int connection(char *ip,int port){

    printf("ENTREI NO CONNECT\n");

    struct sockaddr_in server_addr;
    //server address handling
    bzero((char *) &server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);    //32 bit Internet address network byte ordered
    server_addr.sin_port = htons(port);                 //server TCP port must be network byte ordered 

    //open a TCP socket
    int sockfd;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(-1);
    }
    //connect to the server
    if (connect(sockfd,(struct sockaddr *) &server_addr,sizeof(server_addr)) < 0) {
        perror("connect()");
        exit(-1);
    }

    return sockfd;

}

int get_response(int sockfd, struct socket_response * response) {

    strcpy(response->response, "");
	FILE * socket = fdopen(sockfd, "r");

	char * buf;
	size_t n_bytes = 0;
    
	while (getline(&buf, &n_bytes, socket)) {
		strncat(response->response, buf, n_bytes - 1);
		if (buf[3] == ' ') {
            sscanf(buf, "%s", response->code);
			break;  
		}
    }

	printf("< %s", response->response);
    return 0;
}


int write_to_socket(int sockfd, int hasBody, char *header, char *body) {

    char message[256];
	strcpy(message, header);

	if (hasBody) {
		strcat(message, " ");
		strcat(message, body);
	}

	strcat(message, "\r\n");

    int bytesSent = write(sockfd, message, strlen(message));
    if (bytesSent != strlen(message)) {		
        fprintf(stderr, "DIDNT SEND COMPLETE BUFFER TO SERVER\n");
        return -1;
    }

	printf("> %s", message);
    return bytesSent;
}

int write_command(int sockfd, char* header, char* body, int reading_file, struct socket_response* response) {

    if (write_to_socket(sockfd, 1, header, body) < 0) {
        printf("Error writing to socket  %s %s\n", header, body);
        return -1;
    }

    while (1) {
        get_response(sockfd, response);
        char codigo=response->code[0];
        switch (codigo) {        
            case '1':
                // Expecting another reply
                printf("ERROR 1\n ");
                if (reading_file) return 2;
          
                else break;
            case '2':
                // Success
                printf("SUCESS \n ");
                return 2;
            case '3':
                // Needs additional information 
                printf("ERROR 3\n ");
                return 3;
            case '4':
                // Error, try again
                printf("ERROR 4\n ");
                if (write_to_socket(sockfd, 1, header, body) < 0) {
                    return -1;
                }
                break;
            case '5':
                // Error in sending command
                printf("ERROR 5\n ");
                printf("Command wasn't accepted... \n");
                close(sockfd);
                exit(-1);
                break;
            default:
                break;
        }
    }

    return -1;
}

int passive_mode(int socketfd) {
    
    struct socket_response response;
    int data_sockfd;
    int ans = write_command(socketfd, "PASV", "", 0, &response);

    if (ans == 2) {
        int ip_1, ip_2, ip_3, ip_4;
        int port_1, port_2;

        if ((sscanf(response.response, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).", &ip_1, &ip_2, &ip_3, &ip_4, &port_1, &port_2)) < 0){
            printf("Error in processing information to calculate port\n\n");
            return -1;
        }

        char ip[20];
        sprintf(ip, "%d.%d.%d.%d", ip_1, ip_2, ip_3, ip_4);
        
        int port = port_1 * 256 + port_2;
        printf("Port number %d\n", port);

        if ((data_sockfd = connection(ip, port)) < 0) {
            printf("Error creating new socket\n\n");
            return -1;
        }
    }

    else {
        printf("Error in passive mode..\n\n");
        return -1;
    }

    return data_sockfd;
}




int download_file(int sockfd, int data_sockfd, struct args * parsed_args) {

    struct socket_response response;
    int ans = write_command(sockfd, "retr", parsed_args->path, 1, &response);
   
    if (ans == 2) {

        FILE *fp = fopen(parsed_args->file_name, "w");
        if (fp == NULL){
            printf("Error opening or creating file\n\n");
            return -1;
        }


        char data[1024];
        int number_of_bytes;
        

        printf("Starting to download \n");

        while(1){

            number_of_bytes = read(data_sockfd, data, 1024);

            if(number_of_bytes==0){
                break;
            }

            if(number_of_bytes  < 0){
                printf("Error reading from socket\n\n");
                return -1;
            }

            fwrite(data, number_of_bytes , 1, fp);
        }
    

        fclose(fp);
        close(data_sockfd);


        get_response(sockfd, &response);
        if(response.code[0] != '2'){
            return -1;
        }

        return 0;
    }


    else {
        printf("Error in server response\n\n");
        return -1;
    }

    return 0;
}

//MEU PROJETO 

int main(int argc, char **argv) {

    if (argc != 2) {
        printf("Usage: rc ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }

    struct args parsed;
    printf("URL PASSADO COMO PARAMETRO: %s\n",argv[1]);

    // CRIAR O STRUCT COM INFORMACOES DO SERVIDOR
    if (parseArgs(argv[1], &parsed) != 0){
        printf("usage: %s ftp://[<user>:<password>@]<host>/<url-path>\n",argv[0]); 
		return -1;
    }

    //CONECTAR-SE AO SERVIDOR
    int sockfd = connection(parsed.ip,PORT);
    if(sockfd==-1){
        printf("ERROR CONECTING TO SERVER");
        return -1;
    }

    printf("\nSOCKFD: %d \n\n",sockfd);

    struct socket_response response;
    if(get_response(sockfd, &response)!=0){
        printf("ERROR RETRIEVING SERVER RESPONSE AFTER CONECTION");
        return -1;
    };

    
    //LOGIN
    struct socket_response response2;
    if (write_command(sockfd, "user", parsed.user, 0, &response2) == -1) {
        printf("Error sending username...\n\n");
        return -1;
    }
    if (write_command(sockfd, "pass", parsed.password, 0, &response2) == -1) {
        printf("Error sending password\n\n");
        return -1;
    }


    //CONECTAR-SE EM MODO PASSIVO AO SERVIDOR COM OUTRA PORTA
    int data_sockfd;
    if ((data_sockfd = passive_mode(sockfd)) == -1) {
        perror("Error in passive mode");
        exit(-1);
    }

    //DOWNLOAD DO FICHEIRO
    if (download_file(sockfd, data_sockfd, &parsed) != 0) {
        perror("Error in passive mode\n\n");
        exit(-1);
    }  
    
    //CLOSE      
    if (close(sockfd)<0) {
        exit(-1);
    }

    return 0;     

}




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#include "utils.h"
#include "dns.h"

#define LEN_BUF 256
#define LEN_MSG 5000

int PORT = NULL;
char* DNS_SERVER = NULL;
char* BLACK_LIST = NULL;

void parse_config(char* filename);
char** parse_blacklist(char* filename);
int handle_connection(char req[], int writesock, int portno, char** blacklist, char* dns_ip);

int main(int argc, char *argv[])
{
	parse_config("settings.conf");
	printf("PORT %d\nDNS=%s\nBLACK LIST=%s\n", PORT, DNS_SERVER, BLACK_LIST);
	
	char** urls = parse_blacklist(BLACK_LIST);
	
	char* error_page = "Error loading\n";
	
	int sockfd, newsockfd, client;
	char buffer[LEN_BUF];
	char msg[LEN_MSG];
	struct sockaddr_in serv_addr, cli_addr;
	int n, flag, i, pid;
	
	sockfd=socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		perror("Error opening socket\n");
		exit(1);
	}
	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(PORT);

	if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("Error on binding\n");
		exit(1);
	}
	
	listen(sockfd, 5);
  
	client = sizeof(cli_addr);
  
	while(1) {
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &client);
		if(newsockfd < 0) {
			perror("Error on accept\n");
			exit(1);
		}
		
		bzero(buffer, LEN_BUF);
		bzero(msg, LEN_MSG);
		
		flag = 0;
		while( (n = read(newsockfd, buffer, LEN_BUF-1)) > 0) {
			strcat(msg, buffer);
			
			for(i = 0; i < strlen(msg); i++) {
				if(msg[i] == '\r' && msg[i+1] == '\n' && msg[i+2] == '\r' && msg[i+3] == '\n') {
						flag = 1;
						break;
				}
			}
			
			if(flag == 1) {
				break;
			}
		}
		
		pid = fork();
		if(pid == 0) {
			if (flag) {
				handle_connection(msg, newsockfd, 80, urls, DNS_SERVER);
			} else {
				n = write(newsockfd, error_page, strlen(error_page));
				if(n < 0) {
					perror("Error reading from socket\n");
					exit(1);
				}
			}
			exit(0);
		} else {
			close(newsockfd);
		}
	}
	
	return 0;
}

void parse_config(char* filename)
{	
	FILE *file = fopen(filename, "r");
	if (!file)
		error("error");
		
	char line[100];
	
	while (fgets(line, sizeof(line), file) != NULL) {
		if (strstr(line, "port"))
			PORT = save_to_int(line);
		
		if (strstr(line, "dns"))
			DNS_SERVER = save_to_string(line);
		
		if (strstr(line, "blacklist"))
			BLACK_LIST = save_to_string(line);
	}
	
	fclose(file);
}

char** parse_blacklist(char* filename)
{
	FILE *file = fopen(filename, "r");
	if (!file)
		error("error");
	int dns_num = 0;
	char **dns = NULL;
	
	char line[100];
	while (fgets(line, sizeof(line), file) != NULL) {
		dns_num += 1;
		dns = realloc(dns, dns_num);
		dns[dns_num-1] = malloc(strlen(line) + 1);
		strcpy(dns[dns_num-1], save_url(line));
	}
	
	fclose(file);
	return dns;
}


int handle_connection(char req[], int writesock, int portno, char** blacklist, char* dns_ip)
{
	char* blocking_page = "Page was blocked\n";
	
	int flag;
	int i, j, n;
	char hoststring[LEN_MSG];
	for(i=0; i<strlen(req); i++) {
		if(req[i] == 'H' && req[i+1] == 'o' && req[i+2] == 's' && req[i+3] == 't') {
			for(j=i+6; req[j] != '\r'; j++) {
				hoststring[j-i-6] = req[j];
			}
			hoststring[j] = '\0';
			break;
		}
	}
	
	for (i = 0; i < strlen(blacklist); i++) {
		if (!strcmp(blacklist[i], hoststring)) {
			flag = 1;
			break;
		} else {
			flag = 0;
		}
	}
	
	if (flag) {
		n = write(writesock, blocking_page, strlen(blocking_page));
		if(n < 0) {
			perror("Error reading from socket\n");
			exit(1);
		}
		  
		return 0;
	} else {
		int sockfd;
		struct sockaddr_in serv_addr;
		struct in_addr *pptr;
		struct hostent *server;
	
		char buffer[LEN_MSG];
		
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if(sockfd < 0) {
			perror("Error opening socket\n");
			exit(1);
		}
		
		RESPONSE *resp = enable_dns(dns_ip, hoststring);
		
		server = gethostbyname(resp[0].res_data);
		if (server == NULL) {
			fprintf(stderr, "No such host\n");
			exit(0);
		}
		
		printf("\nConnected to host\n");
		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		pptr = (struct in_addr  *)server->h_addr;
		bcopy((char *)pptr, (char *)&serv_addr.sin_addr, server->h_length);
		serv_addr.sin_port = htons(portno);
		
		if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
			perror("Error in connecting to server\n");
			exit(1);
		}
		
		bzero(buffer, LEN_BUF);
		n = write(sockfd, req, strlen(req));
		if(n < 0) {
			perror("Error writing to socket\n");
			exit(1);
		}
		
		bzero(buffer, LEN_BUF);
		flag = 1;
		printf("\nreading server response\n");
  
		char msg[LEN_MSG];
		while((n = read(sockfd, buffer, LEN_BUF-1) > 0)) {
			strcat(msg, buffer);
			//printf("%s", buffer);
		}
		
		n = write(writesock, msg, strlen(msg));
		if(n < 0) {
			perror("Error reading from socket\n");
			exit(1);
		}
		
		free(resp);
		
		close(sockfd);
		return n;
	}
}





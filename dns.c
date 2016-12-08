#include "dns.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

static void name_encode(char* name, char* name_encoded);
static unsigned char *processName(unsigned char *bstart, unsigned char *bcur, char *name);

RESPONSE *enable_dns(char* dns, char* sitename) {
	char origin_site_name[NAME_SIZE];
	char buff[BUF_SIZE], *name, buff_rec[BUF_SIZE];
	
	struct RESPONSE_fields *res_fields;
	struct RESPONSE *dns_result;
	struct DNS_query *question;
	int i, bytes, bytes_rec;
	int sockfd;
	struct sockaddr_in dns_setting; 
	unsigned char* pointer;
	
	struct DNS_header *header;
	header = (struct DNS_header *) buff;
	header->id = getpid();
	header->rd = 1;
	header->q_count = htons(1);
	
	name = &buff[sizeof(struct DNS_header)];
	name_encode(sitename, name);
	
	question = (struct DNS_query *) &buff[sizeof(struct DNS_header) + strlen((const char *) name) + 1];
	question->type = htons(1);
	question->class = htons(1);
	
	sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sockfd < 0) {
		perror("error creating socket\n");
	}
	
	bzero(&dns_setting, sizeof(dns_setting));
	dns_setting.sin_family = AF_INET;
	dns_setting.sin_port = htons(53);
	dns_setting.sin_addr.s_addr = inet_addr(dns);
	
	bytes = sendto(sockfd, buff, sizeof(struct DNS_header) + sizeof(struct DNS_query) + strlen((const char *) name) + 1, 0, (struct sockaddr *) &dns_setting, sizeof(dns_setting));
	printf("bytes sent: %d\n", bytes);	
	if (bytes < -1){
		perror("sendto error\n");
		return -1;
	}

	bzero(buff_rec, BUF_SIZE);
	
	bytes_rec = recvfrom(sockfd, &buff_rec, BUF_SIZE, 0, NULL, 0);
	if (bytes_rec < 0){
		perror("receive error\n");
	}
	
	header = (struct DNS_header *) buff_rec;
	
	dns_result = (struct DNS_query *) malloc(sizeof(struct RESPONSE)*ntohs(header->ans_count));
	
	pointer = (unsigned char *) &buff_rec[(sizeof(struct DNS_header) + strlen((const char *) name) + 1 + sizeof(struct DNS_query))];

	struct RESPONSE_fields set_data;
	struct sockaddr_in address;
	
	for (i = 0; i < ntohs(header->ans_count); i++){	
		bzero(origin_site_name, NAME_SIZE);
		pointer = processName ((unsigned char*) buff_rec, pointer, origin_site_name);
		dns_result[i].name = origin_site_name;
		
		res_fields = (struct RESPONSE_fields *) pointer;

		set_data.type = ntohs(res_fields->type);	
		set_data.class = ntohs(res_fields->class);
		set_data.ttl = ntohs(res_fields->ttl);
		set_data.dl = ntohs(res_fields->dl);
		
		dns_result[i].res_data = (unsigned char*)malloc(set_data.dl);
		dns_result[i].rf = &set_data;
		
		pointer = pointer + (sizeof(struct RESPONSE_fields));
		address.sin_addr.s_addr = (*((long*) pointer));

		inet_ntop(AF_INET, &(address.sin_addr), dns_result[i].res_data , INET_ADDRSTRLEN);
	
		//printf("response ip address: %s\n", dns_result[i].res_data);
		
		pointer = pointer + dns_result[i].rf->dl;
	}
	
	close(sockfd);
	return dns_result;
}


//FUNCTION FOR TRANSFORM FROM DNS DOMAIN STYLE TO URL
static unsigned char *processName(unsigned char *bstart, unsigned char *bcur, char *name)
{
	unsigned char *p = bcur;
	char strbuf[80];
	char *strp;
	int compressed = 0;
	name[0] = 0;

	do {
		strp = strbuf;

		if ((*p & 0xc0) == 0xc0) {
			unsigned short offset = (*p & 0x3f);
			offset = (offset << 8) + *(p+1);
			p = bstart + offset;
			if (!compressed)
				bcur += 2;
			compressed = 1;
		} else if (*p > 0) {
			memcpy(strbuf, p+1, *p);
			strp += *p;
			p += *p + 1;

			if (!compressed)
				bcur = p;
			*strp = '.';
			*(strp+1) = 0;
			strcat(name, strbuf);
		}
	} while (*p > 0);

	if (!compressed)
		bcur++;
	return bcur;
}

//FUNCTION FOR TRANSFORM FROM URL TO DNS DOMAIN STYLE
static void name_encode(char* name, char* name_encoded){
    char *ptr;
    int diff, i;
    ptr = strchr(name, '.');
    diff = ptr - name;
    name_encoded[0] = diff;
    
    for (i=0; i<strlen(name); i++){
        name_encoded[i+1] = name[i];
        if (name[i] == '.'){
            ptr = strchr(&name[i+1], '.');
            if (ptr == NULL){
                ptr = strchr(&name[i], '\0');
            }
            diff = ptr - &name[i] - 1;      
            name_encoded[i+1] = diff;
        }
    }
    name_encoded[i+1] = 0;
}





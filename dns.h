#ifndef DNS_H_
#define DNS_H_

#define NAME_SIZE 100
#define BUF_SIZE 65536

struct DNS_header {
    unsigned short id;
   	unsigned char rd :1;
    unsigned char tc:1;
    unsigned char aa :1;
    unsigned char opcode :4;
    unsigned char qr :1;
    unsigned char rcode :4;
    unsigned char cd :1;
    unsigned char ad :1;
    unsigned char z :1;
    unsigned char ra :1;
    unsigned short q_count;
    unsigned short ans_count;
    unsigned short auth_count;
    unsigned short add_count;
};

struct DNS_query {
	unsigned short type;
	unsigned short class;
};

typedef struct {
	char *name;
	struct DNS_query *q; 		
} QUERY;

#pragma pack(push,1)
struct RESPONSE_fields {
	unsigned short type;
	unsigned short class;
	unsigned int ttl;
	unsigned short dl;
};
#pragma pack(pop)

typedef struct RESPONSE{
	unsigned char *name;
	struct RESPONSE_fields *rf;
	unsigned char *res_data;
} RESPONSE;

RESPONSE *enable_dns(char* dns, char* sitename);

#endif

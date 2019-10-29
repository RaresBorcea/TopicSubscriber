#include "parson.h"

#ifndef _HELPERS_
#define _HELPERS_

#define BUFLEN 4096
#define LINELEN 1024
#define URLLEN 1024
#define MAXCOOK 24
#define COOKLEN 512
#define IP_SERVER "185.118.200.35"
#define PORT_SERVER 8081
#define IP_PORT "185.118.200.35:8081"

void error(const char *msg);
void compute_message(char *message, const char *line);
int open_connection(char *host_ip, int portno, int ip_type, int socket_type, int flag);
void close_connection(int sockfd);
void send_to_server(int sockfd, char *message);
char *receive_from_server(int sockfd);
char **extract_cookies(char * response, int * size);
char *get_params(JSON_Object *json_object);
char *get_ip(char* name);

#endif

#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"

// Prints error message
void error(const char *msg) {
    perror(msg);
    exit(0);
}

// Concatenates lines to request
void compute_message(char *message, const char *line) {
    if(line != NULL) {
        strcat(message, line);
    }
    strcat(message, "\r\n");
}

// Opens HTTP connection with host
int open_connection(char *host_ip, int portno, int ip_type, int socket_type, int flag) {
    struct sockaddr_in serv_addr;
    int sockfd = socket(ip_type, socket_type, flag);

    // Default port
    if(portno == -1) {
        portno = 80;
    }

    // Set server sockaddr_in struct
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = ip_type;
    serv_addr.sin_port = htons(portno);
    inet_aton(host_ip, &serv_addr.sin_addr);
    if(sockfd < 0) {
        error("ERROR opening socket");
    }

    // Connect socket
    if(connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR connecting");
    }

    return sockfd;
}

// Closes connection on given socket
void close_connection(int sockfd) {
    close(sockfd);
}

// Sends message to server
void send_to_server(int sockfd, char *message) {
    int bytes, sent = 0;
    int total = strlen(message);

    do
    {
        bytes = write(sockfd, message + sent, total - sent);
        if(bytes < 0) {
            error("ERROR writing message to socket");
        }
        if(bytes == 0) {
            break;
        }
        sent += bytes;

    } while(sent < total);
}

// Receives message from server
char *receive_from_server(int sockfd) {
    char *response = calloc(BUFLEN, sizeof(char));
    int total = BUFLEN;
    int received = 0;

    do
    {
        int bytes = read(sockfd, response + received, total - received);
        if(bytes < 0) {
            error("ERROR reading response from socket");
        }
        if (bytes == 0) {
            break;
        }
        received += bytes;

    } while(received < total);

    if(received == total) {
        error("ERROR storing complete response from socket");
    }

    return response;
}

// Extracts cookies from server response
char **extract_cookies(char *response, int *size) {
    // Allocate memory for cookies array
    char **cookies = malloc(sizeof(char *) * MAXCOOK);
    for(int i = 0; i < MAXCOOK; i++) {
        cookies[i] = malloc(sizeof(char) * COOKLEN);
    }

    char *res;
    int poz = 0;

    // Store each cookie in array
    res = strstr(response, "Cookie");
    if(res != NULL) {
        res = strchr(res, ' ');
    }
    while(res != NULL) {
        res++;
        char *end = strchr(res, ';');
        cookies[poz] = strncpy(cookies[poz], res, end - res + 1);
        cookies[poz][end - res + 1] = '\0';
        res = strstr(++res, "Cookie");
        if(res != NULL) {
            res = strchr(res, ' ');
        }
        poz++;
    }

    // Erase last ';' character
    if(poz != 0) {
        cookies[poz - 1][strlen(cookies[poz - 1]) - 1] = '\0';
    }

    // return array and its size
    *size = poz;
    return cookies;
}

// Extracts url parameters from JSON_Object
char *get_params(JSON_Object *json_object) {
    char *params = malloc(sizeof(char) * URLLEN);
    JSON_Value *field_value;

    // Concatenate each field and value as url parameter
    for(int i = 0; i < json_object_get_count(json_object); i++) {
        const char *field = json_object_get_name(json_object, i);
        field_value = json_object_get_value_at(json_object, i);
        const char *field_string = json_value_get_string(field_value);

        strcat(params, field);
        strcat(params, "=");
        strcat(params, field_string);

        if(i != json_object_get_count(json_object) - 1) {
            strcat(params, "&");
        }
    }

    // return url parameters as single string
    return params;
}

// Receives a name and returns IP address
char *get_ip(char* name)
{
    struct addrinfo hints, *result, *p;
    char buffer[100];

    // Set hints
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;

    // Get address
    getaddrinfo(name, NULL, &hints, &result);

    // Iterate through addresses and choose one (first)
    for(p = result; p != NULL; p = p->ai_next) {
        if(p->ai_family == AF_INET) {
            struct sockaddr_in *addr = (struct sockaddr_in*)p->ai_addr;
            inet_ntop(AF_INET, &(addr->sin_addr), buffer, sizeof(buffer));
            break;
        }
    }

    // Free allocated data
    freeaddrinfo(result);

    char *res = malloc(sizeof(char) * 100);
    strcpy(res, buffer);
    return res;
}
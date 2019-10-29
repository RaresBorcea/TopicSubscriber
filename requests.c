#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

// Constructs HTTP request for specified method
char *compute_request(char *host, char *url, char *url_params, char *form_data, 
        int content_type, char *jwt, char **cookies, int cookie_size, char *method) {
    
    if(strcmp(method, "GET") == 0) {
        return compute_get_request(host, url, url_params, jwt, cookies, cookie_size);
    } else {
        return compute_post_request(host, url, form_data, content_type, jwt, cookies, cookie_size);
    }
}

// Constructs GET type HTTP request
char *compute_get_request(char *host, char *url, char *url_params, char *jwt, char **cookies, int cookie_size) {
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    
    // Add method, path, url parameters (if present) and protocol type
    if(url_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, url_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }
    compute_message(message, line);

    // Add host
    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Add cookies, if present
    if(cookie_size != 0) {
        memset(line, 0, LINELEN);
        sprintf(line, "%s", "Cookie:");
        for(int i = 0; i < cookie_size; i++) {
            strcat(line, " ");
            strcat(line, cookies[i]);
        }
        compute_message(message, line);
    }
    

    // Add jwt token, if present
    if(jwt != NULL) {
        memset(line, 0, LINELEN);
        sprintf(line, "Authorization: Bearer %s", jwt);
        compute_message(message, line);
    }

    // Add empty line
    compute_message(message, NULL);

    return message;
}

// Constructs POST type HTTP request
char *compute_post_request(char *host, char *url, char *form_data, int content_type, char *jwt, char **cookies, int cookie_size) {

    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Add method, path and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);

    // Add host
    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Add content type
    memset(line, 0, LINELEN);
    if(content_type == 0) { // Form
        sprintf(line, "%s", "Content-Type: application/x-www-form-urlencoded");
    } else { // JSON
        sprintf(line, "%s", "Content-Type: application/json");
    }
    compute_message(message, line);

    // Add cookies, if present
    if(cookie_size != 0) {
        memset(line, 0, LINELEN);
        sprintf(line, "%s", "Cookie:");
        for(int i = 0; i < cookie_size; i++) {
            strcat(line, " ");
            strcat(line, cookies[i]);
        }
        compute_message(message, line);
    }

    // Add jwt token, if present
    if(jwt != NULL) {
        memset(line, 0, LINELEN);
        sprintf(line, "Authorization: Bearer %s", jwt);
        compute_message(message, line);
    }

    // Add content length
    memset(line, 0, LINELEN);
    sprintf(line, "Content-Length: %zu", strlen(form_data));
    compute_message(message, line);

    // Add empty line
    compute_message(message, NULL);

    // Add request body
    compute_message(message, form_data);
    
    return message;
}
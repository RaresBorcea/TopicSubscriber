#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"     /* library used to parse JSONs */

int main(int argc, char *argv[]) {
    int size;
    char *message, *response, *params;
    char *json_start;   // JSON starting point in response
    int sockfd;
    char **cookies;     // Cookies array
    char url[URLLEN];
    JSON_Value *response_value, *field_value;
    JSON_Object *json_object;

    // Stage 1
    memset(url, 0, URLLEN);
    sprintf(url, "%s", "/task1/start");
    /* Pattern: open connection, compute request, send request, 
       receive response, close connection */
    sockfd = open_connection(IP_SERVER, PORT_SERVER, AF_INET, SOCK_STREAM, 0);
    message = compute_get_request(IP_PORT, url, NULL, NULL, NULL, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close_connection(sockfd);

    // Stage 2
    cookies = extract_cookies(response, &size);

    // Find JSON starting point in response
    json_start = strrchr(response, '\n');
    response_value = json_parse_string(json_start + 1);
    if(json_value_get_type(response_value) != JSONObject) {
        error("ERROR parsing JSON");
    }

    // Extract JSON object from value and necessary fields/objects
    json_object = json_value_get_object(response_value);
    const char *snd_url = json_object_get_string(json_object, "url");
    const char *snd_met = json_object_get_string(json_object, "method");
    json_object = json_object_get_object(json_object, "data");
    // Extract all url parameters from 'data' object
    params = get_params(json_object);
    
    sockfd = open_connection(IP_SERVER, PORT_SERVER, AF_INET, SOCK_STREAM, 0);
    message = compute_request(IP_PORT, (char *)snd_url, NULL, params, 0, NULL, cookies, size, (char *)snd_met);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close_connection(sockfd);

    // Stage 3
    cookies = extract_cookies(response, &size);

    json_start = strrchr(response, '\n');
    response_value = json_parse_string(json_start + 1);
    if(json_value_get_type(response_value) != JSONObject) {
        error("ERROR parsing JSON");
    }

    // Extract JSON object from value and necessary fields/objects
    json_object = json_value_get_object(response_value);
    const char *trd_url = json_object_get_string(json_object, "url");
    const char *trd_met = json_object_get_string(json_object, "method");
    // Extract token from pos 0 in 'data'
    json_object = json_object_get_object(json_object, "data");
    field_value = json_object_get_value_at(json_object, 0);
    const char *jwt = json_value_get_string(field_value);
    // Extract 'queryParams' object from pos 1 in 'data'
    field_value = json_object_get_value_at(json_object, 1);
    json_object = json_value_get_object(field_value);
    // Extract all url parameters from 'queryParams' object
    params = get_params(json_object);

    sockfd = open_connection(IP_SERVER, PORT_SERVER, AF_INET, SOCK_STREAM, 0);
    // Create url parameters with required answers
    char answer_params[URLLEN];
    memset(answer_params, 0, sizeof(URLLEN));
    sprintf(answer_params, "raspuns1=omul&raspuns2=numele&%s", params);
    message = compute_request(IP_PORT, (char *)trd_url, answer_params, NULL, 0, (char *)jwt, cookies, size, (char *)trd_met);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close_connection(sockfd);

    // Stage 4
    cookies = extract_cookies(response, &size);

    json_start = strrchr(response, '\n');
    response_value = json_parse_string(json_start + 1);
    if(json_value_get_type(response_value) != JSONObject) {
        error("ERROR parsing JSON");
    }

    json_object = json_value_get_object(response_value);
    const char *four_url = json_object_get_string(json_object, "url");
    const char *four_met = json_object_get_string(json_object, "method");

    sockfd = open_connection(IP_SERVER, PORT_SERVER, AF_INET, SOCK_STREAM, 0);
    message = compute_request(IP_PORT, (char *)four_url, NULL, NULL, 0, (char *)jwt, cookies, size, (char *)four_met);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close_connection(sockfd);

    // Stage 5
    cookies = extract_cookies(response, &size);

    json_start = strrchr(response, '\n');
    response_value = json_parse_string(json_start + 1);
    if(json_value_get_type(response_value) != JSONObject) {
        error("ERROR parsing JSON");
    }

    json_object = json_value_get_object(response_value);
    const char *fiv_url = json_object_get_string(json_object, "url");
    const char *fiv_met = json_object_get_string(json_object, "method");
    // Extract 'data_url' and get_weather_met from pos 0 and 2 in 'data'
    json_object = json_object_get_object(json_object, "data");
    field_value = json_object_get_value_at(json_object, 0);
    const char *data_url = json_value_get_string(field_value);
    field_value = json_object_get_value_at(json_object, 2);
    const char *data_met = json_value_get_string(field_value);
    // Extract 'queryParams' object from pos 1 in 'data'
    field_value = json_object_get_value_at(json_object, 1);
    json_object = json_value_get_object(field_value);
    // Extract all url parameters from 'queryParams' object
    params = get_params(json_object);

    // Divide url from url parameters in received response
    char *slash_poz = strchr((char *)data_url, '/');
    char weather_param[URLLEN];
    memset(weather_param, 0, URLLEN);
    strcpy(weather_param, slash_poz);
    strcat(weather_param, "?");
    strcat(weather_param, params);
    char weather_url[URLLEN];
    memset(weather_url, 0, URLLEN);
    strncpy(weather_url, data_url, slash_poz - data_url);

    // Get IP address from name
    char *weather_addr = get_ip(weather_url);

    // Connect to OpenWeatherMap server
    sockfd = open_connection(weather_addr, -1, AF_INET, SOCK_STREAM, 0);
    message = compute_request(weather_url, weather_param, NULL, NULL, 0, NULL, NULL, 0, (char *)data_met);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close_connection(sockfd);

    // Extract JSON from response and send to original server
    json_start = strrchr(response, '\n');
    sockfd = open_connection(IP_SERVER, PORT_SERVER, AF_INET, SOCK_STREAM, 0);
    message = compute_request(IP_PORT, (char *)fiv_url, NULL, json_start + 1, 1, (char *)jwt, cookies, size, (char *)fiv_met);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    // Print final response, with received code and body
    fprintf(stderr, "%s\n", response);
    close_connection(sockfd);

    return 0;
}
#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <cstdio>
#include <cstdlib>
#include <cinttypes>
#include <list>
#include <string>
#include <utility>
#include <unordered_map>
#include <set>
#include <vector>
#include <math.h>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>

using namespace std;

/*
 * Macro for error checking
 * Example:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			fprintf(stderr, "%s\n", call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define BUFLEN		1501	// maximum buffer size for transferred data
#define MAX_CLIENTS	128		// somaxconn - no. of possible clients
#define TOPIC_LEN	50		// length of message topic
#define IP_LEN		16		// length of IPv4 address as text format

// Data type constants
#define INT 		0
#define SHORT_REAL 	1
#define FLOAT	 	2
#define STRING 		3


// Struct type for udp-server message
typedef struct {
  char topic[TOPIC_LEN];
  uint8_t type;
  char payload[BUFLEN];
} __attribute__((packed)) udp_msg;

// Struct type for server-client message
typedef struct {
  char ip[IP_LEN];
  int port;
  udp_msg msg;
  int ttl; //time-to-live
} __attribute__((packed)) server_msg;

// Struct type for clients hashmap
typedef struct {
  int socket;
  list<pair<string, unsigned long long>> messages;
} client_val;

// Struct type for topics hashmap
typedef struct {
  set<pair<string, bool>> clients;
  unordered_map<unsigned long long, server_msg> messages; 
} topic_val;

// Based on @Todd Gamblin function of strtok for C++ strings
void split(const string& str, const string& delim, vector<string>& parts) {
  size_t start, end = 0;
  while(end < str.size()) {
    start = end;
    while(start < str.size() && (delim.find(str[start]) != string::npos)) {
      start++;  // skip initial whitespace
    }
    end = start;
    while(end < str.size() && (delim.find(str[end]) == string::npos)) {
      end++; // skip to end of word
    }
    if(end-start != 0) {  // ignore zero-length strings.
      parts.push_back(string(str, start, end-start));
    }
  }
}

#endif

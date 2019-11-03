#include "helpers.h"

// Usage parameters warning
void usage(char *file)
{
	fprintf(stderr, "Usage: %s client_ID server_address server_port.\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];

	if(argc != 4) {
		usage(argv[0]);
	}

	// get TCP connection file descriptor
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "Client_ERROR: Could not allocate a TCP socket. Closing.");

	// deactivate Nagle's algorithm for this socket
	int flag = 1;
	ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
	DIE(ret < 0, "Client_ERROR: Could not deactivate Nagle algorithm for current TCP client. Closing.");

	fd_set read_fds; // reading set for select()
	fd_set tmp_fds; // temporary set for select()

	// zeroing read and temporary file descriptors sets
	FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    // add all sockets to read set (including keyboard/STDIN)
	FD_SET(sockfd, &read_fds);
	FD_SET(STDIN_FILENO, &read_fds);

	// extract maximum file descriptor
	int fdmax = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;

	// set server sockaddr_in struct
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "Client_ERROR: Could not convert given IP address. Closing.");

	// request connection to server
	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "Client_ERROR: Could not connect to server. Closing.");

	// send client_ID
	memset(buffer, 0, BUFLEN);
	sprintf(buffer, "%s", argv[1]);
	n = send(sockfd, buffer, strlen(buffer), 0);
	DIE(n < 0, "Client_ERROR: Could not send client_ID to server. Closing.");

	while (1) {
		// copy set for select
		tmp_fds = read_fds;

		// I/O multiplexing
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "Client_ERROR: Could not multiplex connections. Closing.");

		for(int j = 0; j <= fdmax; j++) {
			if(FD_ISSET(j, &tmp_fds)) {
				if(j == STDIN_FILENO) {

					// reading from STDIN
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);

					// 'exit' command received
					ret = strcmp(buffer, "exit\n");
					if(ret == 0) {
						close(sockfd);
						return 0;
					}

					// parse command and check validity
					bool isTopic = false;
					bool isSF = false;
					char copy[BUFLEN];
					strncpy(copy, buffer, BUFLEN - 1);
					char *command;
					char *token = strtok(copy, " ");
					if(token != NULL) {
					    command = token;
					    token = strtok(NULL, " ");
					}
					char *topic;
					if(token != NULL) {
						isTopic = true;
					    topic = token;
					    token = strtok(NULL, " ");
					}
					char *sf;
					if(token != NULL) {
						isSF = true;
					    sf = token;
					    token = strtok(NULL, " ");
					}

					udp_msg p;
					memset(&p, 0, sizeof(p));
					server_msg m;
			    	memset(&m, 0, sizeof(m));
			    	sprintf(p.payload, "%s", buffer);
			    	m.msg = p;

					if(strcmp(command, "subscribe") == 0 || strcmp(command, "subscribe\n") == 0) {

				    	// received 'subscribe' command
				    	if(isSF == false && isTopic == false) { // both parameters missing
				    		fprintf(stderr, "Provide topic and SF policy.\n");
				    	} else if(isSF == false) { // SF parameter missing
				    		fprintf(stderr, "Provide SF policy.\n");
				    	} else if((strcmp(sf, "1\n") == 0 || strcmp(sf, "0\n") == 0) && (isSF == true && isTopic == true)) {
				    		// valid command - send to server
				    		n = send(sockfd, &m, sizeof(m), 0);
							DIE(n < 0, "Client_ERROR: Could not send command to server. Closing.");
							// display deedback
							fprintf(stderr, "subscribed %s\n", topic);

				    	} else { // wrong format command
				    		fprintf(stderr, "Command not available.\n");
				    	}

					} else if(strcmp(command, "unsubscribe") == 0 || strcmp(command, "unsubscribe\n") == 0) {

						// received 'unsubscribe' command
						if(isTopic == false) { // missing topic parameter
							fprintf(stderr, "Provide topic.\n");
						} else {
							// valid command - send to server
							n = send(sockfd, &m, sizeof(m), 0);
							DIE(n < 0, "Client_ERROR: Could not send command to server. Closing.");
							// display deedback
							fprintf(stderr, "unsubscribed %s", topic);
						}
					} else { // wrong format command
						fprintf(stderr, "Command not available.\n");
					}

				} else if(j == sockfd) {
					
					// received data from server
					server_msg m;
				    memset(&m, 0, sizeof(m));
            		n = recv(sockfd, &m, sizeof(m), 0);
            		DIE(n == 0, "Client_ERROR: Could not receive data from server. Closing.");

            		// received 'exit' command
            		ret = strcmp(m.msg.payload, "exit\n");
            		if(ret == 0) {
            			close(sockfd);
            			return 0;

            		} else { // parse command by payload type
              			
              			if(m.msg.type == INT) {
              				uint8_t sign;
              				int32_t value;
              				memcpy(&sign, m.msg.payload, sizeof(uint8_t));
              				memcpy(&value, m.msg.payload + sizeof(uint8_t), sizeof(int32_t));
              				
              				// convert to host byte-order
              				value = ntohl(value);

              				if(sign == 1) {
              					value *= -1;
              				}

              				// make sure topic respects ASCIIZ reqs
              				char topic[TOPIC_LEN];
              				strncpy(topic, m.msg.topic, TOPIC_LEN - 1);
              				topic[TOPIC_LEN - 1] = '\0';
              				printf("%s:%d - %s - %s - %d\n", m.ip, m.port, topic, "INT", value);

              			} else if(m.msg.type == SHORT_REAL) {
              				uint16_t value;
              				float result;
              				memcpy(&value, m.msg.payload, sizeof(uint16_t));

              				// convert to host byte-order
              				value = ntohs(value);

              				// make sure topic respects ASCIIZ reqs
              				result = (float)value / 100;
              				char topic[TOPIC_LEN];
              				strncpy(topic, m.msg.topic, TOPIC_LEN - 1);
              				topic[TOPIC_LEN - 1] = '\0';
              				printf("%s:%d - %s - %s - %.2f\n", m.ip, m.port, topic, "SHORT_REAL", result);

              			} else if(m.msg.type == FLOAT) {
              				uint8_t sign;
              				int32_t value;
              				uint8_t power;
              				memcpy(&sign, m.msg.payload, sizeof(uint8_t));
              				memcpy(&value, m.msg.payload + sizeof(uint8_t), sizeof(int32_t));
              				memcpy(&power, m.msg.payload + sizeof(uint8_t) + sizeof(int32_t), sizeof(uint8_t));
              				
              				// convert to host byte-order
              				value = ntohl(value);

              				double divisor = pow(10, power);
              				double result = (double)value / divisor;
              				if(sign == 1) {
              					result *= -1;
              				}

              				// make sure topic respects ASCIIZ reqs
              				char topic[TOPIC_LEN];
              				strncpy(topic, m.msg.topic, TOPIC_LEN - 1);
              				topic[TOPIC_LEN - 1] = '\0';
              				printf("%s:%d - %s - %s - %.*f\n", m.ip, m.port, topic, "FLOAT", power, result);

              			} else if(m.msg.type == STRING) {
              				// make sure message respects ASCIIZ reqs
              				char message[BUFLEN];
              				strncpy(message, m.msg.payload, BUFLEN - 1);
              				message[BUFLEN - 1] = '\0';

              				// make sure topic respects ASCIIZ reqs
              				char topic[TOPIC_LEN];
              				strncpy(topic, m.msg.topic, TOPIC_LEN - 1);
              				topic[TOPIC_LEN - 1] = '\0';
              				printf("%s:%d - %s - %s - %s\n", m.ip, m.port, topic, "STRING", message);

              			} 
            		}
				}
			}
		}
	}

	close(sockfd);
	return 0;
}
#include "helpers.h"

// Usage parameters warning
void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port.\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno, sockudp;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret, flag;
	socklen_t clilen;
	udp_msg p;
	unsigned long long msg_index = 0;

	// hashmaps for data storage
	unordered_map<string, client_val> clients;
	unordered_map<string, topic_val> topics;

	fd_set read_fds; // reading set for select()
	fd_set tmp_fds;	// temporary set for select()
	int fdmax; // maximum file descriptor

	if(argc != 2) {
		usage(argv[0]);
	}

	// zeroing read and temporary file descriptors sets
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// get TCP listen file descriptor
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "ERROR: Could not allocate a TCP socket. Closing.");
	// get UDP file descriptor
	sockudp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sockudp < 0, "ERROR: Could not allocate a UDP socket. Closing.");

	// forcefully reuse IP:PORT for boths sockets
	flag = 1;
	ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                  &flag, sizeof(int));
	DIE(ret < 0, "ERROR: Could not reuse address and port. Closing.");
	ret = setsockopt(sockudp, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                  &flag, sizeof(int));
	DIE(ret < 0, "ERROR: Could not reuse address and port. Closing.");

	// get server port number
	portno = atoi(argv[1]);
	DIE(portno == 0, "ERROR: Could not convert given PORT to integer. Closing.");

	// set server sockaddr_in struct
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	// bind properties for both sockets
	ret = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "ERROR: Could not use given address and port for TCP. Closing.");
	ret = bind(sockudp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "ERROR: Could not use given address and port for UDP. Closing.");

	// listen for TCP clients connection requests
	ret = listen(sockfd, MAX_CLIENTS);
	DIE(ret < 0, "ERROR: Could not accept TCP clients. Closing.");

	// add all sockets to read set (including keyboard/STDIN)
	FD_SET(sockfd, &read_fds);
	FD_SET(sockudp, &read_fds);
	FD_SET(STDIN_FILENO, &read_fds);

	// extract maximum file descriptor
	fdmax = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;
	fdmax = sockudp > fdmax ? sockudp : fdmax;

	while(1) {
		// copy set for select
		tmp_fds = read_fds; 
		
		// I/O multiplexing
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "ERROR: Could not multiplex connections. Closing.");

		for(i = 0; i <= fdmax; i++) {
			if(FD_ISSET(i, &tmp_fds)) {
				if(i == STDIN_FILENO) {

					// reading from STDIN
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);

					if(strcmp(buffer, "exit\n") != 0) {
						fprintf(stderr, "Only 'exit' command available.\n");
					} else {
						memset(&p, 0, sizeof(p));
						sprintf(p.payload, "%s", buffer);
						server_msg m;
					        memset(&m, 0, sizeof(m));
					        m.msg = p;

					        // close all connected TCP clients - send 'exit'
						for(int j = 0; j <= fdmax; j++) {
							if(j > 4 && FD_ISSET(j, &read_fds)) {
								if(j != i) {
									n = send(j, &m, sizeof(m), 0);
									DIE(n < 0, "ERROR: Could not send 'exit' command to all clients.");
								}
							}
						}

						//close sockets and server
						close(sockfd);
						close(sockudp);
						return 0;
					}

				} else if(i == sockfd) {

					// accept TCP client connection
					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "ERROR: Could not accept TCP connection. Closing.");

					// deactivate Nagle's algorithm for this socket
					flag = 1;
					ret = setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
					DIE(ret < 0, "ERROR: Could not deactivate Nagle algorithm for current TCP client. Closing.");

					// add socket to read set
					FD_SET(newsockfd, &read_fds);

					// check for new maximum fd
					if(newsockfd > fdmax) { 
						fdmax = newsockfd;
					}

					// get client_ID
					memset(buffer, 0, BUFLEN);
					n = recv(newsockfd, buffer, sizeof(buffer), 0);
					DIE(n < 0, "ERROR: Could not receive ID for current TCP client. Closing.");

					fprintf(stderr, "New client %s connected from %s:%d.\n",
							buffer, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

					string id(buffer);
					auto old_client = clients.find(id);
					// check for new or old TCP client
				        if(old_client != clients.end()) { // old client

				    	// if another client with same ID is connected, kill it
				    	int old_socket = old_client->second.socket;
				    	if(FD_ISSET(old_socket, &read_fds)) {
				    		memset(&p, 0, sizeof(p));
						sprintf(p.payload, "%s", "exit\n");
						server_msg m;
						memset(&m, 0, sizeof(m));
						m.msg = p;
						// send 'exit' command to old TCP client with same ID
				    		n = send(old_socket, &m, sizeof(m), 0);
						DIE(n < 0, "ERROR: Could not send 'exit' command to old_socket of current TCP client. Closing.");
				    		// remove socket from read set
						FD_CLR(old_socket, &read_fds);
				    	}
				    	
				    	// keep new socket for client_ID as the current one
				    	old_client->second.socket = newsockfd;

				    	// send all stored messages
				        list<pair<string, unsigned long long>> c_messages = old_client->second.messages;
				        for(list<pair<string, unsigned long long>>::iterator it = c_messages.begin(); it != c_messages.end(); ++it) {

				        	// extract all required messages from hashmaps, based on topic and message key
				        	string topic = (*it).first;
				        	unsigned long long message = (*it).second;
				        	auto old_topic = topics.find(topic);
				        	auto old_message = old_topic->second.messages.find(message);
				        	server_msg m = old_message->second;
				        	n = send(old_client->second.socket, &m, sizeof(m), 0);
				        	DIE(n < 0, "ERROR: Could not send all messages to reconnected TCP clients. Closing.");

				        	// modify message time-to-live
				        	m.ttl--;
				        	if(m.ttl == 0) {
				        		// if zero, delete message from server
				        		old_topic->second.messages.erase(old_message);
				        	} else {
				        		old_message->second = m;
				        	}
				        }

				        // clear client's unsent messages list
				        old_client->second.messages.clear();

				    } else { // new client - add to clients hashmap
					client_val c_val;
					c_val.socket = newsockfd;
					clients.insert({id, c_val});
				    }

				} else if(i == sockudp) {

					// new message from UDP client
					memset(&p, 0, sizeof(p));
					ret = recvfrom(sockudp, &p, sizeof(p), 0, (struct sockaddr *) &cli_addr, &clilen);
					DIE(ret < 0, "ERROR: Could not receive message from UDP client. Closing.");

					server_msg m;
				        memset(&m, 0, sizeof(m));
				        m.msg = p;
				        sprintf(m.ip, "%s", inet_ntoa(cli_addr.sin_addr));
				        m.port = ntohs(cli_addr.sin_port);

					string topic(p.topic);
					auto old_topic = topics.find(topic);

					// check for new or old topic
				        if(old_topic != topics.end()) { // old topic
				    	int ttl = 0; // time-to-live for message
				        set<pair<string, bool>> cl = old_topic->second.clients;
				        for(set<pair<string, bool>>::iterator it = cl.begin(); it != cl.end(); ++it) {
						    string id = (*it).first;
						    auto old_client = clients.find(id);

						    // send message to all connected subscribed clients
						    if(FD_ISSET(old_client->second.socket, &read_fds)) {
						    	n = send(old_client->second.socket, &m, sizeof(m), 0);
						    	DIE(n < 0, "ERROR: Could not send message to connected TCP clients. Closing.");
						    } else if((*it).second) { // or store for SF=1 clients
						    	ttl++; // keep count of necessary instances
						    	// indicate message location in hash database
						    	old_client->second.messages.push_back({topic, msg_index});
						    }
						}
						if(ttl != 0) { // store message in topics hashmap
							m.ttl = ttl;
							old_topic->second.messages.insert({msg_index++, m});    	
						}
				    } else { // new topic
				        topic_val t_val;
					topics.insert({topic, t_val});
				    }

				} else {
					// TCP client data received
					server_msg m;
					memset(&m, 0, sizeof(m));
					n = recv(i, &m, sizeof(m), 0);
					DIE(n < 0, "ERROR: Could not receive command from TCP client. Closing.");

					// get client_ID from clients hashmap
					string id;
					unordered_map<string, client_val>::iterator it;
					for(it = clients.begin(); it != clients.end(); ++it) {
						id = (*it).first;
				       		client_val c_val = (*it).second;
				       		if(c_val.socket == i) {
				       			break;
				       		}
					}

					if(n == 0) {
						// closed connection
						fprintf(stderr, "Client %s disconnected.\n", id.c_str());
						close(i);
						// set socket to unavailable fd to keep client messages
						(*it).second.socket = -1;

						// remove socket from read set
						FD_CLR(i, &read_fds);

					} else if(n > 0) {
						// command received
						memset(buffer, 0, BUFLEN);
						sprintf(buffer, "%s", m.msg.payload);

						// parse command
					    	vector<string> command;
					    	string delim = " ";
					    	string buf(buffer);
					    	split(buf, delim, command);
					    	string comm = command[0];
					    	string top = command[1];

					    	if(strcmp(comm.c_str(), "subscribe") == 0) { // new subscription
					    		string sf = command[2]; // get SF policy
					    		string topic(top);
					    		auto old_topic = topics.find(topic);

							// check for new or old topic
							if(old_topic != topics.end()) { // topic exists

								if(strcmp(sf.c_str(), "1\n") == 0) {
									// check for existing subscription and modify
									old_topic->second.clients.erase({id, false}); 
									old_topic->second.clients.insert({id, true});
								} else {
									old_topic->second.clients.erase({id, true});
									old_topic->second.clients.insert({id, false});
								}

					    	} else { // create new topic

					    		topic_val t_val;
					    		if(strcmp(sf.c_str(), "1\n") == 0) {
					    			t_val.clients.insert({id, true});
					    		} else {
					    			t_val.clients.insert({id, false});
					    		}
							topics.insert({topic, t_val});
					    	}

					    } else if(strcmp(comm.c_str(), "unsubscribe") == 0) { // unsubscribe client

					    	// make sure topic respects ASCIIZ reqs
					    	char edited_topic[TOPIC_LEN];
					    	strncpy(edited_topic, top.c_str(), TOPIC_LEN - 1);
					    	edited_topic[TOPIC_LEN - 1] = '\0';
					    	edited_topic[strlen(edited_topic) - 1] = '\0';

					    	string topic(edited_topic);
					    	auto old_topic = topics.find(topic);

					    	// check for topic existence
					    	if(old_topic != topics.end()) { 
					    		for(set<pair<string, bool>>::iterator it = old_topic->second.clients.begin(); 
					    		 	it != old_topic->second.clients.end(); ++it) {
					    		 	string c_id = (*it).first;

					    		 	// get client and erase from subscription list
					    		 	if(c_id.compare(id) == 0) {
					    		 		old_topic->second.clients.erase(it);
					    		 		break;
					    		 	}
					    		}
					    	}
					    }
					}
				}
			}
		}
	}

	close(sockfd);
	close(sockudp);
	return 0;
}

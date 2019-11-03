# Protocoale de comunicatii:
# Tema 2
# Makefile

CFLAGS = -Wall -g

# Portul pe care asculta serverul
PORT = 8080

# Adresa IP a serverului 
IP_SERVER = 127.0.0.1

IGNORE = run_subscriber

all: server subscriber

# Compileaza server.cpp
server: server.cpp

# Compileaza subscriber.cpp
subscriber: subscriber.cpp

.PHONY: clean run_server run_subscriber

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza clientul, primeste ca parametru client_id-ul
run_subscriber:
	./subscriber $(filter-out $(IGNORE),$(MAKECMDGOALS)) ${IP_SERVER} ${PORT}
%:
    @:

clean:
	rm -f server subscriber
#include "circd.h"

#define PORT 6667
#define BUFFSIZE 1024

int main() {
	Server server;
	memset(&server, 0, sizeof(server));

	create_server(PORT, &server);

	printf("Server started, listening on :%i\n", PORT);

	while (1) {
		accept_clients(&server);
	}

	return 0;
}


void create_server(int port, Server* server) {
	int serverFD = socket(PF_INET, SOCK_STREAM, 0);
	if (serverFD == -1) {
		fprintf(stderr, "Failed to create server socket\n");
		exit(1);
	}

	struct sockaddr_in serverParam;
	serverParam.sin_family = AF_INET;
	serverParam.sin_port = htons(port);
	serverParam.sin_addr.s_addr = htonl(INADDR_ANY);

	setsockopt(serverFD, 
		SOL_SOCKET, 
		SO_REUSEADDR, 
		&(int){ 1 }, 
		sizeof(int));

	int bound = bind(serverFD, 
		(struct sockaddr*) &serverParam, 
		sizeof(serverParam));
	if (bound == -1) {
		fprintf(stderr, "Failed to bind to port\n");
		exit(1);
	}

	int listening = listen(serverFD, 10);
	if (listening == -1) {
		fprintf(stderr, "Failed to listen\n");
		exit(1);
	}

	server->socket = serverFD;
}


void accept_clients(Server* server) {
	int clientFD = accept(server->socket, NULL, NULL);

	Client* client = calloc(1, sizeof(Client));
	client->socket = clientFD;

	ll_add_node(&server->clients, client); // TODO: mutex this

	ClientThreadArgs* args = calloc(1, sizeof(ClientThreadArgs));
	args->server = server;
	args->client = client;

	pthread_t ptid;
	int created = pthread_create(&ptid, NULL, handle_client, (void *) args);
	if (created != 0) {
		fprintf(stderr, "Failed to create thread to handle client\n");
	}
}


void* handle_client(void* vargs) {
	printf("Client connected\n");
	ClientThreadArgs* args = (ClientThreadArgs *) vargs;
	Client* client = args->client;
	Server* server = args->server;

	while (1) {
		char m[BUFFSIZE] = {0};
		ssize_t r = recv(client->socket, &m, sizeof(m), 0);
		if (r == 0) {
			// TODO: PERFORM CLIENT FREE
			printf("Client disconnected\n");
			pthread_exit(0);
		}


		while (1) {
			if (m[strlen(m) - 1] == '\n' || m[strlen(m) - 1] == '\r') {
				m[strlen(m) - 1] = '\0';
			} else {
				break;
			}
		}

		printf("> %s\n", m);

		if (client->nick && client->username && client->realname) {
			handle_message(m, client, server);
		} else {
			handle_handshake(m, client);
		}
	}
}


void handle_handshake(char* msg, Client* client) {
	if (strncmp(msg, "NICK ", 5) == 0) {
		msg += 5;
		int nicklen = strlen(msg);
		char* nick = calloc(nicklen + 1, sizeof(char));
		strncpy(nick, msg, nicklen);
		client->nick = nick;

	} else if (strncmp(msg, "USER ", 5) == 0) {

		msg += 5;
		int userLength = 0;
		while(msg[userLength] != ' ') { userLength++; }
		char* username = calloc(userLength + 1, sizeof(char));
		strncpy(username, msg, userLength);
		client->username = username;
		msg += userLength + 1;

		unsigned int modes = msg[0] - '0';
		client->modes = modes & 1 << 3;
		msg += 5;

		char* realname = calloc(strlen(msg) + 1, sizeof(char));
		strncpy(realname, msg, strlen(msg));
		client->realname = realname;
	}

	if(client->nick && client->username && client->realname) {
		char m[BUFFSIZE] = {0};
		char t[BUFFSIZE] = {0};
		client_host(t, client);
		snprintf(m, BUFFSIZE, ":cdirc 001 %s :Welcome to the Internet Relay Network %s\n", client->nick, t);
		send(client->socket, m, sizeof(m), 0);
	}
}


void handle_message(char* msg, Client* client, Server* server) {
	char m[BUFFSIZE] = {0};
	if(strncmp(msg, "PING ", 5) == 0) {
		msg += 5;
		snprintf(m, BUFFSIZE, ":cdirc PONG %s\n", msg);

	} else if(strncmp(msg, "JOIN ", 5) == 0) {
		msg += 5;
		
		
	} else if(strncmp(msg, "PART ", 5) == 0) {
		msg += 5;
		
		
	} else if(strncmp(msg, "QUIT ", 5) == 0) {
		msg += 5;
		
		
	} else if(strncmp(msg, "PRIVMSG ", 8) == 0) {
		msg += 8;
		
		
	}
}


void client_host(char* dest, Client* client) {
	snprintf(dest, BUFFSIZE, "%s!%s@secret", client->nick, client->username);
}


// todo: Mutexes
// todo: Don't let nick be picked if nick is in use already
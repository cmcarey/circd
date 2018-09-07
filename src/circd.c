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

	client->server = ll_add_node(&server->clients, client);

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
	ClientThreadArgs* args = (ClientThreadArgs*) vargs;
	Client* client = args->client;
	Server* server = args->server;

	while (1) {
		char m[BUFFSIZE] = {0};
		ssize_t r = recv(client->socket, &m, sizeof(m), 0);
		if (r == 0) {
			if (client->nick && client->username && client->realname) {
				handle_quit("Left the server", client, server);
			}
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
		while(msg[userLength] != ' ') userLength++;
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
	if(strncmp(msg, "PING ", 5) == 0) {
		msg += 5;
		char m[BUFFSIZE] = {0};
		snprintf(m, BUFFSIZE, ":cdirc PONG %s\n", msg);

	} else if(strncmp(msg, "JOIN ", 5) == 0) {
		msg += 5;
		handle_join(msg, client, server);

	} else if(strncmp(msg, "PART ", 5) == 0) {
		msg += 5;
		int channelNameLength = 0;
		while(msg[channelNameLength] != ' ' && channelNameLength < strlen(msg)) channelNameLength++;
		if(channelNameLength == strlen(msg)) {
			msg[channelNameLength] = '\0';
			handle_part(msg, "Goodbye", client, server);
		} else {
			msg[channelNameLength] = '\0';
			handle_part(msg, msg + channelNameLength + 2, client, server);
		}
		

	} else if(strncmp(msg, "QUIT ", 5) == 0) {
		msg += 5;
		handle_quit(msg + 1, client, server);

	} else if(strncmp(msg, "PRIVMSG ", 8) == 0) {
		msg += 8;
		int targetLength = 0;
		while(msg[targetLength] != ' ' && targetLength < strlen(msg)) targetLength++;
		msg[targetLength] = '\0';
		handle_privmsg(msg, msg + targetLength + 2, client, server);
	}
}


void client_host(char* dest, Client* client) {
	snprintf(dest, BUFFSIZE, "%s!%s@secret", client->nick, client->username);
}


void handle_join(char* channelName, Client* client, Server* server) {
	ChannelClient* channelClient = calloc(1, sizeof(ChannelClient));
	channelClient->client = client;
	channelClient->clientNode = ll_add_node(&client->channels, channelClient);

	char m[BUFFSIZE] = {0};
	char ch[BUFFSIZE] = {0};
	client_host(ch, client);
	snprintf(m, BUFFSIZE, ":%s JOIN %s\n", ch, channelName);

	LinkedListNode* channelNext = server->channels.head;
	while(channelNext) {
		Channel* channel = (Channel*) channelNext->ptr;
		if(strcmp(channel->name, channelName) == 0) {
			channelClient->channel = channel;
			channelClient->channelNode = ll_add_node(&channel->clients, channelClient);
			message_channel(m, channel, client, true);

			return;
		}

		channelNext = channelNext->next;
	}

	Channel* channel = calloc(1, sizeof(Channel));
	char* channelNameP = calloc(strlen(channelName) + 1, sizeof(char));
	strncpy(channelNameP, channelName, strlen(channelName));
	channel->name = channelNameP;
	channel->server = ll_add_node(&server->channels, channel);
	channelClient->channel = channel;
	channelClient->channelNode = ll_add_node(&channel->clients, channelClient);
	// todo: set op permission
	send(client->socket, m, strlen(m), 0);
}


void handle_part(char* channelName, char* partMessage, Client* client, Server* server) {
	Channel* channel;
	ChannelClient* channelClient;
	LinkedListNode* channelNext = client->channels.head;
	while (channelNext) {
		channelClient = (ChannelClient*) channelNext->ptr;
		channel = channelClient->channel;
		if(strcmp(channel->name, channelName) == 0) {
			break;
		}
		channelNext = channelNext->next;
	}

	if (!channel) {
		return;
	}

	char m[BUFFSIZE] = {0};
	char t[BUFFSIZE] = {0};
	client_host(t, client);
	snprintf(m, BUFFSIZE, ":%s PART %s :%s\n", t, channelName, partMessage);
	message_channel(m, channel, client, true);


	ll_delete_node(&client->channels, channelClient->clientNode);
	ll_delete_node(&channel->clients, channelClient->channelNode);
	free(channelClient);
}


void handle_quit(char* quitMessage, Client* client, Server* server) {
	char m[BUFFSIZE] = {0};
	char t[BUFFSIZE] = {0};
	client_host(t, client);
	snprintf(m, BUFFSIZE, ":%s QUIT :%s\n", t, quitMessage);

	LinkedListNode* channelNext = client->channels.head;
	while (channelNext) {
		ChannelClient* channelClient = (ChannelClient*) channelNext->ptr;
		Channel* channel = channelClient->channel;
		message_channel(m, channel, client, false);
		ll_delete_node(&client->channels, channelClient->clientNode);
		ll_delete_node(&channel->clients, channelClient->channelNode);
		free(channelClient);
		channelNext = channelNext->next;
	}

	ll_delete_node(&server->clients, client->server);
	free(client->nick);
	free(client->username);
	free(client->realname);
	free(client);
	close(client->socket);
	pthread_exit(0);
}


void handle_privmsg(char* target, char* msg, Client* client, Server* server) {
	if (target[0] == '#') {
		// send to a channel
		// first iterate through to see if user is in channel
		Channel* channel;
		LinkedListNode* channelNext = client->channels.head;
		while (channelNext) {
			ChannelClient* channelClient = (ChannelClient*) channelNext->ptr;
			channel = channelClient->channel;
			if(strcmp(channel->name, target) == 0) {
				break;
			}
			channelNext = channelNext->next;
		}

		if (!channel) {
			return;
		}

		char m[BUFFSIZE] = {0};
		char t[BUFFSIZE] = {0};
		client_host(t, client);
		snprintf(m, BUFFSIZE, ":%s PRIVMSG %s :%s\n", t, target, msg);
		message_channel(m, channel, client, false);

	} else {
		// send to a user
	}
}


void message_channel(char* msg, Channel* channel, Client* client, bool includeSender) {
	LinkedListNode* clientNext = channel->clients.head;
	while(clientNext) {
		ChannelClient* channelClient = (ChannelClient*) clientNext->ptr;
		Client* targetClient = channelClient->client;

		if (includeSender || targetClient != client) {
			send(targetClient->socket, msg, strlen(msg), 0);
		}

		clientNext = clientNext->next;
	}
}

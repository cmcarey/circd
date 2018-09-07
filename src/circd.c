#include "circd.h"

#define PORT 6667
#define BUFFSIZE 1024

int main() {
	Server server;
	memset(&server, 0, sizeof(server));

	// Create, bind, and listen on sockets
	create_server(PORT, &server);

	printf("Server started, listening on :%i\n", PORT);

	while (1) {
		accept_clients(&server);
	}

	return 0;
}


void create_server(int port, Server* server) {
	// Create IPv4 TCP socket
	int serverFD = socket(PF_INET, SOCK_STREAM, 0);
	if (serverFD == -1) {
		fprintf(stderr, "Failed to create server socket\n");
		exit(1);
	}

	// Set socket options to bind on port and any addres
	struct sockaddr_in serverParam;
	serverParam.sin_family = AF_INET;
	serverParam.sin_port = htons(port);
	serverParam.sin_addr.s_addr = htonl(INADDR_ANY);

	// Allow quick reuse of port
	setsockopt(serverFD, 
		SOL_SOCKET, 
		SO_REUSEADDR, 
		&(int){ 1 }, 
		sizeof(int));

	// Bind to port
	int bound = bind(serverFD, 
		(struct sockaddr*) &serverParam, 
		sizeof(serverParam));
	if (bound == -1) {
		fprintf(stderr, "Failed to bind to port\n");
		exit(1);
	}

	// Begin listening for clients
	int listening = listen(serverFD, 10);
	if (listening == -1) {
		fprintf(stderr, "Failed to listen\n");
		exit(1);
	}

	server->socket = serverFD;
}


void accept_clients(Server* server) {
	// Accept client, get socket (file descriptor)
	int clientFD = accept(server->socket, NULL, NULL);

	Client* client = calloc(1, sizeof(Client));
	client->socket = clientFD;

	// Add client to the linked list of clients of the Server
	client->server = ll_add_node(&server->clients, client);

	// Create type to pass in client and server to thread
	ClientThreadArgs* args = calloc(1, sizeof(ClientThreadArgs));
	args->server = server;
	args->client = client;

	pthread_t ptid;
	// Create thread to handle client
	int created = pthread_create(&ptid, NULL, handle_client, (void *) args);
	if (created != 0) {
		fprintf(stderr, "Failed to create thread to handle client\n");
	}
}


void* handle_client(void* vargs) {
	printf("Client connected\n");
	// Parse client and server from input argument struct
	ClientThreadArgs* args = (ClientThreadArgs*) vargs;
	Client* client = args->client;
	Server* server = args->server;
	free(args);

	// Main message loop
	while (1) {
		char m[BUFFSIZE] = {0};
		// Get message from client
		ssize_t r = recv(client->socket, &m, sizeof(m), 0);

		// If r == 0 then client has disconnected
		if (r == 0) {
			// If client had authenticated then send quit messages to connected
			//	channels, otherwise just disconnect
			if (client->nick && client->username && client->realname) {
				handle_quit("Left the server", client, server);
			}
			printf("Client disconnected\n");
			pthread_exit(0);
		}

		// Strip trailing newlines and cariage returns
		while (m[strlen(m) - 1] == '\n' || m[strlen(m) - 1] == '\r') {
			m[strlen(m) - 1] = '\0';
		}

		// Print message to console for debugging
		printf("> %s\n", m);

		// Only parse NICK/USER until client authenticates
		if (client->nick && client->username && client->realname) {
			handle_message(m, client, server);
		} else {
			handle_handshake(m, client);
		}
	}
}


void handle_handshake(char* msg, Client* client) {
	if (strncmp(msg, "NICK ", 5) == 0) {
		// Increment msg to nick part
		msg += 5;

		// Allocate memory to hold the nick string, then fill
		int nicklen = strlen(msg);
		char* nick = calloc(nicklen + 1, sizeof(char));
		strncpy(nick, msg, nicklen);
		client->nick = nick;

	} else if (strncmp(msg, "USER ", 5) == 0) {
		// Increment msg to username part
		msg += 5;
		int userLength = 0;
		// Find length of first string and store in allocated memory
		while(msg[userLength] != ' ') userLength++;
		char* username = calloc(userLength + 1, sizeof(char));
		strncpy(username, msg, userLength);
		client->username = username;

		// Increment msg to skip space
		msg += userLength + 1;

		// Get initial user mode
		unsigned int modes = msg[0] - '0';
		// Set client bit 3 if present
		client->modes = modes & 1 << 3;

		// Increment msg to skip unused field
		msg += 5;

		// Store rest of msg as realname
		char* realname = calloc(strlen(msg) + 1, sizeof(char));
		strncpy(realname, msg, strlen(msg));
		client->realname = realname;
	}

	// Send message once user has authenticated - RPL_WELCOME handshake
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
		// Sends PONG back to client with remainder of message
		msg += 5;
		char m[BUFFSIZE] = {0};
		snprintf(m, BUFFSIZE, ":cdirc PONG %s\n", msg);

	} else if(strncmp(msg, "JOIN ", 5) == 0) {
		// Handle client attempting to join a channel
		msg += 5;
		handle_join(msg, client, server);

	} else if(strncmp(msg, "PART ", 5) == 0) {
		// Handle client leaving a channel with part message or default "goodbye"
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
		// Handle client quitting the server
		msg += 5;
		handle_quit(msg + 1, client, server);

	} else if(strncmp(msg, "PRIVMSG ", 8) == 0) {
		// Handle client sending a `PRIVMSG` to a channel
		msg += 8;
		int targetLength = 0;
		while(msg[targetLength] != ' ' && targetLength < strlen(msg)) targetLength++;
		msg[targetLength] = '\0';
		handle_privmsg(msg, msg + targetLength + 2, client, server);
	}
}


void client_host(char* dest, Client* client) {
	// Concat string in RFC format nick!username@identifier
	// Default identifier as "secret"
	snprintf(dest, BUFFSIZE, "%s!%s@secret", client->nick, client->username);
}


void handle_join(char* channelName, Client* client, Server* server) {
	// Create a `ChannelClient` struct, allows bidirectional lookup
	//	between channel and client, significantly faster than
	//	iterating through all channels in server
	ChannelClient* channelClient = calloc(1, sizeof(ChannelClient));
	channelClient->client = client;
	channelClient->clientNode = ll_add_node(&client->channels, channelClient);

	char m[BUFFSIZE] = {0};
	char ch[BUFFSIZE] = {0};
	client_host(ch, client);
	snprintf(m, BUFFSIZE, ":%s JOIN %s\n", ch, channelName);

	LinkedListNode* channelNext = server->channels.head;
	while(channelNext) {
		// If channel exists, join, send join message to all present users
		Channel* channel = (Channel*) channelNext->ptr;
		if(strcmp(channel->name, channelName) == 0) {
			channelClient->channel = channel;
			channelClient->channelNode = ll_add_node(&channel->clients, channelClient);
			message_channel(m, channel, client, true);

			return;
		}

		channelNext = channelNext->next;
	}

	// If channel does not exist, create channel and add to server's
	//	linked list of channels
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
	// Find channel from PART msg
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
		// If user was not in channel then return
		return;
	}

	// Send PART message to all connected clients in channel
	char m[BUFFSIZE] = {0};
	char t[BUFFSIZE] = {0};
	client_host(t, client);
	snprintf(m, BUFFSIZE, ":%s PART %s :%s\n", t, channelName, partMessage);
	message_channel(m, channel, client, true);

	// Remove and clear channel<->client relation
	ll_delete_node(&client->channels, channelClient->clientNode);
	ll_delete_node(&channel->clients, channelClient->channelNode);
	free(channelClient);
}


void handle_quit(char* quitMessage, Client* client, Server* server) {
	// Prepare QUIT message
	char m[BUFFSIZE] = {0};
	char t[BUFFSIZE] = {0};
	client_host(t, client);
	snprintf(m, BUFFSIZE, ":%s QUIT :%s\n", t, quitMessage);

	LinkedListNode* channelNext = client->channels.head;
	while (channelNext) {
		// Iterate through connected channels, notifying each of client quit
		// Remove and clear channel<->client relation
		ChannelClient* channelClient = (ChannelClient*) channelNext->ptr;
		Channel* channel = channelClient->channel;
		message_channel(m, channel, client, false);
		ll_delete_node(&client->channels, channelClient->clientNode);
		ll_delete_node(&channel->clients, channelClient->channelNode);
		free(channelClient);
		channelNext = channelNext->next;
	}

	// Remove client from server linked list of clients
	ll_delete_node(&server->clients, client->server);
	// Clear client
	free(client->nick);
	free(client->username);
	free(client->realname);
	free(client);
	close(client->socket);
	pthread_exit(0);
}


void handle_privmsg(char* target, char* msg, Client* client, Server* server) {
	if (target[0] == '#') {
		// Handle sending PRIVMSG to a channel
		Channel* channel;
		LinkedListNode* channelNext = client->channels.head;
		while (channelNext) {
			// Find mentioned channel that client is connected to
			ChannelClient* channelClient = (ChannelClient*) channelNext->ptr;
			channel = channelClient->channel;
			if(strcmp(channel->name, target) == 0) {
				break;
			}
			channelNext = channelNext->next;
		}

		if (!channel) {
			// Return if client is not connected to channel they are messaging
			return;
		}

		// Send message to all users in channel except for client
		char m[BUFFSIZE] = {0};
		char t[BUFFSIZE] = {0};
		client_host(t, client);
		snprintf(m, BUFFSIZE, ":%s PRIVMSG %s :%s\n", t, target, msg);
		message_channel(m, channel, client, false);

	} else {
		// todo: Handle PRIVMSG direct to other client
	}
}


void message_channel(char* msg, Channel* channel, Client* client, bool includeSender) {
	// Send message to each client connected to a channel
	// Optionally send message to sender client (for messages like PART
	//	and JOIN, but not PRIVMSG)
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

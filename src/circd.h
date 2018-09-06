#ifndef H_CIRCD
#define H_CIRCD

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include "linkedlist.h"


typedef struct Server {
	LinkedList clients; //@Client
	LinkedList channels; //@Channel
	int socket;
} Server;

typedef struct Client {
	char* nick;
	char* username;
	char* realname;
	unsigned int modes;
	LinkedList channels; //@ChannelChannel
	int socket;
} Client;

typedef struct Channel {
	char* name;
	char* topic;
	unsigned int modes;
	LinkedList clients; //@ChannelClient
} Channel;

typedef struct ChannelClient {
	Client* client;
	Channel* channel;
	unsigned int modes;
} ChannelClient;

typedef struct ClientThreadArgs {
	Server* server;
	Client* client;
} ClientThreadArgs;


int main();
void create_server(int port, Server* server);
void accept_clients(Server* server);
void* handle_client(void* args);
void handle_handshake(char* msg, Client* client);
void handle_message(char* msg, Client* client, Server* server);
void client_host(char* dest, Client* client);
void handle_join(char* channelName, Client* client, Server* server);
void handle_part(char* channelName, char* partMessage, Client* client, Server* server);
void handle_quit(char* quitMessage, Client* client, Server* server);
void handle_privmsg(char* target, char* message, Client* client, Server* server);

#endif

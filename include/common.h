#pragma once

#include <arpa/inet.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/socket.h>

// Operation structure to be used in communication between client and server
struct BlogOperation {
  int client_id;
  int operation_type;
  int server_response;
  char topic[50];
  char content[2048];
};

// Address structure to be used in communication between client and server
struct Address {
  char *ip;
  char *port;
};

// Function prototypes related to sockets
struct BlogOperation createOperation(int client_id, int operation_type,
                                     int server_response, char topic[50],
                                     char content[2048]);
int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage);
int server_sockaddr_init(const char *protocol_version, const char *portstr,
                         struct sockaddr_storage *storage);
void logexit(const char *msg);
size_t receive_all(int socket, void *buffer, size_t length);
void printBlogOperation(struct BlogOperation operation);

// Constants
#define EXIT_FAILURE 1
#define RECEIVING_CONNECTIONS 1
#define RECEIVING_DATA 1
#define NOT_FOUND -1

#define NEW_CONNECTION 1
#define NEW_POST_IN_TOPIC 2
#define LIST_TOPICS 3
#define SUBSCRIBE_IN_TOPIC 4
#define DISCONNECT 5
#define UNSUBSCRIBE_IN_TOPIC 6
#define INVALID 7

#include "common.h"

// Operation structure to be used in communication between client and server
struct BlogOperation createOperation(int clientID, int operationType,
                                     int serverResponse, char topic[50],
                                     char content[2048]) {
  struct BlogOperation operation;
  operation.client_id = clientID;
  operation.operation_type = operationType;
  operation.server_response = serverResponse;
  strcpy(operation.topic, topic);
  strcpy(operation.content, content);
  return operation;
}

// Capture the address and port from the command line and store it in a
// sockaddr_storage structure
int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage) {
  if (addrstr == NULL || portstr == NULL) {
    return -1;
  }

  uint16_t port = (uint16_t)atoi(portstr);

  if (port == 0) {
    return -1;
  }

  port = htons(port);

  struct in_addr inaddr4;
  if (inet_pton(AF_INET, addrstr, &inaddr4)) {
    struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
    addr4->sin_family = AF_INET;
    addr4->sin_port = port;
    addr4->sin_addr = inaddr4;
    return 0;
  }

  struct in6_addr inaddr6;
  if (inet_pton(AF_INET6, addrstr, &inaddr6)) {
    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
    addr6->sin6_family = AF_INET6;
    addr6->sin6_port = port;
    memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
    return 0;
  }

  return -1;
}

void logexit(const char *str) {
  perror(str);
  exit(EXIT_FAILURE);
}

// Initialize the socket address structure
int server_sockaddr_init(const char *protocol_version, const char *portstr,
                         struct sockaddr_storage *storage) {
  uint16_t port = (uint16_t)atoi(portstr);
  if (port == 0) {
    return -1;
  }

  port = htons(port);

  memset(storage, 0, sizeof(*storage));
  if (strcmp(protocol_version, "v4") == 0) {
    struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
    addr4->sin_family = AF_INET;
    addr4->sin_addr.s_addr = INADDR_ANY;
    addr4->sin_port = port;
  } else if (strcmp(protocol_version, "v6") == 0) {
    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
    addr6->sin6_family = AF_INET6;
    addr6->sin6_addr = in6addr_any;
    addr6->sin6_port = port;
  } else {
    return -1;
  }

  return 0;
}

// Receive all data from a socket
size_t receive_all(int socket, void *buffer, size_t size) {
  size_t total_received = 0;
  while (total_received < size) {
    size_t bytes_received =
        recv(socket, buffer + total_received, size - total_received, 0);
    if (bytes_received <= 0) {
      if (bytes_received == 0)
        return total_received;
      else {
        perror("Error receiving data");
        return -1;
      }
    }
    total_received += bytes_received;
  }
  return total_received;
}

// Print blog operation for debugging purposes
void printBlogOperation(struct BlogOperation operation) {
  printf("Client ID: %d\n", operation.client_id);
  printf("Operation Type: %d\n", operation.operation_type);
  printf("Server Response: %d\n", operation.server_response);
  printf("Topic: %s\n", operation.topic);
  printf("Content: %s\n", operation.content);
}

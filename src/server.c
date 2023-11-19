#include "server.h"
#include "common.h"

struct Address serverAdress;

int main(int argc, char *argv[]) {
  // Check and store the command line arguments
  if (argc < 3) {
    printf("Wrong usage");
    exit(EXIT_FAILURE);
  }
  serverAdress.ip = argv[1];
  serverAdress.port = argv[2];

  // Initialize socket
  int sockfd;
  sockfd = createSocket();

  // Initializes blog
  blog.clientsCount = 0;
  blog.topicsCount = 0;
  for (int i = 0; i < 10; i++) {
    blog.clients[i].id = -1;
  }

  // Waiting for someone to connect
  while (RECEIVING_CONNECTIONS) {

    // Accept client connection
    struct sockaddr_storage cstorage;
    struct sockaddr *caddr = (struct sockaddr *)&cstorage;
    socklen_t caddrlen = sizeof(cstorage);
    int csock = accept(sockfd, caddr, &caddrlen);
    if (csock == -1) {
      logexit("accept");
    }

    // Creating and adding new client to the blog
    struct Client newClient;
    newClient.socket = csock;
    // New client ID is the first available ID
    for (int i = 0; i < 10; i++) {
      if (blog.clients[i].id == -1) {
        newClient.id = i;
        break;
      }
    }
    addUserToBlog(newClient);
    messageClientConnected(newClient);

    // Send response to client
    struct BlogOperation serverResponse =
        createOperation(newClient.id, NEW_CONNECTION, 1, "", "");
    size_t size = send(csock, &serverResponse, sizeof(serverResponse), 0);
    printf("\n");
    if (size != sizeof(serverResponse)) {
      logexit("send");
    }

    // Create a thread to handle the client
    pthread_create(&(blog.clients[newClient.id].thread), NULL, function,
                   (void *)&newClient);
  }

  // Wait for all threads to finish
  waitForThreads(blog);
  return 0;
}

// Initialize the socket
int createSocket() {

  // initialize adress storage struct (IPv4 or IPv6)
  struct sockaddr_storage storage;
  if (server_sockaddr_init(serverAdress.ip, serverAdress.port, &storage)) {
    logexit("serverSockaddrInit");
  }

  // create socket file descriptor
  int sockfd = socket(storage.ss_family, SOCK_STREAM, 0);
  if (sockfd == -1) {
    logexit("socket");
  }

  int enableSocketReuse = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enableSocketReuse,
                 sizeof(int)) != 0) {
    logexit("setsockopt");
  }

  // cast to sockaddr
  struct sockaddr *addr = (struct sockaddr *)(&storage);

  // bind socket(sockfd) to address(storage)
  if (bind(sockfd, addr, sizeof(storage)) != 0) {
    logexit("bind");
  }

  // listen for connections on socket(sockfd)
  if (listen(sockfd, 10) != 0) {
    logexit("listen");
  }

  return sockfd;
}

// Add a new client to the blog
void addUserToBlog(struct Client client) {
  blog.clients[client.id] = client;
  blog.clientsCount++;
}

// Print a message when a client connects
void messageClientConnected(struct Client client) {
  printf("Client 0%d connected", client.id + 1);
}

// Wait for all threads to finish
void waitForThreads(struct Blog blog) {
  for (int i = 0; i < 10; i++) {
    if (blog.clients[i].id != -1) {
      pthread_join(blog.clients[i].thread, NULL);
    }
  }
}

// Function to be executed by the client thread
void *function(void *thread) {
  // Assigns this pointer the address held by thread
  struct Client *client = (struct Client *)thread;
  int socket = client->socket;

  while (RECEIVING_DATA) {
    // Receive data from client
    struct BlogOperation operationRequestedByClient;
    size_t size = receive_all(socket, &operationRequestedByClient,
                              sizeof(operationRequestedByClient));

    if (size == 0) {
      operationRequestedByClient.operation_type = DISCONNECT;
    }

    if (size != sizeof(operationRequestedByClient) && size != 0) {
      logexit("receive");
    }

    // Create server response to client
    struct BlogOperation operationSendByServer =
        createOperationToSend(operationRequestedByClient);

    // Send response to client
    size =
        send(socket, &operationSendByServer, sizeof(operationSendByServer), 0);
    if (operationRequestedByClient.operation_type == DISCONNECT) {
      close(socket);
      break;
    }
  }
  return NULL;
}

// Create response operation based on the operation requested by client
struct BlogOperation
createOperationToSend(struct BlogOperation operationRequestedByClient) {
  struct BlogOperation operationToSend;
  operationToSend.client_id = operationRequestedByClient.client_id;
  operationToSend.operation_type = 0;
  operationToSend.server_response = 1;
  int topicIndex;
  struct Client *client = findClientByID(operationRequestedByClient.client_id);

  switch (operationRequestedByClient.operation_type) {

  case NEW_POST_IN_TOPIC:
    // Create a new post in a topic if it exists, if not, create a new topic and
    // add the post in it It's the index in the blog.topics array

    topicIndex = findTopic(operationRequestedByClient.topic);
    if (topicIndex == NOT_FOUND) {
      createTopic(operationRequestedByClient.topic);
      topicIndex = findTopic(operationRequestedByClient.topic);
      addPostInTopic(operationRequestedByClient, topicIndex);
    } else {
      addPostInTopic(operationRequestedByClient, topicIndex);
      // Notify all subscribers
      for (int i = 0; i < blog.topics[topicIndex].subscribersCount; i++) {
        struct BlogOperation operationSendByServer;
        operationSendByServer.client_id = operationRequestedByClient.client_id;
        operationSendByServer.operation_type = NEW_POST_IN_TOPIC;
        operationSendByServer.server_response = 1;
        strcpy(operationSendByServer.topic, operationRequestedByClient.topic);
        strcpy(operationSendByServer.content,
               operationRequestedByClient.content);
        size_t size =
            send(blog.topics[topicIndex].subscribers[i].socket,
                 &operationSendByServer, sizeof(operationSendByServer), 0);
        if (size != sizeof(operationSendByServer)) {
          logexit("send");
        }
      }
    }
    messageNewPostInTopic(topicIndex);
    break;

  case LIST_TOPICS:
    // Check if there are topics in the blog and send them to the client
    if (hasTopics()) {
      char *allTopics = malloc(sizeof(char) * 2048);
      listTopics(allTopics);
      operationToSend = createOperation(operationRequestedByClient.client_id,
                                        LIST_TOPICS, 1, "", allTopics);
    } else {
      operationToSend =
          createOperation(operationRequestedByClient.client_id, LIST_TOPICS, 1,
                          "", "no topics available");
    }
    break;

  case SUBSCRIBE_IN_TOPIC:
    // Subscribe a client in a topic if it exists
    topicIndex = findTopic(operationRequestedByClient.topic);
    char *content = "";
    // If the topic doesn't exist, create it and subscribe the client
    if (topicIndex == NOT_FOUND) {
      createTopic(operationRequestedByClient.topic);
      topicIndex = findTopic(operationRequestedByClient.topic);
      subscribeClientInTopic(*client, topicIndex);
      printf("client 0%d subscribed to %s\n", client->id + 1,
             operationRequestedByClient.topic);
    // If the topic exists and the client is not subscribed, subscribe it
    } else if (!isSubscribed(client->id, topicIndex)) {
      subscribeClientInTopic(*client, topicIndex);
      printf("client 0%d subscribed to %s\n", client->id + 1,
             operationRequestedByClient.topic);
    // If the topic exists and the client is already subscribed, send an error
    } else {
      content = "error: already subscribed\n";
    }
    operationToSend = createOperation(
        operationRequestedByClient.client_id, SUBSCRIBE_IN_TOPIC, 1,
        operationRequestedByClient.topic, content);
    break;

  case DISCONNECT:
    // Remove client from subscriptions and disconnect it
    disconnectClient(operationRequestedByClient.client_id);
    messageClientDisconnected(*client);
    blog.clients[operationRequestedByClient.client_id].id = -1;
    operationToSend = createOperation(operationRequestedByClient.client_id,
                                      DISCONNECT, 1, "", "");
    break;

  case UNSUBSCRIBE_IN_TOPIC:
    // Unsubscribe a client in a topic if it exists
    topicIndex = findTopic(operationRequestedByClient.topic);
    if (topicIndex != NOT_FOUND && isSubscribed(client->id, topicIndex)) {
      unsubscribeClientInTopic(client->id, topicIndex);
    }
    operationToSend = createOperation(operationRequestedByClient.client_id,
                                      UNSUBSCRIBE_IN_TOPIC, 1,
                                      operationRequestedByClient.topic, "");
    break;

  default:
    break;
  }

  return operationToSend;
}

// Check if there are topics in the blog
bool hasTopics() { return blog.topicsCount > 0; }

// Print all topics in the blog separated by ;
void listTopics(char *topics) {

  for (int i = 0; i < blog.topicsCount; i++) {
    strcat(topics, blog.topics[i].title);
    if (i < blog.topicsCount - 1) {
      strcat(topics, ";");
    }
  }
}

// Find an existing topic in the blog
int findTopic(char topic[50]) {
  for (int i = 0; i < blog.topicsCount; i++) {
    if (strcmp(blog.topics[i].title, topic) == 0) {
      return i;
    }
  }
  return NOT_FOUND;
}

// Create a new topic in the blog
void createTopic(char topic[50]) {
  struct Topic newTopic;
  newTopic.id = blog.topicsCount;
  strcpy(newTopic.title, topic);
  newTopic.postCount = 0;
  newTopic.subscribersCount = 0;
  blog.topics[blog.topicsCount] = newTopic;
  blog.topicsCount++;
  for (int i = 0; i < 10; i++) {
    blog.topics[blog.topicsCount - 1].subscribers[i].id = -1;
  }
}

// Add a new post in a topic
void addPostInTopic(struct BlogOperation operationRequestedByClient,
                    int topicIndex) {
  int currentTopic = blog.topicsCount - 1;
  int postIndex = blog.topics[currentTopic].postCount;
  strcpy(blog.topics[currentTopic].posts[postIndex],
         operationRequestedByClient.content);
  blog.topics[currentTopic].postAuthorsID[postIndex] =
      operationRequestedByClient.client_id;
  blog.topics[currentTopic].postCount++;
}

// Print a message when a new post is added in a topic
void messageNewPostInTopic(int topicIndex) {
  struct Topic topic = blog.topics[topicIndex];
  printf("new post added in %s by 0%d\n", topic.title,
         topic.postAuthorsID[topic.postCount - 1] + 1);
}

// Check if a client is already subscribed in a topic
bool isSubscribed(int clientID, int topicIndex) {
  bool isSubscribed = false;
  for (int i = 0; i < blog.topics[topicIndex].subscribersCount; i++) {
    if (clientID == blog.topics[topicIndex].subscribers[i].id) {
      isSubscribed = true;
    }
  }
  return isSubscribed;
}

// Subscribe a client in a topic
void subscribeClientInTopic(struct Client client, int topicIndex) {
  blog.topics[topicIndex]
      .subscribers[blog.topics[topicIndex].subscribersCount] = client;
  blog.topics[topicIndex].subscribersCount++;
}

// Unsubscribe a client in a topic
void unsubscribeClientInTopic(int clientID, int topicIndex) {
  for (int i = 0; i < blog.topics[topicIndex].subscribersCount; i++) {
    if (clientID == blog.topics[topicIndex].subscribers[i].id) {
      blog.topics[topicIndex].subscribers[i] =
          blog.topics[topicIndex]
              .subscribers[blog.topics[topicIndex].subscribersCount - 1];
      blog.topics[topicIndex].subscribersCount--;
    }
  }
}

// Remove client from subscriptions and disconnect it
void disconnectClient(int clientID) {
  for (int i = 0; i < blog.topicsCount; i++) {
    if (isSubscribed(clientID, i)) {
      unsubscribeClientInTopic(clientID, i);
    }
  }
  blog.clientsCount--;
}

// Print a message when a client disconnects
void messageClientDisconnected(struct Client client) {
  printf("client 0%d disconnected\n", client.id + 1);
}

// Find a client in the blog by its ID
struct Client *findClientByID(int clientID) {
  for (int i = 0; i < 10; i++) {
    if (clientID == blog.clients[i].id) {
      return &blog.clients[i];
    }
  }
  return NULL;
}

// Print the current blog for debugging purposes
void printCurrentBlog() {
  printf("\nCurrent blog:\n");
  printf("Clients:\n");
  for (int i = 0; i < 10; i++) {
    printf("Client %d\n", blog.clients[i].id);
  }
  printf("Topics:\n");
  for (int i = 0; i < blog.topicsCount; i++) {
    printf("\tTopic %d\n", blog.topics[i].id);
    printf("\tTitle: %s\n", blog.topics[i].title);
    printf("\tPosts:\n");
    for (int j = 0; j < blog.topics[i].postCount; j++) {
      printf("\t\tPost %d\n", j);
      printf("\t\tContent: %s", blog.topics[i].posts[j]);
      printf("\t\tAuthor: %d\n", blog.topics[i].postAuthorsID[j]);
    }
    printf("\tSubscribers:\n");
    for (int j = 0; j < blog.topics[i].subscribersCount; j++) {
      printf("\t\tSubscriber %d\n", j);
      printf("\t\tID: %d\n", blog.topics[i].subscribers[j].id);
      printf("\t\tSocket: %d\n", blog.topics[i].subscribers[j].socket);
    }
  }
}

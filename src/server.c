#include "server.h"

struct Address serverAdress;

int main(int argc, char *argv[])
{
    // Check and store the command line arguments
    if (argc < 3)
    {
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

    // Waiting for someone to connect
    while (RECEIVING_CONNECTIONS)
    {

        // Accept client connection
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)&cstorage;
        socklen_t caddrlen = sizeof(cstorage);
        int csock = accept(sockfd, caddr, &caddrlen);
        if (csock == -1)
        {
            logexit("accept");
        }

        // Creating and adding new client to the blog
        struct Client newClient;
        newClient.socket = csock;
        newClient.id = blog.clientsCount;
        addUserToBlog(newClient);
        messageClientConnected(newClient);

        // Send response to client
        struct BlogOperation serverResponse = createOperation(newClient.id, NEW_CONNECTION, 1, "", "");
        size_t size = send(csock, &serverResponse, sizeof(serverResponse), 0);
        if (size != sizeof(serverResponse))
        {
            logexit("send");
        }

        // Create a thread to handle the client
        pthread_create(&(blog.clients[newClient.id].thread), NULL, function, (void *)&newClient);

    }

    // Wait for all threads to finish
    waitForThreads(blog);
    return 0;
}

// Initialize the socket
int createSocket()
{

    // initialize adress storage struct (IPv4 or IPv6)
    struct sockaddr_storage storage;
    if (server_sockaddr_init(serverAdress.ip, serverAdress.port, &storage))
    {
        logexit("serverSockaddrInit");
    }

    // create socket file descriptor
    int sockfd = socket(storage.ss_family, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        logexit("socket");
    }

    int enableSocketReuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enableSocketReuse, sizeof(int)) != 0)
    {
        logexit("setsockopt");
    }

    // cast to sockaddr
    struct sockaddr *addr = (struct sockaddr *)(&storage);

    // bind socket(sockfd) to address(storage)
    if (bind(sockfd, addr, sizeof(storage)) != 0)
    {
        logexit("bind");
    }

    // listen for connections on socket(sockfd)
    if (listen(sockfd, 10) != 0)
    {
        logexit("listen");
    }

    return sockfd;
}

// Add a new client to the blog
void addUserToBlog(struct Client client)
{
    blog.clients[blog.clientsCount] = client;
    blog.clientsCount++;
}

// Print a message when a client connects
void messageClientConnected(struct Client client)
{
    printf("Client %d connected\n", client.id);
}

// Wait for all threads to finish
void waitForThreads(struct Blog blog)
{
    for (int i = 0; i < blog.clientsCount; i++)
    {
        pthread_join(blog.clients[i].thread, NULL);
    }
}

// Function to be executed by the client thread
void *function(void *thread)
{
    // Assigns this pointer the address held by thread
    struct Client *client = (struct Client *)thread;
    int socket = client->socket;

    while (RECEIVING_DATA)
    {
        // Receive data from client
        struct BlogOperation operationRequestedByClient;
        size_t size = receive_all(client->socket, &operationRequestedByClient, sizeof(operationRequestedByClient));
        if (size != sizeof(operationRequestedByClient))
        {
            logexit("receive");
        }

        // Create server response to client
        struct BlogOperation operationSendByServer = createOperationToSend(operationRequestedByClient, &operationSendByServer, *client);

        // Send response to client
        size = send(socket, &operationSendByServer, sizeof(operationSendByServer), 0);
    }
    return NULL;
}

struct BlogOperation createOperationToSend(struct BlogOperation operationRequestedByClient, struct BlogOperation *operationSendByServer, struct Client client)
{
    struct BlogOperation operationToSend;
    operationToSend.client_id = operationRequestedByClient.client_id;
    operationToSend.operation_type = 0;
    operationToSend.server_response = 1;
    int topicIndex, hasTopic;
    switch (operationRequestedByClient.operation_type)
    {
    
    case NEW_POST_IN_TOPIC:
        // Create a new post in a topic if it exists, if not, create a new topic and add the post in it
        // It's the index in the blog.topics array

        topicIndex = findTopic(operationRequestedByClient.topic);
        if (topicIndex == NOT_FOUND)
        {
            createTopic(operationRequestedByClient.topic);
            topicIndex = findTopic(operationRequestedByClient.topic);
            addPostInTopic(operationRequestedByClient, topicIndex);
        }
        else
        {
            addPostInTopic(operationRequestedByClient, topicIndex);
            for (int i = 0; i < blog.topics[topicIndex].subscribersCount; i++)
            {
                operationSendByServer->client_id = operationRequestedByClient.client_id;
                operationSendByServer->operation_type = NEW_POST_IN_TOPIC;
                operationSendByServer->server_response = 1;
                strcpy(operationSendByServer->topic, operationRequestedByClient.topic);
                strcpy(operationSendByServer->content, operationRequestedByClient.content);
                size_t size = send(blog.topics[topicIndex].subscribers[i].socket, operationSendByServer, sizeof(*operationSendByServer), 0);
                if (size != sizeof(*operationSendByServer))
                {
                    logexit("send");
                }
            }
        }
        messageNewPostInTopic(topicIndex);
        break;

    case LIST_TOPICS:
        // Check if there are topics in the blog and send them to the client
        if (hasTopics())
        {
            char* allTopics = malloc(sizeof(char) * 2048);
            listTopics(allTopics);
            operationToSend = createOperation(operationRequestedByClient.client_id, LIST_TOPICS, 1, "", allTopics);
        }
        else
        {
            operationToSend = createOperation(operationRequestedByClient.client_id, LIST_TOPICS, 1, "", "no topics available");
        }
        break;

    case SUBSCRIBE_IN_TOPIC:
        // Subscribe a client in a topic if it exists
        topicIndex = findTopic(operationRequestedByClient.topic);
        if (topicIndex != NOT_FOUND && !isSubscribed(client, topicIndex))
        {
            subscribeClientInTopic(client, topicIndex);
        }
        else
        {
            printf("Client %d is already subscribed in topic %s\n", client.id, operationRequestedByClient.topic);
        }
        break;

    case DISCONNECT:
        // Disconnect a client from the blog
        disconnectClient(operationRequestedByClient.client_id);
        messageClientDisconnected(client);
        break;

    case UNSUBSCRIBE_IN_TOPIC:
        // Unsuscribe a client in a topic if it exists
        topicIndex = findTopic(operationRequestedByClient.topic);
        printf("topicIndex: %d\n", topicIndex);
        printf("topic: %s\n", operationRequestedByClient.topic);
        printf("isSubscribed: %d\n", isSubscribed(client, topicIndex));
        if (topicIndex != NOT_FOUND && isSubscribed(client, topicIndex))
        {
            unsubscribeClientInTopic(client, topicIndex);
        }
        else
        {
            printf("Client %d is not subscribed in topic %s\n", client.id, operationRequestedByClient.topic);
        }
        break;

    default:
        break;
    }

    return operationToSend;
}

// Check if there are topics in the blog
bool hasTopics()
{
    return blog.topicsCount > 0;
}

// Print all topics in the blog separated by ;
void listTopics(char* topics)
{

    for (int i = 0; i < blog.topicsCount; i++)
    {
        strcat(topics, blog.topics[i].title);
        if (i < blog.topicsCount - 1)
        {
            strcat(topics, ";");
        }
    }

}

// Find an existing topic in the blog
int findTopic(char topic[50])
{
    for (int i = 0; i < blog.topicsCount; i++)
    {
        if (strcmp(blog.topics[i].title, topic) == 0)
        {
            return i;
        }
    }
    return NOT_FOUND;
}

// Create a new topic in the blog
void createTopic(char topic[50])
{
    struct Topic newTopic;
    newTopic.id = blog.topicsCount;
    strcpy(newTopic.title, topic);
    newTopic.postCount = 0;
    newTopic.subscribersCount = 0;
    blog.topics[blog.topicsCount] = newTopic;
    blog.topicsCount++;
    for (int i = 0; i < 10; i++)
    {
        blog.topics[blog.topicsCount - 1].subscribers[i].id = -1;
    }
}

// Add a new post in a topic
void addPostInTopic(struct BlogOperation operationRequestedByClient, int topicIndex)
{
    struct Topic topic = blog.topics[blog.topicsCount - 1];
    topic.posts[topic.postCount] = operationRequestedByClient.content;
    topic.postAuthorsID[topic.postCount] = operationRequestedByClient.client_id;
    topic.postCount++;
}

// Print a message when a new post is added in a topic
void messageNewPostInTopic(int topicIndex)
{
    struct Topic topic = blog.topics[topicIndex];
    printf("new post added in %s by %d\n", topic.title, topic.postAuthorsID[topic.postCount - 1]);
}

// Check if a client is already subscribed in a topic
bool isSubscribed(struct Client client, int topicIndex)
{
    bool isSubscribed = false;
    struct Topic topic = blog.topics[topicIndex];
    for (int i = 0; i < topic.subscribersCount; i++)
    {
        if (client.id == blog.topics[topicIndex].subscribers[i].id)
        {
            isSubscribed = true;
        }
    }
    return isSubscribed;
}

// Subscribe a client in a topic
void subscribeClientInTopic(struct Client client, int topicIndex)
{
    blog.topics[topicIndex].subscribers[blog.topics[topicIndex].subscribersCount] = client;
    blog.topics[topicIndex].subscribersCount++;
}

// Unsubscribe a client in a topic
void unsubscribeClientInTopic(struct Client client, int topicIndex)
{
    struct Topic topic = blog.topics[topicIndex];
    for (int i = 0; i < topic.subscribersCount; i++)
    {
        if (client.id == blog.topics[topicIndex].subscribers[i].id)
        {
            blog.topics[topicIndex].subscribers[i] = blog.topics[topicIndex].subscribers[topic.subscribersCount - 1];
            blog.topics[topicIndex].subscribersCount--;
        }
    }
}

// Disconnect a client from the blog
void disconnectClient(int clientID)
{
    for (int i = 0; i < blog.clientsCount; i++)
    {
        if (clientID == blog.clients[i].id)
        {
            blog.clients[i] = blog.clients[blog.clientsCount - 1];
            blog.clientsCount--;
        }
    }
}


// Print a message when a client disconnects
void messageClientDisconnected(struct Client client)
{
    printf("client %d was disconnected\n", client.id);
}

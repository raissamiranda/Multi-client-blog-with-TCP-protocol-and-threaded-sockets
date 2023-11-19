#include "client.h"
#include "common.h"

struct Address serverAdress;

int main(int argc, char **argv)
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
    // Stores information about the IP version and port
    struct sockaddr_storage storage;
    if (addrparse(serverAdress.ip, serverAdress.port, &storage) != 0)
    {
        logexit("addrparse");
    }

    // Creates socket for TCP communication
    sockfd = socket(storage.ss_family, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        logexit("socket");
    }

    // Creates connection
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (connect(sockfd, addr, sizeof(storage)) != 0)
    {
        logexit("connect");
    }
    // Create client request
    struct BlogOperation operationToSendByClient = createOperation(0, NEW_CONNECTION, 0, "", "");

    // Send connection request to server
    size_t size = send(sockfd, &operationToSendByClient, sizeof(operationToSendByClient), 0);
    printf("Sending...\n");
    printBlogOperation(operationToSendByClient);
    printf("\n");
    if (size != sizeof(operationToSendByClient))
    {
        logexit("send");
    }

    // Receive response from server
    struct BlogOperation operationReceivedByServer;
    size = receive_all(sockfd, &operationReceivedByServer, sizeof(operationReceivedByServer));
    if (size != sizeof(operationReceivedByServer))
    {
        logexit("receive");
    }
    printf("Received...\n");
    printBlogOperation(operationReceivedByServer);
    printf("\n");

    // Create thread to handle the client
    pthread_create(&clientThread, NULL, waitingFunction, (void *)&sockfd);

    while (1)
    {
        struct BlogOperation operationToSend;
        fgets(command, sizeof(command), stdin);
        int commandType = handleCommand(command);

        switch (commandType)
        {
        case INVALID:
            printf("Invalid command\n");
            break;

        case NEW_POST_IN_TOPIC:
            topic = getTopic(commandType, command);
            char *content = malloc(sizeof(char) * 2048);
            fgets(content, 2048, stdin);
            operationToSend = createOperation(operationReceivedByServer.client_id, NEW_POST_IN_TOPIC, 0, topic, content);
            break;

        case LIST_TOPICS:
            operationToSend = createOperation(operationReceivedByServer.client_id, LIST_TOPICS, 0, "", "");
            break;

        case SUBSCRIBE_IN_TOPIC:
            topic = getTopic(commandType, command);
            operationToSend = createOperation(operationReceivedByServer.client_id, SUBSCRIBE_IN_TOPIC, 0, topic, "");
            break;

        case UNSUBSCRIBE_IN_TOPIC:
            topic = getTopic(commandType, command);
            operationToSend = createOperation(operationReceivedByServer.client_id, UNSUBSCRIBE_IN_TOPIC, 0, topic, "");
            break;

        case DISCONNECT:
            operationToSend = createOperation(operationReceivedByServer.client_id, DISCONNECT, 0, "", "");
            break;

        default:
            break;
        }

        // Send request to server
        if (commandType != INVALID)
        {
            printf("\nENVIADO PELO CLIENTE:\n");
            printBlogOperation(operationToSend);
            size = send(sockfd, &operationToSend, sizeof(operationToSend), 0);
            if (size != sizeof(operationToSend))
            {
                logexit("send");
            }
        }
        if (commandType == DISCONNECT)
        {
            break;
        }
    }

    return 0;
}

int handleCommand(char *input)
{
    input[strlen(input) - 1] = '\0';    // Remove \n from input
    char *command = strtok(input, " "); // Get the command

    for (int i = 0; i < sizeof(commands) / sizeof(struct Command); i++)
    {
        // Check if the command is valid
        if (strncmp(input, commands[i].name, strlen(commands[i].name)) == 0)
        {
            if (strcmp(command, "exit") == 0)
            {
                return DISCONNECT;
            }
            if (strcmp(command, "list") == 0)
            {
                char *keyword = strtok(NULL, " ");
                if (strcmp(keyword, "topics") == 0)
                {
                    return LIST_TOPICS;
                }
                return INVALID;
            }
            if (strcmp(command, "subscribe") == 0 || strcmp(command, "unsubscribe") == 0)
            {
                return (strcmp(command, "subscribe") == 0) ? SUBSCRIBE_IN_TOPIC : UNSUBSCRIBE_IN_TOPIC;
            }
            if (strcmp(command, "publish") == 0)
            {
                char *keyword = strtok(NULL, " ");
                if (keyword == NULL)
                {
                    return INVALID;
                }
                if (strcmp(keyword, "in") == 0)
                    return NEW_POST_IN_TOPIC;
            }
        }
    }
    return INVALID;
}

// Get the content of the command
char *getTopic(int cmd, char *cmdLine)
{
    char *theTopic = malloc(sizeof(char) * 2048);
    switch (cmd)
    {
    case NEW_POST_IN_TOPIC:
        strcpy(theTopic, cmdLine + 11);
        return theTopic;

    case SUBSCRIBE_IN_TOPIC:
        strcpy(theTopic, cmdLine + 10);
        return theTopic;

    case UNSUBSCRIBE_IN_TOPIC:
        strcpy(theTopic, cmdLine + 12);
        return theTopic;

    default:
        return NULL;
    }
    return NULL;
}

void messageDisconnect()
{
    printf("exit\n");
}

void *waitingFunction(void *sock)
{ // socket do cliente
    int *sockfd = (int *)sock;
    while (1)
    {
        struct BlogOperation operationReceivedByServer;
        // Receive response from server
        size_t size = receive_all(*sockfd, &operationReceivedByServer, sizeof(operationReceivedByServer));
        printf("Receiving...\n");
        printBlogOperation(operationReceivedByServer);
        printf("\n");
        if (size != sizeof(operationReceivedByServer))
        {
            logexit("receive");
        }

        switch (operationReceivedByServer.operation_type)
        {
        case NEW_POST_IN_TOPIC:
            printf("new post added in %s by 0%d\n%s", operationReceivedByServer.topic, operationReceivedByServer.client_id, operationReceivedByServer.content);
            break;

        case LIST_TOPICS:
            printf("%s\n", operationReceivedByServer.content);
            break;

        case SUBSCRIBE_IN_TOPIC:
            printf("%s", operationReceivedByServer.content);
            break;

        case DISCONNECT:
            close(*sockfd);
            pthread_exit(NULL);
            printf("OI\n");
            break;

        default:
            break;
        }
    }
}
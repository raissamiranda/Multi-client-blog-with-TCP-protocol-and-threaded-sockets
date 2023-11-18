#include "client.h"

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
    sockfd = createSocket();

    // Create client request
    struct BlogOperation operationToSendByClient = createOperation(0, NEW_CONNECTION, 0, "", "");

    // Send connection request to server
    size_t size = send(sockfd, &operationToSendByClient, sizeof(operationToSendByClient), 0);
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
            messageDisconnect();
            operationToSend = createOperation(operationReceivedByServer.client_id, DISCONNECT, 0, "", "");
            break;

        default:
            break;
        }

        // Send request to server
        if (commandType != INVALID)
        {
            size = send(sockfd, &operationToSend, sizeof(operationToSend), 0);
            if (size != sizeof(operationToSend))
            {
                logexit("send");
            }
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
            if (strcmp(command, "list topics") == 0)
            {
                return LIST_TOPICS;
            }
            if (strcmp(command, "subscribe") == 0 || strcmp(command, "unsubscribe") == 0)
            {
                char *keyword = strtok(NULL, " "); // Get the topic
                if (keyword == NULL)
                {
                    return INVALID;
                }
                return (strcmp(command, "subscribe") == 0) ? SUBSCRIBE_IN_TOPIC : UNSUBSCRIBE_IN_TOPIC;
            }
            if (strcmp(command, "publish") == 0)
            {
                char *keyword = strtok(NULL, " ");
                if (keyword == NULL)
                {
                    return INVALID;
                }
                return NEW_POST_IN_TOPIC;
            }
            return INVALID;
        }

        return INVALID;
    }
}

// Get the content of the command
char *getTopic(int cmd, char *cmdLine)
{
    char *theTopic = malloc(sizeof(char) * 2048);
    for (int i = 0; i < sizeof(commands) / sizeof(struct Command); i++)
    {
        if (cmdLine == commands[i].name)
        {
            switch (cmd)
            {
            case NEW_POST_IN_TOPIC:
                strcpy(theTopic, cmdLine + strlen(commands[i].name) + 1);
                return theTopic;

            case SUBSCRIBE_IN_TOPIC:
                strcpy(theTopic, cmdLine + strlen(commands[i].name) + 1);
                return theTopic;

            case UNSUBSCRIBE_IN_TOPIC:
                strcpy(theTopic, cmdLine + strlen(commands[i].name) + 1);
                return theTopic;

            default:
                return NULL;
            }
            return NULL;
        }
    }
}

void messageDisconnect()
{
    printf("exit\n");
}

void* waitingFunction(void* sock) {
    int* sockfd = (int*)sock;
    struct BlogOperation operationReceivedByServer;
    while (1)
    {
        // Receive response from server
        size_t size = receive_all(*sockfd, &operationReceivedByServer, sizeof(operationReceivedByServer));
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

        case DISCONNECT:
            printf("%s\n", operationReceivedByServer.content);
            close(*sockfd);
            break;

        default:
            break;
        }

    }
}
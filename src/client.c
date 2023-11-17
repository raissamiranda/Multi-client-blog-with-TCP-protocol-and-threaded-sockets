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
    int sockfd = createSocket();

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

        fgets(command, sizeof(command), stdin);
        int commandType = handleCommand(command);

        if (commandType == INVALID)
        {
            printf("Invalid command\n");
            continue;
        }

        switch(commandType) {
            case NEW_POST_IN_TOPIC:
                break;
            case LIST_TOPICS:
                break;
            case SUBSCRIBE_IN_TOPIC:
                break;
            case UNSUBSCRIBE_IN_TOPIC:
                break;
            case DISCONNECT:
                break;
            default:
                break;
        }
    }

    return 0;
}

void waitingFunction()
{
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
                char *inKeyword = strtok(NULL, " ");
                if (inKeyword == NULL || !(strcmp(inKeyword, "in") == 0 || strcmp(inKeyword, "to") == 0))
                {
                    return INVALID;
                }
                return (strcmp(command, "subscribe") == 0) ? SUBSCRIBE_IN_TOPIC : UNSUBSCRIBE_IN_TOPIC;
            }
            if (strcmp(command, "publish") == 0)
            {
                char *inKeyword = strtok(NULL, " ");
                if (inKeyword == NULL || strcmp(inKeyword, "in") != 0)
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
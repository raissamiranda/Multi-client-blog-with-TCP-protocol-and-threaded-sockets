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

    return 0;
}

void waitingFunction()
{
}
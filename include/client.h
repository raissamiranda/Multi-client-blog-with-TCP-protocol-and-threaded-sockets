#include "common.h"

pthread_t clientThread;
char command[2048];
char *topic = " ";
char *content = " ";

// Structure to store valid commands
struct Command {
  const char *name;
  int returnValue;
};

// Array of valid commands
struct Command commands[] = {{"exit", DISCONNECT},
                             {"list", LIST_TOPICS},
                             {"subscribe", SUBSCRIBE_IN_TOPIC},
                             {"unsubscribe", UNSUBSCRIBE_IN_TOPIC},
                             {"publish", NEW_POST_IN_TOPIC}};

// Function prototypes for client
int handleCommand(char *input);
char *getTopic(int cmd, char *cmdLine);
void messageDisconnect();
void *waitingFunction(void *sock);

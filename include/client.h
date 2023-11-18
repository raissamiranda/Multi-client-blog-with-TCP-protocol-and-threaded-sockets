#include "common.h"

pthread_t clientThread;
char command[2048];
char *topic = " ";
char *content = " ";

struct Command {
    const char *name;
    int returnValue;
};


struct Command commands[] = {
    {"exit", DISCONNECT},
    {"list topics", LIST_TOPICS},
    {"subscribe", SUBSCRIBE_IN_TOPIC},
    {"unsubscribe", UNSUBSCRIBE_IN_TOPIC},
    {"publish", NEW_POST_IN_TOPIC}
};

int handleCommand(char *input);
char *getTopic(int cmd, char *cmdLine);
void messageDisconnect();
void* waitingFunction(void* sock);
#include "common.h"

struct Blog blog;

struct Client {
    int id;
    int socket;
    pthread_t thread;
};

struct Topic {
    int id;
    char title[50];
    char posts[100];
    int postAuthorsID[100];
    int postCount;
    struct Client subscribers[100];
    int subscribersCount;
};

// It's a newly created blog with no topics and no clients
struct Blog {
    struct Topic topics[100];
    struct Client clients[10];
    int clientsCount;
    int topicsCount;
};
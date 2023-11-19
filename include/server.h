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
  char posts[10][2048];
  int postAuthorsID[100];
  int postCount;
  struct Client subscribers[10];
  int subscribersCount;
};

// It's a newly created blog with no topics and no clients
struct Blog {
  struct Topic topics[100];
  struct Client clients[10];
  int clientsCount;
  int topicsCount;
};

int createSocket();
void addUserToBlog(struct Client client);
void messageClientConnected(struct Client client);
void waitForThreads(struct Blog blog);
void *function(void *thread);
struct BlogOperation
createOperationToSend(struct BlogOperation operationRequestedByClient);
bool hasTopics();
void listTopics(char *topics);
int findTopic(char topic[50]);
void createTopic(char topic[50]);
void addPostInTopic(struct BlogOperation operationRequestedByClient,
                    int topicIndex);
void messageNewPostInTopic(int topicIndex);
bool isSubscribed(int clientID, int topicIndex);
void subscribeClientInTopic(struct Client client, int topicIndex);
void unsubscribeClientInTopic(int clientID, int topicIndex);
void disconnectClient(int clientID);
void messageClientDisconnected(struct Client client);
struct Client *findClientByID(int clientID);
void printCurrentBlog();

# Multi-client blog with TCP protocol and threaded sockets

## About the project
This system implements an interactive blog using a client-server architecture.
It accommodates multiple clients, each empowered to perform various actions within the blog.
Clients can create new topics, subscribe to receive notifications for posts in their preferred topics and unsubscribe them and others functionalities.
The client-server model ensures a seamless flow of communication, allowing users to actively participate in the dynamic environment of the blog, fostering engagement and collaboration.

## Instructions for execution
To install the program, it is necessary to navigate to the directory where the program is stored:
```
cd <destinationDirectory>
```

Compile the program using make command:
```
make
```
Open a terminal for the server and specify the address type to be used (v4 or v6) and the port number in the format:
It's recommended to use port 51511.
```
./bin/server <addressType> <portNumber>
```

Open a new terminal for each clients you need and specify the serverIP and the same port number:
```
./bin/client <serverIP> <portNumber>
```

Connection established. Any command can be sent by clients now!

## Valid commands
Clients can send the following commands:

### Publish new post in topic
Adds a post to the specified topic. If the topic doesn't exist, it is created, and the post is then added.
> publish in <topicName>

### Subscribe in a topic
User is now subscribed to the specified topic and will receive notifications of new posts. If the topic doesn't exist, it is created, and the user is then subscribed.
> subscribe <topicName>

### Unsubscribe in a topic
This user unsubscribes from the specified topic and will no longer receive notifications of new posts.
> unsubscribe <topicName>

### List all existing topics
Prints a list of all topics existing in the blog.
> list topics

### Disconnect
This user will be disconnected from the blog.
> exit


CC = gcc
BIN = bin
OBJ = obj
SRC = src
INC = include
CFLAGS = -Wall -g -pthread -I$(INC)
HDRS = $(INC)/common.h $(INC)/client.h $(INC)/server.h

all: $(BIN)/server $(BIN)/client

$(BIN)/server: $(HDRS) $(OBJ)/server.o $(OBJ)/common.o | $(BIN)
	$(CC) $(CFLAGS) $(OBJ)/server.o $(OBJ)/common.o -o $(BIN)/server

$(OBJ)/server.o: $(HDRS) $(SRC)/server.c | $(OBJ)
	$(CC) $(SRC)/server.c $(CFLAGS) -c  -o $(OBJ)/server.o

$(BIN)/client: $(HDRS) $(OBJ)/client.o $(OBJ)/common.o | $(BIN)
	$(CC) $(CFLAGS) $(OBJ)/client.o $(OBJ)/common.o -o $(BIN)/client

$(OBJ)/client.o: $(HDRS) $(SRC)/client.c | $(OBJ)
	$(CC) $(SRC)/client.c $(CFLAGS) -c -o $(OBJ)/client.o

$(OBJ)/common.o: $(HDRS) $(SRC)/common.c | $(OBJ)
	$(CC) $(SRC)/common.c $(CFLAGS) -c -o $(OBJ)/common.o

$(OBJ):
	mkdir $(OBJ)

$(BIN):
	mkdir $(BIN)

clean:
	rm -rf $(BIN) $(OBJ)
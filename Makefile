# Compiler and flags
CC = gcc
CFLAGS = -I$(INCLUDE_DIR) -Wall -Wextra -g
LDFLAGS = -lpthread

# Directories
SRC_DIR = src
OBJ_DIR = obj
INCLUDE_DIR = include
BIN_DIR = bin

# Target executables
CLIENT_EXEC = $(BIN_DIR)/client
SERVER_EXEC = $(BIN_DIR)/server

# Source files
CLIENT_SRC = $(SRC_DIR)/client.c
SERVER_SRC = $(SRC_DIR)/server.c

# Header files
HEADERS = $(INCLUDE_DIR)/common.h

# Object files
CLIENT_OBJ = $(OBJ_DIR)/client.o
SERVER_OBJ = $(OBJ_DIR)/server.o

# Default target: Build both executables
all: $(CLIENT_EXEC) $(SERVER_EXEC)

# Build client executable
$(CLIENT_EXEC): $(CLIENT_OBJ) | $(BIN_DIR)
	$(CC) -o $@ $^ $(LDFLAGS)

# Build server executable
$(SERVER_EXEC): $(SERVER_OBJ) | $(BIN_DIR)
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile client object file
$(CLIENT_OBJ): $(CLIENT_SRC) $(HEADERS) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile server object file
$(SERVER_OBJ): $(SERVER_SRC) $(HEADERS) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create directories if they don't exist
$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

# Clean up object and binary files
clean:
	rm -f $(OBJ_DIR)/*.o
	rm -f $(BIN_DIR)/*
	@echo "Cleaning Done!"

# Phony targets to avoid conflicts
.PHONY: all clean run-client run-server

# Run the client executable
run-client: $(CLIENT_EXEC)
	@echo "Running Client..."
	$(CLIENT_EXEC)

# Run the server executable
run-server: $(SERVER_EXEC)
	@echo "Running Server..."
	$(SERVER_EXEC)


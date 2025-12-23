CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lws2_32

# Directories
SRC_DIR = src
SERVER_DIR = $(SRC_DIR)/server
THREADS_DIR = $(SRC_DIR)/threads
BUILD_DIR = build

# Source files
SRCS = $(SRC_DIR)/main.c \
       $(SERVER_DIR)/server.c \
       $(THREADS_DIR)/threads.c \
       $(SRC_DIR)/utils.c \
       $(SRC_DIR)/client/client.c \
	   $(SRC_DIR)/client/client_request.c \
	   $(SRC_DIR)/cache/cache.c

# Object files in build/
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)

TARGET = nomad.exe

# Default target
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# build/%.o depends on %.c
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
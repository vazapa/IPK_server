CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = ipk24chat-server
SRCS = main.c

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = ipk24chat-server
SRCS = main.c server.c udp.c

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

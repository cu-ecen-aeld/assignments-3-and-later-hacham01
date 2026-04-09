CC ?= gcc
CFLAGS = -Wall -Wextra -g
TARGET = aesdsocket
SOURCE = aesdsocket.c

all: $(TARGET)

default: all

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

clean:
	rm -f $(TARGET)

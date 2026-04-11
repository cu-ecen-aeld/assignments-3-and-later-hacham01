# Use Buildroot's cross-compiler
CC ?= $(CROSS_COMPILE)gcc

CFLAGS ?= -Wall -Wextra -O2
TARGET = aesdsocket
SOURCE = aesdsocket.c

all: $(TARGET)

default: all

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

clean:
	rm -f $(TARGET)


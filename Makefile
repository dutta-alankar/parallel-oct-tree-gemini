CC = mpicc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS =
TARGET = octree_parallel

.PHONY: all clean

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o *.txt *.png

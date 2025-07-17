CC = mpicc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS =
TARGET_OCTREE = octree_parallel
TARGET_QUADTREE = quadtree_parallel

.PHONY: all clean

all: $(TARGET_OCTREE) $(TARGET_QUADTREE)

$(TARGET_OCTREE): main.c
	$(CC) $(CFLAGS) -o $(TARGET_OCTREE) main.c $(LDFLAGS)

$(TARGET_QUADTREE): quadtree.c
	$(CC) $(CFLAGS) -o $(TARGET_QUADTREE) quadtree.c $(LDFLAGS)

clean:
	rm -f $(TARGET_OCTREE) $(TARGET_QUADTREE) *.o *.txt *.png

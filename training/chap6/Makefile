TARGET := soloFlyQueue queueTest \
	soloFlyQueue2 queueTest2

CFLAGS := $(CFLAGS) -g -pthread
LDFLAGS := $(LDFLAGS) -pthread

all:

soloFlyQueue: soloFlyQueue.o xyqueue.o
	$(CC) $(LDFLAGS) $^ -lm -o $@

queueTest: queueTest.o xyqueue.o
	$(CC) $(LDFLAGS) $^ -o $@

soloFlyQueue2: soloFlyQueue.o xyqueue2.o
	$(CC) $(LDFLAGS) $^ -lm -o $@

queueTest2: queueTest.o xyqueue2.o
	$(CC) $(LDFLAGS) $^ -o $@

clean:
	rm -f $(TARGET) *.o

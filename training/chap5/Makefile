TARGET := soloFly soloFly2 soloFly3\
	condWait condTimedWait multiCondWait multiCondWait2\
	fly5 fly6\
	starvation

CFLAGS := $(CFLAGS) -g -pthread
LDFLAGS := $(LDFLAGS) -pthread

all:

soloFly: soloFly.o
	$(CC) $(LDFLAGS) $^ -lm -o $@

soloFly2: soloFly2.o
	$(CC) $(LDFLAGS) $^ -lm -o $@

soloFly3: soloFly3.o
	$(CC) $(LDFLAGS) $^ -lm -o $@

condWait: condWait.o
	$(CC) $(LDFLAGS) $^ -o $@

condTimedWait: condTimedWait.o
	$(CC) $(LDFLAGS) $^ -o $@

multiCondWait: multiCondWait.o
	$(CC) $(LDFLAGS) $^ -o $@

multiCondWait2: multiCondWait2.o
	$(CC) $(LDFLAGS) $^ -o $@

fly5: fly5.o
	$(CC) $(LDFLAGS) $^ -lm -o $@

fly6: fly6.o
	$(CC) $(LDFLAGS) $^ -lm -o $@

starvation: starvation.o
	$(CC) $(LDFLAGS) $^ -o $@

clean:
	rm -f $(TARGET) *.o

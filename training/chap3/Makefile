TARGET := variables variables3 variables4 variables5\
	variablesProc fdProcess fdThread\
	maxData maxData2 maxData3

CFLAGS := $(CFLAGS) -pthread
LDFLAGS := $(LDFLAGS) -pthread

all:

variables: variables.o
	$(CC) $(LDFLAGS) $^ -o $@

variables3: variables3.o
	$(CC) $(LDFLAGS) $^ -o $@

variables4: variables4.o
	$(CC) $(LDFLAGS) $^ -o $@

variables5: variables5.o
	$(CC) $(LDFLAGS) $^ -o $@

variablesProc: variablesProc.o
	$(CC) $(LDFLAGS) $^ -o $@

fdProcess: fdProcess.o
	$(CC) $(LDFLAGS) $^ -o $@

fdThread: fdThread.o
	$(CC) $(LDFLAGS) $^ -o $@

clean:
	rm -f $(TARGET) *.o

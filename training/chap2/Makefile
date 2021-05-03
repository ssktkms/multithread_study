TARGET := firstThread firstThread2 firstThread3 firstThread4 threadAttr

CFLAGS := $(CFLAGS) -pthread
LDFLAGS := $(LDFLAGS) -pthread

all:

firstThread: firstThread.o
	$(CC) $(LDFLAGS) $^ -o $@

firstThread2: firstThread2.o
	$(CC) $(LDFLAGS) $^ -o $@

firstThread3: firstThread3.o
	$(CC) $(LDFLAGS) $^ -o $@

threadAttr: threadAttr.o
	$(CC) $(LDFLAGS) $^ -o $@

maxData: maxData.o
	$(CC) $(LDFLAGS) $^ -o $@

maxData2: maxData2.o
	$(CC) $(LDFLAGS) $^ -o $@

maxData3: maxData3.o
	$(CC) $(LDFLAGS) $^ -o $@

clean:
	rm -f $(TARGET) *.o

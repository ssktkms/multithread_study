TARGET := postal socketPostal socketPostal2 socketPostal3 tnc

CFLAGS := $(CFLAGS) -pthread
LDFLAGS := $(LDFLAGS) -pthread

all: $(TARGET)

postal: postal.o postalNumber.o
	$(CC) $(LDFLAGS) $^ -o $@

socketPostal: socketPostal.o postalNumber.o
	$(CC) $(LDFLAGS) $^ -o $@

socketPostal2: socketPostal2.o postalNumber.o
	$(CC) $(LDFLAGS) $^ -o $@

socketPostal3: socketPostal3.o postalNumber.o intqueue.o
	$(CC) $(LDFLAGS) $^ -o $@

tnc: tnc.o
	$(CC) $^ -o $@

clean:
	rm -f $(TARGET) *.o

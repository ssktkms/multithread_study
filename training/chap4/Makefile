TARGET := primeNumber primeNumber2 primeNumber3 primeNumber4 primeNumber5\
	fastPrimeNumber fastPrimeNumber2\
	fly fly2 fly3 fly4 fly2rwlock

CFLAGS := $(CFLAGS) -g -pthread
LDFLAGS := $(LDFLAGS) -pthread

all:

primeNumber: primeNumber.o
	$(CC) $(LDFLAGS) $^ -o $@

primeNumber2: primeNumber2.o
	$(CC) $(LDFLAGS) $^ -o $@

primeNumber3: primeNumber3.o
	$(CC) $(LDFLAGS) $^ -o $@

primeNumber4: primeNumber4.o
	$(CC) $(LDFLAGS) $^ -o $@

primeNumber5: primeNumber5.o
	$(CC) $(LDFLAGS) $^ -o $@

fastPrimeNumber: fastPrimeNumber.o
	$(CC) $(LDFLAGS) $^ -o $@

fastPrimeNumber2: fastPrimeNumber2.o
	$(CC) $(LDFLAGS) $^ -o $@

fly: fly.o
	$(CC) $(LDFLAGS) $^ -lm -o $@

fly2: fly2.o
	$(CC) $(LDFLAGS) $^ -lm -o $@

fly3: fly3.o
	$(CC) $(LDFLAGS) $^ -lm -o $@

fly4: fly4.o
	$(CC) $(LDFLAGS) $^ -lm -o $@

fly2rwlock: fly2rwlock.o
	$(CC) $(LDFLAGS) $^ -lm -o $@

clean:
	rm -f $(TARGET) *.o

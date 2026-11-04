CC     = gcc
CFLAGS = -Wall -Wextra -O2

all: jitter percentiles deadline

jitter: jitter.c
	$(CC) $(CFLAGS) -o jitter jitter.c

percentiles: percentiles.c stats.c stats.h
	$(CC) $(CFLAGS) -o percentiles percentiles.c stats.c -lm

deadline: deadline.c stats.c stats.h
	$(CC) $(CFLAGS) -o deadline deadline.c stats.c -lm

run: jitter
	./jitter

test: test_stats.c stats.c stats.h
	$(CC) $(CFLAGS) -o test_stats test_stats.c stats.c -lm
	./test_stats

clean:
	rm -f jitter percentiles deadline test_stats *.csv

.PHONY: all run test clean

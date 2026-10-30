CC     = gcc
CFLAGS = -Wall -Wextra -O2

all: jitter

jitter: jitter.c
	$(CC) $(CFLAGS) -o jitter jitter.c

run: jitter
	./jitter

clean:
	rm -f jitter latences.csv

.PHONY: all run clean

CC=g++
CFLAGS=-I/opt/napatech3/include
LDFLAGS=-lntapi -L/opt/napatech3/lib

dedupe: Deduplicator.cc
	$(CC) -std=c++11 $(CFLAGS) Deduplicator.cc -o dedupe $(LDFLAGS)

all: dedupe
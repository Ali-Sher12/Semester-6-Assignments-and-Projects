# Makefile - Assignment 3 PageRank (23i-0683)
# CS 3006 - Parallel and Distributive Computing

CC     = mpicc
CFLAGS = -O2 -Wall
LIBS   = -lm

TARGETS = pagerank_s1 pagerank_s2 pagerank_s3

all: $(TARGETS)

pagerank_s1: 23i-0683_F_A3_S1.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

pagerank_s2: 23i-0683_F_A3_S2.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

pagerank_s3: 23i-0683_F_A3_S3.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

run1: pagerank_s1
	mpirun -np 4 ./pagerank_s1 web-Google.txt

run2: pagerank_s2
	mpirun -np 4 ./pagerank_s2 web-Google.txt

run3: pagerank_s3
	mpirun -np 4 ./pagerank_s3 web-Google.txt

run: all
	@echo "=== Scenario 1: P2P Blocking ==="
	mpirun -np 4 ./pagerank_s1 web-Google.txt
	@echo ""
	@echo "=== Scenario 2: Collective ==="
	mpirun -np 4 ./pagerank_s2 web-Google.txt
	@echo ""
	@echo "=== Scenario 3: Async Overlap ==="
	mpirun -np 4 ./pagerank_s3 web-Google.txt

clean:
	rm -f $(TARGETS)

.PHONY: all clean run1 run2 run3 run

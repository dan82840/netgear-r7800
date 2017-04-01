#
#	Copyright (C) 2014 Delta Networks Inc.
#

CC = gcc


CFLAGS += -Wall -Wunused -g -O2

all: sb_set_priority poll_set_priority soap_flowman_nodes

sb_set_priority: sb_set_priority.o
	$(CC) $(CFLAGS) -o $@ $^
	@echo "make $@ finished on `date`"

poll_set_priority: poll_set_priority.o
	$(CC) $(CFLAGS) -o $@ $^ -lconfig
	@echo "make $@ finished on `date`"

soap_flowman_nodes: soap_flowman_nodes.o
	$(CC) $(CFLAGS) -o $@ $^
	@echo "make $@ finished on `date`"

%.o : %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o 
	rm -f sb_set_priority poll_set_priority soap_flowman_nodes

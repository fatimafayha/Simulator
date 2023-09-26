CC=gcc
TARGET=sim_cache
DEPS=simulator.h

all: simulator.c $(DEPS)
	$(CC) -g simulator.c -lm -o $(TARGET)
clean:
	rm -f $(TARGET)
	

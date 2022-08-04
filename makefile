OBJS	= main.o
SOURCE	= main.cpp
HEADER	= main.h
OUT	= ex3.out
CC	 = g++
FLAGS	 = -g -c -Wall
LFLAGS	 = -lpthread

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

main.o: main.cpp
	$(CC) $(FLAGS) main.cpp -std=c++11


clean:
	rm -f $(OBJS) $(OUT)
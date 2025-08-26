CC = g++
LIBS = -std=c++11 -O3
OBJS = main.o
hw2: $(OBJS)
	$(CC) $(OBJS) -o ../bin/hw2
main.o: main.cpp lib.h
	$(CC) -c main.cpp $(LIBS)
clean:
	rm *.o
	rm -f ../bin/hw2
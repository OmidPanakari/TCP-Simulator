CC = g++
FLAGS = -std=c++17 -Wall
RM = rm
SRC = Src/




all: Host.out Router.out


Host.out: Host.o Frame.o
	$(CC) $(FLAGS) $(SRC)Host.o $(SRC)Frame.o -o $(SRC)Host.out

Router.out: Router.o Frame.o
	$(CC) $(FLAGS) $(SRC)Router.o $(SRC)Frame.o -o $(SRC)Router.out

Host.o: $(SRC)Host.cpp $(SRC)Frame.hpp
	$(CC) $(FLAGS) $(SRC)Host.cpp -c -o $(SRC)Host.o

Router.o: $(SRC)Router.cpp $(SRC)Frame.hpp
	$(CC) $(FLAGS) $(SRC)Router.cpp -c -o $(SRC)Router.o

Frame.o: Frame.cpp Frame.hpp
	$(CC) $(FLAGS) $(SRC)Frame.cpp -c




.PHONY: clean
clean:
	$(RM) *.out	*.o
all: collatz

collatz: collatz.cpp
	g++ collatz.cpp -std=c++17 -O3 -march=native -lpthread -Wall -Wextra -o collatz

clean:
	rm collatz

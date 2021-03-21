all:
	@g++ assignment3.cpp dram.cpp -I . -o assignment3 -std=c++11 -g
clean:
	@rm assignment3
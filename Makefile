all:
	@g++ assignment4.cpp dram.cpp mrm.cpp -I . -o assignment4 -std=c++11 -g
clean:
	@rm assignment4
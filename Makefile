all:
	@g++ assignment3.cpp -o assignment3 -std=c++11
	@./assignment3
clean:
	@rm assignment3
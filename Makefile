all:
	@g++ mips_pms.cpp dram.cpp mrm.cpp -I . -o mips_pms -std=c++11 -g
clean:
	@rm mips_pms
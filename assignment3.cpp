#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <string.h>
#include <boost/algorithm/string/replace.hpp>

using namespace std;

class IntRegister
{
public:
	int content;
	string name;

	IntRegister()
	{
		content = 0;
	}
	IntRegister(string n)
	{
		content = 0;
		name = n;
	}
	void setName(string n)
	{
		name = n;
	}
	void setContent(int n)
	{
		content = n;
	}
	string getHex()
	{
		stringstream stream;
		stream << "0x"
			   << hex << content;
		return stream.str();
	}
};

class MIPS
{
private:
	IntRegister registers[32];

	int clockCycles;
	int instructionsExecuted;
	int instructionIndex;
	int no_exec_instructions[9];

public:
	int Memory[262144];
	// Instruction mapping
	static string instructions[10];
	int maxArguments;

public:
	MIPS()
	{

		maxArguments = 4;
		clockCycles = 0;

		registers[0].setName("zero");
		registers[0].setContent(0);
		registers[1].setName("at");
		registers[2].setName("v0");
		registers[3].setName("v1");
		registers[4].setName("a0");
		registers[5].setName("a1");
		registers[6].setName("a2");
		registers[7].setName("a3");
		registers[8].setName("t0");
		registers[9].setName("t1");
		registers[10].setName("t2");
		registers[11].setName("t3");
		registers[12].setName("t4");
		registers[13].setName("t5");
		registers[14].setName("t6");
		registers[15].setName("t7");
		registers[16].setName("s0");
		registers[17].setName("s1");
		registers[18].setName("s2");
		registers[19].setName("s3");
		registers[20].setName("s4");
		registers[21].setName("s5");
		registers[22].setName("s6");
		registers[23].setName("s7");
		registers[24].setName("t8");
		registers[25].setName("t9");
		registers[26].setName("k0");
		registers[27].setName("k1");
		registers[28].setName("gp");
		registers[29].setName("sp");
		registers[30].setName("fp");
		registers[31].setName("ra");
	}

	void setMemory(int loc, int val)
	{
		Memory[loc] = val;
	}

	void throwError(string argument, int ErrorType)
	{
		// ErrorType:
		// 0 means no error
		// 1 means invalid register being used
		// 2 means overflow has occurred
		// 3 means the instruction has extra arguments
		// 4 means the instruction is not implemented
		// 5 means the instruction has too few arguments
		// 6 means there was no closing bracket in the statement
		// 7 for invalid instruction address
		if (ErrorType == 0)
		{
			return;
		}
		else if (ErrorType == 1)
		{
			cout << "Error: Register " << argument << " not found!" << '\n';
		}
		else if (ErrorType == 2)
		{
			cout << "Error: Memory address " << argument << " not found!" << '\n';
		}
		else if (ErrorType == 3)
		{
			cout << "Error: The instruction: " << argument << " has more arguments than allowed!" << '\n';
		}
		else if (ErrorType == 4)
		{
			cout << "Error: The instruction" << argument << "is either undefined or out of scope of this assignment.\n";
			cout << "Kindly use add, sub, mul, beq, bne, slt, j, lw, sw, addi instructions only\n";
		}
		else if (ErrorType == 5)
		{
			cout << "Error: The instruction: " << argument << " has fewer arguments than required!" << '\n';
		}
		else if (ErrorType == 6)
		{
			cout << "Error: The instruction: " << argument << " is missing a closing parentheses" << '\n';
		}
		else if (ErrorType == 7)
		{
			cout << "Error: The address: " << argument << " is not a valid instruction address" << '\n';
		}
		exit(1);
	}

	int getIntRegister(char *name)
	{
		for (int i = 0; i < 32; i++)
		{
			if (registers[i].name.compare(name) == 0)
			{
				return i;
			}
		}
		throwError(name, 1);
		return -1;
	}

	void printRegisterContents()
	{
		//  		cout << "Reg no.\tReg name"<<"\t"<<"Content" << '\n';
		// for(int i = 0; i<32;i++){
		// 	cout <<i<<'\t'<< registers[i].name<<"\t\t"<<registers[i].getHex() << '\n';
		// }
		cout << setw(6) << "Reg no." << setw(10) << "Reg name" << setw(10) << "Content" << '\n';
		for (int i = 0; i < 32; i++)
		{
			cout << setw(6) << i << setw(10) << registers[i].name << setw(10) << registers[i].getHex() << '\n';
		}
		// setw(4)
	}
	void lw(int a, int b, int c)
	{
		if (a > 31 || a < 0)
		{
			throwError(to_string(a), 1);
		}
		if (b > 31 || b < 0)
		{
			throwError(to_string(b), 1);
		}
		int addr = registers[b].content + c;
		if (addr > sizeof(Memory) / sizeof(*Memory) || addr < instructionIndex)
		{
			throwError(to_string(addr), 2);
		}
		registers[a].setContent(Memory[addr]);
		clockCycles += 1;
		instructionsExecuted += 1;
	}
	void sw(int a, int b, int c)
	{
		if (a > 31 || a < 0)
		{
			throwError(to_string(a), 1);
		}
		if (b > 31 || b < 0)
		{
			throwError(to_string(b), 1);
		}
		int addr = registers[b].content + c;
		if (addr > sizeof(Memory) / sizeof(*Memory) || addr < instructionIndex)
		{
			throwError(to_string(addr), 2);
		}
		Memory[addr] = registers[a].content;
		clockCycles += 1;
		instructionsExecuted += 1;
	}
	void add(int a, int b, int c)
	{
		if (a > 31 || a < 0)
		{
			throwError(to_string(a), 1);
		}
		if (b > 31 || b < 0)
		{
			throwError(to_string(b), 1);
		}
		if (c > 31 || c < 0)
		{
			throwError(to_string(c), 1);
		}
		registers[a].setContent(registers[b].content + registers[c].content);
		clockCycles += 1;
		instructionsExecuted += 1;
	}
	void sub(int a, int b, int c)
	{
		if (a > 31 || a < 0)
		{
			throwError(to_string(a), 1);
		}
		if (b > 31 || b < 0)
		{
			throwError(to_string(b), 1);
		}
		if (c > 31 || c < 0)
		{
			throwError(to_string(c), 1);
		}
		registers[a].setContent(registers[b].content - registers[c].content);
		clockCycles += 1;
		instructionsExecuted += 1;
	}
	void mul(int a, int b, int c)
	{
		if (a > 31 || a < 0)
		{
			throwError(to_string(a), 1);
		}
		if (b > 31 || b < 0)
		{
			throwError(to_string(b), 1);
		}
		if (c > 31 || c < 0)
		{
			throwError(to_string(c), 1);
		}
		registers[a].setContent(registers[b].content * registers[c].content);
		clockCycles += 1;
		instructionsExecuted += 1;
	}
	void addi(int a, int b, int c)
	{
		if (a > 31 || a < 0)
		{
			throwError(to_string(a), 1);
		}
		if (b > 31 || b < 0)
		{
			throwError(to_string(b), 1);
		}
		registers[a].setContent(registers[b].content + c);
		clockCycles += 1;
		instructionsExecuted += 1;
	}
	bool beq(int a, int b)
	{
		bool toJump = false;
		if (a > 31 || a < 0)
		{
			throwError(to_string(a), 1);
		}
		if (b > 31 || b < 0)
		{
			throwError(to_string(b), 1);
		}
		if (registers[a].content == registers[b].content)
		{
			toJump = true;
		}
		clockCycles += 1;
		instructionsExecuted += 1;
		return toJump;
	}
	bool bne(int a, int b)
	{
		bool toJump = true;
		if (a > 31 || a < 0)
		{
			throwError(to_string(a), 1);
		}
		if (b > 31 || b < 0)
		{
			throwError(to_string(b), 1);
		}
		if (registers[a].content == registers[b].content)
		{
			toJump = false;
		}
		clockCycles += 1;
		instructionsExecuted += 1;
		return toJump;
	}
	void slt(int a, int b, int c)
	{
		if (registers[b].content < registers[c].content)
		{
			registers[a].setContent(1);
		}
		else
		{
			registers[a].setContent(0);
		}
	}
	string preprocessRegisters(string str)
	{

		return str;
	}
	void printStatistics()
	{
		cout << "===========================================";
		cout << "\nNumber of clock cycles:\t\t\t " << clockCycles;
		cout << "\nNumber of instructions executed:\t " << clockCycles;
		if (instructionsExecuted != 0)
		{
			cout << "\nAverage clock cycles per instruction:\t " << clockCycles / instructionsExecuted;
		}
		cout << "\n===========================================\n";
		// printing number of times each instruction was executed
		cout << setw(10) << "Instruction" << setw(18) << "Executed\n";
		for (int i = 0; i < 10; i++)
		{
			cout << setw(10) << instructions[i] << setw(9) << no_exec_instructions[i] << setw(10) << " time(s)\n";
		}
	}

	void encode(string command, string arguments[], int maxArguments, int instructionIndex)
	{
		auto itr = find(instructions, instructions + (sizeof(instructions) / sizeof(*instructions)), command);
		if (itr == end(instructions))
		{
			throwError(command, 4);
		}
		int commandCode = distance(instructions, itr);
		if (commandCode == 0 || commandCode == 1)
		{
			// lw and sw instructions
			int registerNo, baseRegister, offset;
			if (arguments[0] == "none" || arguments[1] == "none")
			{
				throwError(command, 5);
			}
			for (int j = 2; j < maxArguments; j++)
			{
				if (arguments[j] != "none")
				{
					throwError(command, 3);
				}
			}
			registerNo = stoi(arguments[0]);
			string secondArg = "";
			string thirdArg = "";
			int i = 0;
			while (arguments[1][i] != "(" && i < arguments[1].size())
			{
				secondArg += arguments[1][i];
				i++;
			}
			if (i == arguments[1].size())
			{
				//Then there is no base register given
				baseRegister = 0;
				offset = stoi(arguments[1]);
			}
			else
			{
				int j = i + 1;
				while (arguments[1][j] != ")" && j < arguments[1].size())
				{
					thirdArg += arguments[1][j];
					j++;
				}
				if (j == arguments[1].size())
				{
					throwError(command, 6);
				}
				baseRegister = stoi(thirdArg);
				offset = stoi(secondArg);
			}
			Memory[instructionIndex + 1] = registerNo;
			Memory[instructionIndex + 2] = baseRegister;
			Memory[instructionIndex + 3] = offset;
		}
		else if (commandCode == 7 || commandCode == 8)
		{
			// beq or bne command
			int reg1, reg2, targetAddress;
			for (int k = 0; k < 3; k++)
			{
				if (arguments[k] == "none")
				{
					throwError(command, 5);
				}
			}
			for (int j = 3; j < maxArguments; j++)
			{
				if (arguments[j] != "none")
				{
					throwError(command, 3);
				}
			}
			reg1 = stoi(arguments[1]);
			reg2 = stoi(arguments[2]);
			targetAddress = stoi(arguments[3]);
			Memory[instructionIndex + 1] = reg1;
			Memory[instructionIndex + 2] = reg2;
			Memory[instructionIndex + 3] = targetAddress;
		}
		else if (commandCode == 5)
		{
			// addi instruction
			int reg1, reg2, imValue;
			for (int k = 0; k < 3; k++)
			{
				if (arguments[k] == "none")
				{
					throwError(command, 5);
				}
			}
			for (int j = 3; j < maxArguments; j++)
			{
				if (arguments[j] != "none")
				{
					throwError(command, 3);
				}
			}
			reg1 = stoi(arguments[0]);
			reg2 = stoi(arguments[1]);
			imValue = stoi(arguments[2]);
			Memory[instructionIndex + 1] = reg1;
			Memory[instructionIndex + 2] = reg2;
			Memory[instructionIndex + 3] = imValue;
		}
		else if (commandCode == 6)
		{
			// j instruction
			int jumpAddr;
			if (arguments[0] == "none")
			{
				throwError(command, 5);
			}
			for (int j = 1; j < maxArguments; j++)
			{
				if (arguments[j] != "none")
				{
					throwError(command, 3);
				}
			}
			jumpAddr = stoi(arguments[0]);
			Memory[instructionIndex + 1] = jumpAddr;
		}
		else
		{
			// add, sub, mul, slt
			int reg1, reg2, reg3;
			for (int k = 0; k < 3; k++)
			{
				if (arguments[k] == "none")
				{
					throwError(command, 5);
				}
			}
			for (int j = 3; j < maxArguments; j++)
			{
				if (arguments[j] != "none")
				{
					throwError(command, 3);
				}
			}
			reg1 = stoi(arguments[0]);
			reg2 = stoi(arguments[1]);
			reg3 = stoi(arguments[2]);

			Memory[instructionIndex + 1] = reg1;
			Memory[instructionIndex + 2] = reg2;
			Memory[instructionIndex + 3] = reg3;
		}

		Memory[instructionIndex] = commandCode;
	}
	void decode(int index, int *instructDecoded)
	{
		// add everything decoded in instructDecoded.
		// instructDecoded[0] contains command, and other elements contain arguments
		int commandCode = Memory[index];
		instructDecoded[0] = commandCode;
		switch (commandCode)
		{
		case 6:
			instructDecoded[1] = Memory[index + 1];
		default:
			for (int k = 1; k < 4; k++)
			{
				instructDecoded[k] = Memory[index + k];
			}
		}
	}

	void readInstructions(string filename)
	{
		// read input from file, store instructions in memory
		ifstream infile;
		infile.open(filename);
		string str;
		instructionIndex = 0;
		// const int maxArguments = 4;

		while (getline(infile, str))
		{
			// Some preprocessing
			for (int i = 0; i < 32; i++)
			{
				boost::replace_all(str, "$" + registers[i].name, to_string(i));
			}
			boost::replace_all(str, "\t", " ");
			// Initialisations
			string command;
			string arguments[maxArguments];
			for (int i = 0; i < maxArguments; i++)
			{
				arguments[i] = "none";
			}
			int i = 0;
			int argument_number = 0;
			int first = 1;
			int non_empty = false;
			while (i < str.size())
			{
				// handling whitespace
				if (str[i] == ' ' || str[i] == '\t')
				{
					i++;
					continue;
				}
				string token = "";
				// command
				if (first == 1)
				{
					first = 0;
					while (i < str.size() && str[i] != ' ')
					{
						token = token + str[i];
						i += 1;
						non_empty = true;
					}
					command = token;
					i += 1;
					continue;
				}
				// arguments
				while (i < str.size() && str[i] != ',' && str[i] != '\n')
				{
					token = token + str[i];
					i += 1;
				}
				i += 1;
				if (argument_number >= maxArguments)
				{
					throwError(str, 3);
					break;
				}
				arguments[argument_number] = token;
				argument_number += 1;
			}
			if (non_empty)
			{
				cout << command << " " << arguments[0] << " " << arguments[1] << " " << arguments[2] << '\n';
				encode(command, arguments, maxArguments, instructionIndex);
				instructionIndex += 4;
			}
		}
		infile.close();
	}
	void execute()
	{
		// Reading instructions from memory, decoding and executing them
		// const int maxArguments = 4;

		int program_counter = 0;

		for (int i = 0; i < 10; i++)
		{
			no_exec_instructions[i] = 0;
		}

		while (program_counter < instructionIndex)
		{
			int instructDecoded[maxArguments + 1];
			decode(program_counter, instructDecoded);
			cout << instructDecoded[0] << " " << instructDecoded[1] << " " << instructDecoded[2] << " " << instructDecoded[3] << '\n';
			if (instructDecoded[0] < 10)
			{
				no_exec_instructions[instructDecoded[0]] += 1;
			}
			if (instructDecoded[0] == 0)
			{
				lw(instructDecoded[1], instructDecoded[2], instructDecoded[3]);
			}
			else if (instructDecoded[0] == 1)
			{
				sw(instructDecoded[1], instructDecoded[2], instructDecoded[3]);
			}
			else if (instructDecoded[0] == 2)
			{
				add(instructDecoded[1], instructDecoded[2], instructDecoded[3]);
			}
			else if (instructDecoded[0] == 3)
			{
				sub(instructDecoded[1], instructDecoded[2], instructDecoded[3]);
			}
			else if (instructDecoded[0] == 4)
			{
				mul(instructDecoded[1], instructDecoded[2], instructDecoded[3]);
			}
			else if (instructDecoded[0] == 5)
			{
				addi(instructDecoded[1], instructDecoded[2], instructDecoded[3]);
			}
			else if (instructDecoded[0] == 6)
			{
				if (instructDecoded[1] >= instructionIndex || instructDecoded[1] < 0)
				{
					throwError(to_string(instructDecoded[1]), 7);
				}
				program_counter = instructDecoded[1];
				continue;
			}
			else if (instructDecoded[0] == 7)
			{
				if (instructDecoded[3] >= instructionIndex || instructDecoded[3] < 0)
				{
					throwError(to_string(instructDecoded[3]), 7);
				}
				if (beq(instructDecoded[1], instructDecoded[2]))
				{
					printRegisterContents();
					program_counter = instructDecoded[3];
					continue;
				}
			}
			else if (instructDecoded[0] == 8)
			{
				if (instructDecoded[3] >= instructionIndex || instructDecoded[3] < 0)
				{
					throwError(to_string(instructDecoded[3]), 7);
				}
				if (bne(instructDecoded[1], instructDecoded[2]))
				{
					printRegisterContents();
					program_counter = instructDecoded[3];
					continue;
				}
			}
			else if (instructDecoded[0] == 9)
			{
				slt(instructDecoded[1], instructDecoded[2], instructDecoded[3]);
			}
			else
			{
				cout << "Error: The instruction is either undefined or out of scope of this assignment.\n";
				cout << "Kindly use add, sub, mul, beq, bne, slt, j, lw, sw, addi instructions only\n";
				exit(0);
			}
			program_counter += 1;
			printRegisterContents();
		}
	}
};
string MIPS::instructions[10] = {"lw", "sw", "add", "sub", "mul", "addi", "j", "beq", "bne", "slt"};

int main()
{

	MIPS interpreter;
	interpreter.setMemory(100, 100);
	interpreter.setMemory(101, 2);
	interpreter.setMemory(102, 3);
	interpreter.setMemory(103, 4);

	interpreter.readInstructions("program.asm");

	// interpreter.execute();

	interpreter.printStatistics();
}
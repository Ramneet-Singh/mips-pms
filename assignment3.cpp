#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>
#include <vector>
#include <string.h>
#include <boost/algorithm/string/replace.hpp>

#include <dram.hpp>

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
	int instructionIndex;
	vector<string> labels;
	map<string, int> labelToAddr;

public:
	// Instruction mapping
	static string instructions[10];
	int maxArguments;
	bool firstLoad;
	DRAM dramMemory;

public:
	MIPS(int rowAccessDelay, int colAccessDelay, bool blockMode)
	{
		dramMemory = DRAM(rowAccessDelay, colAccessDelay, blockMode);
		firstLoad = true;

		maxArguments = 3;
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
		// 8 for empty arguments
		// 9 for modifying register $zero
		// 10 for referring to an unaligned word (reading or writing)
		// 11 for instruction address which is not a multiple of 4
		// 12 for instructions causing integer overflow
		// 13 for a label being declared twice
		// 14 for using an undeclared label
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
			cout << "Error: The instruction \"" << argument << "\" is either undefined or out of scope of this assignment.\n";
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
		else if (ErrorType == 8)
		{
			cout << "Error: The instruction: " << argument << " has an empty argument!" << '\n';
		}
		else if (ErrorType == 9)
		{
			cout << "Error: The instruction: " << argument << " is trying to modify register $zero!" << '\n';
		}
		else if (ErrorType == 10)
		{
			cout << "Error: The instruction: " << argument << " is referring to an un-aligned word" << '\n';
		}
		else if (ErrorType == 11)
		{
			cout << "Error: Instruction address is not a multiple of 4 in " << argument << "\n";
		}
		else if (ErrorType == 12)
		{
			cout << "Error: The instruction: " << argument << " is causing integer overflow"
				 << "\n";
		}
		else if (ErrorType == 13)
		{
			cout << "Error: The label: \"" << argument << "\" has been declared twice"
				 << "\n";
		}
		else if (ErrorType == 14)
		{
			cout << "Error: The label: \"" << argument << "\" has not been declared"
				 << "\n";
		}
		exit(1);
	}

	void checkNumArguments(string cmnd, string args[], int maxArgs, int expectNum)
	{
		for (int k = 0; k < expectNum; k++)
		{
			if (args[k] == "")
			{
				throwError(cmnd, 5);
			}
		}
		for (int j = expectNum; j < maxArgs; j++)
		{
			if (args[j] != "")
			{
				throwError(cmnd, 3);
			}
		}
	}

	void printRegisterContents()
	{
		cout << setw(6) << "Reg no." << setw(11) << "Reg name" << setw(15) << "Content" << '\n';
		for (int i = 0; i < 32; i++)
		{
			cout << setw(6) << i << setw(11) << registers[i].name << setw(15) << registers[i].getHex() << '\n';
		}
	}

	/*           -----------------------------------------------------------------           
	                                INSTRUCTION FUNCTIONS
				 -----------------------------------------------------------------
	*/

	void issueLw(int a, int b, int c)
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
		if (addr >= (sizeof(dramMemory.Memory) / sizeof(**dramMemory.Memory)) || addr < instructionIndex)
		{
			throwError(to_string(addr), 2);
		}
		if (a == 0)
		{
			throwError("lw $" + registers[a].name + " " + to_string(c) + "($" + registers[b].name + ")", 9);
		}
		if ((addr % 4) != 0)
		{
			throwError("lw $" + registers[a].name + " " + to_string(c) + "($" + registers[b].name + ")", 10);
		}

		array<int, 3> dramInstr;
		dramInstr[0] = 0;
		dramInstr[1] = a;
		dramInstr[2] = addr;
		dramMemory.pendingInstructions.push_back(dramInstr);
	}
	void issueSw(int a, int b, int c)
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
		int val = registers[a].content;
		if (addr >= (sizeof(dramMemory.Memory) / sizeof(**dramMemory.Memory)) || addr < instructionIndex)
		{
			throwError(to_string(addr), 2);
		}
		if ((addr % 4) != 0)
		{
			throwError("sw $" + to_string(a) + " " + to_string(c) + "($" + registers[b].name + ")", 10);
		}

		array<int, 3> dramInstr;
		dramInstr[0] = 1;
		dramInstr[1] = val;
		dramInstr[2] = addr;
		dramMemory.pendingInstructions.push_back(dramInstr);
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
		if (a == 0)
		{
			throwError("add $" + registers[a].name + " $" + registers[b].name + " $" + registers[c].name, 9);
		}
		int result = registers[b].content + registers[c].content;
		if (registers[b].content > 0 && registers[c].content > 0 && result < 0)
		{
			throwError("add $" + registers[a].name + " $" + registers[b].name + " $" + registers[c].name, 12);
		}
		if (registers[b].content < 0 && registers[c].content < 0 && result > 0)
		{
			throwError("add $" + registers[a].name + " $" + registers[b].name + " $" + registers[c].name, 12);
		}
		registers[a].setContent(result);
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
		if (a == 0)
		{
			throwError("sub $" + registers[a].name + " $" + registers[b].name + " $" + registers[c].name, 9);
		}
		int result = registers[b].content - registers[c].content;
		if (registers[b].content < 0 && registers[c].content > 0 && result > 0)
		{
			throwError("sub $" + registers[a].name + " $" + registers[b].name + " $" + registers[c].name, 12);
		}
		if (registers[b].content > 0 && registers[c].content < 0 && result < 0)
		{
			throwError("sub $" + registers[a].name + " $" + registers[b].name + " $" + registers[c].name, 12);
		}
		registers[a].setContent(result);
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
		if (a == 0)
		{
			throwError("mul $" + registers[a].name + " $" + registers[b].name + " $" + registers[c].name, 9);
		}
		registers[a].setContent(registers[b].content * registers[c].content);
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
		if (a == 0)
		{
			throwError("addi $" + registers[a].name + " $" + registers[b].name + " " + to_string(c), 9);
		}
		registers[a].setContent(registers[b].content + c);
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
		return toJump;
	}
	void slt(int a, int b, int c)
	{
		if (a == 0)
		{
			throwError("slt $zero $" + registers[b].name + " $" + registers[c].name, 9);
		}
		if (registers[b].content < registers[c].content)
		{
			registers[a].setContent(1);
		}
		else
		{
			registers[a].setContent(0);
		}
	}

	/*           -----------------------------------------------------------------           
	                                PRINTING FUNCTION
				 -----------------------------------------------------------------
	*/

	void printStatistics()
	{
		cout << "===================================================\n";
		cout << setw(40) << "Total number of clock cycles :  " << setw(10) << clockCycles << '\n';
		cout << setw(40) << "Number of row buffer updates:  " << setw(10) << dramMemory.rowBufferUpdates << '\n';
		cout << "\n===================================================\n";
	}

	void handleActivity()
	{
		if (dramMemory.dramCompletedActivity[0] == -1)
		{
			return;
		}

		switch (dramMemory.dramCompletedActivity[0])
		{
		case 0:
		{
			cout << "DRAM Activity: Copied Row " << dramMemory.dramCompletedActivity[1] << " to Row Buffer\n";
			break;
		}
		case 1:
		{
			cout << "DRAM Activity: Writeback Row " << dramMemory.dramCompletedActivity[1] << " to Main Memory\n";
			break;
		}
		case 2:
		{
			registers[dramMemory.dramCompletedActivity[2]].setContent(dramMemory.dramCompletedActivity[1]);
			cout << "Register Modified: $" << dramMemory.dramCompletedActivity[2] << " == $" << registers[dramMemory.dramCompletedActivity[2]].name << " == " << registers[dramMemory.dramCompletedActivity[2]].content << "\n";
			break;
		}
		case 3:
		{
			cout << "Memory Location Modified: Address == " << dramMemory.dramCompletedActivity[1] << " Value == " << dramMemory.rowBuffer[dramMemory.dramCompletedActivity[1] % 512] << "\n";
			break;
		}
		default:
		{
			break;
		}
		}
	}

	void printInstruction(int *instr)
	{
		if (instr[0] == 6)
		{
			cout << instructions[instr[0]] << " " << instr[1] << "\n";
		}
		else if (instr[0] == 1 || instr[0] == 0 || instr[0] == 5 || instr[0] == 7 || instr[0] == 8)
		{
			cout << instructions[instr[0]] << " $" << instr[1] << " $" << instr[2] << " " << instr[3] << '\n';
		}
		else
		{
			cout << instructions[instr[0]] << " $" << instr[1] << " $" << instr[2] << " $" << instr[3] << '\n';
		}
	}

	/*           -----------------------------------------------------------------           
	                                INSTRUCTION ENCODING FUNCTIONS
				 -----------------------------------------------------------------
	*/

	void encode(string command, string arguments[], int maxArguments, int instructionIndex)
	{
		auto itr = find(begin(instructions), end(instructions), command);
		if (itr == end(instructions))
		{
			throwError(command, 4);
		}
		int commandCode = distance(instructions, itr);
		if (commandCode == 0 || commandCode == 1)
		{
			// lw and sw instructions
			int registerNo, baseRegister, offset;
			checkNumArguments(command, arguments, maxArguments, 2);
			registerNo = stoi(arguments[0]);
			string secondArg = "";
			string thirdArg = "";
			int i = 0;
			while (arguments[1][i] != '(' && i < arguments[1].size())
			{
				secondArg += arguments[1][i];
				i++;
			}
			if (i == arguments[1].size())
			{
				//Then there is no base register given
				baseRegister = 0;
				offset = stoi(arguments[1], nullptr, 0);
			}
			else
			{
				int j = i + 1;
				while (arguments[1][j] != ')' && j < arguments[1].size())
				{
					thirdArg += arguments[1][j];
					j++;
				}
				if (j == arguments[1].size())
				{
					throwError(command, 6);
				}
				baseRegister = stoi(thirdArg);
				offset = stoi(secondArg, nullptr, 0);
			}
			dramMemory.store(instructionIndex + 1, registerNo);
			dramMemory.store(instructionIndex + 2, baseRegister);
			dramMemory.store(instructionIndex + 3, offset);
		}
		else if (commandCode == 7 || commandCode == 8)
		{
			// beq or bne command
			int reg1, reg2, labelPos;
			checkNumArguments(command, arguments, maxArguments, 3);
			reg1 = stoi(arguments[0]);
			reg2 = stoi(arguments[1]);
			string label = arguments[2];
			auto ptr = find(labels.begin(), labels.end(), label);
			if (ptr != labels.end())
			{
				labelPos = ptr - labels.begin();
			}
			else
			{
				labelPos = labels.size();
				labels.push_back(label);
			}
			dramMemory.store(instructionIndex + 1, reg1);
			dramMemory.store(instructionIndex + 2, reg2);
			dramMemory.store(instructionIndex + 3, labelPos);
		}
		else if (commandCode == 5)
		{
			// addi instruction
			int reg1, reg2, imValue;
			checkNumArguments(command, arguments, maxArguments, 3);
			reg1 = stoi(arguments[0]);
			reg2 = stoi(arguments[1]);
			imValue = stoi(arguments[2]);
			dramMemory.store(instructionIndex + 1, reg1);
			dramMemory.store(instructionIndex + 2, reg2);
			dramMemory.store(instructionIndex + 3, imValue);
		}
		else if (commandCode == 6)
		{
			// j instruction
			int jumpLabelPos;
			checkNumArguments(command, arguments, maxArguments, 1);
			string jumpLabel = arguments[0];
			auto ptr = find(labels.begin(), labels.end(), jumpLabel);
			if (ptr != labels.end())
			{
				jumpLabelPos = ptr - labels.begin();
			}
			else
			{
				jumpLabelPos = labels.size();
				labels.push_back(jumpLabel);
			}
			dramMemory.store(instructionIndex + 1, jumpLabelPos);
		}
		else
		{
			// add, sub, mul, slt
			int reg1, reg2, reg3;
			checkNumArguments(command, arguments, maxArguments, 3);
			reg1 = stoi(arguments[0]);
			reg2 = stoi(arguments[1]);
			reg3 = stoi(arguments[2]);

			dramMemory.store(instructionIndex + 1, reg1);
			dramMemory.store(instructionIndex + 2, reg2);
			dramMemory.store(instructionIndex + 3, reg3);
		}
		// cout<<command<<instructionIndex<<commandCode<<'\n';
		dramMemory.store(instructionIndex, commandCode);
	}

	void decode(int index, int *instructDecoded)
	{
		// add everything decoded in instructDecoded.
		// instructDecoded[0] contains command, and other elements contain arguments
		int commandCode = dramMemory.fetch(index);
		instructDecoded[0] = commandCode;
		string cmdLabel;
		int labelPos;
		switch (commandCode)
		{
		case 6:
		{
			// labels may be going out of range
			cmdLabel = labels[dramMemory.fetch(index + 1)];
			auto itr1 = labelToAddr.find(cmdLabel);
			if (itr1 == labelToAddr.end())
			{
				throwError(cmdLabel, 14);
			}
			instructDecoded[1] = itr1->second;
			break;
		}
		case 7:
		case 8:
		{
			instructDecoded[1] = dramMemory.fetch(index + 1);
			instructDecoded[2] = dramMemory.fetch(index + 2);
			// labels may be going out of range
			cmdLabel = labels[dramMemory.fetch(index + 3)];
			auto itr2 = labelToAddr.find(cmdLabel);
			if (itr2 == labelToAddr.end())
			{
				throwError(cmdLabel, 14);
			}
			instructDecoded[3] = itr2->second;
			break;
		}
		default:
		{
			for (int k = 1; k < 4; k++)
			{
				instructDecoded[k] = dramMemory.fetch(index + k);
			}
		}
		}
	}

	/*           -----------------------------------------------------------------           
	                                PARSING FUNCTION
				 -----------------------------------------------------------------
	*/

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
			boost::replace_all(str, "$", "");
			// Initialisations
			string command;
			string arguments[maxArguments];
			for (int i = 0; i < maxArguments; i++)
			{
				arguments[i] = "";
			}
			int i = 0;
			int argument_number = 0;
			int first = 1;
			bool non_empty = false;
			bool string_encountered = false;
			while (i < str.size() || string_encountered)
			{
				// handling whitespace
				if (i < str.size() && (str[i] == ' ' || str[i] == '\t'))
				{
					i++;
					continue;
				}

				string token = "";
				// command
				if (first == 1)
				{
					first = 0;
					while (i < str.size() && str[i] != ' ' && str[i] != ':')
					{
						token = token + str[i];
						i += 1;
						non_empty = true;
					}
					if (i < str.size() && str[i] == ':')
					{
						// We have reached a label
						// Store token in labels vector
						if (find(labels.begin(), labels.end(), token) == labels.end())
						{
							labels.push_back(token);
						}
						// Map label to the instruction index
						if (labelToAddr.find(token) != labelToAddr.end())
						{
							throwError(token, 13);
						}
						labelToAddr.insert(make_pair(token, instructionIndex));
						// cout << "Found a label: " << token << "\n";
						first = 1;
						non_empty = false;
						i += 1;
						continue;
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
				if (argument_number >= maxArguments)
				{
					throwError(str, 3);
					break;
				}
				if (token == "")
				{
					throwError(str, 8);
				}
				arguments[argument_number] = token;
				string_encountered = false;
				if (str[i] == ',')
				{
					string_encountered = true;
				}
				i += 1;
				argument_number += 1;
			}
			if (non_empty)
			{
				// cout << command << " " << arguments[0] << " " << arguments[1] << " " << arguments[2] << " " << instructionIndex << " " << endl;
				encode(command, arguments, maxArguments, instructionIndex);
				instructionIndex += 4;
			}
		}

		/*
		cout << "----------------------------LABELS VECTOR----------------------\n";
		for (int i = 0; i < labels.size(); i++)
			cout << labels.at(i) << ' ';
		cout << "\n";

		cout << "----------------------------LABELS MAP----------------------\n";
		for (auto it = labelToAddr.begin(); it != labelToAddr.end(); ++it)
		{
			cout << it->first << " => " << it->second << " \n";
		}
		*/

		infile.close();
	}

	/*           -----------------------------------------------------------------           
	                                EXECUTION FUNCTION
				 -----------------------------------------------------------------
	*/

	void execute()
	{
		int programCounter = 0;

		// Till you cross the instruction section of memory, keep fetching instructions and executing
		while (programCounter < instructionIndex)
		{
			clockCycles++;
			cout << "------------Clock Cycle: " << clockCycles << "----------------\n";

			int instructDecoded[maxArguments + 1];
			decode(programCounter, instructDecoded);

			// Check if we can issue the next instruction
			bool issueNext = !(dramMemory.isBlocked(instructDecoded));

			dramMemory.executeNext();

			// If we can execute the next instruction, do it
			if (issueNext)
			{
				printInstruction(instructDecoded);

				switch (instructDecoded[0])
				{
				case 0:
				{
					// lw instruction
					issueLw(instructDecoded[1], instructDecoded[2], instructDecoded[3]);
					break;
				}
				case 1:
				{
					// sw instruction
					issueSw(instructDecoded[1], instructDecoded[2], instructDecoded[3]);
					break;
				}
				case 2:
				{
					add(instructDecoded[1], instructDecoded[2], instructDecoded[3]);
					cout << "Register Modified: $" << instructDecoded[1] << " == $" << registers[instructDecoded[1]].name << " == " << registers[instructDecoded[1]].content << "\n";
					break;
				}
				case 3:
				{
					sub(instructDecoded[1], instructDecoded[2], instructDecoded[3]);
					cout << "Register Modified: $" << instructDecoded[1] << " == $" << registers[instructDecoded[1]].name << " == " << registers[instructDecoded[1]].content << "\n";
					break;
				}
				case 4:
				{
					mul(instructDecoded[1], instructDecoded[2], instructDecoded[3]);
					cout << "Register Modified: $" << instructDecoded[1] << " == $" << registers[instructDecoded[1]].name << " == " << registers[instructDecoded[1]].content << "\n";
					break;
				}
				case 5:
				{
					addi(instructDecoded[1], instructDecoded[2], instructDecoded[3]);
					cout << "Register Modified: $" << instructDecoded[1] << " == $" << registers[instructDecoded[1]].name << " == " << registers[instructDecoded[1]].content << "\n";
					break;
				}
				case 6:
				{
					if (instructDecoded[1] >= instructionIndex || instructDecoded[1] < 0)
					{
						throwError(to_string(instructDecoded[1]), 7);
					}
					if ((instructDecoded[1] % 4) != 0)
					{
						throwError("j " + to_string(instructDecoded[1]), 11);
					}
					programCounter = instructDecoded[1] - 4;
					break;
				}
				case 7:
				{
					if (instructDecoded[3] >= instructionIndex || instructDecoded[3] < 0)
					{
						throwError(to_string(instructDecoded[3]), 7);
					}
					bool check = beq(instructDecoded[1], instructDecoded[2]);
					if ((instructDecoded[3] % 4) != 0)
					{
						throwError("beq " + registers[instructDecoded[1]].name + " " + registers[instructDecoded[2]].name + " " + to_string(instructDecoded[3]), 11);
					}
					if (check)
					{
						programCounter = instructDecoded[3] - 4;
					}
					break;
				}
				case 8:
				{
					if (instructDecoded[3] >= instructionIndex || instructDecoded[3] < 0)
					{
						throwError(to_string(instructDecoded[3]), 7);
					}
					bool check = bne(instructDecoded[1], instructDecoded[2]);
					if ((instructDecoded[3] % 4) != 0)
					{
						throwError("bne " + registers[instructDecoded[1]].name + " " + registers[instructDecoded[2]].name + " " + to_string(instructDecoded[3]), 11);
					}
					if (check)
					{
						programCounter = instructDecoded[3] - 4;
					}
					break;
				}
				case 9:
				{
					slt(instructDecoded[1], instructDecoded[2], instructDecoded[3]);
					cout << "Register Modified: $" << instructDecoded[1] << " == $" << registers[instructDecoded[1]].name << " == " << registers[instructDecoded[1]].content << "\n";
					break;
				}
				default:
				{
					cout << "Error: The instruction is either undefined or out of scope of this assignment.\n";
					cout << "Kindly use add, sub, mul, beq, bne, slt, j, lw, sw, addi instructions only\n";
					exit(0);
				}
				}

				programCounter = programCounter + 4;
			}

			// Check for any activity completed by the DRAM
			if (dramMemory.dramCompletedActivity[0] != -1)
			{
				handleActivity();
			}

			cout << "\n";
		}

		while (!dramMemory.pendingInstructions.empty())
		{
			clockCycles++;
			cout << "------------Clock Cycle: " << clockCycles << "----------------\n";

			dramMemory.executeNext();

			// Check for any activity completed by the DRAM
			if (dramMemory.dramCompletedActivity[0] != -1)
			{
				handleActivity();
			}

			cout << "\n";
		}
		if (dramMemory.bufferRowIndex != -1)
		{
			int rowW = dramMemory.bufferRowIndex;
			dramMemory.writeback();
			clockCycles += 10;
			dramMemory.rowBufferUpdates += 1;
			cout << "------------Clock Cycle: " << clockCycles << "----------------\n";
			cout << "DRAM Activity: Writeback Row " << rowW << " to Main Memory\n";
		}
		cout << "\n";
		printStatistics();
	}
};

string MIPS::instructions[10] = {"lw", "sw", "add", "sub", "mul", "addi", "j", "beq", "bne", "slt"};

int main(int argc, char **argv)
{
	string filename;
	int rowDelay, colDelay;
	bool blockMode;
	if (argc == 5)
	{
		filename = argv[1];
		rowDelay = stoi(argv[2]);
		colDelay = stoi(argv[3]);
		blockMode = (stoi(argv[4]) == 0) ? true : false;
	}
	else
	{
		cout << "Error: incorrect usage. \nPlease run: \t./assignment3 path_to_program ROW_ACCESS_DELAY COL_ACCESS_DELAY blockingMode\n";
		return -1;
	}

	MIPS interpreter(rowDelay, colDelay, blockMode);

	interpreter.readInstructions(filename);

	interpreter.execute();
}
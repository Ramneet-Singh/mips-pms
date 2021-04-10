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

	vector<int> addressesAccessed;

public:
	// Instruction mapping
	static string instructions[10];
	static string registerNames[32];
	int no_exec_instructions[10];
	int maxArguments;
	bool firstLoad;
	DRAM dramMemory;
	int programCounter;
	// Are we simulating an execution or not?
	bool dryrun;
	// If we are making the dram wait, till what clock cycle number? If not, we set it to -1
	int waitDramTill;

public:
	MIPS(int rowAccessDelay, int colAccessDelay, bool blockMode, bool dry = false)
	{

		dramMemory = DRAM(rowAccessDelay, colAccessDelay, blockMode, dry);
		firstLoad = true;
		dryrun = dry;
		waitDramTill = -1;

		maxArguments = 3;
		clockCycles = 0;
		programCounter = 0;

		for (int i = 0; i < 32; i++)
		{
			registers[i].setName(registerNames[i]);
			registers[i].setContent(0);
		}

		for (int i = 0; i < 10; i++)
		{
			no_exec_instructions[i] = 0;
		}
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

	/*           ================================================================-           
	                                INSTRUCTION FUNCTIONS
				 ================================================================-
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
		addressesAccessed.push_back(addr);
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

		Instruction *dramInstr = new Instruction(addr, a, 0);
		dramMemory.addInstruction(*dramInstr);
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
		addressesAccessed.push_back(addr);
		int val = registers[a].content;
		if (addr >= (sizeof(dramMemory.Memory) / sizeof(**dramMemory.Memory)) || addr < instructionIndex)
		{
			throwError(to_string(addr), 2);
		}
		if ((addr % 4) != 0)
		{
			throwError("sw $" + to_string(a) + " " + to_string(c) + "($" + registers[b].name + ")", 10);
		}

		Instruction *dramInstr = new Instruction(addr, val, 1);
		dramMemory.addInstruction(*dramInstr);
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

	/*           ================================================================-           
	                                PRINTING FUNCTION
				 ================================================================-
	*/

	void printStatistics()
	{
		cout << "====================================================\n\n";
		cout << setw(35) << "Total number of clock cycles :  " << setw(10) << clockCycles << '\n';
		cout << setw(35) << "Number of row buffer updates :  " << setw(10) << dramMemory.rowBufferUpdates;

		cout << "\n\n===================================================\n";
		// printing number of times each instruction was executed
		cout << "   Number of times each instruction was executed:\n";
		cout << "===================================================\n";
		cout << setw(18) << "Instruction" << setw(19) << "Executed\n";
		for (int i = 0; i < 10; i++)
		{
			cout << setw(18) << instructions[i] << setw(9) << no_exec_instructions[i] << setw(10) << " time(s)\n";
		}
		cout << "====================================================\n";
	}

	void printMemory()
	{
		cout << "====================================================\n";
		cout << "Accessed memory content at the end of the execution:\n";
		cout << "====================================================\n";
		cout << setw(20) << "Memory address:" << setw(25) << "Memory content:\n";

		sort(addressesAccessed.begin(), addressesAccessed.end());
		addressesAccessed.erase(unique(addressesAccessed.begin(), addressesAccessed.end()), addressesAccessed.end());

		for (auto &address : addressesAccessed)
		{
			int col = address % NUMCOLS;
			int row = (address - col) / NUMCOLS;
			stringstream stream;
			if (row == dramMemory.bufferRowIndex)
			{
				stream << "0x" << hex << dramMemory.rowBuffer[col];
				cout << setw(20) << to_string(address) + " - " + to_string(address + 3) << setw(13) << stream.str() << setw(12) << to_string(dramMemory.rowBuffer[col]) + "\n";
			}
			else
			{
				stream << "0x" << hex << dramMemory.Memory[row][col];
				cout << setw(20) << to_string(address) + " - " + to_string(address + 3) << setw(13) << stream.str() << setw(12) << to_string(dramMemory.Memory[row][col]) + "\n";
			}
		}
	}

	void printRegisterContents()
	{
		cout << "====================================================\n";
		cout << "  Register contents at the end of the execution:\n";
		cout << "====================================================\n";
		cout << setw(6) << "Reg no." << setw(11) << "Reg name" << setw(15) << "Content-Dec" << setw(15) << "Content-Hex" << '\n';
		for (int i = 0; i < 32; i++)
		{
			cout << setw(6) << i << setw(11) << registers[i].name << setw(15) << registers[i].content << setw(15) << registers[i].getHex() << '\n';
		}
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
			if (!dryrun)
			{
				cout << "DRAM Activity: Copied Row " << dramMemory.dramCompletedActivity[1] << " to Row Buffer\n";
			}
			break;
		}
		case 1:
		{
			if (!dryrun)
			{
				cout << "DRAM Activity: Writeback Row " << dramMemory.dramCompletedActivity[1] << " to Main Memory\n";
			}
			break;
		}
		case 2:
		{
			registers[dramMemory.dramCompletedActivity[2]].setContent(dramMemory.dramCompletedActivity[1]);
			if (!dryrun)
			{
				cout << "Register Modified: $" << dramMemory.dramCompletedActivity[2] << " == $" << registers[dramMemory.dramCompletedActivity[2]].name << " == " << registers[dramMemory.dramCompletedActivity[2]].content << "\n";
			}
			break;
		}
		case 3:
		{
			if (!dryrun)
			{
				cout << "Memory Location Modified: Address == " << dramMemory.dramCompletedActivity[1] << " Value == " << dramMemory.rowBuffer[dramMemory.dramCompletedActivity[1] % NUMCOLS] << "\n";
			}
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
			cout << instructions[instr[0]] << " " << instr[1];
		}
		else if (instr[0] == 1 || instr[0] == 0 || instr[0] == 5 || instr[0] == 7 || instr[0] == 8)
		{
			cout << instructions[instr[0]] << " $" << registerNames[instr[1]] << " $" << registerNames[instr[2]] << " " << instr[3];
		}
		else
		{
			cout << instructions[instr[0]] << " $" << registerNames[instr[1]] << " $" << registerNames[instr[2]] << " $" << registerNames[instr[3]];
		}
	}

	/*           ================================================================-           
	                                INSTRUCTION ENCODING FUNCTIONS
				 ================================================================-
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
				try{
					offset = stoi(arguments[1], nullptr, 0);
				}
				catch (std::out_of_range& e) {
					throwError(arguments[1], 2);
				}
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
				try{
					offset = stoi(secondArg, nullptr, 0);
				}
				catch (std::out_of_range& e) {
					throwError(secondArg, 2);
				}
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

	/*           ================================================================-           
	                                PARSING FUNCTION
				 ================================================================-
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
		cout << "================------------LABELS VECTOR================------\n";
		for (int i = 0; i < labels.size(); i++)
			cout << labels.at(i) << ' ';
		cout << "\n";

		cout << "================------------LABELS MAP================------\n";
		for (auto it = labelToAddr.begin(); it != labelToAddr.end(); ++it)
		{
			cout << it->first << " => " << it->second << " \n";
		}
		*/

		infile.close();
	}

	/*           ================================================================-           
	                                EXECUTION FUNCTION
				 ================================================================-
	*/

	int execute(int numLook = 0, int targetRow = -1);
};

// [ASSIGNMENT 4]
int lookahead(int numCycles, MIPS &inter, int targetRow)
{
	// Create a new interpreter at the same state as the MIPS interpreter, run it for numCycles, without the DRAM executing anything, and see if an instruction comes which is targeting the same row as is in the row buffer
	int oldNumInst = Instruction::numInstr;
	int oldBufferIndex = Instruction::rowBufferIndex;
	MIPS interpreter(inter);
	interpreter.dryrun = true;
	interpreter.dramMemory.dryrun = true;
	int ans = interpreter.execute(numCycles, targetRow);
	Instruction::numInstr = oldNumInst;
	Instruction::rowBufferIndex = oldBufferIndex;
	interpreter.dramMemory.deleteAllDryrunInst();
	return ans;
}

int MIPS::execute(int numLook, int targetRow)
{
	// Till you cross the instruction section of memory, keep fetching instructions and executing
	int i = 0;
	while ((i < numLook || !dryrun) && programCounter < instructionIndex)
	{
		if (dryrun)
		{
			clockCycles++;
			i++;
			int instructDecoded[maxArguments + 1];
			decode(programCounter, instructDecoded);

			// Check if we can issue the next instruction
			bool issueNext = !(dramMemory.isBlocked(instructDecoded));

			dramMemory.executeNext();

			// If we can execute the next instruction, do it
			if (issueNext)
			{
				if (instructDecoded[0] < 10)
				{
					no_exec_instructions[instructDecoded[0]] += 1;
				}

				switch (instructDecoded[0])
				{
				case 0:
				{
					// lw instruction
					issueLw(instructDecoded[1], instructDecoded[2], instructDecoded[3]);
					int addr = registers[instructDecoded[2]].content + instructDecoded[3];
					if ((addr / NUMCOLS) == targetRow)
					{
						return clockCycles;
					}
					break;
				}
				case 1:
				{
					// sw instruction
					issueSw(instructDecoded[1], instructDecoded[2], instructDecoded[3]);
					int addr = registers[instructDecoded[2]].content + instructDecoded[3];
					if ((addr / NUMCOLS) == targetRow)
					{
						return clockCycles;
					}
					break;
				}
				case 2:
				{
					add(instructDecoded[1], instructDecoded[2], instructDecoded[3]);
					break;
				}
				case 3:
				{
					sub(instructDecoded[1], instructDecoded[2], instructDecoded[3]);
					break;
				}
				case 4:
				{
					mul(instructDecoded[1], instructDecoded[2], instructDecoded[3]);
					break;
				}
				case 5:
				{
					addi(instructDecoded[1], instructDecoded[2], instructDecoded[3]);
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
		}
		else
		{
			clockCycles++;
			cout << "==================Clock Cycle: " << setw(3) << clockCycles << "==================\n";

			int instructDecoded[maxArguments + 1];
			decode(programCounter, instructDecoded);

			// Check if we can issue the next instruction
			bool issueNext = !(dramMemory.isBlocked(instructDecoded));

			if (dramMemory.willPerformWritebackNext())
			{
				waitDramTill = lookahead(LOOK, *this, dramMemory.bufferRowIndex);
			}

			if (waitDramTill == -1)
			{
				dramMemory.executeNext();
			}
			else if (waitDramTill == clockCycles)
			{
				dramMemory.executeNext();
				waitDramTill = -1;
			}
			else
			{
				// We must now be waiting
				for (int i = 0; i < 4; i++)
				{
					dramMemory.dramCompletedActivity[i] = -1;
				}
				assert(clockCycles < waitDramTill);
			}

			// If we can execute the next instruction, do it
			if (issueNext)
			{
				if (instructDecoded[0] < 10)
				{
					no_exec_instructions[instructDecoded[0]] += 1;
				}

				cout << "Instruction: ";
				printInstruction(instructDecoded);
				cout<<" issued. Memory address : " << programCounter <<" - "<<programCounter+3<<'\n' ;
				// printInstruction(instructDecoded);

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
	}

	if ((i >= numLook || programCounter >= instructionIndex) && dryrun)
	{
		return -1;
	}

	assert(!dryrun); // If we were dryrunning, we must have returned a value by now
	while (!dramMemory.pendingInstructionsPriority.empty())
	{
		clockCycles++;
		cout << "==================Clock Cycle: " << setw(3) << clockCycles << "==================\n";

		dramMemory.executeNext();

		// Check for any activity completed by the DRAM
		if (dramMemory.dramCompletedActivity[0] != -1)
		{
			handleActivity();
		}

		cout << "\n";
	}
	cout << "\n";
	printRegisterContents();
	printMemory();
	printStatistics();
	return 0;
}

string MIPS::instructions[10] = {"lw", "sw", "add", "sub", "mul", "addi", "j", "beq", "bne", "slt"};
string MIPS::registerNames[32] = {"zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"};

int Instruction::rowBufferIndex = -1;
int Instruction::numInstr = 0;

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
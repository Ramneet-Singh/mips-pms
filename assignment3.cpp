#include <fstream>
#include <iostream>
#include <string.h>

using namespace std;

string decToHex(int n){
	if(n<10){ return to_string(n); }
	else if(n == 10){ return "A";  }
	else if(n == 11){ return "B";  }
	else if(n == 12){ return "C";  }
	else if(n == 13){ return "D";  }
	else if(n == 14){ return "F";  }
	else if(n == 15){ return "G";  }
	else if(n<0){
		return "-1" + decToHex(0-n);
	}
	else{
		return decToHex((n-n%16)/16) + decToHex(n%16);
	}

}

class IntRegister {       
	public:             
		int content;
		string name;

	public:        
		IntRegister(){
			content = 0;
		}
		IntRegister(string n){
			content = 0;
			name = n;
		}
		void setName(string n){
			name = n;
		}
		void setContent(int n){
			content = n;
		}
};

class MIPS {
	public:

		IntRegister registers[32];

		float Memory[1048576];

		int ErrorType ;
		// 0 means no error
		// 1 means invalid register being used
		// 2 means overflow has occurred

	public: 

		MIPS(){
			ErrorType = 0;
			
			registers[0].setName("zero");
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

		void setMemory(int loc, float val){
			Memory[loc] = val;
		}
		void throwError(char* argument){
			if (ErrorType == 0){
				return;
			}
			else if(ErrorType == 1){
				cout<<"Error: Register "<< argument<<" not found!"<<'\n';
			}
			else if(ErrorType == 2){
				cout<<"Error: Register " << argument << " has overflow!"<<'\n';
			}
			else if(ErrorType == 3){
				cout<<"Error: Memory address " << argument << " not found!"<<'\n';
			}
			exit(0);

		}

		int getIntRegister(char* name){
			for(int i = 0; i<32;i++){
				if(registers[i].name.compare(name) == 0){
					return i;
				}
			}
			ErrorType = 1;
			throwError(name);
			return -1;

		}

		void printRegisterContents(){
    		cout << "Reg no.\tReg name"<<"\t"<<"Content" << '\n';
			for(int i = 0; i<32;i++){
				cout <<i<<'\t'<< registers[i].name<<"\t\t"<<decToHex(registers[i].content) << '\n';
			}
		}

		void add(int a, int b, int c){
			registers[a].setContent(registers[b].content + registers[c].content);
		}
		void sub(int a, int b, int c){
			registers[a].setContent(registers[b].content - registers[c].content);
		}
		void mul(int a, int b, int c){
			registers[a].setContent(registers[b].content * registers[c].content);
		}
		void addi(int a, int b, int c){
			registers[a].setContent(registers[b].content + c);
		}
		void lw(int a, int b){
			registers[a].setContent(Memory[b]);
		}
		void sw(int a, int b){
			Memory[a] = registers[b].content;
		}
		bool checkEqual(int a, int b){
			if(registers[a].content == registers[b].content){
				return true;
			}
			return false;
		}
};

void tokenise(string instr, char *tokenized_inst[10]) {
	int len = instr.length();
	char str[len+1];
	strcpy(str, instr.c_str());
	char *token = strtok(str, " ");
	int cnt = 0;
	while(token != NULL) {
		if(strchr(token, ',')) {
			token[strlen(token)-1] = '\0';
		}
		if(strcmp(token, "$ra") != 0 && strchr(token, '$')) {
			for(int i = 1; i < strlen(token); i++) {
				token[i-1] = token[i];
			}
			token[strlen(token)-1] = '\0';
		}
		tokenized_inst[cnt] = token;
		cnt++;
		token = strtok(NULL, " ");
	}
}

int main() {


	MIPS interpreter;
	interpreter.setMemory(0, 100.0);
	interpreter.setMemory(1, 2.0);
	interpreter.setMemory(2, 3.0);
	interpreter.setMemory(3, 4.0);


	ifstream infile;
	string str;
	string instruction[1000];

	
	//read input from file
	infile.open("program.asm");
	int index = 0;
	while(getline(infile, str)) {
		instruction[index] = str;
		index++;
	}
	int no_of_instructions = index;
	int program_counter = 0;
	while ( program_counter < no_of_instructions) {
		string instr = instruction[program_counter];
		cout << instr <<endl;
		char *tokens[10];
		tokenise(instr, tokens);
		if(strcmp(tokens[0], "lw") == 0) {
			interpreter.lw(atoi(tokens[1]), atoi(tokens[2]));
		}
		else if(strcmp(tokens[0], "sw") == 0) {
			interpreter.sw(atoi(tokens[1]), atoi(tokens[2]));
		}
		else if(strcmp(tokens[0], "add") == 0) {
			interpreter.add(atoi(tokens[1]), atoi(tokens[2]), atoi(tokens[3]));
		}
		else if(strcmp(tokens[0], "sub") == 0) {
			interpreter.sub(atoi(tokens[1]), atoi(tokens[2]), atoi(tokens[3]));
		}
		else if(strcmp(tokens[0], "mul") == 0) {
			interpreter.mul(atoi(tokens[1]), atoi(tokens[2]), atoi(tokens[3]));
		}
		else if(strcmp(tokens[0], "addi") == 0) {
			interpreter.addi(atoi(tokens[1]), atoi(tokens[2]), atoi(tokens[3]));	
		}
		else if(strcmp(tokens[0], "beq") == 0) {
			if(interpreter.checkEqual(atoi(tokens[1]), atoi(tokens[2]))){
				interpreter.registers[31].setContent(program_counter+1);
				program_counter = atoi(tokens[3]) - 1;
				interpreter.printRegisterContents();
				continue;
			}
			
		}
		else if(strcmp(tokens[0], "bne") == 0) {
			if( ! interpreter.checkEqual(atoi(tokens[1]), atoi(tokens[2]))){
				interpreter.registers[31].setContent(program_counter+1);
				program_counter = atoi(tokens[3]) - 1;
				interpreter.printRegisterContents();
				continue;
			}
		}
		else if(strcmp(tokens[0], "j") == 0) {
			interpreter.registers[31].setContent(program_counter+1);
			program_counter = atoi(tokens[1]) - 1;
			interpreter.printRegisterContents();
			continue;
		}
		else if (strcmp(tokens[0], "exit") == 0) {
			break;
		}
		else{
			cout<<"Error: The instruction is either undefined or out of scope of this assignment.\n";
			cout<<"Kindly use add, sub, mul, beq, bne, slt, j, lw, sw, addi instructions only\n";
			exit(0);
		}
		interpreter.printRegisterContents();
		program_counter = program_counter + 1;
	}
	infile.close();
}
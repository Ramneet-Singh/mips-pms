#include <iostream>
#include <string>

using namespace std;

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

		int ErrorType ;
		// 0 means no error
		// 1 means invalid register being used
		// 2 means overflow has occurred

	public: 

		MIPS(){
			ErrorType = 0;
			
			registers[0].setName("r0");
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
			registers[30].setName("s8");
			registers[31].setName("ra");
		}

		void throwError(string argument){
			if (ErrorType == 0){
				return;
			}
			else if(ErrorType == 1){
				cout<<"Error: Register " + argument + " not found!"<<'\n';
			}
			else if(ErrorType == 2){
				cout<<"Error: Register " + argument + " has overflow!"<<'\n';
			}

		}

		int getIntRegister(string name){
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
    		cout << "Register"<<"\t"<<"Content" << '\n';
			for(int i = 0; i<32;i++){
				cout << registers[i].name<<"\t\t"<<registers[i].content << '\n';
			}
		}

		void add(string a, string b, string c){
			try{
				registers[getIntRegister(a)].setContent(registers[getIntRegister(b)].content + registers[getIntRegister(c)].content);
			}
			catch(...) {
				ErrorType = 2;
				throwError(a);
			}
			
		}

		void sub(string a, string b, string c){
			try{
				registers[getIntRegister(a)].setContent(registers[getIntRegister(b)].content - registers[getIntRegister(c)].content);
			}
			catch(...) {
				ErrorType = 2;
				throwError(a);
			}
		}

		void mul(string a, string b, string c){
			try{
				registers[getIntRegister(a)].setContent(registers[getIntRegister(b)].content * registers[getIntRegister(c)].content);
			}
			catch(...) {
				ErrorType = 2;
				throwError(a);
			}
		}

		void addi(string a, string b, int c){
			try{
				registers[getIntRegister(a)].setContent(registers[getIntRegister(b)].content + c);
			}
			catch(...) {
				ErrorType = 2;
				throwError(a);
			}
		}
		

};

int main(){
	MIPS interpreter;
	interpreter.addi("t0", "t0", 5);
	interpreter.addi("t1", "t0", 10);
	interpreter.add("t2", "t0", "t1");
	interpreter.mul("s0", "t0", "t2");
	interpreter.sub("s1", "s0", "t2");
	interpreter.sub("random1", "s0", "t2");
	interpreter.addi("t8", "t8", 100000000);
	interpreter.mul("t8", "t8", "t8");
	interpreter.printRegisterContents();
}
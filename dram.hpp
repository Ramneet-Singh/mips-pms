#include <iostream>
#include <deque>
#include <queue>
#include <array>
#define NUMCOLS 512

class Instruction
{

public:
    static int rowBufferIndex;
    static int numInstr;
    int type;
    // 0 means lw
    // 1 means sw
    int target;
    int address;
    int id;
    std::vector<Instruction *> dependencies;

    Instruction(int add, int tar, int typ)
    {
        id = numInstr++;
        address = add;
        target = tar;
        type = typ;
    }

    bool operator==(const Instruction &rhs) const;

    int getRowDifference() const;
};

struct CmpInstPtrs
{
    bool operator()(const Instruction *lhs, const Instruction *rhs) const
    {
        return lhs->getRowDifference() > rhs->getRowDifference();
    }
};

class MyPriorityQueue: public std::priority_queue<Instruction *, std::vector<Instruction *>, CmpInstPtrs> {
public:
    decltype(c.begin()) begin() { return c.begin(); }
    decltype(c.end()) end() { return c.end(); }
};


class DRAM
{
private:
    /*
        [ASSIGNMENT 4]
        Checks whether the instruction given is conflicting with the second instruction (which is pending in dram)
        The rules for an instruction I and a pending dram instruction D to conflict are:
        1. If D is an sw instruction, then there is no conflict as all it says is to store a certain value at a certain address. 
           For subsequent instructions, the values of any of the registers can be changed as we have already made use of them and abstracted them out while issuing the sw request.
        2. Now let D be an lw instruction with register Rd being the target register and AddrD being the address from which to load
           Simply, any instruction which makes use of a value stored in Rd, or seeks to change the value stored in Rd is a conflicting instruction.
           This is because, if it makes use of a value, the programmer expects the lw instruction to have taken place by the time I is executed.
           And if its value changes, then we will change it back to a wrong value after a few cycles if we don't block instruction I.
    */
    bool isClashing(int *instr, Instruction &dramInstr);

    /*
        [ASSIGNMENT 4]
        Called upon completion of the last activity of the currently executing instruction. Deletes it from the pendingInstructionsPriority priority queue and also from the dependencies of each instruction in the queue.
    */
    void deleteCurrentInstruction();

    /*
        Copy the row at rowIdx to row buffer
    */
    void copyToRowBuffer(int rowIdx);

    /*
        Update the columnNum offset in row buffer with the value val
    */
    void updateRowBuffer(int columnNum, int val);

    bool blockingMode;

public:
    // A 2-D Byte Addressable Memory array
    int Memory[/*CHANGED1024*/ 512][/*CHANGED1024*/ 512];
    // Row Buffer for the DRAM
    int rowBuffer[/*CHANGED1024*/ 512];
    int bufferRowIndex;

    int currentInstId;

    int rowBufferUpdates;

    int ROW_ACCESS_DELAY;
    int COL_ACCESS_DELAY;

    /* 
        Have a Buffer of pending instructions
    */
    MyPriorityQueue pendingInstructionsPriority;

    /*
        Have a FIFO Buffer of pending activities of the instruction which is currently being executed
        Each activity is of the form:
        activity[0] = activity type with mapping:-
        0 => copy row into row buffer. For this 
             activity[1] = row index to be copied
             activity[2] = Number of clock cycles it will take for this to be completed (initially ROW_ACCESS_DELAY)
        1 => writeback, i.e., copy the row from row buffer back to main memory. For this
             activity[1] = Number of clock cycles it will take for this to be completed (initially ROW_ACCESS_DELAY)
        2 => copy data at the column offset from row buffer into register. For this 
             activity[1] = column index inside row buffer 
             activity[2] = register number to be written to
             activity[3] = Number of clock cycles it will take for this to be completed (initially COL_ACCESS_DELAY)
        3 => Update data at the column offset in row buffer. For this
             activity[1] = column index inside row buffer
             activity[2] = value to be written to the address
             activity[3] = Number of clock cycles it will take for this to be completed (initially COL_ACCESS_DELAY)
    */
    std::queue<std::array<int, 4>> pendingActivities;

    /*
        Have an array to store the comleted activity in current cycle if any
        dramCompletedActivity[0] = -1 means there are none in this cycle
        set dramCompletedActivity[0] to -1 after printing out the activity
    */
    int dramCompletedActivity[4];

    DRAM();

    DRAM(int rowAccessDelay, int colAccessDelay = 2, bool blockMode = true);

    // 0 cycle delay instruction store and fetch operations
    void store(int address, int val);
    int fetch(int address);

    /*
        Copy the row in row buffer back to main memory
    */
    void writeback();

    /*
        [ASSIGNMENT 4]
        Adds the activities for a dram instruction. Activities for a lw instruction:
        1. Writeback the row in row buffer
        2. Activate the required row and copy it to row buffer
        3. Copy the data at column offset from row buffer to register
        In case the required row is already present in row buffer, skip activities 2 and 3 
    */
    void addActivities(Instruction &dramInstr);

    /*
        Take the front of the pending activities queue, and move it forward by one cycle
    */
    void performActivity();

    /*
        Takes in a request from the processor, figures out its dependencies, and adds it to the pendingInstructionsPriority queue.
    */
    void addInstruction(Instruction &inst);

    /*
        Given a  pending DRAM Instruction inst1 and an instruction to be inserted inst2, checks whether inst1 should be added to the dependencies of inst2
    */
    bool isConflicting(Instruction &inst1, Instruction &inst2);

    /*
        Perform next activity. Checks if there are any already pending activities.
        If there aren't any, checks if there are any pending instructions and adds their activities.
        If an activity has been completed in this cycle, then puts that in dramCompletedActivity.
        Else makes sure that dramCompletedActivity = [-1, -1, -1, -1]
        If the completed activity was the last one of some instruction, then remove that from the pending instructions
    */
    void executeNext();

    /*
        It picks the next instruction to be performed from the currently pending instructions in the DRAM.
        The logic for exploiting row buffer locality is present in this function.
    */
    Instruction *scheduleNextInstr();

    /*
        Checks if the instruction given is safe to execute before all pending instructions of dram are completed
        Iterates through the deque of pending instructions and checks one by one whether there is a conflict between this and the given instruction
    */
    bool isBlocked(int *instruction);
};
#include <iostream>
#include <deque>
#include <queue>
#include <array>
#include <constants.h>

// [ASSIGNMENT 4]
class Instruction
{

public:
    static int rowBufferIndex;
    static int numInstr;
    int type;
    // 0 means lw
    // 1 means sw
    // -1 means empty instruction
    int target;
    int address;
    int id;

    int core_no;
    int index;
    // std::vector<Instruction *> dependencies;
    int dependencies[BUFFER_SIZE];

    Instruction(int add, int tar, int typ)
    {
        id = numInstr++;
        address = add;
        target = tar;
        type = typ;
        for (int i = 0; i < BUFFER_SIZE; i++)
            dependencies[i] = 0;
    }

    Instruction(int idNum, int add, int tar, int typ)
    {
        id = idNum;
        address = add;
        target = tar;
        type = typ;
        for (int i = 0; i < BUFFER_SIZE; i++)
            dependencies[i] = 0;
    }
    bool operator==(const Instruction &rhs) const;

    int getRowDifference() const;

    void print()
    {
        std::string registerNames[32] = {"zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"};
        if (type == 0)
            std::cout << "Index: " << index << ". lw $" << registerNames[target] << " (" << target << ") " << address << " Dependencies: ";
        else if (type == 1)
            std::cout
                << "Index: " << index << ". sw (value " << target << ") " << address << " Dependencies: ";
        else if (type == -1)
            std::cout
                << "Index: " << index << ". <Empty Instruction>"
                << " Dependencies: ";
        for (int i = 0; i < BUFFER_SIZE; i++)
        {
            if (dependencies[i] == 1)
            {
                std::cout << "Index " << i << ", ";
            }
        }
        std::cout << "\n";
    }
};

struct CmpInstPtrs
{
    bool operator()(const Instruction *lhs, const Instruction *rhs) const
    {
        /*  OLDEST FIRST ALGORITHM INSTEAD OF CLOSEST ROW FIRST
        * if(lhs->getRowDifference()==0)
        * {
        *     return false; 
        * }
        * else if(rhs->getRowDifference()==0)
        * {
        *     return true;
        * }
        * else
        * {
        *     return lhs->id > rhs->id;
        * }
        */
        return lhs->getRowDifference() > rhs->getRowDifference();
    }
};

class MyPriorityQueue : public std::priority_queue<Instruction *, std::vector<Instruction *>, CmpInstPtrs>
{
public:
    decltype(c.begin()) begin() { return c.begin(); }
    decltype(c.end()) end() { return c.end(); }
};

// [ASSIGNMENT 4 ends]

class DRAM
{
private:
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
    int Memory[NUMROWS][NUMCOLS];
    // Row Buffer for the DRAM
    int rowBuffer[NUMCOLS];
    int bufferRowIndex;

    int rowBufferUpdates;

    int ROW_ACCESS_DELAY;
    int COL_ACCESS_DELAY;

    // [ASSIGNMENT 4]
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
    Checks if a new instruction's execution will be started in this cycle. If it will, checks which instruction will be scheduled. Returns true only if this instruction would require a writeback and false in all other cases
    */
    bool willPerformWritebackNext();

    /*
        Adds the activities for a dram instruction. Activities for a lw instruction:
        1. Writeback the row in row buffer
        2. Activate the required row and copy it to row buffer
        3. Copy the data at column offset from row buffer to register
        In case the required row is already present in row buffer, skip activities 2 and 3 
    */
    void addActivities(Instruction &dramInstr);

    // [ASSIGNMENT 4 ends]

    /*
        Have an array to store the comleted activity in current cycle if any
        dramCompletedActivity[0] = -1 means there are none in this cycle
        set dramCompletedActivity[0] to -1 after printing out the activity
    */
    int dramCompletedActivity[4];

    DRAM();

    DRAM(int rowAccessDelay, int colAccessDelay = 2);

    void setDelays(int rowAccessDelay, int colAccessDelay);

    // 0 cycle delay instruction store and fetch operations
    void store(int address, int val);
    int fetch(int address);

    /*
        Copy the row in row buffer back to main memory
    */
    void writeback();

    /*
        Take the front of the pending activities queue, and move it forward by one cycle
    */
    bool performActivity();
};
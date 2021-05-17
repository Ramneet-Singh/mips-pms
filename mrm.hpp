#include <dram.hpp>
#include <constants.h>
// TODO: justify/update size of buffer

class MRM
{

public:
    MRM(int rDelay, int cDelay, int block);
    ~MRM();
    int blockingMode;
    int rowAccessDelay;
    int colAccessDelay;

    DRAM dramMemory;
    int currentInstIndex;
    int nextInstIndex;
    int schedulingDelayLeft;

    Instruction *buffer[BUFFER_SIZE];

    /*
        
        Checks whether the instruction given is conflicting with the second instruction (which is pending in dram)
        The rules for an instruction I and a pending dram instruction D to conflict are:
        1. If D is an sw instruction, then there is no conflict as all it says is to store a certain value at a certain address. 
           For subsequent instructions, the values of any of the registers can be changed as we have already made use of them and abstracted them out while issuing the sw request.
        2. Now let D be an lw instruction with register Rd being the target register and AddrD being the address from which to load
           Simply, any instruction which makes use of a value stored in Rd, or seeks to change the value stored in Rd is a conflicting instruction.
           This is because, if it makes use of a value, the programmer expects the lw instruction to have taken place by the time I is executed.
           And if its value changes, then we will change it back to a wrong value after a few cycles if we don't block instruction I.
    */
    bool isClashing(int *instr, Instruction &dramInstr, int cpu_core);
    /*
        Checks if the instruction given is safe to execute before all pending instructions of dram are completed
        Iterates through the deque of pending instructions and checks one by one whether there is a conflict between this and the given instruction
    */
    bool isBlocked(int *instruction, int cpu_core);
    /*
        Given a  pending DRAM Instruction inst1 and an instruction to be inserted inst2, checks whether inst1 should be added to the dependencies of inst2
    */
    bool isConflicting(Instruction &inst1, Instruction &inst2);
    /*
        Called upon completion of the last activity of the currently executing instruction. Deletes it from the pendingInstructionsPriority priority queue and also from the dependencies of each instruction in the queue.
    */
    void deleteCurrentInstruction();
    int getEmptyIndex(int cpu_core);

    /*
    Takes in a request from the processor, figures out its dependencies, and adds it to the pendingInstructionsPriority queue.
    */
    void addInstruction(Instruction &inst);

    /*
        It picks the next instruction to be performed from the currently pending instructions in the DRAM.
        The logic for exploiting row buffer locality is present in this function.
    */

    Instruction *scheduleNextInstr();

    /*
        Perform next activity. Checks if there are any already pending activities.
        If there aren't any, checks if there are any pending instructions and adds their activities.
        If an activity has been completed in this cycle, then puts that in dramCompletedActivity.
        Else makes sure that dramCompletedActivity = [-1, -1, -1, -1]
        If the completed activity was the last one of some instruction, then remove that from the pending instructions
    */
    void executeNext();

    bool isBufferEmpty(int cpu_core);

    bool isBufferEmpty();

    void getDramActivity(int *ar);
};
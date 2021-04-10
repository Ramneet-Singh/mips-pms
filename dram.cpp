#include <dram.hpp>

using namespace std;

DRAM::DRAM()
{
    blockingMode = true;
    ROW_ACCESS_DELAY = 10;
    COL_ACCESS_DELAY = 2;
    bufferRowIndex = -1;
    Instruction::rowBufferIndex = -1;

    for (int i = 0; i < 3; i++)
    {
        dramCompletedActivity[i] = -1;
    }
    rowBufferUpdates = 0;
}

DRAM::DRAM(int rowAccessDelay, int colAccessDelay, bool blockMode)
{
    blockingMode = blockMode;
    ROW_ACCESS_DELAY = rowAccessDelay;
    COL_ACCESS_DELAY = colAccessDelay;
    bufferRowIndex = -1;
    Instruction::rowBufferIndex = -1;

    for (int i = 0; i < 3; i++)
    {
        dramCompletedActivity[i] = -1;
    }
    rowBufferUpdates = 0;
}

void DRAM::store(int address, int val)
{
    if (address < 0 || address >= (sizeof(Memory) / sizeof(Memory[0][0])))
    {
        throw "Error: Invalid address being stored to: " + std::to_string(address);
    }

    int row = address / /*CHANGED1024*/ 512;
    int col = address % /*CHANGED1024*/ 512;
    Memory[row][col] = val;
}

int DRAM::fetch(int address)
{
    if (address < 0 || address >= (sizeof(Memory) / sizeof(Memory[0][0])))
    {
        throw "Error: Invalid address being stored to: " + std::to_string(address);
    }

    int row = address / /*CHANGED1024*/ 512;
    int col = address % /*CHANGED1024*/ 512;
    return Memory[row][col];
}

void DRAM::copyToRowBuffer(int rowIndex)
{
    for (int i = 0; i < /*CHANGED1024*/ 512; i++)
    {
        rowBuffer[i] = Memory[rowIndex][i];
    }
    bufferRowIndex = rowIndex;
    Instruction::rowBufferIndex = rowIndex;
}

void DRAM::writeback()
{
    if (bufferRowIndex < 0 || bufferRowIndex >= /*CHANGED1024*/ 512)
    {
        return;
    }

    for (int i = 0; i < /*CHANGED1024*/ 512; i++)
    {
        Memory[bufferRowIndex][i] = rowBuffer[i];
    }
    bufferRowIndex = -1;
    Instruction::rowBufferIndex = -1;
}

void DRAM::updateRowBuffer(int columnNum, int val)
{
    rowBuffer[columnNum] = val;
}

bool DRAM::isClashing(int *instr, Instruction &dramInstr)
{
    if (dramInstr.type == 1)
    {
        return false; // sw instruction
    }

    int targetReg = dramInstr.target;
    switch (instr[0])
    {
    case 0:
        if (instr[2] == targetReg)
        {
            return true;
        }
        else
        {
            return false;
        }
        break;
    case 1:
        if (instr[1] == targetReg || instr[2] == targetReg)
        {
            return true;
        }
        else
        {
            return false;
        }
        break;
    case 2:
    case 3:
    case 4:
    case 9:
        for (int k = 1; k <= 3; k++)
        {
            if (instr[k] == targetReg)
            {
                return true;
            }
        }
        return false;
        break;
    case 5:
    case 7:
    case 8:
        if ((instr[1] == targetReg) || (instr[2] == targetReg))
        {
            return true;
        }
        else
        {
            return false;
        }
        break;
    case 6:
        return false;
        break;
    default:
        return true;
        break;
    }
}

// [ASSIGNMENT 4]
bool DRAM::isBlocked(int *instruction)
{
    if (blockingMode)
    {
        return !pendingInstructionsPriority.empty();
    }
    
    for(auto &i: pendingInstructionsPriority) {
        if (isClashing(instruction, *i))
        {
            return true;
        }
    }

    return false;
}

// [ASSIGNMENT 4]
void DRAM::addActivities(Instruction &dramInstr)
{
    if (dramInstr.type == 0)
    {
        // lw instruction
        int targetReg = dramInstr.target;
        int addr = dramInstr.address;

        std::array<int, 4> activity;
        int targetRow = addr / /*CHANGED1024*/ 512;
        int targetCol = addr % /*CHANGED1024*/ 512;

        // Check if row is already loaded into buffer
        if (bufferRowIndex != targetRow)
        {
            // If any row has been activated before we need to write the row back to main memory
            if (bufferRowIndex != -1)
            {
                activity[0] = 1;
                activity[1] = ROW_ACCESS_DELAY;
                pendingActivities.push(activity);
            }

            // Now we copy the row of interest to the row buffer
            activity[0] = 0;
            activity[1] = targetRow;
            activity[2] = ROW_ACCESS_DELAY;
            pendingActivities.push(activity);
        }

        // Copy the data at column offset to the register
        activity[0] = 2;
        activity[1] = targetCol;
        activity[2] = targetReg;
        activity[3] = COL_ACCESS_DELAY;
        pendingActivities.push(activity);
    }
    else
    {
        // sw instruction
        int value = dramInstr.target;
        int address = dramInstr.address;

        std::array<int, 4> activity;
        int row = address / /*CHANGED1024*/ 512;
        int col = address % /*CHANGED1024*/ 512;

        // Check if row is already loaded into buffer
        if (bufferRowIndex != row)
        {
            // If another row is present in buffer, write it back to main memory
            if (bufferRowIndex != -1)
            {
                activity[0] = 1;
                activity[1] = ROW_ACCESS_DELAY;
                pendingActivities.push(activity);
            }

            // Now we copy the row of interest to the row buffer
            activity[0] = 0;
            activity[1] = row;
            activity[2] = ROW_ACCESS_DELAY;
            pendingActivities.push(activity);
        }

        // Update the data at the required address in the row buffer
        activity[0] = 3;
        activity[1] = col;
        activity[2] = value;
        activity[3] = COL_ACCESS_DELAY;
        pendingActivities.push(activity);
    }
}

void DRAM::performActivity()
{
    std::array<int, 4> &act = pendingActivities.front();

    switch (act[0])
    {
    case 0:
    {
        // Copy row to row buffer
        act[2] = act[2] - 1; // Reduce time left by one cycle

        // Check if it has completed
        if (act[2] == 0)
        {
            // execute
            copyToRowBuffer(act[1]);
            // Put this inside the completed activity
            dramCompletedActivity[0] = act[0];
            dramCompletedActivity[1] = act[1];
            // Increment the number of row buffer updates
            rowBufferUpdates++;
            // Remove from pending activities
            pendingActivities.pop();
        }
        break;
    }
    case 1:
    {
        // Writeback row to main memory
        act[1] = act[1] - 1; // Reduce time left by one cycle

        // Check if it has completed
        if (act[1] == 0)
        {
            int rowCopied = bufferRowIndex;
            // execute
            writeback();
            // Put this inside the completed activity
            dramCompletedActivity[0] = act[0];
            dramCompletedActivity[1] = rowCopied;
            // Remove from pending activities
            pendingActivities.pop();
        }
        break;
    }
    case 2:
    {
        // Copy data at the column offset from row buffer into register
        act[3] = act[3] - 1; // Reduce time left by one cycle

        // Check if it has completed
        if (act[3] == 0)
        {
            // Put this inside the completed activity
            dramCompletedActivity[0] = act[0];
            dramCompletedActivity[1] = rowBuffer[act[1]];
            dramCompletedActivity[2] = act[2];
            // Remove from pending activities
            pendingActivities.pop();
            // Remove its instruction from pending instructions
            // [ASSIGNMENT 4]
            deleteCurrentInstruction();
        }
        break;
    }
    case 3:
    {
        // Update data at the column offset in row buffer
        act[3] = act[3] - 1; // Reduce time left by one cycle

        // Check if it has completed
        if (act[3] == 0)
        {
            //execute
            updateRowBuffer(act[1], act[2]);
            // Put this inside the completed activity
            dramCompletedActivity[0] = act[0];
            dramCompletedActivity[1] = ((bufferRowIndex * /*CHANGED1024*/ 512) + act[1]);
            dramCompletedActivity[2] = act[2];
            // Increment the number of row buffer updates
            rowBufferUpdates++;
            // Remove from pending activities
            pendingActivities.pop();
            // Remove its instruction from pending instructions
            deleteCurrentInstruction();
        }
        break;
    }
    default:
    {
        break;
    }
    }
}

void DRAM::executeNext()
{
    for (int i = 0; i < 4; i++)
    {
        dramCompletedActivity[i] = -1;
    }

    if (pendingActivities.empty())
    {
        // Check for pending instructions
        if (pendingInstructionsPriority.empty())
        {
            return;
        }

        // [ASSIGNMENT 4]
        Instruction *nextInstr = scheduleNextInstr();
        currentInstId = nextInstr->id;

        addActivities(*(nextInstr));
    }

    performActivity();
}
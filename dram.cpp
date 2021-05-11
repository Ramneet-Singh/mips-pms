#include <dram.hpp>

using namespace std;

DRAM::DRAM()
{
    blockingMode = true;
    dryrun = false;
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

DRAM::DRAM(int rowAccessDelay, int colAccessDelay, bool blockMode, bool dry)
{
    blockingMode = blockMode;
    dryrun = dry;
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

void DRAM::copyToRowBuffer(int rowIndex)
{
    for (int i = 0; i < NUMCOLS; i++)
    {
        rowBuffer[i] = Memory[rowIndex][i];
    }
    bufferRowIndex = rowIndex;
    Instruction::rowBufferIndex = rowIndex;
}

void DRAM::writeback()
{
    if (bufferRowIndex < 0 || bufferRowIndex >= NUMROWS)
    {
        return;
    }

    for (int i = 0; i < NUMCOLS; i++)
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



// [ASSIGNMENT 4] :: just converted instruction array to object
void DRAM::addActivities(Instruction &dramInstr)
{
    if (dramInstr.type == 0)
    {
        // lw instruction
        int targetReg = dramInstr.target;
        int addr = dramInstr.address;

        std::array<int, 4> activity;
        int targetRow = addr / NUMCOLS;
        int targetCol = addr % NUMCOLS;

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
        int row = address / NUMCOLS;
        int col = address % NUMCOLS;

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
    std::array<int, 5> &act = pendingActivities.front();

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
            // [ASSIGNMENT 4 end]
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
            dramCompletedActivity[1] = ((bufferRowIndex * NUMCOLS) + act[1]);
            dramCompletedActivity[2] = act[2];
            // Increment the number of row buffer updates
            rowBufferUpdates++;
            // Remove from pending activities
            pendingActivities.pop();
            // Remove its instruction from pending instructions
            // [ASSIGNMENT 4]
            deleteCurrentInstruction();
            // [ASSIGNMENT 4 end]
        }
        break;
    }
    default:
    {
        break;
    }
    }
}
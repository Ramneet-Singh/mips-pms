#include <mrm.hpp>

int Instruction::getRowDifference() const
{
    int row_number = address / NUMCOLS;
    if (row_number > rowBufferIndex)
    {
        return row_number - rowBufferIndex;
    }
    else
    {
        return rowBufferIndex - row_number;
    }
}

// bool Instruction::operator==(const Instruction &rhs) const
// {
//     return rhs.id == id;
// }

MRM::MRM(int rDelay, int cDelay, int block, bool p)
{
    rowAccessDelay = rDelay;
    colAccessDelay = cDelay;
    blockingMode = block;
    toPrint = p;
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        buffer[i] = new Instruction(-1, 0, 0, -1, -1);
    }
    dramMemory.setDelays(rowAccessDelay, colAccessDelay);
    schedulingDelayLeft = -1;
}
MRM::~MRM()
{
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        delete buffer[i];
    }
}

bool MRM::isConflicting(Instruction &inst1, Instruction &inst2)
{
    if (inst1.core_no != inst2.core_no)
        return false;
    if (inst1.type == 0)
    {
        if (inst2.type == 0)
        {
            // lw and lw
            if (inst1.target == inst2.target)
            {
                return true;
            }
        }
        else if (inst2.type == 1)
        {
            // lw and sw
            if (inst1.address == inst2.address)
            {
                return true;
            }
        }
    }
    else if (inst1.type == 1)
    {
        if (inst2.type == 0)
        {
            // sw and lw
            if (inst1.address == inst2.address)
            {
                /// FORWARDING ///
                if (inst1.id >= inst2.forwarded_id)
                {
                    inst2.forwarded_value = inst1.target;
                    inst2.forwarded_id = inst1.id;
                }
                return true;
            }
        }
        else if (inst2.type == 1)
        {
            // sw and sw
            if (inst1.address == inst2.address)
            {
                return true;
            }
        }
    }
    return false;
}

int MRM::getEmptyIndex(int cpu_core)
{
    for (int i = cpu_core * BUFFER_SIZE / MAX_CPU_CORES; i < (cpu_core + 1) * BUFFER_SIZE / MAX_CPU_CORES; i++)
    {

        if (buffer[i]->type == -1)
        {
            return i;
        }
    }
    return -1;
}

void MRM::addInstruction(Instruction &inst)
{
    // Fill up dependencies vector by iterating over priority queue

    for (int i = 0; i < BUFFER_SIZE; i++)
    {

        if (isConflicting(*buffer[i], inst))
        {
            inst.dependencies[i] = 1;
        }
    }
    int index = getEmptyIndex(inst.core_no);
    if (index == -1)
    {
        throw "Buffer overflow!";
    }
    else
    {
        buffer[index] = &inst;
        inst.index = index;
    }
}

Instruction *MRM::scheduleNextInstr()
{

    int min_id = INT32_MAX, min_index = -1;
    bool isIndependent = true;
    int min_id_row_same = INT32_MAX, min_index_row_same = -1;

    /// FORWARDING ///
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        isIndependent = true;
        for (int j = 0; j < BUFFER_SIZE; j++)
        {
            if (buffer[i]->dependencies[j] != 0)
                isIndependent = false;
        }

        bool isEmpty = (buffer[i]->type == -1);

        if (isIndependent && !isEmpty)
        {
            if (buffer[i]->isForwarded())
            {
                return buffer[i];
            }
        }
    }

    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        isIndependent = true;
        for (int j = 0; j < BUFFER_SIZE; j++)
        {
            if (buffer[i]->dependencies[j] != 0)
                isIndependent = false;
        }

        bool isEmpty = (buffer[i]->type == -1);

        if (!isEmpty && isIndependent && (buffer[i]->getRowDifference() == 0) && (buffer[i]->id <= min_id_row_same))
        {
            min_id_row_same = buffer[i]->id;
            min_index_row_same = i;
        }
        // std::cout<<isEmpty<<isIndependent<<"here\n";
        // std::cout<<buffer[i]->id<<' '<<min_id<<'\n';
        if ((buffer[i]->id <= min_id) && !isEmpty && isIndependent)
        {
            min_id = buffer[i]->id;
            min_index = i;
        }
    }
    if (min_index_row_same != -1)
    {
        return buffer[min_index_row_same];
    }
    if (min_index != -1)
    {
        return buffer[min_index];
    }

    throw "No Suitable instruction!";
}

void MRM::deleteCurrentInstruction()
{

    int targetInd = currentInstIndex;

    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        buffer[i]->dependencies[targetInd] = 0;
    }

    buffer[targetInd]->type = -1;
}

void MRM::executeNext()
{
    if(toPrint){
        if (isBufferEmpty())
        {
            std::cout << "Row buffer is Empty! \n";
        }
        else
        {
            std::cout << "Instructions in the row buffer: \n";
            for (int i = 0; i < BUFFER_SIZE; i++)
            {
                if (buffer[i]->type != -1)
                    buffer[i]->print();
            }
        }
    }

    for (int i = 0; i < 5; i++)
    {
        dramMemory.dramCompletedActivity[i] = -1;
    }

    if (dramMemory.pendingActivities.empty() && schedulingDelayLeft == -1)
    {
        // Check for pending instructions
        if (isBufferEmpty())
            return;
        Instruction *nextInstr = scheduleNextInstr();
        nextInstIndex = nextInstr->index;
        schedulingDelayLeft = SCHEDULING_DELAY;
    }
    if (dramMemory.pendingActivities.empty())
    {
        if (schedulingDelayLeft == 0)
        {
            currentInstIndex = buffer[nextInstIndex]->index;
            if (buffer[nextInstIndex]->isForwarded())
            {
                dramMemory.dramCompletedActivity[0] = 2;
                dramMemory.dramCompletedActivity[1] = buffer[nextInstIndex]->forwarded_value;
                dramMemory.dramCompletedActivity[2] = buffer[nextInstIndex]->target;
                if(toPrint){
                    std::cout << "Forwarded sw-lw value " << buffer[nextInstIndex]->forwarded_value << " to instruction: ";
                    buffer[nextInstIndex]->print();
                }
                deleteCurrentInstruction();
                schedulingDelayLeft = -1;
                return;
            }
            if(toPrint){
                std::cout << "DRAM request issued | instruction: ";
                buffer[nextInstIndex]->print();
            }
            dramMemory.addActivities(*(buffer[nextInstIndex]));
            schedulingDelayLeft = -1;
        }
        else
        {
            schedulingDelayLeft--;
            return;
        }
    }

    bool instructionOver;
    instructionOver = dramMemory.performActivity();
    if (instructionOver)
    {
        deleteCurrentInstruction();
    }
}

bool MRM::isClashing(int *instr, Instruction &dramInstr, int cpu_core)
{
    if (dramInstr.type == -1)
        return false;
    if (dramInstr.core_no != cpu_core)
        return false;
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

bool MRM::isBufferEmpty()
{
    bool isEmpty = true;
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        if (buffer[i]->type != -1)
            isEmpty = false;
    }
    return isEmpty;
}

bool MRM::isBlocked(int *instruction, int cpu_core)
{
    if (blockingMode)
    {
        return !isBufferEmpty();
    }

    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        if (isClashing(instruction, *buffer[i], cpu_core))
        {
            if(toPrint){
                std::cout << "|  Instruction dependencies unsatisfied. Stalling...\n";
                std::cout << "|  Dependent Instruction: ";
                buffer[i]->print();
            }
            return true;
        }
    }

    return false;
}

void MRM::getDramActivity(int *ar)
{
    for (int i = 0; i < 5; i++)
    {
        ar[i] = dramMemory.dramCompletedActivity[i];
    }
}

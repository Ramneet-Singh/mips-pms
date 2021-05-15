#include <mrm.hpp>



// int Instruction::getRowDifference() const
// {
//     int row_number = address / NUMCOLS;
//     if (row_number > rowBufferIndex)
//     {
//         return row_number - rowBufferIndex;
//     }
//     else
//     {
//         return rowBufferIndex - row_number;
//     }
// }

// bool Instruction::operator==(const Instruction &rhs) const
// {
//     return rhs.id == id;
// }

MRM::MRM(int block){
    blockingMode = block;
    for (int i  = 0; i<BUFFER_SIZE; i++){
        buffer[i] = new Instruction(-1, 0, 0, -1);
    }
}
MRM::~MRM(){
    for (int i  = 0; i<BUFFER_SIZE; i++){
        delete buffer[i];
    }
}

bool MRM::isConflicting(Instruction &inst1, Instruction &inst2)
{
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
        else
        {
            // lw and sw
            if (inst1.address == inst2.address)
            {
                return true;
            }
        }
    }
    else
    {
        if (inst2.type == 0)
        {
            // sw and lw
            if (inst1.address == inst2.address)
            {
                return true;
            }
        }
        else
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

int MRM::getEmptyIndex(){
    for(int i = 0; i<BUFFER_SIZE; i++){
        
        if(buffer[i]->type == -1){
            return i;
        }
    }
    return -1;
}

void MRM::addInstruction(Instruction &inst)
{
    // Fill up dependencies vector by iterating over priority queue
    for(int i = 0; i<BUFFER_SIZE; i++){
        
        if (isConflicting(*buffer[i], inst))
        {
            inst.dependencies[i] = 1;
        }
    }
    int index = getEmptyIndex();
    if(index == -1){
        throw "Buffer overflow!";
    }
    else{
        buffer[index] = &inst;
    }
}


Instruction *MRM::scheduleNextInstr()
{
    int min_id = buffer[0]->id, min_index = -1;
    for(int i = 0; i<BUFFER_SIZE; i++){
        bool isIndependent = true;
        for(int j = 0; j< BUFFER_SIZE; j++){
            if(buffer[i]->dependencies[j] != 0)
                isIndependent = false;
        }

        bool isEmpty = (buffer[i]->type == -1);
        // std::cout<<isEmpty<<isIndependent<<"here\n";
        // std::cout<<buffer[i]->id<<' '<<min_id<<'\n';
        if((buffer[i]->id<=min_id) && !isEmpty && isIndependent){
            min_id = buffer[i]->id;
            min_index = i;
            std::cout<<"here\n";
        }
    }
    if(min_index == -1){
        
        throw "No independent instruction!";
    }
    return buffer[min_index];
}

void MRM::deleteCurrentInstruction()
{

    int targetInd = currentInstIndex;

    for(int i = 0; i<BUFFER_SIZE; i++){
        buffer[i]->dependencies[targetInd] = 0;
    }

    buffer[targetInd]->type = -1;
}

void MRM::executeNext()
{
    for (int i = 0; i < 4; i++)
    {
        dramMemory.dramCompletedActivity[i] = -1;
    }

    if (dramMemory.pendingActivities.empty())
    {
        // Check for pending instructions
        if(isBufferEmpty())
            return;
        Instruction *nextInstr = scheduleNextInstr();
        currentInstIndex = nextInstr->index;
        deleteCurrentInstruction();
        dramMemory.addActivities(*(nextInstr));
    }
    dramMemory.performActivity();
        
}


bool MRM::isClashing(int *instr, Instruction &dramInstr, int cpu_core)
{
    if(dramInstr.core_no != cpu_core)
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

bool MRM::isBufferEmpty(){
    bool isEmpty = true;
    for(int i = 0; i<BUFFER_SIZE; i++){
        if(buffer[i]->type != -1)
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

    for (int i = 0; i<BUFFER_SIZE; i++)
    {
        if (isClashing(instruction, *buffer[i], cpu_core))
            return true;
    }

    return false;
}

void MRM::getDramActivity(int * ar){
    for(int i = 0; i<4; i++){
        ar[i] = dramMemory.dramCompletedActivity[i];
    }
}
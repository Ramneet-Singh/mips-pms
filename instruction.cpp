#include <dram.hpp>

using namespace std;

// [ASSIGNMENT 4]

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

bool Instruction::operator==(const Instruction &rhs) const
{
    return rhs.id == id;
}

bool DRAM::isConflicting(Instruction &inst1, Instruction &inst2)
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

void DRAM::addInstruction(Instruction &inst)
{
    // Fill up dependencies vector by iterating over priority queue
    for (auto &i : pendingInstructionsPriority)
    {
        // (*i).to_string();
        if (isConflicting(*i, inst))
        {

            inst.dependencies.push_back(new Instruction((*i).id, (*i).address, (*i).target, (*i).type));
        }
    }
    pendingInstructionsPriority.push(&inst);
}


Instruction *DRAM::scheduleNextInstr()
{
    queue<Instruction *> auxilliary;
    Instruction *instPtr;
    instPtr = pendingInstructionsPriority.top();
    while (!instPtr->dependencies.empty())
    {
        auxilliary.push(instPtr);
        pendingInstructionsPriority.pop();
        if (pendingInstructionsPriority.empty())
        {
            cerr << "ERROR: There are no instructions with empty dependencies!" << endl;
            exit(EXIT_FAILURE);
        }
        instPtr = pendingInstructionsPriority.top();
    }

    // assert: We have found the lowest priority instruction with empty dependencies
    // Insert the popped instructions back into the priority queue
    Instruction *auxPtr;
    while (!auxilliary.empty())
    {
        auxPtr = auxilliary.front();
        pendingInstructionsPriority.push(auxPtr);
        auxilliary.pop();
    }

    return instPtr;
}

void DRAM::deleteCurrentInstruction()
{
    int targetId = currentInstId;

    Instruction *deleteInstPtr;
    queue<Instruction *> auxilliary;
    while (!pendingInstructionsPriority.empty())
    {
        
        Instruction *instPtr;
        bool found = false;
        instPtr = pendingInstructionsPriority.top();
        do{

            found = false;
            vector<Instruction *>::iterator deleteIt;
            vector<Instruction *>::iterator it = instPtr->dependencies.begin();
            while (it != instPtr->dependencies.end())
            {
                if ((*it)->id == currentInstId)
                {
                    found = true;
                    break;
                }
                it++;
            }
            deleteIt = it;
            if (deleteIt != instPtr->dependencies.end())
            {
                instPtr->dependencies.erase(deleteIt);
            }
        }
        while(found);

        if (instPtr->id != targetId)
        {
            auxilliary.push(instPtr);
        }
        else
        {
            deleteInstPtr = instPtr;
        }
        pendingInstructionsPriority.pop();
    }

    for (auto it = deleteInstPtr->dependencies.begin(); it != deleteInstPtr->dependencies.end(); it++)
    {
        delete (*it);
    }
    deleteInstPtr->dependencies.clear();

    delete deleteInstPtr;

    Instruction *auxPtr;
    while (!auxilliary.empty())
    {
        auxPtr = auxilliary.front();
        pendingInstructionsPriority.push(auxPtr);
        auxilliary.pop();
    }
}

bool DRAM::willPerformWritebackNext()
{
    if (!pendingActivities.empty())
    {
        return false;
    }
    if (pendingInstructionsPriority.empty())
    {
        return false;
    }

    queue<Instruction *> auxilliary;
    Instruction *instPtr;
    instPtr = pendingInstructionsPriority.top();
    while (!instPtr->dependencies.empty())
    {
        auxilliary.push(instPtr);
        pendingInstructionsPriority.pop();
        if (pendingInstructionsPriority.empty())
        {
            cerr << "ERROR: There are no instructions with empty dependencies!" << endl;
            exit(EXIT_FAILURE);
        }
        instPtr = pendingInstructionsPriority.top();
    }

    // assert: We have found the lowest priority instruction with empty dependencies
    // Insert the popped instructions back into the priority queue
    Instruction *auxPtr;
    while (!auxilliary.empty())
    {
        auxPtr = auxilliary.front();
        pendingInstructionsPriority.push(auxPtr);
        auxilliary.pop();
    }
    bool isDiffRow = ((instPtr->address / NUMCOLS) != bufferRowIndex) && (bufferRowIndex != -1);
    pendingInstructionsPriority.push(instPtr);
    return isDiffRow;
}

void DRAM::deleteAllDryrunInst()
{
    for (auto &i : pendingInstructionsPriority)
    {
        for (auto it = i->dependencies.begin(); it != i->dependencies.end(); it++)
        {
            delete (*it);
        }
        i->dependencies.clear();
        delete i;
    }
}



DRAM::DRAM(DRAM &other)
{
    blockingMode = other.blockingMode;
    for (int i = 0; i < NUMROWS; i++)
    {
        for (int j = 0; j < NUMCOLS; j++)
        {
            Memory[i][j] = other.Memory[i][j];
        }
    }
    for (int k = 0; k < NUMCOLS; k++)
    {
        rowBuffer[k] = other.rowBuffer[k];
    }
    bufferRowIndex = other.bufferRowIndex;
    currentInstId = other.currentInstId;
    rowBufferUpdates = other.rowBufferUpdates;
    ROW_ACCESS_DELAY = other.ROW_ACCESS_DELAY;
    COL_ACCESS_DELAY = other.COL_ACCESS_DELAY;
    pendingActivities = other.pendingActivities;
    for (int i = 0; i < 3; i++)
    {
        dramCompletedActivity[i] = other.dramCompletedActivity[i];
    }
    dryrun = other.dryrun;

    for (auto &i : other.pendingInstructionsPriority)
    {
        pendingInstructionsPriority.push(new Instruction(*i));
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
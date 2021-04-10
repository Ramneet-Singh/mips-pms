#include <dram.hpp>

using namespace std;

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

void DRAM::addInstruction(Instruction &inst)
{
    // Fill up dependencies vector by iterating over priority queue
    for (auto &i : pendingInstructionsPriority)
    {
        if (isConflicting(*i, inst))
        {
            inst.dependencies.push_back(i);
        }
    }
    pendingInstructionsPriority.push(&inst);
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
        instPtr = pendingInstructionsPriority.top();
        vector<Instruction *>::iterator deleteIt;
        vector<Instruction *>::iterator it = instPtr->dependencies.begin();
        while (it != instPtr->dependencies.end())
        {
            if ((*it)->id == currentInstId)
            {
                break;
            }
            it++;
        }
        deleteIt = it;
        if (deleteIt != instPtr->dependencies.end())
        {
            instPtr->dependencies.erase(deleteIt);
        }

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

    delete deleteInstPtr;

    Instruction *auxPtr;
    while (!auxilliary.empty())
    {
        auxPtr = auxilliary.front();
        pendingInstructionsPriority.push(auxPtr);
        auxilliary.pop();
    }
}
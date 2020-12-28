
#ifndef RISCV_SIM_EXECUTOR_H
#define RISCV_SIM_EXECUTOR_H

#include "Instruction.h"

class Executor
{
public:
    void Execute(InstructionPtr& instr, Word ip)
    {
        Word val2;
//        if (!instr->_src1.has_value()){
//            instr->_nextIp = ip + 4;
//            return;
//        }
        if (instr->_imm.has_value()) {
            val2 = instr->_imm.value();
        } else {
            val2 = instr->_src2Val;
        }

        switch (instr->_type) {
            case IType::Alu:
                instr->_data = ChAluFunc(instr->_aluFunc, instr->_src1Val, val2);
                break;
            case IType::Csrr:
                instr->_data = instr->_csrVal;
                break;
            case IType::Csrw:
                instr->_data = instr->_src1Val;
                break;
            case IType::St:
                instr->_data = instr->_src2Val;
                instr->_addr = ChAluFunc(instr->_aluFunc, instr->_src1Val, val2);
                break;
            case IType::J:
            case IType::Jr:
                instr->_data = ip + 4;
                break;
            case IType::Auipc:
                instr->_data = ip + instr->_imm.value();
                break;
            case IType::Ld:
                instr->_addr = ChAluFunc(instr->_aluFunc, instr->_src1Val, val2);
                break;
        }



        if (ChBrFunc(instr->_brFunc, instr->_src1Val, instr->_src2Val))
        {
            switch (instr->_type) {
                case IType::Br:
                case IType::J:
                    instr->_nextIp = ip + instr->_imm.value();
                    break;
                case IType::Jr:
                    instr->_nextIp = instr->_imm.value() + instr->_src1Val;
                    break;
            }
        }
        else
        {
            instr->_nextIp = ip + 4;
        }
    }

private:
    Word Add(Word a, Word b) {
        return a + b;
    }

    Word Sub(Word a, Word b) {
        return a - b;
    }

    Word And(Word a, Word b) {
        return a & b;
    }

    Word Or(Word a, Word b) {
        return a | b;
    }

    Word Xor(Word a, Word b) {
        return a ^ b;
    }

    Word Slt(Word a, Word b) {
        return (signed)a < (signed)b;
    }

    Word Sltu(Word a, Word b) {
        return a < b;
    }

    Word Sll(Word a, Word b){
        b = b % 32;
        return a << b;
    }

    Word Srl(Word a, Word b) {
        b = b % 32;
        return a >> b;
    }

    Word Sra(Word a, Word b) {
        b = b % 32;
        return (signed)a >> (signed)b;
    }

    // brunch

    Word ChAluFunc(AluFunc aluFunc, Word a, Word b) { //ссылка?
        switch (aluFunc) {
            case AluFunc::Add:
                return Add(a, b);
            case AluFunc::Sub:
                return Sub(a, b);
            case AluFunc::And:
                return And(a, b);
            case AluFunc::Or:
                return Or(a, b);
            case AluFunc::Xor:
                return Xor(a, b);
            case AluFunc::Slt:
                return Slt(a, b);
            case AluFunc::Sltu:
                return Sltu(a, b);
            case AluFunc::Sll:
                return Sll(a, b);
            case AluFunc::Srl:
                return Srl(a, b);
            case AluFunc::Sra:
                return Sra(a, b);
        }
    }

    bool ChBrFunc(BrFunc brFunc, Word a, Word b) {
        switch (brFunc) {
            case BrFunc::Eq:
                return a == b;
            case BrFunc::Neq:
                return a != b;
            case BrFunc::Lt:
                return (signed)a < (signed)b;
            case BrFunc::Ltu:
                return a < b;
            case BrFunc::Ge:
                return (signed)a >= (signed)b;
            case BrFunc::Geu:
                return a >= b;
            case BrFunc::AT:
                return true;
            case BrFunc::NT:
                return false;
        }
    }
};

#endif // RISCV_SIM_EXECUTOR_H

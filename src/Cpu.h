
#ifndef RISCV_SIM_CPU_H
#define RISCV_SIM_CPU_H

#include "Memory.h"
#include "Decoder.h"
#include "RegisterFile.h"
#include "CsrFile.h"
#include "Executor.h"
#include <optional>
class Cpu
{
public:
    Cpu(IMem& mem)
            : _mem(mem)
    {
        status=Status::Request;
    }
    void Clock()
    {
        //std::cout << "Start";
        _csrf.Clock();
        switch (status) {
            case Status::Request:
                _mem.Request(_ip);
                status=Status::Read;
            case Status::Read:
                instr=_mem.Response();
                if(!instr.has_value()) { break;}
                currInstr=_decoder.Decode(instr.value());
                _csrf.Read(currInstr);
                _rf.Read(currInstr);// чтение из регистров
                _exe.Execute(currInstr, _ip); //выполнение
                _mem.Request(currInstr);
                status=Status::Write;

            case Status::Write:
                if(!_mem.Response(currInstr)){ break;}
                _rf.Write(currInstr);
                _csrf.Write(currInstr);
                _csrf.InstructionExecuted();
                _ip=currInstr->_nextIp;
                status=Status::Request;
        }
    }

    void Reset(Word ip)
    {
        _csrf.Reset();
        _ip = ip;
    }

    std::optional<CpuToHostData> GetMessage()
    {
        return _csrf.GetMessage();
    }

private:
    Reg32 _ip;
    Decoder _decoder;
    RegisterFile _rf;
    CsrFile _csrf;
    Executor _exe;
    IMem& _mem;
    // Add your code here, if needed
    enum class Status{
        Request,
        Read,
        Write,
    };
    Status status;
    InstructionPtr currInstr;
    std::optional<Word> instr;////////
};


#endif //RISCV_SIM_CPU_H

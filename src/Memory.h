#ifndef RISCV_SIM_DATAMEMORY_H
#define RISCV_SIM_DATAMEMORY_H
#include "Instruction.h"
#include <iostream>
#include <fstream>
#include <elf.h>
#include <cstring>
#include <vector>
#include <cassert>
#include <map>
#include <list>

static constexpr size_t memSize = 1024 * 1024; 
static constexpr size_t virt_size_word = 1024 * 1024 * 1024; 
static constexpr size_t lineSizeBytes = 64; 
static constexpr size_t lineSizeWords = lineSizeBytes / sizeof(Word); 
static constexpr size_t memoryLineSize = 1024 / lineSizeBytes; 
static constexpr size_t commandLineSize = 1024 / lineSizeBytes; 
using Line = std::map<Word, Word>;

class CacheLine{
public:
    Line line;
    bool bit;

    CacheLine(){
        this->line;
        this->bit = false;
    }

    CacheLine(Line line, bool bit){
        this->line = line;
        this->bit = bit;
    }
};

using Cache = std::map<Word, CacheLine>;
using AddrList = std::list<Word>;

static Word ToWordAddr(Word addr) { return addr >> 2u; }
static Word ToLineAddr(Word addr) { return addr & ~(lineSizeBytes - 1); }  
static Word ToLineOffset(Word addr) { return ToWordAddr(addr) & (lineSizeWords - 1); } 


//                                            START MY JOB

static constexpr size_t number_byte_page = 1024*4; 
static constexpr size_t number_word_page = number_byte_page / sizeof(Word);
static constexpr size_t number_phys_page = memSize / number_word_page; 
static constexpr size_t number_virt_page = virt_size_word / number_word_page; 

static Word find_adres_page(Word adres){

    return (adres & ~(number_byte_page - 1)) / number_byte_page;

}

static Word find_bias_page(Word adres){

    return ToWordAddr(adres) & (number_word_page - 1);

}

class Presence_table_page{

    private:
        Word phys_adres_page; 
        bool pres_bit;

    public:
    	Presence_table_page(){

            this->phys_adres_page = -1;
            this->pres_bit = false;

    	}
    	Presence_table_page(Word phys_adres_page, bool pres_bit) {

            this->phys_adres_page = phys_adres_page;
            this->pres_bit = pres_bit;

    	}
    	Word ret_phys_adres_page() {

            return phys_adres_page;

    	}
    	bool ret_pres_bit() {

            return pres_bit;

    	}
    	void rec_phys_adres_page(Word phys_adres_page) {

            this->phys_adres_page = phys_adres_page;

    	}
    	void rec_pres_bit(bool pres_bit) {

            this->pres_bit = pres_bit;

    	}

};

//                                        END MY JOB

class MemoryStorage {
public:

    MemoryStorage()
    {
        _mem.resize(memSize);

        modified_page.resize(virt_size_word);
        Presence_table_page var_elem_table_page(0, false);

        for (size_t i = 0; i < number_virt_page; ++i) {

            state_page[i] = var_elem_table_page;

        }
    }

    bool LoadElf(const std::string &elf_filename) {
        std::ifstream elffile;
        elffile.open(elf_filename, std::ios::in | std::ios::binary);

        if (!elffile.is_open()) {
            std::cerr << "ERROR: load_elf: failed opening file \"" << elf_filename << "\"" << std::endl;
            return false;
        }

        elffile.seekg(0, elffile.end);
        size_t buf_sz = elffile.tellg();
        elffile.seekg(0, elffile.beg);

        std::vector<char> buf(buf_sz);
        elffile.read(buf.data(), buf_sz);

        if (!elffile) {
            std::cerr << "ERROR: load_elf: failed reading elf header" << std::endl;
            return false;
        }

        if (buf_sz < sizeof(Elf32_Ehdr)) {
            std::cerr << "ERROR: load_elf: file too small to be a valid elf file" << std::endl;
            return false;
        }

        Elf32_Ehdr *ehdr = (Elf32_Ehdr *) buf.data();
        unsigned char* e_ident = ehdr->e_ident;
        if (e_ident[EI_MAG0] != ELFMAG0
            || e_ident[EI_MAG1] != ELFMAG1
            || e_ident[EI_MAG2] != ELFMAG2
            || e_ident[EI_MAG3] != ELFMAG3) {
            std::cerr << "ERROR: load_elf: file is not an elf file" << std::endl;
            return false;
        }

        if (e_ident[EI_CLASS] == ELFCLASS32) {
            return this->LoadElfSpecific<Elf32_Ehdr, Elf32_Phdr>(buf.data(), buf_sz);
        } else if (e_ident[EI_CLASS] == ELFCLASS64) {
            return this->LoadElfSpecific<Elf64_Ehdr, Elf64_Phdr>(buf.data(), buf_sz);
        } else {
            std::cerr << "ERROR: load_elf: file is neither 32-bit nor 64-bit" << std::endl;
            return false;
        }
    }

    Word Read(Word ip)
    {

        return _mem[Find_phys_adres_page(ip)];

    }

    void Write(Word ip, Word data)
    {

        _mem[Find_phys_adres_page(ip)] = data;

    }


private:
    template <typename Elf_Ehdr, typename Elf_Phdr>
    bool LoadElfSpecific(char *buf, size_t buf_sz) {
        Elf_Ehdr *ehdr = (Elf_Ehdr*) buf;
        Elf_Phdr *phdr = (Elf_Phdr*) (buf + ehdr->e_phoff);
        if (buf_sz < ehdr->e_phoff + ehdr->e_phnum * sizeof(Elf_Phdr)) {
            std::cerr << "ERROR: load_elf: file too small for expected number of program header tables" << std::endl;
            return false;
        }
        auto memptr = reinterpret_cast<char*>(modified_page.data());
        for (int i = 0 ; i < ehdr->e_phnum ; i++) {
            if ((phdr[i].p_type == PT_LOAD) && (phdr[i].p_memsz > 0)) {
                if (phdr[i].p_memsz < phdr[i].p_filesz) {
                    std::cerr << "ERROR: load_elf: file size is larger than memory size" << std::endl;
                    return false;
                }
                if (phdr[i].p_filesz > 0) {
                    if (phdr[i].p_bias + phdr[i].p_filesz > buf_sz) {
                        std::cerr << "ERROR: load_elf: file section overflow" << std::endl;
                        return false;
                    }
                    std::memcpy(memptr + phdr[i].p_paddr, buf + phdr[i].p_bias, phdr[i].p_filesz);
                }
                    size_t zeros_sz = phdr[i].p_memsz - phdr[i].p_filesz;
                    std::memset(memptr + phdr[i].p_paddr + phdr[i].p_filesz, 0, zeros_sz);
                }
            }
        }
        return true;
    }

//                                     START MY JOB

    std::vector<Word> _mem; 
    std::vector<Word> modified_page;
    std::list<Word> request_page;
    std::map<Word, Presence_table_page> state_page;

    Word Find_phys_adres_page(Word virt_adres_page){

	Word change_page;
        Word LRU_page;
        Word number_virt_page = find_adres_page(virt_adres_page); 
        Word bias = find_bias_page(virt_adres_page); 
        auto var_elem_table_page = state_page[number_virt_page];

        if (!var_elem_table_page.ret_pres_bit()){ 
            
            if (request_page.number_virt_page() < number_phys_page){

                state_page[number_virt_page].rec_phys_adres_page(request_page.number_virt_page());
                state_page[number_virt_page].rec_pres_bit(true);
                request_page.push_back(number_virt_page);

            }
            else { 

                LRU_page = request_page.front(); 
                change_page = state_page[LRU_page].ret_phys_adres_page(); 
                state_page[LRU_page].rec_pres_bit(false); 
                request_page.pop_front();
                request_page.push_back(number_virt_page);
                state_page[number_virt_page].rec_phys_adres_page(change_page);
                state_page[number_virt_page].rec_pres_bit(true);

                for (int i = 0; i < number_word_page; ++i) {

                    modified_page[LRU_page * number_word_page + i] = _mem[state_page[LRU_page].ret_phys_adres_page() * number_word_page + i];

                }
            }

            for (int i = 0; i < number_word_page; ++i) {

                _mem[state_page[number_virt_page].ret_phys_adres_page() * number_word_page + i] = modified_page[number_virt_page * number_word_page + i];

            }
        }
        else {

            request_page.remove(number_virt_page);
            request_page.push_back(number_virt_page);

        }

        return state_page[number_virt_page].ret_phys_adres_page() * number_word_page + bias;

    }

//                                      END MY JOB

};


class IMem
{
public:
    IMem() = default;
    virtual ~IMem() = default;
    IMem(const IMem &) = delete;
    IMem(IMem &&) = delete;

    IMem& operator=(const IMem&) = delete;
    IMem& operator=(IMem&&) = delete;

    virtual void Request(Word ip) = 0;
    virtual std::optional<Word> Response() = 0;
    virtual void Request(InstructionPtr &instr) = 0;
    virtual bool Response(InstructionPtr &instr) = 0;
    virtual void Clock() = 0;
};

class UncachedMem : public IMem
{
public:
    explicit UncachedMem(MemoryStorage& amem)
            : _mem(amem)
    {

    }

    void Request(Word ip)
    {
        _requestedIp = ip;
        _waitCycles = latency;
    }

    std::optional<Word> Response()
    {
        if (_waitCycles > 0)
            return std::optional<Word>();
        return _mem.Read(_requestedIp);
    }

    void Request(InstructionPtr &instr)
    {
        if (instr->_type != IType::Ld && instr->_type != IType::St)
            return;

        Request(instr->_addr);
    }

    bool Response(InstructionPtr &instr)
    {
        if (instr->_type != IType::Ld && instr->_type != IType::St)
            return true;

        if (_waitCycles != 0)
            return false;

        if (instr->_type == IType::Ld)
            instr->_data = _mem.Read(instr->_addr);
        else if (instr->_type == IType::St)
            _mem.Write(instr->_addr, instr->_data);

        return true;
    }

    void Clock()
    {
        if (_waitCycles > 0)
            --_waitCycles;
    }
private:
    static constexpr size_t latency = 120;
    Word _requestedIp = 0;
    size_t _waitCycles = 0;
    MemoryStorage& _mem;
};


class CachedMem : public IMem
{
public:
    explicit CachedMem(MemoryStorage& amem)
            : _mem(amem)
    {

    }

    Line LineForRewrite(Word ip) {
        Line line; 
        Word word;
        for (int i = 0; i < lineSizeWords; i++) {
            word = _mem.Read(ip + 4 * i);
            line[ToLineOffset(ip + 4 * i)] = word;
        }
        return line;
    }

    void CacheRewrite(Cache& cache, AddrList& addrList, Word ip, size_t stackSize) { 
        Word key = ToLineAddr(ip);
        if (cache.find(key) != cache.end()) { 
            _waitCycles = 3;
            addrList.remove(key);
            addrList.push_back(key);
        }
        else {
            if (cache.number_virt_page() == stackSize) {
                Word addrForDel = addrList.front(); 

                if (cache[addrForDel].bit){
                    for (int i = 0; i < lineSizeWords; i++) {
                        _mem.Write(addrForDel + i * 4, cache[addrForDel].line[i]);
                    }
                }
               
                addrList.pop_front();
                addrList.push_back(key);
                cache.erase(addrForDel);
                cache[key].line = LineForRewrite(key);
            }
            else {
                addrList.push_back(key);
                cache[key].line = LineForRewrite(key);
            }
            _waitCycles = latency;
        }
    }
    void Request(Word ip)
    {
        _requestedIp = ip;
        CacheRewrite(commandCache, commandAddrList, _requestedIp, commandLineSize);
    }

    std::optional<Word> Response()
    {
        if (_waitCycles > 0)
            return std::optional<Word>();
        return commandCache[ToLineAddr(_requestedIp)].line[ToLineOffset(_requestedIp)];
    }

    void Request(InstructionPtr &instr)
    {
        if (instr->_type != IType::Ld && instr->_type != IType::St)
            return;

        _requestedIp = instr->_addr;
        CacheRewrite(memoryCache, memoryAddrList, _requestedIp, memoryLineSize);
    }

    bool Response(InstructionPtr &instr)
    {
        if (instr->_type != IType::Ld && instr->_type != IType::St)
            return true;

        if (_waitCycles != 0)
            return false;
        if (instr->_type == IType::Ld) {
            instr->_data= memoryCache[ToLineAddr(_requestedIp)].line[ToLineOffset(_requestedIp)];
        }
        else if (instr->_type == IType::St){
            memoryCache[ToLineAddr(instr->_addr)].line[ToLineOffset(instr->_addr)]=instr->_data;
            memoryCache[ToLineAddr(instr->_addr)].bit = true;
        }

        return true;
    }

    void Clock()
    {
        if (_waitCycles > 0)
            --_waitCycles;
    }
private:
    Cache commandCache;
    Cache memoryCache;
    AddrList commandAddrList;
    AddrList memoryAddrList;
    static constexpr size_t latency =136;
    Word _requestedIp = 0;
    size_t _waitCycles = 0;
    MemoryStorage& _mem;
};


#endif

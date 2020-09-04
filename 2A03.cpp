#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "NES.h"

// Initialization
NES_Cpu::NES_Cpu() {
	// Initialize values
	pc = 0x0000;						
	opcode = 0x00;
	sp = STACK_OFFSET + 0x00;			// Decrements instead of increment: https://www.youtube.com/watch?v=fWqBmmPQP40
	accumulator = 0x00;
	X = 0x00;
	Y = 0x00;
	proc_status = 0x00;

	memset(memory, 0, 0xFFFF);			// Clear memory
}

// Destruction
NES_Cpu::~NES_Cpu() {

}

/*
	typedef struct instruction_entry {
			std::string instr_name;
			int (NES_Cpu::*operation)();
			int (NES_Cpu::*addr_setup)();
			unsigned int cycles;
		} instruction_entry;
*/

// Emulation cycling
void NES_Cpu::cycle() {

	// Get opcode
	opcode = memory[pc];

	// Retrieve the instruction details, instruction_entry in format:
	// {
	//	std::string instr_name;
	//	int (NES_Cpu::*operation)();
	//	int (NES_Cpu::*addr_setup)();
	//	unsigned int cycles;
	// }
	instruction_entry *instruction = &instruction_table[opcode];

	// Pointer member details in link
	// https://docs.microsoft.com/en-us/cpp/cpp/pointer-to-member-operators-dot-star-and-star?view=vs-2019
	// https://stackoverflow.com/questions/6586205/what-are-the-pointer-to-member-and-operators-in-c

	// Setup addressing mode variables
	(this->*instruction_table[opcode].addr_setup)();

	// Run the operation for the appropriate amount of cycles
	(this->*instruction_table[opcode].operation)();

	/*
		Note on cycle counting. I may count the cycles through timing the time needed for the operation and then sleeping
		off the rest of the time. Otherwise I could do the opposite and wait for a certain amount of time before I start
		the instruction
	*/

	// Increment program counter
	pc++;
}


/*
	Instruction implementations are here

	Returns 0 upon success and returns 1 when requiring an additional clock cycle.
*/
int NES_Cpu::ADC() {

	return 0;
}

int NES_Cpu::AND() {

	return 0;
}

int NES_Cpu::ASL() {

	return 0;
}

int NES_Cpu::BCC() {

	return 0;
}

int NES_Cpu::BCS() {

	return 0;
}

int NES_Cpu::BEQ() {

	return 0;
}

int NES_Cpu::BIT() {

	return 0;
}

int NES_Cpu::BMI() {

	return 0;
}

int NES_Cpu::BNE() {

	return 0;
}

int NES_Cpu::BPL() {

	return 0;
}

int NES_Cpu::BRK() {

	return 0;
}

int NES_Cpu::BVC() {

	return 0;
}

int NES_Cpu::BVS() {

	return 0;
}

int NES_Cpu::CLC() {

	return 0;
}

int NES_Cpu::CLD() {

	return 0;
}

int NES_Cpu::CLI() {

	return 0;
}

int NES_Cpu::CLV() {

	return 0;
}

int NES_Cpu::CMP() {

	return 0;
}

int NES_Cpu::CPX() {

	return 0;
}

int NES_Cpu::CPY() {

	return 0;
}

int NES_Cpu::DEC() {

	return 0;
}

int NES_Cpu::DEX() {

	return 0;
}

int NES_Cpu::DEY() {

	return 0;
}

int NES_Cpu::EOR() {

	return 0;
}

int NES_Cpu::INC() {

	return 0;
}

int NES_Cpu::INX() {

	return 0;
}

int NES_Cpu::INY() {

	return 0;
}

int NES_Cpu::JMP() {

	return 0;
}

int NES_Cpu::JSR() {

	return 0;
}

int NES_Cpu::LDA() {

	return 0;
}

int NES_Cpu::LDX() {

	return 0;
}

int NES_Cpu::LDY() {

	return 0;
}

int NES_Cpu::LSR() {

	return 0;
}

int NES_Cpu::NOP() {

	return 0;
}

int NES_Cpu::ORA() {

	return 0;
}

int NES_Cpu::PHA() {

	return 0;
}

int NES_Cpu::PHP() {

	return 0;
}

int NES_Cpu::PLA() {

	return 0;
}

int NES_Cpu::PLP() {

	return 0;
}

int NES_Cpu::ROL() {

	return 0;
}

int NES_Cpu::ROR() {

	return 0;
}

int NES_Cpu::RTI() {

	return 0;
}

int NES_Cpu::RTS() {

	return 0;
}

int NES_Cpu::SBC() {

	return 0;
}

int NES_Cpu::SEC() {

	return 0;
}

int NES_Cpu::SED() {

	return 0;
}

int NES_Cpu::SEI() {

	return 0;
}

int NES_Cpu::STA() {

	return 0;
}

int NES_Cpu::STX() {

	return 0;
}

int NES_Cpu::STY() {

	return 0;
}

int NES_Cpu::TAX() {

	return 0;
}

int NES_Cpu::TAY() {

	return 0;
}

int NES_Cpu::TSX() {

	return 0;
}

int NES_Cpu::TXA() {

	return 0;
}

int NES_Cpu::TXS() {

	return 0;
}

int NES_Cpu::TYA() {

	return 0;
}

// Illegal Instruction
int NES_Cpu::ILL() {

	// Right now just do nothing
	
	return 0;
}


/*
	Address mode implementations are here
*/

int NES_Cpu::ACC() {

	return 0;
}

int NES_Cpu::ABS() {

	return 0;
}

int NES_Cpu::ABS_X() {

	return 0;
}

int NES_Cpu::ABS_Y() {

	return 0;
}

int NES_Cpu::IMM() {

	return 0;
}

int NES_Cpu::IMP() {

	return 0;
}

int NES_Cpu::IND() {

	return 0;
}

int NES_Cpu::IND_X() {

	return 0;
}

int NES_Cpu::IND_Y() {

	return 0;
}

int NES_Cpu::REL() {

	return 0;
}

int NES_Cpu::ZPG() {

	return 0;
}

int NES_Cpu::ZPG_X() {

	return 0;
}

int NES_Cpu::ZPG_Y() {

	return 0;
}

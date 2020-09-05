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

	use_accumulator = 0;
	target_address = 0x0000;

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

	// Flag setup
	use_accumulator = 0;

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
	Interrupt implementations are here
*/
void NES_Cpu::irq() {

}

void NES_Cpu::nmi() {

}

void NES_Cpu::reset() {

}


/*
	Instruction implementations are here

	Returns 0 upon success and returns 1 when requiring an additional clock cycle.
*/

/*
	N Z C I D V
    + + + - - +
*/
int NES_Cpu::ADC() {

	// Add the accumulator with the data at the target address and the carry bit
	unsigned short sum = accumulator + memory[target_address] + ((proc_status & CARRY_FLAG) ? 1 : 0);

	proc_status &= ~(CARRY_FLAG | ZERO_FLAG | NEGATIVE_FLAG | OVERFLOW_FLAG);
	
	// Check flags
	if (sum >= 0x0100) { // Carry flag 
		proc_status |= CARRY_FLAG;
	}

	if ((sum & 0x00FF) == 0) { // Zero flag
		proc_status |= ZERO_FLAG;
	}

	if (sum & 0x0080) { // Negative flag
		proc_status |= NEGATIVE_FLAG;
	}

	// Explanation for signed overflow flags: http://forums.nesdev.com/viewtopic.php?t=6331
	if (!((accumulator ^ memory[target_address]) & 0x80) && ((accumulator ^ sum) & 0x80)) { // Overflow flag, (!((AC ^ src) & 0x80) && ((AC ^ temp) & 0x80))
		proc_status |= OVERFLOW_FLAG;
	}

	// Store result in accumulator
	accumulator = sum & 0x00FF;

	return 0;
}

/*
	N Z C I D V
	+ + - - - -
*/
int NES_Cpu::AND() {

	// Bitwise AND with accumulator and data. Result is 1 byte
	accumulator = accumulator & memory[target_address];

	proc_status &= ~(NEGATIVE_FLAG | ZERO_FLAG);

	// Check flags
	if (accumulator & 0x0080) { // Negative flag
		proc_status |= NEGATIVE_FLAG;
	}

	if (accumulator == 0) { // Zero flag
		proc_status |= ZERO_FLAG;
	}

	return 0;
}


/*
	N Z C I D V
	+ + + - - -
*/
int NES_Cpu::ASL() {

	// Arithmetic shift left the accumulator
	accumulator <<= 1;

	proc_status &= ~(NEGATIVE_FLAG | ZERO_FLAG | CARRY_FLAG);

	// Check flags
	if (accumulator & 0x0080) { // Negative flag
		proc_status |= NEGATIVE_FLAG;
	}

	if (accumulator == 0) { // Zero flag
		proc_status |= ZERO_FLAG;
	}

	if (accumulator >= 0x0100) { // Carry flag 
		proc_status |= CARRY_FLAG;
	}

	return 0;
}

/*
	N Z C I D V
	- - - - - -
*/
int NES_Cpu::BCC() {

	// Branch if the carry flag is clear
	if (!(proc_status & CARRY_FLAG)) { // Carry flag 
		pc = target_address;
	}

	return 0;
}

/*
	N Z C I D V
	- - - - - -
*/
int NES_Cpu::BCS() {

	// Branch if the carry flag is set
	if (proc_status & CARRY_FLAG) {
		pc = target_address;
	}

	return 0;
}

/*
	N Z C I D V
	- - - - - -
*/
int NES_Cpu::BEQ() {

	// Branch if the zero flag is set
	if (proc_status & ZERO_FLAG) {
		pc = target_address;
	}

	return 0;
}

/*
	 N  Z C I D  V
     M7 + - - - M6
*/
int NES_Cpu::BIT() {

	// Bitwise AND with accumulator and data, the result is not saved and is only used to set flags
	unsigned short result = accumulator & memory[target_address];

	proc_status &= ~(ZERO_FLAG | NEGATIVE_FLAG | OVERFLOW_FLAG);

	// Check flags
	if ((result & 0x00FF) == 0) { // Zero flag
		proc_status |= ZERO_FLAG;
	}

	if (memory[target_address] & NEGATIVE_FLAG) { // Negative flag
		proc_status |= NEGATIVE_FLAG;
	}

	if (memory[target_address] & OVERFLOW_FLAG) { // Overflow flag
		proc_status |= OVERFLOW_FLAG;
	}

	return 0;
}

/*
	N Z C I D V
	- - - - - -
*/
int NES_Cpu::BMI() {

	// Branch if the negative flag is set
	if (proc_status & NEGATIVE_FLAG) {
		pc = target_address;
	}

	return 0;
}

/*
	N Z C I D V
	- - - - - -
*/
int NES_Cpu::BNE() {
	
	// Branch if the zero flag is clear
	if (!(proc_status & ZERO_FLAG)) {
		pc = target_address;
	}

	return 0;
}

/*
	N Z C I D V
	- - - - - -
*/
int NES_Cpu::BPL() {

	// Branch if the negative flag is clear
	if (!(proc_status & NEGATIVE_FLAG)) {
		pc = target_address;
	}

	return 0;
}

/*
	N Z C I D V
    - - - 1 - -
*/
int NES_Cpu::BRK() {

	// Set the interrupt disable flag
	proc_status |= DISABLE_FLAG;

	// Push the 2 byte program counter into the stack, remember to do it in little endian
	memory[sp] = (pc >> 8) & 0x00FF;
	memory[sp - 1] = pc & 0x00FF;
	sp -= 2;

	// Push the proc_status with the break bit set into the stack
	memory[sp] = proc_status | BREAK_FLAG;
	sp--;

	// The program counter then must jump to the instruction in $FFFF and $FFFE
	pc = (memory[0xFFFF] << 8) | memory[0xFFFE];

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

	use_accumulator = 1;

	return 0;
}

int NES_Cpu::ABS() {

	// Read little endian byte address
	target_address = memory[pc + 1] | memory[pc + 2] << 8;

	// Update to show that we have made two accesses
	pc += 2;

	return 0;
}

int NES_Cpu::ABS_X() {

	// Read little endian byte address and add X
	target_address = memory[pc + 1] | memory[pc + 2] << 8;
	target_address += X;

	// Update to show that we have made two accesses
	pc += 2;

	return 0;
}

int NES_Cpu::ABS_Y() {

	// Read little endian byte address and add Y
	target_address = memory[pc + 1] | memory[pc + 2] << 8;
	target_address += Y;

	// Update to show that we have made two accesses
	pc += 2;

	return 0;
}

int NES_Cpu::IMM() {
	
	// Data is in next byte
	target_address = pc + 1;

	// Update to show that we have made an additional access
	pc += 1;

	return 0;
}

int NES_Cpu::IMP() {

	// No need to do anything

	return 0;
}

int NES_Cpu::IND() {

	// Set target address to the address in the next two bytes
	unsigned short indirect_address = memory[pc + 2] << 8 | memory[pc + 1];
	target_address = memory[indirect_address + 1] << 8 | memory[indirect_address];

	// Update to show that we have made two accesses
	pc += 2;

	return 0;
}

int NES_Cpu::IND_X() {

	// Add immediate and X for the indirect address
	unsigned short indirect_address = memory[pc + 1] + X;
	target_address = memory[indirect_address + 1] << 8 | memory[indirect_address];

	// Update to show that we have made an additional access
	pc += 1;

	return 0;
}

int NES_Cpu::IND_Y() {

	// Add immediate's data and Y for the indirect address
	unsigned short indirect_address = memory[pc + 1];
	target_address = memory[indirect_address + 1] << 8 | memory[indirect_address];
	target_address += Y;

	// Update to show that we have made an additional access
	pc += 1;

	return 0;
}

int NES_Cpu::REL() {

	// Target address is the pc plus the value after it
	target_address = pc + memory[pc + 1];

	// Update to show that we have made an additional access
	pc += 1;

	return 0;
}

int NES_Cpu::ZPG() {

	// Target address is in the next byte
	target_address = memory[pc + 1];

	// Update to show that we have made an additional access
	pc += 1;

	return 0;
}

int NES_Cpu::ZPG_X() {

	// Target address is the first byte of the sum of the X register and the next byte
	target_address = (memory[pc + 1] + X) & 0x00FF;

	// Update to show that we have made an additional access
	pc += 1;

	return 0;
}

int NES_Cpu::ZPG_Y() {

	// Target address is the first byte of the sum of the Y register and the next byte
	target_address = (memory[pc + 1] + Y) & 0x00FF;

	// Update to show that we have made an additional access
	pc += 1;

	return 0;
}

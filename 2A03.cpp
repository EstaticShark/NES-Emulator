#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "NES.h"

// Set to 1 for logging
int trace = 1;

// Initialization
NES_Cpu::NES_Cpu() {
	// Initialize values
	pc = 0x0000;						
	opcode = 0x00;
	sp = 0x00;							// Decrements instead of increment: https://www.youtube.com/watch?v=fWqBmmPQP40
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

int NES_Cpu::load(const char* game) {

	printf("game: %s\n", game);

	// Open the game file and read it into our memory
	FILE *game_file = fopen(game, "rb");
	if (game_file == NULL) {
		printf("Failed to open game file\n");
		return 1;
	}

	// Find the file size
	fseek(game_file, 0, SEEK_END);
	int size = ftell(game_file);
	fseek(game_file, 0, SEEK_SET);

	void *reading_space = malloc(sizeof(char) * size);

	std::cout << reading_space;

	// Close the game file
	fclose(game_file);

	printf("Game loaded\n");

	return 0;
}


void NES_Cpu::log() {
	printf("\tAccumulator: %d\nX Register: %d\nY Register: %d\n", accumulator, X, Y);

	char bits[8];
	char proc = proc_status;
	int i = 0;
	while (i < 8){
		bits[i] = proc % 2;
		proc /= 2;
		i++;
	}

	//(N,V,-,B,D,I,Z,C)
	printf("\tN: %d,V: %d,-: %d,B: %d,D: %d,I: %d,Z: %d,C: %d", bits[7], bits[6], bits[5], bits[4], bits[3], bits[2], bits[1], bits[0]);
}

// Emulation cycling
void NES_Cpu::cycle() {

	// Get opcode
	opcode = memory[pc];

	if (trace){
		printf("Opcode: %s\n", this->instruction_table[opcode].instr_name);
	}

	// Retrieve the instruction details, instruction_entry in format:
	// {
	//	std::string instr_name;
	//	int (NES_Cpu::*operation)();
	//	int (NES_Cpu::*addr_setup)();
	//	unsigned int cycles;
	// }
	//instruction_entry *instruction = &instruction_table[opcode];

	// Pointer member details in link
	// https://docs.microsoft.com/en-us/cpp/cpp/pointer-to-member-operators-dot-star-and-star?view=vs-2019
	// https://stackoverflow.com/questions/6586205/what-are-the-pointer-to-member-and-operators-in-c

	// Flag setup
	use_accumulator = 0;

	// Increment program counter
	pc++;

	
	if (trace) {
		printf("Pre-setup\n");
		this->log();
	}


	// Setup addressing mode variables
	(this->*instruction_table[opcode].addr_setup)();

	if(trace){
		printf("Post-setup\n");
		this->log();
	}

	// Run the operation for the appropriate amount of cycles
	(this->*instruction_table[opcode].operation)();

	if(trace){
		printf("Post-operation\n");
		this->log();
	}

	/*
		Note on cycle counting. I may count the cycles through timing the time needed for the operation and then sleeping
		off the rest of the time. Otherwise I could do the opposite and wait for a certain amount of time before I start
		the instruction
	*/

}


/*
	Interrupt implementations are here
*/
void NES_Cpu::irq() {

	// Return if interrupt disable is on  
	if (proc_status & DISABLE_FLAG) {
		return;
	}

	// Push up the current program counter to the stack
	memory[STACK_OFFSET + sp] = (pc >> 8) & 0x00FF;
	memory[STACK_OFFSET + sp - 1] = pc & 0x00FF;
	sp -= 2;

	// Push up the current status
	memory[STACK_OFFSET + sp] = proc_status;
	sp--;

	// The program counter then must jump to the instruction in $FFFF and $FFFE
	pc = (memory[0xFFFF] << 8) | memory[0xFFFE];

}

void NES_Cpu::nmi() {

	// Unlike IRQ, there is nothing that can stop the execution of the NMI

	// Push up the current program counter to the stack
	memory[STACK_OFFSET + sp] = (pc >> 8) & 0x00FF;
	memory[STACK_OFFSET + sp - 1] = pc & 0x00FF;
	sp -= 2;

	// Push up the current status
	memory[STACK_OFFSET + sp] = proc_status;
	sp--;

	// The program counter then must jump to the instruction in $FFFB and $FFFA
	pc = (memory[0xFFFB] << 8) | memory[0xFFFA];

}

void NES_Cpu::reset() {
	// Clear registers and variables
	opcode = 0x00;
	sp = 0x00;
	accumulator = 0x00;
	X = 0x00;
	Y = 0x00;
	proc_status = 0x00;
	use_accumulator = 0;
	target_address = 0x0000;

	// Clear flags
	proc_status = 0;

	// The program counter then must jump to the instruction in $FFFF and $FFFE
	pc = (memory[0xFFFD] << 8) | memory[0xFFFC];

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
	uint16_t sum = accumulator + memory[target_address] + ((proc_status & CARRY_FLAG) ? 1 : 0);

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

	proc_status &= ~(NEGATIVE_FLAG | ZERO_FLAG | CARRY_FLAG);

	if (use_accumulator) {
		// Arithmetic shift left the accumulator

		// Carry flag is set to original bit 7
		if (accumulator & 0x0080) { // Carry flag 
			proc_status |= CARRY_FLAG;
		}

		accumulator <<= 1;

		// Check flags
		if (accumulator & 0x0080) { // Negative flag
			proc_status |= NEGATIVE_FLAG;
		}

		if ((accumulator & 0x00FF) == 0) { // Zero flag
			proc_status |= ZERO_FLAG;
		}
	}
	else {
		// Arithmetic shift left the data at the target address
		uint16_t shifted_result = memory[target_address] << 1;

		// Carry flag is set to original bit 7
		if (memory[target_address] & 0x0080) { // Carry flag 
			proc_status |= CARRY_FLAG;
		}

		memory[target_address] = shifted_result;

		// Check flags
		if (shifted_result & 0x0080) { // Negative flag
			proc_status |= NEGATIVE_FLAG;
		}

		if ((shifted_result & 0x00FF) == 0) { // Zero flag
			proc_status |= ZERO_FLAG;
		}
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
	uint16_t result = accumulator & memory[target_address];

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
	memory[STACK_OFFSET + sp] = (pc >> 8) & 0x00FF;
	memory[STACK_OFFSET + sp - 1] = pc & 0x00FF;
	sp -= 2;

	// Push the proc_status with the break bit set into the stack
	memory[STACK_OFFSET + sp] = proc_status | BREAK_FLAG;
	sp--;

	// The program counter then must jump to the instruction in $FFFF and $FFFE
	pc = (memory[0xFFFF] << 8) | memory[0xFFFE];

	return 0;
}

/*
	N Z C I D V
	- - - - - -
*/
int NES_Cpu::BVC() {

	// Branch if the overflow flag is clear
	if (!(proc_status & OVERFLOW_FLAG)) {
		pc = target_address;
	}

	return 0;
}

/*
	N Z C I D V
	- - - - - -
*/
int NES_Cpu::BVS() {

	// Branch if the overflow flag is set
	if (proc_status & OVERFLOW_FLAG) {
		pc = target_address;
	}

	return 0;
}

/*
	N Z C I D V
    - - 0 - - -
*/
int NES_Cpu::CLC() {

	// Clear the carry flag
	if (proc_status & CARRY_FLAG) {
		proc_status &= ~CARRY_FLAG;
	}

	return 0;
}

/*
	N Z C I D V
	- - - - 0 -
*/
int NES_Cpu::CLD() {

	// Clear the decimal flag
	if (proc_status & DECIMAL_FLAG) {
		proc_status &= ~DECIMAL_FLAG;
	}

	return 0;
}

/*
	N Z C I D V
	- - - 0 - -
*/
int NES_Cpu::CLI() {

	// Clear the interrupt disable flag
	if (proc_status & DISABLE_FLAG) {
		proc_status &= ~DISABLE_FLAG;
	}

	return 0;
}

/*
	N Z C I D V
	- - - - - 0
*/
int NES_Cpu::CLV() {

	// Clear the overflow flag
	if (proc_status & OVERFLOW_FLAG) {
		proc_status &= ~OVERFLOW_FLAG;
	}

	return 0;
}

/*
	 N Z C I D V
     + + + - - -
*/
int NES_Cpu::CMP() {
	uint8_t data = memory[target_address];
	uint16_t diff = accumulator - data;

	proc_status &= ~(NEGATIVE_FLAG | CARRY_FLAG | ZERO_FLAG);

	if (accumulator < data) { // Negative flag
		proc_status |= NEGATIVE_FLAG;
	}

	if (accumulator >= data) { // Carry flag
		proc_status |= CARRY_FLAG;
	}
	
	if ((diff & 0x00FF) == 0x0000) { // Zero flag
		proc_status |= ZERO_FLAG;
	}

	return 0;
}

/*
	 N Z C I D V
	 + + + - - -
*/
int NES_Cpu::CPX() {

	uint8_t data = memory[target_address];
	uint16_t diff = X - data;

	proc_status &= ~(NEGATIVE_FLAG | CARRY_FLAG | ZERO_FLAG);

	if (X < data) { // Negative flag
		proc_status |= NEGATIVE_FLAG;
	}

	if (X >= data) { // Carry flag
		proc_status |= CARRY_FLAG;
	}

	if ((diff & 0x00FF) == 0x0000) { // Zero flag
		proc_status |= ZERO_FLAG;
	}

	return 0;
}

/*
	 N Z C I D V
	 + + + - - -
*/
int NES_Cpu::CPY() {

	uint8_t data = memory[target_address];
	uint16_t diff = Y - data;

	proc_status &= ~(NEGATIVE_FLAG | CARRY_FLAG | ZERO_FLAG);

	if (Y < data) { // Negative flag
		proc_status |= NEGATIVE_FLAG;
	}

	if (Y >= data) { // Carry flag
		proc_status |= CARRY_FLAG;
	}

	if ((diff & 0x00FF) == 0x0000) { // Zero flag
		proc_status |= ZERO_FLAG;
	}

	return 0;
}

/*
	 N Z C I D V
	 + + - - - -
*/
int NES_Cpu::DEC() {
	memory[target_address]--;

	proc_status &= ~(NEGATIVE_FLAG | ZERO_FLAG);

	if (memory[target_address] & 0x80) { // Negative flag
		proc_status |= NEGATIVE_FLAG;
	}

	if (memory[target_address] == 0x00) { // Zero flag
		proc_status |= ZERO_FLAG;
	}

	return 0;
}

/*
	 N Z C I D V
	 + + - - - -
*/
int NES_Cpu::DEX() {
	X--;

	proc_status &= ~(NEGATIVE_FLAG | ZERO_FLAG);

	if (X & 0x80) { // Negative flag
		proc_status |= NEGATIVE_FLAG;
	}

	if (X == 0x00) { // Zero flag
		proc_status |= ZERO_FLAG;
	}

	return 0;
}

/*
	 N Z C I D V
	 + + - - - -
*/
int NES_Cpu::DEY() {
	Y--;

	proc_status &= ~(NEGATIVE_FLAG | ZERO_FLAG);

	if (Y & 0x80) { // Negative flag
		proc_status |= NEGATIVE_FLAG;
	}

	if (Y == 0x00) { // Zero flag
		proc_status |= ZERO_FLAG;
	}

	return 0;
}

/*
	 N Z C I D V
	 + + - - - -
*/
int NES_Cpu::EOR() {

	accumulator ^= memory[target_address];

	proc_status &= ~(NEGATIVE_FLAG | ZERO_FLAG);

	if (accumulator & 0x80) { // Negative flag
		proc_status |= NEGATIVE_FLAG;
	}

	if (accumulator == 0x00) { // Zero flag
		proc_status |= ZERO_FLAG;
	}

	return 0;
}

/*
	 N Z C I D V
	 + + - - - -
*/
int NES_Cpu::INC() {
	memory[target_address]++;

	proc_status &= ~(NEGATIVE_FLAG | ZERO_FLAG);

	if (memory[target_address] & 0x80) { // Negative flag
		proc_status |= NEGATIVE_FLAG;
	}

	if (memory[target_address] == 0x00) { // Zero flag
		proc_status |= ZERO_FLAG;
	}

	return 0;
}

/*
	 N Z C I D V
	 + + - - - -
*/
int NES_Cpu::INX() {
	X++;

	proc_status &= ~(NEGATIVE_FLAG | ZERO_FLAG);

	if (X & 0x80) { // Negative flag
		proc_status |= NEGATIVE_FLAG;
	}

	if (X == 0x00) { // Zero flag
		proc_status |= ZERO_FLAG;
	}

	return 0;
}

/*
	 N Z C I D V
	 + + - - - -
*/
int NES_Cpu::INY() {
	Y++;

	proc_status &= ~(NEGATIVE_FLAG | ZERO_FLAG);

	if (Y & 0x80) { // Negative flag
		proc_status |= NEGATIVE_FLAG;
	}

	if (Y == 0x00) { // Zero flag
		proc_status |= ZERO_FLAG;
	}

	return 0;
}

/*
	 N Z C I D V
	 - - - - - -
*/
int NES_Cpu::JMP() {

	// Set program counter to the 2 byte instruction in the target address
	pc = (memory[target_address + 1] << 8) | memory[target_address];

	return 0;
}

/*
	 N Z C I D V
	 - - - - - -
*/
int NES_Cpu::JSR() {

	// Store pc into stack first
	memory[STACK_OFFSET + sp] = (pc >> 8) & 0x00FF;
	memory[STACK_OFFSET + sp - 1] = pc & 0x00FF;
	sp -= 2;

	// Set program counter to the 2 byte instruction in the target address
	pc = (memory[target_address + 1] << 8) | memory[target_address];

	return 0;
}

/*
	 N Z C I D V
	 + + - - - -
*/
int NES_Cpu::LDA() {
	accumulator = memory[target_address];

	proc_status &= ~(NEGATIVE_FLAG | ZERO_FLAG);

	if (accumulator & 0x80) { // Negative flag
		proc_status |= NEGATIVE_FLAG;
	}

	if (accumulator == 0x00) { // Zero flag
		proc_status |= ZERO_FLAG;
	}

	return 0;
}

/*
	 N Z C I D V
	 + + - - - -
*/
int NES_Cpu::LDX() {
	X = memory[target_address];

	proc_status &= ~(NEGATIVE_FLAG | ZERO_FLAG);

	if (X & 0x80) { // Negative flag
		proc_status |= NEGATIVE_FLAG;
	}

	if (X == 0x00) { // Zero flag
		proc_status |= ZERO_FLAG;
	}

	return 0;
}

/*
	 N Z C I D V
	 + + - - - -
*/
int NES_Cpu::LDY() {
	Y = memory[target_address];

	proc_status &= ~(NEGATIVE_FLAG | ZERO_FLAG);

	if (Y & 0x80) { // Negative flag
		proc_status |= NEGATIVE_FLAG;
	}

	if (Y == 0x00) { // Zero flag
		proc_status |= ZERO_FLAG;
	}

	return 0;
}

/*
	 N Z C I D V
	 0 + + - - -
*/
int NES_Cpu::LSR() {

	proc_status &= ~(NEGATIVE_FLAG | ZERO_FLAG | CARRY_FLAG);

	if (use_accumulator) {
		// Arithmetic shift right the accumulator

		if (accumulator & 0x0001) { // Carry flag 
			proc_status |= CARRY_FLAG;
		}

		accumulator >>= 1;

		if ((accumulator & 0x00FF) == 0) { // Zero flag
			proc_status |= ZERO_FLAG;
		}
	}
	else {
		// Arithmetic shift right the data at the target address
		uint16_t shifted_result = memory[target_address] >> 1;

		if (memory[target_address] & 0x0001) { // Carry flag 
			proc_status |= CARRY_FLAG;
		}

		memory[target_address] = shifted_result;

		if ((shifted_result & 0x00FF) == 0) { // Zero flag
			proc_status |= ZERO_FLAG;
		}
	}

	return 0;
}

/*
	 N Z C I D V
	 - - - - - -
*/
int NES_Cpu::NOP() {

	// Do nothing

	return 0;
}

/*
	 N Z C I D V
	 + + - - - -
*/
int NES_Cpu::ORA() {

	accumulator |= memory[target_address];

	proc_status &= ~(NEGATIVE_FLAG | ZERO_FLAG);

	if (accumulator & 0x80) { // Negative flag
		proc_status |= NEGATIVE_FLAG;
	}

	if (accumulator == 0x00) { // Zero flag
		proc_status |= ZERO_FLAG;
	}

	return 0;
}

/*
	 N Z C I D V
	 - - - - - -
*/
int NES_Cpu::PHA() {

	// Push accumulator onto stack
	memory[STACK_OFFSET + sp] = accumulator;
	sp--;

	return 0;
}

/*
	 N Z C I D V
	 - - - - - -
*/
int NES_Cpu::PHP() {

	// Push processor status onto stack
	memory[STACK_OFFSET + sp] = proc_status;
	sp--;

	return 0;
}

/*
	 N Z C I D V
	 + + - - - -
*/
int NES_Cpu::PLA() {

	// Pull accumulator from stack
	sp++;
	accumulator = memory[STACK_OFFSET + sp];

	proc_status &= ~(NEGATIVE_FLAG | ZERO_FLAG);

	if (accumulator & 0x80) { // Negative flag
		proc_status |= NEGATIVE_FLAG;
	}

	if (accumulator == 0x00) { // Zero flag
		proc_status |= ZERO_FLAG;
	}

	return 0;
}


/*
	 N Z C I D V
	 From stacks
*/
int NES_Cpu::PLP() {

	// Pull proc_status from stack
	sp++;
	proc_status = memory[STACK_OFFSET + sp];

	return 0;
}

/*
	 N Z C I D V
	 + + + - - -
*/
int NES_Cpu::ROL() {

	// Set up the data
	uint8_t *reg;
	if (use_accumulator) {
		reg = &accumulator;
	}
	else {
		reg = &memory[target_address];
	}

	proc_status &= ~(CARRY_FLAG | NEGATIVE_FLAG | ZERO_FLAG);

	// Check the carry bit first
	if (*reg & 0x80) { // Check if the leftmost bit is 1
		proc_status |= CARRY_FLAG;
	}

	// Shift left and then fill last bit
	*reg <<= 1;

	// Set last bit to carry bit
	if (proc_status & CARRY_FLAG) { // Set last bit to 1
		*reg |= 0x01;
	}
	else { // Set last bit to 0
		*reg &= 0xFE;
	}

	if (*reg & 0x80) { // Negative flag
		proc_status |= NEGATIVE_FLAG;
	}

	if (*reg == 0x00) { // Zero flag
		proc_status |= ZERO_FLAG;
	}

	return 0;
}

/*
	 N Z C I D V
	 + + + - - -
*/
int NES_Cpu::ROR() {
	// Set up the data
	uint8_t *reg;
	if (use_accumulator) {
		reg = &accumulator;
	}
	else {
		reg = &memory[target_address];
	}

	proc_status &= ~(CARRY_FLAG | NEGATIVE_FLAG | ZERO_FLAG);

	// Check the carry bit first
	if (*reg & 0x01) { // Check if the rightmost bit is 1
		proc_status |= CARRY_FLAG;
	}

	// Shift right and then fill first bit
	*reg >>= 1;

	// Set first bit to carry bit
	if (proc_status & CARRY_FLAG) { // Set last bit to 1
		*reg |= 0x80;
	}
	else { // Set first bit to 0
		*reg &= 0x7F;
	}

	if (*reg & 0x80) { // Negative flag
		proc_status |= NEGATIVE_FLAG;
	}

	if (*reg == 0x00) { // Zero flag
		proc_status |= ZERO_FLAG;
	}

	return 0;
}

/*
	 N Z C I D V
	 From stacks
*/
int NES_Cpu::RTI() {

	// Pull back proc_status
	sp++;
	proc_status = memory[STACK_OFFSET + sp];

	// Pull back program counter
	sp+= 2;
	pc = (memory[STACK_OFFSET + sp] << 8) | memory[STACK_OFFSET + sp - 1];

	return 0;
}

/*
	 N Z C I D V
	 - - - - - -
*/
int NES_Cpu::RTS() {

	// Pull back program counter
	sp += 2;
	pc = (memory[STACK_OFFSET + sp] << 8) | memory[STACK_OFFSET + sp - 1];

	return 0;
}

/*
	 N Z C I D V
	 + + + - - +
*/
int NES_Cpu::SBC() {

	// Set up the data
	uint8_t *reg;
	if (use_accumulator) {
		reg = &accumulator;
	}
	else {
		reg = &memory[target_address];
	}

	// Get difference
	uint16_t diff = accumulator - *reg;

	// Clear flags
	proc_status &= ~(CARRY_FLAG | NEGATIVE_FLAG | ZERO_FLAG | OVERFLOW_FLAG);

	// Check flags
	if (diff >= 0x0100) { // Carry flag 
		proc_status |= CARRY_FLAG;
	}

	if ((diff & 0x00FF) == 0) { // Zero flag
		proc_status |= ZERO_FLAG;
	}

	if (diff & 0x0080) { // Negative flag
		proc_status |= NEGATIVE_FLAG;
	}

	// Explanation for signed overflow flags: http://forums.nesdev.com/viewtopic.php?t=6331
	if (!((accumulator ^ *reg) & 0x80) && ((accumulator ^ diff) & 0x80)) { // Overflow flag, (!((AC ^ src) & 0x80) && ((AC ^ temp) & 0x80))
		proc_status |= OVERFLOW_FLAG;
	}

	// Set accumulator to difference
	accumulator = diff & 0x00FF;

	return 0;
}

/*
	 N Z C I D V
	 - - 1 - - -
*/
int NES_Cpu::SEC() {

	// Set carry flag
	proc_status |= CARRY_FLAG;

	return 0;
}

/*
	 N Z C I D V
	 - - - - 1 -
*/
int NES_Cpu::SED() {

	// Set decimal flag
	proc_status |= DECIMAL_FLAG;

	return 0;
}

/*
	 N Z C I D V
	 - - - 1 - -
*/
int NES_Cpu::SEI() {

	// Set decimal flag
	proc_status |= DISABLE_FLAG;

	return 0;
}

/*
	 N Z C I D V
	 - - - - - -
*/
int NES_Cpu::STA() {

	// Set memory target to the accumulator
	memory[target_address] = accumulator;

	return 0;
}

/*
	 N Z C I D V
	 - - - - - -
*/
int NES_Cpu::STX() {

	// Set memory target to the X register
	memory[target_address] = X;

	return 0;
}

/*
	 N Z C I D V
	 - - - - - -
*/
int NES_Cpu::STY() {

	// Set memory target to the Y register
	memory[target_address] = Y;

	return 0;
}

/*
	 N Z C I D V
	 + + - - - -
*/
int NES_Cpu::TAX() {

	// Transfer accumulator to X
	X = accumulator;

	// Clear flags
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
	 + + - - - -
*/
int NES_Cpu::TAY() {

	// Transfer accumulator to Y
	Y = accumulator;

	// Clear flags
	proc_status &= ~(NEGATIVE_FLAG | ZERO_FLAG);

	// Clear flags
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
	 + + - - - -
*/
int NES_Cpu::TSX() {

	// Stores the stack pointer in X
	X = sp;

	// Clear flags
	proc_status &= ~(NEGATIVE_FLAG | ZERO_FLAG);

	// Clear flags
	proc_status &= ~(NEGATIVE_FLAG | ZERO_FLAG);

	// Check flags
	if (sp & 0x80) { // Negative flag
		proc_status |= NEGATIVE_FLAG;
	}

	if (sp == 0) { // Zero flag
		proc_status |= ZERO_FLAG;
	}

	return 0;
}

/*
	 N Z C I D V
	 + + - - - -
*/
int NES_Cpu::TXA() {

	// Transfer X to accumulator
	accumulator = X;

	// Clear flags
	proc_status &= ~(NEGATIVE_FLAG | ZERO_FLAG);

	// Clear flags
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
	 - - - - - -
*/
int NES_Cpu::TXS() {

	// Transfer X to the stack pointer
	sp = X;

	return 0;
}

/*
	 N Z C I D V
	 + + - - - -
*/
int NES_Cpu::TYA() {

	// Transfer Y to accumulator
	accumulator = Y;

	// Clear flags
	proc_status &= ~(NEGATIVE_FLAG | ZERO_FLAG);

	// Clear flags
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
	target_address = memory[pc] | memory[pc + 1] << 8;

	// Update to show that we have made two accesses
	pc += 2;

	return 0;
}

int NES_Cpu::ABS_X() {

	// Read little endian byte address and add X
	target_address = memory[pc] | memory[pc + 1] << 8;
	target_address += X;

	// Update to show that we have made two accesses
	pc += 2;

	return 0;
}

int NES_Cpu::ABS_Y() {

	// Read little endian byte address and add Y
	target_address = memory[pc] | memory[pc + 1] << 8;
	target_address += Y;

	// Update to show that we have made two accesses
	pc += 2;

	return 0;
}

int NES_Cpu::IMM() {
	
	// Data is in next byte
	target_address = pc;

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
	uint16_t indirect_address = memory[pc + 1] << 8 | memory[pc];
	target_address = memory[indirect_address + 1] << 8 | memory[indirect_address];

	// Update to show that we have made two accesses
	pc += 2;

	return 0;
}

int NES_Cpu::IND_X() {

	// Add immediate and X for the indirect address
	uint16_t indirect_address = memory[pc] + X;
	target_address = memory[indirect_address + 1] << 8 | memory[indirect_address];

	// Update to show that we have made an additional access
	pc += 1;

	return 0;
}

int NES_Cpu::IND_Y() {

	// Add immediate's data and Y for the indirect address
	uint16_t indirect_address = memory[pc];
	target_address = memory[indirect_address + 1] << 8 | memory[indirect_address];
	target_address += Y;

	// Update to show that we have made an additional access
	pc += 1;

	return 0;
}

int NES_Cpu::REL() {

	// Target address is the pc plus the value after it
	target_address = pc + memory[pc];

	// Update to show that we have made an additional access
	pc += 1;

	return 0;
}

int NES_Cpu::ZPG() {

	// Target address is in the next byte
	target_address = memory[pc];

	// Update to show that we have made an additional access
	pc += 1;

	return 0;
}

int NES_Cpu::ZPG_X() {

	// Target address is the first byte of the sum of the X register and the next byte
	target_address = (memory[pc] + X) & 0x00FF;

	// Update to show that we have made an additional access
	pc += 1;

	return 0;
}

int NES_Cpu::ZPG_Y() {

	// Target address is the first byte of the sum of the Y register and the next byte
	target_address = (memory[pc] + Y) & 0x00FF;

	// Update to show that we have made an additional access
	pc += 1;

	return 0;
}

#pragma once

#include <iostream>
#include <vector>
#include <string.h>

// Address modes
#define MODE_ILL	0
#define MODE_ACC	1
#define MODE_ABS	2
#define MODE_ABS_X	3
#define MODE_ABS_Y	4
#define MODE_IMM	5
#define MODE_IMP	6
#define MODE_IND	7
#define MODE_IND_X	8
#define MODE_IND_Y	9
#define MODE_REL	10
#define MODE_ZPG	11
#define MODE_ZPG_X	12
#define MODE_ZPG_Y	13

// Locations
#define STACK_OFFSET 0x0200

// Flags for processor status
#define CARRY_FLAG		1
#define ZERO_FLAG		2
#define DISABLE_FLAG	4
#define DECIMAL_FLAG	8
#define BREAK_FLAG		16
#define OVERFLOW_FLAG	64
#define NEGATIVE_FLAG	128

// Formatting for .nes files
#define PRG_ROM			4
#define CHR_ROM			5
#define FLG_6			6
#define FLG_7			7
#define FLG_8			8
#define FLG_9			9
#define FLG_10			10

// Common sizes
#define PRG_ROM_UNIT	16384
#define CHR_ROM_UNIT	8192


class NES_Cpu {
	private:
		/*
			CPU Memory
			- The $0000 - $FFFF range acts as the 16-bit address bus, while each address acts as the 8-bit data bus
			- The opcode represents the 8-bit control bus used to communicate with the registers and cartridge

			MEMORY LAYOUT
			
			$0000 - $0100 ($0100)	- Zero page
				$0000-$000F ($000F)	- Local variables and function arguments
				$0010-$00FF ($00EF)	- Global variables accessed most often, including certain pointer tables

			$0100 -	$01FF ($0100)	- Stack

			$0200 - $07FF ($0600)	- Ram

			$0800 - $0FFF ($0800)	- Mirror of $0000 - $07FF

			$1000 - $17FF ($0800)	- Mirror of $0000 - $07FF

			$1800 - $1FFF ($0800)	- Mirror of $0000 - $07FF

			$2000 - $2007 ($0008)	- NES PPU Registers
				$2000 - PPUCTRL, (VPHB SINN), 		NMI enable (V), PPU master/slave (P), sprite height (H), background tile select (B),
													sprite tile select (S), increment mode (I), nametable select (NN)
				$2001 - PPUMASK, (BGRs bMmG),		Color emphasis (BGR), sprite enable (s), background enable (b), sprite left column enable (M),
													background left column enable (m), greyscale (G)
				$2002 - PPUSTATUS, (VSO- ----),		vblank (V), sprite 0 hit (S), sprite overflow (O); read resets write pair for $2005/$2006
				$2003 - OAMADDR, (aaaa aaaa),		OAM read/write address
				$2004 - OAMDATA, (dddd dddd),		OAM data read/write
				$2005 - PPUSCROLL, (xxxx xxxx),		fine scroll position (two writes: X scroll, Y scroll)
				$2006 - PPUADDR, (aaaa aaaa),		PPU read/write address (two writes: most significant byte, least significant byte)
				$2007 - PPUDATA, (dddd dddd),		PPU data read/write

			$2008 - $3FFF ($1FF8)	- Mirrors of $2000 - $2007 for every 8 bytes

			$4000 - $4017 ($0018)	- NES APU and I/O registers
				$4000-$4003	- Pulse 1
				$4004-$4007 - Pulse 2
				$4008-$400B - Triangle
				$400C-$400F - Noise
				$4010-$4013 - DMC
				$4015		- All
				$4017		- All

			$4018 - $401F ($0008)	- Normally deactivated APU and I/O operations

			$4020 - $FFFF ($BFE0)	- Cartriges, PRG ROM/RAM, and mapper registers
				$FFFA - $FFFB - NMI (Non-Maskable Interrupt) vector
				$FFFC - $FFFD - RES (Reset) vector
				$FFFE - $FFFF - IRQ (Interrupt Request) vector

		*/
		uint8_t memory[0xFFFF];

		uint16_t pc;								// Program counter
		uint8_t opcode;								// Current opcode
		uint8_t sp;									// Stack pointer
		uint8_t accumulator;						// Accumulator
		uint8_t X;									// X register
		uint8_t Y;									// Y register
		uint8_t proc_status;						// Process Status - (N,V,-,B,D,I,Z,C)

		/*
			Adressing Mode Variables

			Variables that are used for different addressing modes

			We need a flag for the ACC addressing mode since ACC is not a lone addressing mode like IMP is
		*/
		uint8_t use_accumulator;					// Flag to use accumulator
		uint16_t target_address;					// Stores target address for addressing modes


		/*
			INSTRUCTION TABLE

			This lists out each possible instruction, their addressing modes and their cycle timings. Each entry goes:

				{	INSTRUCTION TYPE	,	INSTRUCTION FUNCTION	, ADDRESSING MODE ,	ADDRESSING MODE FUNCTION ,	REQUIRED CYCLES	}

			INSTRUCTION TYPES (56 Different Instructions)

			ADC	AND	ASL	BCC	BCS	BEQ	BIT	BMI	BNE	BPL	BRK BVC BVS CLC
			CLD CLI CLV CMP CPX CPY DEC DEX DEY EOR INC INX INY JMP
			JSR LDA LDX LDY LSR NOP ORA PHA PHP PLA PLP ROL ROR RTI
			RTS SBC SEC SED SEI STA STX STY TAX TAY TSX TXA TXS TYA

			- The instruction function carries out the actual instruction
			- The addressing mode will carry the name of the addressing mode for further identification
			- The addressing mode function will set up the required variables
			for the instruction function to succeed
			- The required cycles will represent the maximum number of cycles needed to run the operation
			- The above represents legal instructions, whereas we will represent illegal instructions as ILL
		*/

		// Table of all possible instructions, legal and illegal; https://www.masswerk.at/6502/6502_instruction_set.html
		typedef int (NES_Cpu::*operation)();
		typedef int (NES_Cpu::*addr_setup)();

		// Definitions for the instruction and address mode functions
		typedef struct instruction_entry {
			std::string instr_name;
			int (NES_Cpu::*operation)();
			int (NES_Cpu::*addr_setup)();
			unsigned int cycles;
		} instruction_entry;

		// For windows
		instruction_entry instruction_table[0x0100] = {
			{ "BRK", &BRK, &IMM, 7 },{ "ORA", &ORA, &IND_X, 6 },{ "???", &ILL, &IMP, 2 },{ "???", &ILL, &IMP, 8 },{ "???", &NOP, &IMP, 3 },{ "ORA", &ORA, &ZPG, 3 },{ "ASL", &ASL, &ZPG, 5 },{ "???", &ILL, &IMP, 5 },{ "PHP", &PHP, &IMP, 3 },{ "ORA", &ORA, &IMM, 2 },{ "ASL", &ASL, &IMP, 2 },{ "???", &ILL, &IMP, 2 },{ "???", &NOP, &IMP, 4 },{ "ORA", &ORA, &ABS, 4 },{ "ASL", &ASL, &ABS, 6 },{ "???", &ILL, &IMP, 6 },
			{ "BPL", &BPL, &REL, 2 },{ "ORA", &ORA, &IND_Y, 5 },{ "???", &ILL, &IMP, 2 },{ "???", &ILL, &IMP, 8 },{ "???", &NOP, &IMP, 4 },{ "ORA", &ORA, &ZPG_X, 4 },{ "ASL", &ASL, &ZPG_X, 6 },{ "???", &ILL, &IMP, 6 },{ "CLC", &CLC, &IMP, 2 },{ "ORA", &ORA, &ABS_Y, 4 },{ "???", &NOP, &IMP, 2 },{ "???", &ILL, &IMP, 7 },{ "???", &NOP, &IMP, 4 },{ "ORA", &ORA, &ABS_X, 4 },{ "ASL", &ASL, &ABS_X, 7 },{ "???", &ILL, &IMP, 7 },
			{ "JSR", &JSR, &ABS, 6 },{ "AND", &AND, &IND_X, 6 },{ "???", &ILL, &IMP, 2 },{ "???", &ILL, &IMP, 8 },{ "BIT", &BIT, &ZPG, 3 },{ "AND", &AND, &ZPG, 3 },{ "ROL", &ROL, &ZPG, 5 },{ "???", &ILL, &IMP, 5 },{ "PLP", &PLP, &IMP, 4 },{ "AND", &AND, &IMM, 2 },{ "ROL", &ROL, &IMP, 2 },{ "???", &ILL, &IMP, 2 },{ "BIT", &BIT, &ABS, 4 },{ "AND", &AND, &ABS, 4 },{ "ROL", &ROL, &ABS, 6 },{ "???", &ILL, &IMP, 6 },
			{ "BMI", &BMI, &REL, 2 },{ "AND", &AND, &IND_Y, 5 },{ "???", &ILL, &IMP, 2 },{ "???", &ILL, &IMP, 8 },{ "???", &NOP, &IMP, 4 },{ "AND", &AND, &ZPG_X, 4 },{ "ROL", &ROL, &ZPG_X, 6 },{ "???", &ILL, &IMP, 6 },{ "SEC", &SEC, &IMP, 2 },{ "AND", &AND, &ABS_Y, 4 },{ "???", &NOP, &IMP, 2 },{ "???", &ILL, &IMP, 7 },{ "???", &NOP, &IMP, 4 },{ "AND", &AND, &ABS_X, 4 },{ "ROL", &ROL, &ABS_X, 7 },{ "???", &ILL, &IMP, 7 },
			{ "RTI", &RTI, &IMP, 6 },{ "EOR", &EOR, &IND_X, 6 },{ "???", &ILL, &IMP, 2 },{ "???", &ILL, &IMP, 8 },{ "???", &NOP, &IMP, 3 },{ "EOR", &EOR, &ZPG, 3 },{ "LSR", &LSR, &ZPG, 5 },{ "???", &ILL, &IMP, 5 },{ "PHA", &PHA, &IMP, 3 },{ "EOR", &EOR, &IMM, 2 },{ "LSR", &LSR, &IMP, 2 },{ "???", &ILL, &IMP, 2 },{ "JMP", &JMP, &ABS, 3 },{ "EOR", &EOR, &ABS, 4 },{ "LSR", &LSR, &ABS, 6 },{ "???", &ILL, &IMP, 6 },
			{ "BVC", &BVC, &REL, 2 },{ "EOR", &EOR, &IND_Y, 5 },{ "???", &ILL, &IMP, 2 },{ "???", &ILL, &IMP, 8 },{ "???", &NOP, &IMP, 4 },{ "EOR", &EOR, &ZPG_X, 4 },{ "LSR", &LSR, &ZPG_X, 6 },{ "???", &ILL, &IMP, 6 },{ "CLI", &CLI, &IMP, 2 },{ "EOR", &EOR, &ABS_Y, 4 },{ "???", &NOP, &IMP, 2 },{ "???", &ILL, &IMP, 7 },{ "???", &NOP, &IMP, 4 },{ "EOR", &EOR, &ABS_X, 4 },{ "LSR", &LSR, &ABS_X, 7 },{ "???", &ILL, &IMP, 7 },
			{ "RTS", &RTS, &IMP, 6 },{ "ADC", &ADC, &IND_X, 6 },{ "???", &ILL, &IMP, 2 },{ "???", &ILL, &IMP, 8 },{ "???", &NOP, &IMP, 3 },{ "ADC", &ADC, &ZPG, 3 },{ "ROR", &ROR, &ZPG, 5 },{ "???", &ILL, &IMP, 5 },{ "PLA", &PLA, &IMP, 4 },{ "ADC", &ADC, &IMM, 2 },{ "ROR", &ROR, &IMP, 2 },{ "???", &ILL, &IMP, 2 },{ "JMP", &JMP, &IND, 5 },{ "ADC", &ADC, &ABS, 4 },{ "ROR", &ROR, &ABS, 6 },{ "???", &ILL, &IMP, 6 },
			{ "BVS", &BVS, &REL, 2 },{ "ADC", &ADC, &IND_Y, 5 },{ "???", &ILL, &IMP, 2 },{ "???", &ILL, &IMP, 8 },{ "???", &NOP, &IMP, 4 },{ "ADC", &ADC, &ZPG_X, 4 },{ "ROR", &ROR, &ZPG_X, 6 },{ "???", &ILL, &IMP, 6 },{ "SEI", &SEI, &IMP, 2 },{ "ADC", &ADC, &ABS_Y, 4 },{ "???", &NOP, &IMP, 2 },{ "???", &ILL, &IMP, 7 },{ "???", &NOP, &IMP, 4 },{ "ADC", &ADC, &ABS_X, 4 },{ "ROR", &ROR, &ABS_X, 7 },{ "???", &ILL, &IMP, 7 },
			{ "???", &NOP, &IMP, 2 },{ "STA", &STA, &IND_X, 6 },{ "???", &NOP, &IMP, 2 },{ "???", &ILL, &IMP, 6 },{ "STY", &STY, &ZPG, 3 },{ "STA", &STA, &ZPG, 3 },{ "STX", &STX, &ZPG, 3 },{ "???", &ILL, &IMP, 3 },{ "DEY", &DEY, &IMP, 2 },{ "???", &NOP, &IMP, 2 },{ "TXA", &TXA, &IMP, 2 },{ "???", &ILL, &IMP, 2 },{ "STY", &STY, &ABS, 4 },{ "STA", &STA, &ABS, 4 },{ "STX", &STX, &ABS, 4 },{ "???", &ILL, &IMP, 4 },
			{ "BCC", &BCC, &REL, 2 },{ "STA", &STA, &IND_Y, 6 },{ "???", &ILL, &IMP, 2 },{ "???", &ILL, &IMP, 6 },{ "STY", &STY, &ZPG_X, 4 },{ "STA", &STA, &ZPG_X, 4 },{ "STX", &STX, &ZPG_Y, 4 },{ "???", &ILL, &IMP, 4 },{ "TYA", &TYA, &IMP, 2 },{ "STA", &STA, &ABS_Y, 5 },{ "TXS", &TXS, &IMP, 2 },{ "???", &ILL, &IMP, 5 },{ "???", &NOP, &IMP, 5 },{ "STA", &STA, &ABS_X, 5 },{ "???", &ILL, &IMP, 5 },{ "???", &ILL, &IMP, 5 },
			{ "LDY", &LDY, &IMM, 2 },{ "LDA", &LDA, &IND_X, 6 },{ "LDX", &LDX, &IMM, 2 },{ "???", &ILL, &IMP, 6 },{ "LDY", &LDY, &ZPG, 3 },{ "LDA", &LDA, &ZPG, 3 },{ "LDX", &LDX, &ZPG, 3 },{ "???", &ILL, &IMP, 3 },{ "TAY", &TAY, &IMP, 2 },{ "LDA", &LDA, &IMM, 2 },{ "TAX", &TAX, &IMP, 2 },{ "???", &ILL, &IMP, 2 },{ "LDY", &LDY, &ABS, 4 },{ "LDA", &LDA, &ABS, 4 },{ "LDX", &LDX, &ABS, 4 },{ "???", &ILL, &IMP, 4 },
			{ "BCS", &BCS, &REL, 2 },{ "LDA", &LDA, &IND_Y, 5 },{ "???", &ILL, &IMP, 2 },{ "???", &ILL, &IMP, 5 },{ "LDY", &LDY, &ZPG_X, 4 },{ "LDA", &LDA, &ZPG_X, 4 },{ "LDX", &LDX, &ZPG_Y, 4 },{ "???", &ILL, &IMP, 4 },{ "CLV", &CLV, &IMP, 2 },{ "LDA", &LDA, &ABS_Y, 4 },{ "TSX", &TSX, &IMP, 2 },{ "???", &ILL, &IMP, 4 },{ "LDY", &LDY, &ABS_X, 4 },{ "LDA", &LDA, &ABS_X, 4 },{ "LDX", &LDX, &ABS_Y, 4 },{ "???", &ILL, &IMP, 4 },
			{ "CPY", &CPY, &IMM, 2 },{ "CMP", &CMP, &IND_X, 6 },{ "???", &NOP, &IMP, 2 },{ "???", &ILL, &IMP, 8 },{ "CPY", &CPY, &ZPG, 3 },{ "CMP", &CMP, &ZPG, 3 },{ "DEC", &DEC, &ZPG, 5 },{ "???", &ILL, &IMP, 5 },{ "INY", &INY, &IMP, 2 },{ "CMP", &CMP, &IMM, 2 },{ "DEX", &DEX, &IMP, 2 },{ "???", &ILL, &IMP, 2 },{ "CPY", &CPY, &ABS, 4 },{ "CMP", &CMP, &ABS, 4 },{ "DEC", &DEC, &ABS, 6 },{ "???", &ILL, &IMP, 6 },
			{ "BNE", &BNE, &REL, 2 },{ "CMP", &CMP, &IND_Y, 5 },{ "???", &ILL, &IMP, 2 },{ "???", &ILL, &IMP, 8 },{ "???", &NOP, &IMP, 4 },{ "CMP", &CMP, &ZPG_X, 4 },{ "DEC", &DEC, &ZPG_X, 6 },{ "???", &ILL, &IMP, 6 },{ "CLD", &CLD, &IMP, 2 },{ "CMP", &CMP, &ABS_Y, 4 },{ "NOP", &NOP, &IMP, 2 },{ "???", &ILL, &IMP, 7 },{ "???", &NOP, &IMP, 4 },{ "CMP", &CMP, &ABS_X, 4 },{ "DEC", &DEC, &ABS_X, 7 },{ "???", &ILL, &IMP, 7 },
			{ "CPX", &CPX, &IMM, 2 },{ "SBC", &SBC, &IND_X, 6 },{ "???", &NOP, &IMP, 2 },{ "???", &ILL, &IMP, 8 },{ "CPX", &CPX, &ZPG, 3 },{ "SBC", &SBC, &ZPG, 3 },{ "INC", &INC, &ZPG, 5 },{ "???", &ILL, &IMP, 5 },{ "INX", &INX, &IMP, 2 },{ "SBC", &SBC, &IMM, 2 },{ "NOP", &NOP, &IMP, 2 },{ "???", &SBC, &IMP, 2 },{ "CPX", &CPX, &ABS, 4 },{ "SBC", &SBC, &ABS, 4 },{ "INC", &INC, &ABS, 6 },{ "???", &ILL, &IMP, 6 },
			{ "BEQ", &BEQ, &REL, 2 },{ "SBC", &SBC, &IND_Y, 5 },{ "???", &ILL, &IMP, 2 },{ "???", &ILL, &IMP, 8 },{ "???", &NOP, &IMP, 4 },{ "SBC", &SBC, &ZPG_X, 4 },{ "INC", &INC, &ZPG_X, 6 },{ "???", &ILL, &IMP, 6 },{ "SED", &SED, &IMP, 2 },{ "SBC", &SBC, &ABS_Y, 4 },{ "NOP", &NOP, &IMP, 2 },{ "???", &ILL, &IMP, 7 },{ "???", &NOP, &IMP, 4 },{ "SBC", &SBC, &ABS_X, 4 },{ "INC", &INC, &ABS_X, 7 },{ "???", &ILL, &IMP, 7 }
		};

		// For my linux laptop
		instruction_entry instruction_table[0x0100] = {
			{ "BRK", &NES_Cpu::BRK, &NES_Cpu::IMM, 7 },{ "ORA", &NES_Cpu::ORA, &NES_Cpu::IND_X, 6 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 8 },{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 3 },{ "ORA", &NES_Cpu::ORA, &NES_Cpu::ZPG, 3 },{ "ASL", &NES_Cpu::ASL, &NES_Cpu::ZPG, 5 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 5 },{ "PHP", &NES_Cpu::PHP, &NES_Cpu::IMP, 3 },{ "ORA", &NES_Cpu::ORA, &NES_Cpu::IMM, 2 },{ "ASL", &NES_Cpu::ASL, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 4 },{ "ORA", &NES_Cpu::ORA, &NES_Cpu::ABS, 4 },{ "ASL", &NES_Cpu::ASL, &NES_Cpu::ABS, 6 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 6 },
			{ "BPL", &NES_Cpu::BPL, &NES_Cpu::REL, 2 },{ "ORA", &NES_Cpu::ORA, &NES_Cpu::IND_Y, 5 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 8 },{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 4 },{ "ORA", &NES_Cpu::ORA, &NES_Cpu::ZPG_X, 4 },{ "ASL", &NES_Cpu::ASL, &NES_Cpu::ZPG_X, 6 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 6 },{ "CLC", &NES_Cpu::CLC, &NES_Cpu::IMP, 2 },{ "ORA", &NES_Cpu::ORA, &NES_Cpu::ABS_Y, 4 },{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 7 },{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 4 },{ "ORA", &NES_Cpu::ORA, &NES_Cpu::ABS_X, 4 },{ "ASL", &NES_Cpu::ASL, &NES_Cpu::ABS_X, 7 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 7 },
			{ "JSR", &NES_Cpu::JSR, &NES_Cpu::ABS, 6 },{ "AND", &NES_Cpu::AND, &NES_Cpu::IND_X, 6 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 8 },{ "BIT", &NES_Cpu::BIT, &NES_Cpu::ZPG, 3 },{ "AND", &NES_Cpu::AND, &NES_Cpu::ZPG, 3 },{ "ROL", &NES_Cpu::ROL, &NES_Cpu::ZPG, 5 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 5 },{ "PLP", &NES_Cpu::PLP, &NES_Cpu::IMP, 4 },{ "AND", &NES_Cpu::AND, &NES_Cpu::IMM, 2 },{ "ROL", &NES_Cpu::ROL, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 2 },{ "BIT", &NES_Cpu::BIT, &NES_Cpu::ABS, 4 },{ "AND", &NES_Cpu::AND, &NES_Cpu::ABS, 4 },{ "ROL", &NES_Cpu::ROL, &NES_Cpu::ABS, 6 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 6 },
			{ "BMI", &NES_Cpu::BMI, &NES_Cpu::REL, 2 },{ "AND", &NES_Cpu::AND, &NES_Cpu::IND_Y, 5 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 8 },{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 4 },{ "AND", &NES_Cpu::AND, &NES_Cpu::ZPG_X, 4 },{ "ROL", &NES_Cpu::ROL, &NES_Cpu::ZPG_X, 6 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 6 },{ "SEC", &NES_Cpu::SEC, &NES_Cpu::IMP, 2 },{ "AND", &NES_Cpu::AND, &NES_Cpu::ABS_Y, 4 },{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 7 },{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 4 },{ "AND", &NES_Cpu::AND, &NES_Cpu::ABS_X, 4 },{ "ROL", &NES_Cpu::ROL, &NES_Cpu::ABS_X, 7 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 7 },
			{ "RTI", &NES_Cpu::RTI, &NES_Cpu::IMP, 6 },{ "EOR", &NES_Cpu::EOR, &NES_Cpu::IND_X, 6 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 8 },{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 3 },{ "EOR", &NES_Cpu::EOR, &NES_Cpu::ZPG, 3 },{ "LSR", &NES_Cpu::LSR, &NES_Cpu::ZPG, 5 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 5 },{ "PHA", &NES_Cpu::PHA, &NES_Cpu::IMP, 3 },{ "EOR", &NES_Cpu::EOR, &NES_Cpu::IMM, 2 },{ "LSR", &NES_Cpu::LSR, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 2 },{ "JMP", &NES_Cpu::JMP, &NES_Cpu::ABS, 3 },{ "EOR", &NES_Cpu::EOR, &NES_Cpu::ABS, 4 },{ "LSR", &NES_Cpu::LSR, &NES_Cpu::ABS, 6 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 6 },
			{ "BVC", &NES_Cpu::BVC, &NES_Cpu::REL, 2 },{ "EOR", &NES_Cpu::EOR, &NES_Cpu::IND_Y, 5 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 8 },{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 4 },{ "EOR", &NES_Cpu::EOR, &NES_Cpu::ZPG_X, 4 },{ "LSR", &NES_Cpu::LSR, &NES_Cpu::ZPG_X, 6 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 6 },{ "CLI", &NES_Cpu::CLI, &NES_Cpu::IMP, 2 },{ "EOR", &NES_Cpu::EOR, &NES_Cpu::ABS_Y, 4 },{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 7 },{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 4 },{ "EOR", &NES_Cpu::EOR, &NES_Cpu::ABS_X, 4 },{ "LSR", &NES_Cpu::LSR, &NES_Cpu::ABS_X, 7 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 7 },
			{ "RTS", &NES_Cpu::RTS, &NES_Cpu::IMP, 6 },{ "ADC", &NES_Cpu::ADC, &NES_Cpu::IND_X, 6 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 8 },{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 3 },{ "ADC", &NES_Cpu::ADC, &NES_Cpu::ZPG, 3 },{ "ROR", &NES_Cpu::ROR, &NES_Cpu::ZPG, 5 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 5 },{ "PLA", &NES_Cpu::PLA, &NES_Cpu::IMP, 4 },{ "ADC", &NES_Cpu::ADC, &NES_Cpu::IMM, 2 },{ "ROR", &NES_Cpu::ROR, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 2 },{ "JMP", &NES_Cpu::JMP, &NES_Cpu::IND, 5 },{ "ADC", &NES_Cpu::ADC, &NES_Cpu::ABS, 4 },{ "ROR", &NES_Cpu::ROR, &NES_Cpu::ABS, 6 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 6 },
			{ "BVS", &NES_Cpu::BVS, &NES_Cpu::REL, 2 },{ "ADC", &NES_Cpu::ADC, &NES_Cpu::IND_Y, 5 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 8 },{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 4 },{ "ADC", &NES_Cpu::ADC, &NES_Cpu::ZPG_X, 4 },{ "ROR", &NES_Cpu::ROR, &NES_Cpu::ZPG_X, 6 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 6 },{ "SEI", &NES_Cpu::SEI, &NES_Cpu::IMP, 2 },{ "ADC", &NES_Cpu::ADC, &NES_Cpu::ABS_Y, 4 },{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 7 },{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 4 },{ "ADC", &NES_Cpu::ADC, &NES_Cpu::ABS_X, 4 },{ "ROR", &NES_Cpu::ROR, &NES_Cpu::ABS_X, 7 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 7 },
			{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 2 },{ "STA", &NES_Cpu::STA, &NES_Cpu::IND_X, 6 },{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 6 },{ "STY", &NES_Cpu::STY, &NES_Cpu::ZPG, 3 },{ "STA", &NES_Cpu::STA, &NES_Cpu::ZPG, 3 },{ "STX", &NES_Cpu::STX, &NES_Cpu::ZPG, 3 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 3 },{ "DEY", &NES_Cpu::DEY, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 2 },{ "TXA", &NES_Cpu::TXA, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 2 },{ "STY", &NES_Cpu::STY, &NES_Cpu::ABS, 4 },{ "STA", &NES_Cpu::STA, &NES_Cpu::ABS, 4 },{ "STX", &NES_Cpu::STX, &NES_Cpu::ABS, 4 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 4 },
			{ "BCC", &NES_Cpu::BCC, &NES_Cpu::REL, 2 },{ "STA", &NES_Cpu::STA, &NES_Cpu::IND_Y, 6 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 6 },{ "STY", &NES_Cpu::STY, &NES_Cpu::ZPG_X, 4 },{ "STA", &NES_Cpu::STA, &NES_Cpu::ZPG_X, 4 },{ "STX", &NES_Cpu::STX, &NES_Cpu::ZPG_Y, 4 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 4 },{ "TYA", &NES_Cpu::TYA, &NES_Cpu::IMP, 2 },{ "STA", &NES_Cpu::STA, &NES_Cpu::ABS_Y, 5 },{ "TXS", &NES_Cpu::TXS, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 5 },{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 5 },{ "STA", &NES_Cpu::STA, &NES_Cpu::ABS_X, 5 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 5 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 5 },
			{ "LDY", &NES_Cpu::LDY, &NES_Cpu::IMM, 2 },{ "LDA", &NES_Cpu::LDA, &NES_Cpu::IND_X, 6 },{ "LDX", &NES_Cpu::LDX, &NES_Cpu::IMM, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 6 },{ "LDY", &NES_Cpu::LDY, &NES_Cpu::ZPG, 3 },{ "LDA", &NES_Cpu::LDA, &NES_Cpu::ZPG, 3 },{ "LDX", &NES_Cpu::LDX, &NES_Cpu::ZPG, 3 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 3 },{ "TAY", &NES_Cpu::TAY, &NES_Cpu::IMP, 2 },{ "LDA", &NES_Cpu::LDA, &NES_Cpu::IMM, 2 },{ "TAX", &NES_Cpu::TAX, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 2 },{ "LDY", &NES_Cpu::LDY, &NES_Cpu::ABS, 4 },{ "LDA", &NES_Cpu::LDA, &NES_Cpu::ABS, 4 },{ "LDX", &NES_Cpu::LDX, &NES_Cpu::ABS, 4 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 4 },
			{ "BCS", &NES_Cpu::BCS, &NES_Cpu::REL, 2 },{ "LDA", &NES_Cpu::LDA, &NES_Cpu::IND_Y, 5 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 5 },{ "LDY", &NES_Cpu::LDY, &NES_Cpu::ZPG_X, 4 },{ "LDA", &NES_Cpu::LDA, &NES_Cpu::ZPG_X, 4 },{ "LDX", &NES_Cpu::LDX, &NES_Cpu::ZPG_Y, 4 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 4 },{ "CLV", &NES_Cpu::CLV, &NES_Cpu::IMP, 2 },{ "LDA", &NES_Cpu::LDA, &NES_Cpu::ABS_Y, 4 },{ "TSX", &NES_Cpu::TSX, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 4 },{ "LDY", &NES_Cpu::LDY, &NES_Cpu::ABS_X, 4 },{ "LDA", &NES_Cpu::LDA, &NES_Cpu::ABS_X, 4 },{ "LDX", &NES_Cpu::LDX, &NES_Cpu::ABS_Y, 4 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 4 },
			{ "CPY", &NES_Cpu::CPY, &NES_Cpu::IMM, 2 },{ "CMP", &NES_Cpu::CMP, &NES_Cpu::IND_X, 6 },{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 8 },{ "CPY", &NES_Cpu::CPY, &NES_Cpu::ZPG, 3 },{ "CMP", &NES_Cpu::CMP, &NES_Cpu::ZPG, 3 },{ "DEC", &NES_Cpu::DEC, &NES_Cpu::ZPG, 5 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 5 },{ "INY", &NES_Cpu::INY, &NES_Cpu::IMP, 2 },{ "CMP", &NES_Cpu::CMP, &NES_Cpu::IMM, 2 },{ "DEX", &NES_Cpu::DEX, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 2 },{ "CPY", &NES_Cpu::CPY, &NES_Cpu::ABS, 4 },{ "CMP", &NES_Cpu::CMP, &NES_Cpu::ABS, 4 },{ "DEC", &NES_Cpu::DEC, &NES_Cpu::ABS, 6 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 6 },
			{ "BNE", &NES_Cpu::BNE, &NES_Cpu::REL, 2 },{ "CMP", &NES_Cpu::CMP, &NES_Cpu::IND_Y, 5 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 8 },{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 4 },{ "CMP", &NES_Cpu::CMP, &NES_Cpu::ZPG_X, 4 },{ "DEC", &NES_Cpu::DEC, &NES_Cpu::ZPG_X, 6 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 6 },{ "CLD", &NES_Cpu::CLD, &NES_Cpu::IMP, 2 },{ "CMP", &NES_Cpu::CMP, &NES_Cpu::ABS_Y, 4 },{ "NOP", &NES_Cpu::NOP, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 7 },{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 4 },{ "CMP", &NES_Cpu::CMP, &NES_Cpu::ABS_X, 4 },{ "DEC", &NES_Cpu::DEC, &NES_Cpu::ABS_X, 7 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 7 },
			{ "CPX", &NES_Cpu::CPX, &NES_Cpu::IMM, 2 },{ "SBC", &NES_Cpu::SBC, &NES_Cpu::IND_X, 6 },{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 8 },{ "CPX", &NES_Cpu::CPX, &NES_Cpu::ZPG, 3 },{ "SBC", &NES_Cpu::SBC, &NES_Cpu::ZPG, 3 },{ "INC", &NES_Cpu::INC, &NES_Cpu::ZPG, 5 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 5 },{ "INX", &NES_Cpu::INX, &NES_Cpu::IMP, 2 },{ "SBC", &NES_Cpu::SBC, &NES_Cpu::IMM, 2 },{ "NOP", &NES_Cpu::NOP, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::SBC, &NES_Cpu::IMP, 2 },{ "CPX", &NES_Cpu::CPX, &NES_Cpu::ABS, 4 },{ "SBC", &NES_Cpu::SBC, &NES_Cpu::ABS, 4 },{ "INC", &NES_Cpu::INC, &NES_Cpu::ABS, 6 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 6 },
			{ "BEQ", &NES_Cpu::BEQ, &NES_Cpu::REL, 2 },{ "SBC", &NES_Cpu::SBC, &NES_Cpu::IND_Y, 5 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 8 },{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 4 },{ "SBC", &NES_Cpu::SBC, &NES_Cpu::ZPG_X, 4 },{ "INC", &NES_Cpu::INC, &NES_Cpu::ZPG_X, 6 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 6 },{ "SED", &NES_Cpu::SED, &NES_Cpu::IMP, 2 },{ "SBC", &NES_Cpu::SBC, &NES_Cpu::ABS_Y, 4 },{ "NOP", &NES_Cpu::NOP, &NES_Cpu::IMP, 2 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 7 },{ "???", &NES_Cpu::NOP, &NES_Cpu::IMP, 4 },{ "SBC", &NES_Cpu::SBC, &NES_Cpu::ABS_X, 4 },{ "INC", &NES_Cpu::INC, &NES_Cpu::ABS_X, 7 },{ "???", &NES_Cpu::ILL, &NES_Cpu::IMP, 7 }
		};

		// All instruction functions
		int ADC();
		int AND();
		int ASL();
		int BCC();
		int BCS();
		int BEQ();
		int BIT();
		int BMI();
		int BNE();
		int BPL();
		int BRK();
		int BVC();
		int BVS();
		int CLC();
		int CLD();
		int CLI();
		int CLV();
		int CMP();
		int CPX();
		int CPY();
		int DEC();
		int DEX();
		int DEY();
		int EOR();
		int INC();
		int INX();
		int INY();
		int JMP();
		int JSR();
		int LDA();
		int LDX();
		int LDY();
		int LSR();
		int NOP();
		int ORA();
		int PHA();
		int PHP();
		int PLA();
		int PLP();
		int ROL();
		int ROR();
		int RTI();
		int RTS(); 
		int SBC();
		int SEC();
		int SED();
		int SEI();
		int STA();
		int STX();
		int STY();
		int TAX();
		int TAY();
		int TSX();
		int TXA();
		int TXS();
		int TYA();

		// Illegal instructions
		int ILL();

		// All addressing mode functions
		int ACC();
		int ABS();
		int ABS_X();
		int ABS_Y();
		int IMM();
		int IMP();
		int IND();
		int IND_X();
		int IND_Y();
		int REL();
		int ZPG();
		int ZPG_X();
		int ZPG_Y();


	public:

		// Initialization and Destruction functions
		NES_Cpu();
		~NES_Cpu();

		// Setup functions
		int load_cpu(uint8_t* reading_space, int size);

		// Interrupts
		void irq();										// Maskable Interrupt. Ignorable in certain cases
		void nmi();										// Non-Maskable Interrupt. Not ignorable
		void reset();									// System reset

		// Emulation
		void cycle();									// Run a cycle of the emulation

		// Debugging function
		void log();
};
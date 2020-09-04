# NES-Emulator
This is an NES emulator that I am working on during my free time. It is a work in progress, but you can see my work if you're interested.

I started this project soon after I finished my Chip-8 interpreter, which was a blast to work on. I plan on getting the emulator to a stage where I can run games such 
as Megaman 2, Super Mario Bros. and Castlevania.

## What is it?

<p align="center">
  <img width="70%" height="auto" src="https://i1.wp.com/www.theunheardnerd.com/wp-content/uploads/2016/09/NES-Famicom.png?resize=640%2C368&ssl=1">
</p>

The Nintendo Entertainment System (NES) was a home console, first released in 1983 as the Famicon and later renamed the as the NES in the western release. The NES used
a Ricoh 2A03 microprocessor that had a built in audio processing unit (APU) and had a custom picture-processing unit (PPU) for the graphics.

At the moment I am still working on emulating the CPU, which is essentially a MOS Technology 6502 core, meant to interpret disassembled 6502 assembly code. I don't have
explicit plans to implement the APU, but if I have time I can try working on it.

## References

A list of sites that I referred to for the project:

* http://wiki.nesdev.com/ - Main hub of NES Emulation information
* http://nesdev.com/NESDoc.pdf - Extremely useful document for getting into NES emulation by Patrick Diskin.
* http://www.emulator101.com/6502-addressing-modes.html - Quick summary of addressing modes and their practical applications
* https://www.masswerk.at/6502/6502_instruction_set.html - Table of instructions, pretty useful as they map out each opcode (in hexadecimal form) to their equivalent
assembly instruction, addressing mode and cycle timing
* https://www.youtube.com/watch?v=8XmxKPJDGU0&t=2962s - Great youtube channel that goes over emulating the NES. Used to understand general structure for NES emulator
code
* https://www.youtube.com/watch?v=fWqBmmPQP40 - Great presentation that goes over the MOS 6502 CPU, includes cycle counting and the general make of the CPU

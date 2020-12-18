#include <stdio.h>
#include <windows.h> 
#include <iostream>
#include <string>

#include "NES.h"

using namespace std;

#define WINDOW_WIDTH 256
#define WINDOW_HEIGHT 224
#define WINDOW_SCALE 1

// The CPU
NES_Cpu cpu;

int load(NES_Cpu* cpu, const char* game) {

	printf("Game: %s\n", game);

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

	// Reserve space to work on file
	uint8_t *buffer = (uint8_t *) malloc(sizeof(uint8_t) * size);
	int bytes_read = fread(buffer, 1, size, game_file);
	
	// Close the file
	fclose(game_file);

	// Check bytes read
	if (bytes_read != size) {
		printf("Reading error, expected to read %d bytes, but instead read %d\n", size, bytes_read);
		free(buffer);
		return 1;
	}

	// Print contents of the game file
	if (trace) {
		printf("Bytes Read: %d\nSize: %d\n", bytes_read, size);
		//print_hex(buffer, size);
	}

	// Map the bytes to the correct locations in memory
	int bytes_mapped = cpu->load_cpu(buffer, size);
	if (bytes_mapped <= 16) {
		printf("Error while loading ROM to the CPU, please report the bug or try a different ROM");
		free(buffer);
		return 1;
	}

	if (trace) {
		printf("CPU loading used the first %d byte(s) of ROM");
	}


	// TODO: Learn about PPU and initialize it here, continuing with bytes_mapped


	printf("Game loaded\n");

	free(buffer);

	return 0;
}

int main(int argc, char * argv[]) {

	// Get the path to the game
	char game_file[20];
	printf("Enter the game's name\n");
	cin >> game_file;

	int load_result = load(&cpu, game_file);

	return 0;
}

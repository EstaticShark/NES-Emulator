#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iomanip>
#include "NES.h"

// Debugging functions
int print_hex(uint8_t* data, int size) {

	if (size % 2 != 0) {
		printf("Irregular amount of bytes read");
		return 1;
	}

	for (int j = 0; j < 16; j++) {
		std::cout << std::hex << std::setfill('0') << std::setw(2) << j << " ";
	}

	std::cout << "\n";

	// Printing in little endian order, increasing sig. from x to x + 1
	for (int i = 0; i < size; i ++) {
		std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)data[i] << " ";
		if (i % 16 == 15) {
			printf("\n");
		}
	}

    printf("\n");
    
	return 0;
	
}
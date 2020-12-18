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

int main(int argc, char * argv[]) {

	/*
	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		printf("SDL_Init failed to intialize.\n");
		return 1;
	}

	// Create window
	SDL_Window *window = SDL_CreateWindow("Martin's NES Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH*WINDOW_SCALE, WINDOW_HEIGHT*WINDOW_SCALE, 0);
	if (window == NULL) { //Window has failed to create
		printf("SDL_CreateWindow failed to create a window.\n");
		SDL_Quit();
		return 1;
	}

	// Setup render
	Uint32 render_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, render_flags);
	if (renderer == NULL) {
		printf("SDL_CreateRenderer failed to create a renderer.\n");
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	// Black out the window
	//SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	//SDL_RenderClear(renderer);
	//SDL_RenderPresent(renderer);
	//SDL_SetRenderDrawColor(renderer, RED, BLUE, GREEN, 255);

	// Initialize the keyboard
	const Uint8 *keystate = SDL_GetKeyboardState(NULL);

	// Get the path to the game
	char game_file[20];
	printf("Enter the game's name\n");
	cin >> game_file;
	
	



	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	printf("SDL exit\n");
	*/


	// Get the path to the game
	char game_file[20];
	printf("Enter the game's name\n");
	cin >> game_file;

	int load_result = cpu.load(game_file);

	return 0;
}

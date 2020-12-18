CC = g++

all: compile

compile: 2A03.cpp Main.cpp NES.h util.h
	g++ -o NES Main.cpp 2A03.cpp util.h -I .

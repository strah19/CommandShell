SRC = $(wildcard src/*.cpp)

CC = g++

COMPILER_FLAGS = -Werror -Wfloat-conversion -ggdb -g -std=c++17

LINKER_FLAGS = 

ifeq ($(OS), Windows_NT)
	LINKER_FLAGS = -lmingw32 
else
	LINKER_FLAGS = -lm 
endif

EXEC_NAME = CommandShell

all : command_shell

command_shell : $(SRC) 
	 $(CC) $(SRC) $(INCLUDE_PATHS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(EXEC_NAME) 
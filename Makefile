Include = -I Include
COMP_FLAGS = -Wshadow -Winit-self -Wredundant-decls -Wcast-align -Wundef -Wfloat-equal -Winline -Wunreachable-code -Wmissing-declarations -Wmissing-include-dirs -Weffc++ -Wmain -Wextra -Wall -g -pipe -fexceptions -Wcast-qual -Wconversion -Wctor-dtor-privacy -Wempty-body -Wformat-security -Wformat=2 -Wignored-qualifiers -Wlogical-op -Wno-missing-field-initializers -Wnon-virtual-dtor -Woverloaded-virtual -Wpointer-arith -Wsign-promo -Wstack-usage=8192 -Wstrict-aliasing -Wstrict-null-sentinel -Wtype-limits -Wwrite-strings -Werror=vla -D_DEBUG -D_EJUDGE_CLIENT_SIDE
COMP_FLAGS += $(Include)
# COMP_FLAGS += -fsanitize=address
CC = g++
MK_FLAGS = -p
RM_FLAGS = -rf

Sources = $(wildcard $(Source)/*.cpp)
Objects = $(patsubst $(Source)/%.cpp,$(Build)/%.o,$(Sources))

Cpp = $(wildcard *.cpp)
H = $(wildcard *.h)

Build = Build
Source = Source
Exe = Compiler.exe

all: clean cls $(Build)/$(Exe)

cls:
	clear

$(Build)/%.o : $(Source)/%.cpp | $(Build)
	$(CC) $(COMP_FLAGS) $< -c -o $@

$(Build)/$(Exe): $(Objects) | $(Build)
	$(CC) $(COMP_FLAGS) -o $(Build)/$(Exe) $(Objects)

$(Build):
	mkdir $(Build)

asm:
	rm -rf lib
	mkdir lib
	nasm -f elf64 -fpie asm/IO.s -o lib/IO.o

cleanup:
ifdef Cpp
	mkdir  $(Source)
	move $(Cpp) ./$(Source)
endif
ifdef H
	mkdir Include
	move $(H) Include
endif
	remove -rf $(wildcard *.o)

run: all
	$(Build)/$(Exe)
	chmod 777 Build/Prog

clean:
	rm -rf $(Build)

test:
	$(Build)/Prog

debug:
	r2 -r Source/input_script.rr2 -d $(Build)/Prog




